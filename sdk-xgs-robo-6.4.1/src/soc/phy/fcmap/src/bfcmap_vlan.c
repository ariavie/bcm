/*
 * $Id: bfcmap_vlan.c,v 1.1 Broadcom SDK $
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
#include <bfcmap.h>
#include <bfcmap_int.h>

/**********************************************************
 * BFCMAP VLANMAP
 *********************************************************/
#define BFCMAP_FL_BUCKETS       8
#define FLID_HASH(vlanid, vfid)  (((vlanid) + (vfid)) % BFCMAP_FL_BUCKETS)

typedef struct bfcmap_vlanmapmgr_s {
    bfcmap_int_vlanmap_entry_t     *buckets[BFCMAP_FL_BUCKETS];

    bfcmap_sal_lock_t       lock;
} bfcmap_vlanmapmgr_t;

STATIC bfcmap_vlanmapmgr_t bfcmap_vlanmapmgr;

#define BFCMAP_GET_VLANMAPMGR(mpc)     &bfcmap_vlanmapmgr

/*
 * Init Vlan Vsan Map DB.
 */
int bfcmap_int_vlanmap_init(void)
{
    BFCMAP_SAL_MEMSET(&bfcmap_vlanmapmgr, 0, sizeof(bfcmap_vlanmapmgr_t));
    bfcmap_vlanmapmgr.lock = BFCMAP_SAL_LOCK_CREATE("BFCMAP_Vlan_Map_lock");
    return 0;
}

/*
 * Create HW mapper object.
 */
bfcmap_int_hw_mapper_t *
bfcmap_int_hw_mapper_create(bfcmap_port_ctrl_t *mpc, int max_entry, 
                             buint32_t cb_arg, bfcmap_hw_mapper_vec_t *vec)
{
    bfcmap_int_hw_mapper_t          *mapper;

    BFCMAP_COMPILER_SATISFY(mpc);

    mapper = BFCMAP_SAL_MALLOC(sizeof(bfcmap_int_hw_mapper_t ), 
                                "mapper table");
    if (!mapper) {
        return NULL;
    }
    BFCMAP_SAL_MEMSET(mapper, 0, sizeof(bfcmap_int_hw_mapper_t ));
    mapper->max_entry = max_entry;
    mapper->cb_data = cb_arg;
    mapper->hw_vlanmap_tbl = BFCMAP_SAL_MALLOC(
                 sizeof(bfcmap_int_vlanmap_entry_t*)*mapper->max_entry,
                                       "bfcmap fltbl");
    BFCMAP_SAL_MEMSET(mapper->hw_vlanmap_tbl, 0,
                    sizeof(bfcmap_int_vlanmap_entry_t*)*mapper->max_entry);
    BFCMAP_SAL_MEMCPY(&mapper->vec, vec, sizeof(bfcmap_hw_mapper_vec_t));
    mapper->lock = BFCMAP_SAL_LOCK_CREATE("mp_lock");
    return mapper;
}

/*
 * Find vlanmap based on vlanid and vfid
 */
bfcmap_int_vlanmap_entry_t*
bfcmap_int_vlanmap_find_by_id(bfcmap_port_ctrl_t *mpc, int vlanid, int vfid)
{
    bfcmap_vlanmapmgr_t *flcb = BFCMAP_GET_VLANMAPMGR(mpc);
    bfcmap_int_vlanmap_entry_t    *fle;

    fle = flcb->buckets[FLID_HASH(vlanid, vfid)];
    while ((fle != NULL) && 
           (fle->pc != mpc ||
            fle->vlanmap.vlan_vid != vlanid || fle->vlanmap.vsan_vfid != vfid)){
        fle = fle->next;
    }
    return fle;
}

/*
 * Find vlanmap based on vlanid and vfid
 */
bfcmap_int_vlanmap_entry_t*
bfcmap_int_vlanmap_get_next_or_first(bfcmap_port_ctrl_t *mpc,
                           bfcmap_int_hw_mapper_t  *mapper, 
                           int vlanid, int vfid)
{
    int index;
    int first = -1;
    int match = -1;
    bfcmap_int_vlanmap_entry_t    *fle;

    for (index = 0; index < mapper->max_entry; index++) {
        if ((fle = BFCMAP_MP_ENTRY(mapper, index)) == NULL) {
            continue;
        }
        if (first < 0) {
            first = index;
        }
        if (match >= 0) {
            return fle;
        } else if (fle->vlanmap.vlan_vid == vlanid &&
                   fle->vlanmap.vsan_vfid == vfid){
            BFCMAP_SAL_ASSERT(fle->pc == mpc);
            match = index ;
        }

    }
    return (match >= 0 || first < 0) ? NULL : BFCMAP_MP_ENTRY(mapper, first) ;
}

/*
 * Add vlanmap with vlanmap-id vlanmapId.
 */
bfcmap_int_vlanmap_entry_t*
bfcmap_int_vlanmap_add_or_update(bfcmap_port_ctrl_t *mpc, 
                           bfcmap_vlan_vsan_map_t *vlanmap)
{
    bfcmap_vlanmapmgr_t *flcb = BFCMAP_GET_VLANMAPMGR(mpc);
    bfcmap_int_vlanmap_entry_t    *fle, **flb;

    if (!vlanmap) {
        return NULL;
    }

    BFCMAP_SAL_LOCK(flcb->lock);


    fle = bfcmap_int_vlanmap_find_by_id(mpc, 
            vlanmap->vlan_vid, vlanmap->vsan_vfid);

    if (fle == NULL) {
        if ((fle = BFCMAP_SAL_MALLOC(sizeof(bfcmap_int_vlanmap_entry_t), 
                                    "bfcmap vlanmap entry")) == NULL) {
            goto done;
        }
        BFCMAP_SAL_MEMSET(fle, 0, sizeof(bfcmap_int_vlanmap_entry_t));
        /*
        fle->vlanmapId = vlanmapId;
        */
        fle->pc = mpc;
        fle->hw_index = -1;

        /*
         * Add this vlanmap to DB.
         */
        flb = &flcb->buckets[FLID_HASH(vlanmap->vlan_vid, vlanmap->vsan_vfid)];
        fle->next = *flb;
        *flb = fle;
    }

    BFCMAP_SAL_ASSERT(fle);

    BFCMAP_SAL_MEMCPY(&fle->vlanmap, vlanmap, sizeof(bfcmap_vlan_vsan_map_t));

done:
    BFCMAP_SAL_UNLOCK(flcb->lock);
    return fle;
}

/*
 * Remove vlanmap with vlanid and vfid
 */
int
bfcmap_int_vlanmap_delete(bfcmap_port_ctrl_t *mpc, int vlanid, int vfid)
{
    bfcmap_int_vlanmap_entry_t    *fl, **pfl;
    bfcmap_vlanmapmgr_t *flcb = BFCMAP_GET_VLANMAPMGR(mpc);

    BFCMAP_SAL_LOCK(flcb->lock);

    pfl = &flcb->buckets[FLID_HASH(vlanid, vfid)];
    while (*pfl && 
            ((*pfl)->pc != mpc ||
             (*pfl)->vlanmap.vlan_vid  != vlanid ||
             (*pfl)->vlanmap.vsan_vfid != vfid )) {
        pfl = &(*pfl)->next;
    }

    if (*pfl == NULL) {
        BFCMAP_SAL_UNLOCK(flcb->lock);
        return BFCMAP_E_NOT_FOUND; /* No found */
    }

    fl = *pfl;
    *pfl = fl->next;

    BFCMAP_SAL_UNLOCK(flcb->lock);

    BFCMAP_SAL_FREE(fl);
    return BFCMAP_E_NONE;
}

#if 0
int 
bfcmap_int_vlanmap_traverse(bfcmap_port_ctrl_t *mpc, 
                          bfcmap_vlanmap_traverse_cb callbk,
                          void *user_data)
{
    bfcmap_vlanmapmgr_t *flcb = BFCMAP_GET_VLANMAPMGR(mpc);
    bfcmap_int_vlanmap_entry_t *vlanmap_entry, *next_vlanmap_entry;
    bfcmap_port_t   p;
    int             ii, rv = BFCMAP_E_NONE;

    p = BFCMAP_GET_PORT_ID(mpc);

    BFCMAP_LOCK_PORT(mpc);
    for (ii = 0; ii < BFCMAP_FL_BUCKETS; ii++) {
        vlanmap_entry = flcb->buckets[FLID_HASH(ii)];
        if (!vlanmap_entry) {
            continue;
        }

        while(vlanmap_entry) {
            next_vlanmap_entry = vlanmap_entry->next;
            if (vlanmap_entry->pc == mpc) {
                rv = callbk(p, &vlanmap_entry->vlanmap, &vlanmap_entry->action,
                                    vlanmap_entry->vlanmapId, user_data);
                if (rv < 0) {
                    BFCMAP_UNLOCK_PORT(mpc);
                    return rv;
                }
            }
            vlanmap_entry = next_vlanmap_entry;
        }
    }
    BFCMAP_UNLOCK_PORT(mpc);
    return rv;
}
#endif


/********************************************************************
 * Filter
 ********************************************************************/

STATIC int
_bfcmap_int_vlanmap_match_hw(bfcmap_port_ctrl_t *mpc,
                           bfcmap_int_hw_mapper_t  *mapper, 
                           bfcmap_int_vlanmap_entry_t *fle,
                           int *idx)
{
    int index;
    bfcmap_int_vlanmap_entry_t *tmp_vlanmap;

    for (index = 0; index < mapper->max_entry; index++) {
        if ((tmp_vlanmap = BFCMAP_MP_ENTRY(mapper, index)) == NULL) {
            continue;
        }
        if (BFCMAP_MP_CMP_ENTRY(mpc, mapper, tmp_vlanmap, fle) == 0) {
            *idx = index;
            return BFCMAP_E_NONE;
        }
    }
    return BFCMAP_E_NOT_FOUND;
}

STATIC int
_bfcmap_int_vlanmap_delete_from_hw(bfcmap_port_ctrl_t *mpc, 
                                 bfcmap_int_hw_mapper_t  *mapper, 
                                 bfcmap_int_vlanmap_entry_t *fle, 
                                 int idx)
{
    bfcmap_int_vlanmap_entry_t **vlanmap_entry;

    vlanmap_entry =  BFCMAP_MP_ENTRY_PTR(mapper, idx);
    while(*vlanmap_entry && (*vlanmap_entry != fle)) {
        vlanmap_entry = &(*vlanmap_entry)->hw_next;
    }

    if (*vlanmap_entry == NULL) {
        BFCMAP_SAL_ASSERT(0);
        BFCMAP_SAL_DBG_PRINTF("vlanmap entry not found in mapper vlanmap chain\n");
        return BFCMAP_E_NOT_FOUND;
    }

    *vlanmap_entry = fle->hw_next;
    fle->hw_index = -1;
    fle->hw_next = NULL;
    fle->flags &= ~BFCMAP_VLANMAP_F_INSTALLED;
    fle->mapper = NULL;

    fle = BFCMAP_MP_ENTRY(mapper, idx);
    if (fle == NULL) {
        /* Clear the HW table */
        BFCMAP_MP_CLR_ENTRY(mpc, mapper, idx, 1);
    } else {
        BFCMAP_MP_WRITE_ENTRY(mpc, mapper, idx);
    }

    return BFCMAP_E_NONE;
}

STATIC int
_bfcmap_int_add_vlanmap_to_hw(bfcmap_port_ctrl_t *mpc, 
                            bfcmap_int_hw_mapper_t  *mapper, 
                            bfcmap_int_vlanmap_entry_t *new_vlanmap, 
                            int idx)
{
    bfcmap_int_vlanmap_entry_t **vlanmap_entry;

    if (new_vlanmap->flags & BFCMAP_VLANMAP_F_INSTALLED) {
        BFCMAP_SAL_DBG_PRINTF("Vlan Vsan Map already installed in HW\n");
        return 0;
    }

    vlanmap_entry =  BFCMAP_MP_ENTRY_PTR(mapper, idx);
    while(*vlanmap_entry) {
        vlanmap_entry = &((*vlanmap_entry)->hw_next);
    }
    new_vlanmap->hw_next = *vlanmap_entry;
    *vlanmap_entry = new_vlanmap;
    new_vlanmap->hw_index = idx;
    new_vlanmap->flags |= BFCMAP_VLANMAP_F_INSTALLED;
    new_vlanmap->mapper = mapper;
   
    if (BFCMAP_MP_WRITE_ENTRY(mpc, mapper, idx)) {
        _bfcmap_int_vlanmap_delete_from_hw(mpc, mapper, new_vlanmap, idx);
    }

    return 0;
}


int
bfcmap_int_vlanmap_add_to_hw(bfcmap_port_ctrl_t *mpc, 
                           bfcmap_int_hw_mapper_t  *mapper,
                           bfcmap_int_vlanmap_entry_t *fle)
{
    bfcmap_int_vlanmap_entry_t *te;
    int     rv = BFCMAP_E_NONE;
    int     index;
    int     insert_at;


    if (fle->flags & BFCMAP_VLANMAP_F_INSTALLED) {
        /*
         * Vlan Vsan Map is already installed, maybe it needs an update.
         * But the caller should always uninstall the vlanmap
         * and then install back.
         */
        return BFCMAP_E_EXISTS;
    }

    BFCMAP_SAL_LOCK(mapper->lock);

    if ((_bfcmap_int_vlanmap_match_hw(mpc, mapper, 
                                 fle, &index) == BFCMAP_E_NONE)) {
        if (_bfcmap_int_add_vlanmap_to_hw(mpc, mapper, fle, index) < 0) {
            rv = BFCMAP_E_INTERNAL;
            goto done;
        }
        rv = BFCMAP_E_NONE;
        goto done;
    }

    /*
     * Find an empty location to program this vlanmap.
     */
    insert_at = -1;
    for (index = 0; index < mapper->max_entry; index++) {
        if ((te = BFCMAP_MP_ENTRY(mapper, index)) == NULL) {
            insert_at  = index;
            break;
        }
    }

    if (insert_at < 0) {
        rv = BFCMAP_E_RESOURCE;
        goto done;
    }

    if (_bfcmap_int_add_vlanmap_to_hw(mpc, mapper, fle, insert_at) < 0) {
        rv = BFCMAP_E_INTERNAL;
        goto done;
    }

done:
    BFCMAP_SAL_UNLOCK(mapper->lock);
    return rv;
}

int
bfcmap_int_vlanmap_delete_from_hw(bfcmap_port_ctrl_t *mpc, 
                                bfcmap_int_hw_mapper_t  *mapper,
                                bfcmap_int_vlanmap_entry_t *fle)
{
    int     fp_index;

    if ((fle->flags & BFCMAP_VLANMAP_F_INSTALLED) == 0) {
        return BFCMAP_E_INTERNAL;
    }

    BFCMAP_SAL_LOCK(mapper->lock);

    fp_index = fle->hw_index;
    if ((fp_index >= 0) && (fp_index < mapper->max_entry)) {
        if (_bfcmap_int_vlanmap_delete_from_hw(mpc, mapper, fle, fp_index) < 0) {
            BFCMAP_SAL_UNLOCK(mapper->lock);
            return BFCMAP_E_INTERNAL;
        }
    }

    BFCMAP_SAL_UNLOCK(mapper->lock);
    return BFCMAP_E_NONE;
}

