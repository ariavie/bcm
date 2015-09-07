# $Id: Makefile 1.113 Broadcom SDK $
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
# Makefile for BCM driver Library
#

LOCALDIR = src/bcm/esw

export BCM_HIDE_DISPATCHABLE=1

include ${SDK}/make/Make.config

ifdef BCM_5675_A0
SUBDIR_HERC = hercules
endif

ifdef BCM_56504_A0
SUBDIR_FIREBOLT = firebolt
endif

ifdef BCM_56504_B0
SUBDIR_FIREBOLT = firebolt
endif

ifdef BCM_56102_A0
SUBDIR_FIREBOLT = firebolt
endif

ifdef BCM_56304_B0
SUBDIR_FIREBOLT = firebolt
endif

ifdef BCM_56112_A0
SUBDIR_FIREBOLT = firebolt
endif

ifdef BCM_56314_A0
SUBDIR_FIREBOLT = firebolt
endif

ifdef BCM_56580_A0
SUBDIR_FIREBOLT = firebolt
SUBDIR_BRADLEY = bradley
SUBDIR_HERC = hercules
SUBDIR_HUMV = humv
endif

ifdef BCM_56700_A0
SUBDIR_FIREBOLT = firebolt
SUBDIR_BRADLEY = bradley
SUBDIR_HERC = hercules
SUBDIR_HUMV = humv
endif

ifdef BCM_56800_A0
SUBDIR_FIREBOLT = firebolt
SUBDIR_BRADLEY = bradley
SUBDIR_HERC = hercules
SUBDIR_HUMV = humv
endif

ifdef BCM_56218_A0
SUBDIR_RAPTOR = raptor
SUBDIR_FIREBOLT = firebolt
endif

ifdef BCM_56514_A0
SUBDIR_FIREBOLT = firebolt
endif

ifdef BCM_56624_A0
SUBDIR_TRX = trx
SUBDIR_TRIUMPH = triumph
SUBDIR_BRADLEY = bradley
SUBDIR_FIREBOLT = firebolt
endif

ifdef BCM_56680_A0
SUBDIR_TRX = trx
SUBDIR_TRIUMPH = triumph
SUBDIR_BRADLEY = bradley
SUBDIR_FIREBOLT = firebolt
endif

ifdef BCM_56624_B0
SUBDIR_TRX = trx
SUBDIR_TRIUMPH = triumph
SUBDIR_BRADLEY = bradley
SUBDIR_FIREBOLT = firebolt
endif

ifdef BCM_56680_B0
SUBDIR_TRX = trx
SUBDIR_TRIUMPH = triumph
SUBDIR_BRADLEY = bradley
SUBDIR_FIREBOLT = firebolt
endif

ifdef BCM_56224_A0
SUBDIR_RAPTOR = raptor
SUBDIR_FIREBOLT = firebolt
endif

ifdef BCM_56224_B0
SUBDIR_RAPTOR = raptor
SUBDIR_FIREBOLT = firebolt
endif

ifdef BCM_53314_A0
SUBDIR_RAPTOR = raptor
SUBDIR_FIREBOLT = firebolt
endif

ifdef BCM_53324_A0
SUBDIR_RAPTOR = raptor
SUBDIR_FIREBOLT = firebolt
endif

ifdef BCM_56820_A0
SUBDIR_FIREBOLT = firebolt
SUBDIR_BRADLEY = bradley
SUBDIR_HERC = hercules
SUBDIR_HUMV = humv
SUBDIR_TRX = trx
SUBDIR_TRIUMPH = triumph
SUBDIR_SCORPION = scorpion
endif

ifdef BCM_56725_A0
SUBDIR_FIREBOLT = firebolt
SUBDIR_BRADLEY = bradley
SUBDIR_HERC = hercules
SUBDIR_HUMV = humv
SUBDIR_TRX = trx
SUBDIR_TRIUMPH = triumph
SUBDIR_SCORPION = scorpion
SUBDIR_CONQUEROR = conqueror
endif

ifdef BCM_56634_A0
SUBDIR_TRIUMPH2 = triumph2
SUBDIR_TRX = trx
SUBDIR_TRIUMPH = triumph
SUBDIR_BRADLEY = bradley
SUBDIR_FIREBOLT = firebolt
endif

ifdef BCM_56634_B0
SUBDIR_TRIUMPH2 = triumph2
SUBDIR_TRX = trx
SUBDIR_TRIUMPH = triumph
SUBDIR_BRADLEY = bradley
SUBDIR_FIREBOLT = firebolt
endif

ifdef BCM_56524_A0
SUBDIR_TRIUMPH2 = triumph2
SUBDIR_TRX = trx
SUBDIR_TRIUMPH = triumph
SUBDIR_BRADLEY = bradley
SUBDIR_FIREBOLT = firebolt
endif

ifdef BCM_56524_B0
SUBDIR_TRIUMPH2 = triumph2
SUBDIR_TRX = trx
SUBDIR_TRIUMPH = triumph
SUBDIR_BRADLEY = bradley
SUBDIR_FIREBOLT = firebolt
endif

ifdef BCM_56685_A0
SUBDIR_TRIUMPH2 = triumph2
SUBDIR_TRX = trx
SUBDIR_TRIUMPH = triumph
SUBDIR_BRADLEY = bradley
SUBDIR_FIREBOLT = firebolt
endif

ifdef BCM_56685_B0
SUBDIR_TRIUMPH2 = triumph2
SUBDIR_TRX = trx
SUBDIR_TRIUMPH = triumph
SUBDIR_BRADLEY = bradley
SUBDIR_FIREBOLT = firebolt
endif

ifdef BCM_56334_A0
SUBDIR_ENDURO = enduro
SUBDIR_TRIUMPH2 = triumph2
SUBDIR_TRX = trx
SUBDIR_TRIUMPH = triumph
SUBDIR_BRADLEY = bradley
SUBDIR_FIREBOLT = firebolt
endif

ifdef BCM_56334_B0
SUBDIR_ENDURO = enduro
SUBDIR_TRIUMPH2 = triumph2
SUBDIR_TRX = trx
SUBDIR_TRIUMPH = triumph
SUBDIR_BRADLEY = bradley
SUBDIR_FIREBOLT = firebolt
endif

ifdef BCM_56840_A0
SUBDIR_HERC = hercules
SUBDIR_TRIDENT = trident
SUBDIR_TITAN = titan
SUBDIR_TRIUMPH2 = triumph2
SUBDIR_TRX = trx
SUBDIR_TRIUMPH = triumph
SUBDIR_BRADLEY = bradley
SUBDIR_HUMV = humv
SUBDIR_SCORPION = scorpion
SUBDIR_FIREBOLT = firebolt
endif

ifdef BCM_56840_B0
SUBDIR_HERC = hercules
SUBDIR_TRIDENT = trident
SUBDIR_TITAN = titan
SUBDIR_TRIUMPH2 = triumph2
SUBDIR_TRX = trx
SUBDIR_ENDURO  = enduro
SUBDIR_TRIUMPH = triumph
SUBDIR_BRADLEY = bradley
SUBDIR_HUMV = humv
SUBDIR_SCORPION = scorpion
SUBDIR_FIREBOLT = firebolt
endif

ifdef BCM_56142_A0
SUBDIR_ENDURO = enduro
SUBDIR_TRIUMPH2 = triumph2
SUBDIR_TRX = trx
SUBDIR_TRIUMPH = triumph
SUBDIR_BRADLEY = bradley
SUBDIR_FIREBOLT = firebolt
SUBDIR_HURRICANE = hurricane
endif

ifdef BCM_56150_A0
SUBDIR_ENDURO = enduro
SUBDIR_TRIUMPH2 = triumph2
SUBDIR_TRIUMPH3 = triumph3
SUBDIR_TRX = trx
SUBDIR_TRIUMPH = triumph
SUBDIR_BRADLEY = bradley
SUBDIR_FIREBOLT = firebolt
SUBDIR_HURRICANE = hurricane
SUBDIR_HURRICANE2 = hurricane2
endif

ifdef BCM_88732_A0
SUBDIR_FIREBOLT = firebolt
SUBDIR_BRADLEY = bradley
SUBDIR_HERC = hercules
SUBDIR_HUMV = humv
SUBDIR_TRX = trx
SUBDIR_TRIUMPH = triumph
SUBDIR_SCORPION = scorpion
SUBDIR_CONQUEROR = conqueror
endif

ifdef BCM_56640_A0
SUBDIR_TRIUMPH3 = triumph3
SUBDIR_TRIDENT = trident
SUBDIR_TITAN = titan
SUBDIR_TRIUMPH2 = triumph2
SUBDIR_HERC = hercules
SUBDIR_HUMV = humv
SUBDIR_TRX = trx
SUBDIR_TRIUMPH = triumph
SUBDIR_BRADLEY = bradley
SUBDIR_SCORPION = scorpion
SUBDIR_FIREBOLT = firebolt
SUBDIR_REGEX = regex
SUBDIR_FLEXCTR = flexctr
SUBDIR_XGS5 = xgs5
endif

ifdef BCM_56850_A0
SUBDIR_HERC = hercules
SUBDIR_TRIDENT = trident
SUBDIR_TITAN = titan
SUBDIR_TRIDENT2 = trident2
SUBDIR_TITAN2 = titan2
SUBDIR_TRIUMPH2 = triumph2
SUBDIR_TRIUMPH3 = triumph3
SUBDIR_TRX = trx
SUBDIR_TRIUMPH = triumph
SUBDIR_BRADLEY = bradley
SUBDIR_HUMV = humv
SUBDIR_SCORPION = scorpion
SUBDIR_FIREBOLT = firebolt
SUBDIR_FLEXCTR = flexctr
SUBDIR_XGS5 = xgs5
endif

ifdef BCM_56440_A0
SUBDIR_HERC = hercules
SUBDIR_KATANA = katana
SUBDIR_TRIDENT = trident
SUBDIR_TITAN = titan
SUBDIR_TRIUMPH2 = triumph2
SUBDIR_TRX = trx
SUBDIR_TRIUMPH = triumph
SUBDIR_BRADLEY = bradley
SUBDIR_SCORPION = scorpion
SUBDIR_FIREBOLT = firebolt
SUBDIR_FLEXCTR = flexctr
SUBDIR_HUMV = humv
SUBDIR_ENDURO = enduro
SUBDIR_REGEX = regex
SUBDIR_XGS5 = xgs5
endif

ifdef BCM_56440_B0
SUBDIR_HERC = hercules
SUBDIR_KATANA = katana
SUBDIR_TRIDENT = trident
SUBDIR_TITAN = titan
SUBDIR_TRIUMPH2 = triumph2
SUBDIR_TRX = trx
SUBDIR_TRIUMPH = triumph
SUBDIR_BRADLEY = bradley
SUBDIR_SCORPION = scorpion
SUBDIR_FIREBOLT = firebolt
SUBDIR_FLEXCTR = flexctr
SUBDIR_HUMV = humv
SUBDIR_ENDURO = enduro
SUBDIR_REGEX = regex
endif

ifdef BCM_56450_A0
SUBDIR_HERC = hercules
SUBDIR_KATANA2 = katana2
SUBDIR_KATANA = katana
SUBDIR_TRIDENT = trident
SUBDIR_TITAN = titan
SUBDIR_TRIUMPH2 = triumph2
SUBDIR_TRIUMPH3 = triumph3
SUBDIR_TRX = trx
SUBDIR_TRIUMPH = triumph
SUBDIR_BRADLEY = bradley
SUBDIR_SCORPION = scorpion
SUBDIR_FIREBOLT = firebolt
SUBDIR_FLEXCTR = flexctr
SUBDIR_HUMV = humv
SUBDIR_ENDURO = enduro
SUBDIR_REGEX = regex
endif

ifdef BCM_56340_A0
SUBDIR_TRIUMPH3 = triumph3
SUBDIR_TRIDENT = trident
SUBDIR_TITAN = titan
SUBDIR_TRIUMPH2 = triumph2
SUBDIR_HERC = hercules
SUBDIR_HUMV = humv
SUBDIR_TRX = trx
SUBDIR_TRIUMPH = triumph
SUBDIR_BRADLEY = bradley
SUBDIR_SCORPION = scorpion
SUBDIR_FIREBOLT = firebolt
SUBDIR_REGEX = regex
SUBDIR_FLEXCTR = flexctr
SUBDIR_XGS5 = xgs5
SUBDIR_TRIDENT2 = trident2
SUBDIR_TITAN2 = titan2
endif

ifneq (,$(findstring -DINCLUDE_PTP,$(CFGFLAGS)))
SUBDIR_PTP = ptp
endif

subdirs := \
        $(SUBDIR_FLEXCTR) \
	$(SUBDIR_HERC) \
	$(SUBDIR_FIREBOLT) \
	$(SUBDIR_BRADLEY) \
	$(SUBDIR_HUMV) \
	$(SUBDIR_RAPTOR) \
	$(SUBDIR_TRX) \
	$(SUBDIR_TRIUMPH) \
	$(SUBDIR_SCORPION) \
	$(SUBDIR_CONQUEROR) \
	$(SUBDIR_TRIUMPH2) \
	$(SUBDIR_ENDURO) \
	$(SUBDIR_TRIDENT) \
	$(SUBDIR_TITAN) \
	$(SUBDIR_HURRICANE) \
	$(SUBDIR_HURRICANE2) \
	$(SUBDIR_KATANA) \
	$(SUBDIR_KATANA2) \
	$(SUBDIR_TRIUMPH3) \
	$(SUBDIR_TRIDENT2) \
	$(SUBDIR_TITAN2) \
	$(SUBDIR_REGEX) \
	$(SUBDIR_PTP) \
	$(SUBDIR_XGS5) \
	$(SUBDIR_NONE)

include ${SDK}/make/Make.subdirs

lib = libbcm_esw
include ${SDK}/make/Make.lib

include ${SDK}/make/Make.depend
