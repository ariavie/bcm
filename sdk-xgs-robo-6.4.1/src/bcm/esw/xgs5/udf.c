/*
 * $Id: 7d5e035d6bb83ba114da7f5cd357fe4e4218b277 $
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
 * File:    udf.c
 * Purpose: Manages XGS5 UDF_TCAM and UDF_OFFSET tables
 */



#include <soc/mem.h>
#include <soc/field.h>

#include <soc/drv.h>
#include <soc/error.h>

#include <shared/bitop.h>

#include <bcm/udf.h>
#include <bcm/field.h>
#include <bcm/error.h>
#include <bcm/module.h>

#include <bcm_int/esw/udf.h>
#include <bcm_int/esw/switch.h>
#include <bcm_int/esw/field.h>

#define OUTER_IP4_TYPE_VAL(x) (SOC_IS_TD_TT(x) || SOC_IS_TRIUMPH3(x)) ? 1 : 2
#define OUTER_IP6_TYPE_VAL(x) (SOC_IS_TD_TT(x) || SOC_IS_TRIUMPH3(x)) ? 5 : 4

/* WarmBoot Sensitive Macro : Need to bump up scache version if changes */
#define TD2_UDF_MAX_OBJECTS 0xff

#define TD2_UDF_OFFSET_GRAN 0x2

#define UDF_MALLOC(_p_, _t_, _sz_, _desc_)                  \
    do {                                                    \
        if (!_p_) {                                         \
            (_p_) = (_t_ *) sal_alloc((_sz_), (_desc_));    \
        }                                                   \
        if (_p_) {                                          \
            sal_memset((_p_), 0, (_sz_));                   \
        }                                                   \
    } while(0)

#define UDF_UNLINK_OFFSET_NODE(_t_) \
    do {                                                    \
        if ((_t_)->prev) {                                   \
            (_t_)->prev->next = (_t_)->next;                 \
        } else {                                            \
            UDF_CTRL(unit)->offset_info_head = (_t_)->next; \
        }                                                   \
        if ((_t_)->next) {                                  \
            ((_t_)->next)->prev = (_t_)->prev;              \
        }                                                   \
    } while(0)

#define UDF_UNLINK_TCAM_NODE(_t_) \
    do {                                                    \
        if ((_t_)->prev) {                                   \
            (_t_)->prev->next = (_t_)->next;                 \
        } else {                                            \
            UDF_CTRL(unit)->tcam_info_head = (_t_)->next;   \
        }                                                   \
        if ((_t_)->next) {                                  \
            ((_t_)->next)->prev = (_t_)->prev;              \
        }                                                   \
    } while(0)

#define BCM_IF_NULL_RETURN_PARAM(_p_) \
    do {                                                    \
        if ((_p_) == NULL) {                                \
            return BCM_E_PARAM;                             \
        }                                                   \
    } while(0)


#define UDF_IF_ERROR_CLEANUP(_rv_) \
    do {                                                    \
        if (BCM_FAILURE(rv)) {                              \
            goto cleanup;                                   \
        }                                                   \
    } while (0)

#define UDF_ID_VALIDATE(_id_) \
    do {                                                    \
        if (((_id_) < BCMI_XGS5_UDF_ID_MIN) ||              \
            ((_id_) > BCMI_XGS5_UDF_ID_MAX)) {              \
            return BCM_E_PARAM;                             \
        }                                                   \
    } while(0)

#define UDF_PKT_FORMAT_ID_VALIDATE(_id_) \
    do {                                                    \
        if (((_id_) < BCMI_XGS5_UDF_PKT_FORMAT_ID_MIN) ||   \
            ((_id_) > BCMI_XGS5_UDF_PKT_FORMAT_ID_MAX)) {   \
            return BCM_E_PARAM;                             \
        }                                                   \
    } while(0)


/* Global UDF control structure pointers */
bcmi_xgs5_udf_ctrl_t *udf_control[BCM_MAX_NUM_UNITS];

typedef enum {
    bcmiUdfObjectUdf = 1,
    bcmiUdfObjectPktFormat = 2
} bcmiUdfObjectType;

typedef enum bcmi_xgs5_udf_pkt_format_misc_e {
    bcmiUdfPktFormatHigig = 0,
    bcmiUdfPktFormatCntag = 1,
    bcmiUdfPktFormatIcnm = 2,
    bcmiUdfPktFormatVntag = 3,
    bcmiUdfPktFormatEtag = 4,
    bcmiUdfPktFormatSubportTag = 5,
    bcmiUdfPktFormatNone
} bcmi_xgs5_udf_pkt_format_misc_t;



/* STATIC function declarations */
STATIC int bcmi_xgs5_udf_ctrl_init(int unit);

STATIC int bcmi_xgs5_udf_hw_init(int unit);

STATIC int bcmi_xgs5_udf_offset_reserve(int unit, int max, int offset[]);

STATIC int bcmi_xgs5_udf_offset_unreserve(int unit, int max, int offset[]);

STATIC int bcmi_xgs5_udf_offset_hw_alloc(int unit,
    bcm_udf_alloc_hints_t *hints,
    bcmi_xgs5_udf_offset_info_t *offset_info);

STATIC int bcmi_xgs5_udf_layer_to_offset_base(int unit,
    bcmi_xgs5_udf_offset_info_t *offset_info,
    bcmi_xgs5_udf_tcam_info_t *tcam_info,
    int *base, int *offset);

STATIC int bcmi_xgs5_udf_offset_info_alloc(int unit,
    bcmi_xgs5_udf_offset_info_t **offset_info);

STATIC int bcmi_xgs5_udf_offset_info_add(int unit,
    bcm_udf_t *udf_info, bcmi_xgs5_udf_offset_info_t **offset_info);

STATIC int bcmi_xgs5_udf_offset_node_add(
    int unit, bcmi_xgs5_udf_offset_info_t *new);

STATIC int bcmi_xgs5_udf_offset_node_delete(int unit,
    bcm_udf_id_t udf_id, bcmi_xgs5_udf_offset_info_t **del);

STATIC int bcmi_xgs5_udf_tcam_info_alloc(int unit,
    bcmi_xgs5_udf_tcam_info_t **tcam_info);

STATIC int bcmi_xgs5_udf_tcam_info_add(int unit,
    bcm_udf_pkt_format_info_t *pkt_format,
    bcmi_xgs5_udf_tcam_info_t **tcam_info);

STATIC int bcmi_xgs5_udf_tcam_node_add(
    int unit, bcmi_xgs5_udf_tcam_info_t *new);

STATIC int bcmi_xgs5_udf_tcam_node_delete(int unit,
    bcm_udf_pkt_format_id_t pkt_format_id, bcmi_xgs5_udf_tcam_info_t **del);

STATIC int bcmi_xgs5_udf_id_running_id_alloc(int unit,
    bcmiUdfObjectType type, int *id);

STATIC int bcmi_xgs5_udf_offset_to_hw_field(int unit, int offset,
    soc_field_t *base_hw_f, soc_field_t *offset_hw_f);

STATIC int bcmi_xgs5_udf_offset_install(int unit,
    int e, uint32 hw_bmap, int base, int offset);

STATIC int bcmi_xgs5_udf_tcam_entry_move(int unit,
    bcmi_xgs5_udf_tcam_entry_t *tcam_entry_arr, int src, int dst);

STATIC int bcmi_xgs5_udf_tcam_move_up(int unit,
    bcmi_xgs5_udf_tcam_entry_t *tcam_entry_arr, int dest, int free_slot);

STATIC int bcmi_xgs5_udf_tcam_move_down(int unit,
    bcmi_xgs5_udf_tcam_entry_t *tcam_entry_arr, int dest, int free_slot);

STATIC int bcmi_xgs5_udf_tcam_entry_match(int unit,
    bcmi_xgs5_udf_tcam_info_t *new,  bcmi_xgs5_udf_tcam_info_t **match);

STATIC int bcmi_xgs5_udf_tcam_entry_insert(int unit,
    bcmi_xgs5_udf_tcam_info_t *tcam_new);

STATIC int bcmi_xgs5_udf_tcam_entry_vlanformat_parse(int unit,
    uint32 *hw_buf, bcm_udf_pkt_format_info_t *pkt_fmt);

STATIC int bcmi_xgs5_udf_tcam_entry_l2format_parse(int unit,
    uint32 *hw_buf, bcm_udf_pkt_format_info_t *pkt_fmt);

STATIC int bcmi_xgs5_udf_tcam_entry_l3_parse(int unit,
    uint32 *hw_buf, bcm_udf_pkt_format_info_t *pkt_fmt);

STATIC int bcmi_xgs5_udf_tcam_entry_misc_parse(int unit,
    int type, uint32 *hw_buf, uint16 *flags);

STATIC int bcmi_xgs5_udf_tcam_entry_vlanformat_init(int unit,
    bcm_udf_pkt_format_info_t *pkt_fmt, uint32 *hw_buf);

STATIC int bcmi_xgs5_udf_tcam_entry_l2format_init(int unit,
    bcm_udf_pkt_format_info_t *pkt_fmt, uint32 *hw_buf);

STATIC int bcmi_xgs5_udf_tcam_entry_l3_init(int unit,
    bcm_udf_pkt_format_info_t *pkt_fmt, uint32 *hw_buf);

STATIC int bcmi_xgs5_udf_tcam_entry_misc_init(int unit,
    int type, uint16 flags, uint32 *hw_buf);

STATIC int bcmi_xgs5_udf_tcam_entry_parse(int unit,
    uint32 *hw_buf, bcm_udf_pkt_format_info_t *pkt_fmt);

STATIC int bcmi_xgs5_udf_pkt_format_tcam_key_init(int unit,
    bcm_udf_pkt_format_info_t *pkt_format, uint32 *hw_buf);

STATIC int bcmi_xgs5_udf_tcam_misc_format_to_hw_fields(int unit, int type,
    soc_field_t *data_f, soc_field_t *mask_f,
    int *present, int *none, int *any, int *support);



/*
 * Function:
 *      bcmi_xgs5_udf_ctrl_free
 * Purpose:
 *      Frees udf control structure and its members.
 * Parameters:
 *      unit            - (IN)  Unit number.
 *      udfc            - (IN)  UDF Control structure to free.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
STATIC int
bcmi_xgs5_udf_ctrl_free(int unit, bcmi_xgs5_udf_ctrl_t *udfc)
{
    bcmi_xgs5_udf_tcam_info_t *prv_t, *cur_t;
    bcmi_xgs5_udf_offset_info_t *prv_o, *cur_o;

    if (NULL == udfc) {
        /* Already freed */
        return BCM_E_NONE;
    }

    /* Free mutex */
    if (NULL != udfc->udf_mutex) {
        sal_mutex_destroy(udfc->udf_mutex);
        udfc->udf_mutex = NULL;
    }

    /* Free tcam entry array */
    if (NULL != udfc->tcam_entry_array) {
        sal_free(udfc->tcam_entry_array);
    }

    /* Free offset entry array */
    if (NULL != udfc->offset_entry_array) {
        sal_free(udfc->offset_entry_array);
    }

    /* Free list of pkt format objects */
    cur_o = udfc->offset_info_head;

    while (NULL != cur_o) {
        prv_o = cur_o;
        cur_o = cur_o->next;

        sal_free(prv_o);
    }
    udfc->offset_info_head = NULL;

    /* Free list of offset objects */
    cur_t = udfc->tcam_info_head;

    while (NULL != cur_t) {
        prv_t = cur_t;
        cur_t = cur_t->next;

        sal_free(prv_t);
    }
    udfc->tcam_info_head = NULL;

    /* Free the udf control structure */
    sal_free(udfc);

    udf_control[unit] = NULL;

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcmi_xgs5_udf_ctrl_init
 * Purpose:
 *      Initialize UDF control and internal data structures.
 * Parameters:
 *      unit           - (IN) bcm device.
 *
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
bcmi_xgs5_udf_ctrl_init(int unit)
{
    int alloc_sz;
    soc_mem_t tcam_mem, offset_mem;
    int nentries, noffsets;
    bcmi_xgs5_udf_ctrl_t *udf_ctrl = NULL;

    tcam_mem = FP_UDF_TCAMm;
    offset_mem = FP_UDF_OFFSETm;

    nentries = soc_mem_index_count(unit, tcam_mem);
    if (SOC_MEM_FIELD_VALID(unit, offset_mem, UDF1_OFFSET4f)) {
        noffsets = 16;
    } else {
        noffsets = 8;
    }

   /* Allocating memory of UDF control structure */
    alloc_sz = sizeof(bcmi_xgs5_udf_ctrl_t);
    UDF_MALLOC(udf_ctrl, bcmi_xgs5_udf_ctrl_t, alloc_sz, "udf control");
    if (udf_ctrl == NULL) {
        return BCM_E_MEMORY;
    }

    /* Allocate memory for tcam_entry_array structure */
    alloc_sz = sizeof(bcmi_xgs5_udf_tcam_entry_t) * nentries;
    UDF_MALLOC(udf_ctrl->tcam_entry_array, bcmi_xgs5_udf_tcam_entry_t,
               alloc_sz, "udf tcam entry array");
    if (udf_ctrl->tcam_entry_array == NULL) {
        /* Free udf control struct */
        (void)bcmi_xgs5_udf_ctrl_free(unit, udf_ctrl);
        return BCM_E_MEMORY;
    }

    /* Allocate memory for offset_entry_array structure */
    alloc_sz = sizeof(bcmi_xgs5_udf_offset_entry_t) * noffsets;
    UDF_MALLOC(udf_ctrl->offset_entry_array, bcmi_xgs5_udf_offset_entry_t,
               alloc_sz, "udf offset entry array");
    if (udf_ctrl->offset_entry_array == NULL) {
        /* Free udf control struct */
        (void)bcmi_xgs5_udf_ctrl_free(unit, udf_ctrl);
        return BCM_E_MEMORY;
    }


    /* Create UDF lock */
    if (udf_ctrl->udf_mutex == NULL) {
        udf_ctrl->udf_mutex = sal_mutex_create("udf_mutex");
        if (udf_ctrl->udf_mutex == NULL) {
            /* Free udf control struct */
            (void)bcmi_xgs5_udf_ctrl_free(unit, udf_ctrl);
            return BCM_E_MEMORY;
        }
    }


    udf_ctrl->tcam_mem = tcam_mem;
    udf_ctrl->offset_mem = offset_mem;

    /* Check if GRE options adjust for UDF selection is supported */
    if (SOC_MEM_FIELD_VALID(unit, offset_mem, UDF1_ADD_GRE_OPTIONS0f)) {
        udf_ctrl->flags |= BCMI_XGS5_UDF_CTRL_OFFSET_ADJUST_GRE;
    }

    /* Check if IPV4 options adjust for UDF selection is supported */
    if (SOC_MEM_FIELD_VALID(unit, offset_mem, UDF1_ADD_IPV4_OPTIONS0f)) {
        udf_ctrl->flags |= BCMI_XGS5_UDF_CTRL_OFFSET_ADJUST_IP4;
    }

    if (SOC_MEM_FIELD_VALID(unit, tcam_mem, HIGIGf)) {
        udf_ctrl->flags |= BCMI_XGS5_UDF_CTRL_TCAM_HIGIG;
    }
    if (SOC_MEM_FIELD_VALID(unit, tcam_mem, VNTAG_PRESENTf)) {
        udf_ctrl->flags |= BCMI_XGS5_UDF_CTRL_TCAM_VNTAG;
    }
    if (SOC_MEM_FIELD_VALID(unit, tcam_mem, ETAG_PACKETf)) {
        udf_ctrl->flags |= BCMI_XGS5_UDF_CTRL_TCAM_ETAG;
    }
    if (SOC_MEM_FIELD_VALID(unit, tcam_mem, CNTAG_PRESENTf)) {
        udf_ctrl->flags |= BCMI_XGS5_UDF_CTRL_TCAM_CNTAG;
    }
    if (SOC_MEM_FIELD_VALID(unit, tcam_mem, ICNM_PACKETf)) {
        udf_ctrl->flags |= BCMI_XGS5_UDF_CTRL_TCAM_ICNM;
    }
    if (SOC_MEM_FIELD_VALID(unit, tcam_mem, SUBPORT_TAG_PRESENTf)) {
        udf_ctrl->flags |= BCMI_XGS5_UDF_CTRL_TCAM_SUBPORT_TAG;
    }

    udf_ctrl->nentries = nentries;
    udf_ctrl->noffsets = noffsets;

    /* restriction merely for warmboot purposes */
    udf_ctrl->max_udfs = TD2_UDF_MAX_OBJECTS;

    /* TD2 offset chunk granularity */
    udf_ctrl->gran = TD2_UDF_OFFSET_GRAN;

    udf_control[unit] = udf_ctrl;

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcmi_xgs5_udf_hw_init
 * Purpose:
 *      Clears the hardware tables related to UDF.
 * Parameters:
 *      unit            - (IN)  Unit number.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
STATIC int
bcmi_xgs5_udf_hw_init(int unit)
{
    int rv;

    /* Clear UDF_TCAM table */
    rv = soc_mem_clear(unit, UDF_CTRL(unit)->tcam_mem, MEM_BLOCK_ALL, TRUE);
    SOC_IF_ERROR_RETURN(rv);

    /* Clear UDF_OFFSET table */
    rv = soc_mem_clear(unit, UDF_CTRL(unit)->offset_mem, MEM_BLOCK_ALL, TRUE);
    SOC_IF_ERROR_RETURN(rv);


    return BCM_E_NONE;
}


#if defined (BCM_WARM_BOOT_SUPPORT)

#define BCM_WB_VERSION_1_0     SOC_SCACHE_VERSION(1, 0)
#define BCM_WB_DEFAULT_VERSION BCM_WB_VERSION_1_0

/* Data structure for UDF Offset entry */
struct bcmi_xgs5_wb_offset_entry_1_0 {
    uint8 flags;
    uint8 grp_id;
    uint8 num_pkt_formats;
};

/* Data structure for UDF offset info */
struct bcmi_xgs5_wb_offset_info_1_0 {

    uint8 layer;
    uint8 start;
    uint8 width;
    uint8 flags;

    uint16 num_pkt_formats;

    uint16 id;
    uint32 byte_bmap;
};

/* Data structure for UDF tcam info */
struct bcmi_xgs5_wb_tcam_info_1_0 {
    uint16 hw_id;
    uint16 num_udfs;

    uint16 id;
    uint16 priority;

    uint16 udf_id_list[MAX_UDF_OFFSET_CHUNKS];
};

/* Returns required scache size in bytes for version = 1_0 */
STATIC int
bcmi_xgs5_udf_wb_scache_size_get_1_0(int unit, int *req_scache_size)
{
    int alloc_size = 0;
    bcmi_xgs5_udf_ctrl_t *udf_ctrl;

    udf_ctrl = UDF_CTRL(unit);

    /* offset entries */
    alloc_size += (udf_ctrl->noffsets *
                   sizeof (struct bcmi_xgs5_wb_offset_entry_1_0));

    /* offset nodes */
    alloc_size += (udf_ctrl->max_udfs *
                   sizeof(struct bcmi_xgs5_wb_offset_info_1_0));

    /* tcam nodes */
    alloc_size += (udf_ctrl->nentries *
                   sizeof(struct bcmi_xgs5_wb_tcam_info_1_0));

    *req_scache_size = alloc_size;

    return BCM_E_NONE;
}

/* Recovers offsets info nodes from scache for version = 1_0 */
STATIC int
bcmi_xgs5_udf_wb_offset_info_reinit_1_0(int unit, int num_offset_nodes,
                                        uint8 **scache_ptr)
{
    int rv;
    int i;
    int val = 0;
    int offset_reserve[MAX_UDF_OFFSET_CHUNKS] = {0};
    int gran, max_chunks;
    bcm_udf_t udf_info;
    bcmi_xgs5_udf_offset_info_t *offset_info;
    struct bcmi_xgs5_wb_offset_info_1_0 *scache_offset_p;

    gran = BCMI_XGS5_UDF_CTRL_OFFSET_GRAN(unit);
    max_chunks = BCMI_XGS5_UDF_CTRL_MAX_UDF_CHUNKS(unit);

    scache_offset_p = (struct bcmi_xgs5_wb_offset_info_1_0 *)(*scache_ptr);

    /* Recover offset nodes from scache */
    while (num_offset_nodes > 0) {

        udf_info.start = scache_offset_p->start;
        udf_info.width = scache_offset_p->width;
        udf_info.layer = scache_offset_p->layer;

        rv = bcmi_xgs5_udf_offset_info_add(unit, &udf_info, &offset_info);
        BCM_IF_ERROR_RETURN(rv);

        offset_info->udf_id = scache_offset_p->id;
        offset_info->flags = scache_offset_p->flags;
        offset_info->num_pkt_formats = scache_offset_p->num_pkt_formats;
        offset_info->byte_bmap = scache_offset_p->byte_bmap;

        if (offset_info->udf_id > UDF_CTRL(unit)->udf_id_running) {
            UDF_CTRL(unit)->udf_id_running = offset_info->udf_id;
        }

        for (i = 0; i < max_chunks; i++) {
            SHR_BITTEST_RANGE(&(scache_offset_p->byte_bmap),
                               (i * gran), gran, val);
            offset_reserve[i] = val;
            if (val) {
                SHR_BITSET(&(offset_info->hw_bmap), i);
            }
        }

        /* Mark the global bitmap as used */
        (void) bcmi_xgs5_udf_offset_reserve(unit, max_chunks, offset_reserve);

        scache_offset_p++;
        num_offset_nodes--;
    }

    *scache_ptr = (uint8 *)scache_offset_p;

    return BCM_E_NONE;
}

/* Recovers tcam info nodes from scache for version = 1_0 */

STATIC int
bcmi_xgs5_udf_wb_tcam_info_reinit_1_0(int unit, int num_tcam_nodes,
                                      uint8 **scache_ptr)
{
    uint32 *buffer;      /* Hw buffer to dma udf tcam. */
    uint32 *entry_ptr = 0;   /* Tcam entry pointer.    */
    int alloc_size;      /* Memory allocation size.    */
    int entry_size;      /* Single tcam entry size.    */
    soc_mem_t tcam_mem;  /* UDF_TCAM memory            */
    int idx;             /* Tcam entries iterator.     */
    int rv;              /* Operation return status.   */
    int udf_id;
    bcmi_xgs5_udf_offset_info_t *offset_info;
    bcmi_xgs5_udf_tcam_info_t *tcam_info = NULL;
    fp_udf_tcam_entry_t *tcam_buf;
    struct bcmi_xgs5_wb_tcam_info_1_0 *scache_tcam_p;

    tcam_mem = UDF_CTRL(unit)->tcam_mem;

    entry_size = sizeof(fp_udf_tcam_entry_t);
    alloc_size = SOC_MEM_TABLE_BYTES(unit, tcam_mem);

    /* Allocate memory buffer. */
    buffer = soc_cm_salloc(unit, alloc_size, "Udf tcam");
    if (buffer == NULL) {
        return (BCM_E_MEMORY);
    }

    sal_memset(buffer, 0, alloc_size);

    /* Read table to the buffer. */
    rv = soc_mem_read_range(unit, tcam_mem, MEM_BLOCK_ANY,
                            soc_mem_index_min(unit, tcam_mem),
                            soc_mem_index_max(unit, tcam_mem), buffer);
    if (BCM_FAILURE(rv)) {
        soc_cm_sfree(unit, buffer);
        return (BCM_E_INTERNAL);
    }



    scache_tcam_p = (struct bcmi_xgs5_wb_tcam_info_1_0 *)(*scache_ptr);

    /* Recover tcam info from scache */
    while (num_tcam_nodes > 0) {

        tcam_info = sal_alloc(sizeof(bcmi_xgs5_udf_tcam_info_t), "tcam info");

        idx = scache_tcam_p->hw_id;
        tcam_info->hw_idx = idx;
        tcam_info->pkt_format_id = scache_tcam_p->id;
        tcam_info->priority = scache_tcam_p->priority;
        tcam_info->num_udfs = scache_tcam_p->num_udfs;

        entry_ptr = soc_mem_table_idx_to_pointer(unit, tcam_mem, uint32 *,
                                                 buffer, idx);

        tcam_buf = (fp_udf_tcam_entry_t *)&(tcam_info->hw_buf);
        sal_memcpy(tcam_buf, entry_ptr, entry_size);

        if (soc_mem_field32_get(unit, tcam_mem, entry_ptr, VALIDf)) {
            (UDF_CTRL(unit)->tcam_entry_array[idx]).valid = 1;
            ((UDF_CTRL(unit)->tcam_entry_array[idx]).tcam_info) = tcam_info;

        }

        /* Not installed entries will have 512 as hw_idx */
        if (idx < MAX_UDF_TCAM_ENTRIES) {

            for (idx = 0; idx < MAX_UDF_OFFSET_CHUNKS; idx++) {

                udf_id = scache_tcam_p->udf_id_list[idx];

                if (udf_id != 0) {
                    rv = bcmi_xgs5_udf_offset_node_get(unit, udf_id,
                                                       &offset_info);
                    /* Continue recovering other udfs added to the tcam entry */
                    if (BCM_SUCCESS(rv)) {
                        tcam_info->offset_info_list[offset_info->grp_id] =
                                                            offset_info;
                    }
                }
            }
            if (tcam_info->pkt_format_id >
                UDF_CTRL(unit)->udf_pkt_format_id_running) {

                UDF_CTRL(unit)->udf_pkt_format_id_running =
                    tcam_info->pkt_format_id;
            }

        }

        /* Continue recovering other packet format entries even on failure */
        (void) bcmi_xgs5_udf_tcam_node_add(unit, tcam_info);

        scache_tcam_p++;
        num_tcam_nodes--;
    }

    *scache_ptr = (uint8 *)scache_tcam_p;

    soc_cm_sfree(unit, buffer);

    return BCM_E_NONE;
}


/* Syncs offset nodes info into scache for version = 1_0 */
STATIC int
bcmi_xgs5_udf_wb_offset_info_sync_1_0(int unit, int num_offset_nodes,
                                      uint8 **scache_ptr)
{
    bcmi_xgs5_udf_offset_info_t *offset_info;
    struct bcmi_xgs5_wb_offset_info_1_0 *scache_offset_p;

    scache_offset_p = (struct bcmi_xgs5_wb_offset_info_1_0 *)(*scache_ptr);

    offset_info = UDF_CTRL(unit)->offset_info_head;

    /* Recover offset nodes from scache */
    while (num_offset_nodes > 0) {

        scache_offset_p->layer = offset_info->layer;
        scache_offset_p->start = offset_info->start;
        scache_offset_p->width = offset_info->width;
        scache_offset_p->flags = offset_info->flags;

        scache_offset_p->num_pkt_formats = offset_info->num_pkt_formats;

        scache_offset_p->id = offset_info->udf_id;
        scache_offset_p->byte_bmap = offset_info->byte_bmap;

        num_offset_nodes--;
        scache_offset_p++;
        offset_info = offset_info->next;
    }

    *scache_ptr = (uint8 *)scache_offset_p;

    return BCM_E_NONE;
}

/* Syncs tcam nodes info into scache for version = 1_0 */
STATIC int
bcmi_xgs5_udf_wb_tcam_info_sync_1_0(int unit, int num_tcam_nodes,
                                    uint8 **scache_ptr)
{
    int idx; 
    bcmi_xgs5_udf_tcam_info_t *tcam_info;
    struct bcmi_xgs5_wb_tcam_info_1_0 *scache_tcam_p;


    tcam_info = UDF_CTRL(unit)->tcam_info_head;
    scache_tcam_p = (struct bcmi_xgs5_wb_tcam_info_1_0 *)(*scache_ptr);

    /* Sync data to scache */
    while (num_tcam_nodes > 0) {

        idx = scache_tcam_p->hw_id;
        scache_tcam_p->hw_id = tcam_info->hw_idx;
        scache_tcam_p->id = tcam_info->pkt_format_id;
        scache_tcam_p->priority = tcam_info->priority;
        scache_tcam_p->num_udfs = tcam_info->num_udfs;

        for (idx = 0; idx < MAX_UDF_OFFSET_CHUNKS; idx++) {
            if (tcam_info->offset_info_list[idx] != NULL) {
                scache_tcam_p->udf_id_list[idx] =
                    (tcam_info->offset_info_list[idx])->udf_id;
            } else {
                scache_tcam_p->udf_id_list[idx] = 0;
            }
        }

        scache_tcam_p++;
        num_tcam_nodes--;
        tcam_info = tcam_info->next;
    }

    *scache_ptr = (uint8 *)scache_tcam_p;

    return BCM_E_NONE;
}


/* Recovers UDF control state from scache for version = 1_0 */
STATIC int
bcmi_xgs5_udf_wb_reinit_1_0(int unit, uint8 **scache_ptr)
{
    int rv;
    int i;
    int max_chunks;
    bcmi_xgs5_udf_offset_entry_t *offset_entry;
    struct bcmi_xgs5_wb_offset_entry_1_0 *offset_entry_scache_p;
    uint32 *u32_scache_p;

    max_chunks = UDF_CTRL(unit)->noffsets;

    u32_scache_p = (uint32 *)(*scache_ptr);

    /* Number of UDFs created */
    UDF_CTRL(unit)->num_udfs = *u32_scache_p;
    u32_scache_p++;

    /* Number of Packet formats created */
    UDF_CTRL(unit)->num_pkt_formats = *u32_scache_p;
    u32_scache_p++;

    offset_entry_scache_p =
        (struct bcmi_xgs5_wb_offset_entry_1_0 *)u32_scache_p;

    for (i = 0; i < max_chunks; i++) {
        offset_entry = &(UDF_CTRL(unit)->offset_entry_array[i]);

        offset_entry->flags = offset_entry_scache_p->flags;
        offset_entry->grp_id = offset_entry_scache_p->grp_id;
        offset_entry->num_pkt_formats = offset_entry_scache_p->num_pkt_formats;

        offset_entry_scache_p++;
    }
    *scache_ptr = (uint8 *)offset_entry_scache_p;

    rv = bcmi_xgs5_udf_wb_offset_info_reinit_1_0(unit,
                 UDF_CTRL(unit)->num_udfs, scache_ptr);
    BCM_IF_ERROR_RETURN(rv);

    rv = bcmi_xgs5_udf_wb_tcam_info_reinit_1_0(unit,
                UDF_CTRL(unit)->num_pkt_formats, scache_ptr);
    BCM_IF_ERROR_RETURN(rv);

    return BCM_E_NONE;
}


/* Syncs UDF control state into scache for version = 1_0 */
STATIC int
bcmi_xgs5_udf_wb_sync_1_0(int unit, uint8 **scache_ptr)
{
    int rv;
    int i;
    int max_chunks;
    struct bcmi_xgs5_wb_offset_entry_1_0 *offset_entry_scache_p;
    uint32 *u32_scache_p;

    max_chunks = UDF_CTRL(unit)->noffsets;

    u32_scache_p = (uint32 *)(*scache_ptr);

    /* Number of UDFs created */
    *u32_scache_p = UDF_CTRL(unit)->num_udfs;
    u32_scache_p++;

    /* Number of Packet formats created */
    *u32_scache_p = UDF_CTRL(unit)->num_pkt_formats;
    u32_scache_p++;

    offset_entry_scache_p =
        (struct bcmi_xgs5_wb_offset_entry_1_0 *)u32_scache_p;

    for (i = 0; i < max_chunks; i++) {
        offset_entry_scache_p->flags =
                (UDF_CTRL(unit)->offset_entry_array[i]).flags;
        offset_entry_scache_p->grp_id =
                (UDF_CTRL(unit)->offset_entry_array[i]).grp_id;
        offset_entry_scache_p->num_pkt_formats =
                (UDF_CTRL(unit)->offset_entry_array[i]).num_pkt_formats;

        offset_entry_scache_p++;
    }

    *scache_ptr = (uint8 *)offset_entry_scache_p;

    rv = bcmi_xgs5_udf_wb_offset_info_sync_1_0(unit,
                UDF_CTRL(unit)->num_udfs, scache_ptr);
    BCM_IF_ERROR_RETURN(rv);

    rv = bcmi_xgs5_udf_wb_tcam_info_sync_1_0(unit,
                UDF_CTRL(unit)->num_pkt_formats, scache_ptr);
    BCM_IF_ERROR_RETURN(rv);

    return BCM_E_NONE;
}

/* Returns required scache size for UDF module */ 
STATIC int
bcmi_xgs5_udf_wb_scache_size_get(int unit, int *req_scache_size)
{
    if (BCM_WB_DEFAULT_VERSION == BCM_WB_VERSION_1_0) {
        return bcmi_xgs5_udf_wb_scache_size_get_1_0(unit, req_scache_size);
    }

    return BCM_E_NOT_FOUND;
}

/* Allocates required scache size for the UDF module recovery */
STATIC int
bcmi_xgs5_udf_wb_alloc(int unit)
{
    int     rv;
    int     req_scache_size;
    uint8   *scache_ptr;
    soc_scache_handle_t scache_handle;

    SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_UDF, 0);

    rv = bcmi_xgs5_udf_wb_scache_size_get(unit, &req_scache_size);
    BCM_IF_ERROR_RETURN(rv);

   rv = _bcm_esw_scache_ptr_get(unit, scache_handle, TRUE,
            req_scache_size, &scache_ptr, BCM_WB_DEFAULT_VERSION, NULL);

    if (BCM_E_NOT_FOUND == rv) {
        /* Proceed with Level 1 Warm Boot */
        rv = BCM_E_NONE;
    }

    BCM_IF_ERROR_RETURN(rv);

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcmi_xgs5_udf_wb_sync
 * Purpose:
 *      Syncs UDF warmboot state to scache
 * Parameters:
 *      unit            - (IN)  Unit number.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
bcmi_xgs5_udf_wb_sync(int unit)
{
    uint8   *scache_ptr;
    soc_scache_handle_t scache_handle;

    SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_UDF, 0);

    BCM_IF_ERROR_RETURN
        (_bcm_esw_scache_ptr_get(unit, scache_handle, FALSE,
                                 0, &scache_ptr,
                                 BCM_WB_DEFAULT_VERSION, NULL));

    return bcmi_xgs5_udf_wb_sync_1_0(unit, &scache_ptr);
}

/*
 * Function:
 *      bcmi_xgs5_udf_reinit
 * Purpose:
 *      Recovers UDF warmboot state from scache
 * Parameters:
 *      unit            - (IN)  Unit number.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
STATIC int
bcmi_xgs5_udf_reinit(int unit)
{
    int     rv = BCM_E_INTERNAL;
    uint8   *scache_ptr;
    uint16  recovered_ver = 0;
    soc_scache_handle_t scache_handle;

    SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_UDF, 0);

    rv = _bcm_esw_scache_ptr_get(unit, scache_handle, FALSE,
                                 0, &scache_ptr,
                                 BCM_WB_DEFAULT_VERSION, &recovered_ver);

    if (BCM_E_NOT_FOUND == rv) {
        /* Proceed with Level 1 Warm Boot */
        rv = BCM_E_NONE;
    }

    BCM_IF_ERROR_RETURN(rv);

    if (recovered_ver == BCM_WB_VERSION_1_0) {
        return bcmi_xgs5_udf_wb_reinit_1_0(unit, &scache_ptr);
    }

    return rv;
}



#endif /* BCM_WARM_BOOT_SUPPORT */


/*
 * Function:
 *      bcmi_xgs5_udf_offset_reserve
 * Purpose:
 *      Reserves requested offset chunks.
 * Parameters:
 *      unit            - (IN)  Unit number.
 *      max_offsets     - (IN)  Number of offsets to reserve.
 *      offset[]        - (IN)  Array of offset chunks requested.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *  offset[i]: 0x1 means only first half of the chunk is used.
 *  offset[i]: 0x2 means only second half of the chunk is used.
 *  offset[i]: 0x3 means both the chunks are used.
 */
STATIC int
bcmi_xgs5_udf_offset_reserve(int unit, int max_offsets, int offset[])
{
    int j;
    int first = 0;
    int gran = BCMI_XGS5_UDF_CTRL_OFFSET_GRAN(unit);
    bcmi_xgs5_udf_offset_entry_t *offset_entry;
    SHR_BITDCL *chunk_bmap = &(UDF_CTRL(unit)->hw_bmap);
    SHR_BITDCL *byte_bmap = &(UDF_CTRL(unit)->byte_bmap);


    for (j = 0; j < max_offsets; j++) {
        if (offset[j] == 0) {
            continue;
        }

        /* Return BCM_E_RESOURCE if the offset chunk is already in use */
        if (SHR_BITGET(chunk_bmap, j)) {
            return BCM_E_RESOURCE;
        }

        offset_entry = &(UDF_CTRL(unit)->offset_entry_array[j]);

        offset_entry->num_udfs += 1;

        if (UDF_CTRL(unit)->offset_entry_array[j].num_udfs == 1) {
            /* Mark the entries as used */
            SHR_BITSET(chunk_bmap, j);

            *byte_bmap |= offset[j] << (j * gran);
        }

        if ((offset[j] == 0x1) || (offset[j] == 0x2)) {
            UDF_CTRL(unit)->offset_entry_array[j].flags |=
                BCMI_XGS5_UDF_OFFSET_ENTRY_HALF;
        }

        if (first == 0) {
            UDF_CTRL(unit)->offset_entry_array[j].flags |=
                BCMI_XGS5_UDF_OFFSET_ENTRY_GROUP;

            UDF_CTRL(unit)->offset_entry_array[j].grp_id = j;

            first = j;
        } else {
            UDF_CTRL(unit)->offset_entry_array[j].flags |=
                BCMI_XGS5_UDF_OFFSET_ENTRY_MEMBER;
            UDF_CTRL(unit)->offset_entry_array[j].grp_id = first;
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcmi_xgs5_udf_offset_unreserve
 * Purpose:
 *      UnReserves offset chunks.
 * Parameters:
 *      unit            - (IN)  Unit number.
 *      max_offsets     - (IN)  Number of offsets to unreserve.
 *      offset[]        - (IN)  Array of offset chunks to be unreserved.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
STATIC int
bcmi_xgs5_udf_offset_unreserve(int unit, int max_offsets, int offset[])
{
    int j; 
    int gran = BCMI_XGS5_UDF_CTRL_OFFSET_GRAN(unit);
    bcmi_xgs5_udf_offset_entry_t *offset_entry;
    SHR_BITDCL *chunk_bmap = &(UDF_CTRL(unit)->hw_bmap);
    SHR_BITDCL *byte_bmap = &(UDF_CTRL(unit)->byte_bmap);


    for (j = 0; j < max_offsets; j++) {
        if (offset[j] == 0) {
            continue;
        }

        offset_entry = &(UDF_CTRL(unit)->offset_entry_array[j]);

        /* Decr ref_cnt on the offset entry */
        offset_entry->num_udfs -=1;

        if (offset_entry->num_udfs == 0) {
            /* Defensive Check: num_pkt_formats must be 0 */
            if (offset_entry->num_pkt_formats != 0) {
                return BCM_E_INTERNAL;
            }

            /* Mark the entries as free */
            SHR_BITCLR(chunk_bmap, j);
            *byte_bmap &= ~(offset[j] << (j * gran));

            /* Clear the UDF Offset entry */
            offset_entry->flags = 0x0;
            offset_entry->grp_id = 0x0;
        }
    }

    return BCM_E_NONE;
}


/*
 * Function:
 *      bcmi_xgs5_udf_offset_hw_alloc
 * Purpose:
 *      Allocates offsets for the UDF
 * Parameters:
 *      unit            - (IN)      Unit number.
 *      hints           - (IN)      Hints for allocation of offset chunks.
 *      offset_info     - (INOUT)   UDF Offset info struct
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
STATIC int
bcmi_xgs5_udf_offset_hw_alloc(int unit, bcm_udf_alloc_hints_t *hints,
                              bcmi_xgs5_udf_offset_info_t *offset_info)
{
    int rv;
    int i, j;
    int gran;
    int max_chunks;
    uint32 hw_bmap = 0;
    uint32 byte_bmap = 0;
    int *offset_order;
    int offset_array[MAX_UDF_OFFSET_CHUNKS] = {0};
    int offset_order_d[MAX_UDF_OFFSET_CHUNKS] = {0};
    int alloc_cnt = 0, max_offsets = 0, req_offsets = 0;

    /* offset allocation order */
    int bcmi_xgs5_udf_offsets_default[]  = {2, 3, 6, 7, 0, 1, 4, 5,
                                            8, 9, 10, 11, 12, 13, 14, 15};
    int bcmi_xgs5_udf_offsets_flexhash[] = {12, 13, 14, 15};
    int bcmi_xgs5_udf_offsets_udfhash[]  = {7, 8};

    gran = BCMI_XGS5_UDF_CTRL_OFFSET_GRAN(unit);
    max_chunks = BCMI_XGS5_UDF_CTRL_MAX_UDF_CHUNKS(unit);

    /*
     * The 'start' and 'width' need not always align to the offset granularity.
     * Every offset on TD2 can extract 2 bytes.
     *
     * If the user request a UDF that starts/ends on an odd-byte boundary,
     * SDK should allocate extra chunk(s) to facilitate for the requested
     * byte extraction and account for this extra byte in the field APIs
     * (by masking "don'tcare" for those extra bytes.
     */
    req_offsets += ((offset_info->width / gran) +
                    (offset_info->width % gran));
    req_offsets += (((offset_info->width % gran) == 0) &&
                     (offset_info->start % gran));


    /*
     * FLEXHASH and IFP/VFP flags can be set at the same time
     * Since FLEXHASH has limited chunks, it is preferred during allocation
     */

    if ((hints != NULL) && (hints->flags & BCM_UDF_CREATE_O_FLEXHASH)) {
        max_offsets = COUNTOF(bcmi_xgs5_udf_offsets_flexhash);
        offset_order = &bcmi_xgs5_udf_offsets_flexhash[0];

    } else if (0 /* && (hints->flags & BCM_UDF_CREATE_O_UDFHASH) */) {
        max_offsets = COUNTOF(bcmi_xgs5_udf_offsets_udfhash);
        offset_order = &bcmi_xgs5_udf_offsets_udfhash[0];

    } else if ((hints != NULL) &&
               ((hints->flags & BCM_UDF_CREATE_O_FIELD_INGRESS) ||
                (hints->flags & BCM_UDF_CREATE_O_FIELD_LOOKUP))) {

        _field_stage_id_t stage;

        if (hints->flags & BCM_UDF_CREATE_O_FIELD_INGRESS) {
            stage  = _BCM_FIELD_STAGE_INGRESS;
        } else {
            stage  = _BCM_FIELD_STAGE_LOOKUP;
        }

        /* allocate based on qset */
        rv = _bcm_esw_field_qset_udf_offsets_alloc(unit, stage, hints->qset,
                     req_offsets, &offset_order_d[0], &max_offsets);
        BCM_IF_ERROR_RETURN(rv);

        offset_order = &offset_order_d[0];

    } else {
        max_offsets = COUNTOF(bcmi_xgs5_udf_offsets_default);
        offset_order = &bcmi_xgs5_udf_offsets_default[0];

    }


    for (i = 0; i < max_chunks; i++) {

        /* All chunks are reserved */
        if ((UDF_CTRL(unit)->hw_bmap & 0xffff) == 0xffff) {
            return BCM_E_RESOURCE;
        }

        for (j = 0; j < max_offsets; j++) {
            if ((SHR_BITGET(&(UDF_CTRL(unit)->hw_bmap), offset_order[j]))) {
                offset_array[offset_order[j]] = 0;
                continue;
            }

            /* offset_order will have order of chunks to be allocated.
             * offset_array should be filled with 0x1, 0x2 or 0x3 if
             * first half, or second half or full chunk is alloc'ed respy.
             */
            hw_bmap |= (1 << offset_order[j]);
            if ((alloc_cnt == 0) && (offset_info->start % 2) == 1) {
                byte_bmap |= (0x1 << (offset_order[j] * gran));
                offset_array[offset_order[j]] = (1 << 0);
            } else if ((req_offsets == (alloc_cnt - 1)) &&
                       ((offset_info->width % 2) == 1)) {
                byte_bmap |= (0x2 << (offset_order[j] * gran));
                offset_array[offset_order[j]] = (1 << 1);
            } else {
                byte_bmap |= (0x3 << (offset_order[j] * gran));
                offset_array[offset_order[j]] = ((1 << 0) | (1 << 1));
            }

            if (++alloc_cnt == req_offsets) {
                break;
            }
        }
    }


    if (alloc_cnt < req_offsets) {
        return BCM_E_RESOURCE;
    }

    rv = bcmi_xgs5_udf_offset_reserve(unit, max_offsets, offset_array);
    BCM_IF_ERROR_RETURN(rv);

    /* grp_id is the first chunk number */
    offset_info->grp_id = offset_order[0];

    offset_info->hw_bmap |= hw_bmap;
    offset_info->byte_bmap |= byte_bmap;

    return BCM_E_NONE;
}


/*
 * Function:
 *      bcmi_xgs5_udf_layer_to_offset_base
 * Purpose:
 *      Fetches base and offsets values to configure in UDF_OFFSET table.
 * Parameters:
 *      unit            - (IN)      Unit number.
 *      offset_info     - (IN)      UDF Offset info struct
 *      tcam_info       - (IN)      UDF tcam info struct
 *      base            - (OUT)     base_offset to be configured in BASE_OFFSETx
 *      offset          - (OUT)     offset to be configured in OFFSETx field
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *  Users can implement bcmUdfLayerUserPayload according to the requirement.
 */
STATIC int
bcmi_xgs5_udf_layer_to_offset_base(int unit,
                                   bcmi_xgs5_udf_offset_info_t *offset_info,
                                   bcmi_xgs5_udf_tcam_info_t *tcam_info,
                                   int *base, int *offset)
{

    /* base offset */
    switch (offset_info->layer) {
        case bcmUdfLayerL2Header:
            *base = BCMI_XGS5_UDF_OFFSET_BASE_START_OF_L2;
            *offset = offset_info->start;
            break;
        case bcmUdfLayerL3OuterHeader:
        case bcmUdfLayerTunnelHeader:
            *base = BCMI_XGS5_UDF_OFFSET_BASE_START_OF_OUTER_L3;
            *offset = offset_info->start;
            break;
        case bcmUdfLayerL3InnerHeader:
            *base = BCMI_XGS5_UDF_OFFSET_BASE_START_OF_INNER_L3;
            *offset = offset_info->start;
            break;
        case bcmUdfLayerL4OuterHeader:
        case bcmUdfLayerL4InnerHeader:
        case bcmUdfLayerTunnelPayload:
            *base = BCMI_XGS5_UDF_OFFSET_BASE_START_OF_L4;
            *offset = offset_info->start;
            break;
        case bcmUdfLayerHigigHeader:
        case bcmUdfLayerHigig2Header:
            *base = BCMI_XGS5_UDF_OFFSET_BASE_START_OF_MH;
            *offset = offset_info->start;
            break;
        default:
            /* Unknown layer type: Return Error */
            return BCM_E_PARAM;
    }


    return BCM_E_NONE;
}

/*
 * Function:
 *      bcmi_xgs5_udf_get
 * Purpose:
 *      To fetch the udf info given the UDF ID.
 * Parameters:
 *      unit            - (IN)      Unit number.
 *      udf_id          - (IN)      UDF Id.
 *      udf_info        - (OUT)     UDF info struct
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
bcmi_xgs5_udf_get(int unit, bcm_udf_id_t udf_id, bcm_udf_t *udf_info)
{
    int rv;
    bcmi_xgs5_udf_offset_info_t *offset_info = NULL;

    /* UDF Module Init check */
    UDF_INIT_CHECK(unit);

    BCM_IF_NULL_RETURN_PARAM(udf_info);

    /* UDF Module Lock */
    UDF_LOCK(unit);

    rv = bcmi_xgs5_udf_offset_node_get(unit, udf_id, &offset_info);
    if (BCM_FAILURE(rv)) {
        UDF_UNLOCK(unit);
        return rv;
    }

    /* Copy tmp to offset_info */
    udf_info->start = BYTES2BITS(offset_info->start);
    udf_info->width = BYTES2BITS(offset_info->width);
    udf_info->layer = offset_info->layer;

    /* Not used for XGS5 */
    udf_info->flags = 0;

    /* Release UDF mutex */
    UDF_UNLOCK(unit);

    return BCM_E_NONE;
}


/*
 * Function:
 *      bcmi_xgs5_udf_get_all
 * Purpose:
 *      To fetch all the udfs in the system.
 * Parameters:
 *      unit            - (IN)      Unit number.
 *      max             - (IN)      Number of udf ids to fetch.
 *      udf_id_list     - (OUT)     List of UDF Ids fetched.
 *      actual          - (OUT)     Actual udfs fetched.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
bcmi_xgs5_udf_get_all(int unit, int max, bcm_udf_id_t *udf_list, int *actual)
{
    bcmi_xgs5_udf_offset_info_t *tmp;

    /* UDF Module Init check */
    UDF_INIT_CHECK(unit);

    BCM_IF_NULL_RETURN_PARAM(actual);

    /* UDF Module Lock */
    UDF_LOCK(unit);

    *actual = 0;
    tmp = UDF_CTRL(unit)->offset_info_head;

    while (tmp != NULL) {
        if ((udf_list != NULL) && (*actual < max)) {
            udf_list[(*actual)] = tmp->udf_id;
        }

        (*actual) += 1;
        tmp = tmp->next;
    }

    /* Release UDF mutex */
    UDF_UNLOCK(unit);

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcmi_xgs5_udf_get_all
 * Purpose:
 *      To destroy udf object.
 * Parameters:
 *      unit            - (IN)      Unit number.
 *      udf_id          - (IN)      UDF ID to be destroyed.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
bcmi_xgs5_udf_destroy(int unit, bcm_udf_id_t udf_id)
{
    int rv;
    int i;
    int max, gran;
    int offset[MAX_UDF_OFFSET_CHUNKS] = {0};
    bcmi_xgs5_udf_offset_info_t *cur = NULL;

    /* UDF Module Init check */
    UDF_INIT_CHECK(unit);

    /* UDF Module Lock */
    UDF_LOCK(unit);

    gran = BCMI_XGS5_UDF_CTRL_OFFSET_GRAN(unit);
    max = BCMI_XGS5_UDF_CTRL_MAX_UDF_CHUNKS(unit);;

    rv = bcmi_xgs5_udf_offset_node_get(unit, udf_id, &cur);
    if (BCM_FAILURE(rv)) {
        UDF_UNLOCK(unit);
        return rv;
    }

    /* Matching node not found */
    if (cur == NULL) {
        return BCM_E_NOT_FOUND;
    }

    if (cur->num_pkt_formats >= 1) {
        UDF_UNLOCK(unit);
        return BCM_E_BUSY;
    }

    for (i = 0; i < max; i++) {
        SHR_BITTEST_RANGE(&(cur->byte_bmap), i*gran, gran, offset[i]);
    }

    /* unreserve the offset chunks */
    rv = bcmi_xgs5_udf_offset_unreserve(unit, max, offset);
    if (BCM_FAILURE(rv)) {
        UDF_UNLOCK(unit);
        return rv;
    }

    /* Free the cur node */
    UDF_UNLINK_OFFSET_NODE(cur);
    sal_free(cur);

    UDF_CTRL(unit)->num_udfs -= 1;

    /* Release UDF mutex */
    UDF_UNLOCK(unit);

    return BCM_E_NONE;
}


/*
 * Function:
 *      bcmi_xgs5_udf_offset_info_alloc
 * Purpose:
 *      To allocate memory for UDF offset info struct
 * Parameters:
 *      unit            - (IN)      Unit number.
 *      offset_info     - (OUT)     Pointer to hold the allocated memory.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
STATIC int
bcmi_xgs5_udf_offset_info_alloc(int unit,
                                bcmi_xgs5_udf_offset_info_t **offset_info)
{
    int mem_sz;

    mem_sz = sizeof(bcmi_xgs5_udf_offset_info_t);

    /* Allocate memory for bcmi_xgs5_udf_offset_info_t structure. */
    UDF_MALLOC((*offset_info), bcmi_xgs5_udf_offset_info_t,
                mem_sz, "udf offset info");

    if (NULL == *offset_info) {
        return (BCM_E_MEMORY);
    }


    return BCM_E_NONE;
}


/*
 * Function:
 *      bcmi_xgs5_udf_offset_info_add
 * Purpose:
 *      To convert the UDF info to offset info and add to the linked list.
 * Parameters:
 *      unit            - (IN)      Unit number.
 *      udf_info        - (IN)      UDF info to be added to linked list.
 *      offset_info     - (INOUT)   Pointer to hold the allocated memory.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
STATIC int
bcmi_xgs5_udf_offset_info_add(int unit, bcm_udf_t *udf_info,
                              bcmi_xgs5_udf_offset_info_t **offset_info)
{
    int rv;

    /* Allocate memory for bcmi_xgs5_udf_offset_info_t structure. */
    rv = bcmi_xgs5_udf_offset_info_alloc(unit, offset_info);
    BCM_IF_ERROR_RETURN(rv);

    /* Save user inputs into the offset_info struct */
    (*offset_info)->layer = udf_info->layer;
    (*offset_info)->start = BITS2BYTES(udf_info->start);
    (*offset_info)->width = BITS2BYTES(udf_info->width);

    /* Add offset_info item to the linked list */
    rv = bcmi_xgs5_udf_offset_node_add(unit, *offset_info);
    if (BCM_FAILURE(rv)) {
        sal_free(*offset_info);
        *offset_info = NULL;
    }

    return rv;
}

/* Linked list node add: for offset nodes */
STATIC int
bcmi_xgs5_udf_offset_node_add(int unit, bcmi_xgs5_udf_offset_info_t *new)
{
    bcmi_xgs5_udf_offset_info_t *tmp;

    if (new == NULL) {
        return BCM_E_INTERNAL;
    }

    tmp = (UDF_CTRL(unit)->offset_info_head);

    if (NULL == tmp) {
        /* Adding first node */
        (UDF_CTRL(unit)->offset_info_head) = new;
        new->prev = NULL;

    } else {

        /* traverse to the end of the list */
        while (tmp->next != NULL) {
            tmp = tmp->next;
        }

        /* insert new node to the end of the list */
        new->prev = tmp;
        tmp->next = new;

    }

    new->next = NULL;

    return  BCM_E_NONE;
}

/* Linked list node delete: for offset nodes */
STATIC int
bcmi_xgs5_udf_offset_node_delete(int unit, bcm_udf_id_t udf_id,
                                 bcmi_xgs5_udf_offset_info_t **del)
{
    int rv;
    bcmi_xgs5_udf_offset_info_t *cur = NULL;

    rv = bcmi_xgs5_udf_offset_node_get(unit, udf_id, &cur);
    BCM_IF_ERROR_RETURN(rv);

    /* Matching node not found */
    if (cur == NULL) {
        return BCM_E_NOT_FOUND;
    }

    /* unlink cur from the list */
    if (cur->prev) {
        (cur->prev)->next = cur->next;
    } else {
        /* cur node is the head */
        UDF_CTRL(unit)->offset_info_head = cur->next;
    }

    if (cur->next) {
        (cur->next)->prev = cur->prev;
    }

    /* Callee function to free the node */
    *del = cur;

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcmi_xgs5_udf_tcam_info_alloc
 * Purpose:
 *      To allocate memory for UDF tcam info struct
 * Parameters:
 *      unit            - (IN)      Unit number.
 *      tcam_info       - (OUT)     Pointer to hold the allocated memory.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
STATIC int
bcmi_xgs5_udf_tcam_info_alloc(int unit, bcmi_xgs5_udf_tcam_info_t **tcam_info)
{
    int mem_sz;

    mem_sz = sizeof(bcmi_xgs5_udf_tcam_info_t);

    /* Allocate memory for bcmi_xgs5_udf_offset_info_t structure. */
    UDF_MALLOC((*tcam_info), bcmi_xgs5_udf_tcam_info_t,
                mem_sz, "udf tcam info");

    if (NULL == *tcam_info) {
        return (BCM_E_MEMORY);
    }

    /* Mark entry invalid */
    (*tcam_info)->hw_idx = MAX_UDF_TCAM_ENTRIES;

    return BCM_E_NONE;
}


/*
 * Function:
 *      bcmi_xgs5_udf_tcam_info_add
 * Purpose:
 *      To convert the packet format info to tcam info and add to the list.
 * Parameters:
 *      unit            - (IN)      Unit number.
 *      pkt_format      - (IN)      UDF Packet format to be added to list.
 *      tcam_info       - (INOUT)   Pointer to hold the allocated memory.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
STATIC int
bcmi_xgs5_udf_tcam_info_add(int unit,
                            bcm_udf_pkt_format_info_t *pkt_format,
                            bcmi_xgs5_udf_tcam_info_t **tcam_info)
{
    int rv;
    fp_udf_tcam_entry_t *tcam_buf;

    /* Allocate memory for bcmi_xgs5_udf_tcam_info_t structure. */
    rv = bcmi_xgs5_udf_tcam_info_alloc(unit, tcam_info);
    BCM_IF_ERROR_RETURN(rv);

    tcam_buf = (fp_udf_tcam_entry_t *)&((*tcam_info)->hw_buf);
    sal_memset(tcam_buf, 0, sizeof(fp_udf_tcam_entry_t));

    /* Initialize packet format entry */
    rv = bcmi_xgs5_udf_pkt_format_tcam_key_init(unit, pkt_format,
                                           (uint32 *)tcam_buf);
    BCM_IF_ERROR_RETURN(rv);

    (*tcam_info)->priority = pkt_format->prio;

    /* Add offset_info item to the linked list */
    rv = bcmi_xgs5_udf_tcam_node_add(unit, *tcam_info);
    if (BCM_FAILURE(rv)) {
        sal_free(*tcam_info);
        (*tcam_info) = NULL;
    }

    return rv;
}

/* Linked list node add: for tcam nodes */
STATIC int
bcmi_xgs5_udf_tcam_node_add(int unit, bcmi_xgs5_udf_tcam_info_t *new)
{
    bcmi_xgs5_udf_tcam_info_t *tmp;

    if (new == NULL) {
        return BCM_E_INTERNAL;
    }

    tmp = (UDF_CTRL(unit)->tcam_info_head);

    if (NULL == tmp) {
        /* Adding first node */
        (UDF_CTRL(unit)->tcam_info_head) = new;
        new->prev = NULL;

    } else {

        /* traverse to the end of the list */
        while (tmp->next != NULL) {
            tmp = tmp->next;
        }

        /* insert new node to the end of the list */
        new->prev = tmp;
        tmp->next = new;

    }
    new->next = NULL;

    return  BCM_E_NONE;
}

/* Linked list node delete: for tcam nodes */
STATIC int
bcmi_xgs5_udf_tcam_node_delete(int unit, bcm_udf_pkt_format_id_t pkt_format_id,
                               bcmi_xgs5_udf_tcam_info_t **del)
{
    int rv;
    bcmi_xgs5_udf_tcam_info_t *cur = NULL;

    rv = bcmi_xgs5_udf_tcam_node_get(unit, pkt_format_id, &cur);
    BCM_IF_ERROR_RETURN(rv);

    *del = NULL;

    /* Matching node not found */
    if (cur == NULL) {
        return BCM_E_NOT_FOUND;
    }

    /* unlink cur from the list */
    if (cur->prev) {
        (cur->prev)->next = cur->next;
    } else {
        /* cur node is the head node */
        UDF_CTRL(unit)->tcam_info_head = cur->next;
    }
    if (cur->next) {
        (cur->next)->prev = cur->prev;
    }

    /* Callee function to free the node */
    *del = cur;

    return BCM_E_NONE;
}


/* To allocate running id for UDF and Packet format IDs. */
STATIC int
bcmi_xgs5_udf_id_running_id_alloc(int unit, bcmiUdfObjectType type, int *id)
{
    int rv;
    int ix;
    int min_ix, max_ix;
    bcmi_xgs5_udf_offset_info_t *offset_info = NULL;
    bcmi_xgs5_udf_tcam_info_t *tcam_info = NULL;

    if (type == bcmiUdfObjectUdf) {
        min_ix = BCMI_XGS5_UDF_ID_MIN;
        max_ix = BCMI_XGS5_UDF_ID_MAX;

        ix = BCMI_XGS5_UDF_ID_RUNNING(unit);
    } else {
        min_ix = BCMI_XGS5_UDF_PKT_FORMAT_ID_MIN;
        max_ix = BCMI_XGS5_UDF_PKT_FORMAT_ID_MAX;

        ix = BCMI_XGS5_UDF_PKT_FORMAT_ID_RUNNING(unit);
    }

    if (max_ix < ix) {
        for (ix = min_ix; ix < max_ix; ix++) {
            if (type == bcmiUdfObjectUdf) {
                rv = bcmi_xgs5_udf_offset_node_get(unit, ix, &offset_info);
                if (BCM_E_NOT_FOUND == rv) {
                    break;
                }
            } else {
                rv = bcmi_xgs5_udf_tcam_node_get(unit, ix, &tcam_info);
                if (BCM_E_NOT_FOUND == rv) {
                    break;
                }
            }
            if (BCM_FAILURE(rv)) {
                break;
            }
        }
    }

    /* All 64k udf ids are used up */
    if (max_ix == ix) {
        return BCM_E_FULL;
    }

    *id = ix;

    return BCM_E_NONE;
}

/* Conveerts chunk ID to OFFSET_xf and BASE_OFFSET_xf fields */
STATIC int
bcmi_xgs5_udf_offset_to_hw_field(int unit, int offset,
            soc_field_t *base_hw_f, soc_field_t *offset_hw_f)
{
    soc_field_t offset_f[] =
        {
            UDF1_OFFSET0f,            UDF1_OFFSET1f,
            UDF1_OFFSET2f,            UDF1_OFFSET3f,
            UDF1_OFFSET4f,            UDF1_OFFSET5f,
            UDF1_OFFSET6f,            UDF1_OFFSET7f,
            UDF2_OFFSET0f,            UDF2_OFFSET1f,
            UDF2_OFFSET2f,            UDF2_OFFSET3f,
            UDF2_OFFSET4f,            UDF2_OFFSET5f,
            UDF2_OFFSET6f,            UDF2_OFFSET7f,
        };

    soc_field_t base_offset_f[] =
        {
            UDF1_BASE_OFFSET_0f,      UDF1_BASE_OFFSET_1f,
            UDF1_BASE_OFFSET_2f,      UDF1_BASE_OFFSET_3f,
            UDF1_BASE_OFFSET_4f,      UDF1_BASE_OFFSET_5f,
            UDF1_BASE_OFFSET_6f,      UDF1_BASE_OFFSET_7f,
            UDF2_BASE_OFFSET_0f,      UDF2_BASE_OFFSET_1f,
            UDF2_BASE_OFFSET_2f,      UDF2_BASE_OFFSET_3f,
            UDF2_BASE_OFFSET_4f,      UDF2_BASE_OFFSET_5f,
            UDF2_BASE_OFFSET_6f,      UDF2_BASE_OFFSET_7f,
        };

    *base_hw_f = base_offset_f[offset];
    *offset_hw_f = offset_f[offset];

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcmi_xgs5_udf_offset_uninstall
 * Purpose:
 *      To uninstall the UDF offsets in the hardware
 * Parameters:
 *      unit            - (IN)      Unit number.
 *      e               - (IN)      Entry in the UDF_OFFSET table.
 *      hw_bmap         - (IN)      Offsets to uninstall.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
STATIC int
bcmi_xgs5_udf_offset_uninstall(int unit, int e, uint32 hw_bmap)
{
    int ix;
    int rv;
    int base = 0x0;
    int offset = 0x0;
    soc_mem_t mem;
    soc_field_t base_f, offset_f;
    fp_udf_offset_entry_t offset_buf;

    if ((e < 0) && (e > BCMI_XGS5_UDF_CTRL_MAX_UDF_ENTRIES(unit))) {
        return BCM_E_PARAM;
    }

    mem = UDF_CTRL(unit)->offset_mem;

    rv = soc_mem_read(unit, mem, MEM_BLOCK_ALL, e, &offset_buf);
    BCM_IF_ERROR_RETURN(rv);

    for (ix = 0; ix < BCMI_XGS5_UDF_CTRL_MAX_UDF_CHUNKS(unit); ix++) {
        if (!(SHR_BITGET(&hw_bmap, ix))) {
            continue;
        }

        rv = bcmi_xgs5_udf_offset_to_hw_field(unit, ix, &base_f, &offset_f);
        BCM_IF_ERROR_RETURN(rv);

        soc_mem_field32_set(unit, mem, &offset_buf, base_f, base);
        soc_mem_field32_set(unit, mem, &offset_buf, offset_f, offset);
    }

    rv = soc_mem_write(unit, mem, MEM_BLOCK_ALL, e, &offset_buf);
    BCM_IF_ERROR_RETURN(rv);

    return BCM_E_NONE;
}


/*
 * Function:
 *      bcmi_xgs5_udf_offset_install
 * Purpose:
 *      To install the UDF offsets in the hardware
 * Parameters:
 *      unit            - (IN)      Unit number.
 *      e               - (IN)      Entry in the UDF_OFFSET table.
 *      hw_bmap         - (IN)      Offsets to uninstall.
 *      base            - (IN)      value to write in BASE_OFFSETx.
 *      offset          - (IN)      Value to write in the OFFSETx.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
STATIC int
bcmi_xgs5_udf_offset_install(int unit, int e, uint32 hw_bmap,
                             int base, int offset)
{
    int ix;
    int rv;
    int gran;
    soc_mem_t mem;
    soc_field_t base_f, offset_f;
    fp_udf_offset_entry_t offset_buf;

    if ((e < 0) && (e > BCMI_XGS5_UDF_CTRL_MAX_UDF_ENTRIES(unit))) {
        return BCM_E_PARAM;
    }

    mem = UDF_CTRL(unit)->offset_mem;
    gran = BCMI_XGS5_UDF_CTRL_OFFSET_GRAN(unit);

    rv = soc_mem_read(unit, mem, MEM_BLOCK_ALL, e, &offset_buf);
    BCM_IF_ERROR_RETURN(rv);

    for (ix = 0; ix < BCMI_XGS5_UDF_CTRL_MAX_UDF_CHUNKS(unit); ix++) {
        if (!(SHR_BITGET(&hw_bmap, ix))) {
            continue;
        }

        rv = bcmi_xgs5_udf_offset_to_hw_field(unit, ix, &base_f, &offset_f);
        BCM_IF_ERROR_RETURN(rv);

        soc_mem_field32_set(unit, mem, &offset_buf, base_f, base);
        soc_mem_field32_set(unit, mem, &offset_buf, offset_f, offset);

        /* Every other offset chunk should have offset incr by gran */
        offset += gran;
    }

    rv = soc_mem_write(unit, mem, MEM_BLOCK_ALL, e, &offset_buf);
    BCM_IF_ERROR_RETURN(rv);

    return BCM_E_NONE;
}

/*
 * Function:
 *     bcmi_xgs5_udf_tcam_entry_move
 * Purpose:
 *     Move a single entry in  udf tcam and offset tables.
 * Parameters:
 *     unit      - (IN) BCM device number.
 *     tcam_entry_arr - (IN) Data control structure.
 *     src       - (IN) Move from index.
 *     dst       - (IN) Move to index. 
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
bcmi_xgs5_udf_tcam_entry_move(int unit,
                         bcmi_xgs5_udf_tcam_entry_t *tcam_entry_arr,
                         int src, int dst)
{
    int rv;                                 /* Operation return status.   */
    soc_mem_t offset_mem, tcam_mem;         /* Memory Ids                 */
    uint32 hw_buf[SOC_MAX_MEM_FIELD_WORDS]; /* Buffer to read hw entry.   */

    /* Input parameters check. */
    BCM_IF_NULL_RETURN_PARAM(tcam_entry_arr);

    tcam_mem = UDF_CTRL(unit)->tcam_mem;
    offset_mem = UDF_CTRL(unit)->offset_mem;

    rv = soc_mem_read(unit, offset_mem, MEM_BLOCK_ANY, src, hw_buf);
    BCM_IF_ERROR_RETURN(rv);
    rv = soc_mem_write(unit, offset_mem, MEM_BLOCK_ALL, dst, hw_buf);
    BCM_IF_ERROR_RETURN(rv);

    rv = soc_mem_read(unit, tcam_mem, MEM_BLOCK_ANY, src, hw_buf);
    BCM_IF_ERROR_RETURN(rv);
    rv = soc_mem_write(unit, tcam_mem, MEM_BLOCK_ALL, dst, hw_buf);
    BCM_IF_ERROR_RETURN(rv);

    /* Clear entries at old index. */
    rv = soc_mem_write(unit, offset_mem, MEM_BLOCK_ALL, src,
                       soc_mem_entry_null(unit, offset_mem));
    BCM_IF_ERROR_RETURN(rv);

    rv = soc_mem_write(unit, tcam_mem, MEM_BLOCK_ALL, src,
                       soc_mem_entry_null(unit, tcam_mem));
    BCM_IF_ERROR_RETURN(rv);

    /* Update hw_idx in the packet formats */
    tcam_entry_arr[src].tcam_info->hw_idx = dst;

    /* Update sw structure tracking entry use. */
    sal_memcpy(tcam_entry_arr + dst,
               tcam_entry_arr + src,
               sizeof(bcmi_xgs5_udf_tcam_entry_t));

    sal_memset(tcam_entry_arr + src, 0,
               sizeof(bcmi_xgs5_udf_tcam_entry_t));


    return (BCM_E_NONE);
}



/*
 * Function:
 *     bcmi_xgs5_udf_tcam_move_up
 * Purpose:
 *     Moved udf tcam entries up.
 * Parameters:
 *     unit      - (IN) BCM device number.
 *     tcam_entry_arr - (IN) Data control structure.
 *     dest      - (IN) Insertion target index.
 *     free_slot - (IN) Free slot.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
bcmi_xgs5_udf_tcam_move_up(int unit,
                      bcmi_xgs5_udf_tcam_entry_t *tcam_entry_arr,
                      int dest, int free_slot)
{
    int idx;          /* Entries iterator.        */
    int rv;           /* Operation return status. */

    for (idx = free_slot; idx > dest; idx --) {
        rv = bcmi_xgs5_udf_tcam_entry_move(unit, tcam_entry_arr, idx - 1, idx);
        BCM_IF_ERROR_RETURN(rv);
    }

    return (BCM_E_NONE);
}

/*
 * Function:
 *     bcmi_xgs5_udf_tcam_move_down
 * Purpose:
 *     Moved udf tcam entries down
 * Parameters:
 *     unit      - (IN) BCM device number.
 *     tcam_entry_arr - (IN) Data control structure.
 *     dest      - (IN) Insertion target index.
 *     free_slot - (IN) Free slot.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
bcmi_xgs5_udf_tcam_move_down(int unit,
                        bcmi_xgs5_udf_tcam_entry_t *tcam_entry_arr,
                        int dest, int free_slot)
{
    int idx;          /* Entries iterator.        */
    int rv;           /* Operation return status. */

    for (idx = free_slot; idx < dest; idx ++) {
        rv = bcmi_xgs5_udf_tcam_entry_move(unit, tcam_entry_arr, idx + 1, idx);
        BCM_IF_ERROR_RETURN(rv);
    }
    return (BCM_E_NONE);
}


STATIC int
bcmi_xgs5_udf_tcam_entries_compare(int unit,
                                   uint32 *hw_entry, uint32 *sw_entry)
{
    return sal_memcmp(sw_entry, hw_entry, sizeof(fp_udf_tcam_entry_t));
}


/*
 * Function:
 *     bcmi_xgs5_udf_tcam_entry_match
 * Purpose:
 *     Match tcam entry against currently instllaed ones
 * Parameters:
 *     unit     - (IN) BCM device number.
 *     new      - (IN  New Tcam info structure.
 *     match    - (IN  Matched Tcam info structure.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
bcmi_xgs5_udf_tcam_entry_match(int unit, bcmi_xgs5_udf_tcam_info_t *new,
                               bcmi_xgs5_udf_tcam_info_t **match)
{
    uint32 *old_buf;   /* Tcam entry pointer.        */
    uint32 *new_buf;   /* Tcam entry pointer.        */
    bcmi_xgs5_udf_tcam_info_t *tmp;

    /* Input parameters check. */
    BCM_IF_NULL_RETURN_PARAM(new);

    new_buf = (uint32 *)&(new->hw_buf);

    tmp = UDF_CTRL(unit)->tcam_info_head;

    *match = NULL;

    while (tmp != NULL) {
        if (tmp == new) {
            tmp = tmp->next;
            continue;
        }

        old_buf = (uint32 *)&(tmp->hw_buf);

        if (!bcmi_xgs5_udf_tcam_entries_compare(unit, old_buf, new_buf)) {
            *match = tmp;
            break;
        }

        tmp = tmp->next;
    }

    if (*match == NULL) {
        return BCM_E_NOT_FOUND;
    }


    return BCM_E_NONE;

}

/*
 * Function:
 *      bcmi_xgs5_udf_tcam_entry_insert
 * Purpose:
 *     To insert an entry in UDF_TCAM at an available index.
 * Parameters:
 *     unit        - (IN) BCM device number.
 *     tcam_new    - (IN) Tcam info to be inserted.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
bcmi_xgs5_udf_tcam_entry_insert(int unit, bcmi_xgs5_udf_tcam_info_t *tcam_new)
{
    int idx;                       /* FP UDF tcam table interator.    */
    int rv;                        /* Operation return status.        */
    int idx_max;                   /* FP UDF tcam table index max.    */
    int idx_min;                   /* FP UDF tcam table index min.    */
    int range_min;                 /* Index min for entry insertion.  */
    int range_max;                 /* Index max for entry insertion.  */
    int tcam_idx;
    int unused_entry_min;          /* Unused entry before the range.  */
    int unused_entry_max;          /* Unused entry after the range.   */
    bcmi_xgs5_udf_tcam_info_t *tcam_info;
    bcmi_xgs5_udf_tcam_entry_t *tcam_entry_arr; /* Tcam entries array.  */

    /* Initialization. */
    idx_min = soc_mem_index_min(unit, UDF_CTRL(unit)->tcam_mem);
    idx_max = soc_mem_index_max(unit, UDF_CTRL(unit)->tcam_mem);

    range_min = idx_min;
    range_max = idx_max;
    unused_entry_min = unused_entry_max = -1;

    tcam_entry_arr = UDF_CTRL(unit)->tcam_entry_array;


    /* No identical entry found try to allocate an unused entry and
     * reshuffle tcam if necessary to organize entries by priority.
     */
    for (idx = idx_min; idx <= idx_max; idx++) {
        if (0 == tcam_entry_arr[idx].valid) {
            if (idx <= range_max) {
                /* Any free index below range max can be used for insertion. */
                unused_entry_min = idx;
            } else {
                /* There is no point to continue after first
                 * free index above range max.
                 */
                unused_entry_max = idx;
                break;
            }
        } else {
            tcam_info = tcam_entry_arr[idx].tcam_info;
            /* Identify insertion range. */
            if ((tcam_info->priority + tcam_info->pkt_format_id) >
                (tcam_new->priority + tcam_new->pkt_format_id)) {
                range_min = idx;
            } else if (tcam_info->priority < tcam_new->priority) {
                if (idx < range_max) {
                    range_max = idx;
                }
            }
        }
    }

    /* Check if tcam is completely full. */
    if ((unused_entry_min == -1) && (unused_entry_max == -1)) {
        return (BCM_E_FULL);
    }

    /*  Tcam entries shuffling. */
    if (unused_entry_min > range_min) {
        tcam_idx = unused_entry_min;
    } else if (unused_entry_min == -1) {
        rv = bcmi_xgs5_udf_tcam_move_up(unit, tcam_entry_arr,
                                         range_max, unused_entry_max);

        BCM_IF_ERROR_RETURN(rv);
        tcam_idx = range_max;
    } else if (unused_entry_max == -1) {
        rv = bcmi_xgs5_udf_tcam_move_down(unit, tcam_entry_arr,
                                           range_min, unused_entry_min);
        BCM_IF_ERROR_RETURN(rv);
        tcam_idx = range_min;
    } else if ((range_min - unused_entry_min) >
               (unused_entry_max - range_max)) {
        rv = bcmi_xgs5_udf_tcam_move_up(unit, tcam_entry_arr,
                                         range_max, unused_entry_max);
        BCM_IF_ERROR_RETURN(rv);
        tcam_idx = range_max;
    } else {
        rv = bcmi_xgs5_udf_tcam_move_down(unit, tcam_entry_arr,
                                           range_min, unused_entry_min);
        BCM_IF_ERROR_RETURN(rv);
        tcam_idx = range_min;
    }

    /* Install the entry into hardware */
    rv = soc_mem_write(unit, UDF_CTRL(unit)->tcam_mem,
                       MEM_BLOCK_ALL, tcam_idx, &(tcam_new->hw_buf));
    SOC_IF_ERROR_RETURN(rv);

    /* Index was successfully allocated. */
    tcam_new->hw_idx = tcam_idx;
    tcam_entry_arr[tcam_idx].valid = 1;
    tcam_entry_arr[tcam_idx].tcam_info = tcam_new;

    return (BCM_E_NONE);
}



/*
 * Function:
 *     bcmi_xgs5_udf_tcam_entry_l3_parse
 * Purpose:
 *     Parse udf tcam entry l3 format match key.
 * Parameters:
 *     unit     - (IN) BCM device number.
 *     hw_buf   - (IN) Hw buffer.
 *     pkt_fmt  - (OUT) Packet format structure.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
bcmi_xgs5_udf_tcam_entry_l3_parse(int unit, uint32 *hw_buf,
                                  bcm_udf_pkt_format_info_t *pkt_fmt)
{
    uint32 ethertype = -1;
    uint32 inner_iptype = -1;
    uint32 ethertype_mask = -1;
    uint32 outer_iptype = -1;
    uint32 outer_iptype_ip4_val = -1;
    uint32 outer_iptype_ip6_val = -1;
    uint32 outer_iptype_mask = -1;
    uint32 inner_iptype_mask = -1;
    uint32 l3fields = -1;
    uint32 l3fields_mask = -1;
    soc_mem_t mem = FP_UDF_TCAMm;
    uint32 iptype_none_val = 0;
    uint8  fc_header_encode = 0;
    uint8  fc_hdr_encode_mask = 0;

    /* Input parameters check. */
    BCM_IF_NULL_RETURN_PARAM(hw_buf);
    BCM_IF_NULL_RETURN_PARAM(pkt_fmt);

    if (SOC_IS_TD_TT(unit)
        || SOC_IS_KATANAX(unit)
        || SOC_IS_TRIUMPH3(unit)) {
        iptype_none_val = 2;
    }
    if ((ethertype_mask =
            soc_mem_field32_get(unit, mem, hw_buf, L2_ETHER_TYPE_MASKf))) {
        ethertype = soc_mem_field32_get(unit, mem, hw_buf, L2_ETHER_TYPEf);
    }

    if ((inner_iptype_mask =
            soc_mem_field32_get(unit, mem, hw_buf, INNER_IP_TYPE_MASKf))) {
        inner_iptype = soc_mem_field32_get(unit, mem, hw_buf, INNER_IP_TYPEf);
    }

    outer_iptype = soc_mem_field32_get(unit, mem, hw_buf, OUTER_IP_TYPEf);
    outer_iptype_mask =  soc_mem_field32_get(unit, mem,
                                          hw_buf, OUTER_IP_TYPE_MASKf);

    if (soc_mem_field32_get(unit, mem, hw_buf, L3_FIELDS_MASKf)) {
        l3fields = soc_mem_field32_get(unit, mem, hw_buf, L3_FIELDSf);
        l3fields_mask = soc_mem_field32_get(unit, mem, hw_buf, L3_FIELDS_MASKf);

        pkt_fmt->ip_protocol = (l3fields >> 16);
        pkt_fmt->ip_protocol_mask = (l3fields_mask >> 16);
    } else if (0x8847 == ethertype) {
        /* MPLS_ANY has L3_FIELDSf == 0 */
        l3fields = soc_mem_field32_get(unit, mem, hw_buf, L3_FIELDSf);
    }

    outer_iptype_ip4_val = OUTER_IP4_TYPE_VAL(unit);
    outer_iptype_ip6_val = OUTER_IP6_TYPE_VAL(unit);

    pkt_fmt->ethertype = ethertype;
    pkt_fmt->ethertype_mask = ethertype_mask;

    /*
     * Logic below implicitly checks Ethertype value for IP, MPLS and FCoE
     * frames.
     * 0x0800 - IPv4, 0x86DD - IPv6, 0x8847 - MPLS , 0x8906 + 0x8914 - FCoE
     */

    if ((ethertype == 0x800) && (outer_iptype == outer_iptype_ip4_val)) {
        pkt_fmt->tunnel = BCM_PKT_FORMAT_TUNNEL_NONE;
        pkt_fmt->outer_ip = BCM_PKT_FORMAT_IP4_WITH_OPTIONS;
        pkt_fmt->inner_ip = BCM_PKT_FORMAT_IP_NONE;
    } else if ((ethertype == 0x86dd) &&
               (outer_iptype == outer_iptype_ip6_val)) {
        pkt_fmt->tunnel = BCM_PKT_FORMAT_TUNNEL_NONE;
        pkt_fmt->outer_ip = BCM_PKT_FORMAT_IP6_WITH_OPTIONS;
        pkt_fmt->inner_ip = BCM_PKT_FORMAT_IP_NONE;
    } else if ((ethertype == 0x800) && (inner_iptype ==  iptype_none_val)) {
        pkt_fmt->tunnel = BCM_PKT_FORMAT_TUNNEL_NONE;
        pkt_fmt->outer_ip = BCM_PKT_FORMAT_IP4;
        pkt_fmt->inner_ip = BCM_PKT_FORMAT_IP_NONE;
    } else if ((ethertype == 0x86dd) && (inner_iptype ==  iptype_none_val)) {
        pkt_fmt->tunnel = BCM_PKT_FORMAT_TUNNEL_NONE;
        pkt_fmt->outer_ip = BCM_PKT_FORMAT_IP6;
        pkt_fmt->inner_ip = BCM_PKT_FORMAT_IP_NONE;
    } else if ((ethertype == 0x800) && (l3fields ==  0x40000)) {
        pkt_fmt->tunnel = BCM_PKT_FORMAT_TUNNEL_IP_IN_IP;
        pkt_fmt->outer_ip = BCM_PKT_FORMAT_IP4;
        pkt_fmt->inner_ip = BCM_PKT_FORMAT_IP4;
    } else if ((ethertype == 0x800) && (l3fields ==  0x290000)) {
        pkt_fmt->tunnel = BCM_PKT_FORMAT_TUNNEL_IP_IN_IP;
        pkt_fmt->outer_ip = BCM_PKT_FORMAT_IP4;
        pkt_fmt->inner_ip = BCM_PKT_FORMAT_IP6;
    } else if ((ethertype == 0x86dd) && (l3fields ==  0x40000)) {
        pkt_fmt->tunnel = BCM_PKT_FORMAT_TUNNEL_IP_IN_IP;
        pkt_fmt->outer_ip = BCM_PKT_FORMAT_IP6;
        pkt_fmt->inner_ip = BCM_PKT_FORMAT_IP4;
    } else if ((ethertype == 0x86dd) && (l3fields ==  0x290000)) {
        pkt_fmt->tunnel = BCM_PKT_FORMAT_TUNNEL_IP_IN_IP;
        pkt_fmt->outer_ip = BCM_PKT_FORMAT_IP6;
        pkt_fmt->inner_ip = BCM_PKT_FORMAT_IP6;
    } else if ((ethertype == 0x800) && (l3fields ==  0x2f0800)) {
        pkt_fmt->tunnel = BCM_PKT_FORMAT_TUNNEL_GRE;
        pkt_fmt->outer_ip = BCM_PKT_FORMAT_IP4;
        pkt_fmt->inner_ip = BCM_PKT_FORMAT_IP4;
    } else if ((ethertype == 0x800) && (l3fields ==  0x2f86dd)) {
        pkt_fmt->tunnel = BCM_PKT_FORMAT_TUNNEL_GRE;
        pkt_fmt->outer_ip = BCM_PKT_FORMAT_IP4;
        pkt_fmt->inner_ip = BCM_PKT_FORMAT_IP6;
    } else if ((ethertype == 0x86dd) && (l3fields ==  0x2f0800)) {
        pkt_fmt->tunnel = BCM_PKT_FORMAT_TUNNEL_GRE;
        pkt_fmt->outer_ip = BCM_PKT_FORMAT_IP6;
        pkt_fmt->inner_ip = BCM_PKT_FORMAT_IP4;
    } else if ((ethertype == 0x86dd) && (l3fields ==  0x2f86dd)) {
        pkt_fmt->tunnel = BCM_PKT_FORMAT_TUNNEL_GRE;
        pkt_fmt->outer_ip = BCM_PKT_FORMAT_IP6;
        pkt_fmt->inner_ip = BCM_PKT_FORMAT_IP6;
    } else if ((ethertype == 0x8847)) {
        pkt_fmt->tunnel = BCM_PKT_FORMAT_TUNNEL_MPLS;

        switch (l3fields) {

            case 1:
                pkt_fmt->mpls = BCM_PKT_FORMAT_MPLS_ONE_LABEL;
                break;
            case 2:
                pkt_fmt->mpls = BCM_PKT_FORMAT_MPLS_TWO_LABELS;
                break;
            case 3:
                pkt_fmt->mpls = BCM_PKT_FORMAT_MPLS_THREE_LABELS;
                break;
            case 4:
                pkt_fmt->mpls = BCM_PKT_FORMAT_MPLS_FOUR_LABELS;
                break;
            case 5:
                pkt_fmt->mpls = BCM_PKT_FORMAT_MPLS_FIVE_LABELS;
                break;
            default :
                pkt_fmt->mpls = BCM_PKT_FORMAT_MPLS_ANY;
                break;
        }

    } else if (ethertype == 0x8906 || ethertype == 0x8914) {
        pkt_fmt->tunnel = (ethertype == 0x8906)
            ? BCM_PKT_FORMAT_TUNNEL_FCOE
            : BCM_PKT_FORMAT_TUNNEL_FCOE_INIT;

        fc_header_encode = soc_mem_field32_get(unit, FP_UDF_TCAMm, hw_buf,
                                FC_HDR_ENCODE_1f);
        fc_hdr_encode_mask = soc_mem_field32_get(unit, FP_UDF_TCAMm, hw_buf,
                                FC_HDR_ENCODE_1_MASKf);

        if (0 != fc_header_encode && 0 != fc_hdr_encode_mask) {

            switch (fc_header_encode & fc_hdr_encode_mask) {
                case 1:
                    pkt_fmt->fibre_chan_outer
                        = BCM_PKT_FORMAT_FIBRE_CHAN;
                    break;
                case 2:
                    pkt_fmt->fibre_chan_outer
                        = BCM_PKT_FORMAT_FIBRE_CHAN_VIRTUAL;
                    break;
                case 3:
                    pkt_fmt->fibre_chan_outer
                        = BCM_PKT_FORMAT_FIBRE_CHAN_ENCAP;
                    break;
                case 4:
                    pkt_fmt->fibre_chan_outer
                        = BCM_PKT_FORMAT_FIBRE_CHAN_ROUTED;
                    break;
                default:
                    return (BCM_E_INTERNAL);
            }
        }

        fc_header_encode = soc_mem_field32_get(unit, FP_UDF_TCAMm, hw_buf,
                                FC_HDR_ENCODE_2f);
        fc_hdr_encode_mask = soc_mem_field32_get(unit, FP_UDF_TCAMm, hw_buf,
                                FC_HDR_ENCODE_2_MASKf);
        if (0 != fc_header_encode && 0 != fc_hdr_encode_mask) {

            switch (fc_header_encode & fc_hdr_encode_mask) {
                case 1:
                    pkt_fmt->fibre_chan_inner
                        = BCM_PKT_FORMAT_FIBRE_CHAN;
                    break;
                case 2:
                    pkt_fmt->fibre_chan_inner
                        = BCM_PKT_FORMAT_FIBRE_CHAN_VIRTUAL;
                    break;
                case 3:
                    pkt_fmt->fibre_chan_inner
                        = BCM_PKT_FORMAT_FIBRE_CHAN_ENCAP;
                    break;
                case 4:
                    pkt_fmt->fibre_chan_inner
                        = BCM_PKT_FORMAT_FIBRE_CHAN_ROUTED;
                    break;
                default:
                    return (BCM_E_INTERNAL);
            }
        }
    } else if (outer_iptype_mask == 0x7) {
        /*
         * For Non-IP traffic: OUTER_IP_TYPE_MASK = 0x7 and
         * outer_ip_type_val = 0 (TR2) or 2 (Trident, Katana).
         */
        pkt_fmt->tunnel = BCM_PKT_FORMAT_TUNNEL_NONE;
        pkt_fmt->outer_ip = BCM_PKT_FORMAT_IP_NONE;
        pkt_fmt->inner_ip = BCM_PKT_FORMAT_IP_NONE;
    } else if (0x0 == ethertype_mask && 0x0 == inner_iptype_mask
                && 0x0 == outer_iptype_mask) {
        /*
         * Do not care about IP headers and Tunnel Type.
         */
        pkt_fmt->tunnel = BCM_PKT_FORMAT_TUNNEL_ANY;
        pkt_fmt->outer_ip = BCM_PKT_FORMAT_IP_ANY;
        pkt_fmt->inner_ip = BCM_PKT_FORMAT_IP_ANY;
    } else if (outer_iptype_mask == 0x0) {
        /*
         * Do not care about Outer IP. Inner IP is not valid
         * as its not a Tunnel.
         */
        pkt_fmt->tunnel = BCM_PKT_FORMAT_TUNNEL_NONE;
        pkt_fmt->outer_ip = BCM_PKT_FORMAT_IP_ANY;
        pkt_fmt->inner_ip = BCM_PKT_FORMAT_IP_NONE;
    } else {
        return (BCM_E_INTERNAL);
    }


    return (BCM_E_NONE);
}

/*
 * Function:
 *     bcmi_xgs5_udf_tcam_entry_l3_init
 * Purpose:
 *     Initialize udf tcam entry l2 format match key.
 * Parameters:
 *     unit     - (IN) BCM device number.
 *     pkt_fmt  - (IN) Packet format structure.
 *     hw_buf   - (IN/OUT) Hw buffer.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
bcmi_xgs5_udf_tcam_entry_l3_init(int unit,
                                 bcm_udf_pkt_format_info_t *pkt_fmt,
                                 uint32 *hw_buf)
{
    uint32 iptype_none_val = 0;
    uint32 outer_iptype_ip4_val = 0;
    uint32 outer_iptype_ip6_val = 0;

    uint16 l2_ethertype_val    = 0;
    uint16 l2_ethertype_mask   = 0;
    uint32 l3_fields_val       = 0;
    uint32 l3_fields_mask      = 0;
    uint32 inner_ip_type_val   = 0;
    uint32 inner_ip_type_mask  = 0;
    uint32 outer_ip_type_val   = 0;
    uint32 outer_ip_type_mask  = 0;
    uint32 inner_chn_key       = 0;
    uint32 inner_chn_mask      = 0;
    uint32 outer_chn_key       = 0;
    uint32 outer_chn_mask      = 0;


    /* Input parameters check. */
    BCM_IF_NULL_RETURN_PARAM(hw_buf);
    BCM_IF_NULL_RETURN_PARAM(pkt_fmt);

    iptype_none_val = 2;

    outer_iptype_ip4_val = OUTER_IP4_TYPE_VAL(unit);
    outer_iptype_ip6_val = OUTER_IP6_TYPE_VAL(unit);

    if (pkt_fmt->tunnel == BCM_PKT_FORMAT_TUNNEL_NONE) {

        /* inner ip type (none). */
        inner_ip_type_val   = iptype_none_val;
        inner_ip_type_mask  = 0x7;
        l2_ethertype_mask   = 0xffff;


        switch (pkt_fmt->outer_ip) {

            case BCM_PKT_FORMAT_IP4_WITH_OPTIONS:
                l2_ethertype_val     = 0x800;
                outer_ip_type_val = outer_iptype_ip4_val;
                outer_ip_type_mask     = 0x7;
            break;

            case BCM_PKT_FORMAT_IP6_WITH_OPTIONS:
                l2_ethertype_val     = 0x86dd;
                outer_ip_type_val = outer_iptype_ip6_val;
                outer_ip_type_mask     = 0x7;
            break;

            case BCM_PKT_FORMAT_IP4:
                l2_ethertype_val = 0x0800;
            break;

            case BCM_PKT_FORMAT_IP6:
                l2_ethertype_val = 0x86dd;
            break;

            case BCM_PKT_FORMAT_IP_NONE:
                outer_ip_type_val = iptype_none_val;
                outer_ip_type_mask     = 0x7;
            break;

            default:
                outer_ip_type_mask = 0x0;
        }
    } else if (pkt_fmt->tunnel == BCM_PKT_FORMAT_TUNNEL_IP_IN_IP) {
        l2_ethertype_mask = 0xffff;
        l3_fields_mask    = 0xff0000;
        if (pkt_fmt->inner_ip == BCM_PKT_FORMAT_IP4) {
            /* Inner ip protocol v4. */
            l3_fields_val  = 0x40000;
        } else if (pkt_fmt->inner_ip == BCM_PKT_FORMAT_IP6) {
            /* Inner ip protocol v6. */
            l3_fields_val  = 0x290000;
        } else {
            return (BCM_E_UNAVAIL);
        }

        if (pkt_fmt->outer_ip == BCM_PKT_FORMAT_IP4) {
            /* L2 ether type 0x800 */
            l2_ethertype_val     = 0x800;
        } else if (pkt_fmt->outer_ip == BCM_PKT_FORMAT_IP6) {
            /* L2 ether type 0x86dd */
            l2_ethertype_val     = 0x86dd;
        } else {
            return (BCM_E_UNAVAIL);
        }
    } else if (pkt_fmt->tunnel == BCM_PKT_FORMAT_TUNNEL_GRE) {

        l2_ethertype_mask = 0xffff;
        l3_fields_mask    = 0xffffff;
        if (pkt_fmt->inner_ip == BCM_PKT_FORMAT_IP4) {
            /* Inner ip protocol gre, gre ethertype 0x800. */
            l3_fields_val  = 0x2f0800;
        } else if (pkt_fmt->inner_ip == BCM_PKT_FORMAT_IP6) {
            /* Inner ip protocol gre, gre ethertype 0x86dd. */
            l3_fields_val  = 0x2f86dd;
        } else {
            return (BCM_E_UNAVAIL);
        }

        if (pkt_fmt->outer_ip == BCM_PKT_FORMAT_IP4) {
            l2_ethertype_val     = 0x0800;
        } else if (pkt_fmt->outer_ip == BCM_PKT_FORMAT_IP6) {
            /* L2 ether type 0x86dd */
            l2_ethertype_val     = 0x86dd;
        } else {
            return (BCM_E_UNAVAIL);
        }
    } else if (pkt_fmt->tunnel == BCM_PKT_FORMAT_TUNNEL_MPLS) {

        l3_fields_mask = 0xffffff;
        switch (pkt_fmt->mpls) {

            case BCM_PKT_FORMAT_MPLS_ONE_LABEL:
                l3_fields_val = 0x01;
            break;

            case BCM_PKT_FORMAT_MPLS_TWO_LABELS:
                l3_fields_val = 0x02;
            break;

#ifdef MPLS_FIVE_LABELS_SUPPORT
            
            case BCM_PKT_FORMAT_MPLS_THREE_LABELS:
                l3_fields_val = 0x03;
            break;

            case BCM_PKT_FORMAT_MPLS_FOUR_LABELS:
                l3_fields_val = 0x04;
            break;

            case BCM_PKT_FORMAT_MPLS_FIVE_LABELS:
                l3_fields_val = 0x05;
            break;
#endif /* MPLS_FIVE_LABELS_SUPPORT */
            case BCM_PKT_FORMAT_MPLS_ANY:
                l3_fields_val = 0x0;
                l3_fields_mask   = 0x0;
            break;

            default:
                return (BCM_E_UNAVAIL);
        }

        l2_ethertype_val = 0x8847;
        l2_ethertype_mask = 0xffff;

    } else if (pkt_fmt->tunnel == BCM_PKT_FORMAT_TUNNEL_FCOE ||
               pkt_fmt->tunnel == BCM_PKT_FORMAT_TUNNEL_FCOE_INIT) {
        if (!SOC_MEM_FIELD_VALID(unit, FP_UDF_TCAMm, FC_HDR_ENCODE_1f)) {
            return BCM_E_UNAVAIL;
        }

        l2_ethertype_val = ((pkt_fmt->tunnel ==
                            BCM_PKT_FORMAT_TUNNEL_FCOE) ?
                            0x8906 : 0x8914);

        l2_ethertype_mask = 0xffff;

        outer_chn_mask = 0x7;
        switch (pkt_fmt->fibre_chan_outer) {
            case BCM_PKT_FORMAT_FIBRE_CHAN:
                outer_chn_key = 1;
                break;
            case BCM_PKT_FORMAT_FIBRE_CHAN_ENCAP:
                outer_chn_key = 3;
                break;
            case BCM_PKT_FORMAT_FIBRE_CHAN_VIRTUAL:
                outer_chn_key = 2;
                break;
            case BCM_PKT_FORMAT_FIBRE_CHAN_ROUTED:
                outer_chn_key = 4;
                break;
            case BCM_PKT_FORMAT_FIBRE_CHAN_ANY:
                outer_chn_key = 0;
                outer_chn_mask = 0;
                break;
            default:
                return BCM_E_UNAVAIL;
        }

        /* Entry matching on two extended headers has higher priority */
        inner_chn_mask = 0x07;

        switch (pkt_fmt->fibre_chan_inner) {
            case BCM_PKT_FORMAT_FIBRE_CHAN:
                inner_chn_key = 1;
                break;
            case BCM_PKT_FORMAT_FIBRE_CHAN_ENCAP:
                inner_chn_key = 3;
                break;
            case BCM_PKT_FORMAT_FIBRE_CHAN_VIRTUAL:
                inner_chn_key = 2;
                break;
            case BCM_PKT_FORMAT_FIBRE_CHAN_ROUTED:
                inner_chn_key = 4;
                break;
            case BCM_PKT_FORMAT_FIBRE_CHAN_ANY:
                inner_chn_key = 0;
                inner_chn_mask = 0;
                break;
            default:
                return BCM_E_UNAVAIL;
        }

    } else if (BCM_PKT_FORMAT_TUNNEL_ANY == pkt_fmt->tunnel
                && BCM_PKT_FORMAT_IP_ANY == pkt_fmt->outer_ip
                && BCM_PKT_FORMAT_IP_ANY == pkt_fmt->inner_ip) {

        l2_ethertype_val = 0;
        l2_ethertype_mask = 0;
        l3_fields_val = 0;
        l3_fields_mask = 0;
        outer_ip_type_val = 0;
        outer_ip_type_mask = 0;
        inner_ip_type_val = 0;
        inner_ip_type_mask = 0;
        inner_chn_key =0;
        inner_chn_mask =0;
        outer_chn_key =0;
        outer_chn_mask =0;

    } else {
        return (BCM_E_UNAVAIL);
    }

    /* Configure ip_protocol/mask fields */
    l3_fields_val |= (pkt_fmt->ip_protocol << 16);
    l3_fields_mask |= (pkt_fmt->ip_protocol << 16);


    /* ALL the keys are configured. Now set them in entry */
    soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                        L2_ETHER_TYPEf, l2_ethertype_val);
    soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                        L2_ETHER_TYPE_MASKf, l2_ethertype_mask);
    soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                        L3_FIELDSf, l3_fields_val);
    soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                        L3_FIELDS_MASKf, l3_fields_mask);
    soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                        OUTER_IP_TYPEf, outer_ip_type_val);
    soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                        OUTER_IP_TYPE_MASKf, outer_ip_type_mask);
    soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                        INNER_IP_TYPEf, inner_ip_type_val);
    soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                        INNER_IP_TYPE_MASKf,inner_ip_type_mask);

    if (SOC_MEM_FIELD_VALID(unit,FP_UDF_TCAMm,FC_HDR_ENCODE_1f)) {
        soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                            FC_HDR_ENCODE_1f, outer_chn_key);
    }
    if (SOC_MEM_FIELD_VALID(unit,FP_UDF_TCAMm,FC_HDR_ENCODE_1_MASKf)) {
        soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                            FC_HDR_ENCODE_1_MASKf, outer_chn_mask);
    }
    if (SOC_MEM_FIELD_VALID(unit,FP_UDF_TCAMm,FC_HDR_ENCODE_2f)) {
        soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                            FC_HDR_ENCODE_2f, inner_chn_key);
    }
    if (SOC_MEM_FIELD_VALID(unit,FP_UDF_TCAMm,FC_HDR_ENCODE_2_MASKf)) {
        soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                            FC_HDR_ENCODE_2_MASKf, inner_chn_mask);
    }

    return (BCM_E_NONE);
}


/* Converts the misc packet format flags to fields in the UDF_TCAM */
STATIC int
bcmi_xgs5_udf_tcam_misc_format_to_hw_fields(
    int unit, int type,
    soc_field_t *data_f, soc_field_t *mask_f,
    int *present, int *none, int *any, int *support)
{

    soc_field_t hw_fields_f[2][6] = {
            {
                HIGIGf,        CNTAG_PRESENTf,
                ICNM_PACKETf,  VNTAG_PRESENTf,
                ETAG_PACKETf,  SUBPORT_TAG_PRESENTf
            },
            {
                HIGIG_MASKf,        CNTAG_PRESENT_MASKf,
                ICNM_PACKET_MASKf,  VNTAG_PRESENT_MASKf,
                ETAG_PACKET_MASKf,  SUBPORT_TAG_PRESENT_MASKf
            }
        };
    int sw_flags[3][6] = {
            {
                BCM_PKT_FORMAT_HIGIG_PRESENT,
                BCM_PKT_FORMAT_CNTAG_PRESENT,
                BCM_PKT_FORMAT_ICNM_PRESENT,
                BCM_PKT_FORMAT_VNTAG_PRESENT,
                BCM_PKT_FORMAT_ETAG_PRESENT,
                BCM_PKT_FORMAT_SUBPORT_TAG_PRESENT
            },
            {
                BCM_PKT_FORMAT_HIGIG_NONE,
                BCM_PKT_FORMAT_CNTAG_NONE,
                BCM_PKT_FORMAT_ICNM_NONE,
                BCM_PKT_FORMAT_VNTAG_NONE,
                BCM_PKT_FORMAT_ETAG_NONE,
                BCM_PKT_FORMAT_SUBPORT_TAG_NONE
            },
            {
                BCM_PKT_FORMAT_HIGIG_ANY,
                BCM_PKT_FORMAT_CNTAG_ANY,
                BCM_PKT_FORMAT_ICNM_ANY,
                BCM_PKT_FORMAT_VNTAG_ANY,
                BCM_PKT_FORMAT_ETAG_ANY,
                BCM_PKT_FORMAT_SUBPORT_TAG_ANY
            }
        };
    int ctrl_flags[1][6] = {
            {
                BCMI_XGS5_UDF_CTRL_TCAM_HIGIG,
                BCMI_XGS5_UDF_CTRL_TCAM_CNTAG,
                BCMI_XGS5_UDF_CTRL_TCAM_ICNM,
                BCMI_XGS5_UDF_CTRL_TCAM_VNTAG,
                BCMI_XGS5_UDF_CTRL_TCAM_ETAG,
                BCMI_XGS5_UDF_CTRL_TCAM_SUBPORT_TAG
            }
        };

    *data_f     = hw_fields_f[0][type];
    *mask_f     = hw_fields_f[1][type];
    *present    = sw_flags[0][type];
    *none       = sw_flags[1][type];
    *any        = sw_flags[2][type];
    *support    = ctrl_flags[0][type];

    return BCM_E_NONE;
}

/*
 * Function:
 *     bcmi_xgs5_udf_tcam_entry_misc_parse
 * Purpose:
 *     Parse udf tcam entry flags format match key.
 * Parameters:
 *     unit     - (IN) BCM device number.
 *     hw_buf   - (IN) Hw buffer.
 *     flags    - (OUT) BCM_PKT_FORMAT_F_XXX
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
bcmi_xgs5_udf_tcam_entry_misc_parse(int unit, int type,
                                    uint32 *hw_buf, uint16 *flags)
{
    int rv;
    soc_mem_t mem;
    uint32 data, mask;
    soc_field_t data_f, mask_f;
    int present, none, any, support;

    /* Input parameters check. */
    BCM_IF_NULL_RETURN_PARAM(hw_buf);
    BCM_IF_NULL_RETURN_PARAM(flags);

    mem = UDF_CTRL(unit)->tcam_mem;

    rv = bcmi_xgs5_udf_tcam_misc_format_to_hw_fields(unit, type,
             &data_f, &mask_f, &present, &none, &any, &support);
    BCM_IF_ERROR_RETURN(rv);

    if (soc_mem_field32_get(unit, mem, hw_buf, mask_f)) {
        data = soc_mem_field32_get(unit, mem, hw_buf, data_f);
        mask = soc_mem_field32_get(unit, mem, hw_buf, mask_f);
        *flags |= ((mask == 0) ? any : ((data == 1) ? present : none));
    }

    return (BCM_E_NONE);
}

/*
 * Function:
 *     bcmi_xgs5_udf_tcam_entry_misc_init
 * Purpose:
 *     Initialize udf tcam entry flags match key.
 * Parameters:
 *     unit     - (IN) BCM device number.
 *     flags    - (IN) BCM_PKT_FORMAT_F_XXX
 *     hw_buf   - (OUT) Hw buffer.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
bcmi_xgs5_udf_tcam_entry_misc_init(int unit, int type,
                                   uint16 flags, uint32 *hw_buf)
{
    int rv;
    soc_mem_t mem;
    soc_field_t data_f, mask_f;
    int present, none, any, support;


    /* Input parameters check. */
    BCM_IF_NULL_RETURN_PARAM(hw_buf);

    mem = UDF_CTRL(unit)->tcam_mem;

    rv = bcmi_xgs5_udf_tcam_misc_format_to_hw_fields(unit, type,
             &data_f, &mask_f, &present, &none, &any, &support);
    BCM_IF_ERROR_RETURN(rv);

    if (!(UDF_CTRL(unit)->flags & support)) {
        if (flags != any) {
            return BCM_E_PARAM;
        }
    }

    /* Both PRESENT and NONE flags cannot be set
     * If it is required to match on all packets, set ANY */
    if ((flags & present) && (flags & none)) {
        return BCM_E_PARAM;
    }

    if ((flags & present) || (flags & none)) {
        soc_mem_field32_set(unit, mem, hw_buf, mask_f, 1);
        soc_mem_field32_set(unit, mem, hw_buf, data_f, (flags & present) ? 1:0);
    } else {
        /* any */
        soc_mem_field32_set(unit, mem, hw_buf, data_f, 0);
        soc_mem_field32_set(unit, mem, hw_buf, mask_f, 0);
    }

    return BCM_E_NONE;

}

/*
 * Function:
 *     bcmi_xgs5_udf_tcam_entry_l2format_init
 * Purpose:
 *     Initialize udf tcam entry l2 format match key.
 * Parameters:
 *     unit     - (IN) BCM device number.
 *     l2format - (IN) BCM_PKT_FORMAT_L2_XXX
 *     hw_buf   - (OUT) Hw buffer.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
bcmi_xgs5_udf_tcam_entry_l2format_init(int unit,
                                       bcm_udf_pkt_format_info_t *pkt_fmt,
                                       uint32 *hw_buf)
{
    uint16 l2format = pkt_fmt->l2;

    /* Input parameters check. */
    BCM_IF_NULL_RETURN_PARAM(hw_buf);
    BCM_IF_NULL_RETURN_PARAM(pkt_fmt);

    /* Translate L2 flag bits to index */
    switch (l2format) {
      case BCM_PKT_FORMAT_L2_ETH_II:
          /* L2 Format . */
          soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                              L2_TYPEf, 0);
          soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                              L2_TYPE_MASKf, 0x3);

          break;
      case BCM_PKT_FORMAT_L2_SNAP:
          /* L2 Format . */
          soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                              L2_TYPEf, 1);
          soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                              L2_TYPE_MASKf, 0x3);

          break;
      case BCM_PKT_FORMAT_L2_LLC:
          /* L2 Format . */
          soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                              L2_TYPEf, 2);
          soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                              L2_TYPE_MASKf, 0x3);

          break;
      case BCM_PKT_FORMAT_L2_ANY:
          /* L2 Format . */
          soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                              L2_TYPEf, 0);
          soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                              L2_TYPE_MASKf, 0);
          break;
      default:
          return (BCM_E_PARAM);
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *     bcmi_xgs5_udf_tcam_entry_l2format_parse
 * Purpose:
 *     Parse udf tcam entry l2 format match key.
 * Parameters:
 *     unit     - (IN) BCM device number.
 *     hw_buf   - (IN) Hw buffer.
 *     l2format - (OUT) BCM_PKT_FORMAT_L2_XXX
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
bcmi_xgs5_udf_tcam_entry_l2format_parse(int unit, uint32 *hw_buf,
                                        bcm_udf_pkt_format_info_t *pkt_fmt)
{
    uint32 l2type;
    uint16 *l2format = &pkt_fmt->l2;

    /* Input parameters check. */
    BCM_IF_NULL_RETURN_PARAM(hw_buf);
    BCM_IF_NULL_RETURN_PARAM(pkt_fmt);

    if (soc_mem_field32_get(unit, FP_UDF_TCAMm, hw_buf, L2_TYPE_MASKf)) {
        l2type = soc_mem_field32_get(unit, FP_UDF_TCAMm, hw_buf, L2_TYPEf);
        switch (l2type) {
          case 0:
              *l2format = BCM_PKT_FORMAT_L2_ETH_II;
              break;
          case 1:
              *l2format = BCM_PKT_FORMAT_L2_SNAP;
              break;
          case 2:
              *l2format = BCM_PKT_FORMAT_L2_LLC;
              break;
          default:
              break;
        }
    } else {
        *l2format = BCM_PKT_FORMAT_L2_ANY;
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *     bcmi_xgs5_udf_tcam_entry_vlanformat_parse
 * Purpose:
 *     Parse udf tcam entry l2 format match key.
 * Parameters:
 *     unit     - (IN) BCM device number.
 *     hw_buf   - (IN) Hw buffer.
 *     vlanformat - (OUT) BCM_PKT_FORMAT_L2_XXX
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
bcmi_xgs5_udf_tcam_entry_vlanformat_parse(int unit, uint32 *hw_buf,
                                          bcm_udf_pkt_format_info_t *pkt_fmt)
{
    uint32 tag_status;
    uint16 *vlanformat = &pkt_fmt->vlan_tag;

    /* Input parameters check. */
    BCM_IF_NULL_RETURN_PARAM(hw_buf);
    BCM_IF_NULL_RETURN_PARAM(pkt_fmt);

    if (soc_mem_field32_get(unit, FP_UDF_TCAMm, hw_buf, L2_TAG_STATUS_MASKf)) {
        tag_status = soc_mem_field32_get(unit, FP_UDF_TCAMm, hw_buf,
                                         L2_TAG_STATUSf);
        switch (tag_status) {
          case 0:
              *vlanformat = BCM_PKT_FORMAT_VLAN_TAG_NONE;
              break;
          case 1:
              *vlanformat = BCM_PKT_FORMAT_VLAN_TAG_SINGLE;
              break;
          case 2:
              *vlanformat = BCM_PKT_FORMAT_VLAN_TAG_DOUBLE;
              break;
          default:
              break;
        }
    } else {
        *vlanformat = BCM_PKT_FORMAT_VLAN_TAG_ANY;
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *     bcmi_xgs5_udf_tcam_entry_vlanformat_init
 * Purpose:
 *     Initialize udf tcam entry vlan tag format match key.
 * Parameters:
 *     unit       - (IN) BCM device number.
 *     vlanformat - (IN) BCM_PKT_FORMAT_L2_XXX
 *     hw_buf     - (IN/OUT) Hw buffer.
 * Returns:
 *     BCM_E_XXX
 */
STATIC int
bcmi_xgs5_udf_tcam_entry_vlanformat_init(int unit,
                                         bcm_udf_pkt_format_info_t *pkt_fmt,
                                         uint32 *hw_buf)
{
    uint16 vlanformat = pkt_fmt->vlan_tag;

    /* Input parameters check. */
    BCM_IF_NULL_RETURN_PARAM(hw_buf);
    BCM_IF_NULL_RETURN_PARAM(pkt_fmt);

    /* Translate L2 flag bits to index */
    switch (vlanformat) {
        case BCM_PKT_FORMAT_VLAN_TAG_NONE:
            /* L2 Format . */
            soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                L2_TAG_STATUSf, 0);
            soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                L2_TAG_STATUS_MASKf, 0x3);

          break;
        case BCM_PKT_FORMAT_VLAN_TAG_SINGLE:
            /* L2 Format . */
            soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                L2_TAG_STATUSf, 1);
            soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                L2_TAG_STATUS_MASKf, 0x3);
        break;
        case BCM_PKT_FORMAT_VLAN_TAG_DOUBLE:
            /* L2 Format . */
            soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                L2_TAG_STATUSf, 2);
            soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                L2_TAG_STATUS_MASKf, 0x3);

        break;
        case BCM_PKT_FORMAT_VLAN_TAG_ANY:
          /* L2 Format . */
            soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                L2_TAG_STATUSf, 0);
            soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf,
                                L2_TAG_STATUS_MASKf, 0);
        break;
        default:
            return (BCM_E_PARAM);
    }
    return (BCM_E_NONE);
}


/*
 * Function:
 *     bcmi_xgs5_udf_tcam_entry_parse
 * Purpose:
 *     Parse udf tcam entry key to packet format info
 * Parameters:
 *     unit     - (IN) BCM device number.
 *     hw_buf   - (IN) Hw buffer.
 *     pkt_fmt  - (OUT) Packet format to be filled in.
 * Returns:
 *     BCM_E_XXX
 */

STATIC int
bcmi_xgs5_udf_tcam_entry_parse(int unit, uint32 *hw_buf,
                               bcm_udf_pkt_format_info_t *pkt_fmt)
{
    int rv;

    /* Input parameters check. */
    BCM_IF_NULL_RETURN_PARAM(hw_buf);

    bcm_udf_pkt_format_info_t_init(pkt_fmt);

    /* If entry not valid, return empty pkt_format. */
    if (!soc_mem_field32_get(unit, FP_UDF_TCAMm, hw_buf, VALIDf)) {
        return (BCM_E_NONE);
    }

    /* Parse vlan_tag format.*/
    rv = bcmi_xgs5_udf_tcam_entry_vlanformat_parse(unit, hw_buf, pkt_fmt);
    BCM_IF_ERROR_RETURN(rv);

    /* Parse l2 format.*/
    rv = bcmi_xgs5_udf_tcam_entry_l2format_parse(unit, hw_buf, pkt_fmt);
    BCM_IF_ERROR_RETURN(rv);

    /* Parse l3 fields.*/
    rv = bcmi_xgs5_udf_tcam_entry_l3_parse(unit, hw_buf, pkt_fmt);
    BCM_IF_ERROR_RETURN(rv);

    /* Parse Misc Flags fields */
    rv = bcmi_xgs5_udf_tcam_entry_misc_parse(unit, bcmiUdfPktFormatHigig,
                                             hw_buf, (uint16 *)&pkt_fmt->higig);
    BCM_IF_ERROR_RETURN(rv);

    rv = bcmi_xgs5_udf_tcam_entry_misc_parse(unit, bcmiUdfPktFormatVntag,
                                             hw_buf, (uint16 *)&pkt_fmt->vntag);
    BCM_IF_ERROR_RETURN(rv);

    rv = bcmi_xgs5_udf_tcam_entry_misc_parse(unit, bcmiUdfPktFormatEtag,
                                             hw_buf, (uint16 *)&pkt_fmt->etag);
    BCM_IF_ERROR_RETURN(rv);

    rv = bcmi_xgs5_udf_tcam_entry_misc_parse(unit, bcmiUdfPktFormatCntag,
                                             hw_buf, (uint16 *)&pkt_fmt->cntag);
    BCM_IF_ERROR_RETURN(rv);

    rv = bcmi_xgs5_udf_tcam_entry_misc_parse(unit, bcmiUdfPktFormatIcnm,
                                             hw_buf, (uint16 *)&pkt_fmt->icnm);
    BCM_IF_ERROR_RETURN(rv);

    return (BCM_E_NONE);
}

/* Parse ethertype and mask from pkt_format to hw_buf */
STATIC int
bcmi_xgs5_udf_tcam_ethertype_init(int unit,
        bcm_udf_pkt_format_info_t *pkt_format, uint32 *hw_buf)
{
    soc_mem_t mem;
    bcm_ethertype_t ethertype;
    bcm_ethertype_t ethertype_mask;

    mem = UDF_CTRL(unit)->tcam_mem;

    if (pkt_format->ethertype_mask != 0) {
        ethertype = soc_mem_field32_get(unit, mem,
                                        hw_buf, L2_ETHER_TYPEf);
        ethertype_mask = soc_mem_field32_get(unit, mem,
                                             hw_buf, L2_ETHER_TYPE_MASKf);

        if ((ethertype_mask != 0) && 
            ((ethertype & ethertype_mask) != 
             (pkt_format->ethertype & pkt_format->ethertype_mask))) {
            return BCM_E_CONFIG;
        } else {
            soc_mem_field32_set(unit, mem, hw_buf,
                               L2_ETHER_TYPEf, pkt_format->ethertype);
            soc_mem_field32_set(unit, mem, hw_buf,
                               L2_ETHER_TYPE_MASKf, pkt_format->ethertype_mask);
        }
    }

    return BCM_E_NONE;
}


/*
 * Function:
 *      _field_trx2_data_qualifier_etype_tcam_key_init
 * Purpose:
 *      Initialize ethertype based udf tcam entry.
 * Parameters:
 *      unit       - (IN) bcm device.
 *      pkt_format - (IN) Packet format info
 *      hw_buf     - (OUT) Hardware buffer.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
bcmi_xgs5_udf_pkt_format_tcam_key_init(int unit,
                                       bcm_udf_pkt_format_info_t *pkt_format,
                                       uint32 *hw_buf)
{
    int rv;

    /* Input parameters check. */
    BCM_IF_NULL_RETURN_PARAM(hw_buf);
    BCM_IF_NULL_RETURN_PARAM(pkt_format);

    /* Set valid bit. */
    soc_mem_field32_set(unit, FP_UDF_TCAMm, hw_buf, VALIDf, 1);

    /* Set l2 format. */
    rv = bcmi_xgs5_udf_tcam_entry_l2format_init(unit, pkt_format, hw_buf);
    BCM_IF_ERROR_RETURN(rv);

    /* Set vlan tag format. */
    rv = bcmi_xgs5_udf_tcam_entry_vlanformat_init(unit, pkt_format, hw_buf);
    BCM_IF_ERROR_RETURN(rv);

    /* Set l3 packet format. */
    rv = bcmi_xgs5_udf_tcam_entry_l3_init(unit, pkt_format, hw_buf);
    BCM_IF_ERROR_RETURN(rv);

    /* Set HiGig packet formats */
    rv = bcmi_xgs5_udf_tcam_entry_misc_init(unit, bcmiUdfPktFormatHigig,
                                       pkt_format->higig, hw_buf);
    BCM_IF_ERROR_RETURN(rv);

    /* Set VNTAG packet formats */
    rv = bcmi_xgs5_udf_tcam_entry_misc_init(unit, bcmiUdfPktFormatVntag,
                                       pkt_format->vntag, hw_buf);
    BCM_IF_ERROR_RETURN(rv);

    /* Set ETAG packet formats */
    rv = bcmi_xgs5_udf_tcam_entry_misc_init(unit, bcmiUdfPktFormatEtag,
                                       pkt_format->etag, hw_buf);
    BCM_IF_ERROR_RETURN(rv);

    /* Set CNTAG packet formats */
    rv = bcmi_xgs5_udf_tcam_entry_misc_init(unit, bcmiUdfPktFormatCntag,
                                       pkt_format->cntag, hw_buf);
    BCM_IF_ERROR_RETURN(rv);

    /* Set ICNM packet formats */
    rv = bcmi_xgs5_udf_tcam_entry_misc_init(unit, bcmiUdfPktFormatIcnm,
                                       pkt_format->icnm, hw_buf);
    BCM_IF_ERROR_RETURN(rv);

#if defined UDF_TCAM_SUBPORT_TAG_PRESENT
    /* Set Subport tag packet formats */
    rv = bcmi_xgs5_udf_tcam_entry_misc_init(unit, bcmiUdfPktFormatSubportTag,
                                       pkt_format->subport_tag, hw_buf);
    BCM_IF_ERROR_RETURN(rv);
#endif /* UDF_TCAM_SUBPORT_TAG_PRESENT */


    rv = bcmi_xgs5_udf_tcam_ethertype_init(unit, pkt_format, hw_buf);
    BCM_IF_ERROR_RETURN(rv);

    return (rv);
}

/*
 * Function:
 *      bcmi_xgs5_udf_offset_node_get
 * Purpose:
 *      Fetches UDF offset node given udf id.
 * Parameters:
 *      unit            - (IN)      Unit number.
 *      udf_id          - (IN)      UDF Id.
 *      offset_info     - (INOUT)   Fecthed udf offset info.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
bcmi_xgs5_udf_offset_node_get(int unit, bcm_udf_id_t udf_id,
                              bcmi_xgs5_udf_offset_info_t **cur)
{
    bcmi_xgs5_udf_offset_info_t *tmp;

    tmp = UDF_CTRL(unit)->offset_info_head;

    while (tmp != NULL) {
        if (tmp->udf_id == udf_id) {
            break;
        }
        tmp = tmp->next;
    }

    /* Reached end of the list */
    if (tmp == NULL) {
        *cur = NULL;
        return BCM_E_NOT_FOUND;
    }

    *cur = tmp;

    return BCM_E_NONE;
}


/*
 * Function:
 *      bcmi_xgs5_udf_tcam_node_get
 * Purpose:
 *      Fetches UDF tcam node given packet format id.
 * Parameters:
 *      unit            - (IN)      Unit number.
 *      pkt_format_id   - (IN)      UDF Packet format Id.
 *      offset_info     - (INOUT)   Fecthed udf tcam info.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
bcmi_xgs5_udf_tcam_node_get(int unit,
                            bcm_udf_pkt_format_id_t pkt_format_id,
                            bcmi_xgs5_udf_tcam_info_t **cur)
{
    bcmi_xgs5_udf_tcam_info_t *tmp;

    tmp = UDF_CTRL(unit)->tcam_info_head;

    while (tmp != NULL) {
        if (tmp->pkt_format_id == pkt_format_id) {
            break;
        }
        tmp = tmp->next;
    }

    /* Reached end of the list */
    if (tmp == NULL) {
        *cur = NULL;
        return BCM_E_NOT_FOUND;
    }

    *cur = tmp;

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcmi_xgs5_udf_detach
 * Purpose:
 *      Detaches udf module and cleans software state.
 * Parameters:
 *      unit            - (IN)      Unit number.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
bcmi_xgs5_udf_detach(int unit)
{
    UDF_INIT_CHECK(unit);

    /* Free resources related to UDF module */
    BCM_IF_ERROR_RETURN(bcmi_xgs5_udf_ctrl_free(unit, UDF_CTRL(unit)));

    /* Reset udf module init state */

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcmi_xgs5_udf_init
 * Purpose:
 *      initializes/ReInitializes the UDF module.
 * Parameters:
 *      unit            - (IN)      Unit number.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
bcmi_xgs5_udf_init(int unit)
{
    int rv;

    /* Init control structures */
    rv = bcmi_xgs5_udf_ctrl_init(unit);
    BCM_IF_ERROR_RETURN(rv);

#if defined (BCM_WARM_BOOT_SUPPORT)
    if (SOC_WARM_BOOT(unit)) {
        rv = bcmi_xgs5_udf_reinit(unit);
        BCM_IF_ERROR_RETURN(rv);
    } else
#endif /* BCM_WARM_BOOT_SUPPORT */
    {
        rv = bcmi_xgs5_udf_hw_init(unit);
        BCM_IF_ERROR_RETURN(rv);

#if defined (BCM_WARM_BOOT_SUPPORT)
        rv = bcmi_xgs5_udf_wb_alloc(unit);
        BCM_IF_ERROR_RETURN(rv);
#endif /* BCM_WARM_BOOT_SUPPORT */
    }

    return BCM_E_NONE;
}

/* To sanitize udf_info/pkt_format_info input structures */
STATIC int
bcmi_xgs5_udf_sanitize_inputs(int unit, bcm_udf_t *udf_info,
                              bcm_udf_pkt_format_info_t *pkt_format)
{
    if (udf_info != NULL) {

        /* start and width should be at byte boundary */
        if (((udf_info->start % 8) != 0) || ((udf_info->width % 8) != 0)) {
            return BCM_E_PARAM;
        }

        /* flags in udf_info is currently not used; must be 0 */
        if (udf_info->flags != 0) {
            return BCM_E_PARAM;
        }

        /* check if layer is in known range */
        if ((udf_info->layer < bcmUdfLayerL2Header) ||
            (udf_info->layer >= bcmUdfLayerCount)) {
            return BCM_E_PARAM;
        }

        
        if (udf_info->layer == bcmUdfLayerUserPayload) {
            return BCM_E_PARAM;
        }

    }

    if (pkt_format != NULL) {

        if ((pkt_format->prio < 0) && (pkt_format->prio > 0xffff)) {
            /* Soft restriction - prio maintained as 'short' integer */
            return BCM_E_PARAM;
        }
    }

    return BCM_E_NONE;
}


/*
 * Function:
 *      bcm_udf_create
 * Purpose:
 *      Creates a UDF object
 * Parameters:
 *      unit - (IN) Unit number.
 *      hints - (IN) Hints to UDF allocator
 *      udf_info - (IN) UDF structure
 *      udf_id - (IN/OUT) UDF ID
 * Returns:
 *      BCM_E_NONE          UDF created successfully.
 *      BCM_E_EXISTS        Entry already exists.
 *      BCM_E_FULL          UDF table full.
 *      BCM_E_UNIT          Invalid BCM Unit number.
 *      BCM_E_INIT          UDF module not initialized.
 *      BCM_E_UNAVAIL       Feature not supported.
 *      BCM_E_XXX           Standard error code.
 * Notes:
 */
int
bcmi_xgs5_udf_create(
    int unit,
    bcm_udf_alloc_hints_t *hints,
    bcm_udf_t *udf_info,
    bcm_udf_id_t *udf_id)
{
    int i;
    int gran;
    int max_chunks;
    int id_running_allocated = 0;
    int rv = BCM_E_NONE;
    bcmi_xgs5_udf_offset_info_t *offset_del = NULL;
    bcmi_xgs5_udf_offset_info_t *offset_info = NULL;

    /* UDF Module Init check */
    UDF_INIT_CHECK(unit);

    /* NULL check; hints can be NULL */
    BCM_IF_NULL_RETURN_PARAM(udf_id);
    BCM_IF_NULL_RETURN_PARAM(udf_info);

    gran = BCMI_XGS5_UDF_CTRL_OFFSET_GRAN(unit);

    /* Sanitize input parameters */
    rv = bcmi_xgs5_udf_sanitize_inputs(unit, udf_info, NULL);
    BCM_IF_ERROR_RETURN(rv);

    /* UDF Module Lock */
    UDF_LOCK(unit);


    max_chunks = BCMI_XGS5_UDF_CTRL_MAX_UDF_CHUNKS(unit);

    /* Check if similar entry already existing */
    if ((hints != NULL) && (hints->flags & BCM_UDF_CREATE_O_WITHID)) {

        UDF_ID_VALIDATE(*udf_id);

        /* Fetch the udf offset info;  get should be successful */
        rv = bcmi_xgs5_udf_offset_node_get(unit, *udf_id, &offset_info);

        if ((BCM_FAILURE(rv)) || (offset_info == NULL)) {
            UDF_UNLOCK(unit);
            return BCM_E_NOT_FOUND;
        }

        if ((hints->flags & BCM_UDF_CREATE_O_REPLACE)) {
            if (offset_info->num_pkt_formats >= 1) {
                /* UDF already in use */
                return BCM_E_CONFIG;
            }
            /* Delete the UDF entry and recreate one */
            rv = bcmi_xgs5_udf_offset_node_delete(unit, *udf_id, &offset_del);
            BCM_IF_ERROR_RETURN(rv);

            sal_free(offset_del);
        }
    } else {
        /* Fresh udf creation */

        /* Allocate udf id */
        rv = bcmi_xgs5_udf_id_running_id_alloc(unit, bcmiUdfObjectUdf, udf_id);
        if (BCM_FAILURE(rv)) {
            UDF_UNLOCK(unit);
            return rv;
        } else {
            id_running_allocated = 1;
        }
    }

    /* Add udf info structure to linked list */
    rv = bcmi_xgs5_udf_offset_info_add(unit, udf_info, &offset_info);
    if ((BCM_FAILURE(rv)) || (offset_info == NULL)) {
        UDF_UNLOCK(unit);
        return BCM_E_MEMORY;
    }

    if ((UDF_CTRL(unit)->num_udfs + 1) == UDF_CTRL(unit)->max_udfs) {
        rv = BCM_E_RESOURCE;
    }
    UDF_IF_ERROR_CLEANUP(rv);

    /* For shared hwid, just copy the hw details */
    if ((hints != NULL) && (hints->flags & BCM_UDF_CREATE_O_SHARED_HWID)) {
        bcmi_xgs5_udf_offset_info_t *offset_shared;

        rv = bcmi_xgs5_udf_offset_node_get(unit, hints->shared_udf,
                                           &offset_shared);
        UDF_IF_ERROR_CLEANUP(rv);

        /* A shared udf should also should use the same width */
        if (offset_info->width != offset_shared->width) {
            rv = BCM_E_CONFIG;
        }

        /* Shared offset should 'start' at the same byte boundary
         * Otherwise the chunks allocated may not suffice the requirement
         */
        if ((offset_info->start % gran) != (offset_shared->start % gran)) {
            rv = BCM_E_CONFIG;
        }
        UDF_IF_ERROR_CLEANUP(rv);

        /* Since shared, copy the same chunk bitmap */
        offset_info->hw_bmap = offset_shared->hw_bmap;
        offset_info->byte_bmap = offset_shared->byte_bmap;

        offset_info->flags |= BCMI_XGS5_UDF_OFFSET_INFO_SHARED;
        offset_shared->flags |= BCMI_XGS5_UDF_OFFSET_INFO_SHARED;

        /* Mark the entry as shared */
        for (i = 0; i < max_chunks; i ++) {
            if (SHR_BITGET(&(offset_info->hw_bmap), i)) {
                UDF_CTRL(unit)->offset_entry_array[i].flags |=
                    BCMI_XGS5_UDF_OFFSET_ENTRY_SHARED;
            }
        }
    } else {

        /* allocate udf chunks */
        rv = bcmi_xgs5_udf_offset_hw_alloc(unit, hints, offset_info);
        UDF_IF_ERROR_CLEANUP(rv);

    }

    /* update udf_id in the offset_info */
    offset_info->udf_id = *udf_id;

    if (hints != NULL) {
        if (hints->flags & BCM_UDF_CREATE_O_FLEXHASH) {
            offset_info->flags |= BCMI_XGS5_UDF_OFFSET_FLEXHASH;
        }
        if (hints->flags & BCM_UDF_CREATE_O_FIELD_INGRESS) {
            offset_info->flags |= BCMI_XGS5_UDF_OFFSET_IFP;
        }
        if (hints->flags & BCM_UDF_CREATE_O_FIELD_LOOKUP) {
            offset_info->flags |= BCMI_XGS5_UDF_OFFSET_VFP;
        }
    }

    /* Increment num udfs */
    UDF_CTRL(unit)->num_udfs += 1;

cleanup:


    if (BCM_FAILURE(rv)) {

        if ((id_running_allocated == 1) &&
            (UDF_CTRL(unit)->udf_id_running != BCMI_XGS5_UDF_ID_MAX)) {
            /* Decrement the running udf id */
            UDF_CTRL(unit)->udf_id_running -= 1;
        }

        /* Delete the offset node */
        if (offset_info != NULL) {
            UDF_UNLINK_OFFSET_NODE(offset_info);
            sal_free(offset_info);
        }
    }

    /* Release UDF mutex */
    UDF_UNLOCK(unit);


    return rv;
}

/*
 * Function:
 *      bcm_udf_pkt_format_create
 * Purpose:
 *      Create a packet format entry
 * Parameters:
 *      unit - (IN) Unit number.
 *      options - (IN) API Options.
 *      pkt_format - (IN) UDF packet format info structure
 *      pkt_format_id - (OUT) Packet format ID
 * Returns:
 *      BCM_E_NONE          UDF packet format entry created
 *                          successfully.
 *      BCM_E_UNIT          Invalid BCM Unit number.
 *      BCM_E_INIT          UDF module not initialized.
 *      BCM_E_UNAVAIL       Feature not supported.
 *      BCM_E_XXX           Standard error code.
 * Notes:
 */
int
bcmi_xgs5_udf_pkt_format_create(
    int unit,
    bcm_udf_pkt_format_options_t options,
    bcm_udf_pkt_format_info_t *pkt_format_info,
    bcm_udf_pkt_format_id_t *pkt_format_id)
{
    int id_running_allocated = 0;
    bcmi_xgs5_udf_tcam_info_t *tcam_info_match = NULL;
    bcmi_xgs5_udf_tcam_info_t *tcam_info_local = NULL;

    int rv = BCM_E_NONE;

    /* UDF Module Init check */
    UDF_INIT_CHECK(unit);


    /* Input parameters check. */
    BCM_IF_NULL_RETURN_PARAM(pkt_format_id);
    BCM_IF_NULL_RETURN_PARAM(pkt_format_info);

    /* Sanitize input parameters */
    rv = bcmi_xgs5_udf_sanitize_inputs(unit, NULL, pkt_format_info);
    BCM_IF_ERROR_RETURN(rv);

    /* UDF Module Lock */
    UDF_LOCK(unit);

    /* Check if similar entry already existing */
    if (options & BCM_UDF_PKT_FORMAT_CREATE_O_WITHID) {

        UDF_PKT_FORMAT_ID_VALIDATE(*pkt_format_id);

        /* Fetch the tcam info;  get should be successful */
        rv = bcmi_xgs5_udf_tcam_node_get(unit, *pkt_format_id,
                                         &tcam_info_local);

        if (BCM_FAILURE(rv)) {
            UDF_UNLOCK(unit);
            return rv;
        }

        if ((options & BCM_UDF_PKT_FORMAT_CREATE_O_REPLACE)) {
            if (tcam_info_local->num_udfs >= 1) {
                /* UDF Pkt format entry already in use */
                UDF_UNLOCK(unit);
                return BCM_E_CONFIG;
            }
            /* Delete the UDF pkt format entry and recreate one */
            rv = bcmi_xgs5_udf_tcam_node_delete(unit, *pkt_format_id,
                                                &tcam_info_match);
            BCM_IF_ERROR_RETURN(rv);

            sal_free(tcam_info_match);
        }
    } else {

        /* Allocate new id */
        rv = bcmi_xgs5_udf_id_running_id_alloc(unit, bcmiUdfObjectPktFormat,
                                               pkt_format_id);
        if (BCM_FAILURE(rv)) {
            UDF_UNLOCK(unit);
            return rv;
        } else {
            id_running_allocated = 1;
        }
    }

    /* Insert the packet format entry in the linked list */
    rv = bcmi_xgs5_udf_tcam_info_add(unit, pkt_format_info, &tcam_info_local);
    UDF_IF_ERROR_CLEANUP(rv);

    if ((UDF_CTRL(unit)->num_pkt_formats + 1) ==
         BCMI_XGS5_UDF_CTRL_MAX_UDF_CHUNKS(unit)) {
        rv = BCM_E_RESOURCE;
    }
    UDF_IF_ERROR_CLEANUP(rv);


    /*
     * The packet format matching hardware is a tcam and hence
     * there is no point in allowing to create more than one entry
     * of similar configuration. Search for a matching entry in
     * list of already installed objects and return error appropriately.
     */

    rv = bcmi_xgs5_udf_tcam_entry_match(unit,
                                        tcam_info_local, &tcam_info_match);
    if (BCM_SUCCESS(rv)) {
        if (tcam_info_match->priority == pkt_format_info->prio) {
            rv = BCM_E_EXISTS;
        } else {
            rv = BCM_E_RESOURCE;
        }
    } else {
        if (rv == BCM_E_NOT_FOUND) {
            rv = BCM_E_NONE;
        }

    }
    UDF_IF_ERROR_CLEANUP(rv);


    /* Increment num udfs */
    UDF_CTRL(unit)->num_pkt_formats += 1;

    /* update pkt_format_id in the tcam_info */
    tcam_info_local->pkt_format_id = *pkt_format_id;

cleanup:

    if (BCM_FAILURE(rv)) {
        if ((id_running_allocated) &&
            (UDF_CTRL(unit)->udf_pkt_format_id_running !=
             BCMI_XGS5_UDF_PKT_FORMAT_ID_MAX)) {
            /* Decrement the running pkt format id */
            UDF_CTRL(unit)->udf_pkt_format_id_running -= 1;
        }

        /* Delete the offset node */
        if (tcam_info_local != NULL) {
            UDF_UNLINK_TCAM_NODE(tcam_info_local);
            sal_free(tcam_info_local);
        }
    }

    /* Release UDF mutex */
    UDF_UNLOCK(unit);


    return rv;
}


/*
 * Function:
 *      bcm_udf_pkt_format_destroy
 * Purpose:
 *      Destroy existing packet format entry
 * Parameters:
 *      unit - (IN) Unit number.
 *      pkt_format_id - (IN) Packet format ID
 *      pkt_format - (IN) UDF packet format info structure
 * Returns:
 *      BCM_E_NONE          Destroy packet format entry.
 *      BCM_E_NOT_FOUND     Packet format ID does not exist.
 *      BCM_E_UNIT          Invalid BCM Unit number.
 *      BCM_E_INIT          UDF module not initialized.
 *      BCM_E_UNAVAIL       Feature not supported.
 *      BCM_E_XXX           Standard error code.
 * Notes:
 */
int
bcmi_xgs5_udf_pkt_format_destroy(int unit,
                                 bcm_udf_pkt_format_id_t pkt_format_id)
{
    int rv;
    bcmi_xgs5_udf_tcam_info_t *tcam_info = NULL;

    /* UDF Module Init check */
    UDF_INIT_CHECK(unit);

    /* UDF Module Lock */
    UDF_LOCK(unit);

    /* Retrieve the node indexed by pkt_format_id */
    rv = bcmi_xgs5_udf_tcam_node_get(unit, pkt_format_id, &tcam_info);
    if (BCM_FAILURE(rv)) {
        UDF_UNLOCK(unit);
        return rv;
    }

    /* Delete the software object */
    if (tcam_info->num_udfs >= 1) {
        UDF_UNLOCK(unit);
        return BCM_E_BUSY;
    } else {
        UDF_UNLINK_TCAM_NODE(tcam_info);

        /* Free the tcam node */
        sal_free(tcam_info);
    }

    /* Release UDF mutex */
    UDF_UNLOCK(unit);

    return BCM_E_NONE;
}


/*
 * Function:
 *      bcm_udf_pkt_format_info_get
 * Purpose:
 *      Retrieve packet format info given the packet format Id
 * Parameters:
 *      unit - (IN) Unit number.
 *      pkt_format_id - (IN) Packet format ID
 *      pkt_format - (OUT) UDF packet format info structure
 * Returns:
 *      BCM_E_NONE          Packet format get successful.
 *      BCM_E_NOT_FOUND     Packet format entry does not exist.
 *      BCM_E_UNIT          Invalid BCM Unit number.
 *      BCM_E_INIT          UDF module not initialized.
 *      BCM_E_UNAVAIL       Feature not supported.
 *      BCM_E_XXX           Standard error code.
 * Notes:
 */
int
bcmi_xgs5_udf_pkt_format_info_get(
    int unit,
    bcm_udf_pkt_format_id_t pkt_format_id,
    bcm_udf_pkt_format_info_t *pkt_format_info)
{
    int rv;
    bcmi_xgs5_udf_tcam_info_t *tcam_info = NULL;

    /* UDF Module Init check */
    UDF_INIT_CHECK(unit);

    BCM_IF_NULL_RETURN_PARAM(pkt_format_info);

    /* UDF Module Lock */
    UDF_LOCK(unit);

    /* Retrieve the node indexed by pkt_format_id */
    rv = bcmi_xgs5_udf_tcam_node_get(unit, pkt_format_id, &tcam_info);
    if (BCM_FAILURE(rv)) {
        UDF_UNLOCK(unit);
        return rv;
    }

    /* Fetch the tcam entry info and convert to user-readable form */
    rv = bcmi_xgs5_udf_tcam_entry_parse(unit,
             (uint32 *)&(tcam_info->hw_buf), pkt_format_info);
    if (BCM_FAILURE(rv)) {
        UDF_UNLOCK(unit);
        return rv;
    }

    pkt_format_info->prio = tcam_info->priority;

    /* Release UDF mutex */
    UDF_UNLOCK(unit);


    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_udf_pkt_format_add
 * Purpose:
 *      Adds packet format entry to UDF object
 * Parameters:
 *      unit - (IN) Unit number.
 *      udf_id - (IN) UDF ID
 *      pkt_format_id - (IN) Packet format ID
 * Returns:
 *      BCM_E_NONE          Packet format entry added to UDF
 *                          successfully.
 *      BCM_E_NOT_FOUND     UDF Id or packet format entry does not
 *                          exist.
 *      BCM_E_UNIT          Invalid BCM Unit number.
 *      BCM_E_INIT          UDF module not initialized.
 *      BCM_E_UNAVAIL       Feature not supported.
 *      BCM_E_XXX           Standard error code.
 * Notes:
 */
int
bcmi_xgs5_udf_pkt_format_add(
    int unit,
    bcm_udf_id_t udf_id,
    bcm_udf_pkt_format_id_t pkt_format_id)
{
    int rv;
    int base = 0;
    int offset = 0;
    bcmi_xgs5_udf_offset_entry_t *offset_entry;
    bcmi_xgs5_udf_offset_info_t *offset_info = NULL;
    bcmi_xgs5_udf_tcam_info_t *tcam_info = NULL;

    /* UDF Module Init check */
    UDF_INIT_CHECK(unit);

    /* Sanitize input params */
    UDF_ID_VALIDATE(udf_id);
    UDF_PKT_FORMAT_ID_VALIDATE(pkt_format_id);

    /* UDF Module Lock */
    UDF_LOCK(unit);


    /* Retrieve the node indexed by pkt_format_id */
    rv = bcmi_xgs5_udf_tcam_node_get(unit, pkt_format_id, &tcam_info);
    if (BCM_FAILURE(rv)) {
        UDF_UNLOCK(unit);
        return rv;
    }

    /* Read UDF info */
    rv = bcmi_xgs5_udf_offset_node_get(unit, udf_id, &offset_info);
    if (BCM_FAILURE(rv)) {
        UDF_UNLOCK(unit);
        return rv;
    }

    /* Convert udf layer+start to udf_offset info */
    rv = bcmi_xgs5_udf_layer_to_offset_base(unit, offset_info,
                                            tcam_info, &base, &offset);
    if (BCM_FAILURE(rv)) {
        UDF_UNLOCK(unit);
        return rv;
    }

    if (tcam_info->offset_bmap & offset_info->hw_bmap) {
        /* Packet format object is already using the bmap */
        UDF_UNLOCK(unit);
        return BCM_E_EXISTS;
    }

    if ((tcam_info->num_udfs + 1) >= MAX_UDF_OFFSET_CHUNKS) {
        UDF_UNLOCK(unit);
        return BCM_E_RESOURCE;
    }
    if ((offset_info->num_pkt_formats + 1) >= MAX_UDF_TCAM_ENTRIES) {
        UDF_UNLOCK(unit);
        return  BCM_E_RESOURCE;
    }

    /* Packet format is associated with the first UDF, install it */
    if ((tcam_info->num_udfs == 0)) {
        rv = bcmi_xgs5_udf_tcam_entry_insert(unit, tcam_info);
        if (BCM_FAILURE(rv)) {
            UDF_UNLOCK(unit);
            return rv;
        }
    }

    /* Add the udf offset info to the UDF_OFFSET entry */
    rv = bcmi_xgs5_udf_offset_install(unit, tcam_info->hw_idx,
                                      offset_info->hw_bmap, base, offset);
    if (BCM_FAILURE(rv)) {
        UDF_UNLOCK(unit);
        return rv;
    }

    /* set the offset_bmap */
    tcam_info->offset_bmap |= (offset_info->hw_bmap);
    tcam_info->num_udfs++;
    offset_info->num_pkt_formats++;

    tcam_info->offset_info_list[offset_info->grp_id] = offset_info;

    offset_entry = &(UDF_CTRL(unit)->offset_entry_array[(offset_info->grp_id)]);
    offset_entry->num_pkt_formats++;

    /* Release UDF mutex */
    UDF_UNLOCK(unit);

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_udf_pkt_format_delete
 * Purpose:
 *      Deletes packet format spec associated with the UDF
 * Parameters:
 *      unit - (IN) Unit number.
 *      udf_id - (IN) UDF ID
 *      pkt_format_id - (IN) Packet format ID
 * Returns:
 *      BCM_E_NONE          Packet format configuration successfully
 *                          deleted from UDF.
 *      BCM_E_NOT_FOUND     UDF Id or packet format entry does not
 *                          exist.
 *      BCM_E_UNIT          Invalid BCM Unit number.
 *      BCM_E_INIT          UDF module not initialized.
 *      BCM_E_UNAVAIL       Feature not supported.
 *      BCM_E_XXX           Standard error code.
 * Notes:
 */
int
bcmi_xgs5_udf_pkt_format_delete(
    int unit,
    bcm_udf_id_t udf_id,
    bcm_udf_pkt_format_id_t pkt_format_id)
{
    int rv;
    bcmi_xgs5_udf_tcam_entry_t *tcam_entry;
    bcmi_xgs5_udf_offset_entry_t *offset_entry;
    bcmi_xgs5_udf_offset_info_t *offset_info = NULL;
    bcmi_xgs5_udf_tcam_info_t *tcam_info = NULL;

    /* UDF Module Init check */
    UDF_INIT_CHECK(unit);

    /* Sanitize input params */
    UDF_ID_VALIDATE(udf_id);
    UDF_PKT_FORMAT_ID_VALIDATE(pkt_format_id);

    /* UDF Module Lock */
    UDF_LOCK(unit);

    /* Retrieve the node indexed by pkt_format_id */
    rv = bcmi_xgs5_udf_tcam_node_get(unit, pkt_format_id, &tcam_info);
    if (BCM_FAILURE(rv)) {
        UDF_UNLOCK(unit);
        return rv;
    }

    /* Read UDF info */
    rv = bcmi_xgs5_udf_offset_node_get(unit, udf_id, &offset_info);
    if (BCM_FAILURE(rv)) {
        UDF_UNLOCK(unit);
        return rv;
    }

    /* Either of the entry is not installed to the hardware yet */
    if ((tcam_info->num_udfs == 0) || (offset_info->num_pkt_formats == 0)) {
        UDF_UNLOCK(unit);
        return BCM_E_CONFIG;
    }

    /* Unrelated packet format id and udf id */
    if (!(tcam_info->offset_bmap & offset_info->hw_bmap)) {
        UDF_UNLOCK(unit);
        return BCM_E_CONFIG;
    }

    tcam_entry = &(UDF_CTRL(unit)->tcam_entry_array[(tcam_info->hw_idx)]);
    offset_entry = &(UDF_CTRL(unit)->offset_entry_array[(offset_info->grp_id)]);

    offset_entry->num_pkt_formats -= 1;
    /* tcam_entry->num_pkt_formats -= 1; */

    tcam_info->offset_info_list[offset_info->grp_id] = NULL;

    /* Packet format is dissociated with the first UDF, uninstall it */
    if ((tcam_info->num_udfs == 0)) {

        /* Install the entry into hardware */
        rv = soc_mem_write(unit, UDF_CTRL(unit)->tcam_mem,
                           MEM_BLOCK_ALL, tcam_info->hw_idx,
                           soc_mem_entry_null(unit, UDF_CTRL(unit)->tcam_mem));

        UDF_UNLOCK(unit);
        SOC_IF_ERROR_RETURN(rv);

        tcam_entry->valid = 0;
    }

    /* clear the offset_bmap */
    tcam_info->offset_bmap &= ~(offset_info->hw_bmap);

    /* Remove the udf offset info to the UDF_OFFSET entry */
    rv = bcmi_xgs5_udf_offset_uninstall(unit, tcam_info->hw_idx,
                                        offset_info->hw_bmap);
    if (BCM_FAILURE(rv)) {
        UDF_UNLOCK(unit);
        return rv;
    }

    /* Release UDF mutex */
    UDF_UNLOCK(unit);

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_udf_pkt_format_get_all
 * Purpose:
 *      Retrieves the user defined format specification configuration
 *      from UDF
 * Parameters:
 *      unit - (IN) Unit number.
 *      udf_id - (IN) UDF ID
 *      max - (IN) Max Packet formats attached to a UDF object
 *      pkt_format_id_list - (OUT) List of packet format entries added to udf id
 *      actual - (OUT) Actual number of Packet formats retrieved
 * Returns:
 *      BCM_E_NONE          Success.
 *      BCM_E_NOT_FOUND     Either the UDF or packet format entry does
 *                          not exist.
 *      BCM_E_UNIT          Invalid BCM Unit number.
 *      BCM_E_INIT          UDF module not initialized.
 *      BCM_E_UNAVAIL       Feature not supported.
 *      BCM_E_XXX           Standard error code.
 * Notes:
 */
int
bcmi_xgs5_udf_pkt_format_get_all(
    int unit,
    bcm_udf_id_t udf_id,
    int max,
    bcm_udf_pkt_format_id_t *pkt_format_id_list,
    int *actual)
{
    int i;
    int rv;
    bcmi_xgs5_udf_offset_info_t *offset_info = NULL;
    bcmi_xgs5_udf_offset_info_t *offset_match = NULL;
    bcmi_xgs5_udf_tcam_info_t *tcam_info = NULL;

    /* UDF Module Init check */
    UDF_INIT_CHECK(unit);

    BCM_IF_NULL_RETURN_PARAM(actual);

    /* Sanitize input params */
    UDF_ID_VALIDATE(udf_id);

    /* UDF Module Lock */
    UDF_LOCK(unit);

    /* Read UDF info */
    rv = bcmi_xgs5_udf_offset_node_get(unit, udf_id, &offset_info);
    if (BCM_FAILURE(rv)) {
        UDF_UNLOCK(unit);
        return rv;
    }

    if ((max == 0) || (pkt_format_id_list == NULL)) {
        *actual = offset_info->num_pkt_formats;
        UDF_UNLOCK(unit);
        return BCM_E_NONE;
    }

    tcam_info = UDF_CTRL(unit)->tcam_info_head;

    while (tcam_info != NULL) {

        for (i = 0; i < BCMI_XGS5_UDF_CTRL_MAX_UDF_ENTRIES(unit); i++) {

            offset_match = tcam_info->offset_info_list[i];

            if (offset_info == offset_match) {
                if (*actual < max) {
                    pkt_format_id_list[(*actual)] = tcam_info->pkt_format_id;
                }
                (*actual) += 1;

                break;
            }
        }

        tcam_info = tcam_info->next;
    }

    /* Release UDF mutex */
    UDF_UNLOCK(unit);

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_udf_pkt_format_delete_all
 * Purpose:
 *      Deletes all packet format specs associated with the UDF
 * Parameters:
 *      unit - (IN) Unit number.
 *      udf_id - (IN) UDF ID
 * Returns:
 *      BCM_E_NONE          Deletes all packet formats associated with
 *                          the UDF.
 *      BCM_E_NOT_FOUND     UDF Id does not exist.
 *      BCM_E_UNIT          Invalid BCM Unit number.
 *      BCM_E_INIT          UDF module not initialized.
 *      BCM_E_UNAVAIL       Feature not supported.
 *      BCM_E_XXX           Standard error code.
 * Notes:
 */
int
bcmi_xgs5_udf_pkt_format_delete_all(
    int unit, bcm_udf_id_t udf_id)
{
    int i;
    int rv;
    bcmi_xgs5_udf_offset_info_t *offset_info = NULL;
    bcmi_xgs5_udf_offset_info_t *offset_match = NULL;
    bcmi_xgs5_udf_tcam_info_t *tcam_info = NULL;

    /* UDF Module Init check */
    UDF_INIT_CHECK(unit);

    /* Sanitize input params */
    UDF_ID_VALIDATE(udf_id);

    /* UDF Module Lock */
    UDF_LOCK(unit);

    /* Read UDF info */
    rv = bcmi_xgs5_udf_offset_node_get(unit, udf_id, &offset_info);
    if (BCM_FAILURE(rv)) {
        UDF_UNLOCK(unit);
        return rv;
    }

    tcam_info = UDF_CTRL(unit)->tcam_info_head;

    while (tcam_info != NULL) {

        for (i = 0; i < BCMI_XGS5_UDF_CTRL_MAX_UDF_ENTRIES(unit); i++) {

            offset_match = tcam_info->offset_info_list[i];

            if (offset_info == offset_match) {

                rv = bcmi_xgs5_udf_pkt_format_delete(unit, udf_id,
                                                     tcam_info->pkt_format_id);

                if (BCM_FAILURE(rv)) {
                    UDF_UNLOCK(unit);
                    return rv;
                }
                break;
            }
        }

        tcam_info = tcam_info->next;
    }

    /* Release UDF mutex */
    UDF_UNLOCK(unit);

    return BCM_E_NONE;
}


/*
 * Function:
 *      bcm_udf_pkt_format_get
 * Purpose:
 *      Fetches all UDFs that share the common packet format entry.
 * Parameters:
 *      unit - (IN) Unit number.
 *      pkt_format_id - (IN) Packet format ID.
 *      max - (IN) Max number of UDF IDs
 *      udf_id_list - (OUT) List of UDF IDs
 *      actual - (OUT) Actual udfs retrieved
 * Returns:
 *      BCM_E_NONE          Success.
 *      BCM_E_UNIT          Invalid BCM Unit number.
 *      BCM_E_INIT          UDF module not initialized.
 *      BCM_E_UNAVAIL       Feature not supported.
 *      BCM_E_XXX           Standard error code.
 * Notes:
 */
int
bcmi_xgs5_udf_pkt_format_get(
    int unit,
    bcm_udf_pkt_format_id_t pkt_format_id,
    int max,
    bcm_udf_id_t *udf_id_list,
    int *actual)
{
    int rv;
    int i;
    bcmi_xgs5_udf_offset_info_t *offset_info = NULL;
    bcmi_xgs5_udf_tcam_info_t *tcam_info = NULL;

    /* UDF Module Init check */
    UDF_INIT_CHECK(unit);

    BCM_IF_NULL_RETURN_PARAM(actual);

    /* Sanitize input params */
    UDF_PKT_FORMAT_ID_VALIDATE(pkt_format_id);

    /* UDF Module Lock */
    UDF_LOCK(unit);


    /* Read UDF info */
    rv = bcmi_xgs5_udf_tcam_node_get(unit, pkt_format_id, &tcam_info);
    if (BCM_FAILURE(rv)) {
        UDF_UNLOCK(unit);
        return rv;
    }

    if ((udf_id_list == NULL) || (max == 0)) {
        *actual = tcam_info->num_udfs;
        UDF_UNLOCK(unit);
        return rv;
    }

    for (i = 0; i < BCMI_XGS5_UDF_CTRL_MAX_UDF_ENTRIES(unit); i++) {
        offset_info = tcam_info->offset_info_list[i];
        if (offset_info != NULL) {
            if ((*actual) < max) {
                udf_id_list[*actual] = offset_info->udf_id;
            }

            (*actual) += 1;
        }
    }

    /* Release UDF mutex */
    UDF_UNLOCK(unit);

    return BCM_E_NONE;
}


