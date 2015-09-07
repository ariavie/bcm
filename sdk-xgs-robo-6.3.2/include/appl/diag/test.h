/*
 * $Id: test.h 1.84 Broadcom SDK $
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
 * File:        test.h
 * Purpose:     Defines and types required for building test lists.
 */

#ifndef _DIAG_TEST_H
#define _DIAG_TEST_H

#include <soc/types.h>

#include <appl/diag/parse.h>

/* File loaded when RC initialization required */

typedef int     (*test_ifunc_t)(int, args_t *, void **);
typedef int     (*test_dfunc_t)(int, void *);
typedef int     (*test_func_t)(int, args_t *, void *);

typedef struct test_s {
    char        *t_name;                 /* Test Name */
    uint32      t_flags;                 /* Flags for test */
#   define      T_F_SEL_ALL     (1 << 0) /* Test selected for any chip */
#   define      T_F_SEL_CHIP    (1 << 1) /* Test selected for supported chip */
#   define      T_F_SELECT      (T_F_SEL_ALL | T_F_SEL_CHIP)
#   define      T_F_ACTIVE      (1 << 2) /* Currently Active */
#   define      T_F_STOP        (1 << 3) /* Stop-on-error processed */
#   define      T_F_ERROR       (1 << 4) /* Error reported this run */
#   define      T_F_RC          (1 << 5) /* Std init script should be run */

/* Following flags are used in testlist, but are part of flags */

#   define      TSEL            T_F_SEL_CHIP

/*#if defined (BCM_DFE_SUPPORT)|| defined (BCM_PETRA_SUPPORT)*/
#   define      T88650            (1 << 20)
#   define      T88750            (1 << 31)
#   define      DPP_ALL           (T88650)
#   define      DFE_ALL           (T88750)
#   define      DPP_DFE           (DPP_ALL | DFE_ALL)
/*#endif*/

#ifdef BCM_FE2000_SUPPORT
#   define      T5324            (0)
#   define      T88025           (1 << 15)
#else
#   define      T5324            (1 << 15)
#   define      T88025           (0)
#endif
#   define      T5396           (1 << 14)
#   define      T5389           (1 << 13)
#   define      T5398           (1 << 12)
#   define      T5397           (T5398)
#   define      T5348           (1 << 11)
#   define      T5347           (T5348)
#   define      T5395           (1 << 10)
#   define      T88100           (1 << 6)  /* SBX BME-3200 */
#   define      T88230           (1 << 7)  /* SIRIUS */
#   define      T88030           (1 << 17) /* Caladan3  */
#   define      T88020           (1 << 16) /* SBX FE-2000  */
#   define      T53242           (1 << 9)
#   define      T53115           (1 << 8)
#   define      T53118           (T53115)
#   define      T53101           (T53115)
#   define      T53125           (T53115)
#   define      T53128           (T53115)
#   define      T89500           (T53115)
#   define      T53010           (T53115)
#   define      T53020           (T53115)
#   define      T53262           (T53242)
#   define      T53280           (T53242)
#   define      TROBODINO         (T5389 | T5396)
#   define      TROBO            (T5324 | T5396 | T5389 | T5398 |T5348 | T5395 | T53242 | T53115)
#   define      T56850          (1 << 18)
#   define      T5675           (1 << 19)
#   define      T88200          (1 << 22)  /* SBX QE-2000  */
#   define      T88130          (1 << 23) /* SBX BME-9600 */
#   define      T56504          (1 << 24)
#   define      T56218          (1 << 25)
#   define      T56624          (1 << 26)
#   define      T56634          (T56624)
#   define      T56334          (T56624)
#   define      T56142          (T56624)
#   define      T56150          (T56142)
#   define      T56840          (T56624)
#   define      T56640          (1 << 21) /* reusing a depricated device id */
#   define      T56340          (T56640)  /* Helix4 - replicate TR3 case for now */
#   define      T56224          (1 << 27)
#   define      T56700          (1 << 28)
#   define      T56820          (1 << 29)
#   define      T88732          (T56820)
#   define      T53314          (1 << 30)
#   define      T56440          (T56624)
#   define      T56450          (T56440)

#   define      TXGS3SW         (T56504 | T56218 | T56224 | T56624 | \
                                 T56700 | T53314 | T56820 | T56640 | \
                                 T56850)
#   define      TXGS12SW        (T5675)
#   define      TXGSSW          (TXGS12SW | TXGS3SW)
#   define      TSBX            (T88020 | T88100 | T88200 | T88025 )
#   define      TALL            (TXGSSW)

#   define      TRC             (T_F_RC)

    int         t_test;                 /* Test Number */
    int         t_loops;                /* Default # iterations */
    test_ifunc_t t_init_f;              /* Initialization routine */
    test_dfunc_t t_done_f;              /* Completion cleanup */
    test_func_t t_test_f;               /* Test function */

    char        *t_default_string;      /* Possible init string */
    char        *t_override_string;     /* Possible override init string */

    /* Counters of run statistics */

    int         t_runs;                 /* Total Number of runs */
    int         t_success;              /* # sucessful completions */
    int         t_fail;                 /* # failures */
} test_t;

/*
 * Macro:       TEST
 * Purpose:     Macro to help build a test entry.
 * Parameters:  _nu - (integer) Test Number
 *                      If negative, test is not selected by default
 *              _na - (string) Test Name
 *              _f  - (integer) Test Flags (chips and RC)
 *              _fi - (function) Initialization function.
 *              _fd - (function) Termination function.
 *              _ft - (function) Actual test Function
 *              _l  - (integer) loop count, # times test
 *                      function called in one run.
 *              _arg- (string) Default argument string.
 */
#define TEST_DECL(_nu, _na, _flg, _fi, _fd, _ft, _l, _arg)      \
    {(_na), (_flg), (_nu), (_l), (_fi), \
     (_fd), (_ft), (_arg), 0, 0, 0, 0}

#define TEST(_nu, _na, _f, _fi, _fd, _ft, _l, _arg)     \
    TEST_DECL((_nu), (_na) , (_f), (_fi), (_fd), (_ft), (_l), (_arg)),

# define TUNIMP(_nu, _na, _f, _fi, _fd, _ft, _l, _arg)


extern void     test_exit(int u, int status);
extern void     test_error(int u, const char *fmt, ...)  /* May not return */
     COMPILER_ATTRIBUTE ((format (printf, 2, 3)));
extern void     test_msg(const char *fmt, ...)
     COMPILER_ATTRIBUTE ((format (printf, 1, 2)));
extern int      test_get_last_test_status(int unit);
extern int      test_is_no_reinit_mode(int unit);

/* testlist.c */

extern  test_t  test_list[];
extern  int     test_cnt;

#endif /* _DIAG_TEST_H */
