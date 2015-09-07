/* 
 * $Id: cmicsim.h 1.12 Broadcom SDK $
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
 * File:        cmicsim.h
 * Purpose:
 * Notes:
 *     REMEMBER  REMEMBER  REMEMBER  REMEMBER  REMEMBER
 *
 * SCHAN memory 0 to 0x4c is aliased to 0x800 on some chips.  If you
 * ever manually adjust one of those spaces, make sure to take care
 * of the other.  This is done currently in schan_op and in
 * pli_setreg_service.
 */

#ifndef   _CMICSIM_H_
#define   _CMICSIM_H_

#include <soc/defs.h>
#include <sys/types.h>

#include "pcid.h"
#include "mem.h"

#define PCIM_SIZE(unit) \
    (soc_feature(unit, soc_feature_led_proc) ? 0x2000 : \
     soc_feature(unit, soc_feature_schmsg_alias) ? 0x900 : \
     0x200)

#define PCIM_CMICM_SIZE         0x38000

#define PCIM_ALIAS_OFFSET	0x800
#define PCIM_ALIASED_BYTES	0x50

#define PCIM(pcid, addr)        (pcid)->pcim[(addr) / 4]

#define PCIC(pcid, addr)        (pcid)->pcic[(addr) / 4]

#define	MF_ES_PIO		0	/* PIO endian swapping */
#define	MF_ES_DMA_PACKET	1	/* DMA packet endian swapping */
#define	MF_ES_DMA_OTHER		2	/* DMA other endian swapping */

extern void soc_internal_pcic_init(pcid_info_t *pcid_info,
               uint16 pci_dev_id, uint16 pci_ven_id, uint8 pci_rev_id,
               uint32 pci_mbar0);
extern uint32 soc_internal_endian_swap(pcid_info_t *, uint32 data, int mode);
extern int soc_internal_write_reg(pcid_info_t * pcid_info, uint32 address,
                                  uint32 * data);
extern int soc_internal_read_reg(pcid_info_t * pcid_info, uint32 address,
                                 uint32 * data);
extern int soc_internal_write_mem(pcid_info_t * pcid_info, uint32 address,
                                  uint32 * data);
extern int soc_internal_read_mem(pcid_info_t * pcid_info, uint32 address,
                                 uint32 * data);
extern int soc_internal_read_aggrmem(pcid_info_t * pcid_info, uint32 addr,
                                 uint32 * data);
extern int soc_internal_write_aggrmem(pcid_info_t * pcid_info, uint32 addr,
                                 uint32 * data);
extern int soc_internal_read_bytes(pcid_info_t *pcid_info, uint32 address, 
                                   uint8 *data, int len);
extern int soc_internal_write_bytes(pcid_info_t *pcid_info, uint32 address, 
                                    uint8 *data, int len);
extern int soc_internal_vlan_tab_init(pcid_info_t *pcid_info);
extern void soc_internal_miim_op(pcid_info_t * pcid_info, int read);
extern void soc_internal_reset(pcid_info_t *pcid_info);
extern void soc_internal_memory_fetch(pcid_info_t * pcid_info, uint32 addr,
                                        uint8 * buf, int length, int mode);
extern void soc_internal_memory_store(pcid_info_t * pcid_info, uint32 addr,
                                        uint8 * buf, int length, int mode);
extern void soc_internal_bytes_fetch(pcid_info_t *pcid_info, uint32 addr,
                                     uint8 *buf, int length);
extern void soc_internal_bytes_store(pcid_info_t *pcid_info, uint32 addr,
                                     uint8 *buf, int length);

extern int soc_internal_vlan_tab_init(pcid_info_t *pcid_info);
extern void soc_internal_send_int(pcid_info_t *pcid_info);
extern void soc_internal_cmicm_send_int(pcid_info_t *pcid_info, uint32 reg);
extern void soc_internal_schan_ctrl_write(pcid_info_t *pcid_info, uint32 value);
extern void soc_internal_cmicm_schan_ctrl_write(pcid_info_t *pcid_info, uint32 reg, uint32 value);
extern void soc_internal_xgs3_table_dma(pcid_info_t *pcid_info);
extern void soc_internal_xgs3_cmicm_table_dma(pcid_info_t *pcid_info);
extern void soc_internal_xgs3_tslam_dma(pcid_info_t *pcid_info);
extern void soc_internal_xgs3_cmicm_tslam_dma(pcid_info_t *pcid_info);
extern void soc_internal_cmicm_sbusdma(pcid_info_t *pcid_info, uint32 reg);


extern int pcid_add_pkt(pcid_info_t *pcid_info, uint8 *pkt_data,
                        int pkt_len, uint32 *dcbd);


extern int soc_internal_extended_write_reg(pcid_info_t *pcid_info,
                                           soc_block_t block, int acc_type,
                                           uint32 address, uint32 *data);
extern int soc_internal_extended_read_reg(pcid_info_t *pcid_info,
                                          soc_block_t block, int acc_type,
                                          uint32 address, uint32 *data);
extern int soc_internal_extended_write_mem(pcid_info_t *pcid_info,
                                           soc_block_t block, int acc_type,
                                           uint32 address, uint32 *data);
extern int soc_internal_extended_read_mem(pcid_info_t *pcid_info,
                                          soc_block_t block, int acc_type,
                                          uint32 address, uint32 *data);
extern int soc_internal_extended_write_aggrmem(pcid_info_t *pcid_info,
                                               soc_block_t block,
                                               int acc_type,
                                               uint32 address, uint32 *data);
extern int soc_internal_extended_read_aggrmem(pcid_info_t *pcid_info,
                                              soc_block_t block,
                                              int acc_type,
                                              uint32 address, uint32 *data);
extern int soc_internal_extended_write_bytes(pcid_info_t *pcid_info,
                                             soc_block_t block, int acc_type,
                                             uint32 address,
                                             uint8 *data, int len);
extern int soc_internal_extended_read_bytes(pcid_info_t *pcid_info,
                                            soc_block_t block, int acc_type,
                                            uint32 address,
                                            uint8 *data, int len);
extern int soc_internal_extended_write_mem_index(pcid_info_t *pcid_info,
                                                 soc_mem_t mem, int index,
                                                 uint32 *data);
extern int soc_internal_extended_read_mem_index(pcid_info_t *pcid_info,
                                                soc_mem_t mem, int index,
                                                uint32 *data);
#endif /* _CMICSIM_H_ */
