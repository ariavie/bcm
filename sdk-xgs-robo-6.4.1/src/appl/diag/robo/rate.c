/*
 * $Id: rate.c,v 1.6 Broadcom SDK $
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

#include <shared/bsl.h>

#include <appl/diag/system.h>
#include <appl/diag/parse.h>

#include <bcm/init.h>
#include <bcm/error.h>
#include <bcm/rate.h>
#include <bcm/debug.h>

#define _DFT_MAGIC_NUMBER -123456798

/*
 * Manage packet rate controls
 */

cmd_result_t
cmd_robo_rate(int unit, args_t *a)
{
    int limit = _DFT_MAGIC_NUMBER; /* magic number */
    int	fRateBCast;
    int fRateMCast;
    int fRateDLF;
    int fmask = 0;
    int flags = 0;
    int r;
    parse_table_t pt;
    pbmp_t pbmp;
    int limitGet = FALSE;
    int bRateSet, mRateSet,dlfRateSet;
    int bRateFlGet, mRateFlGet, dlfRateFlGet;
    int bRateLmGet, mRateLmGet, dlfRateLmGet;
    int displayOnly = 0;
    soc_port_t p;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
        return CMD_FAIL;
    }

    pbmp       = PBMP_E_ALL(unit);  /* All port by default */
    limit      = _DFT_MAGIC_NUMBER;
    fRateBCast = _DFT_MAGIC_NUMBER;
    fRateMCast = _DFT_MAGIC_NUMBER;
    fRateDLF   = _DFT_MAGIC_NUMBER;
    
    if (ARG_CNT(a)) {
        parse_table_init(unit, &pt);
        parse_table_add(&pt, "PortBitMap", PQ_DFL|PQ_PBMP,
                    (void *)0, &pbmp, 0);
        parse_table_add(&pt, "Limit", PQ_DFL|PQ_HEX,
                    INT_TO_PTR(limit) , &limit, 0);    
        parse_table_add(&pt, "Bcast", PQ_DFL|PQ_BOOL,
                    (void *)0, &fRateBCast, 0);
        parse_table_add(&pt, "Mcast",        PQ_BOOL,
                    INT_TO_PTR(fRateMCast), &fRateMCast, 0);
        parse_table_add(&pt, "Dlf", PQ_DFL|PQ_BOOL,
                    INT_TO_PTR(fRateDLF), &fRateDLF, 0);

        if (parse_arg_eq(a, &pt) < 0) {
            cli_out("%s: Error: Unknown option: %s\n", ARG_CMD(a), ARG_CUR(a));
            parse_arg_eq_done(&pt);
            return(CMD_FAIL);
        }
        parse_arg_eq_done(&pt);
        fmask = flags = 0;   

        /* Verify what is being changed */
        if (fRateBCast != _DFT_MAGIC_NUMBER) {
            fmask |= BCM_RATE_BCAST;
            if (fRateBCast == TRUE) {
                flags |= BCM_RATE_BCAST;
            }
        }
        if (fRateMCast != _DFT_MAGIC_NUMBER) {
            fmask |= BCM_RATE_MCAST;
            if (fRateMCast == TRUE) {
                flags |= BCM_RATE_MCAST;
            }
        }
        if (fRateDLF != _DFT_MAGIC_NUMBER) {
            fmask |= BCM_RATE_DLF;
            if (fRateDLF == TRUE) {
                flags |= BCM_RATE_DLF;
            }
        }
        if (limit < 0 && limit != _DFT_MAGIC_NUMBER) {
            cli_out("Error: Negative rate limit: %d is not " 
                    "allowed\n", (int)limit);
        }

        /* Check whether original value shall be read back */ 
        if (fmask == 0 && limit == _DFT_MAGIC_NUMBER) {
            /* only PBM is specified */
            displayOnly = TRUE;
        } else if (limit == _DFT_MAGIC_NUMBER) {
            limitGet = TRUE;
        }
    
    } else {
        displayOnly = TRUE;
    }

    if (displayOnly) {
        cli_out("Current settings:\n");

        PBMP_ITER(pbmp, p) {
            cli_out("%4s:",SOC_PORT_NAME(unit, p));
            flags = 0;
            r = bcm_rate_bcast_get(unit, &limit, &flags, p);
            cli_out(" Limit=%d,", limit);
            if (r < 0) {
                cli_out(" Bcast = UNKNOWN, %s;", bcm_errmsg(r));
            } else if (flags & BCM_RATE_BCAST) {
                cli_out(" Bcast= TRUE,");
            } else {
                cli_out(" Bcast=FALSE,");
            }
            if ((r = bcm_rate_mcast_get(unit, &limit, &flags, p)) < 0) {
                cli_out(" Mcast=UNKNOWN, %s;", bcm_errmsg(r));
            } else if (flags & BCM_RATE_MCAST) {
                cli_out(" Mcast= TRUE,");
            } else {
                cli_out(" Mcast=FALSE,");
            }
            if ((r = bcm_rate_dlfbc_get(unit, &limit, &flags, p)) < 0) {
                cli_out(" Dlf=UNKNOWN, %s;", bcm_errmsg(r));
            } else if (flags & BCM_RATE_DLF) {
                cli_out(" Dlf= TRUE");
            } else {
                cli_out(" Dlf=FALSE");
            }
            cli_out("\n");
        }
        return(CMD_OK);
    }

    bRateSet   = mRateSet   = dlfRateSet   = FALSE;
    bRateFlGet = mRateFlGet = dlfRateFlGet = FALSE;
    bRateLmGet = mRateLmGet = dlfRateLmGet = FALSE;
    fRateBCast = fRateMCast = fRateDLF     = 0;

    if (!fmask) { 
        if (limitGet == FALSE) {
            /* only new limit specified */
            bRateSet   = mRateSet   = dlfRateSet   = TRUE;
            bRateFlGet = mRateFlGet = dlfRateFlGet = TRUE;
        }
    } else {
        if ((fmask & BCM_RATE_BCAST)) {
            bRateSet = TRUE;
            fRateBCast = flags & BCM_RATE_BCAST;
            if (fRateBCast && limitGet) {
                bRateLmGet = TRUE;
            }
        } 
        if ((fmask & BCM_RATE_MCAST)) {
            mRateSet = TRUE;
            fRateMCast = flags & BCM_RATE_MCAST;
            if (fRateMCast && limitGet) {
                mRateLmGet = TRUE;
            }
        } 
        if ((fmask & BCM_RATE_DLF)) {
            dlfRateSet = TRUE;
            fRateDLF = flags & BCM_RATE_DLF;
            if (fRateDLF && limitGet) {
                dlfRateLmGet = TRUE;
            }
        } 
    }

    /* Set new config in ports */
    PBMP_ITER(pbmp, p) {
        /* Brocast meter */
        if (bRateSet) {
            int limitPro, flagPro;
            int oriLimit, oriFlag;
            int r = 0;
    
            if (bRateFlGet || bRateLmGet) {
                if ((r = bcm_rate_bcast_get(unit, &oriLimit, &oriFlag, p)) < 0) {
                    cli_out("%4s Error: can not get Brocast limit info: %s\n", 
                            SOC_PORT_NAME(unit, p), bcm_errmsg(r));
                }
            }

            limitPro = (!r && bRateLmGet)? oriLimit : limit;
            flagPro  = (!r && bRateFlGet)? oriFlag  : fRateBCast;

            if (!r && (r = bcm_rate_bcast_set(unit, 
                                        limitPro, flagPro, p)) < 0) {
                cli_out("%4s Error: can not set Broadcast rate: %s\n", 
                        SOC_PORT_NAME(unit, p), bcm_errmsg(r));
            }
        }
        /* Mcast meter control */
        if (mRateSet) {
            int limitPro, flagPro;
            int oriLimit, oriFlag;
            int r = 0;

            if (mRateFlGet || mRateLmGet) {
                if ((r = bcm_rate_mcast_get(unit, &oriLimit, &oriFlag, p)) < 0) {
                    cli_out("%4s Error: can not get Mcast limit info: %s\n", 
                            SOC_PORT_NAME(unit, p), bcm_errmsg(r));
                }
             }

             limitPro = (!r && mRateLmGet)? oriLimit : limit;
             flagPro  = (!r && mRateFlGet)? oriFlag  : fRateMCast;

             if (!r && (r = bcm_rate_mcast_set(unit, 
                                        limitPro, flagPro, p)) < 0) {
                 cli_out("%4s Error: can not set Mcast rate: %s\n", 
                         SOC_PORT_NAME(unit, p), bcm_errmsg(r));
             }
        }
        /* Dlf meter */
        if (dlfRateSet) {
            int limitPro, flagPro;
            int oriLimit, oriFlag;
            int r = 0;

            if (dlfRateFlGet || dlfRateLmGet) {
                if ((r = bcm_rate_dlfbc_get(unit, &oriLimit, &oriFlag, p)) < 0) {
                    cli_out("%4s Error: can not get DLF limit info: %s\n", 
                            SOC_PORT_NAME(unit, p), bcm_errmsg(r));
                }
             }

             limitPro = (!r && dlfRateLmGet)? oriLimit : limit;
             flagPro  = (!r && dlfRateFlGet)? oriFlag  : fRateDLF;

             if (!r && (r = bcm_rate_dlfbc_set(unit, 
                                        limitPro, flagPro, p)) < 0) {
                 cli_out("%4s Error: can not set DLF rate: %s\n", 
                         SOC_PORT_NAME(unit, p), bcm_errmsg(r));
             }
        }
    }
    return CMD_OK;
}

