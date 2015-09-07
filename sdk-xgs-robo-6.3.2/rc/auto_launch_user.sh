#!/bin/sh
# $Id: auto_launch_user.sh 1.12 Broadcom SDK $
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

cd /broadcom

grep -q BCM9562 /proc/cpuinfo
if [ $? == 0 ] ; then
    mount -t vfat /dev/mtdblock0 /mnt
else
    grep -q BCM5300 /proc/cpuinfo
    if [ $? == 0 ] ; then
        mount -t vfat /dev/mtdblock2 /mnt
    else
        grep -q BCM98548 /proc/cpuinfo
        if [ $? == 0 ] ; then
            mount -t vfat /dev/mtdblock1 /mnt
        fi
    fi
fi

if [ -f /mnt/config.bcm ] ; then
    export BCM_CONFIG_FILE=/mnt/config.bcm
else
    # See if Petra co-processor (vendor 1172, device 4) / ARAD / FE1600
    # is present. on the PCI bus. If it is, then this is a Dune board.
    #
    egrep -q "11720004|14e48650|14e48750" /proc/bus/pci/devices
    if [ $? == 0 ] ; then
        export BCM_CONFIG_FILE=/broadcom/config-sand.bcm
    elif [ -r  /proc/bus/pci/00/14.0 ] ; then
        export BCM_CONFIG_FILE=/broadcom/config-sbx-polaris.bcm
    elif [ -d /proc/bus/pci/02 ] ; then
        export BCM_CONFIG_FILE=/broadcom/config-sbx-sirius.bcm
    elif [ -d /proc/bus/pci/11.0 ] ; then
        export BCM_CONFIG_FILE=/broadcom/config-sbx-fe2kxt.bcm
    else
        # Not XCore, not SAND. Must be an XGS board...
        # Load the default configuration file.
        #
        export BCM_CONFIG_FILE=/broadcom/config.bcm
    fi
fi

./bcm.user

