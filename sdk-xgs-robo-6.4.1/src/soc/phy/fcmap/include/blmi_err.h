/*
 * $Id: blmi_err.h,v 1.1 Broadcom SDK $
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

#ifndef BLMI_ERR_H
#define BLMI_ERR_H

typedef enum {
    BLMI_E_NONE		=  0,
    BLMI_E_INTERNAL		= -1,
    BLMI_E_MEMORY		= -2,
    BLMI_E_PARAM		= -3,
    BLMI_E_EMPTY             = -4,
    BLMI_E_FULL		= -5,
    BLMI_E_NOT_FOUND		= -6,
    BLMI_E_EXISTS		= -7,
    BLMI_E_TIMEOUT		= -8,
    BLMI_E_FAIL		= -9, 
    BLMI_E_DISABLED		= -10,
    BLMI_E_BADID		= -11,
    BLMI_E_RESOURCE		= -12,
    BLMI_E_CONFIG		= -13,
    BLMI_E_UNAVAIL		= -14,
    BLMI_E_INIT		= -15,
    BLMI_E_PORT		= -16,
    BLMI_E_UNKNOWN           = -17
} blmi_error_t;

#define	BLMI_ERRMSG_INIT	{ \
	"Ok",				/* E_NONE */ \
	"Internal error",		/* E_INTERNAL */ \
	"Out of memory",		/* E_MEMORY */ \
	"Invalid parameter",		/* E_PARAM */ \
	"Table empty",			/* E_EMPTY */ \
	"Table full",			/* E_FULL */ \
	"Entry not found",		/* E_NOT_FOUND */ \
	"Entry exists",			/* E_EXISTS */ \
	"Operation timed out",		/* E_TIMEOUT */ \
	"Operation failed",		/* E_FAIL */ \
	"Operation disabled",		/* E_DISABLED */ \
	"Invalid identifier",		/* E_BADID */ \
	"No resources for operation",	/* E_RESOURCE */ \
	"Invalid configuration",	/* E_CONFIG */ \
	"Feature unavailable",		/* E_UNAVAIL */ \
	"Feature not initialized",	/* E_INIT */ \
	"Invalid port",			/* E_PORT */ \
	"Unknown error",		/* E_UNKNOWN*/ \
	}

extern char *blmi_errmsg[];

#define	BLMI_ERRMSG(r)		\
	blmi_errmsg[((r) <= 0 && (r) > BLMI_E_UNKNOWN) ? -(r) : -BLMI_E_UNKNOWN]

#define BLMI_E_SUCCESS(rv)		((rv) >= 0)
#define BLMI_E_FAILURE(rv)		((rv) < 0)

/*
 * Macro:
 *	BLMI_E_IF_ERROR_RETURN
 * Purpose:
 *	Evaluate _op as an expression, and if an error, return.
 * Notes:
 *	This macro uses a do-while construct to maintain expected
 *	"C" blocking, and evaluates "op" ONLY ONCE so it may be
 *	a function call that has side affects.
 */

#define BLMI_E_IF_ERROR_RETURN(op) \
    do { int __rv__; if ((__rv__ = (op)) < 0) return(__rv__); } while(0)

#define BLMI_E_IF_ERROR_NOT_UNAVAIL_RETURN(op)                        \
    do {                                                                \
        int __rv__;                                                     \
        if (((__rv__ = (op)) < 0) && (__rv__ != BLMI_E_UNAVAIL)) {      \
            return(__rv__);                                             \
        }                                                               \
    } while(0)

#endif /* BLMI_ERR_H */


