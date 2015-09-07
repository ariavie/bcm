/*
 * $Id: extender.c 1.1 Broadcom SDK $
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
 * File:    extender.c
 * Purpose: Implements bcm_extender_forward_* APIs for Trident2.
 */

#include <soc/defs.h>
#include <sal/core/libc.h>

#if defined(BCM_TRIDENT2_SUPPORT) && defined(INCLUDE_L3)

#include <soc/mem.h>
#include <soc/ism_hash.h>

#include <bcm/error.h>

#include <bcm_int/common/multicast.h>
#include <bcm_int/esw/trident2.h>

/*
 * Purpose:
 *	Add a downstream forwarding entry
 * Parameters:
 *	unit - (IN) Device Number
 *	forward_entry - (IN) Forwarding entry
 */
int
bcm_td2_extender_forward_add(int unit, bcm_extender_forward_t *forward_entry)
{
    int rv = BCM_E_NONE;
    bcm_module_t mod_out;
    bcm_port_t port_out;
    bcm_trunk_t tgid_out;
    int id_out;
    l2x_entry_t l2x_entry;
    int lower_th, higher_th;

    if (forward_entry->name_space > 0xfff) {
        return BCM_E_PARAM;
    }

    if (!(forward_entry->flags & BCM_EXTENDER_FORWARD_MULTICAST)) {
        if (forward_entry->extended_port_vid > 0xfff) {
            return BCM_E_PARAM;
        }

        BCM_IF_ERROR_RETURN(_bcm_esw_gport_resolve(unit,
                    forward_entry->dest_port,
                    &mod_out, &port_out, &tgid_out, &id_out));
        if (-1 != id_out) {
            return BCM_E_PARAM;
        }

        sal_memset(&l2x_entry, 0, sizeof(l2x_entry));
        soc_L2Xm_field32_set(unit, &l2x_entry, VALIDf, 1);
        soc_L2Xm_field32_set(unit, &l2x_entry, KEY_TYPEf,
                TR_L2_HASH_KEY_TYPE_PE_VID);
        soc_L2Xm_field32_set(unit, &l2x_entry, PE_VID__NAMESPACEf,
                forward_entry->name_space);
        soc_L2Xm_field32_set(unit, &l2x_entry, PE_VID__ETAG_VIDf,
                forward_entry->extended_port_vid);
        if (-1 != tgid_out) {
            BCM_IF_ERROR_RETURN(_bcm_trunk_id_validate(unit, tgid_out));
            soc_L2Xm_field32_set(unit, &l2x_entry, PE_VID__DEST_TYPEf, 1);
            soc_L2Xm_field32_set(unit, &l2x_entry, PE_VID__TGIDf, tgid_out);
        } else {
            soc_L2Xm_field32_set(unit, &l2x_entry, PE_VID__MODULE_IDf, mod_out);
            soc_L2Xm_field32_set(unit, &l2x_entry, PE_VID__PORT_NUMf, port_out);
        }
        soc_L2Xm_field32_set(unit, &l2x_entry, PE_VID__STATIC_BITf, 1);

    } else { /* Multicast forward entry */
        BCM_IF_ERROR_RETURN(bcm_esw_switch_control_get(unit,
                    bcmSwitchExtenderMulticastLowerThreshold, &lower_th));
        BCM_IF_ERROR_RETURN(bcm_esw_switch_control_get(unit,
                    bcmSwitchExtenderMulticastHigherThreshold, &higher_th));
        if ((forward_entry->extended_port_vid < lower_th) ||
               (forward_entry->extended_port_vid > higher_th)) {
            return BCM_E_PARAM;
        }
        if (!_BCM_MULTICAST_IS_L2(forward_entry->dest_multicast)) {
            return BCM_E_PARAM;
        }

        sal_memset(&l2x_entry, 0, sizeof(l2x_entry));
        soc_L2Xm_field32_set(unit, &l2x_entry, VALIDf, 1);
        soc_L2Xm_field32_set(unit, &l2x_entry, KEY_TYPEf,
                TR_L2_HASH_KEY_TYPE_PE_VID);
        soc_L2Xm_field32_set(unit, &l2x_entry, PE_VID__NAMESPACEf,
                forward_entry->name_space);
        soc_L2Xm_field32_set(unit, &l2x_entry, PE_VID__ETAG_VIDf,
                forward_entry->extended_port_vid);
        soc_L2Xm_field32_set(unit, &l2x_entry, PE_VID__L2MC_PTRf,
                _BCM_MULTICAST_ID_GET(forward_entry->dest_multicast));
        soc_L2Xm_field32_set(unit, &l2x_entry, PE_VID__STATIC_BITf, 1);
    }

    soc_mem_lock(unit, L2Xm);

    if (!(forward_entry->flags & BCM_EXTENDER_FORWARD_REPLACE)) {
        rv = soc_mem_insert(unit, L2Xm, MEM_BLOCK_ALL, &l2x_entry);
        if (SOC_FAILURE(rv)) {
            soc_mem_unlock(unit, L2Xm);
            return rv;
        }
    } else { /* Replace existing entry */
        rv = soc_mem_delete(unit, L2Xm, MEM_BLOCK_ALL, &l2x_entry);
        if (SOC_FAILURE(rv)) {
            soc_mem_unlock(unit, L2Xm);
            return rv;
        }
        rv = soc_mem_insert(unit, L2Xm, MEM_BLOCK_ALL, &l2x_entry);
        if (SOC_FAILURE(rv)) {
            soc_mem_unlock(unit, L2Xm);
            return rv;
        }
    }

    soc_mem_unlock(unit, L2Xm);

    return rv;
}

/*
 * Purpose:
 *	Delete a downstream forwarding entry
 * Parameters:
 *      unit - (IN) Device Number
 *      forward_entry - (IN) Forwarding entry
 */
int
bcm_td2_extender_forward_delete(int unit, bcm_extender_forward_t *forward_entry)
{
    int rv = BCM_E_NONE;
    l2x_entry_t l2x_entry;
    int lower_th, higher_th;

    if (forward_entry->name_space > 0xfff) {
        return BCM_E_PARAM;
    }
    if (!(forward_entry->flags & BCM_EXTENDER_FORWARD_MULTICAST)) {
        if (forward_entry->extended_port_vid > 0xfff) {
            return BCM_E_PARAM;
        }
    } else { /* Multicast forward entry */
        BCM_IF_ERROR_RETURN(bcm_esw_switch_control_get(unit,
                bcmSwitchExtenderMulticastLowerThreshold, &lower_th));
        BCM_IF_ERROR_RETURN(bcm_esw_switch_control_get(unit,
                    bcmSwitchExtenderMulticastHigherThreshold, &higher_th));
        if ((forward_entry->extended_port_vid < lower_th) ||
               (forward_entry->extended_port_vid > higher_th)) {
            return BCM_E_PARAM;
        }
    }

    sal_memset(&l2x_entry, 0, sizeof(l2x_entry));
    soc_L2Xm_field32_set(unit, &l2x_entry, KEY_TYPEf,
            TR_L2_HASH_KEY_TYPE_PE_VID);
    soc_L2Xm_field32_set(unit, &l2x_entry, PE_VID__NAMESPACEf,
            forward_entry->name_space);
    soc_L2Xm_field32_set(unit, &l2x_entry, PE_VID__ETAG_VIDf,
            forward_entry->extended_port_vid);
    soc_mem_lock(unit, L2Xm);
    rv = soc_mem_delete(unit, L2Xm, MEM_BLOCK_ALL, &l2x_entry);
    soc_mem_unlock(unit, L2Xm);

    return rv;
}

/*
 * Purpose:
 *	Delete all downstream forwarding entries
 * Parameters:
 *	unit - Device Number
 */
int
bcm_td2_extender_forward_delete_all(int unit)
{
    int rv = BCM_E_NONE;
    l2_bulk_match_mask_entry_t match_mask;
    l2_bulk_match_data_entry_t match_data;
    int field_len;

    sal_memset(&match_mask, 0, sizeof(match_mask));
    sal_memset(&match_data, 0, sizeof(match_data));

    soc_mem_field32_set(unit, L2_BULK_MATCH_MASKm, &match_mask, VALIDf, 1);
    soc_mem_field32_set(unit, L2_BULK_MATCH_DATAm, &match_data, VALIDf, 1);

    field_len = soc_mem_field_length(unit, L2_BULK_MATCH_MASKm, KEY_TYPEf);
    soc_mem_field32_set(unit, L2_BULK_MATCH_MASKm, &match_mask, KEY_TYPEf,
            (1 << field_len) - 1);
    soc_mem_field32_set(unit, L2_BULK_MATCH_DATAm, &match_data, KEY_TYPEf,
            TR_L2_HASH_KEY_TYPE_PE_VID);

    soc_mem_lock(unit, L2Xm);
    rv = WRITE_L2_BULK_MATCH_MASKm(unit, MEM_BLOCK_ALL, 0,
            &match_mask);
    if (BCM_SUCCESS(rv)) {
        rv = WRITE_L2_BULK_MATCH_DATAm(unit, MEM_BLOCK_ALL, 0,
                &match_data);
    }
    if (BCM_SUCCESS(rv)) {
        rv = soc_reg_field32_modify(unit, L2_BULK_CONTROLr, REG_PORT_ANY,
                ACTIONf, 1);
    }
    if (BCM_SUCCESS(rv)) {
        rv = soc_l2x_port_age(unit, L2_BULK_CONTROLr, INVALIDr);
    }
    soc_mem_unlock(unit, L2Xm);

    return rv;
}

/*
 * Purpose:
 *      Get a downstream forwarding entry
 * Parameters:
 *      unit - (IN) Device Number
 *      forward_entry - (IN/OUT) Forwarding entry
 */
int
bcm_td2_extender_forward_get(int unit, bcm_extender_forward_t *forward_entry)
{
    int rv = BCM_E_NONE;
    l2x_entry_t l2x_entry, l2x_entry_out;
    int idx;
    _bcm_gport_dest_t dest;
    int l2mc_index;
    int lower_th, higher_th;

    if (forward_entry->name_space > 0xfff) {
        return BCM_E_PARAM;
    }
    if (!(forward_entry->flags & BCM_EXTENDER_FORWARD_MULTICAST)) {
        if (forward_entry->extended_port_vid > 0xfff) {
            return BCM_E_PARAM;
        }
    } else { /* Multicast forward entry */
        BCM_IF_ERROR_RETURN(bcm_esw_switch_control_get(unit,
                    bcmSwitchExtenderMulticastLowerThreshold, &lower_th));
        BCM_IF_ERROR_RETURN(bcm_esw_switch_control_get(unit,
                    bcmSwitchExtenderMulticastHigherThreshold, &higher_th));
        if ((forward_entry->extended_port_vid < lower_th) ||
               (forward_entry->extended_port_vid > higher_th)) {
            return BCM_E_PARAM;
        }
    }

    sal_memset(&l2x_entry, 0, sizeof(l2x_entry));
    soc_L2Xm_field32_set(unit, &l2x_entry, KEY_TYPEf,
            TR_L2_HASH_KEY_TYPE_PE_VID);
    soc_L2Xm_field32_set(unit, &l2x_entry, PE_VID__NAMESPACEf,
            forward_entry->name_space);
    soc_L2Xm_field32_set(unit, &l2x_entry, PE_VID__ETAG_VIDf,
            forward_entry->extended_port_vid);
    soc_mem_lock(unit, L2Xm);
    rv = soc_mem_search(unit, L2Xm, MEM_BLOCK_ALL, &idx, &l2x_entry,
            &l2x_entry_out, 0);
    soc_mem_unlock(unit, L2Xm);

    if (SOC_SUCCESS(rv)) {
        if (!(forward_entry->flags & BCM_EXTENDER_FORWARD_MULTICAST)) {
            if (soc_L2Xm_field32_get(unit, &l2x_entry_out,
                        PE_VID__DEST_TYPEf)) {
                dest.tgid = soc_L2Xm_field32_get(unit, &l2x_entry_out,
                    PE_VID__TGIDf);
                dest.gport_type = _SHR_GPORT_TYPE_TRUNK;
            } else {
                dest.modid = soc_L2Xm_field32_get(unit, &l2x_entry_out,
                        PE_VID__MODULE_IDf);
                dest.port = soc_L2Xm_field32_get(unit, &l2x_entry_out,
                        PE_VID__PORT_NUMf);
                dest.gport_type = _SHR_GPORT_TYPE_MODPORT;
            }
            BCM_IF_ERROR_RETURN(_bcm_esw_gport_construct(unit, &dest,
                        &(forward_entry->dest_port)));
        } else {
            l2mc_index = soc_L2Xm_field32_get(unit, &l2x_entry_out,
                    PE_VID__L2MC_PTRf);
            _BCM_MULTICAST_GROUP_SET(forward_entry->dest_multicast,
                    _BCM_MULTICAST_TYPE_L2, l2mc_index);
        }
    }

    return rv;
}

/*
 * Purpose:
 *      Traverse all valid downstream forwarding entries and call the
 *      supplied callback routine.
 * Parameters:
 *      unit      - Device Number
 *      cb        - User callback function, called once per forwarding entry.
 *      user_data - cookie
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_td2_extender_forward_traverse(int unit,
                             bcm_extender_forward_traverse_cb cb,
                             void *user_data)
{
    int rv = BCM_E_NONE;
    int chunk_entries, chunk_bytes;
    uint8 *l2_tbl_chunk = NULL;
    int chunk_idx_min, chunk_idx_max;
    int ent_idx;
    l2x_entry_t *l2x_entry;
    bcm_extender_forward_t forward_entry;
    int l2mc_index;
    _bcm_gport_dest_t dest;
    int lower_th, higher_th;

    /* Get ETAG VID multicast range */
    BCM_IF_ERROR_RETURN(bcm_esw_switch_control_get(unit,
                bcmSwitchExtenderMulticastLowerThreshold, &lower_th));
    BCM_IF_ERROR_RETURN(bcm_esw_switch_control_get(unit,
                bcmSwitchExtenderMulticastHigherThreshold, &higher_th));

    chunk_entries = soc_property_get(unit, spn_L2DELETE_CHUNKS,
                                 L2_MEM_CHUNKS_DEFAULT);
    chunk_bytes = 4 * SOC_MEM_WORDS(unit, L2Xm) * chunk_entries;
    l2_tbl_chunk = soc_cm_salloc(unit, chunk_bytes, "extender forward traverse");
    if (NULL == l2_tbl_chunk) {
        return BCM_E_MEMORY;
    }
    for (chunk_idx_min = soc_mem_index_min(unit, L2Xm); 
         chunk_idx_min <= soc_mem_index_max(unit, L2Xm); 
         chunk_idx_min += chunk_entries) {

        /* Read a chunk of L2 table */
        sal_memset(l2_tbl_chunk, 0, chunk_bytes);
        chunk_idx_max = chunk_idx_min + chunk_entries - 1;
        if (chunk_idx_max > soc_mem_index_max(unit, L2Xm)) {
            chunk_idx_max = soc_mem_index_max(unit, L2Xm);
        }
        rv = soc_mem_read_range(unit, L2Xm, MEM_BLOCK_ANY,
                                chunk_idx_min, chunk_idx_max, l2_tbl_chunk);
        if (SOC_FAILURE(rv)) {
            break;
        }
        for (ent_idx = 0;
             ent_idx <= (chunk_idx_max - chunk_idx_min);
             ent_idx++) {
            l2x_entry = soc_mem_table_idx_to_pointer(unit, L2Xm, l2x_entry_t *, 
                                             l2_tbl_chunk, ent_idx);
            if (soc_L2Xm_field32_get(unit, l2x_entry, VALIDf) == 0) {
                continue;
            }
            if (soc_L2Xm_field32_get(unit, l2x_entry, KEY_TYPEf) != 
                    TR_L2_HASH_KEY_TYPE_PE_VID) {
                continue;
            }

            /* Get fields of forwarding entry */
            bcm_extender_forward_t_init(&forward_entry);
            forward_entry.name_space = soc_L2Xm_field32_get(unit, l2x_entry,
                    PE_VID__NAMESPACEf);
            forward_entry.extended_port_vid = soc_L2Xm_field32_get(unit,
                    l2x_entry, PE_VID__ETAG_VIDf);
            if ((forward_entry.extended_port_vid >= lower_th) &&
                (forward_entry.extended_port_vid <= higher_th)) {
                /* Multicast forward entry */
                forward_entry.flags |= BCM_EXTENDER_FORWARD_MULTICAST;
                l2mc_index = soc_L2Xm_field32_get(unit, l2x_entry,
                        PE_VID__L2MC_PTRf);
                _BCM_MULTICAST_GROUP_SET(forward_entry.dest_multicast,
                        _BCM_MULTICAST_TYPE_L2, l2mc_index);
            } else {
                if (soc_L2Xm_field32_get(unit, l2x_entry,
                            PE_VID__DEST_TYPEf)) {
                    dest.tgid = soc_L2Xm_field32_get(unit, l2x_entry,
                            PE_VID__TGIDf);
                    dest.gport_type = _SHR_GPORT_TYPE_TRUNK;
                } else {
                    dest.modid = soc_L2Xm_field32_get(unit, l2x_entry,
                            PE_VID__MODULE_IDf);
                    dest.port = soc_L2Xm_field32_get(unit, l2x_entry,
                            PE_VID__PORT_NUMf);
                    dest.gport_type = _SHR_GPORT_TYPE_MODPORT;
                }
                rv = _bcm_esw_gport_construct(unit, &dest,
                        &(forward_entry.dest_port));
                if (BCM_FAILURE(rv)) {
                    break;
                }
            }

            rv = cb(unit, &forward_entry, user_data);
            if (BCM_FAILURE(rv)) {
                break;
            }
        }
        if (BCM_FAILURE(rv)) {
            break;
        }
    }
    soc_cm_sfree(unit, l2_tbl_chunk);

    return rv;
}

#endif /* BCM_TRIDENT2_SUPPORT && INCLUDE_L3 */
