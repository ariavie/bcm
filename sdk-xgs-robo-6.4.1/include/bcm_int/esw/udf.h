/*
 * $Id: a017e713333b18ad3ec624f67007b82aa0f5da69 $
 *
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
 */

#include <bcm/udf.h>
#include <bcm/field.h>
#include <bcm/types.h>

#include <soc/field.h>
#include <soc/mcm/memregs.h>

#include<sal/core/sync.h>

#define BCMI_XGS5_UDF_ID_MIN 0x1
#define BCMI_XGS5_UDF_ID_MAX 0xffff

#define BCMI_XGS5_UDF_PKT_FORMAT_ID_MIN 0x1
#define BCMI_XGS5_UDF_PKT_FORMAT_ID_MAX 0xffff

/* Offset entry flags */
#define BCMI_XGS5_UDF_OFFSET_ENTRY_GROUP    0x1
#define BCMI_XGS5_UDF_OFFSET_ENTRY_MEMBER  0x2
#define BCMI_XGS5_UDF_OFFSET_ENTRY_SHARED   0x4
#define BCMI_XGS5_UDF_OFFSET_ENTRY_HALF   0x8

/* Offset Support flags */
#define BCMI_XGS5_UDF_CTRL_MODULE_INITIALIZED (1 << 1)
#define BCMI_XGS5_UDF_CTRL_OFFSET_ADJUST_IP4  (1 << 2)
#define BCMI_XGS5_UDF_CTRL_OFFSET_ADJUST_GRE  (1 << 3)

#define BCMI_XGS5_UDF_CTRL_TCAM_HIGIG         (1 << 4)
#define BCMI_XGS5_UDF_CTRL_TCAM_VNTAG         (1 << 5)
#define BCMI_XGS5_UDF_CTRL_TCAM_ETAG          (1 << 6)
#define BCMI_XGS5_UDF_CTRL_TCAM_ICNM          (1 << 7)
#define BCMI_XGS5_UDF_CTRL_TCAM_CNTAG         (1 << 8)
#define BCMI_XGS5_UDF_CTRL_TCAM_SUBPORT_TAG   (1 << 9)

/* Offset Base */
#define BCMI_XGS5_UDF_OFFSET_BASE_START_OF_MH        (1 << 0)
#define BCMI_XGS5_UDF_OFFSET_BASE_START_OF_L2        (1 << 1)
#define BCMI_XGS5_UDF_OFFSET_BASE_START_OF_OUTER_L3  (1 << 2)
#define BCMI_XGS5_UDF_OFFSET_BASE_START_OF_INNER_L3  (1 << 3)
#define BCMI_XGS5_UDF_OFFSET_BASE_START_OF_L4        (1 << 4)

/* Offset entry flags */
#define BCMI_XGS5_UDF_OFFSET_INSTALLED  (1 << 0)
#define BCMI_XGS5_UDF_OFFSET_IFP        (1 << 1)
#define BCMI_XGS5_UDF_OFFSET_VFP        (1 << 2)
#define BCMI_XGS5_UDF_OFFSET_FLEXHASH   (1 << 3)

/* TD2 Maximums */
#define MAX_UDF_TCAM_ENTRIES 512
#define MAX_UDF_OFFSET_CHUNKS 16

#define UDF_CTRL(u) udf_control[u]
#define UDF_CTRL_FLAGS(u) udf_control[u]->flags

#define BCMI_XGS5_UDF_ID_RUNNING(u) \
    (++udf_control[u]->udf_id_running)
#define BCMI_XGS5_UDF_PKT_FORMAT_ID_RUNNING(u) \
    (++udf_control[u]->udf_pkt_format_id_running)

#define BCMI_XGS5_UDF_CTRL_OFFSET_GRAN(u)     udf_control[u]->gran
#define BCMI_XGS5_UDF_CTRL_MAX_UDF_CHUNKS(u)  udf_control[u]->noffsets
#define BCMI_XGS5_UDF_CTRL_MAX_UDF_ENTRIES(u) udf_control[u]->nentries

#define BCMI_XGS5_UDF_OFFSET_INFO_SHARED 0x1

#define UDF_LOCK(u) sal_mutex_take((UDF_CTRL(u)->udf_mutex), sal_mutex_FOREVER);
#define UDF_UNLOCK(u) sal_mutex_give(UDF_CTRL(u)->udf_mutex);

/* User input UDF info structure and hardware offset map */
typedef struct bcmi_xgs5_udf_offset_info_s {
    bcm_udf_id_t    udf_id;
    bcm_udf_layer_t layer;
    uint16          start;
    uint16          width;

    uint8 flags;
    uint8 grp_id;         /* First chunk when group of chunks in use     */
    uint32 hw_bmap;       /* HW bitmap : 1 bit for each chunk            */
    uint32 byte_bmap;     /* Byte granular bitmap: 2 bits for each chunk */

    int num_pkt_formats;  /* Number of packet formats associated         */

    struct bcmi_xgs5_udf_offset_info_s *next;
    struct bcmi_xgs5_udf_offset_info_s *prev;
} bcmi_xgs5_udf_offset_info_t;


typedef struct bcmi_xgs5_udf_tcam_info_s {
    bcm_udf_pkt_format_id_t pkt_format_id;    /* Packet format ID          */
    int priority;                             /* priority of the entry     */
    uint32 offset_bmap;                       /* offset chunk bitmap       */

    int hw_idx;                               /* TCAM index                */
    fp_udf_tcam_entry_t hw_buf;               /* UDF tcam buffer           */
    int num_udfs;                             /* Number of UDFs associated */

    /* List of udfs associated - used in delete_all/get_all */
    struct bcmi_xgs5_udf_offset_info_s *offset_info_list[MAX_UDF_OFFSET_CHUNKS];

    struct bcmi_xgs5_udf_tcam_info_s *next;
    struct bcmi_xgs5_udf_tcam_info_s *prev;
} bcmi_xgs5_udf_tcam_info_t;


typedef struct bcmi_xgs5_udf_tcam_entry_s {
    int valid;                              /* Entry Valid/Installed */
    bcmi_xgs5_udf_tcam_info_t   *tcam_info; /* Pointer to tcam info  */
} bcmi_xgs5_udf_tcam_entry_t;

typedef struct bcmi_xgs5_udf_offset_entry_s {
    uint8 flags;            /* BCMI_XGS5_UDF_OFFSET_ENTRY_XXX                 */
    uint8 grp_id;           /* first chunk id in an offset group              */
    uint8 num_udfs;         /* Number of (shared) udfs using the offset chunk */
    uint8 num_pkt_formats;  /* Num of pkt formats currently using             */
} bcmi_xgs5_udf_offset_entry_t;

typedef struct bcmi_xgs5_udf_ctrl_s {
    soc_mem_t offset_mem;  /* offset memory */
    soc_mem_t tcam_mem;    /* tcam memory   */
    sal_mutex_t udf_mutex; /* UDF lock      */

    uint32 flags;          /* misc. info    */

    uint8 gran;           /* UDF chunk granularity (in bytes) */
    uint8 noffsets;       /* Number of offsets per entry      */
    uint16 nentries;      /* Number of entries in UDF table   */

    uint16 max_udfs;      /* Restrict max udfs (for WB)       */

    uint16 num_udfs;        /* Number of UDFs existing in the system     */
    uint16 num_pkt_formats; /* Number of packet formats installed so far */

    /* UDF & Packet format ID running trackers */
    uint16 udf_id_running; 
    uint16 udf_pkt_format_id_running;

    uint32 hw_bmap;   /* HW bitmap : 1 bit for each chunk */
    uint32 byte_bmap; /* Byte granular bitmap: 2 bits for each chunk */

    /* Pointers to tcam/offset entry arrays */
    bcmi_xgs5_udf_tcam_entry_t *tcam_entry_array;
    bcmi_xgs5_udf_offset_entry_t *offset_entry_array;

    /* Pointers to tcam/offset nodes */
    bcmi_xgs5_udf_offset_info_t *offset_info_head;
    bcmi_xgs5_udf_tcam_info_t *tcam_info_head;
} bcmi_xgs5_udf_ctrl_t;

/* UDF module initialized state */
extern int _bcm_xgs_udf_init[BCM_MAX_NUM_UNITS];

extern bcmi_xgs5_udf_ctrl_t *udf_control[BCM_MAX_NUM_UNITS];

#define UDF_INIT_CHECK(unit) \
    do {                                        \
        if (udf_control[unit] == NULL) {        \
            return BCM_E_INIT;                  \
        }                                       \
    } while(0)


/* Extern function declarations */
extern int bcmi_xgs5_udf_init(int unit);
extern int bcmi_xgs5_udf_detach(int unit);

#ifdef BCM_WARM_BOOT_SUPPORT
extern int _bcm_esw_udf_scache_sync(int unit);
extern int bcmi_xgs5_udf_wb_sync(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT */

extern int bcmi_xgs5_udf_tcam_node_get(int unit,
    bcm_udf_pkt_format_id_t pkt_format_id,
    bcmi_xgs5_udf_tcam_info_t **cur);

extern int bcmi_xgs5_udf_offset_node_get(int unit,
    bcm_udf_id_t udf_id,
    bcmi_xgs5_udf_offset_info_t **cur);

extern int bcm_xgs5_udf_detach(int unit);

extern int bcm_xgs5_udf_init(int unit);

extern int bcmi_xgs5_udf_create(int unit, bcm_udf_alloc_hints_t *hints,
    bcm_udf_t *udf_info, bcm_udf_id_t *udf_id);

extern int bcmi_xgs5_udf_get(int unit,
    bcm_udf_id_t udf_id, bcm_udf_t *udf_info);

extern int bcmi_xgs5_udf_destroy(int unit, bcm_udf_id_t udf_id);

extern int bcmi_xgs5_udf_get_all(int unit, int max,
    bcm_udf_id_t *udf_list, int *actual);

extern int bcmi_xgs5_udf_pkt_format_create(int unit,
    bcm_udf_pkt_format_options_t options,
    bcm_udf_pkt_format_info_t *pkt_format_info,
    bcm_udf_pkt_format_id_t *pkt_format_id);

extern int bcmi_xgs5_udf_pkt_format_destroy(int unit,
    bcm_udf_pkt_format_id_t pkt_format_id);

extern int bcmi_xgs5_udf_pkt_format_info_get(int unit,
    bcm_udf_pkt_format_id_t pkt_format_id,
    bcm_udf_pkt_format_info_t *pkt_format_info);

extern int bcmi_xgs5_udf_pkt_format_add(int unit,
    bcm_udf_id_t udf_id, bcm_udf_pkt_format_id_t pkt_format_id);

extern int bcmi_xgs5_udf_pkt_format_delete(int unit,
    bcm_udf_id_t udf_id, bcm_udf_pkt_format_id_t pkt_format_id);

extern int bcmi_xgs5_udf_pkt_format_delete_all(int unit, bcm_udf_id_t udf_id);

extern int bcmi_xgs5_udf_pkt_format_get(int unit,
    bcm_udf_pkt_format_id_t pkt_format_id,
    int max, bcm_udf_id_t *udf_id_list, int *actual);

extern int bcmi_xgs5_udf_pkt_format_get_all(int unit,
    bcm_udf_id_t udf_id, int max,
    bcm_udf_pkt_format_id_t *pkt_format_id_list, int *actual);


