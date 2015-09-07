/*

 * $Id: drv.c,v 1.146 Broadcom SDK $    
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
 * StrataSwitch driver
 */

#include <shared/bsl.h>

#include <sal/core/libc.h>
#include <shared/alloc.h>
#include <shared/switch.h>
#include <shared/bitop.h>
#include <sal/core/spl.h>
#include <sal/core/sync.h>
#include <sal/core/boot.h>
#include <sal/core/dpc.h>

#include <soc/cm.h>
#include <soc/drv.h>
#include <soc/debug.h>
#ifdef BCM_SBX_SUPPORT
#include <soc/sbx/sbx_drv.h>
#endif
#if defined(BCM_DPP_SUPPORT) || defined(BCM_PETRA_SUPPORT)
#include <soc/dpp/drv.h>
#endif
#ifdef BCM_DFE_SUPPORT
#include <soc/dfe/cmn/dfe_drv.h>
#endif
#ifdef BCM_ARAD_SUPPORT
#include <soc/dpp/drv.h>
#endif
#ifdef BCM_EA_SUPPORT
#include <soc/ea/tk371x/ea_drv.h>
#endif
#ifdef BCM_WARM_BOOT_SUPPORT
#include <soc/mem.h>
#endif
#ifdef BCM_CMICM_SUPPORT
#include <soc/cmicm.h>
#endif
#ifdef BCM_IPROC_SUPPORT
#include <soc/iproc.h>
#endif

/*
 * Driver global variables
 *
 *   soc_control: per-unit driver control structure
 *   soc_persist: persistent per-unit driver control structure
 *   soc_ndev: the number of units created
 *   soc_ndev_attached: the number of units attached
 *   soc_family_suffix: a family suffix, configured by soc property soc_family.
 *     Used what parsing soc properties.
 *     If configured, and property.unit, property.chip and property.group was not
 *     found, property.family is searched for.
 */
soc_control_t   *soc_control[SOC_MAX_NUM_DEVICES];
soc_persist_t   *soc_persist[SOC_MAX_NUM_DEVICES];
int             soc_ndev = 0;
int             soc_eth_ndev = 0;
int             soc_ndev_attached = 0;
int             soc_all_ndev;
uint32          soc_state[SOC_MAX_NUM_DEVICES];
char            *soc_family_suffix[SOC_MAX_NUM_DEVICES] = {0};

/*   soc_wb_mim_state: indicate that all virtual ports / VPNs are MiM */
/*                     Level 1 warm boot only */
#ifdef BCM_WARM_BOOT_SUPPORT
int             soc_wb_mim_state[SOC_MAX_NUM_DEVICES];
#endif

/*
 * Function:
 *      soc_misc_init
 * Purpose:
 *      Initialize miscellaneous chip registers
 * Parameters:
 *      unit - unit number
 * Returns:
 *      SOC_E_XXX
 */
int
soc_misc_init(int unit)
{
    int rv = SOC_E_UNAVAIL;

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "soc_misc_init\n")));

    if (!soc_attached(unit)) {
        return SOC_E_INIT;
    }

    if (SOC_IS_RCPU_ONLY(unit)) {
        return SOC_E_NONE;
    }

    /* Perform Chip-Dependent Initializations */

    if (SOC_FUNCTIONS(unit)) {
        if (SOC_FUNCTIONS(unit)->soc_misc_init) {
#ifdef BCM_XGS_SUPPORT
            if (SOC_IS_XGS(unit)) {
                SOC_MEM_CLEAR_HW_ACC_SET(unit, 1);
            }
#endif /* BCM_XGS_SUPPORT */
            rv = SOC_FUNCTIONS(unit)->soc_misc_init(unit);
#ifdef BCM_XGS_SUPPORT
            if (SOC_IS_XGS(unit)) {
                SOC_MEM_CLEAR_HW_ACC_SET(unit, 0);
            }
#endif /* BCM_XGS_SUPPORT */
        }
    }

    return rv;
}

/*
 * Function:
 *      soc_init
 * Purpose:
 *      Initialize a StrataSwitch without resetting it.
 * Parameters:
 *      unit - StrataSwitch unit #
 * Returns:
 *      SOC_E_XXX
 */
int
soc_init(int unit)
{
    soc_family_suffix[unit] = soc_property_get_str(unit, spn_SOC_FAMILY);

#if defined(BCM_PETRA_SUPPORT)
    if (SOC_IS_DPP(unit)) {
        return (soc_dpp_init(unit, SOC_DPP_RESET_ACTION_OUT_RESET));
    }
#endif /* defined(BCM_PETRA_SUPPORT) */
#if defined(BCM_DFE_SUPPORT)
    if(SOC_IS_DFE(unit)) {
        return (soc_dfe_init(unit,FALSE));
    }
#endif /*  defined(BCM_DFE_SUPPORT)  */

    if (SOC_IS_SIRIUS(unit)) {
#ifdef BCM_SIRIUS_SUPPORT
        return (soc_sbx_reset_init(unit));
#endif /* BCM_SIRIUS_SUPPORT */
    } else if (SOC_IS_ROBO(unit)) {
#ifdef BCM_ROBO_SUPPORT
        return(soc_robo_do_init(unit, FALSE));
#endif
    } else {
#ifdef BCM_ESW_SUPPORT
        return(soc_do_init(unit, FALSE));
#endif
    }

    return SOC_E_UNAVAIL;
}

/*
 * Function:
 *      soc_deinit
 * Purpose:
 *      DeInitialize a Device.
 * Parameters:
 *      unit - Device unit #
 * Returns:
 *      SOC_E_XXX
 */
int
soc_deinit(int unit)
{
#if defined(BCM_DPP_SUPPORT) && defined(BCM_PETRA_SUPPORT)
    if (SOC_IS_DPP(unit)) {
        return (soc_dpp_deinit(unit));
    }
#endif /* defined(BCM_DPP_SUPPORT) */

#if defined(BCM_DFE_SUPPORT)
    if (SOC_IS_DFE(unit)) {
        return (soc_dfe_deinit(unit));
    }
#endif /* defined(BCM_DFE_SUPPORT) */

    /*inform that detaching device is done*/
    SOC_DETACH(unit,0);

    return SOC_E_UNAVAIL;
}

/*
 * Function:
 *      soc_device_reset
 * Purpose:
 *      Perform different device reset modes/actions.
 * Parameters:
 *      unit - Device unit #
 *      mode - reset mode: hard reset, soft reset, init ....
 *      action - im/out/inout.
 * Returns:
 *      SOC_E_XXX
 */
int
soc_device_reset(int unit, int mode, int action)
{

#if defined(BCM_DPP_SUPPORT) && defined(BCM_PETRA_SUPPORT)
    if (SOC_IS_DPP(unit)) {
        return (soc_dpp_device_reset(unit, mode, action));
    }
#endif /* defined(BCM_DPP_SUPPORT) */

    return SOC_E_UNAVAIL;
}

/*
 * Function:
 *      soc_reset_init
 * Purpose:
 *      Reset and initialize a StrataSwitch.
 * Parameters:
 *      unit - StrataSwitch unit #
 * Returns:
 *      SOC_E_XXX
 */
int
soc_reset_init(int unit)
{
    soc_family_suffix[unit] = soc_property_get_str(unit, spn_SOC_FAMILY);

#if defined(BCM_PETRA_SUPPORT)
    if (SOC_IS_DPP(unit)) {
        return (soc_dpp_init(unit, SOC_DPP_RESET_ACTION_INOUT_RESET));
    } else
#endif /* defined(BCM_PETRA_SUPPORT) */
#if defined(BCM_DFE_SUPPORT)
    if (SOC_IS_DFE(unit)) {
        return (soc_dfe_init(unit, TRUE));
    } else 
#endif /* BCM_DFE_SUPPORT */
#ifdef BCM_SBX_SUPPORT
    if (SOC_IS_SBX(unit)) {
        return (soc_sbx_reset_init(unit));
    } else 
#endif /* BCM_SBX_SUPPORT */
    if (SOC_IS_ROBO(unit)) {
#ifdef BCM_ROBO_SUPPORT
        return(soc_robo_do_init(unit, FALSE));
#endif
    } else {
#ifdef BCM_ESW_SUPPORT
        return(soc_do_init(unit, TRUE));
#endif
    }

    return SOC_E_UNAVAIL;
}

/*
 * Function:
 *    soc_dev_name
 * Purpose:
 *    Return official name (or optionally nickname) of attached chip.
 * Parameters:
 *      unit - unit number
 * Returns:
 *    Pointer to static string
 */

const char *
soc_dev_name(int unit)
{
    const char *name = soc_cm_get_name(unit);
    return (name == NULL ? "<UNKNOWN>" : name);
}

/*
 * Function:
 *    soc_attached
 * Purpose:
 *    Check if a specified SOC device is probed and attached.
 * Returns:
 *    TRUE if attached, FALSE otherwise.
 */
int
soc_attached(int unit)
{
    if (!SOC_UNIT_VALID(unit)) {
        return(0);
    }

    return ((SOC_CONTROL(unit)->soc_flags & SOC_F_ATTACHED) != 0);
}

/*
 * Function:
 *      _str_to_val
 * Purpose:
 *      Convert string to value
 * Parameters:
 *      str - (IN) value string
 *      val - (OUT) converted value
 *      scale - (IN) scale up factor (for decimal only)
 *                   1 means to multiply by 10 before return
 *                   2 means to multiply by 100 before return
 *                   ...
 *      suffix - (OUT) the unit specified in the property (for decimal only)
 *                     'b' for 100b, or 'c' for 500c, ...
 * Notes:
 *      The string is parsed as a C-style numeric constant.
 *      
 */
STATIC char *
_str_to_val(const char *str, int *val, int scale, char *suffix)
{
    uint32 abs_val;
    int neg, base, shift;
    unsigned char symbol;
    
    if (*str == '0' && str[1] != '.' && str[1] != ',') {
        str++;

        symbol = *str | 0x20;
        if (symbol == 'x') { /* hexadecimal */
            str++;
            for (abs_val = 0; *str != '\0'; str++) {
                symbol = *str - '0';
                if (symbol < 10) {
                    abs_val = (abs_val << 4) | symbol;
                    continue;
                }
                symbol = (*str | 0x20) - 'a';
                if (symbol < 6) {
                    abs_val = (abs_val << 4) | (symbol + 10);
                    continue;
                }
                break;
            }
        } else {
            if (symbol == 'b') { /* binary */
                base = 2;
                shift = 1;
                str++;
            } else { /* octal */
                base = 8;
                shift = 3;
            }
            for (abs_val = 0; *str != '\0'; str++) {
                symbol = *str - '0';
                if (symbol < base) {
                    abs_val = (abs_val << shift) | symbol;
                    continue;
                }
                break;
            }
        }
        *val = abs_val;
        *suffix = (*str == ',') ? ',' : '\0';
    } else {
        if (*str == '-') {
            str++;
            neg = TRUE;
        } else {
            neg = FALSE;
        }
        for (abs_val = 0; *str != '\0'; str++) {
            symbol = *str - '0';
            if (symbol < 10) {
                abs_val = abs_val * 10 + symbol;
                continue;
            }
            break;
        }
        if (*str == '.') {
            str++;
            for (; scale > 0 && *str != '\0'; scale--, str++) {
                symbol = *str - '0';
                if (symbol < 10) {
                    abs_val = abs_val * 10 + symbol;
                    continue;
                }
                break;
            }
        }
        for (; scale > 0; scale--) {
            abs_val *= 10;
        }

        *val = neg ? -(int)abs_val : (int)abs_val;
        *suffix = *str;
    }

    return (char *)str;
}

/*
 * Function:
 *  soc_property_get_str
 * Purpose:
 *  Retrieve a global configuration property string.
 * Parameters:
 *  unit - unit number
 *  name - base name of property to look up
 * Notes:
 *  Each property requested is looked up with one suffix after
 *  another until it is found.  Suffixes are tried in the following
 *  order:
 *  .<unit-num>     (e.g. ".0")
 *  .<CHIP_TYPE>    (e.g. BCM5680_B0)
 *  .<CHIP_GROUP>   (e.g. BCM5680)
 *  <nothing>
 */
char *
soc_property_get_str(int unit, const char *name)
{
    if (unit >= 0) {
        int     l = sal_strlen(name) + 1;
        int     ltemp;
        char    name_exp[SOC_PROPERTY_NAME_MAX], *s;

        if (l >= (SOC_PROPERTY_NAME_MAX - 3)) {
            return NULL;
        }
        sal_sprintf(name_exp, "%s.%d", name, unit);
        if ((s = soc_cm_config_var_get(unit, name_exp)) != NULL) {
            return s;
        }

        /* Remaining operations require that the unit is attached */
        if (!soc_attached(unit)) {
            return NULL;
        }

        ltemp = sal_strlen(SOC_UNIT_NAME(unit));
        if (l > (SOC_PROPERTY_NAME_MAX - ltemp)) {
            return NULL;
        }
        sal_strncpy(name_exp + l, SOC_UNIT_NAME(unit), ltemp);
        if (ltemp)
            *(name_exp + l + ltemp) = '\0';
        if ((s = soc_cm_config_var_get(unit, name_exp)) != NULL) {
            return s;
        }

        ltemp = sal_strlen(SOC_UNIT_GROUP(unit));
        if (l > (SOC_PROPERTY_NAME_MAX - ltemp)) {
            return NULL;
        }
        sal_strncpy(name_exp + l, SOC_UNIT_GROUP(unit), ltemp);
        if (ltemp)
            *(name_exp + l + ltemp) = '\0';
        if ((s = soc_cm_config_var_get(unit, name_exp)) != NULL) {
            return s;
        }

        if (soc_family_suffix[unit]) {
            ltemp = sal_strlen(soc_family_suffix[unit]);
            if (l > (SOC_PROPERTY_NAME_MAX - ltemp)) {
                return NULL;
            }
            sal_strncpy(name_exp + l, soc_family_suffix[unit], ltemp);
            if (ltemp)
                *(name_exp + l + ltemp) = '\0';
            if ((s = soc_cm_config_var_get(unit, name_exp)) != NULL) {
                return s;
            }
        }

        
    }

    return soc_cm_config_var_get(unit, name);
}

/*
 * Function:
 *      soc_property_get
 * Purpose:
 *      Retrieve a global configuration numeric value.
 * Parameters:
 *      unit - unit number
 *      name - base name of numeric property to look up
 *      defl - default value of numeric property
 * Notes:
 *      See soc_property_get_str for suffix handling.
 *      The string is parsed as a C-style numeric constant.
 */

uint32
soc_property_get(int unit, const char *name, uint32 defl)
{
    char        *s;

    if ((s = soc_property_get_str(unit, name)) == NULL) {
        return defl;
    }

    return _shr_ctoi(s);
}

/*
 * Function:
 *      soc_property_obj_attr_get
 * Purpose:
 *      Retrieve a global configuration numeric value and its unit for the
 *      specified prefix, object and attribute.
 * Parameters:
 *      unit - unit number
 *      prefix - prefix name of the numeric property to look up
 *      obj - object name of the numeric property to look up
 *      index - object index of the numeric property to look up
 *      attr - attribute name of the numeric property to look up
 *      scale - scale up factor (see _str_to_val)
 *      suffix - the unit specified in the property (see _str_to_val)
 *      defl - default value of numeric property
 * Notes:
 *      The variable name is lookup up with a number of object suffixes
 *      until it is found.
 *              prefix.obj1.attr    attribute of a particular object instance
 *              prefix.obj.attr     attribute of the object type
 *      If none are found then the default value in defl is returned.
 *      See soc_property_get_str for other suffix handling in property name.
 *      See _str_to_val for value string handling.
 *      The string is parsed as a C-style numeric constant.
 */

uint32
soc_property_obj_attr_get(int unit, const char *prefix, const char *obj,
                          int index, const char *attr, int scale,
                          char *suffix, uint32 defl)
{
    char        *str;
    int         val;
    char        prop[80];

    str = NULL;

    if (index != -1) {
        sal_sprintf(prop, "%s.%s%d.%s", prefix, obj, index, attr);
        str = soc_property_get_str(unit, prop);
    }

    if (str == NULL) {
        sal_sprintf(prop, "%s.%s.%s", prefix, obj, attr);
        str = soc_property_get_str(unit, prop);
    }

    if (str == NULL) {
        return defl;
    }

    (void)_str_to_val(str, &val, scale, suffix);
    return (uint32)val;
}

/*
 * Function:
 *      soc_property_get_pbmp
 * Purpose:
 *      Retrieve a global configuration bitmap value.
 * Parameters:
 *      unit - unit number
 *      name - base name of numeric property to look up
 *      defneg - should default bitmap be all 1's?
 * Notes:
 *      See soc_property_get_str for suffix handling.
 *      The string can be more than 32 bits worth of
 *      data if it is in hex format (0x...).  If not
 *      hex, it is treated as a 32 bit value.
 */

pbmp_t
soc_property_get_pbmp(int unit, const char *name, int defneg)
{
    pbmp_t      bm;
    char        *s;

    if ((s = soc_property_get_str(unit, name)) == NULL) {
        SOC_PBMP_CLEAR(bm);
        if (defneg) {
            SOC_PBMP_NEGATE(bm, bm);
        }
        return bm;
    }
    if (_shr_pbmp_decode(s, &bm) < 0) {
        SOC_PBMP_CLEAR(bm);
    }
    return bm;
}

/*
 * Function:
 *      soc_property_get_pbmp_default
 * Purpose:
 *      Retrieve a global configuration bitmap value with specific default.
 * Parameters:
 *      unit - unit number
 *      name - base name of numeric property to look up
 *      def_pbmp - default bitmap
 * Notes:
 *      See soc_property_get_str for suffix handling.
 *      The string can be more than 32 bits worth of
 *      data if it is in hex format (0x...).  If not
 *      hex, it is treated as a 32 bit value.
 */

pbmp_t
soc_property_get_pbmp_default(int unit, const char *name,
                              pbmp_t def_pbmp)
{
    pbmp_t      bm;
    char        *s;

    if ((s = soc_property_get_str(unit, name)) == NULL) {
        SOC_PBMP_ASSIGN(bm, def_pbmp);
        return bm;
    }
    if (_shr_pbmp_decode(s, &bm) < 0) {
        SOC_PBMP_CLEAR(bm);
    }
    return bm;
}

/*
 * Function:
 *      soc_property_suffix_num_str_get
 * Purpose:
 *      Retrieve a per-enuerated suffix global configuration bitmap value.
 * Parameters:
 *      unit - unit number
 *      num - enumerated suffix for which property is applicable
 *      name - base name of numeric property to look up
 *      suffix - suffix of numeric property to look up
 *      pbmp_def - default value of bitmap property
 * Notes:
 *      The variable name is lookup up with a number of suffixes
 *      until it is found.
 *              name_"suffix"0  enumerated suffix name
 *              name            plain name
 *      If none are found then the default value in pbmp_def is returned.
 *      See soc_property_get_str for additional suffix handling.
 *      The string is parsed as an hexadecimal value
 */

pbmp_t
soc_property_suffix_num_pbmp_get(int unit, int num, const char *name,
                            const char *suffix, soc_pbmp_t pbmp_def)
{
    pbmp_t      bm;
    char        *s;

    if ((s = soc_property_suffix_num_str_get(unit, num, name, suffix)) == NULL) {
        SOC_PBMP_ASSIGN(bm, pbmp_def);
        return bm;
    }
    if (_shr_pbmp_decode(s, &bm) < 0) {
        SOC_PBMP_ASSIGN(bm, pbmp_def);
    }
    return bm;
}


/*
 * Function:
 *      soc_property_get_bitmap_default
 * Purpose:
 *      Retrieve a global configuration bitmap value with specific default.
 * Parameters:
 *      unit - unit number
 *      name - base name of numeric property to look up
 *      def_bitmap - default bitmap
 *      bitmap - reterived bitmap
 * Notes:
 *      See soc_property_get_str for suffix handling.
 *      The string can be more than 32 bits worth of
 *      data if it is in hex format (0x...).  If not
 *      hex, it is treated as a 32 bit value.
 */

void
soc_property_get_bitmap_default(int unit, const char *name,
                              uint32 *bitmap, int max_words, uint32 *def_bitmap)
{
    char        *s;

    if ((s = soc_property_get_str(unit, name)) == NULL) {
        sal_memcpy(bitmap, def_bitmap, sizeof(uint32) * max_words);
        return;
    }
    if (shr_bitop_str_decode(s, bitmap, max_words) < 0) {
        sal_memset(bitmap, 0, sizeof(uint32) * max_words);
        return;
    }
}

/*
 * Function:
 *      soc_property_get_csv
 * Purpose:
 *      Retrieve a global configuration comma-separated value in an array.
 * Parameters:
 *      unit - unit number
 *      name - base name of numeric property to look up
 *      val_array - Integer array to put the values
 *      val_max - Maximum number of elements to put into the array
 * Notes:
 *      Returns the count of values read.
 */

int
soc_property_get_csv(int unit, const char *name,
                          int val_max, int *val_array)
{
    char *str, suffix;
    int count;

    str = soc_property_get_str(unit, name);
    if (str == NULL) {
        return 0;
    }

    count = 0;
    for (count = 0; count < val_max; count++) {
        str = _str_to_val(str, &val_array[count], 0, &suffix);
        if (suffix == ',') {
            str++;
        } else {
            count++;
            break;
        }
    }

    return count;
}

/*
 * Function:
 *      soc_property_port_get
 * Purpose:
 *      Retrieve a per-port global configuration value
 * Parameters:
 *      unit - unit number
 *      port - Zero-based port number.
 *      name - base name of numeric property to look up
 *      defl - default value of numeric property
 * Notes:
 *      The variable name is lookup up with a number of suffixes
 *      until it is found.
 *              name_fe0        port name
 *              name_fe         port type
 *              name.port1      physical port number (one based)
 *              name_port1      logical port number (one based)
 *              name            plain name
 *      If none are found then NULL is returned
 *      See soc_property_get_str for additional suffix handling.
 *      The string is parsed as a C-style numeric constant.
 */

char *
soc_property_port_get_str(int unit, soc_port_t port,
                          const char *name)
{
    char        *s, *p;
    char        prop[80];
    int         pno;

    /* try "name_fe0" if in valid range (can be out of range for subport / internal */
    if (port < SOC_MAX_NUM_PORTS) {        
        sal_sprintf(prop, "%s_%s", name, SOC_PORT_NAME(unit, port));
        if ((s = soc_property_get_str(unit, prop)) != NULL) {
            return s;
        }

        /* try "name_fe" (strip off trailing digits) */
        p = &prop[sal_strlen(prop)-1];
        while (*p >= '0' && *p <= '9') {
          --p;
        }
        *++p = '\0';
        if ((s = soc_property_get_str(unit, prop)) != NULL) {
          return s;
        }
    }

    /* try "name.port%d" for explicit match first */
    sal_sprintf(prop, "%s.port%d", name, port+1);
    if ((s = soc_property_get_str(unit, prop)) != NULL) {
        return s;
    }

    /* try "name_port1": search for matching port number */
    /* In the case of the SOC_PORT macros, "port" is being 
     * passed in as a ptype, not a variable */
    for (pno = 0; pno < SOC_PORT_NUM(unit, port); pno++) {
        if (SOC_PORT(unit, port, pno) == port) {
            sal_sprintf(prop, "%s_port%d", name, pno+1);
            if ((s = soc_property_get_str(unit, prop)) != NULL) {
                return s;
            }            
            break;
        }
    }

    /* try "name_%d" for physical port match */
    sal_sprintf(prop, "%s_%d", name, port);
    if ((s = soc_property_get_str(unit, prop)) != NULL) {
        return s;
    }

    /* try plain "name" */
    if ((s = soc_property_get_str(unit, name)) != NULL) {
        return s;
    }

    return NULL;
}

/*
 * Function:
 *      soc_property_port_get
 * Purpose:
 *      Retrieve a per-port global configuration numeric value.
 * Parameters:
 *      unit - unit number
 *      port - Zero-based port number.
 *      name - base name of numeric property to look up
 *      defl - default value of numeric property
 * Notes:
 *      The variable name is lookup up with a number of suffixes
 *      until it is found.
 *              name_fe0        port name
 *              name_fe         port type
 *              name_port1      port number (one based)
 *              name            plain name
 *      If none are found then the default value in defl is returned.
 *      See soc_property_get_str for additional suffix handling.
 *      The string is parsed as a C-style numeric constant.
 */

uint32
soc_property_port_get(int unit, soc_port_t port,
                      const char *name, uint32 defl)
{
    char        *s;

    s = soc_property_port_get_str(unit, port, name);
    if (s) {
      return _shr_ctoi(s);
    }

    return defl;
}

/*
 * Function:
 *      soc_property_port_obj_attr_get
 * Purpose:
 *      Retrieve a global configuration numeric value and its unit for the
 *      specified prefix, object and attribute.
 * Parameters:
 *      unit - unit number
 *      port - Zero-based port number.
 *      prefix - prefix name of the numeric property to look up
 *      obj - object name of the numeric property to look up
 *      index - object index of the numeric property to look up
 *      attr - attribute name of the numeric property to look up
 *      scale - scale up factor (see _str_to_val)
 *      suffix - the unit specified in the property (see _str_to_val)
 *      defl - default value of numeric property
 * Notes:
 *      The variable name is lookup up with a number of object suffixes
 *      until it is found.
 *              prefix.obj1.attr    attribute of a particular object instance
 *              prefix.obj.attr     attribute of the object type
 *      If none are found then the default value in defl is returned.
 *      See soc_property_port_get_str for other suffix handling in property
 *      name.
 *      See _str_to_val for value string handling.
 *      The string is parsed as a C-style numeric constant.
 */

uint32
soc_property_port_obj_attr_get(int unit, soc_port_t port, const char *prefix,
                               const char *obj, int index, const char *attr,
                               int scale, char *suffix, uint32 defl)
{
    char        *str;
    int         val;
    char        prop[80];

    str = NULL;

    if (index != -1) {
        sal_sprintf(prop, "%s.%s%d.%s", prefix, obj, index, attr);
        str = soc_property_port_get_str(unit, port, prop);
    }

    if (str == NULL) {
        sal_sprintf(prop, "%s.%s.%s", prefix, obj, attr);
        str = soc_property_port_get_str(unit, port, prop);
    }

    if (str == NULL) {
        return defl;
    }

    (void)_str_to_val(str, &val, scale, suffix);
    return (uint32)val;
}

int
soc_property_port_get_csv(int unit, soc_port_t port, const char *name,
                          int val_max, int *val_array)
{
    char *str, suffix;
    int count;

    str = soc_property_port_get_str(unit, port, name);
    if (str == NULL) {
        return 0;
    }

    count = 0;
    for (count = 0; count < val_max; count++) {
        str = _str_to_val(str, &val_array[count], 0, &suffix);
        if (suffix == ',') {
            str++;
        } else {
            count++;
            break;
        }
    }

    return count;
}

/*
 * Function:
 *      soc_property_cos_get
 * Purpose:
 *      Retrieve a per-port global configuration value
 * Parameters:
 *      unit - unit number
 *      cos - Zero-based cos number.
 *      name - base name of numeric property to look up
 *      defl - default value of numeric property
 * Notes:
 *      The variable name is lookup up with a number of suffixes
 *      until it is found.
 *              name.cos1       cos number (one based)
 *              name            plain name
 *      If none are found then NULL is returned
 *      See soc_property_get_str for additional suffix handling.
 *      The string is parsed as a C-style numeric constant.
 */

char *
soc_property_cos_get_str(int unit, soc_cos_t cos,
                          const char *name)
{
    char        *s;
    char        prop[80];

    /* try "name.cos%d" for explicit match first */
    sal_sprintf(prop, "%s.cos%d", name, (cos + 1));
    if ((s = soc_property_get_str(unit, prop)) != NULL) {
        return(s);
    }

    /* try plain "name" */
    if ((s = soc_property_get_str(unit, name)) != NULL) {
        return(s);
    }

    return(NULL);
}


/*
 * Function:
 *      soc_property_cos_get
 * Purpose:
 *      Retrieve a per-cos global configuration numeric value.
 * Parameters:
 *      unit - unit number
 *      cos - Zero-based cos number.
 *      name - base name of numeric property to look up
 *      defl - default value of numeric property
 * Notes:
 *      The variable name is lookup up with a number of suffixes
 *      until it is found.
 *              name_cos1       cos number (one based)
 *              name            plain name
 *      If none are found then the default value in defl is returned.
 *      See soc_property_get_str for additional suffix handling.
 *      The string is parsed as a C-style numeric constant.
 */
uint32
soc_property_cos_get(int unit, soc_cos_t cos,
                                    const char *name, uint32 defl)
{
    char        *s;

    s = soc_property_cos_get_str(unit, cos, name);
    if (s) {
      return _shr_ctoi(s);
    }

    return defl;
}


/*
 * Function:
 *      soc_property_suffix_num_get
 * Purpose:
 *      Retrieve a per-enuerated suffix global configuration numeric value.
 * Parameters:
 *      unit - unit number
 *      num - enumerated suffix for which property is applicable
 *      name - base name of numeric property to look up
 *      suffix - suffix of numeric property to look up
 *      defl - default value of numeric property
 * Notes:
 *      The variable name is lookup up with a number of suffixes
 *      until it is found.
 *              name_"suffix"0  enumerated suffix name
 *              name            plain name
 *      If none are found then the default value in defl is returned.
 *      See soc_property_get_str for additional suffix handling.
 *      The string is parsed as a C-style numeric constant.
 */

uint32
soc_property_suffix_num_get(int unit, int num, const char *name,
                            const char *suffix, uint32 defl)
{
    char        *s;
    char        prop[80];

    /* try "name_'suffix'0" */
    sal_sprintf(prop, "%s_%s%1d", name, suffix, num);
    if ((s = soc_property_get_str(unit, prop)) != NULL) {
        return _shr_ctoi(s);
    }

    /* try "name.port%d.suffix" for explicit match first */
    sal_sprintf(prop, "%s.port%d.%s", name, num+1, suffix);
    if ((s = soc_property_get_str(unit, prop)) != NULL) {
        return _shr_ctoi(s);
    }

    /* try "name_'suffix'" */
    sal_sprintf(prop, "%s_%s", name, suffix);
    if ((s = soc_property_get_str(unit, prop)) != NULL) {
        return _shr_ctoi(s);
    }

    /* try plain "name" */
    if ((s = soc_property_get_str(unit, name)) != NULL) {
        return _shr_ctoi(s);
    }

    return defl;
}

/*
 * Function:
 *      soc_property_suffix_num_get_only_suffix
 * Purpose:
 *      Retrieve a per-enuerated suffix global configuration numeric value.
 * Parameters:
 *      unit - unit number
 *      num - enumerated suffix for which property is applicable
 *      name - base name of numeric property to look up
 *      suffix - suffix of numeric property to look up
 *      defl - default value of numeric property
 * Notes:
 *      The variable name is lookup up with a number of suffixes
 *      until it is found.
 *              name_"suffix"0  enumerated suffix name
 *      If none are found then the default value in defl is returned.
 *      See soc_property_get_str for additional suffix handling.
 *      The string is parsed as a C-style numeric constant.
 */

uint32
soc_property_suffix_num_get_only_suffix(int unit, int num, const char *name,
                            const char *suffix, uint32 defl)
{
    char        *s;
    char        prop[80];

    /* try "name_'suffix'0" */
    sal_sprintf(prop, "%s_%s%1d", name, suffix, num);
    if ((s = soc_property_get_str(unit, prop)) != NULL) {
        return _shr_ctoi(s);
    }

    /* try "name.port%d.suffix" for explicit match first */
    sal_sprintf(prop, "%s.port%d.%s", name, num+1, suffix);
    if ((s = soc_property_get_str(unit, prop)) != NULL) {
        return _shr_ctoi(s);
    }

    /* try "name_'suffix'" */
    sal_sprintf(prop, "%s_%s", name, suffix);
    if ((s = soc_property_get_str(unit, prop)) != NULL) {
        return _shr_ctoi(s);
    }

    return defl;
}

/*
 * Function:
 *      soc_property_suffix_num_str_get
 * Purpose:
 *      Retrieve a per-enuerated suffix global configuration numeric value.
 * Parameters:
 *      unit - unit number
 *      num - enumerated suffix for which property is applicable
 *      name - base name of numeric property to look up
 *      suffix - suffix of numeric property to look up
 *      defl - default value of numeric property
 * Notes:
 *      The variable name is lookup up with a number of suffixes
 *      until it is found.
 *              name_"suffix"0  enumerated suffix name
 *              name            plain name
 *      If none are found then the default value in defl is returned.
 *      See soc_property_get_str for additional suffix handling.
 *      The string is parsed as a C-style numeric constant.
 */

char*
soc_property_suffix_num_str_get(int unit, int num, const char *name,
                            const char *suffix)
{
    char        *s;
    char        prop[80];

    /* try "name_'suffix'0" */
    sal_sprintf(prop, "%s_%s%1d", name, suffix, num);
    if ((s = soc_property_get_str(unit, prop)) != NULL) {
        return s;
    }

    /* try "name.port%d.suffix" for explicit match first */
    sal_sprintf(prop, "%s.port%d.%s", name, num+1, suffix);
    if ((s = soc_property_get_str(unit, prop)) != NULL) {
        return s;
    }

    /* try "name_'suffix'" */
    sal_sprintf(prop, "%s_%s", name, suffix);
    if ((s = soc_property_get_str(unit, prop)) != NULL) {
        return s;
    }

    /* try plain "name" */
    if ((s = soc_property_get_str(unit, name)) != NULL) {
        return s;
    }

    return NULL;
}

/*
 * Function:
 *      soc_property_port_suffix_num_get
 * Purpose:
 *      Retrieve a per-port, per-enuerated suffix global
 *      configuration numeric value.
 * Parameters:
 *      unit - unit number
 *      port - Zero-based port number.
 *      num - enumerated suffix for which property is applicable
 *      name - base name of numeric property to look up
 *      suffix - suffix of numeric property to look up
 *      defl - default value of numeric property
 * Notes:
 *      The variable name is lookup up with a number of suffixes
 *      until it is found.
 *              name_"suffix"0_port  enumerated suffix name per port
 *              name_"suffix"0       enumerated suffix name
 *              name_port            plain name per port
 *              name                 plain name
 *      If none are found then the default value in defl is returned.
 *      See soc_property_get_str for additional suffix handling.
 *      The string is parsed as a C-style numeric constant.
 */

uint32
soc_property_port_suffix_num_get(int unit, soc_port_t port,
                                 int num, const char *name,
                                 const char *suffix, uint32 defl)
{
    char        *s;
    char        prop[80];

    /* try "name_'suffix'#" per port*/
    sal_sprintf(prop, "%s_%s%1d", name, suffix, num);
    if ((s = soc_property_port_get_str(unit, port, prop)) != NULL) {
        return _shr_ctoi(s);
    }

    /* try "name_'suffix'#" */
    if ((s = soc_property_get_str(unit, prop)) != NULL) {
        return _shr_ctoi(s);
    }

    /* try "name_'suffix'" per port */
    sal_sprintf(prop, "%s_%s", name, suffix);
    if ((s = soc_property_port_get_str(unit, port, prop)) != NULL) {
        return _shr_ctoi(s);
    }
    /* try "name_'suffix'"*/
    if ((s = soc_property_get_str(unit, prop)) != NULL) {
        return _shr_ctoi(s);
    }

    /* try plain "name" per port*/
    if ((s = soc_property_port_get_str(unit, port, name)) != NULL) {
        return _shr_ctoi(s);
    }

    /* try plain "name" */
    if ((s = soc_property_get_str(unit, name)) != NULL) {
        return _shr_ctoi(s);
    }

    return defl;
}

/*
 * Function:
 *      soc_property_port_suffix_num_get_str
 * Purpose:
 *      Retrieve a per-port, per-enuerated suffix global
 *      configuration string value.
 * Parameters:
 *      unit - unit number
 *      port - Zero-based port number.
 *      num - enumerated suffix for which property is applicable
 *      name - base name of numeric property to look up
 *      suffix - suffix of numeric property to look up     
 * Notes:
 *      The variable name is lookup up with a number of suffixes
 *      until it is found.
 *              name_"suffix"0_port  enumerated suffix name per port
 *              name_"suffix"0       enumerated suffix name
 *              name_port            plain name per port
 *              name                 plain name
 *      If none are found then the NULL is returned.
 *      See soc_property_get_str for additional suffix handling.
 *      The string is parsed as a C-style numeric constant.
 */

char*
soc_property_port_suffix_num_get_str(int unit, soc_port_t port,
                                 int num, const char *name,
                                 const char *suffix)
{
    char        *s;
    char        prop[80];

    /* try "name_'suffix'#" per port*/
    sal_sprintf(prop, "%s_%s%1d", name, suffix, num);
    if ((s = soc_property_port_get_str(unit, port, prop)) != NULL) {
        return s;
    }

    /* try "name_'suffix'#" */
    if ((s = soc_property_get_str(unit, prop)) != NULL) {
        return s;
    }

    /* try "name_'suffix'" per port */
    sal_sprintf(prop, "%s_%s", name, suffix);
    if ((s = soc_property_port_get_str(unit, port, prop)) != NULL) {
        return s;
    }
    /* try "name_'suffix'"*/
    if ((s = soc_property_get_str(unit, prop)) != NULL) {
        return s;
    }

    /* try plain "name" per port*/
    if ((s = soc_property_port_get_str(unit, port, name)) != NULL) {
        return s;
    }

    /* try plain "name" */
    if ((s = soc_property_get_str(unit, name)) != NULL) {
        return s;
    }

    return NULL;
}


/*
 * Function:
 *      soc_property_uc_get_str
 * Purpose:
 *      Retrieve a per-uc global configuration value
 * Parameters:
 *      unit - unit number
 *      uc - Microcontroller number. 0 is PCI Host.
 *      name - base name of numeric property to look up
 * Notes:
 *      The variable name is lookup up with a number of suffixes
 *      until it is found.
 *              name_"suffix"   suffix is pci | uc0 | uc1 ...
 *              name            plain name
 *      If none are found then NULL is returned.
 *      See soc_property_get_str for additional suffix handling.
 *      The string is parsed as a C-style numeric constant.
 */ 
char *
soc_property_uc_get_str(int unit, int uc,
                          const char *name)
{
    char        *s;
    char        prop[80];

    /* try "name_uc%d" for explicit match first */
    if (0 == uc) {
        sal_sprintf(prop, "%s_pci", name);
    } else {
        sal_sprintf(prop, "%s_uc%d", name, (uc - 1));
    }    
    if ((s = soc_property_get_str(unit, prop)) != NULL) {
        return(s);
    }

    /* try plain "name" */
    if ((s = soc_property_get_str(unit, name)) != NULL) {
        return(s);
    }

    return(NULL);
}

/*
 * Function:
 *      soc_property_uc_get
 * Purpose:
 *      Retrieve a per-uc global configuration value
 * Parameters:
 *      unit - unit number
 *      uc - Microcontroller number. 0 is PCI Host.
 *      name - base name of numeric property to look up
 *      defl - default value of numeric property
 * Notes:
 *      The variable name is lookup up with a number of suffixes
 *      until it is found.
 *              name_"suffix"   suffix is pci | uc0 | uc1 ...
 *              name            plain name
 *      If none are found then the default value in defl is returned.
 *      See soc_property_get_str for additional suffix handling.
 *      The string is parsed as a C-style numeric constant.
 */ 
uint32
soc_property_uc_get(int unit, int uc,
                    const char *name, uint32 defl)
{
    char        *s;

    s = soc_property_uc_get_str(unit, uc, name);
    if (s) {
      return _shr_ctoi(s);
    }

    return defl;
}

/*
 * Function:
 *      soc_property_ci_get_str
 * Purpose:
 *      Retrieve a per-ci global configuration value
 * Parameters:
 *      unit - unit number
 *      ci - Chip Intergface Number.
 *      name - base name of numeric property to look up
 * Notes:
 *      The variable name is lookup up with a number of suffixes
 *      until it is found.
 *              name_"suffix"   suffix is ci0 | ci1 ...
 *              name            plain name
 *      If none are found then NULL is returned.
 *      See soc_property_get_str for additional suffix handling.
 *      The string is parsed as a C-style numeric constant.
 */ 
char *
soc_property_ci_get_str(int unit, int ci,
                          const char *name)
{
    char        *s;
    char        prop[80];

    /* try "name_ci%d" for explicit match first */
    sal_sprintf(prop, "%s_ci%d", name, ci);

    if ((s = soc_property_get_str(unit, prop)) != NULL) {
        return(s);
    }

    /* try plain "name" */
    if ((s = soc_property_get_str(unit, name)) != NULL) {
        return(s);
    }

    return(NULL);
}

/*
 * Function:
 *      soc_property_ci_get
 * Purpose:
 *      Retrieve a per-ci global configuration value
 * Parameters:
 *      unit - unit number
 *      ci - Chip Intergface Number.
 *      name - base name of numeric property to look up
 *      defl - default value of numeric property
 * Notes:
 *      The variable name is lookup up with a number of suffixes
 *      until it is found.
 *              name_"suffix"   suffix is ci0 | ci1 ...
 *              name            plain name
 *      If none are found then the default value in defl is returned.
 *      See soc_property_get_str for additional suffix handling.
 *      The string is parsed as a C-style numeric constant.
 */ 
uint32
soc_property_ci_get(int unit, int ci,
                    const char *name, uint32 defl)
{
    char        *s;

    s = soc_property_ci_get_str(unit, ci, name);
    if (s) {
      return _shr_ctoi(s);
    }

    return defl;
}

/*
 * Function:
 *      soc_property_ci_get_csv
 * Purpose:
 *      Retrieve a per-ci global configuration comma-separated value in an array.
 * Parameters:
 *      unit - unit number
 *      ci - Chip Intergface Number.
 *      name - base name of numeric property to look up
 *      val_array - Integer array to put the values
 *      val_max - Maximum number of elements to put into the array
 * Notes:
 *      Returns the count of values read.
 *      The variable name is lookup up with a number of suffixes
 *      until it is found.
 *              name_"suffix"   suffix is ci0 | ci1 ...
 *              name            plain name
 *      If none are found then the default value in defl is returned.
 *      See soc_property_ci_get_str for additional suffix handling.
 *      The string is parsed as a C-style numeric constant.
 */ 
int
soc_property_ci_get_csv(int unit, int ci, const char *name,
                          int val_max, int *val_array)
{
    char *str, suffix;
    int count;

    str = soc_property_ci_get_str(unit, ci, name);
    if (str == NULL) {
        return 0;
    }

    count = 0;
    for (count = 0; count < val_max; count++) {
        str = _str_to_val(str, &val_array[count], 0, &suffix);
        if (suffix == ',') {
            str++;
        } else {
            count++;
            break;
        }
    }

    return count;
}

soc_block_name_t        soc_block_port_names[] =
        SOC_BLOCK_PORT_NAMES_INITIALIZER;
soc_block_name_t        soc_block_names[] =
        SOC_BLOCK_NAMES_INITIALIZER;

soc_block_name_t        soc_sbx_block_port_names[] =
        SOC_BLOCK_SBX_PORT_NAMES_INITIALIZER;
soc_block_name_t        soc_sbx_block_names[] =
        SOC_BLOCK_SBX_NAMES_INITIALIZER;

soc_block_name_t        soc_dpp_block_port_names[] =
        SOC_BLOCK_DPP_PORT_NAMES_INITIALIZER;
soc_block_name_t        soc_dpp_block_names[] =
        SOC_BLOCK_DPP_NAMES_INITIALIZER;

char *
soc_block_port_name_lookup_ext(soc_block_t blk, int unit)
{
    int         i;

#if defined(BCM_SIRIUS_SUPPORT)
    if (SOC_IS_SIRIUS(unit)) {
    for (i = 0; soc_sbx_block_port_names[i].blk != SOC_BLK_NONE; i++) {
        if (soc_sbx_block_port_names[i].blk == blk) {
        return soc_sbx_block_port_names[i].name;
        }
    }
    }
    else 
#endif
    {
        for (i = 0; soc_block_port_names[i].blk != SOC_BLK_NONE; i++) {
            if (soc_block_port_names[i].blk == blk) {
                if (blk == SOC_BLK_GXPORT) {
                    if (SOC_IS_TD_TT(unit)) {
                        return "xlport";
                    }
                } else if (blk == SOC_BLK_BSAFE) {
                    if (SOC_IS_TD_TT(unit)) {
                        return "otpc";
                    }
                }
                return soc_block_port_names[i].name;
            }
        }
    }
    return "?";
}

char *
soc_block_name_lookup_ext(soc_block_t blk, int unit)
{
    int         i;

#if defined(BCM_SIRIUS_SUPPORT) || defined(BCM_CALADAN3_SUPPORT) 
    if (SOC_IS_SIRIUS(unit) || SOC_IS_CALADAN3(unit)) {
    for (i = 0; soc_sbx_block_names[i].blk != SOC_BLK_NONE; i++) {
        if (soc_sbx_block_names[i].blk == blk) {
        return soc_sbx_block_names[i].name;
        }
    }
    }
    else
#endif
#if defined(BCM_DFE_SUPPORT) || defined(BCM_PETRA_SUPPORT)
    if (SOC_IS_DPP(unit) || SOC_IS_DFE(unit)) {
        for (i = 0; soc_dpp_block_names[i].blk != SOC_BLK_NONE; i++) {
            if (soc_dpp_block_names[i].blk == blk) {
                return soc_dpp_block_names[i].name;
            }
        }
    }
    else
#endif

    {
        for (i = 0; soc_block_names[i].blk != SOC_BLK_NONE; i++) {
            if (soc_block_names[i].blk == blk) {
                if (blk == SOC_BLK_GXPORT) {
                    if (SOC_IS_TD_TT(unit)) {
                        return "xlport";
                    }
                } else if (blk == SOC_BLK_BSAFE) {
                    if (SOC_IS_TD_TT(unit)) {
                        return "otpc";
                    }
                }
                return soc_block_names[i].name;
            }
        }
    }
    return "?";
}

soc_block_t
soc_block_port_name_match(char *name)
{
    int         i;

    
    for (i = 0; soc_block_port_names[i].blk != SOC_BLK_NONE; i++) {
        if (sal_strcmp(soc_block_port_names[i].name, name) == 0) {
            return soc_block_port_names[i].blk;
        }
    }
    return SOC_BLK_NONE;
}

soc_block_t
soc_block_name_match(char *name)
{
    int         i;

    
    for (i = 0; soc_block_names[i].blk != SOC_BLK_NONE; i++) {
        if (sal_strcmp(soc_block_names[i].name, name) == 0) {
            return soc_block_names[i].blk;
        }
    }
    return SOC_BLK_NONE;
}

#if defined(BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined(BCM_ARAD_SUPPORT) || defined(BCM_CALADAN3_SUPPORT) || defined(BCM_DFE_SUPPORT)
/*
 * Function:
 *      soc_xport_type_update
 * Purpose:
 *      Rewrite the SOC control port structures to reflect the 10G port mode,
 *      and reinitialize the port
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      to_hg_port - (TRUE/FALSE)
 * Returns:
 *      None.
 * Notes:
 *      Must pause linkscan and take COUNTER_LOCK before calling this.
 */
void
soc_xport_type_update(int unit, soc_port_t port, int to_hg_port)
{
#if defined(BCM_FIREBOLT_SUPPORT) || defined(BCM_BRADLEY_SUPPORT)
    soc_info_t          *si;
    soc_port_t          it_port;
    int                 blk, blktype;

    si = &SOC_INFO(unit);

    /* We need to lock the SOC structures until we finish the update */
    SOC_CONTROL_LOCK(unit);

    if (to_hg_port) {
        SOC_PBMP_PORT_ADD(si->st.bitmap, port);
        SOC_PBMP_PORT_ADD(si->hg.bitmap, port);
        SOC_PBMP_PORT_REMOVE(si->ether.bitmap, port);
        SOC_PBMP_PORT_REMOVE(si->xe.bitmap, port);
    } else {
        SOC_PBMP_PORT_ADD(si->ether.bitmap, port);
        SOC_PBMP_PORT_ADD(si->xe.bitmap, port);
        SOC_PBMP_PORT_REMOVE(si->st.bitmap, port);
        SOC_PBMP_PORT_REMOVE(si->hg.bitmap, port);
    }

#define RECONFIGURE_PORT_TYPE_INFO(ptype) \
    si->ptype.num = 0; \
    si->ptype.min = si->ptype.max = -1; \
    PBMP_ITER(si->ptype.bitmap, it_port) { \
        si->ptype.port[si->ptype.num++] = it_port; \
        if (si->ptype.min < 0) { \
            si->ptype.min = it_port; \
        } \
        if (it_port > si->ptype.max) { \
            si->ptype.max = it_port; \
        } \
    }

    /* Recalculate port type data */
    RECONFIGURE_PORT_TYPE_INFO(ether);
    RECONFIGURE_PORT_TYPE_INFO(st);
    RECONFIGURE_PORT_TYPE_INFO(hg);
    RECONFIGURE_PORT_TYPE_INFO(xe);
#undef  RECONFIGURE_PORT_TYPE_INFO

    soc_dport_map_update(unit);

    /* Resolve block membership for registers */
    if (SOC_IS_FB_FX_HX(unit)) { /* Not HBS */
        for (blk = 0; SOC_BLOCK_INFO(unit, blk).type >= 0; blk++) {
            blktype = SOC_BLOCK_INFO(unit, blk).type;
            switch (blktype) {
            case SOC_BLK_IPIPE:
                if (!to_hg_port) {
                    SOC_PBMP_PORT_ADD(si->block_bitmap[blk], port);
                } else {
                    SOC_PBMP_PORT_REMOVE(si->block_bitmap[blk], port);
                }
                break;
            case SOC_BLK_IPIPE_HI:
                if (to_hg_port) {
                    SOC_PBMP_PORT_ADD(si->block_bitmap[blk], port);
                } else {
                    SOC_PBMP_PORT_REMOVE(si->block_bitmap[blk], port);
                }
                break;
            case SOC_BLK_EPIPE:
                if (!to_hg_port) {
                    SOC_PBMP_PORT_ADD(si->block_bitmap[blk], port);
                } else {
                    SOC_PBMP_PORT_REMOVE(si->block_bitmap[blk], port);
                }
                break;
            case SOC_BLK_EPIPE_HI:
                if (to_hg_port) {
                    SOC_PBMP_PORT_ADD(si->block_bitmap[blk], port);
                } else {
                    SOC_PBMP_PORT_REMOVE(si->block_bitmap[blk], port);
                }
                break;
            }
        }
    }

    /* Release SOC structures lock */
    SOC_CONTROL_UNLOCK(unit);

#else /* BCM_FIREBOLT_SUPPORT || BCM_BRADLEY_SUPPORT */
    COMPILER_REFERENCE(unit);
    COMPILER_REFERENCE(port);
    COMPILER_REFERENCE(to_hg_port);
#endif
}

/*
 * Function:
 *      soc_pci_burst_enable
 * Purpose:
 *      Turn on read/write bursting in the cmic
 * Parameters:
 *      unit - unit number
 *
 * Our hardware by default does not accept burst transactions.
 * These transactions need to be enabled in the CMIC_CONFIG register
 * before a burst is performed or the cmic will hang.
 *
 * This code should be called directly after endian configuration to
 * enable bursting as soon as possible.
 *
 */

void
soc_pci_burst_enable(int unit)
{
    uint32 reg;

    /* Make sure these reads/writes are not combined */
    sal_usleep(1000);

    /* Enable Read/Write bursting in the CMIC */
#ifdef BCM_CMICM_SUPPORT
    if(soc_feature(unit, soc_feature_cmicm)) {
        return;
    }
#endif /* CMICM Support */
    reg = soc_pci_read(unit, CMIC_CONFIG);
    reg |= (CC_RD_BRST_EN | CC_WR_BRST_EN);
    soc_pci_write(unit, CMIC_CONFIG, reg);

    /* Make sure our previous write is not combined */
    sal_usleep(1000);
}

/*
 * Function:
 *      soc_endian_config (private)
 * Purpose:
 *      Utility routine for configuring CMIC endian select register.
 * Parameters:
 *      unit - unit number
 */

void
soc_endian_config(int unit)
{
    uint32          val = 0;
    int             big_pio, big_packet, big_other;
#ifdef BCM_CMICM_SUPPORT
    int cmc, cmc_start, cmc_end;
#endif
#ifdef BCM_IPROC_SUPPORT
    uint32 endian_addr;
#endif

    soc_cm_get_endian(unit, &big_pio, &big_packet, &big_other);

#if defined(WRX_PLT) || defined(GTR_PLT)
    /* Don't enable byte swapping for register read/write. XLP PCIE controllor is configured to do this. */
    big_pio = 0;
#endif

#ifdef BCM_CMICM_SUPPORT
    if (soc_feature(unit, soc_feature_cmicm)) {
        int y;
        uint32 pio_endian = 0;

        if (big_pio) {
            pio_endian = 0x01010101;
        }

#ifdef BCM_IPROC_SUPPORT
        if (soc_feature(unit, soc_feature_iproc) &&
            (soc_cm_get_bus_type(unit) & SOC_PCI_DEV_TYPE)) {
            endian_addr = (soc_cm_get_bus_type(unit) & SOC_DEV_BUS_ALT) ?
                    PAXB_1_ENDIANESS : PAXB_0_ENDIANESS;
            soc_cm_iproc_write(unit, endian_addr, pio_endian);
            if (soc_cm_iproc_read(unit, endian_addr) != 0) {
                pio_endian = 0;
                big_packet = 1;
                big_other = 0;
            }
        }
#endif
        soc_pci_write(unit, CMIC_COMMON_PCIE_PIO_ENDIANESS_OFFSET, pio_endian);

        if (soc_feature(unit, soc_feature_cmicm_multi_dma_cmc)) {
            cmc_start = 0;
            cmc_end = SOC_CMCS_NUM(unit) - 1;
        } else {
            cmc_start = SOC_PCI_CMC(unit);
            cmc_end = cmc_start;
        }

#if defined(BCM_IPROC_SUPPORT) && defined(BE_HOST)
        if (soc_feature(unit, soc_feature_iproc)) {
            WRITE_CMIC_COMMON_UC0_PIO_ENDIANESSr(unit, pio_endian);
        }
#endif

        for (cmc = cmc_start; cmc <= cmc_end; cmc++) {
            if (big_packet) {
                for(y = 0; y < N_DMA_CHAN; y++) {
                    val = soc_pci_read(unit, CMIC_CMCx_CHy_DMA_CTRL_OFFSET(cmc,y));
                    val |= PKTDMA_BIG_ENDIAN;
                    soc_pci_write(unit, CMIC_CMCx_CHy_DMA_CTRL_OFFSET(cmc,y), val);
                }
            }
            if (big_other) {
                /* Set Packet DMA Descriptor Endianness */
                for(y = 0; y < N_DMA_CHAN; y++) {
                    val = soc_pci_read(unit, CMIC_CMCx_CHy_DMA_CTRL_OFFSET(cmc,y));
                    val |= PKTDMA_DESC_BIG_ENDIAN;
                    soc_pci_write(unit, CMIC_CMCx_CHy_DMA_CTRL_OFFSET(cmc,y), val);
                }
#ifdef BCM_SBUSDMA_SUPPORT
                if (soc_feature(unit, soc_feature_sbusdma)) {
                    for(y=0; y < N_SBUSDMA_CHAN; y++) {
                        /* dma/slam */
                        val = soc_pci_read(unit, CMIC_CMCx_SBUSDMA_CHy_REQUEST(cmc, y));
                        soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_REQUESTr, &val,
                                HOSTMEMWR_ENDIANESSf, 1);
                        soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_REQUESTr, &val,
                                HOSTMEMRD_ENDIANESSf, 1);
                        soc_pci_write(unit, CMIC_CMCx_SBUSDMA_CHy_REQUEST(cmc,y), val);
                    }
                } else 
#endif        
                {
                    /* Set SLAM DMA Endianness */
                    val = soc_pci_read(unit, CMIC_CMCx_SLAM_DMA_CFG_OFFSET(cmc));
                    val |= SLDMA_BIG_ENDIAN;
                    soc_pci_write(unit, CMIC_CMCx_SLAM_DMA_CFG_OFFSET(cmc), val);

                    /* Set STATS DMA Endianness */
                    val = soc_pci_read(unit, CMIC_CMCx_STAT_DMA_CFG_OFFSET(cmc));
                    val |= STDMA_BIG_ENDIAN;
                    soc_pci_write(unit, CMIC_CMCx_STAT_DMA_CFG_OFFSET(cmc), val);

                    /* Set TABLE DMA Endianness */
                    val = soc_pci_read(unit, CMIC_CMCx_TABLE_DMA_CFG_OFFSET(cmc));
                    val |= TDMA_BIG_ENDIAN;
                    soc_pci_write(unit, CMIC_CMCx_TABLE_DMA_CFG_OFFSET(cmc), val);
                }

                /* Set FIFO DMA Endianness */
                for(y = 0; y < N_DMA_CHAN; y++) {
                    val = soc_pci_read(unit, CMIC_CMCx_FIFO_CHy_RD_DMA_CFG_OFFSET(cmc,y));
                    val |= FIFODMA_BIG_ENDIAN;
                    soc_pci_write(unit, CMIC_CMCx_FIFO_CHy_RD_DMA_CFG_OFFSET(cmc,y), val);
                }

                /* Set Cross Coupled Memory Endianness */
                /* NA for PCI */
            }
        }
    } else
#endif
    {
        if (big_pio) {
            val |= ES_BIG_ENDIAN_PIO;
        }

        if (big_packet) {
            val |= ES_BIG_ENDIAN_DMA_PACKET;
        }

        if (big_other) {
            val |= ES_BIG_ENDIAN_DMA_OTHER;
        }

        if ((SOC_IS_HAWKEYE(unit) || SOC_IS_RAPTOR(unit) || SOC_IS_RAVEN(unit)) && 
             soc_property_get(unit, spn_EB2_2BYTES_BIG_ENDIAN, 0)) {
            val |= EN_BIG_ENDIAN_EB2_2B_SEL;
        }
#ifdef BCM_SHADOW_SUPPORT
        if (SOC_IS_SHADOW(unit) && soc_property_get(unit, spn_DEVICE_EB_VLI, 0)) {
            val = 0x30000030;
        }
#endif
        soc_pci_write(unit, CMIC_ENDIAN_SELECT, val);
    }
}

void
soc_pci_ep_config(int unit, int pcie)
{
#ifdef BCM_IPROC_SUPPORT
    int blk;
    uint8 acc_type;
    uint32 rval;
    int pci_num = pcie;
    int cmc = SOC_PCI_CMC(unit);

    if (soc_feature(unit, soc_feature_iproc) &&
        (soc_cm_get_bus_type(unit) & SOC_PCI_DEV_TYPE)) {

        if (pcie == -1) { /* Auto-Detect PCI # */
            if ((soc_cm_get_bus_type(unit) & SOC_DEV_BUS_ALT)) {
                pci_num = 1;
            } else {
                pci_num = 0;
            }
        }

        soc_cm_iproc_write(unit, soc_reg_addr_get(unit, DMU_PCU_PCIE_SLAVE_RESET_MODEr, REG_PORT_ANY, 0, TRUE, &blk, &acc_type), 1);

        /* Configure Outbound Address translation */
        if (pci_num == 0) {
            soc_pci_write(unit, CMIC_CMCx_HOSTMEM_ADDR_REMAP_0_OFFSET(cmc), 0x144D2450);
            soc_pci_write(unit, CMIC_CMCx_HOSTMEM_ADDR_REMAP_1_OFFSET(cmc), 0x19617595);
            soc_pci_write(unit, CMIC_CMCx_HOSTMEM_ADDR_REMAP_2_OFFSET(cmc), 0x1E75C6DA);
            soc_pci_write(unit, CMIC_CMCx_HOSTMEM_ADDR_REMAP_3_OFFSET(cmc), 0x1f);
            soc_cm_iproc_write(unit, soc_reg_addr_get(unit, PAXB_0_PCIE_EP_AXI_CONFIGr, REG_PORT_ANY, 0, TRUE, &blk, &acc_type), 0);
            soc_cm_iproc_write(unit, soc_reg_addr_get(unit, PAXB_0_OARR_2r, REG_PORT_ANY, 0, TRUE, &blk, &acc_type), 1);
            soc_cm_iproc_write(unit, soc_reg_addr_get(unit, PAXB_0_OARR_2_UPPERr, REG_PORT_ANY, 0, TRUE, &blk, &acc_type), 1);
            if(soc_feature(unit, soc_feature_iproc_7)){
                soc_cm_iproc_write(unit, soc_reg_addr_get(unit, PAXB_0_OARR_2_UPPERr, 
                     REG_PORT_ANY, 0, TRUE, &blk, &acc_type), 2);
            }
        } else {
            soc_pci_write(unit, CMIC_CMCx_HOSTMEM_ADDR_REMAP_0_OFFSET(cmc), 0x248e2860);
            soc_pci_write(unit, CMIC_CMCx_HOSTMEM_ADDR_REMAP_1_OFFSET(cmc), 0x29a279a5);
            soc_pci_write(unit, CMIC_CMCx_HOSTMEM_ADDR_REMAP_2_OFFSET(cmc), 0x2eb6caea);
            soc_pci_write(unit, CMIC_CMCx_HOSTMEM_ADDR_REMAP_3_OFFSET(cmc), 0x2f);
            soc_cm_iproc_write(unit, soc_reg_addr_get(unit, PAXB_1_PCIE_EP_AXI_CONFIGr, REG_PORT_ANY, 0, TRUE, &blk, &acc_type), 0);
            soc_cm_iproc_write(unit, soc_reg_addr_get(unit, PAXB_1_OARR_2r, REG_PORT_ANY, 0, TRUE, &blk, &acc_type), 1);
            soc_cm_iproc_write(unit, soc_reg_addr_get(unit, PAXB_1_OARR_2_UPPERr, REG_PORT_ANY, 0, TRUE, &blk, &acc_type), 2);

            rval = soc_pci_read(unit, CMIC_CMCx_PCIE_MISCEL_OFFSET(cmc));
                soc_reg_field_set(unit, CMIC_CMC0_PCIE_MISCELr,
                                &rval, MSI_ADDR_SELf, 1);
            soc_pci_write(unit, CMIC_CMCx_PCIE_MISCEL_OFFSET(cmc), rval);
        }
    }
#endif
}

/*
 * Function:
 *      soc_autoz_set
 * Purpose:
 *      Set the autoz bit in the active mac
 * Parameters:
 *      unit - SOC unit #
 *      port - port number on device.
 *      enable - Boolean value to set state
 * Returns:
 *      SOC_E_XXX
 */
int
soc_autoz_set(int unit, soc_port_t port, int enable)
{
    /* XGS (Hercules): AutoZ not supported */
    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_autoz_get
 * Purpose:
 *      Get the state of the autoz bit in the active MAC
 * Parameters:
 *      unit - SOC unit #
 *      port - port number on device.
 *      enable - (OUT) Boolean pointer to return state
 * Returns:
 *      SOC_E_XXX
 */
int
soc_autoz_get(int unit, soc_port_t port, int *enable)
{
    /* XGS (Hercules): AutoZ not supported */
    *enable = 0;
    return SOC_E_NONE;
}

#endif /* defined(BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined(BCM_ARAD_SUPPORT) || defined(BCM_DFE_SUPPORT) */


#if defined(BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || \
  defined(BCM_ROBO_SUPPORT) || defined(BCM_QE2000_SUPPORT) || defined(BCM_EA_SUPPORT) || \
  defined(BCM_PETRA_SUPPORT)|| defined(BCM_DFE_SUPPORT) || defined(BCM_CALADAN3_G3P1_SUPPORT) || defined(PORTMOD_SUPPORT)

int 
soc_event_register(int unit, soc_event_cb_t cb, void *userdata)
{
    soc_control_t       *soc;
    soc_event_cb_list_t *curr, *prev;

    
    /* Input validation */
    if (!SOC_UNIT_VALID(unit)) {
        return SOC_E_UNIT;
    }

    if (NULL == cb) {
        return SOC_E_PARAM;
    }

    soc = SOC_CONTROL(unit);
    curr = prev = soc->ev_cb_head; 

    if (NULL == curr) { /* Fist time a call back registered */
        curr = (soc_event_cb_list_t *)sal_alloc(sizeof(soc_event_cb_list_t),
                                               "event call back linked list");
        if (NULL == curr) {
            return SOC_E_MEMORY;
        }
        curr->cb = cb;
        curr->userdata = userdata;
        curr->next = NULL;
        soc->ev_cb_head = curr;
    } else { /* Not a first registered callback */
        while (NULL != curr) {
            if ((curr->cb == cb) && (curr->userdata == userdata)) {
                /* call back with exact same userdata exists - nothing to be done */
                return SOC_E_NONE;
            }
            prev = curr;
            curr = prev->next;
        }
        curr = (soc_event_cb_list_t *)sal_alloc(sizeof(soc_event_cb_list_t),
                                               "event call back linked list");
        if (NULL == curr) {
            return SOC_E_MEMORY;
        }
        curr->cb = cb;
        curr->userdata = userdata;
        curr->next = NULL;
        prev->next = curr;
    }

    return SOC_E_NONE;
}

int
soc_event_unregister(int unit, soc_event_cb_t cb, void *userdata)
{
    soc_control_t       *soc;
    soc_event_cb_list_t *curr, *prev;

    /* Input validation */
    if (!SOC_UNIT_VALID(unit)) {
        return SOC_E_UNIT;
    }

    if (NULL == cb) {
        return SOC_E_PARAM;
    }

    soc = SOC_CONTROL(unit);
    curr = prev = soc->ev_cb_head;

    while (NULL != curr) {
        if (curr->cb == cb) {
            if ((NULL != userdata) && (curr->userdata != userdata)) {
                /* No match keep searching */
                prev = curr;
                curr = curr->next;
                continue;
            }
            /* if cb & userdata matches or userdata is NULL -> delete */
            if (curr == soc->ev_cb_head) {
                soc->ev_cb_head = curr->next;
                sal_free((void *)curr);
                break;
            }
            prev->next = curr->next;
            sal_free((void *)curr);
            break;
        }
        prev = curr;
        curr = curr->next;
    }

    return (SOC_E_NONE);
}

int
soc_event_generate(int unit,  soc_switch_event_t event, uint32 arg1,
                   uint32 arg2, uint32 arg3)
{
    soc_control_t       *soc;
    soc_event_cb_list_t *curr;

    /* Input validation */
    if (!SOC_UNIT_VALID(unit)) {
        return SOC_E_UNIT;
    }

    soc = SOC_CONTROL(unit);
    curr = soc->ev_cb_head;
    while (NULL != curr) {
        curr->cb(unit, event, arg1, arg2, arg3, curr->userdata);
        curr = curr->next;
    }

    return (SOC_E_NONE);
}

void
soc_event_assert(const char *expr, const char *file, int line)
{
    uint32 arg1, arg3;

    arg1 = PTR_TO_INT(file);
    arg3 = PTR_HI_TO_INT(file);
    
    soc_event_generate(0, SOC_SWITCH_EVENT_ASSERT_ERROR, arg1, line, arg3);
   _default_assert(expr, file, line);
}

#endif /*defined(BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || \
    defined(BCM_ROBO_SUPPORT) || defined(BCM_EA_SUPPORT) || defined(PORTMOD_SUPPORT) */


#if defined(BCM_XGS3_SWITCH_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined(BCM_ARAD_SUPPORT)
/*
 * LCPLL lock check for FBX devices
 */
void
soc_xgxs_lcpll_lock_check(int unit)
{
    if ((!SAL_BOOT_PLISIM) && (!SAL_BOOT_QUICKTURN) &&
        soc_feature(unit, soc_feature_xgxs_lcpll)) {
        uint32 val;
        int pll_lock_usec = 500000;
        int locked = 0;
        int retry = 3;
        soc_timeout_t to;

        /* Check if LCPLL locked */
        while (!locked && retry--) {
            soc_timeout_init(&to, pll_lock_usec, 0);
            while (!soc_timeout_check(&to)) {
#if defined(BCM_RAVEN_SUPPORT)
                if (SOC_REG_IS_VALID(unit, CMIC_XGXS_PLL_STATUSr)) {
                    READ_CMIC_XGXS_PLL_STATUSr(unit, &val);
                    locked = soc_reg_field_get(unit, CMIC_XGXS_PLL_STATUSr,
                                            val, CMIC_XG_PLL_LOCKf);
                } else
#endif
                {
                    READ_CMIC_XGXS_PLL_CONTROL_2r(unit, &val);
                    locked = soc_reg_field_get(unit, CMIC_XGXS_PLL_CONTROL_2r,
                                               val, PLL_SM_FREQ_PASSf);
                }
                if (locked) {
                    break;
                }
            }
            if (!locked) {
                READ_CMIC_XGXS_PLL_CONTROL_1r(unit, &val);
                soc_reg_field_set(unit, CMIC_XGXS_PLL_CONTROL_1r, &val,
                                  RESETf, 1);
                WRITE_CMIC_XGXS_PLL_CONTROL_1r(unit, val);
                sal_usleep(100);

                READ_CMIC_XGXS_PLL_CONTROL_1r(unit, &val);
                val |= 0xf0000000;
                WRITE_CMIC_XGXS_PLL_CONTROL_1r(unit, val);
                sal_usleep(100);

                READ_CMIC_XGXS_PLL_CONTROL_1r(unit, &val);
                soc_reg_field_set(unit, CMIC_XGXS_PLL_CONTROL_1r, &val,
                                  RESETf, 0);
                WRITE_CMIC_XGXS_PLL_CONTROL_1r(unit, val);
                sal_usleep(50);
            }
        }

        if (!locked) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "fbx_lcpll_lock_check: LCPLL not locked on unit %d "
                                  "status = 0x%08x\n"),
                       unit, val));
        }
    }
}
#endif /* BCM_XGS3_SWITCH_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_ARAD_SUPPORT*/


#if defined(BCM_BIGMAC_SUPPORT) || defined(BCM_ARAD_SUPPORT) || defined(BCM_XMAC_SUPPORT)
/* Reset XGXS (unicore, hyperlite, ...) via BigMAC fusion interface or port
 * registers */
int
soc_xgxs_reset(int unit, soc_port_t port)
{
    soc_reg_t           reg;
    uint64              rval64;
    int                 reset_sleep_usec;
    soc_field_t rstb_hw, rstb_mdioregs, rstb_pll, 
                txd1_g_fifo_restb, txd10_g_fifo_restb,
                pwrdwn, pwrdwn_pll;

    reset_sleep_usec = SAL_BOOT_QUICKTURN ? 500000 : 1100;

    reg = MAC_XGXS_CTRLr;
#if defined(BCM_TRIUMPH_SUPPORT)
    if (SOC_IS_TR_VL(unit) && !SOC_IS_TRIUMPH2(unit) && !SOC_IS_APOLLO(unit) &&
         !SOC_IS_ENDURO(unit) && !SOC_IS_HURRICANE(unit) && !SOC_IS_VALKYRIE2(unit) && 
        (port == 6 || port == 7 || port == 18 || port == 19 ||
         port == 35 || port == 36 || port == 46 || port == 47)) {
        reg = XGPORT_XGXS_CTRLr;
    }
#endif /* BCM_TRIUMPH_SUPPORT */
#if defined(BCM_SCORPION_SUPPORT)
    if (SOC_IS_SC_CQ(unit) && (port >= 25 && port <= 28)) {
        reg = QGPORT_MAC_XGXS_CTRLr;
    }
#endif /* BCM_SCORPION_SUPPORT */
#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_SHADOW_SUPPORT)
    if (SOC_IS_TD_TT(unit) || SOC_IS_SHADOW(unit)) {
        reg = XLPORT_XGXS_CTRL_REGr;
    }
#endif /* BCM_TRIDENT_SUPPORT */
#if defined(BCM_KATANA_SUPPORT)
    if (SOC_IS_KATANAX(unit)) {
        reg = XPORT_XGXS_CTRLr;
    }
#endif /* BCM_KATANA_SUPPORT */
#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        reg = PORT_XGXS0_CTRL_REGr;
    }
#endif /* BCM_TRIUMPH3_SUPPORT */
#if defined(BCM_ARAD_SUPPORT)
    if (SOC_IS_ARAD(unit)) {
        reg = PORT_XGXS_0_CTRL_REGr;
    }
#endif /* BCM_ARAD_SUPPORT */

    if (SOC_IS_HELIX4(unit) && SOC_INFO(unit).port_l2p_mapping[port] == 49) {
        /* REFSEL=2'b01, REFDIV=2'b00, REF_TERM_SEL=1'b1. */
        SOC_IF_ERROR_RETURN(soc_reg_get(unit, reg, port, 0, &rval64));
        soc_reg64_field32_set(unit, reg, &rval64, REFSELf, 1);
        soc_reg64_field32_set(unit, reg, &rval64, REF_TERM_SELf, 1);
        soc_reg64_field32_set(unit, reg, &rval64, REFDIVf, 0);
        SOC_IF_ERROR_RETURN(soc_reg_set(unit, reg, port, 0, rval64));
    }

#if defined(BCM_XGS3_SWITCH_SUPPORT) || defined(BCM_SIRIUS_SUPPORT)|| defined(BCM_ARAD_SUPPORT)
    if (SOC_REG_FIELD_VALID(unit, reg, LCREFENf) || 
        SOC_REG_FIELD_VALID(unit, reg, LCREF_ENf)) {
        int lcpll;
        soc_field_t lcpll_f;

        lcpll_f = SOC_REG_FIELD_VALID(unit, reg, LCREFENf) ? LCREFENf : LCREF_ENf;
        /*
         * Reference clock selection
         */
        lcpll = soc_property_port_get(unit, port, spn_XGXS_LCPLL,
                                      SAL_BOOT_QUICKTURN ? 0 : 1);
        if (lcpll && !SOC_IS_ARAD(unit)) { /* Internal LCPLL reference clock */
            /* Double-check LCPLL lock */
            soc_xgxs_lcpll_lock_check(unit);
        }
        SOC_IF_ERROR_RETURN(soc_reg_get(unit, reg, port, 0, &rval64));
        soc_reg64_field32_set(unit, reg, &rval64, lcpll_f, lcpll ? 1 : 0);
        SOC_IF_ERROR_RETURN(soc_reg_set(unit, reg, port, 0, rval64));
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_ARAD_SUPPORT*/

#ifdef BCM_ARAD_SUPPORT
    if (SOC_IS_ARAD(unit)) {
        pwrdwn = PWR_DWNf;
        pwrdwn_pll = PWR_DWN_PLLf;
        rstb_hw = RST_B_HWf;
        rstb_mdioregs = RST_B_MDIOREGSf;
        rstb_pll = RST_B_PLLf;
        txd1_g_fifo_restb = TXD_1_G_FIFO_RST_Bf;
        txd10_g_fifo_restb = TXD_10_G_FIFO_RST_Bf;
    } else 
#endif /* BCM_ARAD_SUPPORT */
    {
        pwrdwn = PWRDWNf;
        pwrdwn_pll = PWRDWN_PLLf;
        rstb_hw = RSTB_HWf;
        rstb_mdioregs = RSTB_MDIOREGSf;
        rstb_pll = RSTB_PLLf;
        txd1_g_fifo_restb = TXD1G_FIFO_RSTBf;
        txd10_g_fifo_restb = TXD10G_FIFO_RSTBf;
    }

    /*
     * XGXS MAC initialization steps.
     *
     * A minimum delay is required between various initialization steps.
     * There is no maximum delay.  The values given are very conservative
     * including the timeout for PLL lock.
     */
    /* Release reset (if asserted) to allow bigmac to initialize */
    SOC_IF_ERROR_RETURN(soc_reg_get(unit, reg, port, 0, &rval64));
    soc_reg64_field32_set(unit, reg, &rval64, IDDQf, 0);
    if (soc_reg_field_valid(unit, reg, pwrdwn)) {
        soc_reg64_field32_set(unit, reg, &rval64, pwrdwn, 0);
    }
    if (soc_reg_field_valid(unit, reg, pwrdwn_pll)) {
        soc_reg64_field32_set(unit, reg, &rval64, pwrdwn_pll, 0);
    }
    if (soc_reg_field_valid(unit,reg, HW_RSTLf)) {
        soc_reg64_field32_set(unit, reg, &rval64, HW_RSTLf, 1);
    } else if (soc_reg_field_valid(unit,reg, rstb_hw)) {
        soc_reg64_field32_set(unit, reg, &rval64, rstb_hw, 1);
    }
    SOC_IF_ERROR_RETURN(soc_reg_set(unit, reg, port, 0, rval64));
    sal_usleep(reset_sleep_usec);

    /* Power down and reset */
    SOC_IF_ERROR_RETURN(soc_reg_get(unit, reg, port, 0, &rval64));
    if (soc_reg_field_valid(unit, reg, pwrdwn)) {
        soc_reg64_field32_set(unit, reg, &rval64, pwrdwn, 1);
    }
    if (soc_reg_field_valid(unit, reg, pwrdwn_pll)) {
        soc_reg64_field32_set(unit, reg, &rval64, pwrdwn_pll, 1);
    }
    soc_reg64_field32_set(unit, reg, &rval64, IDDQf, 1);
    if (soc_reg_field_valid(unit, reg, HW_RSTLf)) {
        soc_reg64_field32_set(unit, reg, &rval64, HW_RSTLf, 0);
    } else if (soc_reg_field_valid(unit, reg, rstb_hw)) {
        soc_reg64_field32_set(unit, reg, &rval64, rstb_hw, 0);
    }
    if (soc_reg_field_valid(unit, reg, TXFIFO_RSTLf)) {
        soc_reg64_field32_set(unit, reg, &rval64, TXFIFO_RSTLf, 0);
    } else if (soc_reg_field_valid(unit, reg, txd1_g_fifo_restb)) {
        soc_reg64_field32_set(unit, reg, &rval64, 
                              txd1_g_fifo_restb, 0);
        soc_reg64_field32_set(unit, reg, &rval64, 
                              txd10_g_fifo_restb, 0);
    }
    if (soc_reg_field_valid(unit, reg, AFIFO_RSTf)) {
        soc_reg64_field32_set(unit, reg, &rval64, AFIFO_RSTf, 1);
    }

    if (SOC_IS_TRX(unit) || SOC_IS_SIRIUS(unit) || SOC_IS_ARAD(unit)) { /* How about Bradley */
        soc_reg64_field32_set(unit, reg, &rval64, rstb_mdioregs, 0);
        soc_reg64_field32_set(unit, reg, &rval64, rstb_pll, 0);
        if (soc_reg_field_valid(unit,reg, BIGMACRSTLf)) {
            soc_reg64_field32_set(unit, reg, &rval64, BIGMACRSTLf, 0);
        }
    }
    SOC_IF_ERROR_RETURN(soc_reg_set(unit, reg, port, 0, rval64));
    sal_usleep(reset_sleep_usec);

    /*
     * Bring up both digital and analog clocks
     *
     * NOTE: Many MAC registers are not accessible until the PLL is locked.
     * An S-Channel timeout will occur before that.
     */
    SOC_IF_ERROR_RETURN(soc_reg_get(unit, reg, port, 0, &rval64));
    if (soc_reg_field_valid(unit, reg, pwrdwn)) {
        soc_reg64_field32_set(unit, reg, &rval64, pwrdwn, 0);
    }
    if (soc_reg_field_valid(unit, reg, pwrdwn_pll)) {
        soc_reg64_field32_set(unit, reg, &rval64, pwrdwn_pll, 0);
    }
    soc_reg64_field32_set(unit, reg, &rval64, IDDQf, 0);
    SOC_IF_ERROR_RETURN(soc_reg_set(unit, reg, port, 0, rval64));
    sal_usleep(reset_sleep_usec);

    /* Bring XGXS out of reset, AFIFO_RST stays 1.  */
    SOC_IF_ERROR_RETURN(soc_reg_get(unit, reg, port, 0, &rval64));
    if (soc_reg_field_valid(unit, reg, HW_RSTLf)) {
        soc_reg64_field32_set(unit, reg, &rval64, HW_RSTLf, 1);
    } else if (soc_reg_field_valid(unit, reg, rstb_hw)) {
        soc_reg64_field32_set(unit, reg, &rval64, rstb_hw, 1);
    }
    SOC_IF_ERROR_RETURN(soc_reg_set(unit, reg, port, 0, rval64));
    sal_usleep(reset_sleep_usec);

    if (SOC_IS_TRX(unit) || SOC_IS_SIRIUS(unit) || SOC_IS_ARAD(unit)) { /* How about Bradley */
        /* Bring MDIO registers out of reset */
        SOC_IF_ERROR_RETURN(soc_reg_get(unit, reg, port, 0, &rval64));
        soc_reg64_field32_set(unit, reg, &rval64, rstb_mdioregs, 1);
        SOC_IF_ERROR_RETURN(soc_reg_set(unit, reg, port, 0, rval64));

        /* Activate all clocks */
        SOC_IF_ERROR_RETURN(soc_reg_get(unit, reg, port, 0, &rval64));
        soc_reg64_field32_set(unit, reg, &rval64, rstb_pll, 1);
        SOC_IF_ERROR_RETURN(soc_reg_set(unit, reg, port, 0, rval64));

        /* Bring BigMac out of reset*/
        if (soc_reg_field_valid(unit,reg, BIGMACRSTLf)) {
            SOC_IF_ERROR_RETURN(soc_reg_get(unit, reg, port, 0, &rval64));
            soc_reg64_field32_set(unit, reg, &rval64, BIGMACRSTLf, 1);
            SOC_IF_ERROR_RETURN(soc_reg_set(unit, reg, port, 0, rval64));
        }
    }

    /* Bring Tx FIFO out of reset */
    SOC_IF_ERROR_RETURN(soc_reg_get(unit, reg, port, 0, &rval64));
    if (soc_reg_field_valid(unit, reg, TXFIFO_RSTLf)) {
        soc_reg64_field32_set(unit, reg, &rval64, TXFIFO_RSTLf, 1);
    } else if (soc_reg_field_valid(unit, reg, txd1_g_fifo_restb)) {
        soc_reg64_field32_set(unit, reg, &rval64, 
                              txd1_g_fifo_restb, 0xf);
        soc_reg64_field32_set(unit, reg, &rval64, 
                              txd10_g_fifo_restb, 1);
    }
    SOC_IF_ERROR_RETURN(soc_reg_set(unit, reg, port, 0, rval64));

#if defined(BCM_HELIX15_SUPPORT) || defined(BCM_FELIX15_SUPPORT) || \
    defined(BCM_FIREBOLT2_SUPPORT)
    /* Release LMD reset */
    if (soc_feature(unit, soc_feature_lmd)) {
#if defined(BCM_FIREBOLT2_SUPPORT)
        if (SOC_REG_IS_VALID(unit, CMIC_1000_BASE_X_MODEr)) {
            uint32 lmd_cmic, lmd_bmp, lmd_ether_bmp;
            int hg_offset;

            hg_offset = port - SOC_HG_OFFSET(unit);

            SOC_IF_ERROR_RETURN
                (READ_CMIC_1000_BASE_X_MODEr(unit, &lmd_cmic));
            lmd_bmp =
                soc_reg_field_get(unit, CMIC_1000_BASE_X_MODEr,
                                  lmd_cmic, LMD_ENABLEf);
            lmd_ether_bmp =
                soc_reg_field_get(unit, CMIC_1000_BASE_X_MODEr,
                                  lmd_cmic, LMD_1000BASEX_ENABLEf);
            if (IS_LMD_ENABLED_PORT(unit, port)) {
                lmd_bmp |= (1 << hg_offset);
                if (IS_XE_PORT(unit,port)) {
                    lmd_ether_bmp |= (1 << hg_offset);
                } else {
                    lmd_ether_bmp &= ~(1 << hg_offset);
                }
            } else {
                lmd_bmp &= ~(1 << hg_offset);
                lmd_ether_bmp &= ~(1 << hg_offset);
            }
            soc_reg_field_set(unit, CMIC_1000_BASE_X_MODEr,
                              &lmd_cmic, LMD_ENABLEf, lmd_bmp);
            soc_reg_field_set(unit, CMIC_1000_BASE_X_MODEr, &lmd_cmic,
                              LMD_1000BASEX_ENABLEf, lmd_ether_bmp);
            SOC_IF_ERROR_RETURN
                (WRITE_CMIC_1000_BASE_X_MODEr(unit, lmd_cmic));
        }
#endif /* BCM_FIREBOLT2_SUPPORT */

        SOC_IF_ERROR_RETURN(READ_MAC_CTRLr(unit, port, &rval64));
        soc_reg64_field32_set(unit, MAC_CTRLr, &rval64, LMD_RSTBf, 1);
        SOC_IF_ERROR_RETURN(WRITE_MAC_CTRLr(unit, port, rval64));
    }
#endif /* BCM_HELIX15_SUPPORT || BCM_FELIX15_SUPPORT ||
        * BCM_FIREBOLT2_SUPPORT */
    return SOC_E_NONE;
}

int
soc_xgxs_in_reset(int unit, soc_port_t port)
{
    soc_reg_t           reg;
    uint64              rval64;
    int                 reset_sleep_usec;
    soc_field_t rstb_hw, rstb_mdioregs, rstb_pll, 
                txd1_g_fifo_restb, txd10_g_fifo_restb,
                pwrdwn, pwrdwn_pll;

    reset_sleep_usec = SAL_BOOT_QUICKTURN ? 500000 : 1100;

    reg = MAC_XGXS_CTRLr;
#if defined(BCM_TRIUMPH_SUPPORT)
    if (SOC_IS_TR_VL(unit) && !SOC_IS_TRIUMPH2(unit) && !SOC_IS_APOLLO(unit) &&
         !SOC_IS_ENDURO(unit) && !SOC_IS_HURRICANE(unit) && !SOC_IS_VALKYRIE2(unit) && 
        (port == 6 || port == 7 || port == 18 || port == 19 ||
         port == 35 || port == 36 || port == 46 || port == 47)) {
        reg = XGPORT_XGXS_CTRLr;
    }
#endif /* BCM_TRIUMPH_SUPPORT */
#if defined(BCM_SCORPION_SUPPORT)
    if (SOC_IS_SC_CQ(unit) && (port >= 25 && port <= 28)) {
        reg = QGPORT_MAC_XGXS_CTRLr;
    }
#endif /* BCM_SCORPION_SUPPORT */
#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_SHADOW_SUPPORT)
    if (SOC_IS_TD_TT(unit) || SOC_IS_SHADOW(unit)) {
        reg = XLPORT_XGXS_CTRL_REGr;
    }
#endif /* BCM_TRIDENT_SUPPORT */
#if defined(BCM_KATANA_SUPPORT)
    if (SOC_IS_KATANAX(unit)) {
        reg = XPORT_XGXS_CTRLr;
    }
#endif /* BCM_KATANA_SUPPORT */
#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        reg = PORT_XGXS0_CTRL_REGr;
    }
#endif /* BCM_TRIUMPH3_SUPPORT */
#if defined(BCM_ARAD_SUPPORT)
    if (SOC_IS_ARAD(unit)) {
        reg = PORT_XGXS_0_CTRL_REGr;
    }
#endif /* BCM_ARAD_SUPPORT */

#if defined(BCM_XGS3_SWITCH_SUPPORT) || defined(BCM_SIRIUS_SUPPORT)|| defined(BCM_ARAD_SUPPORT)
    if (SOC_REG_FIELD_VALID(unit, reg, LCREFENf) || 
        SOC_REG_FIELD_VALID(unit, reg, LCREF_ENf)) {
        int lcpll;
        soc_field_t lcpll_f;

        lcpll_f = SOC_REG_FIELD_VALID(unit, reg, LCREFENf) ? LCREFENf : LCREF_ENf;
        /*
         * Reference clock selection
         */
        lcpll = soc_property_port_get(unit, port, spn_XGXS_LCPLL,
                                      SAL_BOOT_QUICKTURN ? 0 : 1);
        if (lcpll && !SOC_IS_ARAD(unit)) { /* Internal LCPLL reference clock */
            /* Double-check LCPLL lock */
            soc_xgxs_lcpll_lock_check(unit);
        }
        SOC_IF_ERROR_RETURN(soc_reg_get(unit, reg, port, 0, &rval64));
        soc_reg64_field32_set(unit, reg, &rval64, lcpll_f, lcpll ? 1 : 0);
        SOC_IF_ERROR_RETURN(soc_reg_set(unit, reg, port, 0, rval64));
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_ARAD_SUPPORT*/

#ifdef BCM_ARAD_SUPPORT
    if (SOC_IS_ARAD(unit)) {
        pwrdwn = PWR_DWNf;
        pwrdwn_pll = PWR_DWN_PLLf;
        rstb_hw = RST_B_HWf;
        rstb_mdioregs = RST_B_MDIOREGSf;
        rstb_pll = RST_B_PLLf;
        txd1_g_fifo_restb = TXD_1_G_FIFO_RST_Bf;
        txd10_g_fifo_restb = TXD_10_G_FIFO_RST_Bf;
    } else 
#endif /* BCM_ARAD_SUPPORT */
    {
        pwrdwn = PWRDWNf;
        pwrdwn_pll = PWRDWN_PLLf;
        rstb_hw = RSTB_HWf;
        rstb_mdioregs = RSTB_MDIOREGSf;
        rstb_pll = RSTB_PLLf;
        txd1_g_fifo_restb = TXD1G_FIFO_RSTBf;
        txd10_g_fifo_restb = TXD10G_FIFO_RSTBf;
    }

    /*
     * XGXS MAC initialization steps.
     *
     * A minimum delay is required between various initialization steps.
     * There is no maximum delay.  The values given are very conservative
     * including the timeout for PLL lock.
     */
    /* Release reset (if asserted) to allow bigmac to initialize */
    SOC_IF_ERROR_RETURN(soc_reg_get(unit, reg, port, 0, &rval64));
    soc_reg64_field32_set(unit, reg, &rval64, IDDQf, 0);
    if (soc_reg_field_valid(unit, reg, pwrdwn)) {
        soc_reg64_field32_set(unit, reg, &rval64, pwrdwn, 0);
    }
    if (soc_reg_field_valid(unit, reg, pwrdwn_pll)) {
        soc_reg64_field32_set(unit, reg, &rval64, pwrdwn_pll, 0);
    }
    if (soc_reg_field_valid(unit,reg, HW_RSTLf)) {
        soc_reg64_field32_set(unit, reg, &rval64, HW_RSTLf, 1);
    } else if (soc_reg_field_valid(unit,reg, rstb_hw)) {
        soc_reg64_field32_set(unit, reg, &rval64, rstb_hw, 1);
    }
    SOC_IF_ERROR_RETURN(soc_reg_set(unit, reg, port, 0, rval64));
    sal_usleep(reset_sleep_usec);

    /* Power down and reset */
    SOC_IF_ERROR_RETURN(soc_reg_get(unit, reg, port, 0, &rval64));
    if (soc_reg_field_valid(unit, reg, pwrdwn)) {
        soc_reg64_field32_set(unit, reg, &rval64, pwrdwn, 1);
    }
    if (soc_reg_field_valid(unit, reg, pwrdwn_pll)) {
        soc_reg64_field32_set(unit, reg, &rval64, pwrdwn_pll, 1);
    }
    soc_reg64_field32_set(unit, reg, &rval64, IDDQf, 1);
    if (soc_reg_field_valid(unit, reg, HW_RSTLf)) {
        soc_reg64_field32_set(unit, reg, &rval64, HW_RSTLf, 0);
    } else if (soc_reg_field_valid(unit, reg, rstb_hw)) {
        soc_reg64_field32_set(unit, reg, &rval64, rstb_hw, 0);
    }
    if (soc_reg_field_valid(unit, reg, TXFIFO_RSTLf)) {
        soc_reg64_field32_set(unit, reg, &rval64, TXFIFO_RSTLf, 0);
    } else if (soc_reg_field_valid(unit, reg, txd1_g_fifo_restb)) {
        soc_reg64_field32_set(unit, reg, &rval64, 
                              txd1_g_fifo_restb, 0);
        soc_reg64_field32_set(unit, reg, &rval64, 
                              txd10_g_fifo_restb, 0);
    }
    if (soc_reg_field_valid(unit, reg, AFIFO_RSTf)) {
        soc_reg64_field32_set(unit, reg, &rval64, AFIFO_RSTf, 1);
    }

    if (SOC_IS_TRX(unit) || SOC_IS_SIRIUS(unit) || SOC_IS_ARAD(unit)) { /* How about Bradley */
        soc_reg64_field32_set(unit, reg, &rval64, rstb_mdioregs, 0);
        soc_reg64_field32_set(unit, reg, &rval64, rstb_pll, 0);
        if (soc_reg_field_valid(unit,reg, BIGMACRSTLf)) {
            soc_reg64_field32_set(unit, reg, &rval64, BIGMACRSTLf, 0);
        }
    }
    SOC_IF_ERROR_RETURN(soc_reg_set(unit, reg, port, 0, rval64));
    sal_usleep(reset_sleep_usec);

    return SOC_E_NONE;
}

#endif /* BCM_BIGMAC_SUPPORT || BCM_ARAD_SUPPORT*/

#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_ARAD_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
int
soc_wc_xgxs_reset(int unit, soc_port_t port, int reg_idx)
{
#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
    const static soc_reg_t tr3_regs[] = {
        PORT_XGXS0_CTRL_REGr,
        PORT_XGXS1_CTRL_REGr,
        PORT_XGXS2_CTRL_REGr
    };
#endif /* BCM_TRIUMPH3_SUPPORT */
#ifdef BCM_ARAD_SUPPORT
    const static soc_reg_t arad_regs[] = {
        PORT_XGXS_0_CTRL_REGr,
        PORT_XGXS_1_CTRL_REGr,
        PORT_XGXS_2_CTRL_REGr
    };
#endif /* BCM_TRIUMPH3_SUPPORT */
    soc_reg_t   reg;
    uint64      rval64;
    int         reset_sleep_usec = SAL_BOOT_QUICKTURN ? 500000 : 1100;
    int         lcpll;
    soc_field_t rstb_hw, rstb_mdioregs, rstb_pll, 
                txd1_g_fifo_restb, txd10_g_fifo_restb,
                pwrdwn, pwrdwn_pll;

#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit) || SOC_IS_CALADAN3(unit)) {
        reg = tr3_regs[reg_idx];
    } else 
#endif /* BCM_TRIUMPH3_SUPPORT */
#ifdef BCM_ARAD_SUPPORT
    if (SOC_IS_ARAD(unit)) {
        reg = arad_regs[reg_idx];
    } else 
#endif /* BCM_ARAD_SUPPORT */
    {
        reg = XLPORT_XGXS_CTRL_REGr;
    }

#ifdef BCM_ARAD_SUPPORT
    if (SOC_IS_ARAD(unit)) {
        pwrdwn = PWR_DWNf;
        pwrdwn_pll = PWR_DWN_PLLf;
        rstb_hw = RST_B_HWf;
        rstb_mdioregs = RST_B_MDIOREGSf;
        rstb_pll = RST_B_PLLf;
        txd1_g_fifo_restb = TXD_1_G_FIFO_RST_Bf;
        txd10_g_fifo_restb = TXD_10_G_FIFO_RST_Bf;
    } else 
#endif /* BCM_ARAD_SUPPORT */
    {
        pwrdwn = PWRDWNf;
        pwrdwn_pll = PWRDWN_PLLf;
        rstb_hw = RSTB_HWf;
        rstb_mdioregs = RSTB_MDIOREGSf;
        rstb_pll = RSTB_PLLf;
        txd1_g_fifo_restb = TXD1G_FIFO_RSTBf;
        txd10_g_fifo_restb = TXD10G_FIFO_RSTBf;
    }

    /*
     * Reference clock selection
     */
    lcpll = soc_property_port_get(unit, port, spn_XGXS_LCPLL,
                                  SAL_BOOT_QUICKTURN ? 0 : 1);
    SOC_IF_ERROR_RETURN(soc_reg_get(unit, reg, port, 0, &rval64));
    soc_reg64_field32_set(unit, reg, &rval64, LCREF_ENf, lcpll ? 1 : 0);
    SOC_IF_ERROR_RETURN(soc_reg_set(unit, reg, port, 0, rval64));

    /*
     * XGXS MAC initialization steps.
     *
     * A minimum delay is required between various initialization steps.
     * There is no maximum delay.  The values given are very conservative
     * including the timeout for PLL lock.
     */
    /* Release reset (if asserted) to allow xmac/cmac to initialize */
    soc_reg64_field32_set(unit, reg, &rval64, IDDQf, 0);
    soc_reg64_field32_set(unit, reg, &rval64, pwrdwn, 0);
    soc_reg64_field32_set(unit, reg, &rval64, pwrdwn_pll, 0);
    soc_reg64_field32_set(unit, reg, &rval64, rstb_hw, 1);
    SOC_IF_ERROR_RETURN(soc_reg_set(unit, reg, port, 0, rval64));
    sal_usleep(reset_sleep_usec);

    /* Power down and reset */
    soc_reg64_field32_set(unit, reg, &rval64, pwrdwn, 1);
    soc_reg64_field32_set(unit, reg, &rval64, pwrdwn_pll, 1);
    soc_reg64_field32_set(unit, reg, &rval64, IDDQf, 1);
    soc_reg64_field32_set(unit, reg, &rval64, rstb_hw, 0);
    soc_reg64_field32_set(unit, reg, &rval64, rstb_mdioregs, 0);
    soc_reg64_field32_set(unit, reg, &rval64, rstb_pll, 0);
    soc_reg64_field32_set(unit, reg, &rval64, txd1_g_fifo_restb, 0);
    soc_reg64_field32_set(unit, reg, &rval64, txd10_g_fifo_restb, 0);
    SOC_IF_ERROR_RETURN(soc_reg_set(unit, reg, port, 0, rval64));
    sal_usleep(reset_sleep_usec);

    /*
     * Bring up both digital and analog clocks
     *
     * NOTE: Many MAC registers are not accessible until the PLL is locked.
     * An S-Channel timeout will occur before that.
     */
    soc_reg64_field32_set(unit, reg, &rval64, pwrdwn, 0);
    soc_reg64_field32_set(unit, reg, &rval64, pwrdwn_pll, 0);
    soc_reg64_field32_set(unit, reg, &rval64, IDDQf, 0);
    SOC_IF_ERROR_RETURN(soc_reg_set(unit, reg, port, 0, rval64));
    sal_usleep(reset_sleep_usec);

    /* Bring XGXS out of reset */
    soc_reg64_field32_set(unit, reg, &rval64, rstb_hw, 1);
    SOC_IF_ERROR_RETURN(soc_reg_set(unit, reg, port, 0, rval64));
    sal_usleep(reset_sleep_usec);

    /* Bring MDIO registers out of reset */
    soc_reg64_field32_set(unit, reg, &rval64, rstb_mdioregs, 1);
    SOC_IF_ERROR_RETURN(soc_reg_set(unit, reg, port, 0, rval64));

    /* Activate all clocks */
    soc_reg64_field32_set(unit, reg, &rval64, rstb_pll, 1);
    SOC_IF_ERROR_RETURN(soc_reg_set(unit, reg, port, 0, rval64));

    /* Bring Tx FIFO out of reset */
    soc_reg64_field32_set(unit, reg, &rval64, txd1_g_fifo_restb, 0xf);
    soc_reg64_field32_set(unit, reg, &rval64, txd10_g_fifo_restb, 1);
    SOC_IF_ERROR_RETURN(soc_reg_set(unit, reg, port, 0, rval64));

    return SOC_E_NONE;
}

int
soc_wc_xgxs_in_reset(int unit, soc_port_t port, int reg_idx)
{
#ifdef BCM_TRIUMPH3_SUPPORT
    const static soc_reg_t tr3_regs[] = {
        PORT_XGXS0_CTRL_REGr,
        PORT_XGXS1_CTRL_REGr,
        PORT_XGXS2_CTRL_REGr
    };
#endif /* BCM_TRIUMPH3_SUPPORT */
#ifdef BCM_ARAD_SUPPORT
    const static soc_reg_t arad_regs[] = {
        PORT_XGXS_0_CTRL_REGr,
        PORT_XGXS_1_CTRL_REGr,
        PORT_XGXS_2_CTRL_REGr
    };
#endif /* BCM_TRIUMPH3_SUPPORT */
    soc_reg_t   reg;
    uint64      rval64;
    int         reset_sleep_usec = SAL_BOOT_QUICKTURN ? 500000 : 1100;
    int         lcpll;
    soc_field_t rstb_hw, rstb_mdioregs, rstb_pll, 
                txd1_g_fifo_restb, txd10_g_fifo_restb,
                pwrdwn, pwrdwn_pll;

#ifdef BCM_TRIUMPH3_SUPPORT
    if (SOC_IS_TRIUMPH3(unit)) {
        reg = tr3_regs[reg_idx];
    } else 
#endif /* BCM_TRIUMPH3_SUPPORT */
#ifdef BCM_ARAD_SUPPORT
    if (SOC_IS_ARAD(unit)) {
        reg = arad_regs[reg_idx];
    } else 
#endif /* BCM_ARAD_SUPPORT */
    {
        reg = XLPORT_XGXS_CTRL_REGr;
    }

#ifdef BCM_ARAD_SUPPORT
    if (SOC_IS_ARAD(unit)) {
        pwrdwn = PWR_DWNf;
        pwrdwn_pll = PWR_DWN_PLLf;
        rstb_hw = RST_B_HWf;
        rstb_mdioregs = RST_B_MDIOREGSf;
        rstb_pll = RST_B_PLLf;
        txd1_g_fifo_restb = TXD_1_G_FIFO_RST_Bf;
        txd10_g_fifo_restb = TXD_10_G_FIFO_RST_Bf;
    } else 
#endif /* BCM_ARAD_SUPPORT */
    {
        pwrdwn = PWRDWNf;
        pwrdwn_pll = PWRDWN_PLLf;
        rstb_hw = RSTB_HWf;
        rstb_mdioregs = RSTB_MDIOREGSf;
        rstb_pll = RSTB_PLLf;
        txd1_g_fifo_restb = TXD1G_FIFO_RSTBf;
        txd10_g_fifo_restb = TXD10G_FIFO_RSTBf;
    }

    /*
     * Reference clock selection
     */
    lcpll = soc_property_port_get(unit, port, spn_XGXS_LCPLL,
                                  SAL_BOOT_QUICKTURN ? 0 : 1);
    SOC_IF_ERROR_RETURN(soc_reg_get(unit, reg, port, 0, &rval64));
    soc_reg64_field32_set(unit, reg, &rval64, LCREF_ENf, lcpll ? 1 : 0);
    SOC_IF_ERROR_RETURN(soc_reg_set(unit, reg, port, 0, rval64));

    /*
     * XGXS MAC initialization steps.
     *
     * A minimum delay is required between various initialization steps.
     * There is no maximum delay.  The values given are very conservative
     * including the timeout for PLL lock.
     */
    /* Release reset (if asserted) to allow xmac/cmac to initialize */
    soc_reg64_field32_set(unit, reg, &rval64, IDDQf, 0);
    soc_reg64_field32_set(unit, reg, &rval64, pwrdwn, 0);
    soc_reg64_field32_set(unit, reg, &rval64, pwrdwn_pll, 0);
    soc_reg64_field32_set(unit, reg, &rval64, rstb_hw, 1);
    SOC_IF_ERROR_RETURN(soc_reg_set(unit, reg, port, 0, rval64));
    sal_usleep(reset_sleep_usec);

    /* Power down and reset */
    soc_reg64_field32_set(unit, reg, &rval64, pwrdwn, 1);
    soc_reg64_field32_set(unit, reg, &rval64, pwrdwn_pll, 1);
    soc_reg64_field32_set(unit, reg, &rval64, IDDQf, 1);
    soc_reg64_field32_set(unit, reg, &rval64, rstb_hw, 0);
    soc_reg64_field32_set(unit, reg, &rval64, rstb_mdioregs, 0);
    soc_reg64_field32_set(unit, reg, &rval64, rstb_pll, 0);
    soc_reg64_field32_set(unit, reg, &rval64, txd1_g_fifo_restb, 0);
    soc_reg64_field32_set(unit, reg, &rval64, txd10_g_fifo_restb, 0);
    SOC_IF_ERROR_RETURN(soc_reg_set(unit, reg, port, 0, rval64));
    sal_usleep(reset_sleep_usec);

    return SOC_E_NONE;
}

int
soc_wc_xgxs_pll_check(int unit, soc_port_t port, int reg_idx)
{
#ifdef BCM_TRIUMPH3_SUPPORT
    const static soc_reg_t tr3_regs[] = {
        PORT_XGXS0_STATUS_GEN_REGr,
        PORT_XGXS1_STATUS_GEN_REGr,
        PORT_XGXS2_STATUS_GEN_REGr
    };
#endif /* BCM_TRIUMPH3_SUPPORT */
    soc_reg_t   reg;
    uint32      rval;
    int         phy_port, block, retry;
    int         lock_sleep_usec = 5000;

#ifdef BCM_TRIUMPH3_SUPPORT
    if (SOC_IS_TRIUMPH3(unit)) {
        reg = tr3_regs[reg_idx];
    } else
#endif /* BCM_TRIUMPH3_SUPPORT */
    {
        reg = XLPORT_XGXS_STATUS_GEN_REGr;
    }

    retry = 10;

    /* Check TxPLL lock status */
    while (retry > 0) {
        SOC_IF_ERROR_RETURN(soc_reg32_get(unit, reg, port, 0, &rval));
        if (soc_reg_field_get(unit, reg, rval, TXPLL_LOCKf)) {
            return SOC_E_NONE;
        }
        sal_usleep(lock_sleep_usec);
        retry--;
    }

    phy_port = SOC_INFO(unit).port_l2p_mapping[port];
    block = SOC_PORT_BLOCK(unit, phy_port);
    LOG_ERROR(BSL_LS_SOC_COMMON,
              (BSL_META_U(unit,
                          "unit %d %s TXPLL not locked\n"),
               unit, SOC_BLOCK_NAME(unit, block)));
    return SOC_E_NONE;
}

int
soc_wc_xgxs_power_down(int unit, soc_port_t port, int reg_idx)
{
#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
    const static soc_reg_t tr3_regs[] = {
        PORT_XGXS0_CTRL_REGr,
        PORT_XGXS1_CTRL_REGr,
        PORT_XGXS2_CTRL_REGr
    };
#endif /* BCM_TRIUMPH3_SUPPORT */
    soc_reg_t   reg;
    uint64      rval64;

#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit) || SOC_IS_CALADAN3(unit)) {
        reg = tr3_regs[reg_idx];
    } else
#endif /* BCM_TRIUMPH3_SUPPORT */
    {
        reg = XLPORT_XGXS_CTRL_REGr;
    }

    /* Power down and reset */
    SOC_IF_ERROR_RETURN(soc_reg_get(unit, reg, port, 0, &rval64));
    soc_reg64_field32_set(unit, reg, &rval64, PWRDWNf, 1);
    soc_reg64_field32_set(unit, reg, &rval64, PWRDWN_PLLf, 1);
    soc_reg64_field32_set(unit, reg, &rval64, IDDQf, 1);
    soc_reg64_field32_set(unit, reg, &rval64, RSTB_HWf, 0);
    soc_reg64_field32_set(unit, reg, &rval64, RSTB_MDIOREGSf, 0);
    soc_reg64_field32_set(unit, reg, &rval64, RSTB_PLLf, 0);
    soc_reg64_field32_set(unit, reg, &rval64, TXD1G_FIFO_RSTBf, 0);
    soc_reg64_field32_set(unit, reg, &rval64, TXD10G_FIFO_RSTBf, 0);
    SOC_IF_ERROR_RETURN(soc_reg_set(unit, reg, port, 0, rval64));
    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "Power down wc for port: %d\n"), port));
    return SOC_E_NONE;
}
#endif /* BCM_TRIDENT_SUPPORT */

#if defined(BCM_TRIDENT2_SUPPORT) || defined(BCM_HURRICANE2_SUPPORT)
int
soc_tsc_xgxs_reset(int unit, soc_port_t port, int reg_idx)
{
    const static soc_reg_t ctrl_regs[] = {
        XLPORT_XGXS0_CTRL_REGr,
        XLPORT_XGXS0_CTRL_REGr,
        XLPORT_XGXS0_CTRL_REGr,
    };
    soc_reg_t   reg;
    uint64      rval64;
    int         sleep_usec = SAL_BOOT_QUICKTURN ? 500000 : 1100;
    int         lcpll;

    reg = ctrl_regs[reg_idx];

    /*
     * Reference clock selection
     */
    lcpll = soc_property_port_get(unit, port, spn_XGXS_LCPLL,
                                  SAL_BOOT_QUICKTURN ? 0 : 1);
    SOC_IF_ERROR_RETURN(soc_reg_get(unit, reg, port, 0, &rval64));
    soc_reg64_field32_set(unit, reg, &rval64, REFIN_ENf, lcpll ? 1 : 0);
    SOC_IF_ERROR_RETURN(soc_reg_set(unit, reg, port, 0, rval64));

    /* Deassert power down */
    soc_reg64_field32_set(unit, reg, &rval64, PWRDWNf, 0);
    SOC_IF_ERROR_RETURN(soc_reg_set(unit, reg, port, 0, rval64));
    sal_usleep(sleep_usec);

    /* Bring XGXS out of reset */
    soc_reg64_field32_set(unit, reg, &rval64, RSTB_HWf, 1);
    SOC_IF_ERROR_RETURN(soc_reg_set(unit, reg, port, 0, rval64));
    sal_usleep(sleep_usec);

    /* Bring reference clock out of reset */
    if (soc_reg_field_valid(unit, reg, RSTB_REFCLKf)) {
        soc_reg64_field32_set(unit, reg, &rval64, RSTB_REFCLKf, 1);
        SOC_IF_ERROR_RETURN(soc_reg_set(unit, reg, port, 0, rval64));
    }
    /* Activate clocks */
    if (soc_reg_field_valid(unit, reg, RSTB_PLLf)) {
        soc_reg64_field32_set(unit, reg, &rval64, RSTB_PLLf, 1);
        SOC_IF_ERROR_RETURN(soc_reg_set(unit, reg, port, 0, rval64));
    }
    return SOC_E_NONE;
}

int
soc_tsc_xgxs_pll_check(int unit, soc_port_t port, int reg_idx)
{
    const static soc_field_t status_fields[] = {
        TSC_0_AFE_PLL_LOCKf, TSC_1_AFE_PLL_LOCKf,
        TSC_2_AFE_PLL_LOCKf, TSC_3_AFE_PLL_LOCKf,
        TSC_4_AFE_PLL_LOCKf, TSC_5_AFE_PLL_LOCKf,
        TSC_6_AFE_PLL_LOCKf, TSC_7_AFE_PLL_LOCKf,
        TSC_8_AFE_PLL_LOCKf, TSC_9_AFE_PLL_LOCKf,
        TSC_10_AFE_PLL_LOCKf, TSC_11_AFE_PLL_LOCKf,
        TSC_12_AFE_PLL_LOCKf, TSC_13_AFE_PLL_LOCKf,
        TSC_14_AFE_PLL_LOCKf, TSC_15_AFE_PLL_LOCKf,
        TSC_16_AFE_PLL_LOCKf, TSC_17_AFE_PLL_LOCKf,
        TSC_18_AFE_PLL_LOCKf, TSC_19_AFE_PLL_LOCKf,
        TSC_20_AFE_PLL_LOCKf, TSC_21_AFE_PLL_LOCKf,
        TSC_22_AFE_PLL_LOCKf, TSC_23_AFE_PLL_LOCKf,
        TSC_24_AFE_PLL_LOCKf, TSC_25_AFE_PLL_LOCKf,
        TSC_26_AFE_PLL_LOCKf, TSC_27_AFE_PLL_LOCKf,
        TSC_28_AFE_PLL_LOCKf, TSC_29_AFE_PLL_LOCKf,
        TSC_30_AFE_PLL_LOCKf, TSC_31_AFE_PLL_LOCKf,
    };
    soc_reg_t   reg;
    soc_field_t field;
    uint32      rval;
    int         retry;
    int         lock_sleep_usec = 5000;

    reg = TOP_TSC_AFE_PLL_STATUSr;
    field = status_fields[SOC_INFO(unit).port_serdes[port]];

    retry = 10;

    /* Check TxPLL lock status */
    while (retry > 0) {
        SOC_IF_ERROR_RETURN(soc_reg32_get(unit, reg, REG_PORT_ANY, 0, &rval));
        if (soc_reg_field_get(unit, reg, rval, field)) {
            return SOC_E_NONE;
        }
        sal_usleep(lock_sleep_usec);
        retry--;
    }

    LOG_ERROR(BSL_LS_SOC_COMMON,
              (BSL_META_U(unit,
                          "unit %d TSC %d TXPLL not locked\n"),
               unit, SOC_INFO(unit).port_serdes[port]));
    return SOC_E_NONE;
}
#endif /* BCM_TRIDENT2_SUPPORT || BCM_HURRICANE2_SUPPORT */

int soc_is_valid_block_instance(int unit, soc_block_types_t block_types, 
                                int block_instance, int *is_valid) 
{

    int nof_block_instances;

    if (is_valid == NULL) {
        return SOC_E_PARAM;
    }

    if (block_instance < 0) {
        return SOC_E_PARAM;
    }

    /*default*/
    nof_block_instances = 0;

#ifdef BCM_DFE_SUPPORT
    if(SOC_IS_FE1600(unit)) {
        int rv = soc_dfe_nof_block_instances(unit, block_types, &nof_block_instances);
        SOC_IF_ERROR_RETURN(rv);
    }
#endif
#ifdef BCM_ARAD_SUPPORT
    if(SOC_IS_ARAD(unit)) {
        int rv = soc_dpp_nof_block_instances(unit, block_types, &nof_block_instances);
        SOC_IF_ERROR_RETURN(rv);
    }
#endif

    *is_valid = (block_instance < nof_block_instances );

    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_phy_firmware_load
 * Purpose:
 *      Load firmware into internal SerDes core.
 * Parameters:
 *      unit - unit number
 *      port - port number on device.
 *      fw_data - firmware (binary byte array)
 *      fw_size - size of firmware (in bytes)
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      This function is simply a wrapper for a device-specific implmentation.
 */

int
soc_phy_firmware_load(int unit, int port, uint8 *fw_data, int fw_size)
{
    soc_phy_firmware_load_f firmware_load;

    firmware_load = SOC_FUNCTIONS(unit)->soc_phy_firmware_load;
    if (firmware_load != NULL) {
        return firmware_load(unit, port, fw_data, fw_size);
    }
    return SOC_E_UNAVAIL;
}


#ifdef BCM_LEDPROC_SUPPORT

/*
 * Function:
 *      soc_ledproc_config
 * Purpose:
 *      Load a program into the LED microprocessor from a buffer
 *      and start it running.
 * Parameters:
 *      unit - StrataSwitch unit #
 *      program - Array of up to 256 program bytes
 *      bytes - Number of bytes in array, 0 to disable ledproc, upper 16bit
 *              specifies which LED processor(0,1,2, ...). 
 * Notes:
 *      Also clears the LED processor data RAM from 0x80-0xff
 *      so the LED program has a known state at startup.
 */

int
soc_ledproc_config(int unit, const uint8 *program, int bytes)
{
    int         offset;
    uint32      val;
    uint32      led_ctrl;
    uint32      led_prog_base;
    uint32      led_data_base;
#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    int         led_num;
#endif /* BCM_TRIUMPH3_SUPPORT || BCM_TRIUMPH3_SUPPORT */

    if (!soc_feature(unit, soc_feature_led_proc)) {
        return SOC_E_UNAVAIL;
    }
  
#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    led_num = (bytes >> 16) & 0xff;  /* led processor instance */
#endif /* BCM_TRIUMPH3_SUPPORT || BCM_TRIUMPH3_SUPPORT */
    bytes &= 0xffff;   

    /* use soc_feature if more switches use two ledprocs */
#ifdef BCM_TRIDENT_SUPPORT
    if (SOC_IS_TD_TT(unit)) {
        if (led_num == 0) {
            led_ctrl = CMICE_LEDUP0_CTRL;
            led_prog_base = CMICE_LEDUP0_PROGRAM_RAM_BASE;
            led_data_base = CMICE_LEDUP0_DATA_RAM_BASE;
 
        } else if (led_num == 1) { /* Trident only has two */
            led_ctrl = CMICE_LEDUP1_CTRL;
            led_prog_base = CMICE_LEDUP1_PROGRAM_RAM_BASE;
            led_data_base = CMICE_LEDUP1_DATA_RAM_BASE;
        } else {
            return SOC_E_PARAM;
        }
    } else
#endif /* BCM_TRIDENT_SUPPORT */
#ifdef BCM_TRIUMPH3_SUPPORT
    if (SOC_IS_TRIUMPH3(unit)) {
        if (led_num == 0) {
            led_ctrl = CMIC_LEDUP0_CTRL_OFFSET;
            led_prog_base = CMIC_LEDUP0_PROGRAM_RAM_OFFSET;
            led_data_base = CMIC_LEDUP0_DATA_RAM_OFFSET;
        } else if (led_num == 1) {
            led_ctrl = CMIC_LEDUP1_CTRL_OFFSET;
            led_prog_base = CMIC_LEDUP1_PROGRAM_RAM_OFFSET;
            led_data_base = CMIC_LEDUP1_DATA_RAM_OFFSET;
        } else {
            return SOC_E_PARAM;
        }
    } else
#endif /* BCM_TRIUMPH3_SUPPORT */

/* Only Single LED Processor used.. */
#ifdef BCM_CMICM_SUPPORT
    if (soc_feature(unit, soc_feature_cmicm)) {
        led_ctrl = CMIC_LEDUP0_CTRL_OFFSET;
        led_prog_base = CMIC_LEDUP0_PROGRAM_RAM_OFFSET;
        led_data_base = CMIC_LEDUP0_DATA_RAM_OFFSET;
    } else
#endif
    {
        led_ctrl = CMIC_LED_CTRL;
        led_prog_base = CMIC_LED_PROGRAM_RAM_BASE;
        led_data_base = CMIC_LED_DATA_RAM_BASE;
    }

    val = soc_pci_read(unit, led_ctrl);
    val &= ~LC_LED_ENABLE;
    soc_pci_write(unit, led_ctrl, val);

    if (bytes == 0) {
        return SOC_E_NONE;
    }

    for (offset = 0; offset < CMIC_LED_PROGRAM_RAM_SIZE; offset++) {
        soc_pci_write(unit,
                        led_prog_base + CMIC_LED_REG_SIZE * offset,
                        (offset < bytes) ? (uint32) program[offset] : 0);
    }

    for (offset = 0x80; offset < CMIC_LED_DATA_RAM_SIZE; offset++) {
        soc_pci_write(unit,
                        led_data_base + CMIC_LED_REG_SIZE * offset,
                        0);
    }

    val = soc_pci_read(unit, led_ctrl);
    val |= LC_LED_ENABLE;
    soc_pci_write(unit, led_ctrl, val);

    return SOC_E_NONE;
}

#endif /* BCM_LEDPROC_SUPPORT */

