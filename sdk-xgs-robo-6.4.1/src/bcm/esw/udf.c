/*
 * $Id: 0d7474bf58d2c8f1f2f39880ee9be34d76e9b2ea $
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
#include <bcm/error.h>

#include <soc/defs.h>
#include <soc/drv.h>

#include <bcm_int/esw/udf.h>


#if defined (BCM_TRIDENT2_SUPPORT)
#if defined(BCM_WARM_BOOT_SUPPORT)
int
_bcm_esw_udf_scache_sync(int unit)
{
    if (SOC_IS_TD2_TT2(unit)) {
        return bcmi_xgs5_udf_wb_sync(unit);
    }

    return BCM_E_NONE;
}
#endif /* BCM_WARM_BOOT_SUPPORT */
#endif /* BCM_TRIDENT2_SUPPORT */

/*
 * Function:
 *      bcm_udf_init
 * Purpose:
 *      Initialize UDF module
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_NONE          UDF module initialized successfully.
 *      BCM_E_UNIT          Invalid BCM Unit number.
 *      BCM_E_UNAVAIL       Feature not supported.
 *      BCM_E_XXX           Standard error code.
 * Notes:
 */
int
bcm_esw_udf_init(
    int unit)
{
#if defined (BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TD2_TT2(unit)) {
        return bcmi_xgs5_udf_init(unit);
    }
#endif

    return BCM_E_UNAVAIL;
}


/*
 * Function:
 *      bcm_udf_detach
 * Purpose:
 *      Detach UDF module
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_NONE          UDF module detached successfully.
 *      BCM_E_UNIT          Invalid BCM Unit number.
 *      BCM_E_INIT          UDF module not initialized.
 *      BCM_E_UNAVAIL       Feature not supported.
 *      BCM_E_XXX           Standard error code.
 * Notes:
 */
int
bcm_esw_udf_detach(
    int unit)
{
#if defined (BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TD2_TT2(unit)) {
        return bcmi_xgs5_udf_detach(unit);
    }
#endif

    return BCM_E_UNAVAIL;
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
bcm_esw_udf_create(
    int unit,
    bcm_udf_alloc_hints_t *hints,
    bcm_udf_t *udf_info,
    bcm_udf_id_t *udf_id)
{
#if defined (BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TD2_TT2(unit)) {
        return bcmi_xgs5_udf_create(unit, hints, udf_info, udf_id);
    }
#endif

    return BCM_E_UNAVAIL;
}


/*
 * Function:
 *      bcm_udf_get
 * Purpose:
 *      Fetches the UDF object created in the system
 * Parameters:
 *      unit - (IN) Unit number.
 *      udf_id - (IN) UDF Object ID
 *      udf_info - (OUT) UDF info structure
 * Returns:
 *      BCM_E_NONE          UDF get successful.
 *      BCM_E_NOT_FOUND     UDF does not exist.
 *      BCM_E_UNIT          Invalid BCM Unit number.
 *      BCM_E_INIT          UDF module not initialized.
 *      BCM_E_UNAVAIL       Feature not supported.
 *      BCM_E_XXX           Standard error code.
 * Notes:
 */
int
bcm_esw_udf_get(
    int unit,
    bcm_udf_id_t udf_id,
    bcm_udf_t *udf_info)
{
#if defined (BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TD2_TT2(unit)) {
        return bcmi_xgs5_udf_get(unit, udf_id, udf_info);
    }
#endif

    return BCM_E_UNAVAIL;
}


/*
 * Function:
 *      bcm_udf_get_all
 * Purpose:
 *      Fetches all existing UDF ids
 * Parameters:
 *      unit - (IN) Unit number.
 *      max - (IN) Max number of UDF IDs
 *      udf_id_list - (OUT) List of UDF IDs
 *      actual - (OUT) Actual udfs retrieved
 * Returns:
 *      BCM_E_NONE          UDF get successful.
 *      BCM_E_UNIT          Invalid BCM Unit number.
 *      BCM_E_INIT          UDF module not initialized.
 *      BCM_E_UNAVAIL       Feature not supported.
 *      BCM_E_XXX           Standard error code.
 * Notes:
 */
int
bcm_esw_udf_get_all(
    int unit,
    bcm_udf_id_t max,
    bcm_udf_id_t *udf_id_list,
    int *actual)
{
#if defined (BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TD2_TT2(unit)) {
        return bcmi_xgs5_udf_get_all(unit, max, udf_id_list, actual);
    }
#endif

    return BCM_E_UNAVAIL;
}


/*
 * Function:
 *      bcm_udf_destroy
 * Purpose:
 *      Destroys the UDF object
 * Parameters:
 *      unit - (IN) Unit number.
 *      udf_id - (IN) UDF Object ID
 * Returns:
 *      BCM_E_NONE          UDF deleted successfully.
 *      BCM_E_NOT_FOUND     UDF does not exist.
 *      BCM_E_UNIT          Invalid BCM Unit number.
 *      BCM_E_INIT          UDF module not initialized.
 *      BCM_E_UNAVAIL       Feature not supported.
 *      BCM_E_XXX           Standard error code.
 * Notes:
 */
int
bcm_esw_udf_destroy(
    int unit,
    bcm_udf_id_t udf_id)
{
#if defined (BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TD2_TT2(unit)) {
        return bcmi_xgs5_udf_destroy(unit, udf_id);
    }
#endif

    return BCM_E_UNAVAIL;
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
bcm_esw_udf_pkt_format_create(
    int unit,
    bcm_udf_pkt_format_options_t options,
    bcm_udf_pkt_format_info_t *pkt_format,
    bcm_udf_pkt_format_id_t *pkt_format_id)
{
#if defined (BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TD2_TT2(unit)) {
        return bcmi_xgs5_udf_pkt_format_create(unit, options,
                                              pkt_format, pkt_format_id);
    }
#endif

    return BCM_E_UNAVAIL;
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
bcm_esw_udf_pkt_format_info_get(
    int unit,
    bcm_udf_pkt_format_id_t pkt_format_id,
    bcm_udf_pkt_format_info_t *pkt_format)
{
#if defined (BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TD2_TT2(unit)) {
        return bcmi_xgs5_udf_pkt_format_info_get(unit,
                                                pkt_format_id, pkt_format);
    }
#endif

    return BCM_E_UNAVAIL;
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
bcm_esw_udf_pkt_format_destroy(
    int unit,
    bcm_udf_pkt_format_id_t pkt_format_id)
{
#if defined (BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TD2_TT2(unit)) {
        return bcmi_xgs5_udf_pkt_format_destroy(unit, pkt_format_id);
    }
#endif

    return BCM_E_UNAVAIL;
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
bcm_esw_udf_pkt_format_add(
    int unit,
    bcm_udf_id_t udf_id,
    bcm_udf_pkt_format_id_t pkt_format_id)
{
#if defined (BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TD2_TT2(unit)) {
        return bcmi_xgs5_udf_pkt_format_add(unit, udf_id, pkt_format_id);
    }
#endif

    return BCM_E_UNAVAIL;
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
bcm_esw_udf_pkt_format_get(
    int unit,
    bcm_udf_pkt_format_id_t pkt_format_id,
    bcm_udf_id_t max,
    bcm_udf_id_t *udf_id_list,
    int *actual)
{
#if defined (BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TD2_TT2(unit)) {
        return bcmi_xgs5_udf_pkt_format_get(unit, pkt_format_id,
                                            max, udf_id_list, actual);
    }
#endif

    return BCM_E_UNAVAIL;
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
bcm_esw_udf_pkt_format_delete(
    int unit,
    bcm_udf_id_t udf_id,
    bcm_udf_pkt_format_id_t pkt_format_id)
{
#if defined (BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TD2_TT2(unit)) {
        return bcmi_xgs5_udf_pkt_format_delete(unit, udf_id, pkt_format_id);
    }
#endif

    return BCM_E_UNAVAIL;
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
bcm_esw_udf_pkt_format_get_all(
    int unit,
    bcm_udf_id_t udf_id,
    int max,
    bcm_udf_pkt_format_id_t *pkt_format_id_list,
    int *actual)
{
#if defined (BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TD2_TT2(unit)) {
        return bcmi_xgs5_udf_pkt_format_get_all(unit, udf_id, max,
                                               pkt_format_id_list, actual);
    }
#endif

    return BCM_E_UNAVAIL;
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
bcm_esw_udf_pkt_format_delete_all(
    int unit,
    bcm_udf_id_t udf_id)
{
#if defined (BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TD2_TT2(unit)) {
        return bcmi_xgs5_udf_pkt_format_delete_all(unit, udf_id);
    }
#endif

    return BCM_E_UNAVAIL;
}


