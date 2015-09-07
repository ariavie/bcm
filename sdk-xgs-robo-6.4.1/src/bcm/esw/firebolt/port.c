/*
 * $Id: port.c,v 1.217 Broadcom SDK $
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
 * File:        port.c
 * Purpose:     XGS3 port function implementations
 *
 * int bcm_xgs3_port_cfg_init(int unit, bcm_port_t port, bcm_vlan_data_t *vd)
 * int bcm_xgs3_port_cfg_get(int unit, bcm_port_t port, bcm_port_cfg_t *cfg)
 * int bcm_xgs3_port_cfg_set(int unit, bcm_port_t port, bcm_port_cfg_t *cfg)
 */

#include <soc/defs.h>

#if defined(BCM_FIREBOLT_SUPPORT)

#include <sal/core/libc.h>

#include <soc/mem.h>
#include <soc/debug.h>
#include <soc/cm.h>
#include <soc/drv.h>
#include <soc/register.h>
#include <soc/memory.h>
#include <soc/ll.h>
#include <soc/ptable.h>
#include <soc/l2x.h>

#include <bcm/port.h>
#include <bcm/mirror.h>
#include <bcm/error.h>

#include <bcm_int/common/lock.h>
#include <bcm_int/esw/mbcm.h>
#include <bcm_int/esw/firebolt.h>
#include <bcm_int/esw/triumph.h>
#include <bcm_int/esw/mirror.h>
#include <bcm_int/esw/stack.h>
#include <bcm_int/esw/port.h>

#ifdef BCM_TRIDENT_SUPPORT 
#include <bcm_int/esw/trident.h>
#endif /* BCM_TRIDENT_SUPPORT */

#ifdef BCM_TRIDENT2_SUPPORT 
#include <bcm_int/esw/trident2.h>
#endif /* BCM_TRIDENT2_SUPPORT */

#include <bcm_int/esw_dispatch.h>


/*
 * Function:
 *      _bcm_xgs3_port_table_read
 * Purpose:
 *      Read port table entry and parse it to a common 
 *      (bcm_port_cfg_t) structure.
 * Parameters:
 *      unit       - (IN)SOC unit number.
 *      port       - (IN)Port number.
 *      port_cfg   - (OUT)API port information structure.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      The following port configuration settings
 *      do not apply to FB/ER ports:
 *
 *      port_cfg->pc_frame_type
 *      port_cfg->pc_ether_type
 *      port_cfg->pc_stp_state
 *      port_cfg->pc_cpu
 *      port_cfg->pc_ptype
 *      port_cfg->pc_pbm
 *      port_cfg->pc_ut_pbm
 *      port_cfg->pc_trunk
 *      port_cfg->pc_pfm
 *    The following port configuration do not apply to ER
 *       port_cfg->pc_nni_port
 */
STATIC int
_bcm_xgs3_port_table_read(int unit, bcm_port_t port,
                          bcm_port_cfg_t *port_cfg)
{
    port_tab_entry_t ptab_entry;   /* Port table entry.             */
    int discard_tag;               /* Discard tagged packets bit.   */
    int discard_untag;             /* Discard untagged packets bit. */
    soc_mem_t mem = PORT_TABm;     /* Port table memory.            */
    int tbl_index = port;     /* Port table index.             */
    int rv;                        /* Operation return status.      */

    if (BCM_GPORT_IS_SET(port)) {
        BCM_IF_ERROR_RETURN(_bcm_esw_port_gport_validate(unit, port, &port));
    }
    tbl_index = port;

    soc_mem_lock(unit, mem);

    /* Read port table entry from HW. */
    rv = BCM_XGS3_MEM_READ(unit, mem, tbl_index, &ptab_entry);
    if (BCM_FAILURE(rv)) {
       soc_mem_unlock(unit, mem);
       return (rv);
    }

    /* If port cpu managed learning is not frozen read it from port. */
    if (!soc_feature(unit, soc_feature_no_learning) && 
        soc_l2x_frozen_cml_get(unit, port, &port_cfg->pc_cml, &port_cfg->pc_cml_move) < 0) {
#ifdef BCM_TRX_SUPPORT
        if (SOC_IS_TRX(unit)) {
            port_cfg->pc_cml = 
                soc_mem_field32_get(unit, mem, &ptab_entry, CML_FLAGS_NEWf);
            port_cfg->pc_cml_move = 
                soc_mem_field32_get(unit, mem, &ptab_entry, CML_FLAGS_MOVEf);
        } else
#endif  /* BCM_TRX_SUPPORT */
        {
            port_cfg->pc_cml = soc_mem_field32_get(unit, mem, &ptab_entry, CMLf);
        }
    }
    soc_mem_unlock(unit, mem);

    /* Get drop all tagged packets flag. */
    discard_tag = soc_mem_field32_get(unit, mem,  &ptab_entry, PORT_DIS_TAGf);

    /* Get drop all untagged packets flag. */
    discard_untag = 
        soc_mem_field32_get(unit, mem,  &ptab_entry, PORT_DIS_UNTAGf);

    if (discard_tag && discard_untag) {
        port_cfg->pc_disc = BCM_PORT_DISCARD_ALL;
    } else if (discard_tag) {
        port_cfg->pc_disc = BCM_PORT_DISCARD_TAG;
    } else if (discard_untag) {
        port_cfg->pc_disc = BCM_PORT_DISCARD_UNTAG;
    } else {
        port_cfg->pc_disc = BCM_PORT_DISCARD_NONE;
    }

    /* Get drop bpdu's on ingress flag. */
    port_cfg->pc_bpdu_disable = 
        soc_mem_field32_get(unit, mem, &ptab_entry, DROP_BPDUf);

    /* Get enable mirroring flag. */
    if (soc_feature(unit, soc_feature_mirror_flexible)) {
        /* Multi-bit field */
        port_cfg->pc_mirror_ing =
            soc_mem_field32_get(unit, mem, &ptab_entry, MIRRORf);
    } else if (!soc_feature(unit, soc_feature_no_mirror) &&
               soc_mem_field32_get(unit, mem, &ptab_entry, MIRRORf)) {
        port_cfg->pc_mirror_ing |= BCM_MIRROR_MTP_ONE; 
    }


    /* Get port default vlan id (pvid). */
    port_cfg->pc_vlan = soc_mem_field32_get(unit, mem, &ptab_entry, PORT_VIDf);

#ifdef BCM_TRX_SUPPORT
    if (SOC_IS_TRX(unit)) {
        port_cfg->pc_ivlan = soc_mem_field32_get(unit, mem, &ptab_entry, IVIDf);
        port_cfg->pc_vlan_action = soc_mem_field32_get(unit, mem, &ptab_entry, 
                                                       TAG_ACTION_PROFILE_PTRf);
    } else 
#endif /* BCM_TRX_SUPPORT */
    {
        port_cfg->pc_ivlan = 0;
        port_cfg->pc_vlan_action = 0;
    }

    /* Get L3 IPv4 forwarding enable bit. */ 
    if (SOC_MEM_FIELD_VALID(unit, mem, V4L3_ENABLEf)) {
        if (soc_mem_field32_get(unit, mem, &ptab_entry, V4L3_ENABLEf)) { 
            port_cfg->pc_l3_flags |= BCM_PORT_L3_V4_ENABLE;
        }
    }

    /* Get L3 IPv6 forwarding enable bit. */ 
    if (SOC_MEM_FIELD_VALID(unit, mem, V6L3_ENABLEf)) {
        if (soc_mem_field32_get(unit, mem, &ptab_entry, V6L3_ENABLEf)) { 
            port_cfg->pc_l3_flags |= BCM_PORT_L3_V6_ENABLE;
        }
    }

    /* Get port default priority.*/ 
    if (soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
        port_cfg->pc_new_opri =
            soc_mem_field32_get(unit, mem, &ptab_entry, OPRIf);
        port_cfg->pc_new_ocfi =
            soc_mem_field32_get(unit, mem, &ptab_entry, OCFIf);
        port_cfg->pc_new_ipri =
            soc_mem_field32_get(unit, mem, &ptab_entry, IPRIf);
        port_cfg->pc_new_icfi =
            soc_mem_field32_get(unit, mem, &ptab_entry, ICFIf);
    } else {
        port_cfg->pc_new_opri = 
            soc_mem_field32_get(unit, mem, &ptab_entry, PORT_PRIf);
    }

    /* Get incoming packet overwrite/map priority. */
    if (SOC_MEM_FIELD_VALID(unit, mem, MAP_TAG_PKT_PRIORITYf)) {
        port_cfg->pc_remap_pri_en =
            soc_mem_field32_get(unit, mem, &ptab_entry, MAP_TAG_PKT_PRIORITYf);
    } else {
        port_cfg->pc_remap_pri_en = 0;
    }

    /* Get ingress port is trusted port, trust incoming IPv4 DSCP bit. */ 
    if (SOC_MEM_FIELD_VALID(unit, mem, TRUST_DSCP_V4f)) {
        port_cfg->pc_dse_mode =
            soc_mem_field32_get(unit, mem, &ptab_entry, TRUST_DSCP_V4f);
    }

    /* Get ingress port is trusted port, trust incoming IPv6 DSCP bit. */ 
    if (SOC_MEM_FIELD_VALID(unit, mem, TRUST_DSCP_V6f)) {
        port_cfg->pc_dse_mode |=
            soc_mem_field32_get(unit, mem, &ptab_entry, TRUST_DSCP_V6f);
    }

    port_cfg->pc_dscp_prio = port_cfg->pc_dse_mode;
    port_cfg->pc_dscp = -1;

    /* Get enable ingress filtering bit. */
    port_cfg->pc_en_ifilter =
        soc_mem_field32_get(unit, mem, &ptab_entry, EN_IFILTERf);

    /* Get enable L2 bridging on the incoming port. */
    if (SOC_MEM_FIELD_VALID(unit, mem, PORT_BRIDGEf)) {
        port_cfg->pc_bridge_port = 
            soc_mem_field32_get(unit, mem, &ptab_entry, PORT_BRIDGEf);
    }

    /* Get network to network port flag. (QinQ)  */
    if (SOC_MEM_FIELD_VALID(unit, mem, NNI_PORTf)) { 
        port_cfg->pc_nni_port = 
            soc_mem_field32_get(unit, mem, &ptab_entry, NNI_PORTf);
    }
#if defined(BCM_TRX_SUPPORT) || defined(BCM_FIREBOLT2_SUPPORT)
    /* Unicast rpf mode. */
    if (SOC_MEM_FIELD_VALID(unit, mem, URPF_MODEf)) {
        port_cfg->pc_urpf_mode = soc_mem_field32_get(unit, mem, &ptab_entry,
                                                     URPF_MODEf);
    }

    /* Unicast rpf default gateway check. */
    if (SOC_MEM_FIELD_VALID(unit, mem, URPF_DEFAULTROUTECHECKf)) {
        port_cfg->pc_urpf_def_gw_check = 
            soc_mem_field32_get(unit, mem, &ptab_entry, URPF_DEFAULTROUTECHECKf);
    }
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_TRX_SUPPORT */
#if defined(BCM_TRX_SUPPORT)
    /* private VALN enable */
    if (SOC_MEM_FIELD_VALID(unit, mem, PVLAN_ENABLEf)) {
        port_cfg->pc_pvlan_enable = soc_mem_field32_get(unit, mem, &ptab_entry,
                                                        PVLAN_ENABLEf);
    }

    /* Get enable mirroring destination 2 flag. */
    if (SOC_MEM_FIELD_VALID(unit, mem, MIRROR1f)) {
        if (soc_mem_field32_get(unit, mem, &ptab_entry, MIRROR1f)) {
            port_cfg->pc_mirror_ing |= BCM_MIRROR_MTP_TWO; 
        }
    }
#endif /* BCM_TRX_SUPPORT */

    return (BCM_E_NONE); 
}

/*
 * Function:
 *      _bcm_xgs3_port_table_write
 * Purpose:
 *      Prepare & write port table entry 
 * Parameters:
 *      unit       - (IN)SOC unit number.
 *      port       - (IN)Port number.
 *      port_cfg   - (IN)API port information structure.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_xgs3_port_table_write(int unit, bcm_port_t port,
                           bcm_port_cfg_t *port_cfg)
{
    int cml, cml_move;              /* CPU managed learning mode. */   
    int repl_cml, repl_cml_move;    /* CPU managed learning mode. */   
    port_tab_entry_t ptab_entry;   /* Port table entry.          */
    soc_mem_t mem = PORT_TABm;     /* Port table memory.         */
    int tbl_index = port;          /* Port table index.          */
    int rv;
    int cpu_hg_index = 0;

    if (BCM_GPORT_IS_SET(port)) {
        BCM_IF_ERROR_RETURN(_bcm_esw_port_gport_validate(unit, port, &port));
    }
    tbl_index = port;

    soc_mem_lock(unit, mem);    /* Lock port table */

    /* Read current port table entry from HW. */
    rv = BCM_XGS3_MEM_READ(unit, mem, tbl_index, &ptab_entry);
    if (BCM_FAILURE(rv)) {
        soc_mem_unlock(unit, mem);
        return rv;
    }
    /* Set drop all tagged/untagged packets flag. */
    switch (port_cfg->pc_disc) {
      case BCM_PORT_DISCARD_NONE:
          soc_mem_field32_set(unit, mem, &ptab_entry, PORT_DIS_TAGf, 0);
          soc_mem_field32_set(unit, mem, &ptab_entry, PORT_DIS_UNTAGf, 0);
          break;
      case BCM_PORT_DISCARD_ALL:
          soc_mem_field32_set(unit, mem, &ptab_entry, PORT_DIS_TAGf, 1);
          soc_mem_field32_set(unit, mem, &ptab_entry, PORT_DIS_UNTAGf, 1);
          break;
      case BCM_PORT_DISCARD_UNTAG:
          soc_mem_field32_set(unit, mem, &ptab_entry, PORT_DIS_TAGf, 0);
          soc_mem_field32_set(unit, mem, &ptab_entry, PORT_DIS_UNTAGf, 1);
          break;
      case BCM_PORT_DISCARD_TAG:
          soc_mem_field32_set(unit, mem, &ptab_entry, PORT_DIS_TAGf, 1);
          soc_mem_field32_set(unit, mem, &ptab_entry, PORT_DIS_UNTAGf, 0);
          break;
    }

    /* Set drop bpdu's on ingress flag. */
    soc_mem_field32_set(unit, mem, &ptab_entry, DROP_BPDUf,
                        port_cfg->pc_bpdu_disable);

    /* Set ingress mirroring flag. */
    if (soc_feature(unit, soc_feature_mirror_flexible)) {
        soc_mem_field32_set(unit, mem, &ptab_entry, MIRRORf, 
                (port_cfg->pc_mirror_ing & BCM_TR2_MIRROR_MTP_MASK));
    } else if (!soc_feature(unit, soc_feature_no_mirror)) {
        soc_mem_field32_set(unit, mem, &ptab_entry, MIRRORf, 
                            (port_cfg->pc_mirror_ing & BCM_MIRROR_MTP_ONE));
    }

    /*
     * Set cpu managed learning (cml).
     * if unit is not frozen set the same value to port table.
     * else set port table (cml) to PVP_CML_FORWARD.
     */
    if (soc_l2x_frozen_cml_set(unit, port, port_cfg->pc_cml, 
                               port_cfg->pc_cml_move, &repl_cml, &repl_cml_move) < 0) {
        cml = port_cfg->pc_cml;
        cml_move = port_cfg->pc_cml_move;
    } else {
        cml = repl_cml;
        cml_move = repl_cml_move;
    }

#ifdef BCM_TRX_SUPPORT
    if (SOC_IS_TRX(unit)) {
        if (SOC_MEM_FIELD_VALID(unit, PORT_TABm, CML_FLAGS_NEWf)) {
            soc_mem_field32_set(unit, mem, &ptab_entry, CML_FLAGS_MOVEf, cml_move);
            soc_mem_field32_set(unit, mem, &ptab_entry, CML_FLAGS_NEWf, cml);
        }
        soc_mem_field32_set(unit, mem, &ptab_entry, IVIDf, port_cfg->pc_ivlan);
        soc_mem_field32_set(unit, mem, &ptab_entry, TAG_ACTION_PROFILE_PTRf,
                            port_cfg->pc_vlan_action);
    } else
#endif  /* BCM_TRX_SUPPORT */
    {
        soc_mem_field32_set(unit, mem, &ptab_entry, CMLf, cml);
    }

    /* Set port default vlan id (pvid). */
    soc_mem_field32_set(unit, mem, &ptab_entry, PORT_VIDf, port_cfg->pc_vlan);

    /* Set L3 IPv4 forwarding enable bit. */ 
    if (SOC_MEM_FIELD_VALID(unit, mem, V4L3_ENABLEf)) {
        soc_mem_field32_set(unit, mem, &ptab_entry, V4L3_ENABLEf, 
            (port_cfg->pc_l3_flags & BCM_PORT_L3_V4_ENABLE) ? 1 : 0);
    }    

    /* Set L3 IPv6 forwarding enable bit. */ 
    if (SOC_MEM_FIELD_VALID(unit, mem, V6L3_ENABLEf)) {
        soc_mem_field32_set(unit, mem, &ptab_entry, V6L3_ENABLEf, 
            (port_cfg->pc_l3_flags & BCM_PORT_L3_V6_ENABLE) ? 1 : 0);
    }

    /* Set new port default priority.*/ 
    if (soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
        soc_mem_field32_set(unit, mem, &ptab_entry, PORT_PRIf, 
                        port_cfg->pc_new_opri);
        soc_mem_field32_set(unit, mem, &ptab_entry, OCFIf, 
                        port_cfg->pc_new_ocfi);
        soc_mem_field32_set(unit, mem, &ptab_entry, IPRIf, 
                        port_cfg->pc_new_ipri);
        soc_mem_field32_set(unit, mem, &ptab_entry, ICFIf, 
                        port_cfg->pc_new_icfi);
    } else {
        soc_mem_field32_set(unit, mem, &ptab_entry, PORT_PRIf, 
                        port_cfg->pc_new_opri);
    }

    /* Set incoming packet overwrite/map priority. */
    if (SOC_MEM_FIELD_VALID(unit, mem, MAP_TAG_PKT_PRIORITYf)) {
        soc_mem_field32_set(unit, mem, &ptab_entry, MAP_TAG_PKT_PRIORITYf, 
                            port_cfg->pc_remap_pri_en);
    }

    /* Set ingress port is trusted port, trust incoming IPv4 DSCP bit. */ 
    if (SOC_MEM_FIELD_VALID(unit, mem, TRUST_DSCP_V4f)) {
        soc_mem_field32_set(unit, mem, &ptab_entry, TRUST_DSCP_V4f, 
                        port_cfg->pc_dse_mode ? 1 : 0);
    }

    /* Set ingress port is trusted port, trust incoming IPv6 DSCP bit. */ 
    if (SOC_MEM_FIELD_VALID(unit, mem, TRUST_DSCP_V6f)) {
        soc_mem_field32_set(unit, mem, &ptab_entry, TRUST_DSCP_V6f, 
                        port_cfg->pc_dse_mode ? 1 : 0);
    }

    /* Set enable ingress filtering. */
    soc_mem_field32_set(unit, mem, &ptab_entry, EN_IFILTERf, 
                        port_cfg->pc_en_ifilter);

    /* Set enable ingress filtering. */
    if (SOC_MEM_FIELD_VALID(unit, mem, PORT_BRIDGEf)) {
        soc_mem_field32_set(unit, mem, &ptab_entry, PORT_BRIDGEf, 
                        port_cfg->pc_bridge_port);
    }

    if (SOC_MEM_FIELD_VALID(unit, mem, NNI_PORTf)) {
        /* Set network to network port flag. */
        soc_mem_field32_set(unit, mem, &ptab_entry, NNI_PORTf, 
                            port_cfg->pc_nni_port);
    }

#if defined(BCM_TRX_SUPPORT) || defined(BCM_FIREBOLT2_SUPPORT)
    /* Unicast rpf mode. */
    if (SOC_MEM_FIELD_VALID(unit, mem, URPF_MODEf)) {
        soc_mem_field32_set(unit, mem, &ptab_entry, URPF_MODEf,
                            port_cfg->pc_urpf_mode);
    }

    /* Unicast rpf default gateway check. */
    if (SOC_MEM_FIELD_VALID(unit, mem, URPF_DEFAULTROUTECHECKf)) {
        soc_mem_field32_set(unit, mem, &ptab_entry, URPF_DEFAULTROUTECHECKf,
                            port_cfg->pc_urpf_def_gw_check);
    }
#endif /* BCM_TRX_SUPPORT || BCM_FIREBOLT2_SUPPORT */
#if defined(BCM_TRX_SUPPORT)
    /* private VALN enable */
    if (SOC_MEM_FIELD_VALID(unit, mem, PVLAN_ENABLEf)) {
        soc_mem_field32_set(unit, mem, &ptab_entry, PVLAN_ENABLEf,
                            port_cfg->pc_pvlan_enable);
    }

    /* Set ingress mirroring destination 2 enable flag. */
    if (SOC_MEM_FIELD_VALID(unit, mem, MIRROR1f)) {
        soc_mem_field32_set(unit, mem, &ptab_entry, MIRROR1f, 
                            ((port_cfg->pc_mirror_ing & BCM_MIRROR_MTP_TWO) ? 1 : 0));

    }
#endif /* BCM_TRX_SUPPORT */

    /* Write entry to hw. */
    rv = BCM_XGS3_MEM_WRITE(unit, mem, tbl_index, &ptab_entry);
    if (BCM_FAILURE(rv)) {
        soc_mem_unlock(unit, mem);
        return rv;
    }

    /*
     * Set config to IPORT_TABLEm for the CPU port.
     * IPORT table's higig_packet field must be 1.
     */
    cpu_hg_index = SOC_IS_KATANA2(unit) ?
                   SOC_INFO(unit).cpu_hg_pp_port_index :
                   SOC_INFO(unit).cpu_hg_index;
    if (IS_CPU_PORT(unit, port)) {
        int value = 0;
        soc_mem_t cpu_hg_mem = INVALIDm;
        port_tab_entry_t iptab_entry; 
        if (SOC_MEM_FIELD_VALID(unit, mem, HIGIG_PACKETf)) {
            soc_mem_field32_set(unit, mem, &ptab_entry, HIGIG_PACKETf, 1);
        } else if (SOC_MEM_FIELD_VALID(unit, mem, PORT_TYPEf)) {
            soc_mem_field32_set(unit, mem, &ptab_entry, PORT_TYPEf, 1);
        }

        if (SOC_MEM_IS_VALID(unit, IPORT_TABLEm)) {
            cpu_hg_mem = IPORT_TABLEm;
            tbl_index = port;
        } else if (cpu_hg_index != -1) {
            cpu_hg_mem = PORT_TABm;
            tbl_index = cpu_hg_index;
        }

        if(cpu_hg_mem != INVALIDm) {
            rv = soc_mem_read(unit, cpu_hg_mem, MEM_BLOCK_ANY, tbl_index, &iptab_entry);
            if (BCM_FAILURE(rv)) {
                soc_mem_unlock(unit, mem);
                return rv;
            }
            
            if (SOC_MEM_FIELD_VALID(unit, cpu_hg_mem, DISABLE_VLAN_CHECKSf)) {
                value = soc_mem_field32_get(unit, cpu_hg_mem, &iptab_entry, DISABLE_VLAN_CHECKSf);
                soc_mem_field32_set(unit, cpu_hg_mem, &ptab_entry, DISABLE_VLAN_CHECKSf, value);
            }
            
            /*
            * If other fields like DISABLE_VLAN_CHECKSf, add to here.
            */           
            rv = soc_mem_write(unit, cpu_hg_mem, MEM_BLOCK_ALL, tbl_index, &ptab_entry);
            if (BCM_FAILURE(rv)) {
                soc_mem_unlock(unit, mem);
                return rv;
            }
        }        
    }
    
    soc_mem_unlock(unit, mem); /* Unlock port table */

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_xgs3_trunk_table_read
 * Purpose:
 *      Read trunk table entry and parse it to common
 *      (bcm_port_cfg_t) structure.
 * Parameters:
 *      unit       - (IN)SOC unit number.
 *      port       - (IN)Port number.
 *      tid        - (OUT)Trunk group ID.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_xgs3_trunk_table_read(int unit, bcm_port_t port, int *tid)
{
    int    my_modid;
    int    src_trk_idx = 0; /*Source Trunk table index.*/
    source_trunk_map_table_entry_t trunk_map_entry;/*Trunk table entry buffer.*/

    /* Get module id for unit. */
    BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get(unit, &my_modid));

    /* Get index to source trunk map table */
    BCM_IF_ERROR_RETURN(_bcm_esw_src_mod_port_table_index_get(unit, my_modid,
                port, &src_trk_idx));

    /* Read source trunk map table. */
    SOC_IF_ERROR_RETURN (BCM_XGS3_MEM_READ(unit, SOURCE_TRUNK_MAP_TABLEm, 
                                           src_trk_idx, &trunk_map_entry));

    /* Get trunk group id. */
    *tid = 0;
    if (1 == soc_mem_field32_get(unit, SOURCE_TRUNK_MAP_TABLEm,
                &trunk_map_entry, PORT_TYPEf)) {
        *tid = soc_mem_field32_get(unit, SOURCE_TRUNK_MAP_TABLEm,
                &trunk_map_entry, TGIDf);
    }

    return (BCM_E_NONE); 
}

/*
 * Function:
 *      _bcm_xgs3_trunk_table_write
 * Purpose:
 *      Write trunk table entry.
 * Parameters:
 *      unit       - (IN)SOC unit number.
 *      port       - (IN)Port number.
 *      tid        - (IN)Trunk group ID. 
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_xgs3_trunk_table_write(int unit, bcm_port_t port, int tid)
{
    int    rv;
    int    tgid;
    int    port_type;
    int    my_modid;
    int    src_trk_idx = 0; /*Source Trunk table index.*/
    source_trunk_map_table_entry_t trunk_map_entry;/*Trunk table entry buffer.*/

    if (tid != BCM_TRUNK_INVALID) {
        tgid = tid;
        port_type = 1;
    } else {
        tgid = 0;
        port_type = 0;
    }

    /* Get module id for unit. */
    BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get(unit, &my_modid));

    /* Get index to source trunk map table */
    BCM_IF_ERROR_RETURN(_bcm_esw_src_mod_port_table_index_get(unit, my_modid,
                port, &src_trk_idx));

    soc_mem_lock(unit, SOURCE_TRUNK_MAP_TABLEm);

    /* Read source trunk map table. */
    rv = BCM_XGS3_MEM_READ(unit, SOURCE_TRUNK_MAP_TABLEm, 
            src_trk_idx, &trunk_map_entry);
    if (BCM_FAILURE(rv)) {
        soc_mem_unlock(unit, SOURCE_TRUNK_MAP_TABLEm);
        return rv;
    }

    /* Set trunk group id. */ 
    soc_mem_field32_set(unit, SOURCE_TRUNK_MAP_TABLEm, &trunk_map_entry,
            TGIDf, tgid);

    /* Set port is part of trunk flag. */
    soc_mem_field32_set(unit, SOURCE_TRUNK_MAP_TABLEm, &trunk_map_entry,
            PORT_TYPEf, port_type);

    /* Write entry to hw. */
    rv = BCM_XGS3_MEM_WRITE(unit, SOURCE_TRUNK_MAP_TABLEm,
            src_trk_idx, &trunk_map_entry);

    soc_mem_unlock(unit, SOURCE_TRUNK_MAP_TABLEm);

    return rv; 
}

#ifdef  BCM_FIREBOLT_SUPPORT
/*
 * Function:
 *      _bcm_fb_port_cfg_init
 * Purpose:
 *      Initialize port configuration according to Initial System
 *      Configuration (see init.c)
 * Parameters:
 *      unit       - (IN)StrataSwitch unit number.
 *      port       - (IN)Port number.
 *      vlan_data  - (IN)Initial VLAN data information
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_fb_port_cfg_init(int unit, bcm_port_t port, bcm_vlan_data_t *vlan_data)
{
    port_tab_entry_t ptab;                 /* Port table entry. */
    soc_field_t field_ids[SOC_MAX_MEM_FIELD_WORDS], port_type_field;
    uint32 field_values[SOC_MAX_MEM_FIELD_WORDS], port_type;
    soc_reg_t egr_port_reg = EGR_PORTr;
    soc_reg_t iegr_port_reg = IEGR_PORTr;
    int field_count;
    int cpu_hg_index = 0;
#ifdef BCM_TRX_SUPPORT
    int key_type_value;
#endif

#ifdef BCM_TRIUMPH_SUPPORT
    int dual_modid = 0;

    if ((SOC_IS_TR_VL(unit)) && (NUM_MODID(unit) >= 2)) {
        dual_modid = 1;
    }

    if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) || 
        SOC_IS_VALKYRIE2(unit) || SOC_IS_GREYHOUND(unit)) {
        egr_port_reg = EGR_PORT_64r;
        iegr_port_reg = IEGR_PORT_64r;
    }
#endif

    if (SOC_MEM_FIELD_VALID(unit, PORT_TABm, PORT_TYPEf)) {
        port_type_field = PORT_TYPEf;
    } else {
        port_type_field = HIGIG_PACKETf;
    }

    if (IS_ST_PORT(unit, port)) {
        port_type = 1;
    } else if (IS_LB_PORT(unit, port)) {
        port_type = 2;
    } else {
        port_type = 0;
    }

    field_ids[0] = port_type_field;
    field_values[0] = port_type;
    field_ids[1] = EN_EFILTERf;
    field_values[1] = 1;
    field_count = 2;
#ifdef BCM_TRIUMPH_SUPPORT
    if (SOC_IS_TR_VL(unit)) {
        /* Enable/disable dual-modid */
        if (soc_reg_field_valid(unit, egr_port_reg, DUAL_MODID_ENABLEf)) {
            field_ids[field_count] = DUAL_MODID_ENABLEf;
            field_values[field_count] = dual_modid;
            field_count++;
        } else if (SOC_MEM_FIELD_VALID(unit, EGR_PORTm, DUAL_MODID_ENABLEf)) {
            BCM_IF_ERROR_RETURN
                (soc_mem_field32_modify(unit, EGR_PORTm, port, DUAL_MODID_ENABLEf,
                                        dual_modid));
        }
        if (IS_XE_PORT(unit, port)) {
            if (soc_reg_field_valid(unit, XMODID_DUAL_ENr, DUAL_MODID_ENf)) {
                BCM_IF_ERROR_RETURN
                    (soc_reg_field32_modify(unit, XMODID_DUAL_ENr, port,
                                            DUAL_MODID_ENf, dual_modid));
            } else if (soc_reg_field_valid(unit, XMODID_ENr, DUAL_MODID_ENf)) {
                BCM_IF_ERROR_RETURN
                    (soc_reg_field32_modify(unit, XMODID_ENr, port,
                                            DUAL_MODID_ENf, dual_modid));
            }
        }
    }
#endif
#ifdef BCM_TRX_SUPPORT
    if (SOC_IS_TRX(unit)) {
        /* Initialize egress vlan translation port class with identity mapping */
        field_ids[field_count] = VT_PORT_GROUP_IDf;
        field_values[field_count] = port;
        field_count++;
    }
#endif

#if defined(BCM_RAPTOR_SUPPORT) || defined(BCM_TRIDENT_SUPPORT)
    /* For some devices, directed mirroring will always be 1 */
    if (soc_feature(unit, soc_feature_directed_mirror_only)) {
        field_ids[field_count] = EM_SRCMOD_CHANGEf;
        field_values[field_count] = 1;
        field_count++;
    }
#endif

#ifdef BCM_SHADOW_SUPPORT
    /* EGR_PORT fields are Unsupported in shadow */
    if (SOC_IS_SHADOW(unit)) {
        field_count = 0;
    }
#endif

    if (SOC_MEM_IS_VALID(unit, EGR_PORTm)) {
        BCM_IF_ERROR_RETURN 
            (soc_mem_fields32_modify(unit, EGR_PORTm, port, field_count,
                                     field_ids, field_values));
    } else {
        /*  Update EGR_PORTr register. */
        BCM_IF_ERROR_RETURN 
            (soc_reg_fields32_modify(unit, egr_port_reg, port, field_count,
                                     field_ids, field_values));
    }

    /* Write PORT_TABm memory */
    sal_memcpy(&ptab, soc_mem_entry_null(unit, PORT_TABm), sizeof (ptab));

    /* Set default  vlan id(pvid) for port. */
    soc_PORT_TABm_field32_set(unit, &ptab, PORT_VIDf, vlan_data->vlan_tag);

    /* Switching VLAN. */
    if (SOC_MEM_FIELD_VALID(unit, PORT_TABm, OUTER_TPIDf)) {
        soc_PORT_TABm_field32_set(unit, &ptab, OUTER_TPIDf, 0x8100);
    }

    /* Enable mac based vlan classification. */
    soc_PORT_TABm_field32_set(unit, &ptab, MAC_BASED_VID_ENABLEf, 1);

    /* Enable subned based vlan classification. */
    soc_PORT_TABm_field32_set(unit, &ptab, SUBNET_BASED_VID_ENABLEf, 1);

    /* Set port type. */
    soc_PORT_TABm_field32_set(unit, &ptab, port_type_field, port_type);

#if defined(BCM_RAVEN_SUPPORT) || defined(BCM_TRX_SUPPORT)
    if (SOC_MEM_FIELD_VALID(unit, PORT_TABm, TRUST_INCOMING_VIDf)) {
        soc_PORT_TABm_field32_set(unit, &ptab, TRUST_INCOMING_VIDf, 1);
    }
#endif /* BCM_RAVEN_SUPPORT || BCM_TRX_SUPPORT */

#ifdef BCM_TRX_SUPPORT
    /* Set identify mapping for pri/cfi re-mapping */
    if (SOC_MEM_FIELD_VALID(unit, PORT_TABm, PRI_MAPPINGf)) {
        soc_PORT_TABm_field32_set(unit, &ptab, PRI_MAPPINGf, 0xfac688);
        soc_PORT_TABm_field32_set(unit, &ptab, CFI_0_MAPPINGf, 0);
        soc_PORT_TABm_field32_set(unit, &ptab, CFI_1_MAPPINGf, 1);
    }

#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    /* Set identify mapping for ipri/icfi re-mapping */
    if (SOC_MEM_FIELD_VALID(unit, PORT_TABm, IPRI_MAPPINGf)) {
        soc_PORT_TABm_field32_set(unit, &ptab, IPRI_MAPPINGf, 0xfac688);
        soc_PORT_TABm_field32_set(unit, &ptab, ICFI_0_MAPPINGf, 0);
        soc_PORT_TABm_field32_set(unit, &ptab, ICFI_1_MAPPINGf, 1);
    }
#endif

    /* Set the CML to PVP_CML_SWITCH by default (hw learn and forward) */
    if (SOC_MEM_FIELD_VALID(unit, PORT_TABm, CML_FLAGS_NEWf)) {
        soc_PORT_TABm_field32_set(unit, &ptab, CML_FLAGS_NEWf, 0x8);
        soc_PORT_TABm_field32_set(unit, &ptab, CML_FLAGS_MOVEf, 0x8);
    }

    /* Set first VLAN_XLATE key-type to {outer,port}.
     * Set second VLAN_XLATE key-type to {inner,port}.
     */
    if (SOC_MEM_FIELD_VALID(unit, PORT_TABm, VT_KEY_TYPEf)) {
        BCM_IF_ERROR_RETURN
            (_bcm_esw_pt_vtkey_type_value_get(unit,
                     VLXLT_HASH_KEY_TYPE_OVID,&key_type_value));
        soc_PORT_TABm_field32_set(unit, &ptab, VT_KEY_TYPEf,
                                  key_type_value);
    }
    if (SOC_MEM_FIELD_VALID(unit, PORT_TABm, VT_KEY_TYPE_USE_GLPf)) {
        soc_PORT_TABm_field32_set(unit, &ptab, VT_KEY_TYPE_USE_GLPf, 1);
    }
    if (SOC_MEM_FIELD_VALID(unit, PORT_TABm, VT_PORT_TYPE_SELECTf)) {
        soc_PORT_TABm_field32_set(unit, &ptab, VT_PORT_TYPE_SELECTf, 1);
    }
    if (SOC_MEM_FIELD_VALID(unit, PORT_TABm, VT_KEY_TYPE_2f)) {
        BCM_IF_ERROR_RETURN
            (_bcm_esw_pt_vtkey_type_value_get(unit,
                     VLXLT_HASH_KEY_TYPE_IVID,&key_type_value));
        soc_PORT_TABm_field32_set(unit, &ptab, VT_KEY_TYPE_2f,
                                  key_type_value);
    }
    if (SOC_MEM_FIELD_VALID(unit, PORT_TABm, VT_KEY_TYPE_2_USE_GLPf)) {
        soc_PORT_TABm_field32_set(unit, &ptab, VT_KEY_TYPE_2_USE_GLPf, 1);
    }
    if (SOC_MEM_FIELD_VALID(unit, PORT_TABm, VT_PORT_TYPE_SELECT_2f)) {
        soc_PORT_TABm_field32_set(unit, &ptab, VT_PORT_TYPE_SELECT_2f, 1);
    }

    /* Trust the outer tag pri/cfi (to be backwards compatible) */
    if (SOC_MEM_FIELD_VALID(unit, PORT_TABm, TRUST_OUTER_DOT1Pf)) {
        soc_PORT_TABm_field32_set(unit, &ptab, TRUST_OUTER_DOT1Pf, 1);
    }

#ifdef BCM_TRIUMPH_SUPPORT
    if (SOC_IS_TR_VL(unit)) {
        soc_PORT_TABm_field32_set(unit, &ptab, DUAL_MODID_ENABLEf, dual_modid);
    }
#endif
#endif

#ifdef BCM_RCPU_SUPPORT
    if (SOC_IS_RCPU_ONLY(unit) &&
        SOC_MEM_FIELD_VALID(unit, PORT_TABm, OUTER_TPID_ENABLEf)) {
        /* Required if rcpu vlan is not default vlan */
        if (IS_CPU_PORT(unit,port) || IS_RCPU_PORT(unit,port)) {
            soc_PORT_TABm_field32_set(unit, &ptab, OUTER_TPID_ENABLEf, 0x1);
        }
    }
#endif /* BCM_RCPU_SUPPORT */

    BCM_IF_ERROR_RETURN(BCM_XGS3_MEM_WRITE(unit, PORT_TABm, port, &ptab));

    /* 
     * Replicate port information to IPORT_TABLE and IEGR_PORT register
     * for CPU port. 
     */
    cpu_hg_index = SOC_IS_KATANA2(unit) ?
                   SOC_INFO(unit).cpu_hg_pp_port_index :
                   SOC_INFO(unit).cpu_hg_index;
    if (IS_CPU_PORT(unit,port)) {
        soc_PORT_TABm_field32_set(unit, &ptab, port_type_field, 1);
        if (SOC_MEM_IS_VALID(unit, IPORT_TABLEm)) {
            /* IPORT and PORT have identical formats */
            BCM_IF_ERROR_RETURN
                (soc_mem_write(unit, IPORT_TABLEm, MEM_BLOCK_ALL, port,
                               &ptab));
        } else if (cpu_hg_index != -1) {
            BCM_IF_ERROR_RETURN(soc_mem_write(unit, PORT_TABm, MEM_BLOCK_ALL,
                                cpu_hg_index, &ptab));
        }

        field_values[0] = 1;
        if (SOC_REG_IS_VALID(unit, iegr_port_reg)) {
            /* IEGR_PORT and EGR_PORT have identical formats */
            BCM_IF_ERROR_RETURN 
                (soc_reg_fields32_modify(unit, iegr_port_reg, port,
                                         field_count, field_ids,
                                         field_values));
        } else if (SOC_INFO(unit).cpu_hg_index != -1) {
            BCM_IF_ERROR_RETURN 
                (soc_mem_fields32_modify(unit, EGR_PORTm,
                                         SOC_INFO(unit).cpu_hg_index,
                                         field_count, field_ids,
                                         field_values));
        }
    }

    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_fb_port_rate_ingress_set
 * Purpose:
 *      Set ingress rate limiting parameters
 * Parameters:
 *      unit        - (IN)SOC unit number.   
 *      port        - (IN)Port number.
 *      kbits_sec   - (IN)Rate in kilobits (1000 bits) per second.
 *                        Rate of 0 disables rate limiting.
 *      kbits_burst - (IN)Maximum burst size in kilobits (1000 bits).
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Rate-based pause frames automatically will be enabled
 *      whenever ingress rate metering is enabled. Since the
 *      pause frame behavior also affects the discard/accept
 *      behavior of the ingress meter, the pause frame behavior
 *      is set such that the discard/accept hysteresis is as
 *      small as possible.
 */

/* 
 * Please consult the Programmer's Register Reference Guide for
 * a description of the following settings and resultant values.
 */
#define DISCARD_THD_BITS 3 
#define RESUME_THD_BITS  0

STATIC int
_bcm_fb_port_rate_ingress_set(int unit, bcm_port_t port,
                              uint32 kbits_sec, uint32 kbits_burst)
{
    uint32 kbits_pause;     /* Pause frame rate.                     */  
    uint32 regval = 0;      /* Register value.                       */
    uint32 refresh_rate, bucketsize;
#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAVEN_SUPPORT) || \
    defined (BCM_SCORPION_SUPPORT) || defined(BCM_HURRICANE_SUPPORT) || \
    defined (BCM_HURRICANE2_SUPPORT) || defined(BCM_GREYHOUND_SUPPORT)
    uint32 granularity = 3; 
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_RAVEN_SUPPORT || BCM_SCORPION_SUPPORT */
    /* Disable ingress metering and pause frames for this port */
    BCM_IF_ERROR_RETURN(WRITE_BKPMETERINGCONFIGr(unit, port, regval));

    /*reset the BKPMETERING BUCKET Registers*/
    BCM_IF_ERROR_RETURN(WRITE_BKPMETERINGBUCKETr(unit, port, regval));

#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAVEN_SUPPORT) || \
    defined(BCM_HURRICANE_SUPPORT) || defined(BCM_GREYHOUND_SUPPORT)
    if (SOC_IS_FIREBOLT2(unit) || SOC_IS_RAVEN(unit) || SOC_IS_HAWKEYE(unit) || 
            SOC_IS_HURRICANEX(unit) || SOC_IS_GREYHOUND(unit)) {
        BCM_IF_ERROR_RETURN(WRITE_BKPMETERINGCONFIG_EXTr(unit, port, regval));
    }
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_RAVEN_SUPPORT */

    /* If metering is disabled on this ingress port we are done.*/
    if (!kbits_sec || !kbits_burst) { 
        return (BCM_E_NONE); 
    }

#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAVEN_SUPPORT) || \
    defined(BCM_HURRICANE_SUPPORT) || defined(BCM_GREYHOUND_SUPPORT)
    if (SOC_IS_FIREBOLT2(unit) || SOC_IS_RAVEN(unit) || SOC_IS_HAWKEYE(unit) || 
            SOC_IS_HURRICANEX(unit) || SOC_IS_GREYHOUND(unit)) {
        uint32 regval_ext = 0;
        BCM_IF_ERROR_RETURN
            (READ_BKPMETERINGCONFIG_EXTr(unit, port, &regval_ext));
        /* As below, but for in different register */
        soc_reg_field_set(unit, BKPMETERINGCONFIG_EXTr, &regval_ext,
                          BKPDISCARD_ENf, 1);

        /* Turn off backward compatability default */
        soc_reg_field_set(unit, BKPMETERINGCONFIG_EXTr, &regval_ext,
                          BKPDISCARD_ACCT_ENf, 0);

        soc_reg_field_set(unit, BKPMETERINGCONFIG_EXTr, &regval_ext,
                          DISCARD_THDf, DISCARD_THD_BITS);
        soc_reg_field_set(unit, BKPMETERINGCONFIG_EXTr, &regval_ext,
                          RESUME_THDf, RESUME_THD_BITS);

        BCM_IF_ERROR_RETURN
            (WRITE_BKPMETERINGCONFIG_EXTr(unit, port, regval_ext));
    } else
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_RAVEN_SUPPORT */
    {
    /* Enable metering (rate-based discarding) */
    soc_reg_field_set(unit, BKPMETERINGCONFIGr, &regval,
                      BKPDISCARD_ENf, 1);

    /*
     * BKPMETERINGCONFIGr.BKPDISCARD_ACCT_ENf was cleared when we wrote
     * 0 to the BKPMETERINGCONFIGr register above, so we don't need
     * to clear it here.
     */

    /* 
     * Set the "automatic" pause frame behavior. Because the 
     * metering and pause frame behavior controls are intertwined, 
     * truly independent control of both is not possible. Therefore,
     * the pause frame behavior will be hardcoded so that the metering
     * functionality (API) is preserved as much as possible.
     * The smallest drop/resume hysteresis window should be used.
     */ 
    soc_reg_field_set(unit, BKPMETERINGCONFIGr, &regval,
                      DISCARD_THDf, DISCARD_THD_BITS);
    soc_reg_field_set(unit, BKPMETERINGCONFIGr, &regval,
                      RESUME_THDf, RESUME_THD_BITS);
    }

    /* Pre-check kbits_burst upper limit for upcoming math */
    if (kbits_burst > (0xFFFFFFFF / 8)) {
        kbits_burst = (0xFFFFFFFF / 8);
    }
    /* 
     * Convert requested burst size into the appropriate
     * PAUSE_THD field setting. Based on the above hysteresis
     * window, convert the requested burst size to the 
     * associated PAUSE_THD value.
     */
#if (DISCARD_THD_BITS==0)
    kbits_pause = (kbits_burst * 4) / 7;
#endif
#if (DISCARD_THD_BITS==1)
    kbits_pause = (kbits_burst * 2) / 3;
#endif
#if (DISCARD_THD_BITS==2)
    kbits_pause = (kbits_burst * 4) / 5;
#endif
#if (DISCARD_THD_BITS==3)
    kbits_pause = (kbits_burst * 8) / 9;
#endif

#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAVEN_SUPPORT) || \
    defined (BCM_SCORPION_SUPPORT)  || defined (BCM_HURRICANE_SUPPORT) || \
    defined(BCM_GREYHOUND_SUPPORT)
    if (SOC_IS_FIREBOLT2(unit) || SOC_IS_RAVEN(unit) || 
            SOC_IS_HAWKEYE(unit) || SOC_IS_SC_CQ(unit) || 
            SOC_IS_HURRICANEX(unit) || SOC_IS_GREYHOUND(unit)) {
        uint32 miscval, flags = 0;
        int    refresh_bitsize = 0;
        int    bucket_bitsize = 0;
        if (soc_reg_field_valid(unit, MISCCONFIGr, ITU_MODE_SELf)) {
            BCM_IF_ERROR_RETURN(READ_MISCCONFIGr(unit, &miscval));
            if (soc_reg_field_get(unit, MISCCONFIGr,
                                             miscval, ITU_MODE_SELf)) {
                flags |= _BCM_XGS_METER_FLAG_NON_LINEAR;
            }
        }
        if (soc_reg_field_valid(unit, BKPMETERINGCONFIGr, METER_GRANf)) {
            flags |= _BCM_XGS_METER_FLAG_GRANULARITY;
        }
        if (soc_reg_field_valid(unit, BKPMETERINGCONFIGr, REFRESHCOUNTf)) {
            refresh_bitsize = soc_reg_field_length(unit, BKPMETERINGCONFIGr,
                                                   REFRESHCOUNTf);
        } else if (SOC_REG_IS_VALID(unit, BKPMETERINGCONFIG1r) &&
                   soc_reg_field_valid(unit, BKPMETERINGCONFIG1r,
                                       REFRESHCOUNTf)) {
            refresh_bitsize = soc_reg_field_length(unit, BKPMETERINGCONFIG1r,
                                                   REFRESHCOUNTf);
        }
        bucket_bitsize = soc_reg_field_length(unit, BKPMETERINGCONFIGr,
                                                  PAUSE_THDf);
        BCM_IF_ERROR_RETURN
            (_bcm_xgs_kbits_to_bucket_encoding(kbits_sec, kbits_pause, flags,
                            refresh_bitsize, bucket_bitsize,
                            &refresh_rate, &bucketsize, &granularity));
    } else
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_RAVEN_SUPPORT */
    {
        /* Calculate adj_rate rate. */
        BCM_XGS3_PORT_RATE_64KBIT_QUANTUM(kbits_sec, refresh_rate);
        bucketsize = _bcm_fb_kbits_to_bucketsize(kbits_pause);
    }

    soc_reg_field_set(unit, BKPMETERINGCONFIGr, &regval,
                      PAUSE_THDf, bucketsize);
    /* Set adjasted rate to port config register. */
    if (soc_reg_field_valid(unit,BKPMETERINGCONFIGr,REFRESHCOUNTf)) {
        soc_reg_field_set(unit, BKPMETERINGCONFIGr, &regval,
                          REFRESHCOUNTf, refresh_rate);
    }
#if defined(BCM_SCORPION_SUPPORT)
    if (SOC_REG_IS_VALID(unit, BKPMETERINGCONFIG1r)) {
        if (soc_reg_field_valid(unit,BKPMETERINGCONFIG1r,REFRESHCOUNTf)) {
            BCM_IF_ERROR_RETURN(
                soc_reg_field32_modify(unit, BKPMETERINGCONFIG1r, port,
                                       REFRESHCOUNTf, refresh_rate));
        }
    }

    if (soc_reg_field_valid(unit, BKPMETERINGCONFIGr, METER_GRANf)) {
        soc_reg_field_set(unit, BKPMETERINGCONFIGr, &regval, METER_GRANf,
                          granularity);
    }
#endif /* BCM_SCORPION_SUPPORT */

    /* Enable ingress metering and pause frames for this port */
    BCM_IF_ERROR_RETURN(WRITE_BKPMETERINGCONFIGr(unit, port, regval));

    /*  MISCCONFIG.METERING_CLK_EN is set by chip init */

    return (BCM_E_NONE);
}

#ifdef BCM_TRIUMPH_SUPPORT
STATIC int
_bcm_tr_port_rate_ingress_set(int unit, bcm_port_t port,
                              uint32 kbits_sec, uint32 kbits_burst)
{
    uint32 miscval;
    uint32 kbits_pause;     /* Pause frame rate.                     */  
    uint32 regval = 0;      /* Register value.                       */
    uint64 regval_64;       /* 64-bit Register value.                */
    uint32 refresh_rate, bucketsize, granularity = 3, flags = 0;
    int    refresh_bitsize, bucket_bitsize;

    /* Disable ingress metering and pause frames for this port */
    COMPILER_64_ZERO(regval_64);
    BCM_IF_ERROR_RETURN(WRITE_BKPMETERINGCONFIG_64r(unit, port, regval_64));

    /*reset the BKPMETERING BUCKET Registers*/
    BCM_IF_ERROR_RETURN(WRITE_BKPMETERINGBUCKETr(unit, port, regval));

    /* If metering is disabled on this ingress port we are done.*/
    if (!kbits_sec || !kbits_burst) { 
        return (BCM_E_NONE); 
    }

    /* Enable metering (rate-based discarding) */
    soc_reg64_field32_set(unit, BKPMETERINGCONFIG_64r, &regval_64,
                          BKPDISCARD_ENf, 1);

    /*
     * Set the "automatic" pause frame behavior. Because the
     * metering and pause frame behavior controls are intertwined,
     * truly independent control of both is not possible. Therefore,
     * the pause frame behavior will be hardcoded so that the metering
     * functionality (API) is preserved as much as possible.
     * The smallest drop/resume hysteresis window should be used.
     */
    soc_reg64_field32_set(unit, BKPMETERINGCONFIG_64r, &regval_64,
                          DISCARD_THDf, DISCARD_THD_BITS);
    soc_reg64_field32_set(unit, BKPMETERINGCONFIG_64r, &regval_64,
                          RESUME_THDf, RESUME_THD_BITS);

    /* Pre-check kbits_burst upper limit for upcoming math */
    if (kbits_burst > (0xFFFFFFFF / 8)) {
        kbits_burst = (0xFFFFFFFF / 8);
    }

    /* 
     * Convert requested burst size into the appropriate
     * PAUSE_THD field setting. Based on the above hysteresis
     * window, convert the requested burst size to the 
     * associated PAUSE_THD value.
     */
#if (DISCARD_THD_BITS==0)
    kbits_pause = (kbits_burst * 4) / 7;
#endif
#if (DISCARD_THD_BITS==1)
    kbits_pause = (kbits_burst * 2) / 3;
#endif
#if (DISCARD_THD_BITS==2)
    kbits_pause = (kbits_burst * 4) / 5;
#endif
#if (DISCARD_THD_BITS==3)
    kbits_pause = (kbits_burst * 8) / 9;
#endif

    BCM_IF_ERROR_RETURN(READ_MISCCONFIGr(unit, &miscval));
    if (soc_reg_field_get(unit, MISCCONFIGr, miscval, ITU_MODE_SELf)) {
        flags |= _BCM_XGS_METER_FLAG_NON_LINEAR;
    }
    flags |= _BCM_XGS_METER_FLAG_GRANULARITY;

    if (SOC_IS_TD_TT(unit) || SOC_IS_KATANA(unit)) {
        flags |= _BCM_XGS_METER_FLAG_DOUBLE_REFRESH_FREQ;
    }
    refresh_bitsize = soc_reg_field_length(unit, BKPMETERINGCONFIG_64r,
                                           REFRESHCOUNTf);
    bucket_bitsize = soc_reg_field_length(unit, BKPMETERINGCONFIG_64r,
                                          PAUSE_THDf);
    BCM_IF_ERROR_RETURN
        (_bcm_xgs_kbits_to_bucket_encoding(kbits_sec, kbits_pause, flags,
                            refresh_bitsize, bucket_bitsize,
                            &refresh_rate, &bucketsize, &granularity));

    /* Commit values to fields */
    soc_reg64_field32_set(unit, BKPMETERINGCONFIG_64r, &regval_64,
                          PAUSE_THDf, bucketsize);
    /* Set adjasted rate to port config register. */
    soc_reg64_field32_set(unit, BKPMETERINGCONFIG_64r, &regval_64,
                          REFRESHCOUNTf, refresh_rate);
    soc_reg64_field32_set(unit, BKPMETERINGCONFIG_64r, &regval_64,
                          METER_GRANULARITYf, granularity);

    /* Enable ingress metering and pause frames for this port */
    BCM_IF_ERROR_RETURN(WRITE_BKPMETERINGCONFIG_64r(unit, port, regval_64));

    /*  MISCCONFIG.METERING_CLK_EN is set by chip init */

    return (BCM_E_NONE);
}
#endif
#undef DISCARD_THD_BITS 
#undef RESUME_THD_BITS

/*
 * Function:
 *      _bcm_fb_port_rate_ingress_get
 * Purpose:
 *      Get ingress rate limiting parameters
 * Parameters:
 *      unit        - (IN)SOC unit number.
 *      port        - (IN)Port number.
 *      kbits_sec   - (OUT) Rate in kilobits (1000 bits) per second, or
 *                        zero if rate limiting is disabled.
 *      kbits_burst - (OUT) Maximum burst size in kilobits (1000 bits).
 * Returns:
 *      BCM_E_XXX
 */

int
_bcm_fb_port_rate_ingress_get(int unit, bcm_port_t port,
                              uint32 *kbits_sec, uint32 *kbits_burst)
{
    uint32 kbits_pause;     /* Pause frame rate.                     */  
    uint32 regval;          /* Register value.                       */
    int    bpkdiscard_en, discard_thd;
    uint32 refresh_rate = 0, bucketsize = 0;

    /* Read ingress port metering configuration. */
    BCM_IF_ERROR_RETURN(READ_BKPMETERINGCONFIGr(unit, port, &regval));

    /* Extract pause frame rate. */ 
    bucketsize = soc_reg_field_get(unit, BKPMETERINGCONFIGr, 
                                   regval, PAUSE_THDf);

#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAVEN_SUPPORT) || \
    defined(BCM_HURRICANE_SUPPORT) || defined(BCM_GREYHOUND_SUPPORT)
    if (SOC_IS_FIREBOLT2(unit) || SOC_IS_RAVEN(unit) || SOC_IS_HAWKEYE(unit) ||
        SOC_IS_HURRICANEX(unit) || SOC_IS_GREYHOUND(unit)) {
        uint32 regval_ext;
        BCM_IF_ERROR_RETURN
            (READ_BKPMETERINGCONFIG_EXTr(unit, port, &regval_ext));
        bpkdiscard_en = soc_reg_field_get(unit, BKPMETERINGCONFIG_EXTr, 
                                          regval_ext, BKPDISCARD_ENf);
        discard_thd = soc_reg_field_get(unit, BKPMETERINGCONFIG_EXTr,
                                        regval_ext, DISCARD_THDf);
    } else
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_RAVEN_SUPPORT */
    {
        bpkdiscard_en = soc_reg_field_get(unit, BKPMETERINGCONFIGr, 
                                          regval, BKPDISCARD_ENf);
        discard_thd = soc_reg_field_get(unit, BKPMETERINGCONFIGr,
                                        regval, DISCARD_THDf);
    }

    if (!bpkdiscard_en || !bucketsize) {
        *kbits_sec = *kbits_burst = 0;
    } else {
        if (soc_reg_field_valid(unit, BKPMETERINGCONFIGr, REFRESHCOUNTf)) {
            refresh_rate = soc_reg_field_get(unit, BKPMETERINGCONFIGr, 
                                                regval, REFRESHCOUNTf);
        }
#if defined(BCM_SCORPION_SUPPORT)
        if (SOC_REG_IS_VALID(unit, BKPMETERINGCONFIG1r)) {
            uint32 val;
            BCM_IF_ERROR_RETURN(READ_BKPMETERINGCONFIG1r(unit, port, &val));
            if (soc_reg_field_valid(unit, BKPMETERINGCONFIG1r, REFRESHCOUNTf)) {
                refresh_rate = soc_reg_field_get(unit, BKPMETERINGCONFIG1r, 
                                                    val, REFRESHCOUNTf);
            }
        }
#endif

#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAVEN_SUPPORT) || \
    defined(BCM_SCORPION_SUPPORT) || defined (BCM_HURRICANE_SUPPORT) || \
    defined(BCM_GREYHOUND_SUPPORT)
        if (SOC_IS_FIREBOLT2(unit) || SOC_IS_RAVEN(unit) || 
            SOC_IS_HAWKEYE(unit) || SOC_IS_SC_CQ(unit) || 
            SOC_IS_HURRICANEX(unit) || SOC_IS_GREYHOUND(unit)) {
            uint32 miscval, granularity = 3, flags = 0;
            if (soc_reg_field_valid(unit, MISCCONFIGr, ITU_MODE_SELf)) {
                BCM_IF_ERROR_RETURN(READ_MISCCONFIGr(unit, &miscval));
                if (soc_reg_field_get(unit, MISCCONFIGr,
                                             miscval, ITU_MODE_SELf)) {
                    flags |= _BCM_XGS_METER_FLAG_NON_LINEAR;
                }
            }
            if (soc_reg_field_valid(unit, BKPMETERINGCONFIGr, METER_GRANf)) {
                flags |= _BCM_XGS_METER_FLAG_GRANULARITY;
                granularity = soc_reg_field_get(unit, BKPMETERINGCONFIGr,
                                                regval, METER_GRANf);
            }
            BCM_IF_ERROR_RETURN
                (_bcm_xgs_bucket_encoding_to_kbits(refresh_rate, bucketsize,
                                                   granularity, flags,
                                                   kbits_sec, &kbits_pause));
        } else
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_RAVEN_SUPPORT */
        {
            if (soc_reg_field_valid(unit, BKPMETERINGCONFIGr, REFRESHCOUNTf)) {
                        *kbits_sec = 64 * refresh_rate;
            }
            kbits_pause = _bcm_fb_bucketsize_to_kbits(bucketsize);
        }

        switch (discard_thd) {
          case 0:
              *kbits_burst = (kbits_pause * 7) / 4;
              break;
          case 1:
              *kbits_burst = (kbits_pause * 3) / 2;
              break;
          case 2:
              *kbits_burst = (kbits_pause * 5) / 4;
              break;
          case 3:
              *kbits_burst = (kbits_pause * 9) / 8;
              break;
          default:
              /* Should never happen */
              *kbits_burst = 0; 
              break;
        }
    }
    return (BCM_E_NONE); 
}

#ifdef BCM_TRIUMPH_SUPPORT
int
_bcm_tr_port_rate_ingress_get(int unit, bcm_port_t port,
                              uint32 *kbits_sec, uint32 *kbits_burst)
{
    uint32 miscval;
    uint32 kbits_pause;     /* Pause frame rate.                     */  
    uint64 regval;          /* Register value.                       */
    int    bpkdiscard_en, discard_thd;
    uint32 refresh_rate = 0, bucketsize = 0, granularity = 3, flags = 0;

    /* Read ingress port metering configuration. */
    BCM_IF_ERROR_RETURN(READ_BKPMETERINGCONFIG_64r(unit, port, &regval));

    /* Extract pause frame rate. */ 
    bucketsize = soc_reg64_field32_get(unit, BKPMETERINGCONFIG_64r, 
                                        regval, PAUSE_THDf);
    bpkdiscard_en = soc_reg64_field32_get(unit, BKPMETERINGCONFIG_64r, 
                                          regval, BKPDISCARD_ENf);
    discard_thd = soc_reg64_field32_get(unit, BKPMETERINGCONFIG_64r,
                                        regval, DISCARD_THDf);

    if (!bpkdiscard_en || !bucketsize) {
        *kbits_sec = *kbits_burst = 0;
    } else {
        refresh_rate = soc_reg64_field32_get(unit, BKPMETERINGCONFIG_64r, 
                                             regval, REFRESHCOUNTf);
        BCM_IF_ERROR_RETURN(READ_MISCCONFIGr(unit, &miscval));
        if (soc_reg_field_get(unit, MISCCONFIGr, miscval, ITU_MODE_SELf)) {
            flags |= _BCM_XGS_METER_FLAG_NON_LINEAR;
        }
        flags |= _BCM_XGS_METER_FLAG_GRANULARITY;
        granularity = soc_reg64_field32_get(unit, BKPMETERINGCONFIG_64r,
                                            regval, METER_GRANULARITYf);
        if (SOC_IS_TD_TT(unit) || SOC_IS_KATANA(unit)) {
            flags |= _BCM_XGS_METER_FLAG_DOUBLE_REFRESH_FREQ;
        }
        BCM_IF_ERROR_RETURN
            (_bcm_xgs_bucket_encoding_to_kbits(refresh_rate, bucketsize,
                                               granularity, flags,
                                               kbits_sec, &kbits_pause));

        switch (discard_thd) {
          case 0:
              *kbits_burst = (kbits_pause * 7) / 4;
              break;
          case 1:
              *kbits_burst = (kbits_pause * 3) / 2;
              break;
          case 2:
              *kbits_burst = (kbits_pause * 5) / 4;
              break;
          case 3:
              *kbits_burst = (kbits_pause * 9) / 8;
              break;
          default:
              /* Should never happen */
              *kbits_burst = 0; 
              break;
        }
    }
    return (BCM_E_NONE); 
}
#endif

/* 
 * For Firebolt, there is no software support for bcm_port_rate_pause_set().
 * However, there is support for bcm_port_rate_pause_get().
 */

/*
 * Function:
 *      bcm_fb_port_rate_pause_get
 * Purpose:
 *      Get ingress rate limiting pause frame control parameters
 * Parameters:
 *      unit         - (IN)SOC unit number.
 *      port         - (IN)Port number.
 *      kbits_pause  - (OUT) Pause threshold in kbits (1000 bits).
 *              Zero indicates the pause/resume mechanism is disabled.
 *      kbits_resume - (OUT) Resume threshold in kbits (1000 bits).
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      For Firebolt, these values are returned for informational
 *      purposes only. In order to retain the ingress metering
 *      behavior (as much as possible), the pause frame behavior
 *      is hard-coded, and is not configurable.
 *      Rate-based pause frames are automatically enabled whenever
 *      ingress rate metering is enabled.
 */
int
bcm_fb_port_rate_pause_get(int unit, bcm_port_t port,
                           uint32 *kbits_pause, uint32 *kbits_resume)
{
    uint32 regval = 0; 
    uint32 pause_thd_setting = 0;
    uint32 pause = 0; 
    uint32 bucket = 0; 
    uint32 resume = 0;
    soc_reg_t reg;

    BCM_IF_ERROR_RETURN(READ_BKPMETERINGCONFIGr(unit, port, &regval));

    pause_thd_setting = soc_reg_field_get(unit, BKPMETERINGCONFIGr, 
                                          regval, PAUSE_THDf);

    if (!pause_thd_setting) {
        *kbits_pause = *kbits_resume = 0;
        return (BCM_E_NONE);
    }

#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAVEN_SUPPORT) || defined (BCM_SCORPION_SUPPORT) || defined (BCM_HURRICANE_SUPPORT)
    if (SOC_IS_FIREBOLT2(unit) || SOC_IS_RAVEN(unit) || 
        SOC_IS_HAWKEYE(unit) || SOC_IS_SC_CQ(unit) || SOC_IS_HURRICANEX(unit)) {
        uint32 miscval, rate, granularity = 3, flags = 0;
        if (soc_reg_field_valid(unit, MISCCONFIGr, ITU_MODE_SELf)) {
            BCM_IF_ERROR_RETURN(READ_MISCCONFIGr(unit, &miscval));
            if (soc_reg_field_get(unit, MISCCONFIGr,
                                  miscval, ITU_MODE_SELf)) {
                flags |= _BCM_XGS_METER_FLAG_NON_LINEAR;
            }
        }
        if (soc_reg_field_valid(unit, BKPMETERINGCONFIGr, METER_GRANf)) {
            flags |= _BCM_XGS_METER_FLAG_GRANULARITY;
            granularity = soc_reg_field_get(unit, BKPMETERINGCONFIGr,
                                            regval, METER_GRANf);
        }
        BCM_IF_ERROR_RETURN
            (_bcm_xgs_bucket_encoding_to_kbits(0, pause_thd_setting,
                                               granularity, flags,
                                               &rate, &pause));
    } else
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_RAVEN_SUPPORT */
    {
        pause = _bcm_fb_bucketsize_to_kbits(pause_thd_setting);
    }

    reg = soc_reg_field_valid(unit, BKPMETERINGCONFIGr, DISCARD_THDf) ?
                           BKPMETERINGCONFIGr : BKPMETERINGCONFIG_EXTr;

    switch (soc_reg_field_get(unit, reg,
                              regval, DISCARD_THDf)) {
      case 0:
          bucket = (pause * 7) / 4;
          break;
      case 1:
          bucket = (pause * 3) / 2;
          break;
      case 2:
          bucket = (pause * 5) / 4;
          break;
      case 3:
          bucket = (pause * 9) / 8;
          break;
      default:
          /* Should never happen */
          return (BCM_E_INTERNAL); 
          break;
    }

    switch (soc_reg_field_get(unit, reg,
                              regval, RESUME_THDf)) {
      case 0:
          resume = (pause * 3) / 4;
          break;
      case 1:
          resume = pause / 2;
          break;
      case 2:
          resume = pause / 4;
          break;
      case 3:
          resume = pause / 8;
          break;
      default:
          /* Should never happen */
          return (BCM_E_INTERNAL); 
          break;
    }

    /* 
     * The API assumes a "token bucket", i.e. # tokens in the
     * bucket is proportional to the available bandwidth. But
     * Firebolt uses a "leaky bucket", where # tokens is
     * inversely proportional to available bandwidth. 
     * Convert "leaky bucket" thresholds to "token bucket"-like
     * values.
     */
    *kbits_pause = bucket - pause;
    *kbits_resume = bucket - resume;

    return (BCM_E_NONE); 
}

#ifdef BCM_TRIUMPH_SUPPORT
int
bcm_tr_port_rate_pause_get(int unit, bcm_port_t port,
                           uint32 *kbits_pause, uint32 *kbits_resume)
{
    uint64 regval;
    uint32 miscval, pause_thd_setting, pause, bucket, resume;
    uint32 granularity = 3, rate, flags = 0;

    BCM_IF_ERROR_RETURN(READ_BKPMETERINGCONFIG_64r(unit, port, &regval));

    pause_thd_setting = soc_reg64_field32_get(unit, BKPMETERINGCONFIG_64r, 
                                              regval, PAUSE_THDf);

    if (!pause_thd_setting) {
        *kbits_pause = *kbits_resume = 0;
        return (BCM_E_NONE);
    }

    BCM_IF_ERROR_RETURN(READ_MISCCONFIGr(unit, &miscval));
    if (soc_reg_field_get(unit, MISCCONFIGr, miscval, ITU_MODE_SELf)) {
        flags |= _BCM_XGS_METER_FLAG_NON_LINEAR;
    }
    flags |= _BCM_XGS_METER_FLAG_GRANULARITY;
    if (soc_reg_field_valid(unit,BKPMETERINGCONFIG_64r, METER_GRANf)) {
        granularity = soc_reg64_field32_get(unit, BKPMETERINGCONFIG_64r,
                                        regval, METER_GRANf);
    } else if (soc_reg_field_valid(unit, BKPMETERINGCONFIG_64r,
                                METER_GRANULARITYf)) {
        granularity = soc_reg64_field32_get(unit, BKPMETERINGCONFIG_64r,
                                        regval, METER_GRANULARITYf);
    }
    if (SOC_IS_TD_TT(unit) || SOC_IS_KATANA(unit)) {
        flags |= _BCM_XGS_METER_FLAG_DOUBLE_REFRESH_FREQ;
    }
    BCM_IF_ERROR_RETURN
        (_bcm_xgs_bucket_encoding_to_kbits(0, pause_thd_setting,
                                           granularity, flags,
                                           &rate, &pause));

    switch (soc_reg64_field32_get(unit, BKPMETERINGCONFIG_64r,
                                  regval, DISCARD_THDf)) {
      case 0:
          bucket = (pause * 7) / 4;
          break;
      case 1:
          bucket = (pause * 3) / 2;
          break;
      case 2:
          bucket = (pause * 5) / 4;
          break;
      case 3:
          bucket = (pause * 9) / 8;
          break;
      default:
          /* Should never happen */
          return (BCM_E_INTERNAL); 
          break;
    }

    switch (soc_reg64_field32_get(unit, BKPMETERINGCONFIG_64r,
                                  regval, RESUME_THDf)) {
      case 0:
          resume = (pause * 3) / 4;
          break;
      case 1:
          resume = pause / 2;
          break;
      case 2:
          resume = pause / 4;
          break;
      case 3:
          resume = pause / 8;
          break;
      default:
          /* Should never happen */
          return (BCM_E_INTERNAL); 
          break;
    }

    /* 
     * The API assumes a "token bucket", i.e. # tokens in the
     * bucket is proportional to the available bandwidth. But
     * Firebolt uses a "leaky bucket", where # tokens is
     * inversely proportional to available bandwidth. 
     * Convert "leaky bucket" thresholds to "token bucket"-like
     * values.
     */
    *kbits_pause = bucket - pause;
    *kbits_resume = bucket - resume;

    return (BCM_E_NONE); 
}
#endif

/*
 * Function:
 *      bcm_fb_port_rate_egress_set
 * Purpose:
 *      Set egress rate limiting parameters for the Firebolt chip.
 * Parameters:
 *      unit       - (IN)SOC unit number
 *      port       - (IN)Port number
 *      kbits_sec  - (IN)Rate in kilobits (1000 bits) per second.
 *                       Rate of 0 disables rate limiting.
 *      kbits_burst -(IN)Maximum burst size in kilobits (1000 bits).
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_fb_port_rate_egress_set(int unit, bcm_port_t port,
                            uint32 kbits_sec, uint32 kbits_burst, uint32 mode)
{
    int rv = BCM_E_NONE;
    uint32 refresh_rate, bucketsize;
#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAVEN_SUPPORT) || \
    defined(BCM_SCORPION_SUPPORT)  || defined(BCM_HURRICANE_SUPPORT) || \
    defined(BCM_HURRICANE2_SUPPORT) || defined(BCM_GREYHOUND_SUPPORT)
 
    uint32 granularity = 3, flags = 0;
#endif
    
#if defined(BCM_GREYHOUND_SUPPORT)
    if (SOC_IS_GREYHOUND(unit)) {
        /* GH support Byte mode only */
        if (mode != _BCM_PORT_RATE_BYTE_MODE) {
            return BCM_E_UNAVAIL;
        } 
    }
#endif /* BCM_GREYHOUND_SUPPORT */
    
    /* Take lock if IPMC repl freeze is required */
    if (soc_feature(unit, soc_feature_ipmc_repl_freeze)) {
        SOC_EGRESS_METERING_LOCK(unit);
    }

    /*
     * The procedure followed is first disable the egress metering i.e.
     * reset EGRMETERINGCONFIG, then clear the buckets and re-enable the
     * egress metering. Finally enable the MISCCONFING,Metering clocking
     *
     * NOTE: During the period of disabling and re-enabling the Egress metering
     * may be in-effective for couple of cycles
     */

    /* Disable egress metering for this port */
    if (soc_reg_field_valid(unit, EGRMETERINGCONFIGr, REFRESHf)) {
        rv = soc_reg_field32_modify(unit, EGRMETERINGCONFIGr, port, 
                                    REFRESHf, 0);
    }
#if defined(BCM_SCORPION_SUPPORT)
      else if (soc_reg_field_valid(unit, EGRMETERINGCONFIG1r, REFRESHf)) {
        rv = soc_reg_field32_modify(unit, EGRMETERINGCONFIG1r, port, 
                                    REFRESHf, 0);
    }
#endif /* BCM_SCORPION_SUPPORT */

    if (BCM_FAILURE(rv)) {
        if (soc_feature(unit, soc_feature_ipmc_repl_freeze)) {
            SOC_EGRESS_METERING_UNLOCK(unit);
        }
        return rv;
    }

    if (soc_reg_field_valid(unit, EGRMETERINGCONFIGr, THD_SELf)) {
        rv = soc_reg_field32_modify(unit, EGRMETERINGCONFIGr, port, 
                                    THD_SELf, 0);
    } 

    if (BCM_FAILURE(rv)) {
        if (soc_feature(unit, soc_feature_ipmc_repl_freeze)) {
            SOC_EGRESS_METERING_UNLOCK(unit);
        }
        return rv;
    }

    /* Reset metering bucket. */
    rv = WRITE_EGRMETERINGBUCKETr(unit, port, 0);
    if (BCM_FAILURE(rv)) {
        if (soc_feature(unit, soc_feature_ipmc_repl_freeze)) {
            SOC_EGRESS_METERING_UNLOCK(unit);
        }
        return rv;
    }

    /* If kbits_sec=0, metering is disabled on this egress port*/
    if (kbits_sec) {
        /* Enable metering for this port */


#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAVEN_SUPPORT) || \
    defined(BCM_SCORPION_SUPPORT) || defined(BCM_HURRICANE_SUPPORT) || \
    defined(BCM_GREYHOUND_SUPPORT)
        if (SOC_IS_FIREBOLT2(unit) || SOC_IS_RAVEN(unit) || 
            SOC_IS_HAWKEYE(unit) || SOC_IS_SC_CQ(unit) || 
            SOC_IS_HURRICANEX(unit) || SOC_IS_GREYHOUND(unit))  {
            uint32 miscval = 0;
            int    refresh_bitsize = 0;
            int    bucket_bitsize = 0;

            rv = READ_MISCCONFIGr(unit, &miscval);
            if (BCM_FAILURE(rv)) {
                if (soc_feature(unit, soc_feature_ipmc_repl_freeze)) {
                    SOC_EGRESS_METERING_UNLOCK(unit);
                }
                return rv;
            }
            if (soc_reg_field_valid(unit, MISCCONFIGr, ITU_MODE_SELf)) {
                if (soc_reg_field_get(unit, MISCCONFIGr,
                                      miscval, ITU_MODE_SELf)) {
                    flags |= _BCM_XGS_METER_FLAG_NON_LINEAR;
                }
            }
            if (soc_reg_field_valid(unit, EGRMETERINGCONFIGr, METER_GRANf)) {
                flags |= _BCM_XGS_METER_FLAG_GRANULARITY;
            }
            if (soc_reg_field_valid(unit, EGRMETERINGCONFIGr,
                                    REFRESHf)) {
                refresh_bitsize = soc_reg_field_length(unit,
                                      EGRMETERINGCONFIGr, REFRESHf);
            } else if (SOC_REG_IS_VALID(unit, EGRMETERINGCONFIG1r) &&
                       soc_reg_field_valid(unit, EGRMETERINGCONFIG1r,
                                           REFRESHf)) {
                refresh_bitsize = soc_reg_field_length(unit,
                                      EGRMETERINGCONFIG1r, REFRESHf);
            }
            bucket_bitsize = soc_reg_field_length(unit, EGRMETERINGCONFIGr,
                                                  THD_SELf);
            rv = _bcm_xgs_kbits_to_bucket_encoding(kbits_sec, kbits_burst,
                          flags, refresh_bitsize, bucket_bitsize,
                          &refresh_rate, &bucketsize, &granularity);
            if (BCM_FAILURE(rv)) {
                if (soc_feature(unit, soc_feature_ipmc_repl_freeze)) {
                    SOC_EGRESS_METERING_UNLOCK(unit);
                }
                return rv;
            }
        } else 
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_RAVEN_SUPPORT */
        {
            /* Calculate  kbits rate in 64bit quantum equivalent. */ 
            BCM_XGS3_PORT_RATE_64KBIT_QUANTUM(kbits_sec, refresh_rate);
            bucketsize = _bcm_fb_kbits_to_bucketsize(kbits_burst);
        }

        if (soc_reg_field_valid(unit, EGRMETERINGCONFIGr, REFRESHf)) {
            rv = soc_reg_field32_modify(unit, EGRMETERINGCONFIGr, port, 
                                        REFRESHf, refresh_rate);
        } 
#if defined(BCM_SCORPION_SUPPORT)
        else if (soc_reg_field_valid(unit, EGRMETERINGCONFIG1r, REFRESHf)) {
            rv = soc_reg_field32_modify(unit, EGRMETERINGCONFIG1r, port, 
                                        REFRESHf, refresh_rate);
        }

        if (soc_reg_field_valid(unit, EGRMETERINGCONFIGr, METER_GRANf) &&
            (BCM_SUCCESS(rv))) {
            rv = soc_reg_field32_modify(unit, EGRMETERINGCONFIGr, port, 
                                        METER_GRANf, granularity);
        }
#endif /* BCM_SCORPION_SUPPORT */

        if (BCM_FAILURE(rv)) {
            if (soc_feature(unit, soc_feature_ipmc_repl_freeze)) {
                SOC_EGRESS_METERING_UNLOCK(unit);
            }
            return rv;
        }

        if (soc_reg_field_valid(unit, EGRMETERINGCONFIGr, THD_SELf)) {
            rv = soc_reg_field32_modify(unit, EGRMETERINGCONFIGr, port, 
                                        THD_SELf, bucketsize);
        } 
    }

    /* MISCCONFIG.METERING_CLK_EN is set by chip init */

    /* Release lock if IPMC repl freeze is required */
    if (soc_feature(unit, soc_feature_ipmc_repl_freeze)) {
        SOC_EGRESS_METERING_UNLOCK(unit);
    }

    return rv;
}

/*
 * Function:
 *      bcm_fb_port_rate_egress_get
 * Purpose:
 *      Get egress rate limiting parameters from the Firebolt chip.
 * Parameters:
 *      unit       - (IN)SOC unit number.
 *      port       - (IN)Port number.
 *      kbits_sec -  (OUT)Rate in kilobits (1000 bits) per second, or
 *                        zero if rate limiting is disabled.
 *      kbits_burst -(OUT) Maximum burst size in kilobits (1000 bits).
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_fb_port_rate_egress_get(int unit, bcm_port_t port,
                            uint32 *kbits_sec, uint32 *kbits_burst, uint32 *mode)
{
    uint32 regval = 0;               /* EGRMETERINGCONFIGr register value. */
    int rv = BCM_E_NONE;             /* Operation return status.           */
    uint32 refresh_rate = 0, bucketsize = 0;
#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAPTOR_SUPPORT) || \
    defined(BCM_SCORPION_SUPPORT) || defined(BCM_HURRICANE_SUPPORT) || \
    defined(BCM_HURRICANE2_SUPPORT) || defined(BCM_GREYHOUND_SUPPORT)
    uint32 granularity = 3, flags = 0;
#endif
    /* Input parameters check. */
    if (!kbits_sec || !kbits_burst) {
        return (BCM_E_PARAM);
    }

    /* Take lock if IPMC repl freeze is required */
    if (soc_feature(unit, soc_feature_ipmc_repl_freeze)) {
        SOC_EGRESS_METERING_LOCK(unit);
    }

    rv = READ_EGRMETERINGCONFIGr(unit, port, &regval);

    /* Release lock if IPMC repl freeze is required */
    if (soc_feature(unit, soc_feature_ipmc_repl_freeze)) {
        SOC_EGRESS_METERING_UNLOCK(unit);
    }

    if (rv < 0) {
        return rv;
    }

    if (!soc_reg_field_get(unit, EGRMETERINGCONFIGr, regval, THD_SELf)) {
        *kbits_sec = *kbits_burst = 0;
    } else {
        /* Convert the REFRESH field to kbits/sec (1000 bits/sec). */
        if (soc_reg_field_valid(unit, EGRMETERINGCONFIGr, REFRESHf)) {
            refresh_rate =
                soc_reg_field_get(unit, EGRMETERINGCONFIGr, regval, REFRESHf);
        }
#if defined(BCM_SCORPION_SUPPORT)
        else if (soc_reg_field_valid(unit, EGRMETERINGCONFIG1r, REFRESHf)) {
            uint32 val;
            SOC_IF_ERROR_RETURN
                (READ_EGRMETERINGCONFIG1r(unit, port, &val));
            refresh_rate =
                soc_reg_field_get(unit, EGRMETERINGCONFIG1r, val, REFRESHf);
        }
#endif /* BCM_SCORPION_SUPPORT */
        /* Convert the THD_SEL field to burst size in kbits. */

        if (soc_reg_field_valid(unit, EGRMETERINGCONFIGr, THD_SELf)) {
            bucketsize =
                soc_reg_field_get(unit, EGRMETERINGCONFIGr, regval, THD_SELf);
        }
#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAPTOR_SUPPORT) || \
    defined(BCM_SCORPION_SUPPORT) || defined(BCM_HURRICANE_SUPPORT) || \
    defined(BCM_GREYHOUND_SUPPORT)
        if (SOC_IS_FIREBOLT2(unit) || SOC_IS_RAVEN(unit) || 
            SOC_IS_HAWKEYE(unit) || SOC_IS_SC_CQ(unit) || 
            SOC_IS_HURRICANEX(unit) || SOC_IS_GREYHOUND(unit)) {
            uint32 miscval;

            if (soc_reg_field_valid(unit, MISCCONFIGr, ITU_MODE_SELf)) { 
                BCM_IF_ERROR_RETURN(READ_MISCCONFIGr(unit, &miscval));
                if (soc_reg_field_get(unit, MISCCONFIGr,
                                             miscval, ITU_MODE_SELf)) {
                    flags |= _BCM_XGS_METER_FLAG_NON_LINEAR;
                }
            }
            if (soc_reg_field_valid(unit, EGRMETERINGCONFIGr, METER_GRANf)) {
                flags |= _BCM_XGS_METER_FLAG_GRANULARITY;
                granularity = soc_reg_field_get(unit, EGRMETERINGCONFIGr,
                                                regval, METER_GRANf);
            }
            BCM_IF_ERROR_RETURN
                (_bcm_xgs_bucket_encoding_to_kbits(refresh_rate, bucketsize,
                                                   granularity, flags,
                                                   kbits_sec, kbits_burst));
#if defined(BCM_GREYHOUND_SUPPORT)
            if (SOC_IS_GREYHOUND(unit)) {
                *mode = _BCM_PORT_RATE_BYTE_MODE;
            }
#endif /* BCM_GREYHOUND_SUPPORT */
        } else 
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_RAVEN_SUPPORT */
        {
            *kbits_sec = 64 * refresh_rate;
            *kbits_burst = _bcm_fb_bucketsize_to_kbits(bucketsize);
        }
    }
    return (BCM_E_NONE);
}
#endif /* BCM_FIREBOLT_SUPPORT */

#ifdef BCM_TRIUMPH_SUPPORT
/*
 * Function:
 *      bcm_tr_port_rate_egress_set
 * Purpose:
 *      Set egress rate limiting parameters for the Triumph chip.
 * Parameters:
 *      unit       - (IN)SOC unit number
 *      port       - (IN)Port number
 *      kbits_sec  - (IN)Rate in kilobits (1000 bits) per second.
 *                       Rate of 0 disables rate limiting.
 *      kbits_burst -(IN)Maximum burst size in kilobits (1000 bits).
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr_port_rate_egress_set(int unit, bcm_port_t port,
                            uint32 kbits_sec, uint32 kbits_burst, uint32 mode)
{
    uint32 miscval;
    uint64 regval;
    uint32 refresh_rate, bucketsize, granularity = 3, flags = 0;
    int    refresh_bitsize, bucket_bitsize;

    if (mode == _BCM_PORT_RATE_PPS_MODE) {
        return bcm_tr_port_pps_rate_egress_set(
                            unit, port, kbits_sec, kbits_burst);
    }

    /*
     * The procedure followed is first disable the egress metering i.e.
     * reset EGRMETERINGCONFIG, then clear the buckets and re-enable the
     * egress metering. Finally enable the MISCCONFING,Metering clocking
     *
     * NOTE: During the period of disabling and re-enabling the Egress metering
     * may be in-effective for couple of cycles
     */

    /* Disable egress metering for this port */
    BCM_IF_ERROR_RETURN(READ_EGRMETERINGCONFIG_64r(unit, port, &regval));
    soc_reg64_field32_set(unit, EGRMETERINGCONFIG_64r, &regval, REFRESHf, 0);
    soc_reg64_field32_set(unit, EGRMETERINGCONFIG_64r, &regval, THD_SELf, 0);
    if (soc_reg_field_valid(unit, EGRMETERINGCONFIG_64r, MODEf)) {
        soc_reg64_field32_set(unit, EGRMETERINGCONFIG_64r, &regval, MODEf, 0);
    }
    BCM_IF_ERROR_RETURN(WRITE_EGRMETERINGCONFIG_64r(unit, port, regval));

    /* Reset metering bucket. */
    BCM_IF_ERROR_RETURN(WRITE_EGRMETERINGBUCKETr(unit, port, 0));

    /* If kbits_sec=0, metering is disabled on this egress port*/
    if (kbits_sec) {
        /* Enable metering for this port */

        BCM_IF_ERROR_RETURN(READ_MISCCONFIGr(unit, &miscval));
        if (soc_reg_field_get(unit, MISCCONFIGr, miscval, ITU_MODE_SELf)) {
            flags |= _BCM_XGS_METER_FLAG_NON_LINEAR;
        }
        flags |= _BCM_XGS_METER_FLAG_GRANULARITY;
        if (SOC_IS_TD_TT(unit)) {
            flags |= _BCM_XGS_METER_FLAG_DOUBLE_REFRESH_FREQ;
        }
        refresh_bitsize = soc_reg_field_length(unit, EGRMETERINGCONFIG_64r,
                                               REFRESHf);
        bucket_bitsize = soc_reg_field_length(unit, EGRMETERINGCONFIG_64r,
                                              THD_SELf);
        BCM_IF_ERROR_RETURN
            (_bcm_xgs_kbits_to_bucket_encoding(kbits_sec, kbits_burst, flags,
                               refresh_bitsize, bucket_bitsize,
                               &refresh_rate, &bucketsize, &granularity));
        if (soc_reg_field_valid(unit, EGRMETERINGCONFIG_64r, MODEf)) {
            soc_reg64_field32_set(unit, EGRMETERINGCONFIG_64r, &regval,
                            MODEf, 0);
        }
        if (soc_reg_field_valid(unit, EGRMETERINGCONFIG_64r, METER_GRANf)) {
            soc_reg64_field32_set(unit, EGRMETERINGCONFIG_64r, &regval,
                                  METER_GRANf, granularity);
        }
        soc_reg64_field32_set(unit, EGRMETERINGCONFIG_64r, &regval,
                              REFRESHf, refresh_rate);
        soc_reg64_field32_set(unit, EGRMETERINGCONFIG_64r, &regval,
                              THD_SELf, bucketsize);
        BCM_IF_ERROR_RETURN(WRITE_EGRMETERINGCONFIG_64r(unit, port, regval));
    }

    /* MISCCONFIG.METERING_CLK_EN is set by chip init */

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr_port_rate_egress_get
 * Purpose:
 *      Get egress rate limiting parameters from the Triumph chip.
 * Parameters:
 *      unit       - (IN)SOC unit number.
 *      port       - (IN)Port number.
 *      kbits_sec -  (OUT)Rate in kilobits (1000 bits) per second, or
 *                        zero if rate limiting is disabled.
 *      kbits_burst -(OUT) Maximum burst size in kilobits (1000 bits).
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr_port_rate_egress_get(int unit, bcm_port_t port,
                        uint32 *kbits_sec, uint32 *kbits_burst, uint32 *mode)
{
    uint64 regval;                   /* EGRMETERINGCONFIGr register value. */
    uint32 miscval;
    uint32 refresh_rate = 0, bucketsize = 0, granularity = 3, flags = 0;

    COMPILER_64_ZERO(regval);

    /* Input parameters check. */
    if (!kbits_sec || !kbits_burst) {
        return (BCM_E_PARAM);
    }

    BCM_IF_ERROR_RETURN(READ_MISCCONFIGr(unit, &miscval));
    if (soc_reg_field_get(unit, MISCCONFIGr, miscval, ITU_MODE_SELf)) {
        flags |= _BCM_XGS_METER_FLAG_NON_LINEAR;
    }

    BCM_IF_ERROR_RETURN(READ_EGRMETERINGCONFIG_64r(unit, port, &regval));

    if (soc_reg_field_valid(unit, EGRMETERINGCONFIG_64r, MODEf)) {
        if (1 == soc_reg64_field32_get(unit, EGRMETERINGCONFIG_64r, 
                                        regval, MODEf)) {
            *mode = _BCM_PORT_RATE_PPS_MODE;
            return bcm_tr_port_pps_rate_egress_get(unit, port, 
                                                kbits_sec, kbits_burst);
        }
    }

    bucketsize = soc_reg64_field32_get(unit, EGRMETERINGCONFIG_64r,
                                       regval, THD_SELf);
    refresh_rate = soc_reg64_field32_get(unit, EGRMETERINGCONFIG_64r, 
                                         regval, REFRESHf);
    flags |= _BCM_XGS_METER_FLAG_GRANULARITY;
    if (SOC_IS_TD_TT(unit)) {
        flags |= _BCM_XGS_METER_FLAG_DOUBLE_REFRESH_FREQ;
    }

    if (soc_reg_field_valid(unit, EGRMETERINGCONFIG_64r, METER_GRANf)) {
        granularity = soc_reg64_field32_get(unit, EGRMETERINGCONFIG_64r,
                                            regval, METER_GRANf);
    }
    BCM_IF_ERROR_RETURN
        (_bcm_xgs_bucket_encoding_to_kbits(refresh_rate, bucketsize,
                                           granularity, flags,
                                           kbits_sec, kbits_burst));

    return (BCM_E_NONE);
}

int
bcm_tr_port_pps_rate_egress_set(int unit, bcm_port_t port,
                                uint32 pps, uint32 burst)
{
    uint32 miscval;
    uint64 regval;
    uint32 refresh_rate, bucketsize, granularity = 3, flags = 0;
    int    refresh_bitsize, bucket_bitsize;

    /*
     * The procedure followed is first disable the egress metering i.e.
     * reset EGRMETERINGCONFIG, then clear the buckets and re-enable the
     * egress metering. Finally enable the MISCCONFING,Metering clocking
     *
     * NOTE: During the period of disabling and re-enabling the Egress metering
     * may be in-effective for couple of cycles
     */

    /* Disable egress metering for this port */
    BCM_IF_ERROR_RETURN(READ_EGRMETERINGCONFIG_64r(unit, port, &regval));
    soc_reg64_field32_set(unit, EGRMETERINGCONFIG_64r, &regval, REFRESHf, 0);
    soc_reg64_field32_set(unit, EGRMETERINGCONFIG_64r, &regval, THD_SELf, 0);
    if (soc_reg_field_valid(unit, EGRMETERINGCONFIG_64r, MODEf)) {
        soc_reg64_field32_set(unit, EGRMETERINGCONFIG_64r, &regval, MODEf, 0);
    }
    BCM_IF_ERROR_RETURN(WRITE_EGRMETERINGCONFIG_64r(unit, port, regval));

    /* Reset metering bucket. */
    BCM_IF_ERROR_RETURN(WRITE_EGRMETERINGBUCKETr(unit, port, 0));

    /* If pps=0, metering is disabled on this egress port*/
    if (pps) {
        /* Enable metering for this port */

        BCM_IF_ERROR_RETURN(READ_MISCCONFIGr(unit, &miscval));
        if (soc_reg_field_get(unit, MISCCONFIGr, miscval, ITU_MODE_SELf)) {
            flags |= _BCM_XGS_METER_FLAG_NON_LINEAR;
        }
        flags |= _BCM_XGS_METER_FLAG_PACKET_MODE;
        flags |= _BCM_XGS_METER_FLAG_GRANULARITY;
        if (SOC_IS_TD_TT(unit)) {
            flags |= _BCM_XGS_METER_FLAG_DOUBLE_REFRESH_FREQ;
        }
        refresh_bitsize = soc_reg_field_length(unit, EGRMETERINGCONFIG_64r,
                                               REFRESHf);
        bucket_bitsize = soc_reg_field_length(unit, EGRMETERINGCONFIG_64r,
                                              THD_SELf);
        BCM_IF_ERROR_RETURN
            (_bcm_xgs_kbits_to_bucket_encoding(pps, burst, flags,
                               refresh_bitsize, bucket_bitsize,
                               &refresh_rate, &bucketsize, &granularity));

        if (soc_reg_field_valid(unit, EGRMETERINGCONFIG_64r, MODEf)) {
            soc_reg64_field32_set(unit, EGRMETERINGCONFIG_64r, &regval,
                                  MODEf, 1);
        }
        if (soc_reg_field_valid(unit, EGRMETERINGCONFIG_64r, METER_GRANf)) {
            soc_reg64_field32_set(unit, EGRMETERINGCONFIG_64r, &regval,
                                  METER_GRANf, granularity);
        }
        soc_reg64_field32_set(unit, EGRMETERINGCONFIG_64r, &regval,
                              REFRESHf, refresh_rate);
        soc_reg64_field32_set(unit, EGRMETERINGCONFIG_64r, &regval,
                              THD_SELf, bucketsize);
        BCM_IF_ERROR_RETURN(WRITE_EGRMETERINGCONFIG_64r(unit, port, regval));
    }

    /* MISCCONFIG.METERING_CLK_EN is set by chip init */

    return BCM_E_NONE;
}

int
bcm_tr_port_pps_rate_egress_get(int unit, bcm_port_t port,
                                uint32 *pps, uint32 *burst)
{
    uint64 regval;                   /* EGRMETERINGCONFIGr register value. */
    uint32 miscval;
    uint32 refresh_rate = 0, bucketsize = 0, granularity = 3, flags = 0;

    COMPILER_64_ZERO(regval);

    /* Input parameters check. */
    if (!pps || !burst) {
        return (BCM_E_PARAM);
    }

    BCM_IF_ERROR_RETURN(READ_MISCCONFIGr(unit, &miscval));
    if (soc_reg_field_get(unit, MISCCONFIGr, miscval, ITU_MODE_SELf)) {
        flags |= _BCM_XGS_METER_FLAG_NON_LINEAR;
    }

    BCM_IF_ERROR_RETURN(READ_EGRMETERINGCONFIG_64r(unit, port, &regval));

    bucketsize = soc_reg64_field32_get(unit, EGRMETERINGCONFIG_64r,
                                       regval, THD_SELf);
    refresh_rate = soc_reg64_field32_get(unit, EGRMETERINGCONFIG_64r, 
                                         regval, REFRESHf);
    flags |= _BCM_XGS_METER_FLAG_PACKET_MODE;
    flags |= _BCM_XGS_METER_FLAG_GRANULARITY;
    if (SOC_IS_TD_TT(unit)) {
        flags |= _BCM_XGS_METER_FLAG_DOUBLE_REFRESH_FREQ;
    }

    if (soc_reg_field_valid(unit, EGRMETERINGCONFIG_64r, METER_GRANf)) {
        granularity = soc_reg64_field32_get(unit, EGRMETERINGCONFIG_64r,
                                            regval, METER_GRANf);
    }
    BCM_IF_ERROR_RETURN
        (_bcm_xgs_bucket_encoding_to_kbits(refresh_rate, bucketsize,
                                           granularity, flags,
                                           pps, burst));

    return (BCM_E_NONE);
}

#endif /* BCM_TRIUMPH_SUPPORT */

/*
 * Function:
 *      bcm_xgs3_port_cfg_get
 * Purpose:
 *      Get port configuration.
 * Parameters:
 *      unit       - (IN)SOC unit number.
 *      port       - (IN)Port number.
 *      port_cfg   - (OUT)API port information structure.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_xgs3_port_cfg_get(int unit, bcm_port_t port, bcm_port_cfg_t *port_cfg)
{
    /* Input parameters check.*/
    if (NULL == port_cfg) {
        return (BCM_E_PARAM);
    }

    sal_memset(port_cfg, 0, sizeof(bcm_port_cfg_t));

    /* Read port table entry. */
    BCM_IF_ERROR_RETURN(_bcm_xgs3_port_table_read(unit, port, port_cfg));

    return(BCM_E_NONE);
}

/*
 * Function:
 *      bcm_xgs3_port_cfg_set
 * Purpose:
 *      Set port configuration. 
 * Parameters:
 *      unit       - (IN)SOC unit number.
 *      port       - (IN)Port number.
 *      port_cfg   - (IN)API port information structure.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_xgs3_port_cfg_set(int unit, bcm_port_t port, bcm_port_cfg_t *port_cfg)
{
    /* Input parameters check.*/
    if (NULL == port_cfg) {
        return (BCM_E_PARAM);
    }

    /* Write port table entry. */
    BCM_IF_ERROR_RETURN(_bcm_xgs3_port_table_write(unit, port, port_cfg));

    return (BCM_E_NONE);
}

/*
 * Function:
 *      bcm_xgs3_port_cfg_init
 * Purpose:
 *      Initialize the port configuration according to Initial System
 *      Configuration (see init.c)
 * Parameters:
 *      unit       - (IN)StrataSwitch unit number.
 *      port       - (IN)Port number.
 *      vlan_data  - (IN)Initial VLAN data information
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_xgs3_port_cfg_init(int unit, bcm_port_t port, bcm_vlan_data_t *vlan_data)
{
#ifdef BCM_FIREBOLT_SUPPORT
    if(SOC_IS_FBX(unit)) {
        return _bcm_fb_port_cfg_init(unit, port, vlan_data);
    }
#endif /* BCM_FIREBOLT_SUPPORT */
    return (BCM_E_NONE);
}

/*
 * Function:
 *      bcm_xgs3_port_rate_ingress_set
 * Purpose:
 *      Set ingress rate limiting parameters
 * Parameters:
 *      unit        - (IN) SOC unit number.   
 *      port        - (IN) Port number.
 *      kbits_sec   - (IN) Rate in kilobits (1000 bits) per second.
 *                         Rate of 0 disables rate limiting.
 *      kbits_burst - (IN) Maximum burst size in kilobits (1000 bits).
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Rate-based pause frames automatically will be enabled
 *      whenever ingress rate metering is enabled. Since the
 *      pause frame behavior also affects the discard/accept
 *      behavior of the ingress meter, the pause frame behavior
 *      is set such that the discard/accept hysteresis is as
 *      small as possible.
 */
int
bcm_xgs3_port_rate_ingress_set(int unit, bcm_port_t port,
                               uint32 kbits_sec, uint32 kbits_burst)
{
#ifdef BCM_TRIDENT2_SUPPORT
    if (SOC_IS_TD2_TT2(unit)) {
        return bcm_td2_port_rate_ingress_set(unit, port, kbits_sec,
                                             kbits_burst);
    } else
#endif
#ifdef BCM_TRIUMPH_SUPPORT
    if(SOC_IS_TR_VL(unit) && !SOC_IS_HURRICANEX(unit) && 
           !SOC_IS_GREYHOUND(unit)) {
        return _bcm_tr_port_rate_ingress_set(unit, port,
                                             kbits_sec, kbits_burst);
    } else
#endif
#ifdef BCM_FIREBOLT_SUPPORT
    if(SOC_IS_FBX(unit)) {
        return _bcm_fb_port_rate_ingress_set(unit, port,
                                             kbits_sec, kbits_burst);
    }
#endif /* BCM_FIREBOLT_SUPPORT */
    return (BCM_E_UNAVAIL);
}

/*
 * Function:
 *      bcm_xgs3_port_rate_ingress_get
 * Purpose:
 *      Get ingress rate limiting parameters
 * Parameters:
 *      unit        - (IN)SOC unit number.
 *      port        - (IN)Port number.
 *      kbits_sec   - (OUT) Rate in kilobits (1000 bits) per second, or
 *                        zero if rate limiting is disabled.
 *      kbits_burst - (OUT) Maximum burst size in kilobits (1000 bits).
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_xgs3_port_rate_ingress_get(int unit, bcm_port_t port,
                               uint32 *kbits_sec, uint32 *kbits_burst)
{
#ifdef BCM_TRIDENT2_SUPPORT
    if (SOC_IS_TD2_TT2(unit)) {
        return bcm_td2_port_rate_ingress_get(unit, port, kbits_sec,
                                             kbits_burst); 
    } else
#endif
#ifdef BCM_TRIUMPH_SUPPORT
    if(SOC_IS_TR_VL(unit) && !SOC_IS_HURRICANEX(unit) && 
            !SOC_IS_GREYHOUND(unit)) {
        return _bcm_tr_port_rate_ingress_get(unit, port,
                                             kbits_sec, kbits_burst); 
    } else
#endif
#ifdef BCM_FIREBOLT_SUPPORT
    if(SOC_IS_FBX(unit)) {
        return _bcm_fb_port_rate_ingress_get(unit, port,
                                             kbits_sec, kbits_burst); 
    }
#endif /* BCM_FIREBOLT_SUPPORT */
    return (BCM_E_UNAVAIL);
}

/*
 * MAX_SFLOW_RANGE is used for converting a sampling rate to a threshold
 * value used for comparison with the sFlow random number for a port.
 * If the random number is less than the threshold value, an sFlow
 * sample record is sent to the CPU.
 */
#define MAX_SFLOW_RANGE (1 << 24)

/*
 * Function:
 *    _bcm_xgs3_port_sample_rate_set
 * Purpose:
 *    Set rate sampling parameters for XGS3 family chip.
 * Parameters:
 *    unit         - (IN)SOC unit number.
 *    port         - (IN)Port number.
 *    rate         - (IN)Every 1/rate a packet will be sampled
 *            1 indicating sample all.
 *            0 indicating sample none.
 *   rate_reg      - (IN)Ingress/Egress rate register. 
 * Returns:
 *    BCM_E_XXX
 */
int
_bcm_xgs3_port_sample_rate_set(int unit, bcm_port_t port,
                               int rate, soc_reg_t reg) 
{
    soc_field_t reg_fields[2] = {ENABLEf, THRESHOLDf};
    uint32 reg_values[2] = {0, 0};

    /* Setup 'enable' and 'threshold' fields for register. */
    if (rate > 0) {

        /* Enable sFlow */
        reg_values[0] = 1;

        /* Set sampling threshold */
        if (rate == 1) {
            /* Needs special handling to avoid register overflow */
            reg_values[1] = MAX_SFLOW_RANGE - 1;
        } else {
            /* Calculate threshold based on sampling rate */
            reg_values[1] = MAX_SFLOW_RANGE / rate;
        }

        /* 
         * For early sFlow implementations the 16-bit threshold value
         * is compared to bits [23:8] of the sFlow random number.
         */
        if (soc_feature(unit, soc_feature_sample_offset8)) {
            reg_values[1] >>= 8;
        }

        /* Cut the value in case 16 bit threshold can not accomodate it. */
        if (soc_feature(unit, soc_feature_sample_thresh16)) {
            if (reg_values[1] >= (1 << 16)) {
                reg_values[1] = (1 << 16) - 1;
            }
        }
    } 
    /* Update rate limiting regster values. */   
    return soc_reg_fields32_modify(unit, reg, port,
                                   sizeof(reg_fields)/sizeof(uint32), 
                                   reg_fields, reg_values);
}

/*
 * Function:
 *    bcm_xgs3_port_sample_rate_set
 * Purpose:
 *    Set rate sampling parameters for XGS3 family chip.
 * Parameters:
 *    unit         - (IN)SOC unit number.
 *    port         - (IN)Port number.
 *    ingress_rate - (IN)Every 1/ingress_rate a packet will be sampled
 *            1 indicating sample all.
 *            0 indicating sample none.
 *    egress_rate  - (IN)Every 1/egress_rate a packet will be sampled
 *            1 indicating sample all.
 *            0 indicating sample none.
 * Returns:
 *    BCM_E_XXX
 */
int
bcm_xgs3_port_sample_rate_set(int unit, bcm_port_t port,
                              int ingress_rate, int egress_rate ) 
{
    /* Set ingress rate limiting. */
    BCM_IF_ERROR_RETURN
        (_bcm_xgs3_port_sample_rate_set(unit, port, ingress_rate,
                                        SFLOW_ING_THRESHOLDr)); 

    /* Set egress rate limiting. */
    return _bcm_xgs3_port_sample_rate_set(unit, port, egress_rate,
                                          SFLOW_EGR_THRESHOLDr); 
}

/*
 * Function:
 *    bcm_xgs3_port_sample_rate_get
 * Purpose:
 *    Get rate sampling parameters from the hw.
 * Parameters:
 *    unit          - (IN)SOC unit number.
 *    port          - (IN)Port number.
 *    ingress_rate  - (OUT)Every 1/ingress_rate a packet will be sampled
 *            1 indicating sample all.
 *            0 indicating sample none.
 *    egress_rate   - (OUT)Every 1/egress_rate a packet will be sampled
 *            1 indicating sample all.
 *            0 indicating sample none.
 * Returns:
 *    BCM_E_XXX
 */
int
bcm_xgs3_port_sample_rate_get(int unit, bcm_port_t port,
                              int *ingress_rate, int *egress_rate ) 
{
    uint32 ingress_regval       = 0;
    uint32 ingress_field_enable = 0;
    uint32 ingress_field_thresh;
    uint32 egress_regval;
    uint32 egress_field_enable;
    uint32 egress_field_thresh;

    /* Input parameters sanity. */
    if ((NULL ==  ingress_rate) || (NULL == egress_rate)) {
        return (BCM_E_PARAM);
    }

    BCM_IF_ERROR_RETURN(READ_SFLOW_ING_THRESHOLDr(unit, port, &ingress_regval));
    BCM_IF_ERROR_RETURN(READ_SFLOW_EGR_THRESHOLDr(unit, port, &egress_regval));

    /* read 'enable' and 'threshold' fields for ingress and egress ports */
    ingress_field_enable = soc_reg_field_get(unit, SFLOW_ING_THRESHOLDr,
                                             ingress_regval, ENABLEf);
    ingress_field_thresh = soc_reg_field_get(unit, SFLOW_ING_THRESHOLDr,
                                             ingress_regval, THRESHOLDf);
    egress_field_enable = soc_reg_field_get(unit, SFLOW_EGR_THRESHOLDr,
                                            egress_regval, ENABLEf);
    egress_field_thresh = soc_reg_field_get(unit, SFLOW_EGR_THRESHOLDr,
                                            egress_regval, THRESHOLDf);

    /* 
     * For early sFlow implementations the 16-bit threshold value
     * is compared to bits [23:8] of the sFlow random number.
     */
    if (soc_feature(unit, soc_feature_sample_offset8)) {
        ingress_field_thresh <<= 8;
        egress_field_thresh <<= 8;
    }

    /* Translate fields into ingress and egress rates */
    if (ingress_field_enable) {
        if (ingress_field_thresh) {
            *ingress_rate = MAX_SFLOW_RANGE / ingress_field_thresh;
        } else {
            *ingress_rate = MAX_SFLOW_RANGE;
        }
    }
    else {
        *ingress_rate = 0;
        if (ingress_field_thresh) {
            return (BCM_E_INTERNAL);
        }
    }

    if (egress_field_enable) {
        if (egress_field_thresh) { 
            *egress_rate = MAX_SFLOW_RANGE / egress_field_thresh;
        } else {
            *egress_rate = MAX_SFLOW_RANGE;
        }
    }
    else {
        *egress_rate = 0;
        if (egress_field_thresh) { 
            return (BCM_E_INTERNAL);
        }
    }

    return (BCM_E_NONE);
}
#undef MAX_SFLOW_RANGE
/*
 * Function:
 *      _bcm_fb_port_protocol_vlan_add
 * Purpose:
 *      Adds a protocol based vlan to a port.  The protocol
 *      is matched by frame type and ether type.  
 * Parameters:
 *      unit  - (IN) BCM device number
 *      port  - (IN) Port number
 *      frame - (IN) one of BCM_PORT_FRAMETYPE_{ETHER2,8023,LLC}
 *      ether - (IN) 16 bit Ethernet type field
 *      vid   - (IN)VLAN ID
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_fb_port_protocol_vlan_add(int unit,
                           bcm_port_t port,
                           bcm_port_frametype_t frame,
                           bcm_port_ethertype_t ether,
                           bcm_vlan_t vid)
{
    bcm_port_frametype_t         ft;
    bcm_port_ethertype_t         et;
    bcm_port_t                   p;
    _bcm_port_info_t             *pinfo;
    vlan_protocol_entry_t        vpe;
    vlan_protocol_data_entry_t   vde;
    int                          idxmin, idxmax;
    int                          vlan_prot_entries;
    int                          vpentry, vdentry;
    bcm_pbmp_t                   switched_pbm;
    int                          i;
    int                          empty;

    idxmin = soc_mem_index_min(unit, VLAN_PROTOCOLm);
    idxmax = soc_mem_index_max(unit, VLAN_PROTOCOLm);
    vlan_prot_entries = idxmax + 1;
    vpentry = empty = -1;
    for (i = idxmin; i <= idxmax; i++) {
        SOC_IF_ERROR_RETURN
            (READ_VLAN_PROTOCOLm(unit, MEM_BLOCK_ANY, i, &vpe));
        ft = 0;
        if (soc_VLAN_PROTOCOLm_field32_get(unit, &vpe, ETHERIIf)) {
            ft |= BCM_PORT_FRAMETYPE_ETHER2;
        }
        if (soc_VLAN_PROTOCOLm_field32_get(unit, &vpe, SNAPf)) {
            ft |= BCM_PORT_FRAMETYPE_8023;
        }
        if (soc_VLAN_PROTOCOLm_field32_get(unit, &vpe, LLCf)) {
            ft |= BCM_PORT_FRAMETYPE_LLC;
        }
        et = soc_VLAN_PROTOCOLm_field32_get(unit, &vpe, ETHERTYPEf);
        if (empty < 0 && ft == 0) {
            empty = i;
        }
        if (ft == frame && et == ether) {
            vpentry = i;
            break;
        }
    }
    if (vpentry < 0 && empty < 0) {
        return BCM_E_FULL;
    }
    if (vpentry < 0) {
        sal_memset(&vpe, 0, sizeof(vpe));
        soc_VLAN_PROTOCOLm_field32_set(unit, &vpe, ETHERTYPEf, ether);
        if (frame & BCM_PORT_FRAMETYPE_ETHER2) {
            soc_VLAN_PROTOCOLm_field32_set(unit, &vpe, ETHERIIf, 1);
        }
        if (frame & BCM_PORT_FRAMETYPE_8023) {
            soc_VLAN_PROTOCOLm_field32_set(unit, &vpe, SNAPf, 1);
        }
        if (frame & BCM_PORT_FRAMETYPE_LLC) {
            soc_VLAN_PROTOCOLm_field32_set(unit, &vpe, LLCf, 1);
        }
        soc_VLAN_PROTOCOLm_field32_set(unit, &vpe, MATCHUPPERf, 1);
        soc_VLAN_PROTOCOLm_field32_set(unit, &vpe, MATCHLOWERf, 1);
        SOC_IF_ERROR_RETURN
            (WRITE_VLAN_PROTOCOLm(unit, MEM_BLOCK_ALL, empty, &vpe));
        vpentry = empty;
    }

    /*
     * Set VLAN ID for target port. For all other ethernet ports,
     * make sure entries indexed by the matched entry in VLAN_PROTOCOL
     * have initialized values (either default or explicit VID).
     */
    switched_pbm = PBMP_E_ALL(unit);
    if (soc_feature(unit, soc_feature_cpuport_switched)) {
        BCM_PBMP_OR(switched_pbm, PBMP_CMIC(unit));
    }

    BCM_PBMP_ITER(switched_pbm, p) {
        bcm_vlan_t  cvid, defvid, wvid;
        vdentry = (p * vlan_prot_entries)
            + vpentry;
        SOC_IF_ERROR_RETURN
            (READ_VLAN_PROTOCOL_DATAm(unit, MEM_BLOCK_ANY,
                                      vdentry, &vde));
        wvid = vid;
        cvid = soc_VLAN_PROTOCOL_DATAm_field32_get(unit, &vde, VLAN_IDf);
        BCM_IF_ERROR_RETURN
            (bcm_esw_port_untagged_vlan_get(unit, p, &defvid));

        BCM_IF_ERROR_RETURN(_bcm_port_info_get(unit, p, &pinfo));
        if (p == port) {
            if (_BCM_PORT_VD_PBVL_IS_SET(pinfo, vpentry)) {
                /* Already explicit VID; must remove first. */
                return BCM_E_EXISTS;
            }
            /* Set as explicit VID */
            _BCM_PORT_VD_PBVL_SET(pinfo, vpentry);
            if (cvid == vid) {
                /* Current (default) VID same as explicit VID */
                continue;
            }
        } else {
            /*
             * For all other ports, if no non-zero VID already set,
             * set to default VLAN ID.
             */
            if (cvid != 0) {
                continue;
            }
            wvid = defvid;
            _BCM_PORT_VD_PBVL_CLEAR(pinfo, vpentry);
        }

        sal_memset(&vde, 0, sizeof(vde));
        soc_VLAN_PROTOCOL_DATAm_field32_set(unit, &vde, VLAN_IDf, wvid);
        soc_VLAN_PROTOCOL_DATAm_field32_set(unit, &vde, PRIf, pinfo->p_ut_prio);
        SOC_IF_ERROR_RETURN
            (WRITE_VLAN_PROTOCOL_DATAm(unit, MEM_BLOCK_ALL,
                                       vdentry, &vde));
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_fb_port_protocol_vlan_delete
 * Purpose:
 *      Remove an already created proto protocol based vlan
 *      on a port.
 * Parameters:
 *      unit - (IN) BCM device number
 *      port - (IN) Port number
 *      frame - (IN) one of BCM_PORT_FRAMETYPE_{ETHER2,8023,LLC}
 *      ether - (IN) 16 bit Ethernet type field
 * Returns:
 *      BCM_E_XXX
 */

int
_bcm_fb_port_protocol_vlan_delete(int unit,
                              bcm_port_t port,
                              bcm_port_frametype_t frame,
                              bcm_port_ethertype_t ether)
{
    bcm_port_frametype_t         ft;
    bcm_port_ethertype_t         et;
    vlan_protocol_entry_t        vpe;
    vlan_protocol_data_entry_t   vde;
    bcm_vlan_t                   cvid, defvid;
    int                          idxmin, idxmax;
    int                          vlan_prot_entries;
    int                          vpentry, vdentry, p, valid;
    bcm_pbmp_t                   switched_pbm;
    _bcm_port_info_t             *pinfo;
    int                          i;

    idxmin = soc_mem_index_min(unit, VLAN_PROTOCOLm);
    idxmax = soc_mem_index_max(unit, VLAN_PROTOCOLm);
    vlan_prot_entries = idxmax + 1;
    vpentry = -1;
    for (i = idxmin; i <= idxmax; i++) {
        SOC_IF_ERROR_RETURN
            (READ_VLAN_PROTOCOLm(unit, MEM_BLOCK_ANY, i, &vpe));
        ft = 0;
        if (soc_VLAN_PROTOCOLm_field32_get(unit, &vpe, ETHERIIf)) {
            ft |= BCM_PORT_FRAMETYPE_ETHER2;
        }
        if (soc_VLAN_PROTOCOLm_field32_get(unit, &vpe, SNAPf)) {
            ft |= BCM_PORT_FRAMETYPE_8023;
        }
        if (soc_VLAN_PROTOCOLm_field32_get(unit, &vpe, LLCf)) {
            ft |= BCM_PORT_FRAMETYPE_LLC;
        }
        et = soc_VLAN_PROTOCOLm_field32_get(unit, &vpe, ETHERTYPEf);
        if (ft == frame && et == ether) {
            vpentry = i;
            break;
        }
    }
    if (vpentry < 0) {
        return BCM_E_NOT_FOUND;
    }
    vdentry = (port * vlan_prot_entries) + vpentry;
    SOC_IF_ERROR_RETURN
        (READ_VLAN_PROTOCOL_DATAm(unit, MEM_BLOCK_ANY, vdentry, &vde));
    cvid = soc_VLAN_PROTOCOL_DATAm_field32_get(unit, &vde, VLAN_IDf);
    BCM_IF_ERROR_RETURN
        (bcm_esw_port_untagged_vlan_get(unit, port, &defvid));
    /*
     * Entry is initialized (undefined) if VLAN ID is 0.
     * Otherwise, entry is a default VID entry if not programmed.
     * Since we are attempting to remove an "explicit" VID entry,
     * the indexed VLAN_PROTOCOL_DATA entry must be defined and
     * not merely a default VID entry.
     */
    BCM_IF_ERROR_RETURN(_bcm_port_info_get(unit, port, &pinfo));
    if (cvid == 0 ||
        (cvid == defvid && (!_BCM_PORT_VD_PBVL_IS_SET(pinfo, vpentry)))) {
        return BCM_E_NOT_FOUND;
    }
    sal_memset(&vde, 0, sizeof(vde));
    /* set to default VLAN ID and mark it as not programmed   */
    soc_VLAN_PROTOCOL_DATAm_field32_set(unit, &vde, VLAN_IDf, defvid);
    _BCM_PORT_VD_PBVL_CLEAR(pinfo, vpentry);
    SOC_IF_ERROR_RETURN
        (WRITE_VLAN_PROTOCOL_DATAm(unit, MEM_BLOCK_ALL, vdentry, &vde));
    /*
     * See if any other port has an explicit vlan_protocol_data
     * entry for the indicated protocol.
     */
    valid = 0;
    switched_pbm = PBMP_E_ALL(unit);
    if (soc_feature(unit, soc_feature_cpuport_switched)) {
        BCM_PBMP_OR(switched_pbm, PBMP_CMIC(unit));
    }

    BCM_PBMP_ITER(switched_pbm, p) {
        if (p == port) {    /* skip the entry we just "defaulted" */
            continue;
        }
        BCM_IF_ERROR_RETURN(_bcm_port_info_get(unit, p, &pinfo));
        /* Check for explicit VID entry */
        if (_BCM_PORT_VD_PBVL_IS_SET(pinfo, vpentry)) {
            valid = 1;
            break;
        }
    }
    if (!valid) {
        /*
         * Clear all ports' VLAN_PROTOCOL_DATA entries
         * associated with this protocol.
         */
        BCM_PBMP_ITER(switched_pbm, p) {
            vdentry = (p * vlan_prot_entries) + vpentry;
            sal_memset(&vde, 0, sizeof(vde));
            SOC_IF_ERROR_RETURN
                (WRITE_VLAN_PROTOCOL_DATAm(unit, MEM_BLOCK_ALL,
                                           vdentry, &vde));
            BCM_IF_ERROR_RETURN(_bcm_port_info_get(unit, p, &pinfo));
            _BCM_PORT_VD_PBVL_CLEAR(pinfo, vpentry);
        }
        /* clear VLAN_PROTOCOL entry */
        sal_memset(&vpe, 0, sizeof(vpe));
        SOC_IF_ERROR_RETURN
            (WRITE_VLAN_PROTOCOLm(unit, MEM_BLOCK_ALL, vpentry, &vpe));
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_fb_port_untagged_vlan_data_update
 * Purpose:
 *      Update VLAN_PROTOCOL_DATA with new default VLAN ID for port
 * Parameters:
 *      unit - StrataSwitch unit number.
 *      port - StrataSwitch port number of port to get info for
 *      pdvid- Previous default VLAN ID
 *      ndvid- New default VLAN ID
 * Returns:
 *      BCM_XX_NONE
 */
int
_bcm_fb_port_untagged_vlan_data_update(int unit, bcm_port_t port,
                                       bcm_vlan_t pdvid, bcm_vlan_t ndvid)
{
    vlan_protocol_data_entry_t   vde;
    _bcm_port_info_t             *pinfo;
    int                          idx;
    int                          vlan_prot_entries, vlan_data_prot_start;

    vlan_prot_entries = soc_mem_index_count(unit, VLAN_PROTOCOLm);
    vlan_data_prot_start = (port * vlan_prot_entries);
    for (idx = 0; idx < vlan_prot_entries; idx++) {
        SOC_IF_ERROR_RETURN
            (READ_VLAN_PROTOCOL_DATAm(unit, MEM_BLOCK_ANY,
                                      vlan_data_prot_start + idx, &vde));
        /*
         * Update entry with new default VLAN ID if not a programmed
         * VLAN ID and entry has previous default VLAN ID
         */
        BCM_IF_ERROR_RETURN(_bcm_port_info_get(unit, port, &pinfo));
        if ( (!(_BCM_PORT_VD_PBVL_IS_SET(pinfo, idx))) &&
             (soc_VLAN_PROTOCOL_DATAm_field32_get(unit, &vde, VLAN_IDf)
              == pdvid) ) {
            soc_VLAN_PROTOCOL_DATAm_field32_set(unit, &vde, VLAN_IDf, ndvid);
            SOC_IF_ERROR_RETURN
                (WRITE_VLAN_PROTOCOL_DATAm(unit, MEM_BLOCK_ALL,
                                           vlan_data_prot_start + idx, &vde));
        }
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_fb_port_tpid_get
 * Description:
 *      Get the default Tag Protocol ID for a port.
 * Parameters:
 *      unit - Device number
 *      port - Port number
 *      tpid - Tag Protocol ID
 * Return Value:
 *      BCM_E_XXX
 */
int
_bcm_fb_port_tpid_get(int unit, bcm_port_t port, uint16 *tpid)
{
    port_tab_entry_t  ptab;
    int rv; 

    rv = soc_mem_read(unit, PORT_TABm, MEM_BLOCK_ANY, port, &ptab);
    BCM_IF_ERROR_RETURN(rv);

    *tpid = soc_PORT_TABm_field32_get(unit, &ptab, OUTER_TPIDf);
    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_fb_port_tpid_set
 * Description:
 *      Set the default Tag Protocol ID for a port.
 * Parameters:
 *      unit - Device number
 *      port - Port number
 *      tpid - Tag Protocol ID
 * Return Value:
 *      BCM_E_XXX
 */

int
_bcm_fb_port_tpid_set(int unit, bcm_port_t port, uint16 tpid)
{
    port_tab_entry_t  ptab;
    int               rv;

    rv = soc_mem_read(unit, PORT_TABm, MEM_BLOCK_ANY, port, &ptab);
    BCM_IF_ERROR_RETURN(rv);

    soc_PORT_TABm_field32_set(unit, &ptab, OUTER_TPIDf, tpid);
    rv = soc_mem_write(unit, PORT_TABm, MEM_BLOCK_ALL, port, &ptab);
    BCM_IF_ERROR_RETURN(rv);

    rv = soc_reg_field32_modify(unit, EGR_VLAN_CONTROL_1r, port, 
                                OUTER_TPIDf, tpid);
    return rv;
}

#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_TRX_SUPPORT) || \
        defined(BCM_RAVEN_SUPPORT)
/*
 * Function:
 *      _bcm_fb2_port_tpid_get
 * Description:
 *      Get the default Tag Protocol ID for a port.
 * Parameters:
 *      unit - Device number
 *      port - Port number
 *      tpid - Tag Protocol ID
 * Return Value:
 *      BCM_E_XXX
 */
int
_bcm_fb2_port_tpid_get(int unit, bcm_port_t port, uint16 *tpid)
{
    int    index;
    uint32 vctrl;

    SOC_IF_ERROR_RETURN(READ_EGR_VLAN_CONTROL_1r(unit, port, &vctrl));
    index = soc_reg_field_get(unit, EGR_VLAN_CONTROL_1r,
                              vctrl, OUTER_TPID_INDEXf);
    _bcm_fb2_outer_tpid_entry_get(unit, tpid, index);
    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_fb2_port_tpid_set
 * Description:
 *      Set the default Tag Protocol ID for a port.
 * Parameters:
 *      unit - Device number
 *      port - Port number
 *      tpid - Tag Protocol ID
 * Return Value:
 *      BCM_E_XXX
 */
int
_bcm_fb2_port_tpid_set(int unit, bcm_port_t port, uint16 tpid)
{
    int               old_tpid_index;
    uint32            tpid_enable;
    int               tpid_index;
    uint16            old_tpid;
    port_tab_entry_t  ptab;
    uint32            evc;
    int               rv = BCM_E_NONE;

    _bcm_fb2_outer_tpid_tab_lock(unit);

    rv = READ_EGR_VLAN_CONTROL_1r(unit, port, &evc);
    if (BCM_FAILURE(rv)) {
        _bcm_fb2_outer_tpid_tab_unlock(unit);
        return (rv);
    }

    rv = soc_mem_read(unit, PORT_TABm, MEM_BLOCK_ANY, port, &ptab);
    if (BCM_FAILURE(rv)) {
        _bcm_fb2_outer_tpid_tab_unlock(unit);
        return (rv);
    }
         
    /* Delete the TPID referenced by EGR_VLAN_CONTROL_1r */
    old_tpid_index = soc_reg_field_get(unit, EGR_VLAN_CONTROL_1r,
                                       evc, OUTER_TPID_INDEXf);
    rv = _bcm_fb2_outer_tpid_entry_get(unit, &old_tpid, old_tpid_index);
    if (BCM_FAILURE(rv)) {
        _bcm_fb2_outer_tpid_tab_unlock(unit);
        return (rv);
    }
           
    rv = _bcm_fb2_outer_tpid_entry_delete(unit, old_tpid_index);
    if (BCM_FAILURE(rv)) {
        _bcm_fb2_outer_tpid_tab_unlock(unit);
        return (rv);
    }

    /* Delete the TPID referenced by PORT and EGR_SRC_PORT
     * tables. We keep only one reference for both table because
     * the outer TPID enabled will be the same.
     */
    tpid_enable = soc_mem_field32_get(unit, PORT_TABm,
                                      &ptab, OUTER_TPID_ENABLEf);
    tpid_index = 0;
    while (tpid_enable) {
        if (tpid_enable & 1) {
            rv = _bcm_fb2_outer_tpid_entry_delete(unit, tpid_index);
            if (BCM_FAILURE(rv)) {
                _bcm_fb2_outer_tpid_tab_unlock(unit);
                return (rv);
            }
        }
        tpid_enable = tpid_enable >> 1;
        tpid_index++;
    }

    /* Add TPID reference for EGR_VLAN_CONTROL_1r. */
    rv = _bcm_fb2_outer_tpid_entry_add(unit, tpid, &tpid_index);
    if (BCM_FAILURE(rv)) {
        _bcm_fb2_outer_tpid_tab_unlock(unit);
        return (rv);
    }
    
    /* Add TPID reference for EGR_SRC_PORT and PORT tables. 
     * The first insertion and second insertion will return
     * the same status because we are adding the same TPID value.
     */
    rv = _bcm_fb2_outer_tpid_entry_add(unit, tpid, &tpid_index);
    if (BCM_FAILURE(rv)) {
        _bcm_fb2_outer_tpid_entry_delete(unit, tpid_index);
        _bcm_fb2_outer_tpid_tab_unlock(unit);
        return (rv);
    }

    soc_reg_field_set(unit, EGR_VLAN_CONTROL_1r, &evc,
                      OUTER_TPID_INDEXf, tpid_index);
    rv = WRITE_EGR_VLAN_CONTROL_1r(unit, port, evc);
    if (BCM_FAILURE(rv)) {
        _bcm_fb2_outer_tpid_entry_delete(unit, tpid_index);
        _bcm_fb2_outer_tpid_entry_delete(unit, tpid_index);
        _bcm_fb2_outer_tpid_tab_unlock(unit);
        return (rv);
    }

    tpid_enable = 1 << tpid_index;
    soc_PORT_TABm_field32_set(unit, &ptab, OUTER_TPID_ENABLEf, tpid_enable);

    rv = soc_mem_write(unit, PORT_TABm, MEM_BLOCK_ALL, port, &ptab);
    if (BCM_FAILURE(rv)) {
        _bcm_fb2_outer_tpid_entry_delete(unit, tpid_index);
        _bcm_fb2_outer_tpid_entry_delete(unit, tpid_index);
        _bcm_fb2_outer_tpid_tab_unlock(unit);
        return (rv);
    }

#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_ENDURO_SUPPORT)
    if (SOC_REG_FIELD_VALID(unit, EGR_PORT_1r, OUTER_TPID_ENABLEf)) {
        rv = soc_reg_field32_modify(unit, EGR_PORT_1r, port,
                                    OUTER_TPID_ENABLEf, tpid_enable);
    } else
#endif 
    if (SOC_REG_IS_VALID(unit, EGR_SRC_PORTr)) {
        rv = soc_reg_field32_modify(unit, EGR_SRC_PORTr, port, 
                                    OUTER_TPID_ENABLEf, tpid_enable);
    } 

#if defined(BCM_TRIUMPH2_SUPPORT)
    /* For Triumph2 and Apollo, also need to program the SYSTEM_CONFIG_TABLE
     * for second-pass packets. These are treated as Higig-lookup packets */
    if (BCM_SUCCESS(rv) && (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) || 
        SOC_IS_VALKYRIE2(unit) || SOC_IS_ENDURO(unit) || 
        SOC_IS_HURRICANEX(unit) || SOC_IS_TD_TT(unit) || 
        SOC_IS_GREYHOUND(unit))) {
        bcm_module_t my_mod;

        rv = bcm_esw_stk_my_modid_get(unit, &my_mod);
        if (BCM_SUCCESS(rv)) {
#ifdef BCM_TRIDENT_SUPPORT
            if (soc_mem_is_valid(unit, SYSTEM_CONFIG_TABLE_MODBASEm)) {
                rv = _bcm_td_mod_port_tpid_enable_write(unit, my_mod, port,
                        tpid_enable);
            } else
#endif /* BCM_TRIDENT_SUPPORT */
            {
                int sys_index = 0;
                system_config_table_entry_t systab; 

                sys_index = my_mod * 64 + port;
                rv = READ_SYSTEM_CONFIG_TABLEm(unit, MEM_BLOCK_ANY, sys_index,
                        &systab);
                if (BCM_SUCCESS(rv)) {
                    soc_SYSTEM_CONFIG_TABLEm_field32_set(unit, &systab,  
                            OUTER_TPID_ENABLEf, tpid_enable);
                    rv = WRITE_SYSTEM_CONFIG_TABLEm(unit, MEM_BLOCK_ALL, 
                            sys_index, &systab);
                }
            }
        }
    }
#endif /* BCM_TRIUMPH2_SUPPORT */

    if (BCM_FAILURE(rv)) {
        _bcm_fb2_outer_tpid_entry_delete(unit, tpid_index);
        _bcm_fb2_outer_tpid_entry_delete(unit, tpid_index);
    }
    _bcm_fb2_outer_tpid_tab_unlock(unit);
    return (rv);
}

/*
 * Function:
 *      bcm_fb2_port_tpid_add
 * Description:
 *      Add allowed TPID for a port.
 * Parameters:
 *      unit         - (IN) Device number
 *      port         - (IN) Port number
 *      tpid         - (IN) Tag Protocol ID
 *      color_select - (IN) Color mode for TPID
 * Return Value:
 *      BCM_E_XXX
 */
int
_bcm_fb2_port_tpid_add(int unit, bcm_port_t port, 
                      uint16 tpid, int color_select)
{
    int      rv;
    int      index;
    uint32   egr_vlan_ctrl;
    uint32   tpid_enable;  
    uint32   cfi_cng;
    uint32   tpid_mask;
    uint8    remove_tpid;
    port_tab_entry_t  ptab;

    remove_tpid = FALSE;
    _bcm_fb2_outer_tpid_tab_lock(unit);

    rv = soc_mem_read(unit, PORT_TABm, MEM_BLOCK_ANY, port, &ptab);
    if (BCM_FAILURE(rv)) {
        _bcm_fb2_outer_tpid_tab_unlock(unit);
        return (rv);
    }
    tpid_enable = soc_mem_field32_get(unit, PORT_TABm,
                                      &ptab, OUTER_TPID_ENABLEf);

    /* Add new TPID to cached table */
    rv = _bcm_fb2_outer_tpid_lkup(unit, tpid, &index);

    /* If the new TPID is not allowed on the requested port,
     * add the port.
     */
    if (rv == BCM_E_NOT_FOUND || !(tpid_enable & (1 << index))) {
        rv = _bcm_fb2_outer_tpid_entry_add(unit, tpid, &index);
        if (BCM_FAILURE(rv)) {
            _bcm_fb2_outer_tpid_tab_unlock(unit);
            return (rv);
        }
        remove_tpid = TRUE; 
    }

    /* Update registers */

    tpid_mask = 1 << index;

    /* Updat allowed TPID per port */
    tpid_enable |= tpid_mask;
    soc_PORT_TABm_field32_set(unit, &ptab, OUTER_TPID_ENABLEf,
                              tpid_enable);

    rv = READ_EGR_VLAN_CONTROL_1r(unit, port, &egr_vlan_ctrl);
    if (BCM_FAILURE(rv)) {
        if (remove_tpid) {
            _bcm_fb2_outer_tpid_entry_delete(unit, index);
        }
        _bcm_fb2_outer_tpid_tab_unlock(unit);
        return (rv);
    }

    /* Update color selection per TPID per port */
    cfi_cng = soc_reg_field_get(unit, EGR_VLAN_CONTROL_1r,
                                egr_vlan_ctrl, CFI_AS_CNGf);
    switch (color_select) {
      case BCM_COLOR_PRIORITY:
          cfi_cng &= ~tpid_mask;
          break;
      case BCM_COLOR_OUTER_CFI:
          /* Fall through */
      case BCM_COLOR_INNER_CFI:
          cfi_cng |= tpid_mask;
          break;
      default:
          /* Already checked color_select param */
          /* Should never get here              */ 
          break;
    }
    soc_reg_field_set(unit, EGR_VLAN_CONTROL_1r, &egr_vlan_ctrl,
                      CFI_AS_CNGf, cfi_cng);

    soc_PORT_TABm_field32_set(unit, &ptab, CFI_AS_CNGf, cfi_cng);

    /* Update Registers */
#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_ENDURO_SUPPORT)
    if (SOC_REG_FIELD_VALID(unit, EGR_PORT_1r, OUTER_TPID_ENABLEf)) {
        rv = soc_reg_field32_modify(unit, EGR_PORT_1r, port,
                                    OUTER_TPID_ENABLEf, tpid_enable);
    } else
#endif 
    if (0 == SOC_IS_TRX(unit)) {
        rv = soc_reg_field32_modify(unit, EGR_SRC_PORTr, port, 
                                    OUTER_TPID_ENABLEf, tpid_enable);
    } 
    if (BCM_FAILURE(rv)) {
        if (remove_tpid) {
            _bcm_fb2_outer_tpid_entry_delete(unit, index);
        }
        _bcm_fb2_outer_tpid_tab_unlock(unit);
        return (rv);
    }
#if defined(BCM_TRIUMPH2_SUPPORT)
    /* For Triumph2 and Apollo, also need to program the SYSTEM_CONFIG_TABLE
     * for second-pass packets. These are treated as Higig-lookup packets */
    if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) || 
        SOC_IS_VALKYRIE2(unit) || SOC_IS_ENDURO(unit) ||
        SOC_IS_HURRICANEX(unit) || SOC_IS_TD_TT(unit) || 
        SOC_IS_GREYHOUND(unit)) {
        bcm_module_t my_mod;

        rv = bcm_esw_stk_my_modid_get(unit, &my_mod);
        if (BCM_SUCCESS(rv)) {
#ifdef BCM_TRIDENT_SUPPORT
            if (soc_mem_is_valid(unit, SYSTEM_CONFIG_TABLE_MODBASEm)) {
                rv = _bcm_td_mod_port_tpid_enable_write(unit, my_mod, port,
                        tpid_enable);
            } else
#endif /* BCM_TRIDENT_SUPPORT */
            {
                int sys_index = 0;
                system_config_table_entry_t systab; 

                sys_index = my_mod * 64 + port;
                rv = READ_SYSTEM_CONFIG_TABLEm(unit, MEM_BLOCK_ANY, sys_index,
                        &systab);
                if (BCM_SUCCESS(rv)) {
                    soc_SYSTEM_CONFIG_TABLEm_field32_set(unit, &systab,  
                            OUTER_TPID_ENABLEf, tpid_enable);
                    rv = WRITE_SYSTEM_CONFIG_TABLEm(unit, MEM_BLOCK_ALL, 
                            sys_index, &systab);
                }
            }
        }
    }
#endif
    if (BCM_FAILURE(rv)) {
        if (remove_tpid) {
            _bcm_fb2_outer_tpid_entry_delete(unit, index);
        }
        _bcm_fb2_outer_tpid_tab_unlock(unit);
        return (rv);
    }

    rv = WRITE_EGR_VLAN_CONTROL_1r(unit, port, egr_vlan_ctrl);
    if (BCM_FAILURE(rv)) {
        if (remove_tpid) {
            _bcm_fb2_outer_tpid_entry_delete(unit, index);
        }
        _bcm_fb2_outer_tpid_tab_unlock(unit);
        return (rv);
    }

    rv = WRITE_PORT_TABm(unit, MEM_BLOCK_ALL, port, &ptab);
    if (BCM_FAILURE(rv)) {
        if (remove_tpid) {
            _bcm_fb2_outer_tpid_entry_delete(unit, index);
        }
    }
    _bcm_fb2_outer_tpid_tab_unlock(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_fb2_port_tpid_get_all
 * Description:
 *      Retrieve a list of TPIDs available for the port.
 * Parameters:
 *      unit         - (IN) Device number
 *      port         - (IN) Port number
 *      size         - (IN) size of the buffer arrays
 *      tpid_array   - (OUT) tpid buffer array
 *      color_array  - (OUT) color buffer array
 *      count        - (OUT) actual number of tpids/colors retrieved
 * Return Value:
 *      BCM_E_XXX
 */
int
_bcm_fb2_port_tpid_get_all(int unit, bcm_port_t port,
                           int size, uint16 *tpid_array, 
                           int *color_array, int *count)
{
    int      rv;
    int      index;
    uint32   egr_vlan_ctrl;
    uint32   tpid_enable;  
    uint32   cfi_cng;
    port_tab_entry_t  ptab;
    int cnt;
    uint32 tpid = 0;
    int num_entries;

    cnt = 0;
    rv = soc_mem_read(unit, PORT_TABm, MEM_BLOCK_ANY, port, &ptab);
    if (BCM_FAILURE(rv)) {
        return (rv);
    }
    tpid_enable = soc_mem_field32_get(unit, PORT_TABm,
                                      &ptab, OUTER_TPID_ENABLEf);

    /* max tpid entries limited by width of this field */
    num_entries = soc_mem_field_length(unit,PORT_TABm,OUTER_TPID_ENABLEf);

    if (size == 0) {
        for (index = 0; index < num_entries; index++) {
            if (tpid_enable & (1 << index)) {
                cnt++;
            }
        }
        *count = cnt;
        return BCM_E_NONE;
    }

    rv = READ_EGR_VLAN_CONTROL_1r(unit, port, &egr_vlan_ctrl);
    if (BCM_FAILURE(rv)) {
        return (rv);
    }
    cfi_cng = soc_reg_field_get(unit, EGR_VLAN_CONTROL_1r,
                                egr_vlan_ctrl, CFI_AS_CNGf);

    for (index = 0; index < num_entries; index++) {
        if (cnt < size) {
            if (tpid_enable & (1 << index)) {
                rv = READ_EGR_OUTER_TPIDr(unit, index, &tpid);
                if (BCM_FAILURE(rv)) {
                    return (rv);
                }
                tpid_array[cnt] = (uint16)tpid;
                if (cfi_cng & (1 << index)) {
                    color_array[cnt] = BCM_COLOR_OUTER_CFI;
                } else {
                    color_array[cnt] = BCM_COLOR_PRIORITY;
                }
                cnt++;
            }
        }
    }
    *count = cnt;

    return rv;
}

/*
 * Function:
 *      bcm_fb2_port_tpid_delete
 * Description:
 *      Delete allowed TPID for a port.
 * Parameters:
 *      unit - (IN) Device number
 *      port - (IN) Port number
 *      tpid - (IN) Tag Protocol ID
 * Return Value:
 *      BCM_E_XXX
 */
int
_bcm_fb2_port_tpid_delete(int unit, bcm_port_t port, uint16 tpid)
{
    int               rv;
    int               index;
    port_tab_entry_t  ptab;
    uint32            tpid_enable = 0;

    _bcm_fb2_outer_tpid_tab_lock(unit);

    rv = _bcm_fb2_outer_tpid_lkup(unit, tpid, &index);
    if (BCM_FAILURE(rv)) {
        _bcm_fb2_outer_tpid_tab_unlock(unit);
        return (rv);
    } 

    rv = soc_mem_read(unit, PORT_TABm, MEM_BLOCK_ANY, port, &ptab);
    if (BCM_FAILURE(rv)) {
        _bcm_fb2_outer_tpid_tab_unlock(unit);
        return (rv);
    } 

    tpid_enable = soc_PORT_TABm_field32_get(unit, &ptab, OUTER_TPID_ENABLEf);
    if (tpid_enable & (1 << index)) {
        tpid_enable &= ~(1 << index);
        soc_PORT_TABm_field32_set(unit, &ptab, OUTER_TPID_ENABLEf, tpid_enable);
        rv = soc_mem_write(unit, PORT_TABm, MEM_BLOCK_ALL, port, &ptab);   
#if defined(BCM_TRIUMPH2_SUPPORT)
        /* For Triumph2 and Apollo, also need to program the SYSTEM_CONFIG_TABLE
         * for second-pass packets. These are treated as Higig-lookup packets */
        if (BCM_SUCCESS(rv) && (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) || 
            SOC_IS_VALKYRIE2(unit) || SOC_IS_ENDURO(unit) ||
            SOC_IS_HURRICANEX(unit) || SOC_IS_TD_TT(unit))) {
            bcm_module_t my_mod;

            rv = bcm_esw_stk_my_modid_get(unit, &my_mod);
            if (BCM_SUCCESS(rv)) {
#ifdef BCM_TRIDENT_SUPPORT
                if (soc_mem_is_valid(unit, SYSTEM_CONFIG_TABLE_MODBASEm)) {
                    rv = _bcm_td_mod_port_tpid_enable_write(unit, my_mod, port,
                            tpid_enable);
                } else
#endif /* BCM_TRIDENT_SUPPORT */
                {
                    int sys_index = 0;
                    system_config_table_entry_t systab; 

                    rv = bcm_esw_stk_my_modid_get(unit, &my_mod);
                    sys_index = my_mod * 64 + port;
                    rv = READ_SYSTEM_CONFIG_TABLEm(unit, MEM_BLOCK_ANY, sys_index, 
                            &systab);
                    if (BCM_SUCCESS(rv)) {
                        soc_SYSTEM_CONFIG_TABLEm_field32_set(unit, &systab,  
                                OUTER_TPID_ENABLEf, tpid_enable);
                        rv = WRITE_SYSTEM_CONFIG_TABLEm(unit, MEM_BLOCK_ALL, 
                                sys_index, &systab);
                    }
                }
            }
        }
#endif 
    } else {
        rv = BCM_E_NOT_FOUND;
    }
    if (BCM_FAILURE(rv)) {
        _bcm_fb2_outer_tpid_tab_unlock(unit);
        return (rv);
    } 

#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_ENDURO_SUPPORT)
    if (SOC_REG_FIELD_VALID(unit, EGR_PORT_1r, OUTER_TPID_ENABLEf)) {
        rv = soc_reg_field32_modify(unit, EGR_PORT_1r, port,
                                    OUTER_TPID_ENABLEf, tpid_enable);
    } else
#endif
    if (0 == SOC_IS_TRX(unit)) {
         rv = soc_reg_field32_modify(unit, EGR_SRC_PORTr, port,
                                     OUTER_TPID_ENABLEf, tpid_enable);
    }
    if (BCM_FAILURE(rv)) {
        _bcm_fb2_outer_tpid_tab_unlock(unit);
        return (rv);
    } 

    /* Delete the cached entry only after the hardware is
     * successfully updated.
     */
    rv = _bcm_fb2_outer_tpid_entry_delete(unit, index);
    _bcm_fb2_outer_tpid_tab_unlock(unit);
    return rv;
}

/*
 * Function:
 *      bcm_fb2_port_tpid_delete_all
 * Description:
 *      Delete all allowed TPID for a port.
 * Parameters:
 *      unit - (IN) Device number
 *      port - (IN) Port number
 * Return Value:
 *      BCM_E_XXX
 */
int 
_bcm_fb2_port_tpid_delete_all(int unit, bcm_port_t port)
{
    port_tab_entry_t  ptab;
    uint32            tpid_enable, evc;
    int               index = 0;
    int               rv;
    uint16            old_tpid, default_tpid;
    int               default_tpid_index;

    _bcm_fb2_outer_tpid_tab_lock(unit);

    rv = soc_mem_read(unit, PORT_TABm, MEM_BLOCK_ANY, port, &ptab);
    if (BCM_FAILURE(rv)) {
        _bcm_fb2_outer_tpid_tab_unlock(unit);
        return (rv);
    } 
    tpid_enable = soc_mem_field32_get(unit, PORT_TABm,
                                      &ptab, OUTER_TPID_ENABLEf);

    soc_PORT_TABm_field32_set(unit, &ptab, OUTER_TPID_ENABLEf, 0);
    rv = soc_mem_write(unit, PORT_TABm, MEM_BLOCK_ALL, port, &ptab);
    if (BCM_FAILURE(rv)) {
        _bcm_fb2_outer_tpid_tab_unlock(unit);
        return (rv);
    }

#if defined(BCM_TRIUMPH2_SUPPORT)
    if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) || 
            SOC_IS_VALKYRIE2(unit) || SOC_IS_ENDURO(unit) ||
            SOC_IS_HURRICANEX(unit) || SOC_IS_TD_TT(unit) ||
            SOC_IS_GREYHOUND(unit)) {
        bcm_module_t my_mod;

        rv = bcm_esw_stk_my_modid_get(unit, &my_mod);
        if (BCM_SUCCESS(rv)) {
#ifdef BCM_TRIDENT_SUPPORT
            if (soc_mem_is_valid(unit, SYSTEM_CONFIG_TABLE_MODBASEm)) {
                rv = _bcm_td_mod_port_tpid_enable_write(unit, my_mod, port, 0);
            } else
#endif /* BCM_TRIDENT_SUPPORT */
            {
                int sys_index = 0;
                system_config_table_entry_t systab; 

                sys_index = my_mod * 64 + port;
                rv = READ_SYSTEM_CONFIG_TABLEm(unit, MEM_BLOCK_ANY, sys_index, 
                        &systab);
                if (BCM_SUCCESS(rv)) {
                    soc_SYSTEM_CONFIG_TABLEm_field32_set(unit, &systab,  
                            OUTER_TPID_ENABLEf, 0);
                    rv = WRITE_SYSTEM_CONFIG_TABLEm(unit, MEM_BLOCK_ALL, 
                            sys_index, &systab);
                }
            }
        }
    }
#endif /* BCM_TRIUMPH2_SUPPORT */ 
    if (BCM_FAILURE(rv)) {
        _bcm_fb2_outer_tpid_tab_unlock(unit);
        return (rv);
    }

#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_ENDURO_SUPPORT)
    if (SOC_REG_FIELD_VALID(unit, EGR_PORT_1r, OUTER_TPID_ENABLEf)) {
        rv = soc_reg_field32_modify(unit, EGR_PORT_1r, port,
                                    OUTER_TPID_ENABLEf, 0);
    } else
#endif
    if (0 == SOC_IS_TRX(unit)) {
        rv = soc_reg_field32_modify(unit, EGR_SRC_PORTr, port,
                                    OUTER_TPID_ENABLEf, 0);
    }
    if (BCM_FAILURE(rv)) {
        _bcm_fb2_outer_tpid_tab_unlock(unit);
        return (rv);
    }

    rv = READ_EGR_VLAN_CONTROL_1r(unit, port, &evc);
    if (!BCM_FAILURE(rv)) {
        index = soc_reg_field_get(unit, EGR_VLAN_CONTROL_1r,
                                  evc, OUTER_TPID_INDEXf);
    /* If the current port tpid is not default tpid then
     * add the default tpid to the port before deleting current port tpid */

        rv = _bcm_fb2_outer_tpid_entry_get(unit, &old_tpid, index);
        if (BCM_FAILURE(rv)) {
            _bcm_fb2_outer_tpid_tab_unlock(unit);
            return (rv);
        }
        default_tpid = _bcm_fb2_outer_tpid_default_get(unit);
        rv = _bcm_fb2_outer_tpid_entry_add(unit, default_tpid, 
                                           &default_tpid_index);
        if (BCM_FAILURE(rv)) {
            _bcm_fb2_outer_tpid_tab_unlock(unit);
            return (rv);
        }
        soc_reg_field_set(unit, EGR_VLAN_CONTROL_1r, &evc,
                              OUTER_TPID_INDEXf, default_tpid_index);
    } 
    rv = WRITE_EGR_VLAN_CONTROL_1r(unit, port, evc);
   
    if (BCM_FAILURE(rv)) {
        _bcm_fb2_outer_tpid_tab_unlock(unit);
        return (rv);
    }

    /* Delete the cached entries only after the hardware is 
     * successfully updated.
     */

    rv = _bcm_fb2_outer_tpid_entry_delete(unit, index);
    if (BCM_FAILURE(rv)) {
        _bcm_fb2_outer_tpid_tab_unlock(unit);
        return (rv);
    }

    index = 0;
    while (tpid_enable) {
        if (tpid_enable & 1) {
            rv = _bcm_fb2_outer_tpid_entry_delete(unit, index);
            if (BCM_FAILURE(rv)) {
                _bcm_fb2_outer_tpid_tab_unlock(unit);
                return (rv);
            }
        }
        tpid_enable = tpid_enable >> 1;
        index++;
    }
    _bcm_fb2_outer_tpid_tab_unlock(unit);
    return (rv);
}

#endif /* BCM_FIREBOLT2_SUPPORT || BCM_TRX_SUPPORT || BCM_RAVEN_SUPPORT */

/*
 * Function:
 *      _bcm_fb_port_dtag_mode_set
 * Description:
 *      Set the double-tagging mode of a port.
 * Parameters:
 *      unit - (IN) BCM device number
 *      port - (IN) Port number
 *      mode - (IN) Double-tagging mode, one of:
 *              BCM_PORT_DTAG_MODE_NONE            No double tagging
 *              BCM_PORT_DTAG_MODE_INTERNAL        Service Provider port
 *              BCM_PORT_DTAG_MODE_EXTERNAL        Customer port
 *              BCM_PORT_DTAG_REMOVE_EXTERNAL_TAG  Remove customer tag
 *              BCM_PORT_DTAG_ADD_EXTERNAL_TAG     Add customer tag
 *      dt_mode - (IN) True/False double tagged port.
 *      ignore_tag - (IN) Ignore outer tag. 
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *      BCM_PORT_DTAG_MODE_INTERNAL is for service provider ports.
 *              A tag will be added if the packet does not already
 *              have the internal TPID (see bcm_port_tpid_set below).
 *              Internally this sets DT_MODE and clears IGNORE_TAG.
 *      BCM_PORT_DTAG_MODE_EXTERNAL is for customer ports.
 *              The service provider TPID will always be added
 *              (see bcm_port_tpid_set below).
 *              Internally this sets DT_MODE and sets IGNORE_TAG.
 */
int
_bcm_fb_port_dtag_mode_set(int unit, bcm_port_t port, int mode, 
                           int dt_mode, int ignore_tag)
{
    uint32           config, oconfig;
    uint64           config64, oconfig64;
    _bcm_port_info_t *pinfo;
    bcm_port_cfg_t   pcfg;
    int              rv;

    BCM_IF_ERROR_RETURN(_bcm_port_info_get(unit, port, &pinfo));
#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAVEN_SUPPORT)
    if (SOC_IS_FIREBOLT2(unit) || SOC_IS_RAVEN(unit) || SOC_IS_HAWKEYE(unit)) {
        rv =  soc_reg_field32_modify(unit, EGR_SRC_PORTr, port, 
                                     ADD_INNER_TAGf, 
                                     (mode & BCM_PORT_DTAG_ADD_EXTERNAL_TAG)? 1 : 0);
        BCM_IF_ERROR_RETURN(rv);
        rv = soc_reg_field32_modify(unit, EGR_VLAN_CONTROL_1r, port,
                                    REMOVE_INNER_TAGf,
                                    (mode & BCM_PORT_DTAG_REMOVE_EXTERNAL_TAG)? 1 : 0);
        BCM_IF_ERROR_RETURN(rv);
    }
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_RAVEN_SUPPORT */


    SOC_IF_ERROR_RETURN(READ_ING_CONFIGr(unit, &config));
    oconfig = config;
    soc_reg_field_set(unit, ING_CONFIGr, &config,
                      DT_MODEf, dt_mode);
    if (config != oconfig) {
        SOC_IF_ERROR_RETURN(WRITE_ING_CONFIGr(unit, config));
    }

    SOC_IF_ERROR_RETURN(READ_EGR_CONFIGr(unit, &config));
    oconfig = config;
    soc_reg_field_set(unit, EGR_CONFIGr, &config, DT_MODEf, dt_mode);
    if (config != oconfig) {
        SOC_IF_ERROR_RETURN(WRITE_EGR_CONFIGr(unit, config));
    }
    /* Record double tag mode for use in DCBs */
    SOC_DT_MODE(unit) = (dt_mode != 0);

    if (SOC_REG_IS_VALID(unit, EGR_PORT_64r)) {
        SOC_IF_ERROR_RETURN(READ_EGR_PORT_64r(unit, port, &config64));
        oconfig64 = config64;
        soc_reg64_field32_set(unit, EGR_PORT_64r, &config64, NNI_PORTf, !ignore_tag); 
        if (COMPILER_64_NE(config64, oconfig64)) {
            SOC_IF_ERROR_RETURN(WRITE_EGR_PORT_64r(unit, port, config64));
        }
    } else {
        SOC_IF_ERROR_RETURN(READ_EGR_PORTr(unit, port, &config));
        oconfig = config;
        soc_reg_field_set(unit, EGR_PORTr, &config, NNI_PORTf, !ignore_tag); 
        if (config != oconfig) {
            SOC_IF_ERROR_RETURN(WRITE_EGR_PORTr(unit, port, config));
        }
    }

    soc_mem_lock(unit, PORT_TABm);

    rv = mbcm_driver[unit]->mbcm_port_cfg_get(unit, port, &pcfg);
    if (BCM_SUCCESS(rv) && (pcfg.pc_nni_port == ignore_tag)) {
        pcfg.pc_nni_port = !ignore_tag;
        rv = mbcm_driver[unit]->mbcm_port_cfg_set(unit, port, &pcfg);
    }

    soc_mem_unlock(unit, PORT_TABm);
    return rv;
}

/*
 * Function:
 *      _bcm_fb_port_dtag_mode_get
 * Description:
 *      Return the current double-tagging mode of a port.
 * Parameters:
 *      unit - (IN) BCM device number
 *      port - (IN) Port number
 *      mode - (OUT) Double-tagging mode
 * Return Value:
 *      BCM_E_XXX
 */
int
_bcm_fb_port_dtag_mode_get(int unit, bcm_port_t port, int *mode)
{
    uint32           config;
    bcm_port_cfg_t   pcfg;
    int              rv;

    SOC_IF_ERROR_RETURN(READ_ING_CONFIGr(unit, &config));
    if (soc_reg_field_get(unit, ING_CONFIGr, config, DT_MODEf) == 0) {
        *mode = BCM_PORT_DTAG_MODE_NONE;
    } else {
        rv = mbcm_driver[unit]->mbcm_port_cfg_get(unit, port, &pcfg);
        BCM_IF_ERROR_RETURN(rv);

        if (pcfg.pc_nni_port) {
            *mode = BCM_PORT_DTAG_MODE_INTERNAL;
        } else {
            *mode = BCM_PORT_DTAG_MODE_EXTERNAL;
        }
    }

#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAVEN_SUPPORT)
    if (SOC_REG_FIELD_VALID(unit, EGR_SRC_PORTr, ADD_INNER_TAGf)) {
        uint32 value;

        SOC_IF_ERROR_RETURN(READ_EGR_SRC_PORTr(unit, port, &value));
        if (soc_reg_field_get(unit, EGR_SRC_PORTr,
                              value, ADD_INNER_TAGf)) {
            *mode |= BCM_PORT_DTAG_ADD_EXTERNAL_TAG;
        }

        SOC_IF_ERROR_RETURN(READ_EGR_VLAN_CONTROL_1r(unit, port, &value));
        if (soc_reg_field_get(unit, EGR_VLAN_CONTROL_1r, 
                              value, REMOVE_INNER_TAGf)) {
            *mode |= BCM_PORT_DTAG_REMOVE_EXTERNAL_TAG;
        }
    } 
#endif /* BCM_FIREBOLT2_SUPPORT || BCM_RAVEN_SUPPORT */
    return BCM_E_NONE;
}

#endif  /* BCM_FIREBOLT_SUPPORT */
