/*
 * $Id: memory.h 1.61 Broadcom SDK $
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
 * File:        memory.h
 * Purpose:     Base definitions for memory types
 * Requires:    
 */

#ifndef _SOC_MEMORY_H
#define _SOC_MEMORY_H

#include <soc/defs.h>
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
#include <soc/mcm/allenum.h>
#endif
#ifdef BCM_ROBO_SUPPORT
#include <soc/robo/mcm/allenum.h>
#endif
#include <soc/field.h>
#include <soc/macipadr.h>

#define SOC_MEM_FLAG_READONLY                0x00000001 /* True if table is read-only */
#define SOC_MEM_FLAG_VALID                   0x00000002 /* Some tables don't really exist */
#define SOC_MEM_FLAG_DEBUG                   0x00000004 /* Access requires CMIC DebugMode */
#define SOC_MEM_FLAG_SORTED                  0x00000008 /* Table is kept in sorted order */
#define SOC_MEM_FLAG_CBP                     0x00000010 /* Table is part of CBP */ /* Deprecated */
#define SOC_MEM_FLAG_CACHABLE                0x00000020 /* Reasonable to cache in S/W */
#define SOC_MEM_FLAG_BISTCBP                 0x00000040 /* CBP has BIST for this memory */ /* Deprecated */
#define SOC_MEM_FLAG_SER_ENTRY_CLEAR         0x00000040 /* Clear entry on error */
#define SOC_MEM_FLAG_BISTEPIC                0x00000080 /* EPIC has BIST for this memory */ /* Deprecated */
#define SOC_MEM_FLAG_SER_CACHE_RESTORE       0x00000080 /* Restore entry from a valid cache on error */
#define SOC_MEM_FLAG_BISTBIT                 0x00003f00 /* Bit pos for BIST reg (0-63) */ /* Deprecated */
#define SOC_MEM_FLAG_BISTBSHFT               8          /* Shift corresponding to BISTBIT */ /* Deprecated */
#define SOC_MEM_FLAG_BISTFFP                 0x00004000 /* Use FFPBIST for this memory */ /* Deprecated */
#define SOC_MEM_FLAG_SER_ECC_CORRECTABLE     0x00000100 /* Issue read+write to correct error */
#define SOC_MEM_FLAG_SER_SPECIAL             0x00000200 /* Memory needs special correction handling */
#define SOC_MEM_FLAG_SER_WRITE_CACHE_RESTORE 0x00000400 /* Maintain cache for correction purpose even 
                                                           for h/w updated memory. Cache updated on writes only */
#define SOC_MEM_FLAG_SER_SRAM_CORRECTION     0x00000800 /* Do SRAM XOR correction */
#define SOC_MEM_FLAG_SER_ENGINE              0x00001000 /* Memory protected by SER engine */
#define SOC_MEM_FLAG_SCHAN_ERROR_RETURN      0x00002000 /* Return error on SCHAN NAK due to SER error */
#define SOC_MEM_FLAG_IS_ARRAY                0x00008000 /* Is this a memory array */
#define SOC_MEM_FLAG_UNIFIED                 0x00010000 /* Only one copy of table */
#define SOC_MEM_FLAG_HASHED                  0x00020000 /* Hashed table */
#define SOC_MEM_FLAG_WORDADR                 0x00040000 /* Requires special addressing */
#define SOC_MEM_FLAG_CAM                     0x00080000 /* Memory is a CAM */
#define SOC_MEM_FLAG_AGGR                    0x00100000 /* Memory is a AGGREGATE */
#define SOC_MEM_FLAG_CMD                     0x00200000 /* Memory allows CMDMEM access */
#define SOC_MEM_FLAG_MONOLITH                0x04000000 /* no block in addressing */
#define SOC_MEM_FLAG_BE                      0x08000000 /* Big endian */
#define SOC_MEM_FLAG_MULTIVIEW               0x10000000 /* Multiview memories */
#define SOC_MEM_FLAG_EXT_CAM                 0x40000000 /* Memory is a external CAM */
#define SOC_MEM_FLAG_SIGNAL                  0x10000000 /* can be read/ can be write but not compare */
#define SOC_MEM_FLAG_WRITEONLY               0x80000000 /* True if table is write-only */

#define SOC_MEM_SER_FLAGS (SOC_MEM_FLAG_SER_ENTRY_CLEAR |\
                           SOC_MEM_FLAG_SER_CACHE_RESTORE |\
                           SOC_MEM_FLAG_SER_WRITE_CACHE_RESTORE |\
                           SOC_MEM_FLAG_SER_ECC_CORRECTABLE |\
                           SOC_MEM_FLAG_SER_SPECIAL)

#define SOC_MEM_SER_CORRECTION_TYPE(unit, mem) \
    (SOC_MEM_INFO(unit, mem).flags & SOC_MEM_SER_FLAGS)

#define SOC_MEM_FLAG_ACCSHIFT                22         /* Shift corresponding to ACCTYPE */

#define SOC_MEM_RETURN_SCHAN_ERR(unit, mem) \
    (SOC_MEM_INFO(unit, mem).flags & SOC_MEM_FLAG_SCHAN_ERROR_RETURN)

#define SOC_MEM_RETURN_SCHAN_ERR_SET(unit, mem, enable) \
     if (enable) SOC_MEM_INFO(unit, mem).flags |= SOC_MEM_FLAG_SCHAN_ERROR_RETURN; \
     else SOC_MEM_INFO(unit, mem).flags &= ~SOC_MEM_FLAG_SCHAN_ERROR_RETURN


/* Flags for memory snooping callback register */
#define SOC_MEM_SNOOP_WRITE     0x00000001 /* snooping write operations */
#define SOC_MEM_SNOOP_READ      0x00000002 /* snooping read operations */
/*
 * Declarations for null table entries.
 *
 * A null table entry is the data normally written to each entry when a
 * memory is cleared.  This is generally all zeroes, except for sorted
 * tables fields which make up the search key are generally all f's.
 */

extern uint32   _soc_mem_entry_null_zeroes[SOC_MAX_MEM_WORDS];
extern uint32   _soc_mem_entry_null_word0[SOC_MAX_MEM_WORDS];
#if defined(BCM_ROBO_SUPPORT)
extern uint32   _soc_robo_mem_entry_null_zeroes[SOC_ROBO_MAX_MEM_WORDS];
#endif /* BCM_ROBO_SUPPORT */

#if defined(BCM_PETRA_SUPPORT)
/* DPP: Special entries format for specific tables */
extern uint32		_soc_mem_entry_multicast_null[SOC_MAX_MEM_WORDS];
extern uint32		_soc_mem_entry_llr_llvp_null[SOC_MAX_MEM_WORDS];
#endif /* BCM_PETRA_SUPPORT */

typedef int (*soc_mem_cmp_t)(int, void *, void *);
typedef void (*soc_mem_snoop_cb_t) (int unit, soc_mem_t mem, uint32 flags, 
                                    int copyno, int index_min, int index_max, 
                                    void *data_buffer, void *user_data);

/* additional information for memories which are arrays */
/* To be used if in soc_mem_info_t, the SOC_MEM_FLAG_IS_ARRAY bit is set */
typedef struct soc_mem_array_info_t {
    unsigned numels;       /* number of memory elements in the array. */
    uint32   element_skip; /* how many bytes to skip between array elements */
} soc_mem_array_info_t;


/* New unified soc memories structure */
typedef struct soc_mem_info_s {
    uint32  flags;  /* Logical OR of SOC_MEM_FLAG_xxx */
    soc_mem_cmp_t   cmp_fn; /* Function to compare two entries */
    void    *null_entry;    /* Value when table entry is empty */
    int     index_min;  /* Minimum table index */
    int     index_max;  /* Maximum table index */
    uint16  minblock;   /* first blocks entry */
    uint16  maxblock;   /* last blocks entry */
    uint32  blocks;     /* bitmask of valid blocks */
    uint32  blocks_hi;  /* bitmask 32:64 of valid blocks */
    uint32  base;       /* Includes region offset */
    uint32              gran;           /* Phys addr granularity by index */
    uint16  bytes;
    uint16  nFields;
    soc_field_info_t    *fields;
    char                **views;
    soc_mem_snoop_cb_t  snoop_cb;/* User call back for snooping memory */
    void                *snoop_user_data; /* User data for callback function */
    uint32              snoop_flags;    /* Snooping flags SOC_MEM_SNOOP_* */
} soc_mem_info_t;

#define SOC_MEM_IP6_FULL_ADDR       0   /* IPv6 address all 128 bits  */
#define SOC_MEM_IP6_UPPER_ONLY      1   /* IPv6 address upper 64 bits */
#define SOC_MEM_IP6_LOWER_ONLY      2   /* IPv6 address lower 64 bits */
#define SOC_MEM_IP6_UPPER_96BIT     4   /* IPv6 address upper 96 bits */
#define SOC_MEM_IP6_BITS_119_96     5   /* IPv6 address bits [119:96] */
#define SOC_MEM_IP6_LOWER_96BIT     6   /* IPv6 address lower 96 bits */

#define SOC_MEM_MAC_FULL_ADDR       0   /* Mac address all 48 bits  */
#define SOC_MEM_MAC_UPPER_ONLY      1   /* Mac address upper 24 bits */
#define SOC_MEM_MAC_LOWER_ONLY      2   /* Mac address lower 24 bits */

typedef enum soc_mem_base_to_wide_conv_type_e {
    TYPE_1_TO_TYPE_2,
    TYPE_1_TO_TYPE_4,
    TYPE_2_TO_TYPE_4,
    TYPE_2_TO_TYPE_1,
    TYPE_4_TO_TYPE_1,
    TYPE_4_TO_TYPE_2
} soc_mem_base_to_wide_conv_type_t;

uint32 soc_mem_addr(int unit, soc_mem_t mem, unsigned memory_index, int block, int index);
uint32 soc_mem_addr_get(int unit, soc_mem_t mem, unsigned memory_index, soc_block_t block, 
                        int index, uint8 *acc_type);
soc_mem_t soc_addr_to_mem(int unit, uint32 address, uint32 *mem_block);
soc_mem_t soc_addr_to_mem_extended(int unit, uint32 block, int acc_type, uint32 address);
int soc_mem_addr_to_array_element_and_index(int unit, soc_mem_t mem, uint32 address, unsigned* numels, int* index);

uint32 *soc_mem_field_get(int unit, soc_mem_t mem, const uint32 *entbuf,
                          soc_field_t field, uint32 *fldbuf);
void soc_mem_field_set(int unit, soc_mem_t mem, uint32 *entbuf,
                       soc_field_t field, uint32 *fldbuf);
void soc_mem_field_width_fit_set(int unit, soc_mem_t mem, uint32 *entbuf,
                                 soc_field_t field, uint32 *fldbuf);

extern int soc_mem_field_pbmp_fit(int unit, soc_mem_t mem, 
                                  soc_field_t field, uint32 *value);
uint32 soc_mem_field32_get(int unit, soc_mem_t mem, const void *entbuf,
                           soc_field_t field);
uint32
soc_mem_field32_get_def(int         unit,
                        soc_mem_t   mem,
                        const void  *entbuf,
                        soc_field_t field,
                        uint32      def
                        );
void soc_mem_field32_set(int unit, soc_mem_t mem, void *entbuf,
                         soc_field_t field, uint32 value);
void soc_mem_field32_force(int unit, soc_mem_t mem, void *entbuf,
                           soc_field_t field, uint32 value);
uint32 *soc_mem_mask_field_get(int unit, soc_mem_t mem, const uint32 *entbuf,
                               soc_field_t field, uint32 *fldbuf);
void soc_mem_mask_field_set(int unit, soc_mem_t mem, uint32 *entbuf,
                            soc_field_t field, uint32 *fldbuf);
uint32 soc_mem_mask_field32_get(int unit, soc_mem_t mem, const void *entbuf,
                                soc_field_t field);
void soc_mem_mask_field32_set(int unit, soc_mem_t mem, void *entbuf,
                              soc_field_t field, uint32 value);
void soc_mem_mac_addr_set(int unit, soc_mem_t mem, void *entry,
                          soc_field_t field, const sal_mac_addr_t mac);
void soc_mem_mac_addr_get(int unit, soc_mem_t mem, const void *entry,
                          soc_field_t field, sal_mac_addr_t mac);
void soc_mem_ip6_addr_set(int unit, soc_mem_t mem, void *entry, 
                          soc_field_t field, const ip6_addr_t ip6, int flags);
void soc_mem_ip6_addr_get(int unit, soc_mem_t mem, const void *entry, 
                          soc_field_t field, ip6_addr_t ip6, int flags);
void soc_mem_ip6_addr_mask_set(int unit, soc_mem_t mem, void *entry, 
                               soc_field_t field, const ip6_addr_t ip6,
                               int flags);
void soc_mem_ip6_addr_mask_get(int unit, soc_mem_t mem, const void *entry, 
                               soc_field_t field, ip6_addr_t ip6, int flags);
void soc_mem_pbmp_field_set(int unit, soc_mem_t mem, void *entbuf,
                            soc_field_t field, soc_pbmp_t *pbmp);
void soc_mem_pbmp_field_get(int unit, soc_mem_t mem, const void *entbuf,
                            soc_field_t field, soc_pbmp_t *pbmp);
extern void soc_mem_field64_set(int unit, soc_mem_t mem, void *entry,
                                soc_field_t field, const uint64 val64);
extern void soc_mem_field64_get(int unit, soc_mem_t mem, void *entry,
                                soc_field_t field, uint64 *val64);
extern void soc_mem_datamask_get(int unit, soc_mem_t mem, uint32 *buf);
extern void soc_mem_datamask_rw_get(int unit, soc_mem_t mem, uint32 *buf);

extern void soc_mem_tcammask_get(int unit, soc_mem_t mem, uint32 *buf);
extern void soc_mem_eccmask_get(int unit, soc_mem_t mem, uint32 *buf);
extern void soc_mem_forcedata_get(int unit, soc_mem_t mem, uint32 *maskbuf,
                                  uint32 *databuf);

/* These functions do not require a unit number to manipulate mem vars */
uint32 *soc_meminfo_field_get(soc_mem_t mem, soc_mem_info_t *meminfo,
                              const uint32 *entbuf, soc_field_t field,
                              uint32 *fldbuf);
void soc_meminfo_mac_addr_set(soc_mem_t mem, soc_mem_info_t *meminfo,
                              void *entry, soc_field_t field,
                              const sal_mac_addr_t mac);
void soc_meminfo_field_set(soc_mem_t mem, soc_mem_info_t *meminfo,
                           uint32 *entbuf, soc_field_t field, uint32 *fldbuf);
void soc_meminfo_field32_force(soc_mem_t mem, soc_mem_info_t *meminfo,
                               void *entry, soc_field_t field, uint32 value);
void soc_mem_mac_address_set(int unit, soc_mem_t mem, void *entry,
                        soc_field_t field, const sal_mac_addr_t mac, int flags);
void soc_mem_mac_address_get(int unit, soc_mem_t mem, const void *entry,
                        soc_field_t field, sal_mac_addr_t mac, int flags);

void soc_mem_base_to_wide_entry_conv(int unit, soc_mem_t dest_mem, soc_mem_t src_mem, 
                                     uint32 *dest, uint32 *src[4], uint8 conv_type);

/* These functions accelerate bulk memory operations to a single table */
extern soc_field_info_t *soc_mem_fieldinfo_get(int unit, soc_mem_t mem,
                                               soc_field_t field);
extern uint32 *soc_meminfo_fieldinfo_field_get(const uint32 *entbuf,
                                               soc_mem_info_t *meminfo,
                                               soc_field_info_t *fieldinfo,
                                               uint32 *fldbuf);
extern uint32 soc_meminfo_fieldinfo_field32_get(soc_mem_info_t *meminfo,
                                                void *entry,
                                                soc_field_info_t *fieldinfo);
extern void soc_meminfo_fieldinfo_field64_get(soc_mem_info_t *meminfo,
                                              void *entry,
                                              soc_field_info_t *fieldinfo,
                                              uint64 *val64);

extern void soc_meminfo_fieldinfo_field_set(uint32 *entbuf,
                                               soc_mem_info_t *meminfo,
                                               soc_field_info_t *fieldinfo,
                                               uint32 *fldbuf);

extern void soc_meminfo_fieldinfo_field32_set(soc_mem_info_t *meminfo,
                                                void *entry,
                                                soc_field_info_t *fieldinfo, 
                                                uint32 value);

extern void soc_meminfo_fieldinfo_field64_set(soc_mem_info_t *meminfo,
                                                void *entry,
                                                soc_field_info_t *fieldinfo, 
                                                uint64 val64);

/* Compacted memory access functions for bulk operations */
typedef struct soc_memacc_s {
    soc_mem_info_t *minfo;
    soc_field_info_t *finfo;
} soc_memacc_t;

extern int soc_memacc_init(int unit, soc_mem_t mem, soc_field_t fld,
                           soc_memacc_t *memacc);

extern uint32 *soc_memacc_field_get(soc_memacc_t *memacc,
                                    const uint32 *entbuf,
                                    uint32 *fldbuf);
extern uint32 soc_memacc_field32_get(soc_memacc_t *memacc,
                                     void *entry);
extern void soc_memacc_field64_get(soc_memacc_t *memacc,
                                   void *entry,
                                   uint64 *val64);
extern void soc_memacc_mac_addr_get(soc_memacc_t *memacc, void *entry,
                                    sal_mac_addr_t mac);

extern void soc_memacc_field_set(soc_memacc_t *memacc,
                                    uint32 *entbuf,
                                    uint32 *fldbuf);
extern void soc_memacc_field32_set(soc_memacc_t *memacc,
                                     void *entry, uint32 value);
extern void soc_memacc_field64_set(soc_memacc_t *memacc,
                                   void *entry,
                                   uint64 val64);
extern void soc_memacc_mac_addr_set(soc_memacc_t *memacc, void *entry,
                                    const sal_mac_addr_t mac);

#define SOC_MEMACC_FIELD_LENGTH(_memacc_) ((_memacc_)->finfo->len)
#define SOC_MEMACC_VALID(_memacc_) \
        ((NULL != (_memacc_)->minfo) && (NULL != (_memacc_)->finfo))

#define SOC_MEMACC_INVALID_SET(_memacc_) \
        ((_memacc_)->minfo) = NULL; \
        ((_memacc_)->finfo) = NULL


/* Routines to register/unregister user call back routine for memory snooping */
void soc_mem_snoop_register(int unit, soc_mem_t mem, uint32 flags, 
                           soc_mem_snoop_cb_t snoop_cb, void *user_data);

void soc_mem_snoop_unregister(int unit, soc_mem_t mem); 

#endif	/* !_SOC_MEMORY_H */
