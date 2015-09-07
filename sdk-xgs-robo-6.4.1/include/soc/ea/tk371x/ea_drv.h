/*
 * $Id: ea_drv.h,v 1.1 Broadcom SDK $
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
 * This file contains structure and routine declarations for the
 * Switch-on-a-Chip Driver.
 *
 * This file also includes the more common include files so the
 * individual driver files don't have to include as much.
 */
#ifndef _SOC_EA_DRV_H_
#define _SOC_EA_DRV_H_
#include <soc/macipadr.h>
#include <shared/port.h>
#include <shared/portmode.h>
#include <soc/ea/tk371x/TkDefs.h>

#define SOC_EA_NUM_SUPPORTED_CHIPS          1

#define SOC_EA_EXTEND_ID_LEN                64

#define SOC_EA_EVENT_DATA_LEN               64

#define SOC_EA_DB(unit)         ((soc_ea_private_db_t *)(SOC_INFO(unit).private))

#define SOC_EA_CONTROL(unit)             (SOC_EA_DB(unit)->chip_control)
#define SOC_EA_CONTROL_FLAG(unit)        (SOC_EA_CONTROL(unit).flag)
#define SOC_EA_CONTROL_FLAG_IS_SET(unit,bitmask) \
    (bitmask)&SOC_EA_CONTROL_FLAG(unit)
#define SOC_EA_CONTROL_FLAG_IS_NOT_SET(unit, bitmask) \
    !((bitmask)&SOC_EA_CONTROL_FLAG(unit))
#define SOC_EA_CONTROL_FLAG_SET(unit, bitmask) \
    SOC_EA_CONTROL_FLAG(unit)|=(bitmask)
#define SOC_EA_CONTROL_FLAG_CLEAR(unit, bitmask) \
    SOC_EA_CONTROL_FLAG(unit)&=(~(bitmask))
    
#define SOC_EA_CONTROL_MLLID_NUM(unit)  (SOC_EA_CONTROL(unit).mllid_num)
#define SOC_EA_CONTROL_Q_CONFIG(unit)   (&(SOC_EA_CONTROL(unit).q_config)) 

#define SOC_EA_PRIVATE(unit)            (&(SOC_EA_DB(unit)->chip_info))
#define SOC_EA_PRIVATE_FLAG(unit)       (SOC_EA_PRIVATE(unit)->flag)
#define SOC_EA_PRIVATE_FLAG_IS_SET(unit,bitmask) \
    (bitmask)&SOC_EA_PRIVATE_FLAG(unit)
#define SOC_EA_PRIVATE_FLAG_IS_NOT_SET(unit, bitmask) \
    !((bitmask)&SOC_EA_PRIVATE_FLAG(unit))
#define SOC_EA_PRIVATE_FLAG_SET(unit, bitmask) \
    (SOC_EA_PRIVATE(unit)->flag)|=(bitmask)
#define SOC_EA_PRIVATE_FLAG_CLEAR(unit, bitmask) \
    (SOC_EA_PRIVATE(unit)->flag)&=(~(bitmask))

#define SOC_EA_FIRMWARE_VERSION(unit,version) \
    version = (SOC_EA_PRIVATE(unit))->load_info[socEaFileApp0].legacy_ver
#define SOC_EA_JEDEC_ID(unit, jedec_id) \
    jedec_id = SOC_EA_PRIVATE(unit)->onu_info.jedec_id
#define SOC_EA_CHIP_ID(unit, chip_id) \
    chip_id = SOC_EA_PRIVATE(unit)->onu_info.chip_id
#define SOC_EA_CHIP_REVISION(unit, chip_rev) \
    chip_rev = SOC_EA_PRIVATE(unit)->onu_info.chip_ver
#define SOC_EA_VENDOR_ID(unit, vendor_id) \
    vendor_id = (SOC_EA_PRIVATE(unit)->extend_id)
#define SOC_EA_LOAD_INFO(unit, load_type, load_info) \
    load_info = &(SOC_EA_PRIVATE(unit)->load_info[load_type])

#define SOC_EA_SYNC_FLAG_NORMAL                 0
#define SOC_EA_SYNC_FLAG_FORCE                  1
    
typedef enum soc_ea_chip_info_e{
    socEaChipInfoLoadInfo,
    socEaChipInfoOnuInfo,
    socEaChipInfoRevisionId,
    socEaChipInfoVendorId,
    socEaChipInfoCtcOamVer,
    socEaChipInfoLllidCount,
    socEaChipInfoPort,
    socEaChipInfoCount
} soc_ea_chip_info_t;

typedef enum soc_ea_chip_control_e{
    socEaChipControlLlidCount,
    socEaChipControlQueueConfig,
    socEaChipControlCount
}soc_ea_chip_control_t;

typedef enum {
    socEaFileBoot,
    socEaFileApp0,
    socEaFileApp1,
    socEaFilePers,
    socEaFileCount
}soc_ea_file_type_t;

typedef enum {
    socEaVerRel     = 'R',
    socEaVerCustom  = 'C',
    socEaVerBeta    = 'B',
    socEaVerAlpha   = 'A',
    socEaVerEng     = 'E',
    socEaVerDev     = 'D',
    socEaVerCount   = 6
}soc_ea_ver_type_t;

typedef struct {
    uint8  bcm_revision;
    uint32 tk_revision;
    char   *chip_string;  
} soc_ea_chip_rev_map_t;

typedef struct {
   soc_ea_ver_type_t type;
   uint8 major;
   uint8 minor;
   uint16 patch;
}soc_ea_ver_t;

typedef struct {
    uint16  year;
    uint8   month;
    uint8   day;
    uint8   hour;
    uint8   min;
    uint8   sec;
}soc_ea_build_time_t;

typedef struct {
    uint16  legacy_ver;
    uint32  crc;
    soc_ea_ver_t ver;
    uint32  stream;
    uint32  revision;
    soc_ea_build_time_t time;
} soc_ea_load_info_t;

typedef struct {
    uint16 firmware_ver;
    sal_mac_addr_t pon_base_mac;
    sal_mac_addr_t uni_base_mac;
    uint16 port_count;
    uint16 max_link_count;

    uint16 up_queue_count;
    uint16 max_queue_count_per_link;
    uint16 up_queue_unit;

    uint16 dn_queue_count;
    uint16 max_queue_count_per_port;
    uint16 dn_queue_unit;

    uint32 up_queue_total_size;
    uint32 dn_queue_total_size;

    uint16 jedec_id;
    uint16 chip_id;
    uint32 chip_ver;
} soc_ea_onu_info_t;

typedef uint8 soc_ea_extend_id_t[SOC_EA_EXTEND_ID_LEN];

typedef struct {
    uint16 q_count; 
    uint32 q_size[MAX_CNT_OF_UP_QUEUE];
} soc_ea_port_q_info_t;

typedef struct {
    uint8 link_count;
    soc_ea_port_q_info_t link_info[MAX_CNT_OF_LINK];
    uint8 port_count;
    soc_ea_port_q_info_t port_info[SDK_MAX_NUM_OF_PORT];
    soc_ea_port_q_info_t mcast_info;
} soc_ea_chip_q_config_t;

typedef struct {
    unsigned int flag;
#define SOC_EA_CHIP_CONTROL_MLLID           1
#define SOC_EA_CHIP_CONTROL_QUEUE_CONFIG    (1<<1)

    uint16 mllid_num;
    soc_ea_chip_q_config_t q_config;
} soc_ea_chip_control_db_t;

typedef enum {
    socEaPortInterface,
    socEaPortAutoneg,
    socEaPortSpeed,
    socEaPortDuplex,
    socEaPortPauseTx,
    socEaPortPauseRx,
    socEaPortAdvertisement,
    socEaPortAdvertisementRemote,
    socEaPortAbilitySpeedHalfDuplex,
    socEaPortAbilitySpeedFullDuplex,
    socEaPortAbilityPause,
    socEaPortAbilityInterface,
    socEaPortAbilityMedium,
    socEaPortAbilityLoopback,
    socEaPortAbilityEncap,
    socEaPortMdix,
    socEaPortMdixState,
    socEaPortBcastLimit,
    socEaPortBcastLimitState,
    socEaPortPhyMaster,
    socEaPortLoopback,
    socEaPortCount
} soc_ea_port_control_t;
    
typedef struct {
    _shr_port_if_t interface;
    int     autoneg;
    uint32  speed;
    uint32  duplex;
    _shr_port_mode_t advert_local;
    _shr_port_mode_t advert_remote;
    _shr_port_mode_t ability_speed_half_duplex;
    _shr_port_mode_t ability_speed_full_duplex;
    _shr_port_mode_t ability_pause;
    _shr_port_mode_t ability_interface;
    _shr_port_mode_t ability_medium;
    _shr_port_mode_t ability_loopback;
    _shr_pa_encap_t  ability_encap;
    _shr_port_mdix_t mdix;
    _shr_port_mdix_status_t mdix_status;
    int bcast_limit; 
    int bcast_limit_enable; 
    int phy_master;
    int loopback;
} soc_ea_port_control_db_t;

typedef struct {
    uint32 flag;
    
#define SOC_EA_CHIP_INFO_LOAD_INFO      1
#define SOC_EA_CHIP_INFO_ONU_INFO       (1<<1)
#define SOC_EA_CHIP_INFO_REVISION_ID    (1<<2)
#define SOC_EA_CHIP_INFO_VERDOR_ID      (1<<3)
#define SOC_EA_CHIP_INFO_CTC_OAM_VER    (1<<4)
#define SOC_EA_CHIP_INFO_LLID_COUNT     (1<<5)
#define SOC_EA_CHIP_INFO_PORT           (1<<6)

    soc_ea_load_info_t load_info[socEaFileCount];
    soc_ea_onu_info_t onu_info;
    uint32 chip_revision;
    soc_ea_extend_id_t extend_id;
    uint32 oam_ver;
    uint32 llid_count;
    soc_ea_port_control_db_t port[SDK_MAX_NUM_OF_PORT];
} soc_ea_chip_info_db_t;

typedef struct {
    soc_ea_chip_info_db_t chip_info;
    soc_ea_chip_control_db_t chip_control;
} soc_ea_private_db_t;

typedef struct soc_ea_event_userdata_s {
    uint32 len;
    uint8  data[SOC_EA_EVENT_DATA_LEN];
}soc_ea_event_userdata_t;

int 
_soc_ea_chip_info_sync(int unit, soc_ea_chip_info_t type, int flag);

int
_soc_ea_chip_control_sync(int unit, soc_ea_chip_control_t type, int flag);

int 
_soc_ea_chip_control_flag_update(int unit, soc_ea_chip_control_t type, int dirty);

void
soc_ea_private_db_dump(int unit);


#endif /* _SOC_EA_DRV_H_ */
