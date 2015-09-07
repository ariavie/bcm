/*
 * $Id: ab086516d5707e10cd6fbe5a6c9affdb724a92bf $
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

#include <soc/defs.h>

#include <assert.h>

#include <sal/core/libc.h>
#if defined(BCM_GREYHOUND_SUPPORT)
#include <shared/util.h>
#include <soc/mem.h>
#include <soc/cm.h>
#include <soc/drv.h>
#include <soc/register.h>
#include <soc/memory.h>
#include <soc/hash.h>
#include <bcm_int/esw/port.h>
#include <bcm_int/esw/stack.h>
#include <bcm_int/esw/greyhound.h>
#include <bcm_int/esw_dispatch.h>

/*
 * Function:
 *      bcm_gh_port_niv_type_set
 * Description:
 *      Set NIV port type.
 * Parameters:
 *      unit - (IN) Device number
 *      port - (IN) Port number
 *      value - (IN) NIV port type 
 * Return Value:
 *      BCM_E_XXX
 */
int
bcm_gh_port_niv_type_set(int unit, bcm_port_t port, int value)
{
    bcm_module_t my_modid;
    int mod_port_index;
    uint64 val64;

    BCM_IF_ERROR_RETURN(_bcm_esw_port_tab_set(unit, port, _BCM_CPU_TABS_ETHER,
                DISCARD_IF_VNTAG_NOT_PRESENTf, 0));
    BCM_IF_ERROR_RETURN(_bcm_esw_port_tab_set(unit, port, _BCM_CPU_TABS_ETHER,
                DISCARD_IF_VNTAG_PRESENTf, 0));
    BCM_IF_ERROR_RETURN(_bcm_esw_port_tab_set(unit, port, _BCM_CPU_TABS_ETHER,
                VNTAG_ACTIONS_IF_PRESENTf, VNTAG_NOOP));
    BCM_IF_ERROR_RETURN(_bcm_esw_port_tab_set(unit, port, _BCM_CPU_TABS_ETHER,
                VNTAG_ACTIONS_IF_NOT_PRESENTf, VNTAG_NOOP));
    BCM_IF_ERROR_RETURN(_bcm_esw_port_tab_set(unit, port, _BCM_CPU_TABS_ETHER,
                TX_DEST_PORT_ENABLEf, 0));
    BCM_IF_ERROR_RETURN(_bcm_esw_port_tab_set(unit, port, _BCM_CPU_TABS_ETHER,
                NIV_VIF_LOOKUP_ENABLEf, 0));
    BCM_IF_ERROR_RETURN(_bcm_esw_port_tab_set(unit, port, _BCM_CPU_TABS_ETHER,
                NIV_RPF_CHECK_ENABLEf, 0));
    BCM_IF_ERROR_RETURN(_bcm_esw_port_tab_set(unit, port, _BCM_CPU_TABS_ETHER,
                NIV_UPLINK_PORTf, 0));
    BCM_IF_ERROR_RETURN(_bcm_esw_port_tab_set(unit, port, _BCM_CPU_TABS_ETHER,
                VT_ENABLEf, 0));

    BCM_IF_ERROR_RETURN(bcm_esw_port_control_set(unit, port,
                bcmPortControlDoNotCheckVlan, 0));

    BCM_IF_ERROR_RETURN(READ_EGR_PORT_64r(unit, port, &val64));
    soc_reg64_field32_set(unit, EGR_PORT_64r, &val64, \
                VNTAG_ACTIONS_IF_PRESENTf, VNTAG_NOOP); 
    soc_reg64_field32_set(unit, EGR_PORT_64r, &val64, \
                NIV_PRUNE_ENABLEf, 0); 
    soc_reg64_field32_set(unit, EGR_PORT_64r, &val64, \
                NIV_UPLINK_PORTf, 0); 
    SOC_IF_ERROR_RETURN(WRITE_EGR_PORT_64r(unit, port, val64));


    BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get(unit, &my_modid));
    BCM_IF_ERROR_RETURN(_bcm_esw_src_mod_port_table_index_get(unit,
                my_modid, port, &mod_port_index));

    switch (value) {
        case BCM_PORT_NIV_TYPE_NONE:
            break;
        case BCM_PORT_NIV_TYPE_SWITCH:
            BCM_IF_ERROR_RETURN(_bcm_esw_port_tab_set(unit, port,
                            _BCM_CPU_TABS_ETHER, VT_KEY_TYPE_USE_GLPf, 1));
            BCM_IF_ERROR_RETURN(_bcm_esw_port_tab_set(unit, port,
                        _BCM_CPU_TABS_ETHER, VT_ENABLEf, 1));
            break;

        case BCM_PORT_NIV_TYPE_UPLINK:
            BCM_IF_ERROR_RETURN(_bcm_esw_port_tab_set(unit, port,
                        _BCM_CPU_TABS_ETHER, DISCARD_IF_VNTAG_NOT_PRESENTf, 1));
            BCM_IF_ERROR_RETURN(_bcm_esw_port_tab_set(unit, port,
                        _BCM_CPU_TABS_ETHER, NIV_VIF_LOOKUP_ENABLEf, 1));
            BCM_IF_ERROR_RETURN(_bcm_esw_port_tab_set(unit, port,
                        _BCM_CPU_TABS_ETHER, NIV_UPLINK_PORTf, 1));
            BCM_IF_ERROR_RETURN(bcm_esw_port_control_set(unit, port,
                        bcmPortControlDoNotCheckVlan, 1));

            BCM_IF_ERROR_RETURN(READ_EGR_PORT_64r(unit, port, &val64));
            soc_reg64_field32_set(unit, EGR_PORT_64r, &val64, \
                        NIV_UPLINK_PORTf, 1); 
            SOC_IF_ERROR_RETURN(WRITE_EGR_PORT_64r(unit, port, val64));

            break;

        case BCM_PORT_NIV_TYPE_DOWNLINK_ACCESS:
            BCM_IF_ERROR_RETURN(_bcm_esw_port_tab_set(unit, port,
                        _BCM_CPU_TABS_ETHER, DISCARD_IF_VNTAG_PRESENTf, 1));
            BCM_IF_ERROR_RETURN(_bcm_esw_port_tab_set(unit, port,
                        _BCM_CPU_TABS_ETHER, VNTAG_ACTIONS_IF_NOT_PRESENTf,
                        VNTAG_ADD));
            BCM_IF_ERROR_RETURN(_bcm_esw_port_tab_set(unit, port,
                        _BCM_CPU_TABS_ETHER, TX_DEST_PORT_ENABLEf, 1));
            BCM_IF_ERROR_RETURN(bcm_esw_port_control_set(unit, port,
                        bcmPortControlDoNotCheckVlan, 1));

            BCM_IF_ERROR_RETURN(READ_EGR_PORT_64r(unit, port, &val64));
            soc_reg64_field32_set(unit, EGR_PORT_64r, &val64, \
                        VNTAG_ACTIONS_IF_PRESENTf, VNTAG_DELETE); 
            soc_reg64_field32_set(unit, EGR_PORT_64r, &val64, \
                        NIV_PRUNE_ENABLEf, 1); 
            SOC_IF_ERROR_RETURN(WRITE_EGR_PORT_64r(unit, port, val64));

            break;

        case BCM_PORT_NIV_TYPE_DOWNLINK_TRANSIT:
            BCM_IF_ERROR_RETURN(_bcm_esw_port_tab_set(unit, port,
                        _BCM_CPU_TABS_ETHER, DISCARD_IF_VNTAG_NOT_PRESENTf, 1));
            BCM_IF_ERROR_RETURN(_bcm_esw_port_tab_set(unit, port,
                        _BCM_CPU_TABS_ETHER, TX_DEST_PORT_ENABLEf, 1));
            BCM_IF_ERROR_RETURN(_bcm_esw_port_tab_set(unit, port,
                        _BCM_CPU_TABS_ETHER, NIV_RPF_CHECK_ENABLEf, 1));
            BCM_IF_ERROR_RETURN(bcm_esw_port_control_set(unit, port,
                        bcmPortControlDoNotCheckVlan, 1));
            break;

        default:
            return BCM_E_PARAM;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_gh_port_niv_type_get
 * Description:
 *      Get NIV port type.
 * Parameters:
 *      unit - (IN) Device number
 *      port - (IN) Port number
 *      value - (OUT) NIV port type 
 * Return Value:
 *      BCM_E_XXX
 */
int
bcm_gh_port_niv_type_get(int unit, bcm_port_t port, int *value)
{
    int rv = BCM_E_NONE;
    int vt_enable, uplink, rpf_enable, prune_enable;
    uint64 val64;

    BCM_IF_ERROR_RETURN(_bcm_esw_port_tab_get(unit,
                port, VT_ENABLEf, &vt_enable));

    BCM_IF_ERROR_RETURN(_bcm_esw_port_tab_get(unit,
                port, NIV_UPLINK_PORTf, &uplink));

    BCM_IF_ERROR_RETURN(_bcm_esw_port_tab_get(unit,
                port, NIV_RPF_CHECK_ENABLEf, &rpf_enable));

	BCM_IF_ERROR_RETURN(READ_EGR_PORT_64r(unit, port, &val64));
    prune_enable = soc_reg64_field32_get(unit, EGR_PORT_64r, val64, 
                   NIV_PRUNE_ENABLEf);

    if (vt_enable) {
        *value = BCM_PORT_NIV_TYPE_SWITCH;
    } else if (uplink) {
        *value = BCM_PORT_NIV_TYPE_UPLINK;
    } else if (rpf_enable) {
        *value = BCM_PORT_NIV_TYPE_DOWNLINK_TRANSIT;
    } else if (prune_enable) {
        *value = BCM_PORT_NIV_TYPE_DOWNLINK_ACCESS;
    } else {
        *value = BCM_PORT_NIV_TYPE_NONE;
    }

    return rv;
}

/*
 * Function:
 *      bcm_gh_port_extender_type_set
 * Description:
 *      Set Port Extender port type.
 * Parameters:
 *      unit - (IN) Device number
 *      port - (IN) Port number
 *      value - (IN) Port type 
 * Return Value:
 *      BCM_E_XXX
 */
int
bcm_gh_port_extender_type_set(int unit, bcm_port_t port, int value)
{
    switch (value) {
        case BCM_PORT_EXTENDER_TYPE_SWITCH:
            BCM_IF_ERROR_RETURN(bcm_gh_port_niv_type_set(unit, port,
                        BCM_PORT_NIV_TYPE_SWITCH));
            break;
        case BCM_PORT_EXTENDER_TYPE_UPLINK:
            BCM_IF_ERROR_RETURN(bcm_gh_port_niv_type_set(unit, port,
                        BCM_PORT_NIV_TYPE_UPLINK));
            break;
        case BCM_PORT_EXTENDER_TYPE_DOWNLINK_ACCESS:
            BCM_IF_ERROR_RETURN(bcm_gh_port_niv_type_set(unit, port,
                        BCM_PORT_NIV_TYPE_DOWNLINK_ACCESS));
            BCM_IF_ERROR_RETURN(_bcm_esw_port_tab_set(unit, port,
                        _BCM_CPU_TABS_ETHER, VNTAG_ACTIONS_IF_NOT_PRESENTf,
                        2));
            break;
        case BCM_PORT_EXTENDER_TYPE_DOWNLINK_TRANSIT:
            BCM_IF_ERROR_RETURN(bcm_gh_port_niv_type_set(unit, port,
                        BCM_PORT_NIV_TYPE_DOWNLINK_TRANSIT));
            break;
        default:
            return BCM_E_PARAM;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_gh_port_extender_type_get
 * Description:
 *      Get Port Extender port type.
 * Parameters:
 *      unit - (IN) Device number
 *      port - (IN) Port number
 *      value - (IN) Port type 
 * Return Value:
 *      BCM_E_XXX
 */
int
bcm_gh_port_extender_type_get(int unit, bcm_port_t port, int *value)
{
    int type;

    BCM_IF_ERROR_RETURN(bcm_gh_port_niv_type_get(unit, port, &type));
    switch (type) {
        case BCM_PORT_NIV_TYPE_SWITCH:
            *value = BCM_PORT_EXTENDER_TYPE_SWITCH;
            break;
        case BCM_PORT_NIV_TYPE_UPLINK:
            *value = BCM_PORT_EXTENDER_TYPE_UPLINK;
            break;
        case BCM_PORT_NIV_TYPE_DOWNLINK_ACCESS:
            *value = BCM_PORT_EXTENDER_TYPE_DOWNLINK_ACCESS;
            break;
        case BCM_PORT_NIV_TYPE_DOWNLINK_TRANSIT:
            *value = BCM_PORT_EXTENDER_TYPE_DOWNLINK_TRANSIT;
            break;
        default:
            return BCM_E_INTERNAL;
    }

    return BCM_E_NONE;
}

#if defined(INCLUDE_L3) 
/*
 * Function:
 *      bcm_gh_port_etag_pcp_de_source_set
 * Description:
 *      Set source of ETAG's PCP and DE fields.
 * Parameters:
 *      unit - (IN) Device number
 *      port - (IN) Port number
 *      value - (IN) PCP and DE source
 * Return Value:
 *      BCM_E_XXX
 */
int
bcm_gh_port_etag_pcp_de_source_set(int unit, bcm_port_t port, int value)
{
    int source;

    switch (value) {
        case BCM_EXTENDER_PCP_DE_SELECT_OUTER_TAG:
            source = 0;
            break;
        case BCM_EXTENDER_PCP_DE_SELECT_INNER_TAG:
            source = 1;
            break;
        case BCM_EXTENDER_PCP_DE_SELECT_DEFAULT:
            source = 2;
            break;
        case BCM_EXTENDER_PCP_DE_SELECT_PHB:
            source = 3;
            break;
        default:
            return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN(_bcm_esw_port_tab_set(unit, port, _BCM_CPU_TABS_ETHER,
                ETAG_PCP_DE_SOURCEf, source));
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_gh_port_etag_pcp_de_source_get
 * Description:
 *      Get source of ETAG's PCP and DE fields.
 * Parameters:
 *      unit - (IN) Device number
 *      port - (IN) Port number
 *      value - (IN) PCP and DE source
 * Return Value:
 *      BCM_E_XXX
 */
int
bcm_gh_port_etag_pcp_de_source_get(int unit, bcm_port_t port, int *value)
{
    int source;

    BCM_IF_ERROR_RETURN(_bcm_esw_port_tab_get(unit, port, ETAG_PCP_DE_SOURCEf,
                &source));

    switch (source) {
        case 0:
            *value = BCM_EXTENDER_PCP_DE_SELECT_OUTER_TAG;
            break;
        case 1:
            *value = BCM_EXTENDER_PCP_DE_SELECT_INNER_TAG;
            break;
        case 2:
            *value = BCM_EXTENDER_PCP_DE_SELECT_DEFAULT;
            break;
        case 3:
            *value = BCM_EXTENDER_PCP_DE_SELECT_PHB;
            break;
        default:
            return BCM_E_INTERNAL;
    }

    return BCM_E_NONE;
}
#endif /* INCLUDE_L3 */

/*
 * Function:
 *      _bcm_gh_port_pmq_swrst_release
 * Description:
 *      To release reset state of QSGMII PCS cores in PM4x10Q.
 * Parameters:
 *      unit - (IN) Device number
 * Return Value:
 *      BCM_E_XXX
 */
STATIC int
_bcm_gh_port_pmq_swrst_release(int unit)
{
    uint32 rval;
    
    BCM_IF_ERROR_RETURN(READ_CHIP_SWRSTr(unit, &rval));
    soc_reg_field_set(unit, CHIP_SWRSTr, &rval, ILKN_BYPASS_RSTNf, 0xf);
    soc_reg_field_set(unit, CHIP_SWRSTr, &rval, FLUSHf, 0);
    soc_reg_field_set(unit, CHIP_SWRSTr, &rval, SOFT_RESET_QSGMII_PCSf, 0);
    soc_reg_field_set(unit, CHIP_SWRSTr, &rval, SOFT_RESET_GPORT1f, 0);
    soc_reg_field_set(unit, CHIP_SWRSTr, &rval, SOFT_RESET_GPORT0f, 0);
    BCM_IF_ERROR_RETURN(WRITE_CHIP_SWRSTr(unit, rval));

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_gh_port_init
 * Description:
 *      For GH specific port init process.
 * Parameters:
 *      unit - (IN) Device number
 * Return Value:
 *      BCM_E_XXX
 */
int
bcm_gh_port_init(int unit)
{
    uint32 rval, pmq_disabled, qmode_en = 0;

    /* special init process for PMQ(PM4x10Q in QSGMII mode) 
     *  - to release PMQ's QSGMII reset state after QSGMII-PCS and UniMAC init.
     */
    BCM_IF_ERROR_RETURN(READ_PGW_CTRL_0r(unit, &rval));
    pmq_disabled = soc_reg_field_get(unit, 
            PGW_CTRL_0r, rval, SW_PM4X10Q_DISABLEf);
    /* ensure the pm4x10Q is enabled and QMODE enabled */
    if (!pmq_disabled) {
        BCM_IF_ERROR_RETURN(READ_CHIP_CONFIGr(unit, &rval));
        qmode_en = soc_reg_field_get(unit, CHIP_CONFIGr, rval, QMODEf);
        if (qmode_en) {
            BCM_IF_ERROR_RETURN(_bcm_gh_port_pmq_swrst_release(unit));
        }
    }

    return BCM_E_NONE;
}

#endif /* BCM_GREYHOUND_SUPPORT */
