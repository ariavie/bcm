# $Id: Makefile 1.28 Broadcom SDK $
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

LOCALDIR = systems/sim/pcid

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

include ${SDK}/make/Make.config

ifeq ($(INSTALL_DIR),)
INSTALL_DIR=.
endif

ifdef DPP_CHIPS
LOCALDIR=systems/sim/pcid
subdirs := ../dpp/ChipSim
include ${SDK}/make/Make.subdirs
endif

ifdef DFE_CHIPS
LOCALDIR=systems/sim/pcid
subdirs := ../dpp/ChipSim
include ${SDK}/make/Make.subdirs
endif

#
# To use Purify, add a line here similar to the following:
#
#CC=/net/igor/export/home/pure/purify-4.2-solaris2/purify gcc

#
# Define all targets to build
#

PCID = ${INSTALL_DIR}/pcid.${targetplat}${all_suffix}
PCID_LIB = ${LIBDIR}/libpcid.a

all_targets = ${PCID} ${PCID_LIB}

#
# Define targets to be built by default.
#
build_targets = ${all_targets}

#
# Libraries
#
#DIAG_LIBS = \
#	${COND_LIB_EDITLINE} \
#	libsal_appl.a libsal_appl_plat.a libplibde.a





PCID_LIBS = ${EXTRA_TEST_LIBS} ${DRV_LIBS} ${DIAG_LIBS} ${APPL_LIBS}
PCID_LIBS_BLD = $(addprefix ${BLDROOT}/,${PCID_LIBS})

ifdef DPP_CHIPS
PCID_LIB_CHIPSIM =  ${LIBDIR}/libchipsim_sim.a
PCID_LIBS_BLD += ${PCID_LIB_CHIPSIM} 
endif

ifdef DPP_DFE
PCID_LIB_CHIPSIM =  ${LIBDIR}/libchipsim_sim.a
PCID_LIBS_BLD += ${PCID_LIB_CHIPSIM} 
endif

#ifdef BUILD_MACSEC
PCID_LIBS_BLD += ${EXTERNAL_LIBS}
#endif

export IPROC_NO_ATL = 1
CFGFLAGS += -DIPROC_NO_ATL

all:: ${BLDDIR}/.tree ${build_targets}



# Libraries

.PHONY: _bcm_libraries

_bcm_libraries:
ifndef NO_BCM
	$(MAKE) -C ${SDK}
endif

# Broadcom Command Monitor

# Build platform_defines.c to compile in config.bcm
#
ifndef NO_SAL_APPL
PLATFORM_DEFINES_OBJ=${BLDDIR}/platform_defines.o
${BLDDIR}/platform_defines.c: $(BLDDIR)/.tree ${SDK}/rc/config.bcm
	$(PERL) ${SDK}/tools/bcm2c_linux.pl ${SDK}/rc/config.bcm >$@
${BLDDIR}/platform_defines.o: ${BLDDIR}/platform_defines.c
	${CC} ${CFLAGS} -c $< -o $@
ifndef NO_VERSION
	touch ${SDK}/rc/config.bcm
endif
else
PLATFORM_DEFINES_OBJ=
endif

# PCI Daemon (chip simulator)

${PCID}: _bde _bcm_libraries ${BOBJS} ${BLDDIR}/version.o \
         ${PLATFORM_DEFINES_OBJ}
	@rm -f ${PCID}	# Prevent core dump if currently running
	${CC} -o ${PCID} ${BOBJS} ${BLDDIR}/version.o \
	${PLATFORM_DEFINES_OBJ} \
        -Wl,--start-group ${PCID_LIBS_BLD} -Wl,--end-group ${LDFLAGS}

# This list has the .c files except for chipsim.c and pcidappl.c
# Update it when new files are added so the AV simulator works.
PCID_SRCS = \
	datum.c cmicsim.c pli.c util.c dma.c  physim.c\
	ism.c \
	l2x.c l3x.c socintf.c proc_req.c packet.c vlan.c \
    mpls.c
PCID_OBJS = $(addsuffix .o, $(basename ${PCID_SRCS}))
PCID_BOBJS = $(addprefix ${BLDDIR}/,${PCID_OBJS})

${PCID_LIB}: ${PCID_BOBJS}
	${RM} $@
	$(AR) ${ARFLAGS} $@ $(sort ${PCID_BOBJS})

$(BLDDIR)/%.P: $(BLDDIR)/.tree
	$(info $@)

# Build Version

${BLDDIR}/version.c: ${BLDDIR}/.tree
	$(MKVERS) > $@

${BLDDIR}/version.o: ${BLDDIR}/version.c
	${CC} ${CFLAGS} -c $< -o $@

# Make.depend is before clean:: so that Make.depend's clean:: runs first.

include ${SDK}/make/Make.depend

# Clean

clean:: ${BLDDIR}/.tree 
	rm -f ${all_targets}
	rm -f ${BOBJS}
	rm -f ${LIBDIR}/libpcid.a
	rm -f ${BLDDIR}/version.c ${BLDDIR}/version.o
	rm -f ${BLDDIR}/platform_defines.c ${BLDDIR}/platform_defines.o
ifndef NO_BCM
	$(MAKE) -C ${SDK} clean
endif
	$(MAKE) -C ${SDK}/systems/bde/pli clean
	@# Rather than just removing the build directory, the following
	@# will detect any file missed by 'clean' so it can be fixed.
	@-find ${BLDROOT} -depth \
		\( -name .tree -exec rm {} \; \) -o \
		\( -type d -exec rmdir {} \; \) -o \
		-exec echo WARNING: make clean did not remove {} \; \
		-exec rm -f {} \;

distclean:: clean

.PHONY: _bde
_bde:
ifndef NO_BDE
	$(MAKE) -C ${SDK}/systems/bde/pli
endif
