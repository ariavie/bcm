/*
 * $Id: config.h,v 1.11 Broadcom SDK $
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
 * File: 	config.h
 * Purpose: 	SAL Configuration file definitions
 */

#ifndef _SAL_CONFIG_H
#define _SAL_CONFIG_H

#if defined(UNIX) || defined(__ECOS)
# ifndef SAL_CONFIG_FILE
#  define SAL_CONFIG_FILE		"config.bcm"
# endif
# ifndef SAL_CONFIG_TEMP
#  define SAL_CONFIG_TEMP		"config.tmp"
# endif
#else
# ifndef SAL_CONFIG_FILE
#  define SAL_CONFIG_FILE               "config.bcm"
#       ifndef SAL_CONFIG_FILE_FLASH
#        define SAL_CONFIG_FILE_FLASH	"flash:config.bcm"
#       endif
# endif
# ifndef SAL_CONFIG_TEMP
#  define SAL_CONFIG_TEMP		"config.tmp"
#       ifndef SAL_CONFIG_TEMP_FLASH
#        define SAL_CONFIG_TEMP_FLASH	"flash:config.tmp"
#       endif
# endif
#endif

#define SAL_CONFIG_STR_MAX		128	/* Max len of "NAME=VALUE\0" */

/*
 * Defines:	SAL_CONFIG_XXX
 *
 *	SWITCH_MAC 	Mac address used for management interface
 *	SWITCH_IP	IP address used for management interface
 *	ASSIGNED_MAC_BASE Base of MAC address range assigned to switch
 *			(excluding SWITCH_MAC).
 *	ASSIGNED_MAC_COUNT # of MAC addresses assigned to switch
 *			(excluding SWITCH_MAC).
 */

#define	SAL_CONFIG_SWITCH_MAC		"station_mac_address"
#define	SAL_CONFIG_SWITCH_IP		"station_ip_address"
#define	SAL_CONFIG_SWITCH_IP_NETMASK	"station_ip_netmask"
#define	SAL_CONFIG_SWITCH_HOSTNAME	"station_hostname"
#define	SAL_CONFIG_ASSIGNED_MAC_BASE	"switch_mac_base"
#define	SAL_CONFIG_ASSIGNED_MAC_COUNT	"switch_mac_count"

#define SAL_CONFIG_RELOAD_BUFFER_SIZE	"reload_buffer_size"
#define SAL_CONFIG_RELOAD_FILE_NAME	"reload_file_name"

extern int 	sal_config_refresh(void);
extern int	sal_config_file_set(const char *fname, const char *tname);
extern int	sal_config_file_get(const char **fname, const char **tname);
extern int	sal_config_flush(void);
extern int	sal_config_save(char *file, char *pattern, int append);
extern char 	*sal_config_get(const char *name);
extern int	sal_config_get_next(char **name, char **value);
extern int	sal_config_set(char *name, char *value);
extern void	sal_config_show(void);
extern int      sal_config_init(void);

/* Declaration for compiled-in config variables.  These are generated
 * automatically from config.bcm by $SDK/make/bcm2c.pl.
 */
extern void     sal_config_init_defaults(void);

#endif	/* !_SAL_CONFIG_H */
