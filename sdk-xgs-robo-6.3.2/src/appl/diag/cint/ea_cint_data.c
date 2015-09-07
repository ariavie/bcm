/*
 * $Id: ea_cint_data.c 1.15 Broadcom SDK $
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
 * ea_cint_data.c
 *
 * Hand-coded support for a few SAL core routines.
 *
 */
int soc_core_cint_data_not_empty;
#include <sdk_config.h>
#if defined(INCLUDE_LIB_CINT)

#include <cint_config.h>
#include <cint_types.h>
#include <cint_porting.h>
#include <soc/property.h>
#include <soc/drv.h>
#if defined(BCM_EA_SUPPORT) 
static cint_function_pointer_t __cint_function_pointers[4]; 
#if defined(BCM_TK371X_SUPPORT)
#include <soc/ea/tk371x/onu.h>

CINT_FWRAPPER_CREATE_RP2(int,int,0,0,
						 soc_base_mac_get,
                         int,int,unit,0,0,
                         soc_base_mac_t*,soc_base_mac_t,mac,1,0);

CINT_FWRAPPER_CREATE_RP2(int,int,0,0,
						 soc_base_mac_set,
                         int,int,unit,0,0,
                         soc_base_mac_t,soc_base_mac_t,mac,0,0);

CINT_FWRAPPER_CREATE_RP3(int,int,0,0,
						 soc_gpio_read,
                         int,int,unit,0,0,
                         uint32,uint32,flags,0,0,
                         uint32*,uint32,data,1,0);

CINT_FWRAPPER_CREATE_RP4(int,int,0,0,
						 soc_gpio_write,
                         int,int,unit,0,0,
                         uint32,uint32,flags,0,0,
                         uint32,uint32,mask,0,0,
                         uint32,uint32,data,0,0);

CINT_FWRAPPER_CREATE_RP3(int,int,0,0,
						 soc_gpio_config_read,
                         int,int,unit,0,0,
                         uint32,uint32,flags,0,0,
                         uint32*,uint32,mask,1,0);

CINT_FWRAPPER_CREATE_RP3(int,int,0,0,
						 soc_gpio_config_write,
                         int,int,unit,0,0,
                         uint32,uint32,flags,0,0,
                         uint32,uint32,mask,0,0);

CINT_FWRAPPER_CREATE_RP1(int, int, 0, 0,
						 soc_nvs_erase,
                         int,int,unit,0,0);
CINT_FWRAPPER_CREATE_RP1(int, int, 0, 0,
						 soc_chip_tk371x_reset,
                         int,int,unit,0,0);  
						 
static cint_parameter_desc_t __cint_struct_members__soc_base_mac_t[] =
{
    {
        "bcm_mac_t",
        "pon_base_mac",
        0,
        0
    },
    {
        "bcm_mac_t",
        "uni_base_mac",
        0,
        0
    },
    { NULL }
};


static void*
__cint_maddr__soc_base_mac_t(void* p, int mnum, cint_struct_type_t* parent)
{
    void* rv;
    soc_base_mac_t* s = (soc_base_mac_t*) p;

    switch(mnum)
    {
        case 0: rv = &(s->pon_base_mac); break;
        case 1: rv = &(s->uni_base_mac); break;
        default: rv = NULL; break;
    }

    return rv;
}
#endif  /* BCM_TK371X_SUPPORT */

static cint_struct_type_t __cint_ea_structures[] =
{
#if defined(BCM_TK371X_SUPPORT)
	{
	"soc_base_mac_t",
	sizeof(soc_base_mac_t),
	__cint_struct_members__soc_base_mac_t,
	__cint_maddr__soc_base_mac_t
	},
#endif  /* BCM_TK371X_SUPPORT */
	{ NULL }
};

static cint_function_t __cint_ea_functions[] =
    {
#ifdef BCM_TK371X_SUPPORT    	
        CINT_FWRAPPER_ENTRY(soc_base_mac_get),
        CINT_FWRAPPER_ENTRY(soc_base_mac_set),
        CINT_FWRAPPER_ENTRY(soc_gpio_read),
        CINT_FWRAPPER_ENTRY(soc_gpio_write),
        CINT_FWRAPPER_ENTRY(soc_gpio_config_read),
        CINT_FWRAPPER_ENTRY(soc_gpio_config_write),
        CINT_FWRAPPER_ENTRY(soc_nvs_erase),
        CINT_FWRAPPER_ENTRY(soc_chip_tk371x_reset),
#endif  /* BCM_TK371X_SUPPORT */
        CINT_ENTRY_LAST
    };

static cint_enum_type_t __cint_ea_enums[] = 
{   
    { NULL }
};   

static cint_parameter_desc_t __cint_ea_typedefs[] =
{ 
    {NULL}
};  
 

static cint_function_pointer_t __cint_function_pointers[] = 
{  
	{  NULL }
};
cint_data_t ea_cint_data =
    {
        NULL,
        __cint_ea_functions,    /* __cint_ea_functions */
        __cint_ea_structures, 	/* __cint_ea_structures */
        __cint_ea_enums,   		/* __cint_ea_enums */
        __cint_ea_typedefs,   	/* __cint_ea_typedefs */
        NULL,   				/* __cint_ea_constants */
        __cint_function_pointers, /* __cint_ea_function_pointers */
    };

#endif  /* BCM_EA_SUPPORT */
#endif  /* INCLUDE_LIB_CINT */
