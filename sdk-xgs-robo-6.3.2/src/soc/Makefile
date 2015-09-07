# $Id: Makefile 1.24 Broadcom SDK $
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
# Makefile for SOC driver Library
#
LOCALDIR = src/soc

CFGFLAGS += -DSOURCE_CHECK_LIBSOC

include ${SDK}/make/Make.config

subdirs = phy common shared

ifdef ESW_CHIPS
subdirs += mcm
else
ifdef SBX_CHIPS
ifdef BCM_88230_A0
subdirs += mcm
endif
ifdef BCM_88230_B0
subdirs += mcm
endif
ifdef BCM_88230_C0
subdirs += mcm
endif
ifdef BCM_88030_A0
subdirs += mcm
endif
endif
endif

ifdef ESW_CHIPS
subdirs += esw
endif

ifdef SBX_CHIPS
subdirs += sbx
endif

ifdef KBP_DEVICE
subdirs += kbp
endif

ifdef DPP_CHIPS
subdirs += dpp
subdirs += mcm
subdirs += dcmn
endif

ifdef DFE_CHIPS
subdirs += dfe
subdirs += mcm
subdirs += dcmn
endif

ifdef ROBO_CHIPS
subdirs += robo
endif

ifdef EA_CHIPS
subdirs += ea
endif

ifdef COND_LIB_SOC_I2C
subdirs += i2c
endif

ifdef BCM_56440_A0
ifdef COND_LIB_CES
subdirs += ces
endif
endif

ifdef FEATURE_FCMAP
subdirs += phy/fcmap/src
endif

include ${SDK}/make/Make.subdirs

lib = libsoc
include ${SDK}/make/Make.lib

include ${SDK}/make/Make.depend
