#
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
# $Id: Makefile,v 1.11 Broadcom SDK $
ifdef SDK
include ${SDK}/make/Make.config
endif

ifdef BCMSIM_ROOT
bcmsbxlcm_lib=${BCMSIM_ROOT}/lib/libbcmsbxlcm_c2.a
endif

ifdef BCMSBXLCM_LIB
bcmsbxlcm_lib=${BCMSBXLCM_LIB}
endif

sdk_env_check:
ifndef SDK
	@echo "SDK environment variable not set. "
	exit 1
endif

lcm_env_check:
ifndef BCMSBXLCM_LIB
ifndef BCMSIM_ROOT
	@echo "BCMSIM_ROOT environment variable not set "
	exit 1
endif
endif

BCMSIM_DIAG_LIBS = \
	${COND_LIB_EDITLINE} \
	libdiag.$(libext) \
	${ROBO_DIAG_LIBS} \
	${ESW_DIAG_LIBS} \
	${SBX_DIAG_LIBS} \
	${COND_LIB_BCMX_DIAG} \
	${COND_LIB_TEST} \
	${COND_LIB_TCL}  \
	$(COND_LIB_SAL_APPL) \
	$(COND_LIB_SAL_APPL_PLATFORM)
    
BCM_LIBS = ${APPL_LIBS} ${BCMSIM_DIAG_LIBS} ${DRV_LIBS}
BCM_LIBS_BLD = $(addprefix ${LIBDIR}/,${BCM_LIBS})

pcid: sdk_env_check
	$(MAKE) -C ${SDK}/systems/sim

sbxlcm: lcm_env_check pcid
	${CXX} -o bcmsbxlcm_bcmsim.linux ${BLDDIR}/systems/sim/version.o \
		${BLDDIR}/systems/sim/platform_defines.o \
		${bcmsbxlcm_lib}\
		${BCM_LIBS_BLD} ${BCM_LIBS_BLD} ${BCM_LIBS_BLD} ${LDFLAGS} -lstdc++

help:
	@echo " Before doing a make, "
	@echo " Set BCMSIM_ROOT env variable to use the prebuilt LCM library (BCMSIM_ROOT/lib/libbcmsbxlcm_g2p2.a) "
	@echo " Or set BCMSBXLCM_LIB env variable to point to the LCM library you built "

all: pcid
