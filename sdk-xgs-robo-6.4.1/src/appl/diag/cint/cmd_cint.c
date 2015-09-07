/*
 * $Id: cmd_cint.c,v 1.32 Broadcom SDK $
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
 *
 */
#include <sdk_config.h>
#include <shared/bsl.h>
int cmd_cint_c_not_empty; 

#ifdef INCLUDE_LIB_CINT


#if !defined(__KERNEL__) && !defined(VXWORKS)
#include <stdlib.h>
#endif

#include "cint_sdk_atomics.h"
#include <cint_interpreter.h>
#include <appl/diag/autocli.h>
#include <appl/diag/cmd_cint.h>
#include <sal/core/alloc.h>
#include <sal/core/libc.h>
#include <sal/appl/io.h>
#include <appl/diag/shell.h>
#include <appl/diag/parse.h>
#include <cint_porting.h>
#include <soc/defs.h>
#ifdef BCM_ARAD_SUPPORT
#include <soc/dpp/ARAD/arad_sw_db.h>
#endif
#ifdef INCLUDE_LIB_C_UNIT
#include <appl/c_unit/c_unit_framework.h>
#include "c_unit_data.h"
#endif

char cmd_cint_usage[] = "cint usage goes here\n"; 


/* bcm_cint_data.c */
extern cint_data_t bcm_cint_data; 

/* sal_cint_data.c */
extern cint_data_t sal_cint_data; 

/* properties.c */
extern void soc_cint_property_vars_init(void); 

#if defined(BCM_EA_SUPPORT)
/* ea_cint_data */
extern cint_data_t ea_cint_data;
#endif

/* soc_cint_data.c */
extern cint_data_t soc_cint_data;

#ifdef BCM_FE2000_SUPPORT
/* g2p3_cint_data.c */
extern cint_data_t g2p3_cint_data;
#endif

#ifdef BCM_SBX_SUPPORT
#ifdef INCLUDE_TEST
/* sbxpkt_cint_data.c */
extern cint_data_t sbxpkt_cint_data;
#endif
#endif

#if defined(BCM_CALADAN3_SUPPORT)
extern cint_data_t caladan3_cint_data;
#endif

#if defined(BCM_CALADAN3_SUPPORT) && defined(BCM_CALADAN3_G3P1_SUPPORT) 
extern cint_data_t g3p1_cint_data;
extern cint_data_t g3p1_ppe_cint_data;
extern cint_data_t g3p1_ocm_cint_data;
extern cint_data_t g3p1_tmu_cint_data;
extern cint_data_t g3p1_cmu_cint_data;
extern cint_data_t g3p1_cop_cint_data;
#endif
#if defined(BCM_CALADAN3_SUPPORT) && defined(BCM_CALADAN3_T3P1_SUPPORT)
/* extern cint_data_t t3p1_cint_data; */
extern cint_data_t t3p1_ppe_cint_data;
extern cint_data_t t3p1_ocm_cint_data;
extern cint_data_t t3p1_tmu_cint_data;
extern cint_data_t t3p1_cmu_cint_data;
extern cint_data_t t3p1_cop_cint_data;
#endif

#ifdef BCM_DFE_SUPPORT
extern cint_data_t dfe_cint_data;
#endif

extern cint_data_t diag_cint_data;
#if defined(_ARAD_TEST_SWDB) && defined(BCM_ARAD_SUPPORT) && defined(INCLUDE_CINT)
extern cint_data_t arad_mcdb_cint_data;
#endif

#ifdef BCM_PETRA_SUPPORT
extern cint_data_t dpp_cint_data;
extern cint_data_t dpp_defines_cint_data;
extern cint_data_t dpp_implementation_defines_cint_data;
#endif

#if defined(BCM_DFE_SUPPORT) || defined(BCM_PETRA_SUPPORT)
extern cint_data_t source_routed_cint_data;
#endif

#if defined(PHYMOD_SUPPORT)
extern cint_data_t phymod_cint_data;
extern cint_data_t phymod_diagnostics_cint_data;
extern cint_data_t phymod_access_cint_data;
#endif

#if defined(PHYMOD_TEIR1_SUPPORT)
extern cint_data_t phymod_teir1_cint_data;
#endif


/* Custom functions */
int bshell(int unit, char* cmd)
{
    return sh_process_command(unit, cmd); 
}
CINT_FWRAPPER_CREATE_RP2(int,int,0,0,
                  bshell, 
                  int,int,unit,0,0,
                  char*,char,cmd,1,0); 

static int
rcload_file_noargs(int unit, char* file) {
    return sh_rcload_file(
        unit,
        NULL,
        file,
        0);
}
    
CINT_FWRAPPER_CREATE_RP2(int,int,0,0,
                         rcload_file_noargs, 
                         int,int,unit,0,0,
                         char*,char,cmd,1,0); 

static cint_function_t __cint_cmd_functions[] = 
    {
        CINT_FWRAPPER_ENTRY(bshell), 
        CINT_FWRAPPER_ENTRY(rcload_file_noargs), 
        CINT_ENTRY_LAST
    }; 

static cint_constants_t __cint_cmd_constants[] = 
    {   
        { "TRUE", 1 }, 
        { "FALSE", 0 }, 
        { NULL }
    }; 

cint_data_t cmd_cint_data = 
    {
        NULL,
        __cint_cmd_functions,
        NULL, 
        NULL, 
        NULL, 
        __cint_cmd_constants, 
        NULL
    }; 


static int
__cint_sdk_data_init(void)
{
    cint_interpreter_add_atomics(cint_sdk_atomics); 
    cint_interpreter_add_data(&cint_sdk_data, NULL); 
#ifdef INCLUDE_LIB_C_UNIT
    cint_interpreter_add_data(&c_unit_data, NULL); 
#endif
    cint_interpreter_add_data(&bcm_cint_data, NULL);     
    cint_interpreter_add_data(&sal_cint_data, NULL); 
    cint_interpreter_add_data(&soc_cint_data, NULL); 
#if defined(BCM_CALADAN3_SUPPORT)
    cint_interpreter_add_data(&caladan3_cint_data, NULL);
#endif
#if defined(BCM_CALADAN3_SUPPORT) && defined(BCM_CALADAN3_G3P1_SUPPORT) 
    cint_interpreter_add_data(&g3p1_cint_data, NULL);
    cint_interpreter_add_data(&g3p1_ppe_cint_data, NULL);
    cint_interpreter_add_data(&g3p1_ocm_cint_data, NULL);
    cint_interpreter_add_data(&g3p1_tmu_cint_data, NULL);
    cint_interpreter_add_data(&g3p1_cmu_cint_data, NULL);
    cint_interpreter_add_data(&g3p1_cop_cint_data, NULL);
#endif
#if defined(BCM_CALADAN3_SUPPORT) && defined(BCM_CALADAN3_T3P1_SUPPORT)
    /* cint_interpreter_add_data(&t3p1_cint_data, NULL); */
    cint_interpreter_add_data(&t3p1_ppe_cint_data, NULL);
    cint_interpreter_add_data(&t3p1_ocm_cint_data, NULL);
    cint_interpreter_add_data(&t3p1_tmu_cint_data, NULL);
    cint_interpreter_add_data(&t3p1_cmu_cint_data, NULL);
    cint_interpreter_add_data(&t3p1_cop_cint_data, NULL);
#endif
#ifdef BCM_FE2000_SUPPORT
    cint_interpreter_add_data(&g2p3_cint_data, NULL);
#endif
#ifdef BCM_SBX_SUPPORT
#ifdef INCLUDE_TEST
    cint_interpreter_add_data(&sbxpkt_cint_data, NULL);
#endif
#endif
#ifdef  BCM_DFE_SUPPORT
    cint_interpreter_add_data(&dfe_cint_data, NULL);
#endif
#ifdef BCM_PETRA_SUPPORT
    cint_interpreter_add_data(&dpp_cint_data, NULL);
#endif
#if defined(BCM_DFE_SUPPORT) || defined (BCM_PETRA_SUPPORT)
    cint_interpreter_add_data(&source_routed_cint_data, NULL);
#endif
    cint_interpreter_add_data(&diag_cint_data, NULL);
#if defined(_ARAD_TEST_SWDB) && defined(BCM_ARAD_SUPPORT) && defined(INCLUDE_CINT)
    cint_interpreter_add_data(&arad_mcdb_cint_data, NULL);
#endif
#ifdef BCM_PETRA_SUPPORT
    cint_interpreter_add_data(&dpp_defines_cint_data, NULL);
    cint_interpreter_add_data(&dpp_implementation_defines_cint_data, NULL);
#endif
#if defined(BCM_EA_SUPPORT)
    cint_interpreter_add_data(&ea_cint_data, NULL);
#endif
#if defined(PHYMOD_SUPPORT)
    cint_interpreter_add_data(&phymod_cint_data, NULL); 
    cint_interpreter_add_data(&phymod_diagnostics_cint_data, NULL);
    cint_interpreter_add_data(&phymod_access_cint_data, NULL);
#endif
#if defined(PHYMOD_TEIR1_SUPPORT)
    cint_interpreter_add_data(&phymod_teir1_cint_data, NULL);
#endif
    cint_interpreter_add_data(&cmd_cint_data, NULL); 
    soc_cint_property_vars_init(); 
    return 0; 
}

static int
__cint_event_handler(void* cookie, cint_interpreter_event_t event)
{
    switch(event)
        {
        case cintEventReset:
            {
                /*
                 * Interpreter has reset. Re-register our data
                 */
                __cint_sdk_data_init(); 
                break;
            }
        default:
            {
                /* Nothing */
                break;
            }   
        }       
    return 0; 
}

static void
__cint_init_path(void)
{
    char *path;
#if defined(VXWORKS)
    path = var_get("CINT_INCLUDE_PATH");
#elif !defined(__KERNEL__)
    path = getenv("CINT_INCLUDE_PATH");
#else
    path = NULL;
#endif

    if (path) {
        cint_interpreter_include_set(path); 
    }
}

/* fatal error interface to lexer - abort to diag shell */
void cmd_cint_fatal_error(char *msg)
{
    sal_printf("%s",msg);
    sh_ctrl_c_take();
}

int 
cmd_cint_initialize(void)
{ 
    static int init = 0; 
    if(init == 0) {
        cint_interpreter_init(); 
        cint_interpreter_event_register(__cint_event_handler, NULL);
        __cint_init_path();
        __cint_sdk_data_init(); 
#ifdef INCLUDE_LIB_C_UNIT
        c_unit_initialize(c_unit_context_set);
#endif
        init = 1; 
    }
    return 0; 
}
                
cmd_result_t
cmd_cint(int unit, args_t* a)
{
    char* s; 
    int cmp;
    int argc = 0; 
    char* argv[16]; 

    cmd_cint_initialize(); 
    
    s = ARG_CUR(a);
    if(s) {
        cmp = sal_strcmp(s, "allow_file_info");
        if(!cmp) {
            argv[argc] = ARG_GET(a);
            argc++;
        }
    }

    if(ARG_CUR(a)) {
        /* Load file */
        FILE* fp; 
        s = ARG_GET(a); 
        
        if((fp = sal_fopen(s, "r")) != NULL) {
                

                
            sal_memset(argv, 0, sizeof(argv)); 
            while( (argv[argc] = ARG_GET(a)) ) {
                argc++; 
            }   

            cint_interpreter_parse(fp, NULL, argc, argv); 
            sal_fclose(fp); 
        }   
        else {
            cli_out("error: could not open file '%s'\n", s); 
        }       
    }   
    else {
        cli_out("Entering C Interpreter. Type 'exit;' to quit.\n\n"); 
        cint_interpreter_parse(NULL, "cint> ", argc, argv); 
    }
    return 0; 
}


#endif /* INCLUDE_LIB_CINT */
