# $Id: Makefile,v 1.26 Broadcom SDK $
# $Copyright: Copyright 2012 Broadcom Corporation.
# This program is the proprietary software of Broadcom Corporation
# and/or its licensors, and may only be used, duplicated, modified
# or distributed pursuant to the terms and conditions of a separate,
# written license agreement executed between you and Broadcom
# (an "Authorized License").  Except as set forth in an Authorized
# License, Broadcom grants no license (express or implied), right
# to use, or waiver of any kind with respect to the Software, and
# Broadcom expressly reserves all rights in and to the Software
# and all intellectual property rights therein.  IF YOU HAVE
# NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE
# IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE
# ALL USE OF THE SOFTWARE.  
#  
# Except as expressly set forth in the Authorized License,
#  
# 1.     This program, including its structure, sequence and organization,
# constitutes the valuable trade secrets of Broadcom, and you shall use
# all reasonable efforts to protect the confidentiality thereof,
# and to use this information only in connection with your use of
# Broadcom integrated circuit products.
#  
# 2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS
# PROVIDED "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
# REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY,
# OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
# DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
# NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
# ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
# CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
# OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
# 
# 3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL
# BROADCOM OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL,
# INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER
# ARISING OUT OF OR IN ANY WAY RELATING TO YOUR USE OF OR INABILITY
# TO USE THE SOFTWARE EVEN IF BROADCOM HAS BEEN ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN EXCESS OF
# THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF OR USD 1.00,
# WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING
# ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.$
# 

include ${SDK}/make/Make.config

export TARGET
export BLDDIR
export LIBDIR

ifdef ROBO_CHIPS
subdirs=diag
endif

ifdef ESW_CHIPS
subdirs += diag
endif

ifdef SBX_CHIPS
subdirs += diag
endif

ifdef DPP_CHIPS
subdirs += diag
subdirs += dpp
subdirs += dcmn
endif

ifdef DFE_CHIPS
subdirs += diag
subdirs += dfe
subdirs += dcmn
endif

ifdef EA_CHIPS
subdirs += diag
endif

ifdef COND_LIB_PIONEER
subdirs += pioneer_host
endif

ifdef COND_LIB_TEST
subdirs += test
endif

ifdef COND_LIB_TCL
subdirs += tcl8.3.3
endif

ifdef COND_LIB_ACL
subdirs += acl
endif

ifdef COND_LIB_CPUDB
subdirs += cpudb
endif

ifdef COND_LIB_CPUTRANS
subdirs += cputrans
endif

ifdef COND_LIB_DISCOVER
subdirs += discover
endif

ifdef COND_LIB_STKTASK
subdirs += stktask
endif

ifdef COND_LIB_AEDEV
subdirs += aedev
endif

ifdef COND_LIB_CINT
subdirs += cint
endif

ifdef COND_LIB_C_UNIT
subdirs += c_unit
endif

ifdef PORTMOD_STAND_ALONE
subdirs += diag
subdirs += portmod
endif

include ${SDK}/make/Make.subdirs
