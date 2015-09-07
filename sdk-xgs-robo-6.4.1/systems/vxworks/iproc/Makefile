# $Id: Makefile,v 1.20 Broadcom SDK $
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
# Makefile for VxWorks PPC BSP with BCM driver library
# VxWorks configuration variables
#
# Makefile include architecture:
#    sdk/systems/vxworks/iproc/Makefile, 
#        - include sdk/make/Make.config      (chip definition and SDK compile)
#            - include sdk/make/Make.local     (SDK compile include definition)
#            - include sdk/make/Make.tools     (SYSNAME = Linux, define unix commands, using Pearl to generate some files like bcm2c, version)
#            - include sdk/make/Makefile.vxworks-iproc   (Vxworks path definition, config definition, define vxworks commands)
#                - include sdk/make/Makefile.vxworks-common
#            - include sdk/make/Make.boards    (BCM board level definition)
#            - include sdk/make/Make.targets   (define path/files for KERNFLAGS)
#        - include sdk/make/Make.vxworks     (SDK module compile routines like bde, bcm_libraries, driver_libraries)
#    <SDK compile> is in $(DEST_DIR)/${VX_BINARY}: ${IMAGE_DEPENDENCY} bde bcm_libraries driver_libraries.....
#          in bde module, it will change to sdk/systems/bde/vxworks and execute Makefile.
#          in bcm_libraries module, it will change to sdk and execute Makefile.
#          in driver libraries module, it will change to sdk/systems/drv and execute Makefile.
#        Usually the architecture in those Makefile(s) are :
#              - include sdk/make/Make.config (it will include other files like above)
#              - define "subdirs" to tell which sub-directories under current location should be compiled
#              - include sdk/make/Make.subdirs 
#                    it will jump to those sub-directories and run the Makefile.... (recursive)
#              - include sdk/make/Make.depend
#    <Vxworks compile> is in $(MAKE) -C ${SRC_DIR} WIND_BASE="${WIND_BASE}"......
#          it will change directory to $(SRC_DIR) = sdk/systems/vxworks/iproc/vxworks6x/bcm56450 and run Makefile.
#    Combine SDK and Vxworks after "${VX_BINARY} vxWorks.st vxWorks.st.arm".........

VX_VERSIONS :=68

SDK :=$(shell if [ -n "$$SDK" ] ; then\
	echo $$SDK;\
	else\
	cd $(dir $(lastword $(MAKEFILE_LIST))); while /usr/bin/test ! -e RELEASE ; do \
	dir=`cd ../;pwd`;			\
	if [ "$$dir" = "/" ] ; then		\
	   echo Cannot find SDK in $(lastword $(MAKEFILE_LIST)) 1>&2;		\
	   exit 1;				\
	fi ;					\
	cd $$dir;				\
	done ;					\
	pwd;					\
	fi)

ifeq ($(SDK),)					
$(error Please run this in a tree)		
endif 				

export SDK

override-target=vxworks-iproc

LOCALDIR = systems/vxworks/iproc
LOCAL_CLEAN = bsplist_clean

# default is Big endian
#export VX_TARGET_BIG_ENDIAN ?= 0
#ifeq ($(VX_TARGET_BIG_ENDIAN),1)
#export bldroot_suffix=be
#else
#export bldroot_suffix=le
#endif

include ${SDK}/make/Make.config
include ${SDK}/make/Make.vxworks

#
# This optional override builds a binary that loads the symbol table
# over the network instead of having it compiled in.
#
#override VX_TARGET = vxWorks

#
# Point to the VxWorks BSP source directory
#
SRC_DIR=${SDK}/systems/vxworks/iproc/vxworks6x/bcm56450

IMAGE_DEPENDENCY = ImageBanner

#
# Define rule to build ${VX_BINARY}
#

# stripppc.exe is broken in Cygnus
$(DEST_DIR)/${VX_BINARY}: ${IMAGE_DEPENDENCY} bde bcm_libraries driver_libraries  \
		${BLDDIR}/.tree ${BLDDIR}/socdiag.o ${BLDDIR}/version.o\
		${BLDDIR}/platform_defines.o ${BLDDIR}/hal.o ${SRC_DIR}/${VX_TARGET}
	mkdir -p $(@D) 
	${CP} ${SRC_DIR}/${VX_TARGET} $@ 
	${CP} ${SRC_DIR}/${VX_TARGET} $@.sym 
ifneq ($(OS),Windows_Cygnus)
	${STRIP} $@ 
	${STRIPFIX} ${SRC_DIR}/${VX_TARGET} $@ 56 5 
endif


# Following section will change directory to SRC_DIRi first,
#  (SRC_DIR = systems/vxworks/iproc/vxworks6x/bcm56450) 
# then execute the Makefile.

.PHONY : .FORCE

${SRC_DIR}/${VX_TARGET} : .FORCE
	$(MAKE) -C ${SRC_DIR} WIND_BASE="${WIND_BASE}" \
		VX_VERSION="${VX_VERSION}"\
		WIND_BASE="${WIND_BASE}" \
		MAKEFLAGS="${MAKEFLAGS}" ${KERNFLAGS} ${VX_TARGET}

# \todo Remove below once tested
${VX_BINARY} vxWorks.st vxWorks.st.arm: \
		${IMAGE_DEPENDENCY} bde bcm_libraries driver_libraries \
		${BLDDIR}/.tree ${BLDDIR}/socdiag.o ${BLDDIR}/version.o\
		${BLDDIR}/platform_defines.o ${BLDDIR}/hal.o
#	$(MAKE) -C ${SRC_DIR} WIND_BASE="${WIND_BASE}" \
		VX_VERSION="${VX_VERSION}"\
		WIND_BASE="${WIND_BASE}" \
		MAKEFLAGS="${MAKEFLAGS}" ${KERNFLAGS} ${VX_TARGET}
	${CP} ${SRC_DIR}/${VX_TARGET} ${VX_BINARY}.sym
	${CP} ${SRC_DIR}/${VX_TARGET} ${VX_BINARY}
	${STRIP} ${VX_BINARY}
	${STRIPFIX} ${SRC_DIR}/${VX_TARGET} ${VX_BINARY} 56 5
	${MKBEEP}

ImageBanner:
	echo "****	Building KT2(bcm56450) image (VxWorks ${VXWORKS_VERSION}) (Make Version ${MAKE_VERSION})  ****"
