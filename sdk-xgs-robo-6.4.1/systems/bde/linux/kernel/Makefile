# -*- Makefile -*-
# $Id: Makefile,v 1.18 Broadcom SDK $
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
# Makefile for Linux kernel BDE
#
LOCALDIR = systems/bde/linux/kernel

# Make sure we build for the kernel if this is a user-mode build
ifneq ($(platform), )
override-target=linux-$(platform)
endif

include $(SDK)/make/Make.config

LIBS = $(LIBDIR)/libkern.a

BDE = linux-kernel-bde.o

ifdef ROBO_CHIPS
CFLAGS += -I$(ET_ROBO) -I${SDK}/include/shared/et
ET_ROBO = ${SDK}/systems/drv/et
endif

# need to add vpath sources
VPATH = ../shared $(ET_ROBO)

# Add the srcs to be found by vpath
LSRCS += mpool.c
ifdef ROBO_CHIPS
platformsplt = $(subst -, , ${platform})    # change hyphens to spaces
platformbase = $(word 1,${platformsplt})

ifeq ($(platformbase), keystone)
    LSRCS += etc_robo_spi.c aiutils.c
else
    ifeq ($(platformbase), keystone_le)
        LSRCS += etc_robo_spi.c aiutils.c
    else
        ifeq ($(platformbase), iproc)
            LSRCS += robo_srab.c aiutils.c
        endif
    endif
endif # platformbase

endif # ROBO_CHIPS
LHDRS += mpool.h
LOBJS = $(addsuffix .o, $(basename $(LSRCS)))
BOBJS = $(addprefix $(BLDDIR)/,$(LOBJS))

ifneq ($(kernel_version),2_4)
KERNEL_MODULE_DIR = kernel_module

THIS_MOD_NAME := linux-kernel-bde
MODULE = $(LIBDIR)/$(THIS_MOD_NAME).o
KMODULE = $(LIBDIR)/$(THIS_MOD_NAME).ko

build: kernel_libs $(MODULE) $(KMODULE)
else
MODULE = $(LIBDIR)/linux-kernel-bde.o

build: kernel_libs $(MODULE)
endif

$(MODULE): $(BLDDIR)/.tree kernel_libs $(BOBJS)
	mkdir -p $(@D)
	$(LD) $(MODULE_LDFLAGS) -r -d $(BOBJS) $(LIBS) -o $@
ifneq ($(kernel_version),2_4)
$(KMODULE): $(MODULE)
	rm -fr $(BLDDIR)/$(KERNEL_MODULE_DIR)
	mkdir $(BLDDIR)/$(KERNEL_MODULE_DIR)
	cp ${SDK}/make/Makefile.linux-kmodule $(BLDDIR)/$(KERNEL_MODULE_DIR)/Makefile
	MOD_NAME=$(THIS_MOD_NAME) $(MAKE) -C $(BLDDIR)/$(KERNEL_MODULE_DIR) $(THIS_MOD_NAME).ko
endif

kernel_libs:
	$(MAKE) -C $(SDK)/systems/linux/kernel/modules/shared

include $(SDK)/make/Make.depend

# Make.depend is before clean:: so that Make.depend's clean:: runs first.

clean::
	$(MAKE) -C $(SDK)/systems/linux/kernel/modules/shared $@
	$(RM) $(BOBJS) $(MODULE)
	$(RM) $(BLDDIR)/$(KERNEL_MODULE_DIR)

distclean::

ifneq ($(kernel_version),2_4)
.PHONY: build kernel_libs
endif
