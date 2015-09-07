/*
 * $Id: misc.h,v 1.4 Broadcom SDK $
 *
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
 * File:     misc.h
 * Purpose:
 *
 */

#ifndef _SOC_EA_MISC_H
#define _SOC_EA_MISC_H

#include <soc/ea/tk371x/CtcMiscApi.h>

typedef CtcInfoFromONUPONChipsSet
								_soc_ea_ctc_info_from_onu_pon_chip_set_t;
typedef CtcExtRuleCondtion		_soc_ea_ctc_ext_rule_condtion_t;
typedef CtcExtRule				_soc_ea_ctc_ext_rule_t;
typedef CtcExtQConfig			_soc_ea_ctc_ext_qconfig_t;
typedef CtcExtONUTxPowerSupplyControl
								_soc_ea_ctc_ext_onu_tx_power_supply_control_t;

#define _soc_ea_ctc_onu_chipset_info_clear	TKCTCClearONUPONChipsSetInfo
#define	_soc_ea_ctc_mac_for_sn_fill			TKCTCExtOamFillMacForSN
#define _soc_ea_ctc_info_from_onu_pon_chipsets_get \
										TKCTCExtOamGetInfoFromONUPONChipsets
#define _soc_ea_ctc_dba_cfg_get			TKCTCExtOamGetDbaCfg
#define _soc_ea_ctc_dba_cfg_set         TKCTCExtOamSetDbaCfg
#define _soc_ea_ctc_fec_ability_get		TKCTCExtOamGetFecAbility
#define _soc_ea_ctc_oam_none_obj_raw_get	TKCTCExtOamNoneObjGetRaw
#define _soc_ea_ctc_oam_none_obj_raw_set	TKCTCExtOamNoneObjSetRaw
#define _soc_ea_ctc_oam_obj_raw_get			TKCTCExtOamObjGetRaw
#define _soc_ea_ctc_oam_obj_raw_set			TKCTCExtOamObjSetRaw
#define _soc_ea_ctc_onu_sn_get				TKCTCExtOamGetONUSN
#define _soc_ea_ctc_firmware_version_get	TKCTCExtOamGetFirmwareVersion
#define _soc_ea_ctc_chipset_id_get			TKCTCExtOamGetChipsetId
#define _soc_ea_ctc_onu_cap_get				TKCTCExtOamGetONUCap
#define _soc_ea_ctc_onu_cap2_get			TKCTCExtOamGetONUCap2
#define _soc_ea_ctc_holdover_config_get		TKCTCExtOamGetHoldOverConfig
#define _soc_ea_ctc_cls_marking_set			TKCTCExtOamSetClsMarking
#define _soc_ea_ctc_llid_queue_config_set	TKCTCExtOamSetLLIDQueueConfig
#define _soc_ea_ctc_laser_tx_power_admin_ctl_set	\
										TKCTCExtOamSetLaserTxPowerAdminCtl
#define _soc_ea_ctc_mllid_set           CtcExtOamSetMulLlidCtrl

#endif /* _SOC_EA_MISC_H */
