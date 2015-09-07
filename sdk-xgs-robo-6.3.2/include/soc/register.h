/*
 * $Id: register.h 1.37 Broadcom SDK $
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
 * File:        register.h
 * Purpose:     Base definitions for register types
 * Requires:    
 */

#ifndef _SOC_REGISTER_H
#define _SOC_REGISTER_H

#include <soc/defs.h>
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
#include <soc/mcm/allenum.h>
#endif
#ifdef BCM_ROBO_SUPPORT
#include <soc/robo/mcm/allenum.h>
#endif
#include <soc/field.h>

/* argument to soc_reg_addr() */
#define    REG_PORT_ANY        (-10)

/* use bit 31 to indicate the port argument is an instance id */
#define SOC_REG_ADDR_INSTANCE_BP         (31)
#define SOC_REG_ADDR_INSTANCE_MASK       (1<<SOC_REG_ADDR_INSTANCE_BP)

#define _SOC_MAX_REGLIST        (2 * SOC_MAX_NUM_PORTS * SOC_MAX_NUM_COS)


#define _SOC_ROBO_MAX_COSREG         (SOC_ROBO_MAX_NUM_COS)
#define _SOC_ROBO_MAX_PORTREG        (SOC_ROBO_MAX_NUM_PORTS)
#define _SOC_ROBO_MAX_REGLIST        (2 * SOC_ROBO_MAX_NUM_PORTS * SOC_ROBO_MAX_NUM_COS)




#define FE_GET(unit, port, reg, gth_reg, fld, var)  \
    soc_reg_field_get(unit, reg, var, fld)

#define FE_SET(unit, port, reg, gth_reg, fld, var, val)  \
    soc_reg_field_set(unit, reg, &var, fld, val)

#define FE_READ(reg, gth_reg, unit, port, val_p) \
    soc_reg32_get(unit, reg, port, 0, val_p)

#define FE_WRITE(reg, gth_reg, unit, port, val) \
    soc_reg32_set(unit, reg, port, 0, val)

/*************ABOVE 64 REGS******************/

#define SOC_REG_ABOVE_64_MAX_SIZE_U32 20 

typedef uint32 soc_reg_above_64_val_t[SOC_REG_ABOVE_64_MAX_SIZE_U32];

#define SOC_REG_ABOVE_64_SET_PATTERN(reg_above_64_val, patern) \
    sal_memset(reg_above_64_val, patern, SOC_REG_ABOVE_64_MAX_SIZE_U32*4);
    
#define SOC_REG_ABOVE_64_CLEAR(reg_above_64_val) \
    SOC_REG_ABOVE_64_SET_PATTERN(reg_above_64_val, 0);

#define SOC_REG_ABOVE_64_ALLONES(reg_above_64_val) \
    SOC_REG_ABOVE_64_SET_PATTERN(reg_above_64_val, 0xFFFFFFFF);
     
#define SOC_REG_ABOVE_64_CREATE_MASK(reg_above_64_val, len, start) \
    SOC_REG_ABOVE_64_CLEAR(reg_above_64_val) \
    SHR_BITSET_RANGE(reg_above_64_val, start, len);
    
#define SOC_REG_ABOVE_64_NOT(reg_above_64_val) \
    SHR_BITNEGATE_RANGE(reg_above_64_val, 0, SOC_REG_ABOVE_64_MAX_SIZE_U32*32, reg_above_64_val);
 
#define SOC_REG_ABOVE_64_AND(reg_above_64_val1, reg_above_64_val2) \
    SHR_BITAND_RANGE(reg_above_64_val1, reg_above_64_val2, 0, SOC_REG_ABOVE_64_MAX_SIZE_U32*32, reg_above_64_val1);   

#define SOC_REG_ABOVE_64_OR(reg_above_64_val1, reg_above_64_val2) \
    SHR_BITOR_RANGE(reg_above_64_val1, reg_above_64_val2, 0, SOC_REG_ABOVE_64_MAX_SIZE_U32*32, reg_above_64_val1);   

#define SOC_REG_ABOVE_64_XOR(reg_above_64_val1, reg_above_64_val2) \
    SHR_BITXOR_RANGE(reg_above_64_val1, reg_above_64_val2, 0, SOC_REG_ABOVE_64_MAX_SIZE_U32*32, reg_above_64_val1);  
    
#define SOC_REG_ABOVE_64_COPY(reg_above_64_val1, reg_above_64_val2) \
    SHR_BITCOPY_RANGE(reg_above_64_val1, 0, reg_above_64_val2, 0, SOC_REG_ABOVE_64_MAX_SIZE_U32*32);
       
#define SOC_REG_ABOVE_64_IS_ZERO(reg_above_64_val) \
    SHR_BITNULL_RANGE(reg_above_64_val, 0, SOC_REG_ABOVE_64_MAX_SIZE_U32*32) 
   
#define SOC_REG_ABOVE_64_IS_EQUAL(reg_above_64_val1, reg_above_64_val2) \
    SHR_BITEQ_RANGE(reg_above_64_val1, reg_above_64_val2, 0, SOC_REG_ABOVE_64_MAX_SIZE_U32*32)   
    
extern int soc_reg_above_64_get(int unit, soc_reg_t reg, int port, int index, 
                soc_reg_above_64_val_t data);
extern int soc_reg_above_64_set(int unit, soc_reg_t reg, int port, int index, 
                soc_reg_above_64_val_t data);
extern void soc_reg_above_64_field_get(int unit, soc_reg_t reg, soc_reg_above_64_val_t regval, 
                soc_field_t field, soc_reg_above_64_val_t field_val);
extern void soc_reg_above_64_field_set(int unit, soc_reg_t reg, soc_reg_above_64_val_t regval,
                soc_field_t field, soc_reg_above_64_val_t value);

extern int SOC_REG_IS_DYNAMIC(int unit, soc_reg_t reg);

/******************************************/

/* SOC Register Routines */

extern int soc_reg_field_length(int unit, soc_reg_t reg, soc_field_t field);
extern uint32 soc_reg_field_get(int unit, soc_reg_t reg, uint32 regval,
             soc_field_t field);
extern uint64 soc_reg64_field_get(int unit, soc_reg_t reg, uint64 regval,
               soc_field_t field);
extern void soc_reg_field_set(int unit, soc_reg_t reg, uint32 *regval,
                  soc_field_t field, uint32 value);
extern void soc_reg64_field_set(int unit, soc_reg_t reg, uint64 *regval,
                soc_field_t field, uint64 value);
extern uint32 soc_reg64_field32_get(int unit, soc_reg_t reg, uint64 regval,
               soc_field_t field);
extern void soc_reg64_field32_set(int unit, soc_reg_t reg, uint64 *regval,
                  soc_field_t field, uint32 value);
extern int soc_reg_field_validate(int unit, soc_reg_t reg, soc_field_t field, uint32 value);
extern int soc_reg64_field_validate(int unit, soc_reg_t reg, soc_field_t field, uint64 value);

extern uint32 soc_reg_addr(int unit, soc_reg_t reg, int port, int index);
extern uint32 soc_reg_addr_get(int unit, soc_reg_t reg, int port, int index, 
                               int *blk, uint8 *acc_type);
extern uint32 soc_reg_datamask(int unit, soc_reg_t reg, int flags);
extern uint64 soc_reg64_datamask(int unit, soc_reg_t reg, int flags);
extern void   soc_reg_above_64_datamask(int unit, soc_reg_t reg, int flags, soc_reg_above_64_val_t datamask);
extern int soc_reg_fields32_modify(int unit, soc_reg_t reg, soc_port_t port,
                                   int field_count, soc_field_t *fields,
                                   uint32 *values);
extern int soc_reg_field32_modify(int unit, soc_reg_t reg, soc_port_t port, 
                                  soc_field_t field, uint32 value);
extern int soc_reg_field_valid(int unit, soc_reg_t reg, soc_field_t field);

/* Get register length in bytes */
extern int soc_reg_bytes(int unit, soc_reg_t reg);
#define SOC_REG_FIELD_VALID(_u_,_reg_,_fld_) \
    soc_reg_field_valid(_u_, _reg_, _fld_)

extern int soc_reg_port_idx_valid(int unit, soc_reg_t reg, soc_port_t port, int idx);

#define SOC_REG_PORT_VALID(_u_,_reg_,_port_) \
    soc_reg_port_idx_valid(_u_, _reg_, _port_, 0)
#define SOC_REG_PORT_IDX_VALID(_u_,_reg_,_port_,_idx_) \
        soc_reg_port_idx_valid(_u_, _reg_, _port_, _idx_)

void soc_reg_init(void);

typedef struct soc_large_reg_info_t {
    uint32 size; /*Size in uint32*/
    uint32* reset;
    uint32* mask;
} soc_reg_above_64_info_t;

/* Flags for memory snooping callback register */
#define SOC_REG_SNOOP_WRITE     0x00000001 /* snooping write operations */
#define SOC_REG_SNOOP_READ      0x00000002 /* snooping read operations */


/* additional information for registers which are arrays */
/* To be used if in soc_reg_info_t, the SOC_REG_FLAG_REG_ARRAY bit is set */
typedef struct soc_reg_array_info_t { /* additional information for register which are arrays */
    uint32 element_skip; /* how many bytes to skip between array elements */
} soc_reg_array_info_t;

typedef void (*soc_reg_snoop_cb_t) (int unit, soc_reg_t reg, uint32 flags,
                                    uint32 data_hi, uint32 data_lo, 
                                    void *user_data);

typedef struct soc_reg_info_t {
    soc_block_types_t  block;
    soc_regtype_t      regtype;        /* Also indicates invalid */
    int                numels;          /* If array, num els in array. */
                                        /* Otherwise -1. */
    uint32             offset;        /* Includes 2-bit form field */
#define SOC_REG_FLAG_64_BITS (1<<0)     /* Register is 64 bits wide */
#define SOC_REG_FLAG_COUNTER (1<<1)     /* Register is a counter */
#define SOC_REG_FLAG_ARRAY   (1<<2)     /* Register is an array */
#define SOC_REG_FLAG_NO_DGNL (1<<3)     /* Array does not have diagonal els */
#define SOC_REG_FLAG_RO      (1<<4)     /* Register is write only */
#define SOC_REG_FLAG_WO      (1<<5)     /* Register is read only */
#define SOC_REG_FLAG_ED_CNTR (1<<6)     /* Counter of discard/error */
#define SOC_REG_FLAG_SPECIAL (1<<7)     /* Counter requires special
                                           processing */
#define    SOC_REG_FLAG_EMULATION    (1<<8)    /* Available only in emulation */
#define    SOC_REG_FLAG_VARIANT1    (1<<9)    /* Not available in chip variants  */
#define    SOC_REG_FLAG_VARIANT2    (1<<10)    /* -- '' -- */
#define    SOC_REG_FLAG_VARIANT3    (1<<11)    /* -- '' -- */
#define    SOC_REG_FLAG_VARIANT4    (1<<12)    /* -- '' -- */
#define SOC_REG_FLAG_32_BITS    (1<<13) /* Register is 32 bits wide */
#define SOC_REG_FLAG_16_BITS    (1<<14) /* Register is 16 bits wide */
#define SOC_REG_FLAG_8_BITS     (1<<15) /* Register is 8 bits wide */
#define SOC_REG_FLAG_ARRAY2     (1<<16) /* Register is an array with stride 2*/
#define SOC_REG_FLAG_ACCTYPE    (7<<17) /* Access type for the register */
#define SOC_REG_FLAG_ACCSHIFT   17      /* Shift corresponding to ACCTYPE */

#define SOC_REG_FLAG_ABOVE_64_BITS (1<<20)     /* Register is above 64 bits wide */
#define SOC_REG_FLAG_REG_ARRAY  (1<<21)     /* This is a register array with indexed access */
#define SOC_REG_FLAG_INTERRUPT  (1<<22)     /* This is register is an interrupt */
#define SOC_REG_FLAG_GENERAL_COUNTER (1<<23) /* This is a general counter */
#define SOC_REG_FLAG_SIGNAL (1<<24) /* this is signal register */
#define    SOC_REG_FLAG_IGNORE_DEFAULT (1<<25) /* this is register, default value of it should be ignored */


    uint32             flags;
    int                nFields;
    soc_field_info_t   *fields;
#if !defined(SOC_NO_RESET_VALS)||defined(BCM_ROBO_SUPPORT)
    uint32             rst_val_lo;
    uint32             rst_val_hi;      /* 64-bit regs only */
    uint32             rst_mask_lo;     /* 1 where resetVal is valid */
    uint32             rst_mask_hi;     /* 64-bit regs only */
#endif
    int                ctr_idx;         /* Counters only; sw idx */
    int                numelportlist_idx;  /* Per-Port Registers Only */
    soc_reg_snoop_cb_t snoop_cb;         /* UserCallBack for snooping register*/
    void               *snoop_user_data; /* User data for callback function   */
    uint32             snoop_flags;      /* Snooping flags SOC_REG_SNOOP_* */
} soc_reg_info_t;

typedef struct soc_regaddrinfo_t {
    uint32        addr;
    int            valid;
    soc_reg_t        reg;        /* INVALIDr if not used */
    int            idx;            /* If array, this is an index */
    soc_block_t        block;        /* SOC_BLK_NONE if not used */
    soc_port_t        port;        /* -1 if not used */
    soc_cos_t        cos;        /* -1 if not used */
    uint8               acc_type;
    soc_field_t        field;        /* INVALIDf for entire reg */
} soc_regaddrinfo_t;

typedef struct soc_regaddrlist_t {
    int            count;
    soc_regaddrinfo_t    *ainfo;
} soc_regaddrlist_t;

typedef struct soc_numelport_set_t {
    int f_idx;
    int l_idx;
    int pl_idx;
} soc_numelport_set_t;

#ifdef BCM_CMICM_SUPPORT
typedef struct soc_cmicm_reg_t {
    uint32 addr;
    char *name;
    soc_reg_t reg;
} soc_cmicm_reg_t;

extern soc_cmicm_reg_t *soc_cmicm_reg_get(uint32 idx);
extern soc_reg_t soc_cmicm_addr_reg(uint32 addr);
extern char *soc_cmicm_addr_name(uint32 addr);
#endif

extern int soc_regaddrlist_alloc(soc_regaddrlist_t *addrlist);
extern int soc_regaddrlist_free(soc_regaddrlist_t *addrlist);

extern int soc_robo_regaddrlist_alloc(soc_regaddrlist_t *addrlist);
extern int soc_robo_regaddrlist_free(soc_regaddrlist_t *addrlist);

/* Function for register iteration. */
typedef int (*soc_reg_iter_f)(int unit, soc_regaddrinfo_t *ainfo, void *data);

extern void soc_regaddrinfo_get(int unit, soc_regaddrinfo_t *ainfo,
             uint32 addr);
extern void soc_regaddrinfo_extended_get(int unit, soc_regaddrinfo_t *ainfo,
                                         soc_block_t block, int acc_type,
                                         uint32 addr);
extern void soc_cpuregaddrinfo_get(int unit, soc_regaddrinfo_t *ainfo,
                            uint32 addr);

extern int soc_reg_iterate(int unit, soc_reg_iter_f do_it, void *data);

extern void soc_reg_sprint_addr(int unit, char *bp,
                                soc_regaddrinfo_t *ainfo);

extern void soc_robo_regaddrinfo_get(int unit, soc_regaddrinfo_t *ainfo,
             uint32 addr);

extern int soc_robo_reg_iterate(int unit, soc_reg_iter_f do_it, void *data);

extern void soc_robo_reg_sprint_addr(int unit, char *bp,
                                soc_regaddrinfo_t *ainfo);

extern void soc_reg_sprint_data(int unit, char *bp, char *infix,
                                soc_reg_t reg, uint32 value);
/* Routines to register/unregister UserCallBack routine for register snooping */
void soc_reg_snoop_register(int unit, soc_reg_t reg, uint32 flags, 
                           soc_reg_snoop_cb_t snoop_cb, void *user_data);

void soc_reg_snoop_unregister(int unit, soc_reg_t reg); 

/* In soc_common.c */
extern char *soc_regtypenames[];
extern int soc_regtype_gran[];

#endif    /* !_SOC_REGISTER_H */
