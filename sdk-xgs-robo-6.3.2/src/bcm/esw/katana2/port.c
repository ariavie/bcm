/*
 * $Id: port.c 1.35.2.1 Broadcom SDK $
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
 *
 */


#include <soc/defs.h>
#include <soc/debug.h>
#include <soc/drv.h>
#include <soc/mem.h>

#include <sal/core/time.h>

#include <bcm/port.h>
#include <bcm/tx.h>
#include <bcm/error.h>
#include <bcm_int/esw/katana2.h>

#include <soc/katana2.h>
#include <soc/phyreg.h>

#if defined(BCM_KATANA2_SUPPORT)
#include <bcm_int/esw_dispatch.h>
#include <bcm_int/esw/katana.h>
#include <bcm_int/esw/katana2.h>
#include <bcm_int/esw/virtual.h>

/* 56450 port types */
#define KT2_PORT_TYPE_ETHERNET 0
#define KT2_PORT_TYPE_HIGIG    1
#define KT2_PORT_TYPE_LOOPBACK 2
#define KT2_PORT_TYPE_CASCADED 4
#define KT2_PORT_TYPE_GPON_EPON 5
#define KT2_CASCADED_TYPE_LINKPHY 0
#define KT2_CASCADED_TYPE_SUBTAG  1

static uint32             new_tdm[256] = {0};
extern int mxqblock_max_startaddr[KT2_MAX_MXQBLOCKS];
extern int mxqblock_max_endaddr[KT2_MAX_MXQBLOCKS];



/*
 * Function:
 *      _bcm_kt2_port_cfg_init
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
_bcm_kt2_port_cfg_init(int unit, bcm_port_t port, bcm_vlan_data_t *vlan_data)
{
    port_tab_entry_t ptab;                 /* Port table entry. */
    soc_field_t field_ids[SOC_MAX_MEM_FIELD_WORDS];
    uint32 field_values[SOC_MAX_MEM_FIELD_WORDS];
    uint32 cascaded_type = 0, port_type = 0;
    int field_count = 0;
    int key_type_value = 0;
    bcm_pbmp_t port_pbmp;

    if (port < 42) {
        if (IS_ST_PORT(unit, port)) {
            port_type = KT2_PORT_TYPE_HIGIG;
        } else if (IS_LB_PORT(unit, port)) {
            port_type = KT2_PORT_TYPE_LOOPBACK;
        } else if (IS_LP_PORT(unit, port)) {
            port_type = KT2_PORT_TYPE_CASCADED;
            cascaded_type = KT2_CASCADED_TYPE_LINKPHY;
        } else if (IS_SUBTAG_PORT(unit, port)) {
            port_type = KT2_PORT_TYPE_CASCADED;
            cascaded_type = KT2_CASCADED_TYPE_SUBTAG;
        } else {
            port_type = KT2_PORT_TYPE_ETHERNET;
        }

        BCM_IF_ERROR_RETURN(soc_mem_field32_modify(
                   unit, ING_PHYSICAL_PORT_TABLEm, port,
                   PORT_TYPEf,port_type));
        BCM_IF_ERROR_RETURN(soc_mem_field32_modify(
                   unit, ING_PHYSICAL_PORT_TABLEm, port,
                   CASCADED_PORT_TYPEf, cascaded_type));
        BCM_IF_ERROR_RETURN(soc_mem_field32_modify(
                       unit, EGR_PHYSICAL_PORTm, port,
                       PORT_TYPEf,port_type));
        BCM_IF_ERROR_RETURN(soc_mem_field32_modify(
                   unit, ING_PHYSICAL_PORT_TABLEm, port,
                   CASCADED_PORT_TYPEf, cascaded_type));

        if (IS_CPU_PORT(unit,port)) {
            if (SOC_INFO(unit).cpu_hg_index != -1) {
                BCM_IF_ERROR_RETURN(soc_mem_field32_modify(
                       unit, ING_PHYSICAL_PORT_TABLEm,
                       SOC_INFO(unit).cpu_hg_index, PORT_TYPEf, 1));
                BCM_IF_ERROR_RETURN(soc_mem_field32_modify(
                       unit, EGR_PHYSICAL_PORTm, SOC_INFO(unit).cpu_hg_index,
                       PORT_TYPEf, 1));
            }
        }


        /* ingress physical port to pp_port mapping */
        BCM_IF_ERROR_RETURN(soc_mem_field32_modify(
            unit, ING_PHYSICAL_PORT_TABLEm, port, PP_PORTf,
            (port_type != KT2_PORT_TYPE_CASCADED) ? port : 0xff));

        /* pp_port to egress physical port mapping */
        BCM_IF_ERROR_RETURN(soc_mem_field32_modify(
           unit, PP_PORT_TO_PHYSICAL_PORT_MAPm, port, DESTINATIONf,port));
    }

    field_ids[field_count] = EN_EFILTERf;
    field_values[field_count] = 1;
    field_count ++;

    /* Initialize egress vlan translation port class with identity mapping */
    BCM_PBMP_CLEAR(port_pbmp);
    BCM_PBMP_ASSIGN(port_pbmp, PBMP_PORT_ALL(unit));
    if (soc_feature(unit, soc_feature_flex_port)) {
        BCM_IF_ERROR_RETURN(_bcm_kt2_flexio_pbmp_update(unit, &port_pbmp));
    }

    field_ids[field_count] = VT_PORT_GROUP_IDf;
    field_values[field_count] =
        SOC_PBMP_MEMBER(port_pbmp, port) ? port : 0;
    field_count++;

    /* For some devices, directed mirroring will always be 1 */
    if (soc_feature(unit, soc_feature_directed_mirror_only)) {
        field_ids[field_count] = EM_SRCMOD_CHANGEf;
        field_values[field_count] = 1;
        field_count++;
    }

    if (SOC_MEM_IS_VALID(unit, EGR_PORTm)) {
        BCM_IF_ERROR_RETURN 
            (soc_mem_fields32_modify(unit, EGR_PORTm, port, field_count,
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


    if (SOC_MEM_FIELD_VALID(unit, PORT_TABm, TRUST_INCOMING_VIDf)) {
        soc_PORT_TABm_field32_set(unit, &ptab, TRUST_INCOMING_VIDf, 1);

        /* Set identify mapping for pri/cfi re-mapping */
        if (SOC_MEM_FIELD_VALID(unit, PORT_TABm, PRI_MAPPINGf)) {
            soc_PORT_TABm_field32_set(unit, &ptab, PRI_MAPPINGf, 0xfac688);
            soc_PORT_TABm_field32_set(unit, &ptab, CFI_0_MAPPINGf, 0);
            soc_PORT_TABm_field32_set(unit, &ptab, CFI_1_MAPPINGf, 1);
        }
    
        /* Set identify mapping for ipri/icfi re-mapping */
        if (SOC_MEM_FIELD_VALID(unit, PORT_TABm, IPRI_MAPPINGf)) {
            soc_PORT_TABm_field32_set(unit, &ptab, IPRI_MAPPINGf, 0xfac688);
            soc_PORT_TABm_field32_set(unit, &ptab, ICFI_0_MAPPINGf, 0);
            soc_PORT_TABm_field32_set(unit, &ptab, ICFI_1_MAPPINGf, 1);
        }
    
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
                         KT2_VLXLT_HASH_KEY_TYPE_OVID,&key_type_value));
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
                         KT2_VLXLT_HASH_KEY_TYPE_IVID,&key_type_value));
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
    }

    BCM_IF_ERROR_RETURN(BCM_XGS3_MEM_WRITE(unit, PORT_TABm, port, &ptab));

    return (BCM_E_NONE);
}

int
bcm_kt2_port_cfg_init(int unit, bcm_port_t port, bcm_vlan_data_t *vlan_data)
{
#ifdef BCM_KATANA2_SUPPORT
    if(SOC_IS_KATANA2(unit)) {
        return _bcm_kt2_port_cfg_init(unit, port, vlan_data);
    }
#endif /* BCM_KATANA2_SUPPORT */
    return (BCM_E_NONE);
}
int
_bcm_kt2_port_lanes_set_post_operation(int unit, bcm_port_t port)
{
    soc_error_t        rv = SOC_E_NONE;
#if 0
    uint32             rval = 0;
#endif
    uint8              loop = 0;
    uint8              mxqblock = 0;
    uint8              mxqblock_port = 0;
    int                mode = 0;
    int                new_lanes = 0;
    egr_enable_entry_t egr_enable_entry = {{0}};
#if 0
    soc_timeout_t      to = {0};
    sal_usecs_t        timeout_usec = 100000; /* 100ms */
    int                min_polls = 100;
#endif

    sal_memset(&egr_enable_entry, 0, sizeof(egr_enable_entry_t));
    soc_mem_field32_set(unit, EGR_ENABLEm, &egr_enable_entry, PRT_ENABLEf, 1);

    SOC_IF_ERROR_RETURN(soc_katana2_get_port_mxqblock(
                        unit,port,&mxqblock));
    SOC_IF_ERROR_RETURN(_bcm_kt2_port_lanes_get(
                        unit,port,&new_lanes));

#if 0
    /* Bring-out of Soft-reset relevant MXQPORT(s) */
    /* Delay of 100ms */
    soc_timeout_init(&to, timeout_usec, min_polls);
    if(soc_timeout_check(&to)) {
       soc_cm_debug(DK_ERR, 
              "_bcm_kt2_port_lanes_set_post_operation:TimeOut InternalError\n");
       return SOC_E_INTERNAL; 
    }
    soc_katana2_mxqblock_reset(unit, mxqblock, 1);
#endif

    for (loop=0;loop<KT2_MAX_MXQPORTS_PER_BLOCK;loop++) {
         mxqblock_port=kt2_mxqblock_ports[mxqblock][loop];
         if (SOC_PBMP_MEMBER((PBMP_ALL(unit)), mxqblock_port)) {
             BCM_IF_ERROR_RETURN(bcm_esw_port_enable_set(unit,mxqblock_port,1));
             kt2_tdm_update_flag=0;
             rv = bcm_esw_port_speed_set(unit, mxqblock_port ,
                                 mxqspeeds[unit][mxqblock][new_lanes-1]);
             kt2_tdm_update_flag=1;
             SOC_IF_ERROR_RETURN(rv);
             SOC_IF_ERROR_RETURN(bcm_esw_linkscan_mode_get(
                                 unit, mxqblock_port, &mode));
             if (mode == BCM_LINKSCAN_MODE_NONE) {
                 BCM_IF_ERROR_RETURN(bcm_esw_linkscan_mode_set(
                                     unit,mxqblock_port,BCM_LINKSCAN_MODE_SW));
                 BCM_IF_ERROR_RETURN(bcm_esw_port_learn_set(unit, mxqblock_port,
                                     BCM_PORT_LEARN_ARL | BCM_PORT_LEARN_FWD));
                 BCM_IF_ERROR_RETURN(bcm_esw_port_stp_set(
                                     unit, mxqblock_port, BCM_STG_STP_FORWARD));
             }
#if 0
             BCM_IF_ERROR_RETURN(soc_mem_write(
                                 unit, EGR_ENABLEm, MEM_BLOCK_ALL,
                                 mxqblock_port, &egr_enable_entry));
             /* 6. PowerDown MXQBlock/WarpCore */
             READ_XPORT_XGXS_CTRLr(unit, mxqblock_port,&rval);
             soc_reg_field_set(unit, XPORT_XGXS_CTRLr, &rval,
                               PWRDWNf,0);
             WRITE_XPORT_XGXS_CTRLr(unit, mxqblock_port,rval);
#endif
         }
    }
    return SOC_E_NONE;
}
static 
void soc_katana2_pbmp_one_resync(int unit,soc_ptype_t *ptype)
{
    soc_port_t it_port = 0 ;

    ptype->num = 0; \
    ptype->min = ptype->max = -1;

    PBMP_ITER(ptype->bitmap, it_port) {
              ptype->port[ptype->num++] = it_port;
              if (ptype->min < 0) { \
                  ptype->min = it_port;
              }
              if (it_port > ptype->max) {
                  ptype->max = it_port;
              }
    }
}
static 
void soc_katana2_pbmp_all_resync(int unit)
{
    soc_info_t *si = &SOC_INFO(unit);
    soc_katana2_pbmp_one_resync(unit,&si->xe);
    soc_katana2_pbmp_one_resync(unit,&si->ether);
    soc_katana2_pbmp_one_resync(unit,&si->mxq);
    soc_katana2_pbmp_one_resync(unit,&si->port);
    soc_katana2_pbmp_one_resync(unit,&si->all);
    soc_katana2_pbmp_one_resync(unit,&si->ge);
    soc_katana2_pbmp_one_resync(unit,&si->hg);
    soc_katana2_pbmp_one_resync(unit,&si->st);
}
static void soc_katana2_pbmp_remove(int unit,soc_port_t port)
{
    int         blk = 0;
    soc_info_t *si = &SOC_INFO(unit);

    blk = SOC_DRIVER(unit)->port_info[port].blk;
    SOC_PBMP_PORT_REMOVE(si->block_bitmap[blk],port);

    SOC_PBMP_PORT_REMOVE(si->xe.bitmap, port);
    SOC_PBMP_PORT_ADD(si->xe.disabled_bitmap, port);

    SOC_PBMP_PORT_REMOVE(si->ether.bitmap, port);
    SOC_PBMP_PORT_ADD(si->ether.disabled_bitmap, port);

    SOC_PBMP_PORT_REMOVE(si->mxq.bitmap, port);
    SOC_PBMP_PORT_ADD(si->mxq.disabled_bitmap, port);

    SOC_PBMP_PORT_REMOVE(si->port.bitmap, port);
    SOC_PBMP_PORT_ADD(si->port.disabled_bitmap, port);

    SOC_PBMP_PORT_REMOVE(si->all.bitmap, port);
    SOC_PBMP_PORT_ADD(si->all.disabled_bitmap, port);

    SOC_PBMP_PORT_REMOVE(si->ge.bitmap, port);
    SOC_PBMP_PORT_ADD(si->ge.disabled_bitmap, port);

    SOC_PBMP_PORT_REMOVE(si->hg.bitmap, port);
    SOC_PBMP_PORT_ADD(si->hg.disabled_bitmap, port);

    SOC_PBMP_PORT_REMOVE(si->st.bitmap, port);
    SOC_PBMP_PORT_ADD(si->st.disabled_bitmap, port);

}
static 
void soc_katana2_pbmp_add(int unit,soc_port_t port,int mxqblock,int speed) {
    int blk=0;
    soc_info_t *si = &SOC_INFO(unit);

    blk = SOC_DRIVER(unit)->port_info[port].blk;
    SOC_PBMP_PORT_ADD(si->block_bitmap[blk],port);


    if ((mxqblock >=0) && (mxqblock <= 5)) {
         if (speed == 10000) {
             SOC_PBMP_PORT_ADD(si->xe.bitmap, port);
             SOC_PBMP_PORT_REMOVE(si->xe.disabled_bitmap, port);
         } else {
             SOC_PBMP_PORT_ADD(si->ge.bitmap, port);
             SOC_PBMP_PORT_REMOVE(si->xe.disabled_bitmap, port);
         }
         SOC_PBMP_PORT_ADD(si->ether.bitmap, port);
         SOC_PBMP_PORT_REMOVE(si->ether.disabled_bitmap, port);

         SOC_PBMP_PORT_ADD(si->mxq.bitmap, port);
         SOC_PBMP_PORT_REMOVE(si->mxq.disabled_bitmap, port);

         SOC_PBMP_PORT_ADD(si->port.bitmap, port);
         SOC_PBMP_PORT_REMOVE(si->port.disabled_bitmap, port);

         SOC_PBMP_PORT_ADD(si->all.bitmap, port);
         SOC_PBMP_PORT_REMOVE(si->all.disabled_bitmap, port);
    }
    if ((mxqblock >=6) && (mxqblock <= 9)) {
         if (speed >= 13000) {
             SOC_PBMP_PORT_ADD(si->hg.bitmap, port);
             SOC_PBMP_PORT_ADD(si->st.bitmap, port);
             SOC_PBMP_PORT_REMOVE(si->hg.disabled_bitmap, port);
             SOC_PBMP_PORT_REMOVE(si->st.disabled_bitmap, port);
         } else if (speed == 10000) {
             SOC_PBMP_PORT_ADD(si->xe.bitmap, port);
             SOC_PBMP_PORT_REMOVE(si->xe.disabled_bitmap, port);
         } else {
             SOC_PBMP_PORT_ADD(si->ge.bitmap, port);
             SOC_PBMP_PORT_REMOVE(si->ge.disabled_bitmap, port);
         }
         SOC_PBMP_PORT_ADD(si->ether.bitmap, port);
         SOC_PBMP_PORT_REMOVE(si->ether.disabled_bitmap, port);

         SOC_PBMP_PORT_ADD(si->mxq.bitmap, port);
         SOC_PBMP_PORT_REMOVE(si->mxq.disabled_bitmap, port);

         SOC_PBMP_PORT_ADD(si->port.bitmap, port);
         SOC_PBMP_PORT_REMOVE(si->port.disabled_bitmap, port);
         SOC_PBMP_PORT_ADD(si->all.bitmap, port);
         SOC_PBMP_PORT_REMOVE(si->all.disabled_bitmap, port);
    }
    SOC_PORT_TYPE(unit,port)=SOC_BLK_MXQPORT;
    soc_dport_map_port(unit, port,port);
    soc_katana2_pbmp_all_resync(unit) ;
    soc_dport_map_update(unit);
    si->port_speed_max[port] = speed;
}

static
soc_error_t soc_katana2_port_attach(
            int unit,uint8 mxqblock ,uint8 mxqblock_port,uint32 port_speed)
{
    uint16 phy_addr=0;
    uint16 phy_addr_int=0;
    int    okay;

    soc_linkscan_pause(unit);
    _katana2_phy_addr_default(unit, mxqblock_port,
                              &phy_addr, &phy_addr_int);
    SOC_IF_ERROR_RETURN
        (soc_phy_cfg_addr_set(unit,mxqblock_port,0, phy_addr));
    SOC_IF_ERROR_RETURN
        (soc_phy_cfg_addr_set(unit,mxqblock_port,SOC_PHY_INTERNAL,
                              phy_addr_int));
    PHY_ADDR(unit, mxqblock_port)     = phy_addr;
    PHY_ADDR_INT(unit, mxqblock_port) = phy_addr_int;
    SOC_IF_ERROR_RETURN(_bcm_port_probe(unit, mxqblock_port, &okay));
    if (!okay) {
        soc_linkscan_continue(unit);
        return SOC_E_INTERNAL;
    }
    SOC_IF_ERROR_RETURN(_bcm_port_mode_setup(unit, mxqblock_port, FALSE));
    if (port_speed >= 13000) {
        bcm_esw_port_encap_set(unit,mxqblock_port,BCM_PORT_ENCAP_HIGIG);
    } else {
        bcm_esw_port_encap_set(unit,mxqblock_port,BCM_PORT_ENCAP_IEEE);
    }
    bcm_esw_port_enable_set(unit,mxqblock_port,1);
    soc_linkscan_continue(unit);
    return SOC_E_NONE;
}
static 
soc_error_t soc_katana2_port_detach(int unit,uint8 mxqblock_port)
{
    soc_linkscan_pause(unit);

    SOC_IF_ERROR_RETURN(soc_phyctrl_detach(unit, mxqblock_port));
    PHY_FLAGS_CLR_ALL(unit, mxqblock_port);
    bcm_esw_port_enable_set(unit,mxqblock_port,0);

    soc_linkscan_continue(unit);
    return SOC_E_NONE;
}
int _bcm_kt2_update_port_mode(int unit,uint8 port,int speed)
{
   uint32 rval = 0;
   uint32 phy_mode = 0;
   uint32 core_mode = 0;
   uint32 wc_10g_21g_sel = 0;
   uint8  mxqblock = 0;
   bcmMxqConnection_t connection_mode;

   SOC_IF_ERROR_RETURN(READ_XPORT_MODE_REGr(unit, port, &rval));
   SOC_IF_ERROR_RETURN(soc_katana2_get_phy_port_mode(
                       unit, port, speed, &phy_mode));
   SOC_IF_ERROR_RETURN(soc_katana2_get_core_port_mode(
                       unit, port, &core_mode));
   soc_reg_field_set(unit, XPORT_MODE_REGr, &rval, CORE_PORT_MODEf, core_mode);
   SOC_IF_ERROR_RETURN(soc_katana2_get_port_mxqblock( unit,port,&mxqblock));
   if ((mxqblock == 8) || (mxqblock == 9)) {
        if ((speed == 10000) || (speed == 21000)) {
             wc_10g_21g_sel = 1;
        }
   }
   if ((mxqblock == 6) || (mxqblock == 7)) {
        if (speed == 10000) {
            SOC_IF_ERROR_RETURN(soc_katana2_get_phy_connection_mode(
                                unit,port, mxqblock,&connection_mode));
            /* XFI Mode */
            if (connection_mode == bcmMqxConnectionWarpCore) {
                wc_10g_21g_sel = 1;
            }
        }
   }
   soc_reg_field_set(unit, XPORT_MODE_REGr, &rval, 
                     WC_10G_21G_SELf, wc_10g_21g_sel);
   soc_reg_field_set(unit, XPORT_MODE_REGr, &rval, PHY_PORT_MODEf, phy_mode);
   soc_reg_field_set(unit, XPORT_MODE_REGr, &rval,
                     PORT_GMII_MII_ENABLEf, (speed >= 10000) ? 0 : 1);
   /* WORK AROUND: KATANA2-1698
      RTL PORT: Hotswap from quad port mode to dual port mode fails 
      occasionally. */
   soc_reg_field_set(unit, XPORT_MODE_REGr, &rval, CORE_PORT_MODEf, 
                    bcmMxqCorePortModeSingle);
   SOC_IF_ERROR_RETURN(WRITE_XPORT_MODE_REGr(unit, port, rval));
   sal_udelay(100);

   soc_reg_field_set(unit, XPORT_MODE_REGr, &rval, CORE_PORT_MODEf, 
                    core_mode);
   SOC_IF_ERROR_RETURN(WRITE_XPORT_MODE_REGr(unit, port, rval));
   return BCM_E_NONE;
}

int _bcm_kt2_port_lanes_set(int unit, bcm_port_t port, int lanes)
{
    uint64                xmac_ctrl;
    uint32                port_enable=0;
    epc_link_bmap_entry_t epc_link_bmap_entry={{0}};
    txlp_port_addr_map_table_entry_t txlp_port_addr_map_table_entry={{0}};
    soc_field_t port_enable_field[KT2_MAX_MXQPORTS_PER_BLOCK]=
                                 {PORT0f, PORT1f, PORT2f , PORT3f};

    uint32                start_addr=0;
    uint32                end_addr=0;
    soc_pbmp_t            link_pbmp;
    uint32                flush_reg=0;
    uint32             time_multiplier=1;
    uint64             rval64 ;
    uint32             rval = 0;
    uint32             rval1 = 0;
    uint32             rval2 = 0;
    egr_enable_entry_t egr_enable_entry = {{0}};
    uint32             cell_cnt = 0;
    uint32             try = 0;
    uint32             try_count = 0;
    soc_timeout_t      to = {0};
    sal_usecs_t        timeout_usec = 100000;
    int                min_polls = 100;

    uint32             pfc_enable=0;
    int                pfc_index=0;
    uint8              old_ports[][KT2_MAX_MXQPORTS_PER_BLOCK]=
                                  {{1,0,0,0},{1,0,3,0},{0,0,0,0},{1,2,3,4}};
    uint8              new_ports[][KT2_MAX_MXQPORTS_PER_BLOCK]=
                                  {{1,0,0,0},{1,0,3,0},{0,0,0,0},{1,2,3,4}};
    uint8              new_port_mode[5]={0,1,2,0,3};
    uint32             top_misc_control_1_val = 0;
    soc_field_t        wc_xfi_mode_sel_fld[] =
                       {WC0_8_XFI_MODE_SELf,WC1_8_XFI_MODE_SELf};
    soc_info_t         *si = &SOC_INFO(unit);
    uint8              mxqblock = 0;
    uint8              mxqblock_port = 0;
    uint8              try_loop = 0;
    uint8              loop = 0;
    int                old_lanes = 0;
    int                new_lanes = lanes;
    uint8              xfi_mode[2] = {0,0};
    /* uint8              old_xfi_mode[2] = {0,0}; */
    soc_field_t        port_intf_reset_fld[] = {
                       XQ0_PORT_INTF_RESETf, XQ1_PORT_INTF_RESETf,
                       XQ2_PORT_INTF_RESETf, XQ3_PORT_INTF_RESETf,
                       XQ4_PORT_INTF_RESETf, XQ5_PORT_INTF_RESETf,
                       XQ6_PORT_INTF_RESETf, XQ7_PORT_INTF_RESETf};
    soc_field_t        mmu_intf_reset_fld[] = {
                       XQ0_MMU_INTF_RESETf, XQ1_MMU_INTF_RESETf,
                       XQ2_MMU_INTF_RESETf, XQ3_MMU_INTF_RESETf,
                       XQ4_MMU_INTF_RESETf, XQ5_MMU_INTF_RESETf,
                       XQ6_MMU_INTF_RESETf, XQ7_MMU_INTF_RESETf};
    soc_field_t        new_port_mode_fld[] = {
                       XQ0_NEW_PORT_MODEf, XQ1_NEW_PORT_MODEf,
                       XQ2_NEW_PORT_MODEf, XQ3_NEW_PORT_MODEf,
                       XQ4_NEW_PORT_MODEf, XQ5_NEW_PORT_MODEf,
                       XQ6_NEW_PORT_MODEf, XQ7_NEW_PORT_MODEf};
    uint8              required_tdm_slots = 0;
    uint8              available_tdm_slots = 0;
    uint8              speed_index = 0;
    uint8              speed_value = 0;
    int                port_speed = 0;
    uint8              tdm_ports[4] = {0};
    uint8              index = 0;
    uint32             pos = 0;
    uint32             egr_fifo_depth = 0;
    /* int             cfg_num = 0; */
    int                mxqblock_max_nxtaddr;
    char               config_str[80];


    if (!SOC_PORT_VALID(unit, port)) {
        return BCM_E_PORT;
    }
    if (SAL_BOOT_QUICKTURN) {
        time_multiplier=1000;
    }

    if ((new_lanes != 1) && (new_lanes != 2) && (new_lanes != 4)) {
         return BCM_E_PARAM;
    }

    /* cfg_num = soc_property_get(unit, spn_BCM5645X_CONFIG, 0); */

    if (SOC_PORT_TYPE(unit,port) != SOC_BLK_MXQPORT) {
        return SOC_E_CONFIG;
    }
    SOC_IF_ERROR_RETURN(soc_katana2_get_port_mxqblock(
                        unit,port,&mxqblock));
    SOC_IF_ERROR_RETURN(_bcm_kt2_port_lanes_get(
                        unit,port,&old_lanes));
    if(old_lanes == new_lanes) {
       return SOC_E_NONE;
    }

    if ((mxqblock == 8) || (mxqblock == 9)) {
        if (soc_mem_config_set == NULL) {
            soc_cm_print("ATTN:Auto PortGroup Config setting not possible \n");
            return SOC_E_FAIL;
        }
    }
    if (SOC_INFO(unit).olp_port) {
        if (port == KT2_OLP_PORT) {
            soc_cm_print("HotSwap is not supported for OLP port \n");
            return BCM_E_PARAM;
        }
    }
    if (mxqblock == 7 ) {
        if (SOC_INFO(unit).olp_port) {
            soc_cm_print("OLP port is being used so cannot hotswap on MXQ7\n");
            return BCM_E_PARAM;
        }
    }
    if ((mxqblock==8) || (mxqblock==9)) {
         for (loop=0;loop<KT2_MAX_MXQPORTS_PER_BLOCK;loop++) {
              if (IS_LP_PORT(unit, kt2_mxqblock_ports[mxqblock][loop])) {
                  soc_cm_print("HotSwap is not supported for LinkPhy Ports\n");
                  return BCM_E_PARAM;
              }
         }
    }
    for (loop=0;loop<KT2_MAX_MXQPORTS_PER_BLOCK;loop++) {
         if (IS_SUBTAG_PORT(unit, kt2_mxqblock_ports[mxqblock][loop])) {
             soc_cm_print("HotSwap is not supported for SubTag/COE Ports\n");
             return BCM_E_PARAM;
         }
    }
    if ((mxqblock >= 6 ) && (mxqblock <= 9)) {
        SOC_IF_ERROR_RETURN(READ_TOP_MISC_CONTROL_1r(
                            unit,&top_misc_control_1_val));
        if ((mxqblock == 6) || (mxqblock == 8)) {
             xfi_mode[0]= soc_reg_field_get(unit, TOP_MISC_CONTROL_1r,
                                            top_misc_control_1_val,
                                            wc_xfi_mode_sel_fld[0]);
        }
        if (xfi_mode[0]) {
            soc_cm_print("XFI Mode enabled. Hot Swapping not possible \n");
            return SOC_E_UNAVAIL;
        }
        if ((mxqblock == 7) || (mxqblock == 9)) {
             xfi_mode[1]= soc_reg_field_get(unit, TOP_MISC_CONTROL_1r,
                                       top_misc_control_1_val,
                                       wc_xfi_mode_sel_fld[1]);
        }
        if (xfi_mode[1]) {
            soc_cm_print("XFI Mode enabled. Hot Swapping not possible \n");
            return SOC_E_UNAVAIL;
        }
    }

/*
    if ((new_lanes==4)&& ((mxqblock==8) || (mxqblock==9))) {
         for (loop=0;loop<KT2_MAX_MXQPORTS_PER_BLOCK;loop++) {
              if (SOC_PBMP_MEMBER((PBMP_ALL(unit)), 
                                   kt2_mxqblock_ports[mxqblock-2][loop])) {
                  soc_cm_print("MXQBlock%d being used so"
                               " WC cannot be used in XFI mode \n",mxqblock-2);
                  return SOC_E_CONFIG;
              }
         }
         xfi_mode[mxqblock-8]=1;
         for (loop=0;loop<KT2_MAX_MXQPORTS_PER_BLOCK;loop++) {
              if (SOC_PBMP_MEMBER((PBMP_ALL(unit)), 
                                   kt2_mxqblock_ports[mxqblock][loop])) {
                  if (!SOC_PBMP_MEMBER((PBMP_XE_ALL(unit)), 
                                        kt2_mxqblock_ports[mxqblock][loop])) {
                      xfi_mode[mxqblock-8]=0;
                      break;
                  }
              }
         }
         if (xfi_mode[mxqblock-8] == 1) {
             soc_cm_print("NEW XFI MODE ON \n");
         }
    }
    if ((mxqblock==8) || (mxqblock==9)) {
         old_xfi_mode[mxqblock-8]=1;
         for (loop=0;loop<KT2_MAX_MXQPORTS_PER_BLOCK;loop++) {
              if (SOC_PBMP_MEMBER((PBMP_ALL(unit)), 
                                   kt2_mxqblock_ports[mxqblock][loop])) {
                  if (!SOC_PBMP_MEMBER((PBMP_XE_ALL(unit)), 
                                   kt2_mxqblock_ports[mxqblock][loop])) {
                      old_xfi_mode[mxqblock-8]=0;
                      break;
                  }
              }
         }
         if (old_xfi_mode[mxqblock-8] == 1) {
             soc_cm_print("OLD XFI MODE ON \n");
         }
    }
*/
    switch(mxqspeeds[unit][mxqblock][new_lanes-1]) {
    case 1000: speed_index=0;break;
    case 2500: speed_index=2;break;
    case 10000: speed_index=3;break;
    case 13000: speed_index=4;break;
    case 21000: speed_index=6;break;
    default:    return SOC_E_CONFIG;
    }
    required_tdm_slots = kt2_current_tdm_cycles_info[speed_index].
                         min_tdm_cycles * new_lanes;
    available_tdm_slots = kt2_tdm_pos_info[mxqblock].total_slots;
/*
    if (((mxqblock==8) || (mxqblock==9)) && (xfi_mode[mxqblock-8]==1)) {
          available_tdm_slots += kt2_tdm_pos_info[mxqblock-2].total_slots;
    }
*/
    if (required_tdm_slots > available_tdm_slots) {
        soc_cm_print("TDM feasibility failed required_tdm_slots:%d >"
                     "available_tdm_slots:%d \n", required_tdm_slots,
                      available_tdm_slots);
        return SOC_E_CONFIG; 
    }
    if (new_lanes == 1) {
        /* 
        if (((mxqblock==8) || (mxqblock==9)) && (old_xfi_mode[mxqblock-8]==1)) {
             tdm_ports[0]=kt2_mxqblock_ports[mxqblock][0];
             tdm_ports[1]=KT2_IDLE;
             tdm_ports[2]=kt2_mxqblock_ports[mxqblock][0];
             tdm_ports[3]=KT2_IDLE;
        } else */ {
             tdm_ports[0]=kt2_mxqblock_ports[mxqblock][0];
             tdm_ports[1]=kt2_mxqblock_ports[mxqblock][0];
             tdm_ports[2]=kt2_mxqblock_ports[mxqblock][0];
             tdm_ports[3]=kt2_mxqblock_ports[mxqblock][0];
        }
    }
    if (new_lanes == 2) {
        /* 
        if (((mxqblock==8) || (mxqblock==9)) && (old_xfi_mode[mxqblock-8]==1)) {
            tdm_ports[0]=kt2_mxqblock_ports[mxqblock][0];
            tdm_ports[1]=kt2_mxqblock_ports[mxqblock][2];
            tdm_ports[2]=kt2_mxqblock_ports[mxqblock][0];
            tdm_ports[3]=kt2_mxqblock_ports[mxqblock][2];
        } else */ {
            tdm_ports[0]=kt2_mxqblock_ports[mxqblock][0];
            tdm_ports[1]=kt2_mxqblock_ports[mxqblock][2];
            tdm_ports[2]=kt2_mxqblock_ports[mxqblock][0];
            tdm_ports[3]=kt2_mxqblock_ports[mxqblock][2];
       }
    }
    if (new_lanes == 4) {
        /* 
        if (((mxqblock==8) || (mxqblock==9)) && (xfi_mode[mxqblock-8]==1)) {
             tdm_ports[0]=kt2_mxqblock_ports[mxqblock-2][0];
             tdm_ports[1]=kt2_mxqblock_ports[mxqblock-2][2];
             tdm_ports[2]=kt2_mxqblock_ports[mxqblock][0];
             tdm_ports[3]=kt2_mxqblock_ports[mxqblock][2];
        } else */ {
             tdm_ports[0]=kt2_mxqblock_ports[mxqblock][0];
             tdm_ports[1]=kt2_mxqblock_ports[mxqblock][1];
             tdm_ports[2]=kt2_mxqblock_ports[mxqblock][2];
             tdm_ports[3]=kt2_mxqblock_ports[mxqblock][3];
        }
    }
    sal_memcpy(&new_tdm[0],&kt2_current_tdm[0],
               kt2_current_tdm_size *sizeof(kt2_current_tdm[0]));
    /*
    if (((mxqblock==8) || (mxqblock==9)) && (xfi_mode[mxqblock-8]==1)) {
        index=0;
       for (loop=0; loop < kt2_tdm_pos_info[mxqblock-2].total_slots; loop++) {
            pos = kt2_tdm_pos_info[mxqblock-2].pos[loop];
                  new_tdm[pos]=tdm_ports[index];
                  index = (index + 1) %2;
             }
        for (loop=0; loop < kt2_tdm_pos_info[mxqblock].total_slots; loop++) {
              pos = kt2_tdm_pos_info[mxqblock].pos[loop];
                  new_tdm[pos]=tdm_ports[index+2];
                  index = (index + 1) %2;
             }

    } else */ {
        for (index=0,loop=0; loop < kt2_tdm_pos_info[mxqblock].total_slots; loop++) {
             pos = kt2_tdm_pos_info[mxqblock].pos[loop];
                      new_tdm[pos]=tdm_ports[index];
                  index = (index + 1) % 4 ;
            }
        }
    /*
    if (((mxqblock==8) || (mxqblock==9)) && (old_xfi_mode[mxqblock-8]==1)) {
          for (loop=0;loop < kt2_tdm_pos_info[mxqblock-2].total_slots; loop++) {
               pos = kt2_tdm_pos_info[mxqblock-2].pos[loop];
                  new_tdm[pos]=KT2_IDLE;
             }
        }
    if (((mxqblock==8) || (mxqblock==9)) && (xfi_mode[mxqblock-8]==1)) {
         index=0;
         for (loop=0; loop < kt2_tdm_pos_info[mxqblock-2].total_slots; loop++) {
              pos = kt2_tdm_pos_info[mxqblock-2].pos[loop];
                  new_tdm[pos]= kt2_mxqblock_ports[mxqblock-2][index];
                  index = (index + 2) %4;
             }
        }
    */
    /*
    kt2_tdm_display(new_tdm,kt2_current_tdm_size,
                    bcm56450_tdm_info[cfg_num].row,
                    bcm56450_tdm_info[cfg_num].col);
     */

    /* Stop Counter Thread for time-being */
    /* SOC_IF_ERROR_RETURN(soc_counter_stop(unit)); */

    sal_memset(&egr_enable_entry, 0, sizeof(egr_enable_entry_t));
    soc_mem_field32_set(unit, EGR_ENABLEm, &egr_enable_entry, PRT_ENABLEf, 0);
    for (loop=0;loop<KT2_MAX_MXQPORTS_PER_BLOCK;loop++) {
         mxqblock_port=kt2_mxqblock_ports[mxqblock][loop];
         if (SOC_PBMP_MEMBER((PBMP_ALL(unit)), mxqblock_port)) {
             /* 1. START: Stop LinkScanning for this port */
             bcm_esw_kt2_port_unlock(unit);
             BCM_IF_ERROR_RETURN(bcm_esw_linkscan_mode_set(
                                 unit, mxqblock_port, BCM_LINKSCAN_MODE_NONE));
             bcm_esw_kt2_port_lock(unit);

             /* 2. To stop the incoming traffic, disables XMAC in ingress(Rx) 
                   direction. This can be done by disabling RxEn bit in 
                   XMAC_Ctrl register in  MAC XMAC_Ctrl.<old_port>.RxEn= 0 */

             SOC_IF_ERROR_RETURN(READ_XMAC_CTRLr(
                                 unit, mxqblock_port, &xmac_ctrl));
             soc_reg64_field32_set(unit, XMAC_CTRLr, &xmac_ctrl, TX_ENf, 1);
             soc_reg64_field32_set(unit, XMAC_CTRLr, &xmac_ctrl, RX_ENf, 0);
             SOC_IF_ERROR_RETURN(WRITE_XMAC_CTRLr(
                                 unit, mxqblock_port, xmac_ctrl));
             sal_udelay((10*time_multiplier));

             /* 3. Brings the port down. This is done by programming 
                   EPC_LINK_BMAP[x] = 0; (x: port number)

                   This register resides in Sw2 block of ingress pipeline, by 
                   disabling this bit software will block all the new packets 
                   pertaining to this port from entering MMU except for packets
                   utilizing the extended queueing mechanism which may still 
                   be passed to the MMU.

                   Reset appropriate EPC_LINK_BMAP:PORT_BITMAP  */

             BCM_IF_ERROR_RETURN(READ_EPC_LINK_BMAPm(
                                 unit, MEM_BLOCK_ANY, 0, &epc_link_bmap_entry));
             soc_mem_pbmp_field_get(unit, EPC_LINK_BMAPm,
                          &epc_link_bmap_entry, PORT_BITMAPf,&link_pbmp);
             BCM_PBMP_PORT_REMOVE(link_pbmp,mxqblock_port);
             soc_mem_pbmp_field_set(unit, EPC_LINK_BMAPm,
                          &epc_link_bmap_entry, PORT_BITMAPf,&link_pbmp);
             SOC_IF_ERROR_RETURN(WRITE_EPC_LINK_BMAPm(
                                 unit, MEM_BLOCK_ALL, 0, &epc_link_bmap_entry));

             /* 4. Issues MMU port flush command 
                Software issues MMU port flush command (up to 4 ports can be 
                flushed in one command). MMU will drop any further enqueue 
                requests for queues within this port and will indicate port 
                flush command complete when all ports specified in the command 
                have drained all their packets from the MMU */

             BCM_IF_ERROR_RETURN(READ_TOQ_FLUSH0r(unit, &flush_reg));
             soc_reg_field_set(unit, TOQ_FLUSH0r, &flush_reg, 
                               FLUSH_ACTIVEf,1);
             /* PORT FLUSHING not QUEUE FLUSHING */
             soc_reg_field_set(unit, TOQ_FLUSH0r, &flush_reg, FLUSH_TYPEf,1);
             soc_reg_field_set(unit, TOQ_FLUSH0r, &flush_reg, FLUSH_NUMf,1);
             soc_reg_field_set(unit, TOQ_FLUSH0r, &flush_reg, 
                               FLUSH_ID0f, mxqblock_port);
             BCM_IF_ERROR_RETURN(WRITE_TOQ_FLUSH0r(unit, flush_reg));

             /* 5. Saves the current configuration of PAUSE/PFC flow control 
                   Then disables the PFC flow control to ensure all pending 
                   packets can flow through MMU.
                   This can be done by programming the following register in
                   MMU for the relevant port:
                   XPORT_TO_MMU_BKP.PRI15_0_BKP = 16d0; */
             BCM_IF_ERROR_RETURN(bcm_esw_port_speed_get(
                                 unit, mxqblock_port, &port_speed));
             if (port_speed >= 10000) {
                 if ((mxqblock >= 6) && (mxqblock <= 9)) {
                      switch(mxqblock_port) {
                      case 25: pfc_index=6;break;
                      case 36: pfc_index=7;break;
                      case 26: pfc_index=8;break;
                      case 39: pfc_index=9;break;
                      case 27: pfc_index=10;break;
                      case 33: pfc_index=11;break;
                      case 28: pfc_index=12;break;
                      case 30: pfc_index=13;break;
                      default: return BCM_E_INTERNAL;
                      }
                  } else {
                      pfc_index=mxqblock;
                  }
                  BCM_IF_ERROR_RETURN(READ_XPORT_TO_MMU_BKPr(
                                      unit, pfc_index, &pfc_enable));
                  BCM_IF_ERROR_RETURN(WRITE_XPORT_TO_MMU_BKPr(
                                      unit, pfc_index, 0));
             }

             /* 6. flushes any pending egress packets in MXQPORT, in this 
                   scenario MMU sends out the packets but they are dropped in 
                   the MXQPORT, hence the packets are drained out of MMU but 
                   are not sent out of the chip. This is done by programming 
                   the following: XP_TXFIFO_PKT_DROP_CTL.DROP_EN = 1b1; */

             READ_XP_TXFIFO_PKT_DROP_CTLr(unit, mxqblock_port,&rval);
             soc_reg_field_set(unit, XP_TXFIFO_PKT_DROP_CTLr, &rval, 
                               DROP_ENf,1);
             WRITE_XP_TXFIFO_PKT_DROP_CTLr(unit, mxqblock_port,rval);

             /* 7. Waits until all egress port packets are drained. 
                   This is done by making sure MMU is empty for cells destined 
                   for this port and MXQPORT FIFO is empty and no packets are 
                   being sent to MAC.  This is done by checking the following
                   i. MMU Port Flush command has completed by polling 
                        TOQ_FLUSH0.flush_active == 0
                   ii.XP_TXFIFO_CELL_CNT.CELL_CNT == 0

                   SW will have to poll for cell_cnt to be 0 for a few hundred 
                   clocks prior to concluding port TX flush completion */

             try_count=0;
             if (SAL_BOOT_SIMULATION) {
                 BCM_IF_ERROR_RETURN(READ_TOQ_FLUSH0r(unit, &rval));
                 soc_reg_field_set(unit, TOQ_FLUSH0r, &rval, FLUSH_ACTIVEf, 0);
                 BCM_IF_ERROR_RETURN(WRITE_TOQ_FLUSH0r(unit, rval));
             } 
             do {
                 soc_timeout_init(&to, timeout_usec, min_polls);
                 if(soc_timeout_check(&to)) {
                    soc_cm_debug(DK_ERR, "%s:%d:TimeOut InternalError\n",
                                 __FUNCTION__,__LINE__);
                    return BCM_E_INTERNAL;
                 }
                 try_count++;
                 BCM_IF_ERROR_RETURN(READ_TOQ_FLUSH0r(unit, &rval));
                 if (soc_reg_field_get(
                     unit, TOQ_FLUSH0r, rval, FLUSH_ACTIVEf) == 0) {
                     break;
                 }
             } while (try_count != 3);
             if (try_count == 3) {
                 return BCM_E_TIMEOUT;
             }
             try_count=0;
             if (SAL_BOOT_SIMULATION) {
                 BCM_IF_ERROR_RETURN(READ_XP_TXFIFO_CELL_CNTr(
                                     unit, mxqblock_port, &rval));
                 soc_reg_field_set(unit, XP_TXFIFO_CELL_CNTr,
                                   &rval, CELL_CNTf, 0);
                 BCM_IF_ERROR_RETURN(WRITE_XP_TXFIFO_CELL_CNTr(
                                     unit, mxqblock_port, rval));
             } 
             soc_timeout_init(&to, timeout_usec, min_polls);
             for (try=0; try<100 && try_count < 10 ; try++) {
                 if(soc_timeout_check(&to)) {
                    soc_cm_debug(DK_ERR, "%s:%d:TimeOut InternalError\n",
                                 __FUNCTION__,__LINE__);
                    return SOC_E_INTERNAL; 
                 }
                 BCM_IF_ERROR_RETURN(READ_XP_TXFIFO_CELL_CNTr (
                                     unit,mxqblock_port, &rval));
                 cell_cnt=soc_reg_field_get(unit, XP_TXFIFO_CELL_CNTr,     
                                            rval, CELL_CNTf);
                 if (cell_cnt == 0) {
                     try_count++;
                     break;     
                 }
             } 
             if (try == 100) {
                return SOC_E_TIMEOUT;
             }
    
             /*8. Since we have blocked ingress traffic and outgoing packets 
                  have been dropped in MXQPORT, it is safe to powerdown 
                  Unicore/Warpcore serdes.  This is done by programming the 
                  following:XPORT_XGXS_CTRL.Pwrdwn = 1 */
             /* READ_XPORT_XGXS_CTRLr(unit, mxqblock_port,&rval); */
             rval = 0;
             soc_reg_field_set(unit, XPORT_XGXS_CTRLr, &rval, 
                               LCREF_ENf,1);
             soc_reg_field_set(unit, XPORT_XGXS_CTRLr, &rval, 
                               IDDQf,1);
             soc_reg_field_set(unit, XPORT_XGXS_CTRLr, &rval, 
                               PWRDWNf,1);
             WRITE_XPORT_XGXS_CTRLr(unit, mxqblock_port,rval);

             /*9 Disable the egress cell request generation; this will make 
                 sure that egress does not send any request to MMU. 
                 This is done by clearing EGR_ENABLE(for EDB) /port_enable 
                 (for TXLP) register in Egress */
             /* soc_mem_field32_set(unit, EGR_ENABLEm, 
                                    &egr_enable_entry, PRT_ENABLEf, 0); */
             if (mxqblock <= 7) {
                 BCM_IF_ERROR_RETURN(soc_mem_write(
                                     unit, EGR_ENABLEm, MEM_BLOCK_ALL,
                                     mxqblock_port, &egr_enable_entry));
             } else { 
                 BCM_IF_ERROR_RETURN(WRITE_TXLP_PORT_ENABLEr(
                                     unit, mxqblock_port, 0));
             } 

             /*10. At this point, the link is down and all pending packets are 
                   drained.  The software then disables the MXQPORT flush.  
                   This is done by programming the followings: 
                   XP_TXFIFO_PKT_DROP_CTL.DROP_EN */
             READ_XP_TXFIFO_PKT_DROP_CTLr(unit, mxqblock_port,&rval);
             soc_reg_field_set(unit, XP_TXFIFO_PKT_DROP_CTLr, &rval, 
                               DROP_ENf,0);
             WRITE_XP_TXFIFO_PKT_DROP_CTLr(unit, mxqblock_port,rval);

             /* 11. Restore PFC */
             if (port_speed >= 10000) {
                 BCM_IF_ERROR_RETURN(WRITE_XPORT_TO_MMU_BKPr(
                                 unit, pfc_index, pfc_enable));
             }
             /* PLEASE NOTE THAT PORT IS STILL DOWN */
            
         }
    }
    /* ReConfigure H/W */
    /* 1. Soft-reset relevant MXQPORT(s) */
    soc_katana2_mxqblock_reset(unit, mxqblock, 0);
    if ((mxqblock==8) || (mxqblock==9)) {
         if (xfi_mode[mxqblock-8]==1) {
             soc_katana2_mxqblock_reset(unit, mxqblock-2, 0);
         }
    }

    /* ReConfigure TDM */
    soc_katana2_reconfigure_tdm(unit,kt2_current_tdm_size,new_tdm); 
    sal_memcpy(&kt2_current_tdm[0],&new_tdm[0],
               kt2_current_tdm_size *sizeof(kt2_current_tdm[0]));

    /* Update local pbmp's */
    soc_linkscan_pause(unit);
    for (loop=0;loop<KT2_MAX_MXQPORTS_PER_BLOCK;loop++) {
         mxqblock_port=kt2_mxqblock_ports[mxqblock][loop];
         if ((old_ports[old_lanes-1][loop] == 0) &&
             (new_ports[new_lanes-1][loop] == 0)) {
             /* Eithere not used  */
             continue;
         }
         si->port_speed_max[mxqblock_port]=mxqspeeds[unit][mxqblock]
                                                    [new_lanes-1];
         if (old_ports[old_lanes-1][loop] == new_ports[new_lanes-1][loop] ) {
             /* same port was used so only speed change */
             soc_katana2_pbmp_remove(unit,mxqblock_port);
             soc_katana2_pbmp_add(unit,mxqblock_port,mxqblock,
                                  si->port_speed_max[mxqblock_port]);
         }
         if (old_ports[old_lanes-1][loop]  > new_ports[new_lanes-1][loop]) {
             /* This port is not applicable so remove it from pbmp list */
             soc_katana2_pbmp_remove(unit,mxqblock_port);
             continue;
         }
         if (new_ports[new_lanes-1][loop]  > old_ports[old_lanes-1][loop]) {
             /* This port is new so add it in pbmp list first */
             soc_katana2_pbmp_add(unit,mxqblock_port, mxqblock,
                                  si->port_speed_max[mxqblock_port]);
         }
    }
    soc_linkscan_continue(unit);

    SOC_IF_ERROR_RETURN(_soc_katana2_mmu_reconfigure(unit)); 

    mxqblock_max_nxtaddr = mxqblock_max_startaddr[mxqblock];
    for (loop=0;loop<KT2_MAX_MXQPORTS_PER_BLOCK;loop++) {
         mxqblock_port=kt2_mxqblock_ports[mxqblock][loop];
         if (SOC_PBMP_MEMBER((PBMP_ALL(unit)), mxqblock_port)) {
             BCM_IF_ERROR_RETURN(READ_DEQ_EFIFO_CFGr(unit,mxqblock_port,&rval));
             if (si->port_speed_max[mxqblock_port] <= 1000) {
                 egr_fifo_depth = 10;
             }else if (si->port_speed_max[mxqblock_port] <= 2500) {
                 egr_fifo_depth = 15;
             } else if (si->port_speed_max[mxqblock_port] <= 13000) {
                 egr_fifo_depth = 60;
             } else {
                 egr_fifo_depth = 120;
             }
             soc_reg_field_set(unit, DEQ_EFIFO_CFGr, &rval,
                               EGRESS_FIFO_START_ADDRESSf,
                               mxqblock_max_nxtaddr);
             if ((mxqblock_max_nxtaddr + egr_fifo_depth) > mxqblock_max_endaddr[mxqblock]) {
                 egr_fifo_depth = (mxqblock_max_endaddr[mxqblock] -
                                   mxqblock_max_nxtaddr)+1;
                 mxqblock_max_nxtaddr = mxqblock_max_endaddr[mxqblock];
             } else {
                 mxqblock_max_nxtaddr += egr_fifo_depth;
             }
             if (egr_fifo_depth == 1) {
                 soc_cm_print("WARN: EGR_FIFO_DEPTH IS ZERO for port=%d\n",
                              mxqblock_port);
             }
             soc_reg_field_set(unit, DEQ_EFIFO_CFGr, &rval,
                               EGRESS_FIFO_DEPTHf, egr_fifo_depth);
             BCM_IF_ERROR_RETURN(WRITE_DEQ_EFIFO_CFGr(
                                 unit,mxqblock_port,rval));
         }
    }

    /* WORK-AROUND for port-flushing i.e. need to repeat below step twice */
 for (try_loop = 0; try_loop < 2; try_loop++) { 
    /* EP Reset */
    if (SAL_BOOT_SIMULATION) {
        BCM_IF_ERROR_RETURN(WRITE_DYNAMIC_PORT_RECFG_VECTOR_CFG_31_00r(
                            unit, 0));
        BCM_IF_ERROR_RETURN(WRITE_DYNAMIC_PORT_RECFG_VECTOR_CFG_41_32r(
                            unit, 0));
    }
    for (loop=0;loop<KT2_MAX_MXQPORTS_PER_BLOCK;loop++) {
         mxqblock_port=kt2_mxqblock_ports[mxqblock][loop];
         if (SOC_PBMP_MEMBER((PBMP_ALL(unit)), mxqblock_port)) {
             if (mxqblock_port <= 31) {
                 rval1 |= (1 << mxqblock_port);
             } else {
                 rval2 |= (1 << (mxqblock_port-32));
             }
         }
    }
    BCM_IF_ERROR_RETURN(WRITE_DYNAMIC_PORT_RECFG_VECTOR_CFG_31_00r(
                        unit, rval1));
    BCM_IF_ERROR_RETURN(WRITE_DYNAMIC_PORT_RECFG_VECTOR_CFG_41_32r(
                        unit, rval2));
    if (SAL_BOOT_SIMULATION) {
        BCM_IF_ERROR_RETURN(WRITE_DYNAMIC_PORT_RECFG_VECTOR_CFG_31_00r(
                            unit, 0));
        BCM_IF_ERROR_RETURN(WRITE_DYNAMIC_PORT_RECFG_VECTOR_CFG_41_32r(
                            unit, 0));
    }
    try_count = 0;
    do {
       soc_timeout_init(&to, timeout_usec, min_polls);
       if(soc_timeout_check(&to)) {
          soc_cm_debug(DK_ERR, "%s:%d:TimeOut InternalError\n",
                       __FUNCTION__,__LINE__);
          return BCM_E_INTERNAL;
       }
       try_count++;
       BCM_IF_ERROR_RETURN(READ_DYNAMIC_PORT_RECFG_VECTOR_CFG_31_00r(
                           unit, &rval1));
       BCM_IF_ERROR_RETURN(READ_DYNAMIC_PORT_RECFG_VECTOR_CFG_41_32r(
                           unit, &rval2));
       if ((rval1 == 0) && (rval2 == 0)) {
            break;
       }
    } while (try_count != 3);
    if (try_count == 3) {
        return BCM_E_TIMEOUT;
    }
 }

    if (mxqblock <= 7) {
        READ_EDATABUF_DBG_PORT_INTF_RESETr(unit, &rval);
        soc_reg_field_set(unit, EDATABUF_DBG_PORT_INTF_RESETr,
                          &rval,  port_intf_reset_fld[mxqblock], 1);
        WRITE_EDATABUF_DBG_PORT_INTF_RESETr(unit, rval);
        READ_EDATABUF_DBG_MMU_INTF_RESETr(unit, &rval);
        soc_reg_field_set(unit, EDATABUF_DBG_MMU_INTF_RESETr,
                          &rval,  mmu_intf_reset_fld[mxqblock], 1);
        soc_reg_field_set(unit, EDATABUF_DBG_MMU_INTF_RESETr,
                          &rval,  new_port_mode_fld[mxqblock], 
                          new_port_mode[new_lanes]);
        WRITE_EDATABUF_DBG_MMU_INTF_RESETr(unit, rval);
    }
    /* mxqblock_port=port; */
    if ((mxqblock >= 8) && (mxqblock <= 9)) {
        WRITE_TXLP_PORT_CREDIT_RESETr(unit, port,0xf);
        for (loop=0;loop<KT2_MAX_MXQPORTS_PER_BLOCK;loop++) {
             sal_memset(&txlp_port_addr_map_table_entry,0,
                        sizeof(txlp_port_addr_map_table_entry_t));
             mxqblock_port=kt2_mxqblock_ports[mxqblock][loop];
             if (SOC_PBMP_MEMBER((PBMP_ALL(unit)), mxqblock_port)) {
                 /*  5.  As each cell-occupies "4" lines in buffer, [end-start+1] must be a integral
                         multiple of "4". */
                 if (si->port_speed_max[mxqblock_port] <= 2500) {
                     end_addr = start_addr + (( 6 * 4) - 1); /* 6 Cells */
                 } else if (si->port_speed_max[mxqblock_port] <= 10000) {
                     end_addr = start_addr + ((12 * 4) - 1); /* 12 Cells */
                 } else if (si->port_speed_max[mxqblock_port] <= 13000) {
                     end_addr = start_addr + ((16 * 4) - 1); /* 16 Cells */
                 } else if (si->port_speed_max[mxqblock_port] <= 21000) {
                    end_addr = start_addr + ((20 * 4) - 1); /* 20 Cells */
                 }
                 soc_TXLP_PORT_ADDR_MAP_TABLEm_field_set(unit,
                     &txlp_port_addr_map_table_entry,START_ADDRf,&start_addr);
                 soc_TXLP_PORT_ADDR_MAP_TABLEm_field_set(unit,
                     &txlp_port_addr_map_table_entry,END_ADDRf,&end_addr);
                 start_addr = end_addr+1;
             }
             BCM_IF_ERROR_RETURN(WRITE_TXLP_PORT_ADDR_MAP_TABLEm(
                                 unit,SOC_INFO(unit).txlp_block[mxqblock-8],
                                 loop, &txlp_port_addr_map_table_entry));
        }
    }


    /* Begin: Link up sequence */
    sal_udelay((10*time_multiplier));
    /* 1 */
    if (mxqblock <= 7) {
        READ_EDATABUF_DBG_PORT_INTF_RESETr(unit, &rval);
        soc_reg_field_set(unit, EDATABUF_DBG_PORT_INTF_RESETr,
                          &rval,  port_intf_reset_fld[mxqblock], 0);
        WRITE_EDATABUF_DBG_PORT_INTF_RESETr(unit, rval);
        READ_EDATABUF_DBG_MMU_INTF_RESETr(unit, &rval);
        soc_reg_field_set(unit, EDATABUF_DBG_MMU_INTF_RESETr,
                          &rval,  mmu_intf_reset_fld[mxqblock], 0);
        WRITE_EDATABUF_DBG_MMU_INTF_RESETr(unit, rval);
    }
    /* mxqblock_port=port; */
    if ((mxqblock >= 8) && (mxqblock <= 9)) {
        WRITE_TXLP_PORT_CREDIT_RESETr(unit, port,0);
    }
    /* 2 */
    BCM_IF_ERROR_RETURN(READ_EPC_LINK_BMAPm(
                        unit, MEM_BLOCK_ANY, 0, &epc_link_bmap_entry));
    soc_mem_pbmp_field_get(unit, EPC_LINK_BMAPm,
                        &epc_link_bmap_entry, PORT_BITMAPf,&link_pbmp);
    for (loop=0;loop<KT2_MAX_MXQPORTS_PER_BLOCK;loop++) {
         mxqblock_port=kt2_mxqblock_ports[mxqblock][loop];
         if (SOC_PBMP_MEMBER((PBMP_ALL(unit)), mxqblock_port)) {
             BCM_PBMP_PORT_ADD(link_pbmp,mxqblock_port);
         }
    } 
    soc_mem_pbmp_field_set(unit, EPC_LINK_BMAPm,
                        &epc_link_bmap_entry, PORT_BITMAPf,&link_pbmp);
    SOC_IF_ERROR_RETURN(WRITE_EPC_LINK_BMAPm(
                        unit, MEM_BLOCK_ALL, 0, &epc_link_bmap_entry));

    /* Bring out of reset */
    soc_katana2_mxqblock_reset(unit, mxqblock, 1);

    /* XFI Mode ??? */
    for (loop=0;loop<KT2_MAX_MXQPORTS_PER_BLOCK;loop++) {
         mxqblock_port=kt2_mxqblock_ports[mxqblock][loop];
         if (SOC_PBMP_MEMBER((PBMP_ALL(unit)), mxqblock_port)) {
             BCM_IF_ERROR_RETURN(_bcm_kt2_update_port_mode(
                                 unit,mxqblock_port,
                                 si->port_speed_max[mxqblock_port]));
             break;
         }
    }
    for (loop=0;loop<KT2_MAX_MXQPORTS_PER_BLOCK;loop++) {
         mxqblock_port=kt2_mxqblock_ports[mxqblock][loop];
         if (SOC_PBMP_MEMBER((PBMP_ALL(unit)), mxqblock_port)) {
             SOC_IF_ERROR_RETURN(READ_XPORT_CONFIGr(unit, mxqblock_port,&rval));
             if ((si->port_speed_max[mxqblock_port]) >= 13000) {
                  soc_reg_field_set(unit, XPORT_CONFIGr, &rval, HIGIG_MODEf,1);
                  if ((soc_feature(unit, soc_feature_higig2)) &&
                      (soc_property_port_get(unit, mxqblock_port, 
                                            spn_HIGIG2_HDR_MODE, 0))) {
                      soc_reg_field_set(unit, XPORT_CONFIGr, &rval, 
                                        HIGIG2_MODEf,1);
                  } else {
                      soc_reg_field_set(unit, XPORT_CONFIGr, &rval, 
                                        HIGIG2_MODEf,0);
                  } 
             } else {
                  soc_reg_field_set(unit, XPORT_CONFIGr, &rval, HIGIG_MODEf,0);
                  soc_reg_field_set(unit, XPORT_CONFIGr, &rval, HIGIG2_MODEf,0);
             }
             SOC_IF_ERROR_RETURN(WRITE_XPORT_CONFIGr(unit, mxqblock_port,rval));
         } 
    }
    /* mxqblock_port=port; */
    SOC_IF_ERROR_RETURN(WRITE_XPORT_XMAC_CONTROLr(unit, port, 0));

    for (loop=0;loop<KT2_MAX_MXQPORTS_PER_BLOCK;loop++) {
         mxqblock_port=kt2_mxqblock_ports[mxqblock][loop];
         if (SOC_PBMP_MEMBER((PBMP_ALL(unit)), mxqblock_port)) {
             SOC_IF_ERROR_RETURN(READ_MAC_RSV_MASKr(unit,mxqblock_port, &rval));
             if ((si->port_speed_max[mxqblock_port]) >= 13000) {
                  soc_reg_field_set(unit, MAC_RSV_MASKr, &rval, MASKf, 0x58);
             } else {
                  soc_reg_field_set(unit, MAC_RSV_MASKr, &rval, MASKf, 0x78);
             }
             SOC_IF_ERROR_RETURN(WRITE_MAC_RSV_MASKr(unit,mxqblock_port, rval));
         }
    }

    /* mxqblock_port=port; */
    SOC_IF_ERROR_RETURN(WRITE_XPORT_MIB_RESETr(unit,port, 0xf));
    SOC_IF_ERROR_RETURN(WRITE_XPORT_MIB_RESETr(unit,port, 0x0));

    for (loop=0;loop<KT2_MAX_MXQPORTS_PER_BLOCK;loop++) {
         mxqblock_port=kt2_mxqblock_ports[mxqblock][loop];
         if (SOC_PBMP_MEMBER((PBMP_ALL(unit)), mxqblock_port)) {
             SOC_IF_ERROR_RETURN(READ_XMAC_MODEr(unit,mxqblock_port, &rval64));
             if (si->port_speed_max[mxqblock_port] ==  1000) {
                 speed_value = 2;
             } else if (si->port_speed_max[mxqblock_port] == 2500) {
                 speed_value = 3;
             } else {
                 speed_value = 4;
             }
             soc_reg64_field32_set(unit, XMAC_MODEr, &rval64,
                                   SPEED_MODEf,speed_value);
             if (si->port_speed_max[mxqblock_port] <=  10000) {
                 soc_reg64_field32_set(unit, XMAC_MODEr, &rval64,HDR_MODEf,0);
             } else {
                 soc_reg64_field32_set(unit, XMAC_MODEr, &rval64,HDR_MODEf,2);
             }
             SOC_IF_ERROR_RETURN(WRITE_XMAC_MODEr(unit,mxqblock_port, rval64));
         }
    }

    for (loop=0;loop<KT2_MAX_MXQPORTS_PER_BLOCK;loop++) {
         mxqblock_port=kt2_mxqblock_ports[mxqblock][loop];
         if (SOC_PBMP_MEMBER((PBMP_ALL(unit)), mxqblock_port)) {
             SOC_IF_ERROR_RETURN(READ_XMAC_RX_CTRLr(
                                 unit,mxqblock_port, &rval64));
             if (si->port_speed_max[mxqblock_port] <=  10000) {
                 soc_reg64_field32_set(unit, XMAC_RX_CTRLr, &rval64,
                                       RUNT_THRESHOLDf, 64);
             } else {
                 soc_reg64_field32_set(unit, XMAC_RX_CTRLr, &rval64,
                                       RUNT_THRESHOLDf, 76);
             }
             soc_reg64_field32_set(unit, XMAC_RX_CTRLr, &rval64, STRIP_CRCf, 0);
             SOC_IF_ERROR_RETURN(WRITE_XMAC_RX_CTRLr(
                                 unit,mxqblock_port, rval64));
         }
    }
    for (loop=0;loop<KT2_MAX_MXQPORTS_PER_BLOCK;loop++) {
         mxqblock_port=kt2_mxqblock_ports[mxqblock][loop];
         if (SOC_PBMP_MEMBER((PBMP_ALL(unit)), mxqblock_port)) {
             SOC_IF_ERROR_RETURN(READ_XMAC_TX_CTRLr(
                                 unit,mxqblock_port, &rval64));
             if (si->port_speed_max[mxqblock_port] <  2500) {
                 soc_reg64_field32_set(unit, XMAC_TX_CTRLr, &rval64,
                                       TX_64BYTE_BUFFER_ENf,1);
             } else {
                 soc_reg64_field32_set(unit, XMAC_TX_CTRLr, &rval64,
                                       TX_64BYTE_BUFFER_ENf,0);
             }
             soc_reg64_field32_set(unit, XMAC_TX_CTRLr, &rval64, CRC_MODEf,3);
             SOC_IF_ERROR_RETURN(WRITE_XMAC_TX_CTRLr(
                                 unit,mxqblock_port, rval64));
         }
    }
    for (loop=0;loop<KT2_MAX_MXQPORTS_PER_BLOCK;loop++) {
         mxqblock_port=kt2_mxqblock_ports[mxqblock][loop];
         if (SOC_PBMP_MEMBER((PBMP_ALL(unit)), mxqblock_port)) {
             SOC_IF_ERROR_RETURN(READ_XMAC_RX_MAX_SIZEr(
                                 unit,mxqblock_port, &rval64));
             soc_reg64_field32_set(unit, XMAC_RX_MAX_SIZEr, &rval64,
                          RX_MAX_SIZEf,(12*1024));
             SOC_IF_ERROR_RETURN(WRITE_XMAC_RX_MAX_SIZEr(
                                 unit,mxqblock_port, rval64));
         }
    }
    for (loop=0;loop<KT2_MAX_MXQPORTS_PER_BLOCK;loop++) {
         mxqblock_port=kt2_mxqblock_ports[mxqblock][loop];
         if (SOC_PBMP_MEMBER((PBMP_ALL(unit)), mxqblock_port)) {
             SOC_IF_ERROR_RETURN(READ_XMAC_CTRLr(
                                 unit,mxqblock_port, &rval64));
             soc_reg64_field32_set(unit, XMAC_CTRLr, &rval64, SOFT_RESETf,0);
             soc_reg64_field32_set(unit, XMAC_CTRLr, &rval64, TX_ENf,1);
             soc_reg64_field32_set(unit, XMAC_CTRLr, &rval64, RX_ENf,1);
             SOC_IF_ERROR_RETURN(WRITE_XMAC_CTRLr(
                                 unit,mxqblock_port, rval64));
         }
    }

    /* mxqblock_port=port; */
    SOC_IF_ERROR_RETURN(WRITE_XPORT_ECC_CONTROLr(unit,port, 0xf));

    SOC_IF_ERROR_RETURN(READ_XPORT_PORT_ENABLEr(unit,port, &rval));
    for (loop=0;loop<KT2_MAX_MXQPORTS_PER_BLOCK;loop++) {
         mxqblock_port=kt2_mxqblock_ports[mxqblock][loop];
         if (SOC_PBMP_MEMBER((PBMP_ALL(unit)), mxqblock_port)) {
             soc_reg_field_set(unit, XPORT_PORT_ENABLEr, &rval, 
             port_enable_field[kt2_port_to_mxqblock_subports
                               [mxqblock_port-1]],1);
         }
    }
    /* mxqblock_port=port; */
    SOC_IF_ERROR_RETURN(WRITE_XPORT_PORT_ENABLEr(unit,port, rval));

    for (loop=0;loop<KT2_MAX_MXQPORTS_PER_BLOCK;loop++) {
         mxqblock_port=kt2_mxqblock_ports[mxqblock][loop];
         if (SOC_PBMP_MEMBER((PBMP_ALL(unit)), mxqblock_port)) {
             SOC_IF_ERROR_RETURN(READ_THDO_PORT_DISABLE_CFG1r(unit,&rval));
             soc_reg_field_set(unit, THDO_PORT_DISABLE_CFG1r, &rval, 
                               PORT_WRf,1);
             soc_reg_field_set(unit, THDO_PORT_DISABLE_CFG1r, &rval, 
                               PORT_WR_TYPEf,0);
             soc_reg_field_set(unit, THDO_PORT_DISABLE_CFG1r, &rval, 
                               PORT_IDf,mxqblock_port);
             SOC_IF_ERROR_RETURN(WRITE_THDO_PORT_DISABLE_CFG1r(unit,rval));
             sal_udelay(10*time_multiplier);
             if ((SAL_BOOT_BCMSIM || SAL_BOOT_PLISIM)) {
                  soc_reg_field_set(unit, THDO_PORT_DISABLE_CFG1r, &rval, 
                               PORT_WRf,0);
                  SOC_IF_ERROR_RETURN(WRITE_THDO_PORT_DISABLE_CFG1r(
                                      unit,rval));
             }
             SOC_IF_ERROR_RETURN(READ_THDO_PORT_DISABLE_CFG1r(unit,&rval));
             if (soc_reg_field_get(unit, THDO_PORT_DISABLE_CFG1r, rval,
                                    PORT_WRf) != 0) {
                 return BCM_E_TIMEOUT;
             }
         }
    }
    /* mxqblock_port=port; */
    SOC_IF_ERROR_RETURN(READ_XPORT_XGXS_CTRLr(unit,port,&rval));
    soc_reg_field_set(unit,XPORT_XGXS_CTRLr,&rval,PWRDWNf,0);
    SOC_IF_ERROR_RETURN(WRITE_XPORT_XGXS_CTRLr(unit,port,rval));

    SOC_IF_ERROR_RETURN(READ_XPORT_XGXS_CTRLr(unit,port,&rval));
    soc_reg_field_set(unit,XPORT_XGXS_CTRLr,&rval,IDDQf,0);
    SOC_IF_ERROR_RETURN(WRITE_XPORT_XGXS_CTRLr(unit,port,rval));

    SOC_IF_ERROR_RETURN(READ_XPORT_XGXS_CTRLr(unit,port,&rval));
    soc_reg_field_set(unit,XPORT_XGXS_CTRLr,&rval,REFSELf,0);
    SOC_IF_ERROR_RETURN(WRITE_XPORT_XGXS_CTRLr(unit,port,rval));

    SOC_IF_ERROR_RETURN(READ_XPORT_XGXS_CTRLr(unit,port,&rval));
    soc_reg_field_set(unit,XPORT_XGXS_CTRLr,&rval,REFDIVf,0);
    SOC_IF_ERROR_RETURN(WRITE_XPORT_XGXS_CTRLr(unit,port,rval));

    SOC_IF_ERROR_RETURN(READ_XPORT_XGXS_CTRLr(unit,port,&rval));
    soc_reg_field_set(unit,XPORT_XGXS_CTRLr,&rval,RSTB_HWf,1);
    SOC_IF_ERROR_RETURN(WRITE_XPORT_XGXS_CTRLr(unit,port,rval));

    SOC_IF_ERROR_RETURN(READ_XPORT_XGXS_CTRLr(unit,port,&rval));
    soc_reg_field_set(unit,XPORT_XGXS_CTRLr,&rval,RSTB_MDIOREGSf,1);
    SOC_IF_ERROR_RETURN(WRITE_XPORT_XGXS_CTRLr(unit,port,rval));

    SOC_IF_ERROR_RETURN(READ_XPORT_XGXS_CTRLr(unit,port,&rval));
    soc_reg_field_set(unit,XPORT_XGXS_CTRLr,&rval,RSTB_PLLf,1);
    SOC_IF_ERROR_RETURN(WRITE_XPORT_XGXS_CTRLr(unit,port,rval));

    for (loop=0;loop<KT2_MAX_MXQPORTS_PER_BLOCK;loop++) {
         mxqblock_port=kt2_mxqblock_ports[mxqblock][loop];
         if (SOC_PBMP_MEMBER((PBMP_ALL(unit)), mxqblock_port)) {
             SOC_IF_ERROR_RETURN(READ_XPORT_XGXS_CTRLr(
                                 unit,mxqblock_port,&rval));
             if (si->port_speed_max[mxqblock_port] > 2500) {
                 soc_reg_field_set(unit,XPORT_XGXS_CTRLr,&rval,
                                   TXD10G_FIFO_RSTBf,1);
             } else {
                 soc_reg_field_set(unit,XPORT_XGXS_CTRLr,&rval,
                                   TXD10G_FIFO_RSTBf,0);
             }
             SOC_IF_ERROR_RETURN(WRITE_XPORT_XGXS_CTRLr(
                                 unit,mxqblock_port,rval));
         }
    }
    for (loop=0;loop<KT2_MAX_MXQPORTS_PER_BLOCK;loop++) {
         mxqblock_port=kt2_mxqblock_ports[mxqblock][loop];
         if (SOC_PBMP_MEMBER((PBMP_ALL(unit)), mxqblock_port)) {
             SOC_IF_ERROR_RETURN(READ_XPORT_XGXS_CTRLr(
                                 unit,mxqblock_port,&rval));
             if (si->port_speed_max[mxqblock_port] <= 2500) {
                 soc_reg_field_set(unit,XPORT_XGXS_CTRLr,&rval,
                                   TXD1G_FIFO_RSTBf,0xF);
             } else {
                 soc_reg_field_set(unit,XPORT_XGXS_CTRLr,&rval,
                                   TXD1G_FIFO_RSTBf,0);
             }
             SOC_IF_ERROR_RETURN(WRITE_XPORT_XGXS_CTRLr(
                                 unit,mxqblock_port,rval));
         }
    }
    /* Txd10g_FIFO_RstB_DXGXS1  ?? */

    for (loop=0;loop<KT2_MAX_MXQPORTS_PER_BLOCK;loop++) {
         mxqblock_port=kt2_mxqblock_ports[mxqblock][loop];
         if (SOC_PBMP_MEMBER((PBMP_ALL(unit)), mxqblock_port)) {
             if (mxqblock <= 7 ) {
                 sal_memset(&egr_enable_entry, 0, sizeof(egr_enable_entry_t));
                 soc_mem_field32_set(unit, EGR_ENABLEm,
                                     &egr_enable_entry,PRT_ENABLEf, 1);
                 SOC_IF_ERROR_RETURN(WRITE_EGR_ENABLEm(unit, MEM_BLOCK_ALL, 
                                     mxqblock_port, &egr_enable_entry));
             } else {
                 SOC_IF_ERROR_RETURN(READ_TXLP_PORT_ENABLEr(
                                     unit, mxqblock_port, &rval));
                 port_enable = soc_reg_field_get(unit, TXLP_PORT_ENABLEr,
                                     rval, PORT_ENABLEf);
                 port_enable |= (1<< kt2_port_to_mxqblock_subports
                                     [mxqblock_port-1]);
                 SOC_IF_ERROR_RETURN(WRITE_TXLP_PORT_ENABLEr(
                                     unit, mxqblock_port, rval));
             }
         }
    }
    if ((mxqblock == 8) || (mxqblock == 9)) {
         for (loop=0;loop<KT2_MAX_MXQPORTS_PER_BLOCK;loop++) {
              mxqblock_port=kt2_mxqblock_ports[mxqblock][loop];
              sal_sprintf(config_str,"portgroup_%d",mxqblock_port);
              soc_mem_config_set(config_str,NULL);
         }
         switch(new_lanes) {
         case 1:
               sal_sprintf(config_str,"portgroup_%d",port);
               soc_mem_config_set(config_str,"4");
               break;
         case 2:
               for (loop=0;loop<KT2_MAX_MXQPORTS_PER_BLOCK;loop+=2) {
                    mxqblock_port=kt2_mxqblock_ports[mxqblock][loop];
                    sal_sprintf(config_str,"portgroup_%d",mxqblock_port);
                    soc_mem_config_set(config_str,"2");
               }
               break;
         case 4:
               for (loop=0;loop<KT2_MAX_MXQPORTS_PER_BLOCK;loop++) {
                    mxqblock_port=kt2_mxqblock_ports[mxqblock][loop];
                    sal_sprintf(config_str,"portgroup_%d",mxqblock_port);
                    soc_mem_config_set(config_str,"1");
               }
               break;
         }
    }

    /* End: Link up sequence */

    for (loop=0;loop<KT2_MAX_MXQPORTS_PER_BLOCK;loop++) {
         mxqblock_port=kt2_mxqblock_ports[mxqblock][loop];
         if ((old_ports[old_lanes-1][loop] == 0) &&
             (new_ports[new_lanes-1][loop] == 0)) {
             /* Eithere not used  */
             continue;
         }
         if (old_ports[old_lanes-1][loop] == new_ports[new_lanes-1][loop] ) {
             /* same port was used so only speed change */
             SOC_IF_ERROR_RETURN(soc_katana2_port_detach(unit,mxqblock_port));
         }
         if (old_ports[old_lanes-1][loop]  > new_ports[new_lanes-1][loop]) {
             /* This port is not applicable so remove it from pbmp list */
             SOC_IF_ERROR_RETURN(soc_katana2_port_detach(unit,mxqblock_port));
             continue;
         }
         if (new_ports[new_lanes-1][loop]  > old_ports[old_lanes-1][loop]) {
             /* This port is new continue */
             continue;
         }
    }
    for (loop=0;loop<KT2_MAX_MXQPORTS_PER_BLOCK;loop++) {
         mxqblock_port=kt2_mxqblock_ports[mxqblock][loop];
         if ((old_ports[old_lanes-1][loop] == 0) &&
             (new_ports[new_lanes-1][loop] == 0)) {
             /* Eithere not used  */
             continue;
         }
         if (old_ports[old_lanes-1][loop] == new_ports[new_lanes-1][loop] ) {
             /* same port was used so only speed change */
             SOC_IF_ERROR_RETURN(soc_katana2_port_attach(
                                 unit,mxqblock ,mxqblock_port,
                                 mxqspeeds[unit][mxqblock][new_lanes-1]));
         }
         if (old_ports[old_lanes-1][loop]  > new_ports[new_lanes-1][loop]) {
             /* This port is not applicable so remove it from pbmp list */
             continue;
         }
         if (new_ports[new_lanes-1][loop]  > old_ports[old_lanes-1][loop]) {
             /* This port is new so add it in pbmp list first */
             SOC_IF_ERROR_RETURN(soc_katana2_port_attach(
                                 unit,mxqblock ,mxqblock_port,
                                 mxqspeeds[unit][mxqblock][new_lanes-1]));
         }
    }
    /* Special Treatment */
/*
    if ((mxqblock==8) || (mxqblock==9)) {
        if (xfi_mode[mxqblock-8]==1) {
            soc_cm_print("4xXFI Mode used  for mxqblock:%d\n",mxqblock);
        }
        SOC_IF_ERROR_RETURN(READ_TOP_MISC_CONTROL_1r(
                            unit,&top_misc_control_1_val));
        soc_reg_field_set(unit, TOP_MISC_CONTROL_1r, 
                          &top_misc_control_1_val,
                          wc_xfi_mode_sel_fld[mxqblock-8],xfi_mode[mxqblock-8]);
        SOC_IF_ERROR_RETURN(WRITE_TOP_MISC_CONTROL_1r(
                                unit,top_misc_control_1_val));
    }
*/

    /* 1. Bring-out of Soft-reset XFI mode related MXQPORT(s) */
/*
    if ((mxqblock==8) || (mxqblock==9)) {
         if (xfi_mode[mxqblock-8]==1) {
             soc_timeout_init(&to, timeout_usec, min_polls);
             if(soc_timeout_check(&to)) {
                soc_cm_debug(DK_ERR, 
                             "_bcm_kt2_port_lanes_set:TimeOut InternalError\n");
                return SOC_E_INTERNAL; 
             }
             soc_katana2_mxqblock_reset(unit, mxqblock-2, 1);
         }
    }
*/
    return SOC_E_NONE;

}
int
_bcm_kt2_port_lanes_get(int unit, bcm_port_t port, int *lanes)
{
    uint8 loop=0;
    uint8 mxqblock=0;

    *lanes=0;
    SOC_IF_ERROR_RETURN(soc_katana2_get_port_mxqblock(
                        unit,port,&mxqblock));
    for(loop=0;loop<KT2_MAX_MXQPORTS_PER_BLOCK; loop++) {
        if (SOC_PBMP_MEMBER((PBMP_ALL(unit)),
                             kt2_mxqblock_ports[mxqblock][loop])) {
           (*lanes)++;
        }
    }
    return SOC_E_NONE;
}

int 
_bcm_kt2_port_control_oam_loopkup_with_dvp_set(int unit, bcm_port_t port, 
                                               int val)
{ 
#if defined(INCLUDE_L3)
    int vp = 0;
    egr_dvp_attribute_entry_t dvp_entry;
    if (BCM_GPORT_IS_MIM_PORT(port)) {
        /* From the gport get VP index */
        if (_bcm_vp_used_get(unit, port, _bcmVpTypeMim)) {
            vp = BCM_GPORT_MIM_PORT_ID_GET(port);
        }
        /* Set OAM_KEY3 in EGR_DVP_ATTRIBUTE table entry */            
        SOC_IF_ERROR_RETURN
             (READ_EGR_DVP_ATTRIBUTEm(unit, MEM_BLOCK_ANY, vp, &dvp_entry));
        soc_EGR_DVP_ATTRIBUTEm_field32_set(unit, &dvp_entry,
                                           OAM_KEY3f, val ? 1 : 0);
        SOC_IF_ERROR_RETURN
            (WRITE_EGR_DVP_ATTRIBUTEm(unit, MEM_BLOCK_ALL, vp, &dvp_entry));
        return BCM_E_NONE;
    } else {
        return BCM_E_PARAM;
    }
#endif
    return BCM_E_NONE;
}

int 
_bcm_kt2_port_control_oam_loopkup_with_dvp_get(int unit, bcm_port_t port, 
                                               int *val)
{ 
#if defined(INCLUDE_L3)
    int vp = 0;
    egr_dvp_attribute_entry_t dvp_entry;
    if (BCM_GPORT_IS_MIM_PORT(port) || BCM_GPORT_IS_MPLS_PORT(port)) {
    /* From the gport get VP index */
        if (_bcm_vp_used_get(unit, port, _bcmVpTypeMim)) {
            vp = BCM_GPORT_MIM_PORT_ID_GET(port);
        } else if (_bcm_vp_used_get(unit, port, _bcmVpTypeMpls)) {
            vp = BCM_GPORT_MPLS_PORT_ID_GET(port);
        }
        SOC_IF_ERROR_RETURN
                 (READ_EGR_DVP_ATTRIBUTEm(unit, MEM_BLOCK_ANY, vp, &dvp_entry));
        *val = soc_EGR_DVP_ATTRIBUTEm_field32_get(unit, &dvp_entry,
                                                       OAM_KEY3f);
    }
#endif
    return BCM_E_NONE;
}

soc_field_t 
    modid_valid_field[KT2_MAX_MODIDS_PER_TRANSLATION_TABLE] =
    { MODID_0_VALIDf,
        MODID_1_VALIDf,
        MODID_2_VALIDf,
        MODID_3_VALIDf};
soc_field_t 
    modid_field[KT2_MAX_MODIDS_PER_TRANSLATION_TABLE] =
    { MODID_0f,
        MODID_1f,
        MODID_2f,
        MODID_3f };
soc_field_t 
    modid_base_port_ptr_field[KT2_MAX_MODIDS_PER_TRANSLATION_TABLE] =
    {   MODID_0_BASE_PP_PORTf ,
        MODID_1_BASE_PP_PORTf ,
        MODID_2_BASE_PP_PORTf ,
        MODID_3_BASE_PP_PORTf } ;
soc_mem_t 
    pp_port_gpp_translation_table[KT2_MAX_PP_PORT_GPP_TRANSLATION_TABLES + 
    KT2_MAX_EGR_PP_PORT_GPP_TRANSLATION_TABLES] = 
    {   PP_PORT_GPP_TRANSLATION_1m,
        PP_PORT_GPP_TRANSLATION_2m,
        PP_PORT_GPP_TRANSLATION_3m,
        PP_PORT_GPP_TRANSLATION_4m,
        EGR_PP_PORT_GPP_TRANSLATION_1m,
        EGR_PP_PORT_GPP_TRANSLATION_2m};

/*
 * Function:
 *      bcm_kt2_modid_set
 * Purpose:
 *      Update the moduleid , valid bit and corresponding port base pointer
 *      In the appropriates tables as per Katana2
 * Parameters:
 *      unit               - (IN)StrataSwitch unit number.
 *      my_mod_list        - (IN) list of module ID's
 *      my_mod_valid_list  - (IN) list of Valid bits corresponding to Module ID
 *      base_ptr_list      - (IN) list of port base ptr corresponding to
 *                                 ModuleID
 * Returns:
 *      BCM_E_XXX
 */

int 
bcm_kt2_modid_set(int unit,
        int *my_modid_list, int *my_modid_valid_list ,
        int *base_port_ptr_list) 
{
    int loop = 0;
    int index = 0;

    for (index = 0 ; index < ( KT2_MAX_PP_PORT_GPP_TRANSLATION_TABLES
                + KT2_MAX_EGR_PP_PORT_GPP_TRANSLATION_TABLES ) ; index++) { 
        for (loop = 0 ; loop < KT2_MAX_MODIDS_PER_TRANSLATION_TABLE; loop++) { 
            BCM_IF_ERROR_RETURN(soc_mem_field32_modify(unit,
                        pp_port_gpp_translation_table[index], 
                        0, modid_valid_field[loop],
                        my_modid_valid_list[loop]));
            BCM_IF_ERROR_RETURN(soc_mem_field32_modify(unit,
                        pp_port_gpp_translation_table[index],
                        0, modid_field[loop],
                        my_modid_list[loop]));
            BCM_IF_ERROR_RETURN(soc_mem_field32_modify(unit,
                        pp_port_gpp_translation_table[index], 0, 
                        modid_base_port_ptr_field[loop],
                        base_port_ptr_list[loop]));

        }
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_kt2_modid_get
 * Purpose:
 *      Get the moduleids , valid bit and corresponding port base pointer
 * Parameters:
 *      unit               - (IN) unit number.
 *      my_mod_list        - (OUT) list of module ID's
 *      my_mod_valid_list  - (OUT) list of Valid bits corresponding to Module ID
 *      base_ptr_list      - (OUT) list of port base ptr corresponding to
 *                                 ModuleID
 * Returns:
 *      BCM_E_XXX
 */

int 
bcm_kt2_modid_get(int unit,
        int *my_modid_list, int *my_modid_valid_list ,
        int *base_port_ptr_list) 
{
    int loop = 0;
    soc_mem_t mem = pp_port_gpp_translation_table[0];
    uint32 hw_buf[SOC_MAX_MEM_FIELD_WORDS]; /* hw entry  buffer.            */

    sal_memset(hw_buf, 0, SOC_MAX_MEM_FIELD_WORDS * sizeof(uint32));

    BCM_IF_ERROR_RETURN(soc_mem_read(unit, mem, MEM_BLOCK_ANY, 0, hw_buf));

    for (loop = 0 ; loop < KT2_MAX_MODIDS_PER_TRANSLATION_TABLE; loop++) { 
        my_modid_valid_list[loop] = soc_mem_field32_get(unit, mem, hw_buf,
                                    modid_valid_field[loop]);
        my_modid_list[loop] = soc_mem_field32_get(unit, mem, hw_buf,
                                    modid_field[loop]);
        base_port_ptr_list[loop] = soc_mem_field32_get(unit, mem, hw_buf,
                                    modid_base_port_ptr_field[loop]);
    }
    return BCM_E_NONE;
}


/*
 * Function:
 *      _bcm_kt2_port_gport_validate
 * Description:
 *      Helper funtion to validate port/gport parameter 
 * Parameters:
 *      unit  - (IN) BCM device number
 *      port_in  - (IN) Port / Gport to validate
 *      port_out - (OUT) Port number if valid. 
 * Return Value:
 *      BCM_E_NONE - Port OK 
 *      BCM_E_PORT - Port Invalid
 */
int
_bcm_kt2_port_gport_validate(int unit, bcm_port_t port_in,
                                       bcm_port_t *port_out)
{
    if (BCM_GPORT_IS_SET(port_in)) {
        if (BCM_GPORT_IS_SUBPORT_PORT(port_in)) {
            if (_BCM_KT2_GPORT_IS_SUBTAG_SUBPORT_PORT(unit, port_in) || 
             _BCM_KT2_GPORT_IS_LINKPHY_SUBPORT_PORT(unit, port_in)) {
                *port_out = _BCM_KT2_SUBPORT_PORT_ID_GET(port_in);
            } else {
                return BCM_E_PORT;
            }
        } else {
            BCM_IF_ERROR_RETURN(
                bcm_esw_port_local_get(unit, port_in, port_out));
        }
    } else if (SOC_PORT_VALID(unit, port_in)) { 
        *port_out = port_in;
    } else {
        return BCM_E_PORT; 
    }

    return BCM_E_NONE;

}

/*
 * Function:
 *      _bcm_kt2_pp_port_to_modport_get
 * Description:
 *      given a PP_PORT return the modid and port
 * Parameters:
 *      unit  - (IN) BCM device number
 *      pp_port  - (IN) pp_port / Gport
 *      modid    - (OUT) Module ID
 *      port     - (OUT) Port number if valid. 
 * Return Value:
 *      BCM_E_XXX
 */
int
_bcm_kt2_pp_port_to_modport_get(int unit, bcm_port_t pp_port, int *modid,
                              bcm_port_t *port)
{
    int my_modid_list[KT2_MAX_MODIDS_PER_TRANSLATION_TABLE] = {0};
    int my_modid_valid[KT2_MAX_MODIDS_PER_TRANSLATION_TABLE] = {0};
    int my_modport_base_ptr[KT2_MAX_MODIDS_PER_TRANSLATION_TABLE] = {0};

    *modid = -1;
    *port = -1;

    BCM_IF_ERROR_RETURN(_bcm_kt2_port_gport_validate(unit, pp_port, &pp_port));

    BCM_IF_ERROR_RETURN(
        bcm_kt2_modid_get(unit, my_modid_list, my_modid_valid, 
                             my_modport_base_ptr));

    if (my_modid_valid[3] && (pp_port >= my_modport_base_ptr[3])) {
        *modid = my_modid_list[3];
        *port = pp_port - my_modport_base_ptr[3];
    } else if (my_modid_valid[2] && (pp_port >= my_modport_base_ptr[2])) {
        *modid = my_modid_list[2];
        *port = pp_port - my_modport_base_ptr[2];
    } else if (my_modid_valid[1] && (pp_port >= my_modport_base_ptr[1])) {
        *modid = my_modid_list[1];
        *port = pp_port - my_modport_base_ptr[1];
    } else {
        *modid = my_modid_list[0];
        *port = pp_port - my_modport_base_ptr[0];
    }
    return BCM_E_NONE;

}

/*
 * Function:
 *      _bcm_kt2_modport_to_pp_port_get
 * Description:
 *      given a modid and port return the pp_port number
 * Parameters:
 *      unit  - (IN) BCM device number
 *      modid    - (IN) Module ID
 *      port     - (IN) Port number if valid. 
 *      pp_port  - (OUT) pp_port / Gport
 * Return Value:
 *      BCM_E_XXX
 */
int
_bcm_kt2_modport_to_pp_port_get(int unit, int modid, bcm_port_t port,
                            bcm_port_t *pp_port)
{
    int my_modid_list[KT2_MAX_MODIDS_PER_TRANSLATION_TABLE] = {0};
    int my_modid_valid[KT2_MAX_MODIDS_PER_TRANSLATION_TABLE] = {0};
    int my_modport_base_ptr[KT2_MAX_MODIDS_PER_TRANSLATION_TABLE] = {0};

    *pp_port = -1;

    if ((modid > SOC_INFO(unit).modid_max) ||
        (port > SOC_INFO(unit).port_addr_max)) {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN(
        bcm_kt2_modid_get(unit, my_modid_list, my_modid_valid, 
                             my_modport_base_ptr));

    if (my_modid_valid[0] && (modid == my_modid_list[0])) {
        *pp_port = my_modport_base_ptr[0] + port;
    } else if (my_modid_valid[1] && (modid == my_modid_list[1])) {
        *pp_port = my_modport_base_ptr[1] + port;
    } else if(my_modid_valid[2] && (modid == my_modid_list[2])) {
        *pp_port = my_modport_base_ptr[2] + port;
    } else {
        *pp_port = my_modport_base_ptr[3] + port;
    }
    return BCM_E_NONE;

}

/*
 * Function:
 *      _bcm_kt2_subport_pbmp_update
 * Description:
 *      given a pbmp, add linkphy/subtag pp_port pbmp and
 *      remove linkphy/subtag port pbmp
 * Parameters:
 *      unit  - (IN) BCM device number
 *      pbmp    - (IN/OUT) port bit map
 */

void
_bcm_kt2_subport_pbmp_update(int unit, bcm_pbmp_t *pbmp)
{
    if (SOC_INFO(unit).linkphy_enabled) {
        BCM_PBMP_OR(*pbmp, SOC_INFO(unit).linkphy_pp_port_pbm);
    }
    if (SOC_INFO(unit).subtag_enabled) {
        BCM_PBMP_OR(*pbmp, SOC_INFO(unit).subtag_pp_port_pbm);
    }
    return;
}

/*
 * Function:
 *      _bcm_kt2_flexio_pbmp_update
 * Description:
 *      given a pbmp, add flexio pbmp
 * Parameters:
 *      unit  - (IN) BCM device number
 *      pbmp    - (IN/OUT) port bit map
 */

int
_bcm_kt2_flexio_pbmp_update(int unit, bcm_pbmp_t *pbmp)
{
    bcm_port_t first_port, mxqblk_port;
    int        mxqblk, blk_idx;

    if (NULL == pbmp) {
        return BCM_E_PORT;
    }
    for (mxqblk = 0; mxqblk < (KT2_MAX_MXQBLOCKS - 1); mxqblk++) {
        /* Get first port in each MXQ block */
        first_port = kt2_mxqblock_ports[mxqblk][0];
        if (SOC_PORT_TYPE(unit, first_port) == SOC_BLK_MXQPORT) {
            /* Add the mxqport at index 1, 2, & 3 to gven pbmp and update the
            * port_type so that it is treated as valid port*/
            for (blk_idx = 1; blk_idx < KT2_MAX_MXQPORTS_PER_BLOCK; blk_idx++) {
                mxqblk_port = kt2_mxqblock_ports[mxqblk][blk_idx];
                if (mxqblk_port < SOC_INFO(unit).lb_port) {
                    BCM_PBMP_PORT_ADD(*pbmp, mxqblk_port);
                    SOC_PORT_TYPE(unit, mxqblk_port) = SOC_BLK_MXQPORT;
                }
            }
        }
    }
    return BCM_E_NONE;
}


#endif /* BCM_KATANA2_SUPPORT */
