/*
 * $Id: l2x.c,v 1.35 Broadcom SDK $
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
 * XGS GH L2 Table Manipulation API routines.
 *
 * A low-level L2 shadow table is optionally kept in soc->arlShadow by
 * using a callback to get all inserts/deletes from the l2xmsg task.  It
 * can be disabled by setting the l2xmsg_avl property to 0.
 */

#include <sal/core/libc.h>
#include <shared/bsl.h>
#include <soc/drv.h>
#include <soc/l2x.h>
#include <soc/ptable.h>
#include <soc/debug.h>
#include <soc/util.h>
#include <soc/mem.h>
#include <soc/greyhound.h>

#ifdef BCM_GREYHOUND_SUPPORT


#define _SOC_GH_L2_TABLE_DMA_CHUNK_SIZE 1024



/**************************************************
 * @function soc_gh_l2_entry_limit_count_update
 *          
 * 
 * @purpose Read L2 table from hardware, calculate 
 *          and restore port/trunk and vlan/vfi 
 *          mac counts. 
 * 
 * @param  unit   @b{(input)}  unit number
 *
 * @returns BCM_E_NONE
 * @returns BCM_E_FAIL
 * 
 * @comments This rountine gets called for an 
 *           L2 table SER event, if any of the
 *           mac limits(system, port, vlan) are
 *           enabled on the device.  
 * 
 * @end
 *************************************************/

int 
soc_gh_l2_entry_limit_count_update(int unit)
{
    uint32 entry[SOC_MAX_MEM_WORDS], rval;
    uint32 *v_entry;     /* vlan or vfi count entry */
    uint32 *p_entry;     /* port or trunk count entry */
    uint32 *l2;          /* l2 entry */
    int32 v_index = -1;  /* vlan index */
    int32 p_index = -1;  /* port index */
    uint32 s_count = 0;  /* system mac count */
    uint32 p_count = 0;  /* port or trunk mac count */
    uint32 v_count = 0;  /* vlan or vfi mac count */

    /* l2 table */     
    int index_min, index_max; 
    int entry_dw, entry_sz;
    int chnk_idx, chnk_idx_max, table_chnk_sz, idx;
    int chunk_size = _SOC_GH_L2_TABLE_DMA_CHUNK_SIZE;

    /* port or trunk mac count table */
    int ptm_index_min, ptm_index_max; 
    int ptm_table_sz, ptm_entry_dw, ptm_entry_sz;

    /* vlan or vfi mac count table */
    int vvm_index_min, vvm_index_max; 
    int vvm_table_sz, vvm_entry_dw, vvm_entry_sz;

    uint32 *buf = NULL;
    uint32 *ptm_buf = NULL; 
    uint32 *vvm_buf = NULL;

    uint32 dest_type, key_type;
    uint32 is_valid, is_limit_counted = 0;
    soc_mem_t mem;
    int rv;

    p_index = -1;
    v_index = -1;


    /* If MAC learn limit not enabled do nothing */
    SOC_IF_ERROR_RETURN(READ_SYS_MAC_LIMIT_CONTROLr(unit, &rval));
    if (!soc_reg_field_get(unit, SYS_MAC_LIMIT_CONTROLr, 
        rval, ENABLEf)) {
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "\nMAC limits not enabled.\n"))); 
        return SOC_E_NONE;
    }


    /* 
     * Mac limits are enabled.
     * 1. Read (DMA) L2 table to recalculate mac count
     * 2. Iterate through all the entries.
     * 3. if the entry is limit_counted
     * Check if KEY_TYPE is BRIDGE, VLAN or VFI 
     * based on DEST_TYPE determine if Trunk or ModPort
     * Get port index into port_or_trunk_mac_count table
     * Get vlan/vfi index into vlan_or_vfi_mac_count table
     * update the port, vlan/vfi, system mac count.
     * Write(slam) port_or_trunk_mac count and 
     * vlan_or_vfi_mac_count tables.
     */

    mem = L2Xm;
    SOC_L2X_MEM_LOCK(unit);

    /*
     * Allocate memory for port/trunk mac count table to store 
     * the updated count.
     */
    ptm_entry_dw = soc_mem_entry_words(unit, PORT_OR_TRUNK_MAC_COUNTm);
    ptm_entry_sz = ptm_entry_dw * sizeof(uint32); 
    ptm_index_min = soc_mem_index_min(unit, PORT_OR_TRUNK_MAC_COUNTm);
    ptm_index_max = soc_mem_index_max(unit,PORT_OR_TRUNK_MAC_COUNTm);
    ptm_table_sz = (ptm_index_max - ptm_index_min + 1) * ptm_entry_sz;
    
    ptm_buf = soc_cm_salloc(unit, ptm_table_sz, "ptm_tmp");

    if (ptm_buf == NULL) {
        SOC_L2X_MEM_UNLOCK(unit);
        LOG_ERROR(BSL_LS_SOC_L2,
                  (BSL_META_U(unit,
                              "soc_gh_l2_entry_limit_count_update: "
                              "Memory allocation failed for port mac count\n")));
        return SOC_E_MEMORY; 
    }

    sal_memset(ptm_buf, 0, ptm_table_sz);

    /*
     * Allocate memory for vlan/vfi mac count table to store 
     * the updated count.
     */
    vvm_entry_dw = soc_mem_entry_words(unit, VLAN_OR_VFI_MAC_COUNTm);
    vvm_entry_sz = vvm_entry_dw * sizeof(uint32); 
    vvm_index_min = soc_mem_index_min(unit, VLAN_OR_VFI_MAC_COUNTm);
    vvm_index_max = soc_mem_index_max(unit,VLAN_OR_VFI_MAC_COUNTm);
    vvm_table_sz = (vvm_index_max - vvm_index_min + 1) * vvm_entry_sz;
    
    vvm_buf = soc_cm_salloc(unit, vvm_table_sz, "vvm_tmp");

    if (vvm_buf == NULL) {
        SOC_L2X_MEM_UNLOCK(unit);
        LOG_ERROR(BSL_LS_SOC_L2,
                  (BSL_META_U(unit,
                              "soc_gh_l2_entry_limit_count_update: "
                              "Memory allocation failed for vlan mac count\n")));
        return SOC_E_MEMORY; 
    }

    sal_memset(vvm_buf, 0, vvm_table_sz);

    /* Create a copy L2Xm table for calculating mac count */
    entry_dw = soc_mem_entry_words(unit, mem);
    entry_sz = entry_dw * sizeof(uint32); 
    index_min = soc_mem_index_min(unit, mem);
    index_max = soc_mem_index_max(unit, mem);
    table_chnk_sz = chunk_size * entry_sz;
    
    buf = soc_cm_salloc(unit, table_chnk_sz, "l2x_tmp");

    if (buf == NULL) {
        SOC_L2X_MEM_UNLOCK(unit);
        LOG_ERROR(BSL_LS_SOC_L2,
                  (BSL_META_U(unit,
                              "soc_gh_l2_entry_limit_count_update: "
                              "Memory allocation failed for %s\n"), 
                   SOC_MEM_NAME(unit, mem)));
        return SOC_E_MEMORY; 
    }

    for (chnk_idx = index_min; chnk_idx <= index_max; chnk_idx += chunk_size) {

         sal_memset(buf, 0, table_chnk_sz);

         chnk_idx_max = ((chnk_idx + chunk_size) <= index_max) ?
                (chnk_idx + chunk_size - 1) : index_max;

         /* DMA L2 Table */
         rv =  soc_mem_read_range(unit, mem, MEM_BLOCK_ANY, chnk_idx, 
                                  chnk_idx_max, buf);
         if (rv < 0) {
             SOC_L2X_MEM_UNLOCK(unit);
             LOG_ERROR(BSL_LS_SOC_L2,
                       (BSL_META_U(unit,
                                   "DMA failed: %s, mac limts not synced!\n"),
                                   soc_errmsg(rv)));
             return SOC_E_FAIL;
         }                          

         /* Iter through all l2 entries in the chunk */
         for (idx = 0; idx <= (chnk_idx_max - chnk_idx); idx++) {
              l2 =  (buf + (idx * entry_dw));
         
              /* check if entry is valid */
              is_valid = soc_mem_field32_get(unit, mem, l2, VALIDf); 
              if (!is_valid) {
                  /* not valid entry skip it */
                  continue;
              } 

              /* check if entry is limit counted */
              if (soc_mem_field_valid(unit, mem, LIMIT_COUNTEDf)) {
                  is_limit_counted = soc_mem_field32_get(unit,mem, l2, 
                                                         LIMIT_COUNTEDf);
                  if (!is_limit_counted) {
                      /* entry not limit counted skip it */
                      continue;
                  }
              }

              /*
               * Get the key type of the entries
               * Process entries with only key_type
               * GH_L2_HASH_KEY_TYPE_BRIDGE
               */
              key_type = soc_mem_field32_get(unit, mem, l2, KEY_TYPEf); 

              if (key_type != GH_L2_HASH_KEY_TYPE_BRIDGE) {
                  continue; 
              } 

              dest_type = soc_mem_field32_get(unit, mem, l2, L2__DEST_TYPEf);

              switch (dest_type) {
                  case 0: /* mod port */
                      SOC_IF_ERROR_RETURN(soc_mem_read(unit, PORT_TABm, 
                                                       MEM_BLOCK_ANY, 0,
                                                       &entry));
	              if (soc_mem_field32_get(unit, PORT_TABm, 
                                              &entry, MY_MODIDf) ==
	                  soc_mem_field32_get(unit, mem, l2, L2__MODULE_IDf)) {
                          p_index = soc_mem_field32_get(unit, mem, 
                                                        l2, PORT_NUMf) + 
	            	  soc_mem_index_count(unit, TRUNK_GROUPm) ;
	              }
                      break;
                  case 1: /* trunk */
                      p_index = soc_mem_field32_get(unit, mem, l2, L2__TGIDf);
                      break;
                  default:
                      /* if VFI based key then break else continue? */
                      break;
              }

              /*
               * based on key_type get vlan or vfi
               * to index into VLAN_OR_VFI_MAC_COUNTm
               */
              v_index = soc_mem_field32_get(unit, mem, l2, L2__VLAN_IDf); 
  

             /* 
              * read and update vlan or vfi mac count
              * in buffer also update the sys mac count.
              */
             v_count = 0;
             if (v_index >= 0) {
                 v_entry =  (vvm_buf + (v_index * vvm_entry_dw)); 
                 v_count = soc_mem_field32_get(unit, VLAN_OR_VFI_MAC_COUNTm,
                                          v_entry, COUNTf);
                 s_count++;
                 v_count++;
                 soc_mem_field32_set(unit, VLAN_OR_VFI_MAC_COUNTm, 
                                     v_entry, COUNTf, v_count);
             }
    
             /* 
              * read and update port or trunk mac count
              * in buffer.
              */
             p_count = 0;
             if (p_index >= 0) {
                 p_entry = ptm_buf + (p_index * ptm_entry_dw); 
                 p_count = soc_mem_field32_get(unit, PORT_OR_TRUNK_MAC_COUNTm,
                                               p_entry, COUNTf);
                 p_count++;
                 soc_mem_field32_set(unit, PORT_OR_TRUNK_MAC_COUNTm,
                                     p_entry, COUNTf, p_count);
             }
    
         } /* End for index */
    } /* End for chnk_idx */

    /* Free memory */
    soc_cm_sfree(unit, buf);

    /* Write the tables to hardware */
     rv = soc_mem_write_range(unit, PORT_OR_TRUNK_MAC_COUNTm, MEM_BLOCK_ANY, 
                              ptm_index_min, ptm_index_max, (void *)ptm_buf);

    if (rv < 0) {
        SOC_L2X_MEM_UNLOCK(unit);
        LOG_ERROR(BSL_LS_SOC_L2,
                  (BSL_META_U(unit,
                              "PORT_OR_TRUNK_MAC_COUNT write failed: "
                              "%s, mac limts not synced!\n"),
                   soc_errmsg(rv)));
        return SOC_E_FAIL;
    }

    rv = soc_mem_write_range(unit, VLAN_OR_VFI_MAC_COUNTm, MEM_BLOCK_ANY, 
                             vvm_index_min, vvm_index_max, (void *)vvm_buf);

    if (rv < 0) {
        SOC_L2X_MEM_UNLOCK(unit);
        LOG_ERROR(BSL_LS_SOC_L2,
                  (BSL_META_U(unit,
                              "VLAN_OR_VFI_MAC_COUNT write failed: "
                              "%s, mac limts not synced!\n"),
                   soc_errmsg(rv)));
        return SOC_E_FAIL;
    }
     
    /* Update system count */
    soc_reg_field_set(unit, SYS_MAC_COUNTr, &rval, COUNTf, s_count);
    rv = WRITE_SYS_MAC_COUNTr(unit, rval);
    
    /* Free memory */
    soc_cm_sfree(unit, buf);
    soc_cm_sfree(unit, ptm_buf);
    soc_cm_sfree(unit, vvm_buf);

    SOC_L2X_MEM_UNLOCK(unit);
    return rv;
}

#endif /* BCM_GREYHOUND_SUPPORT */
