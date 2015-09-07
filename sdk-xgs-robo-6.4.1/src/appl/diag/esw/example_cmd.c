/*
 * $Id: example_cmd.c,v 1.4 Broadcom SDK $
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

#include <appl/diag/system.h>
#include <appl/diag/parse.h>

#include <appl/applref/applrefIface.h>
#include <bcm/error.h>

#ifdef INCLUDE_EXAMPLES
/** cmd usage */
char cmd_example_exec_usage[] = 
    "Usage: \n\t"
    " example list <category>\n\t"
    " example info cmd\n\t"
    " example exec cmd <param ...>\n\t"
    " example selftest cmd\n";

typedef ApplRefFunction_t * (*_myftype)(void);
/** function pointer, interface with the applreflib */
extern _myftype exampleCmdGetCmdList;

/** dumppest linear search for a command */
static ApplRefFunction_t * findFunc(ApplRefFunction_t *pe, char *pn);

/** return 1 if match previous prefix, otherwise return 0 */
static int match_str(char *pstr, char *pc, int n);

/**
 * execute example_cmd.
 * @ param unit chip unit
 * @ a argument list 
 * @return stat.
 */
cmd_result_t cmd_example_exec(int unit, args_t *a) {
  ApplRefFunction_t * pexpls;
  char *subcmd;
  int rc = 0;

  cli_out("Invoking example cmd\n");

  if ( (subcmd = ARG_GET(a)) == NULL )
    return CMD_USAGE;

  if ( exampleCmdGetCmdList == NULL || 
       (pexpls = (*exampleCmdGetCmdList)()) == NULL ||
       pexpls->pf == NULL ) 
    {
      cli_out("No Examples Available\n");
      return CMD_OK;
    }
  
  if ( sal_strcasecmp(subcmd, "list") == 0 ) {
    ApplRefFunction_t *pe;
    char buf[128];
    buf[0] = 0;

    for( pe = pexpls; pe->pf != NULL; pe++ ) {

      if ( match_str( pe->pname, buf, sizeof(buf)-1 ) )
	cli_out("===================================================================================\n");

      cli_out("  %-25s : %s\n", pe->pname, pe->pbriefdescr);
    }
    return CMD_OK;
  }


  if ( sal_strcasecmp(subcmd, "alllist") == 0 ) {
    ApplRefFunction_t *pe;
    char buf[128];
    buf[0] = 0;

    for( pe = pexpls; pe->pf != NULL; pe++ ) {

      if ( match_str( pe->pname, buf, sizeof(buf)-1 ) )
	cli_out("===================================================================================\n");

      cli_out("  %-25s : %s\n", pe->pname, pe->pbriefdescr);

      cli_out("\n**********************************************\n");
      cli_out("%s\n", pe->pdescr);
      cli_out("\n**********************************************\n");      

    }
    return CMD_OK;
  }


  if ( sal_strcasecmp(subcmd, "info") == 0 ) {
    ApplRefFunction_t *pe;
    
    if ( (subcmd = ARG_GET(a)) == NULL )
      return CMD_USAGE;
    
    pe = findFunc( pexpls, subcmd );
    if ( pe == NULL ) {
      cli_out("Example %s is not found\n", subcmd);
      return CMD_FAIL;
    }

    cli_out("\n**********************************************\n");
    cli_out("%s\n", pe->pdescr);
    cli_out("\n**********************************************\n");
    
    return CMD_OK;
  }

  if ( sal_strcasecmp(subcmd, "exec") == 0 ) {
    ApplRefFunction_t *pe;

    if ( (subcmd = ARG_GET(a)) == NULL )
      return CMD_USAGE;
    
    pe = findFunc( pexpls, subcmd );
    if ( pe == NULL ) {
      cli_out("Example %s is not found\n", subcmd);
      return CMD_FAIL;
    }

    /** invoke command */
    {
      char *args[16]; /** limit to 16 args */
      int n = 0;
      
      while ( (n < 16) && ( (args[n] = ARG_GET(a)) != NULL ) )
	n++;

      if ( n == 16 ) {
	cli_out("Excessive amount of arguments\n");
	return CMD_FAIL;
      }

      if ( BCM_FAILURE(rc = (*pe->dispatch)(pe->pf, n, args)) ) {
	cli_out("Error : fail to execute %d %s\n", rc, bcm_errmsg(rc) );
	return rc;
      }
      	
    }
    
    return CMD_OK;
  }

  return CMD_USAGE;

}


/** change it later to better search algorithm */
static ApplRefFunction_t * findFunc(ApplRefFunction_t *pe, char *pn) {

  if ( pe == NULL )
    return NULL;

  while ( pe->pf != NULL ) {
    if ( sal_strcasecmp( pe->pname, pn ) == 0 )
      return pe;
    pe++;
  }

  return NULL;
}

static int match_str(char *pstr, char *pc, int n) {
  int i, stat;
  
  char *c;
  for( c = pstr; *c != 0; c++ ) 
    if ( *c == '_' )
      break;

  if ( c == 0 ) {
    pc[0] = 0;
    return 1;
  }
  
  stat = 0;
  for(i = 0; i < c - pstr && i < n; i++) {
    if ( pc[i] != pstr[i] ) {
      stat = 1;
      pc[i] = pstr[i];
    }
  }

  if ( pc[i] != 0 || i == n )
    stat = 1;

  pc[i] = 0;

  return stat;
}
#else
int __no_complilation_complain________0;
#endif
