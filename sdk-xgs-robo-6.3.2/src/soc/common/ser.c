/*
 * $Id: ser.c 1.55.2.4 Broadcom SDK $
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
 * SER common code.
 *
 * Common code for memory SER protection.
 */

#include <sal/core/libc.h>
#include <sal/core/dpc.h>
#include <soc/drv.h>
#include <soc/debug.h>
#include <soc/util.h>
#include <soc/mem.h>
#include <soc/l2x.h>
#include <soc/l2u.h>
#include <soc/dma.h>
#ifdef BCM_CMICM_SUPPORT
#include <soc/cmicm.h>
#endif

#if defined(BCM_TRIUMPH2_SUPPORT)
#include <soc/triumph2.h>
#endif /* BCM_TRIUMPH2_SUPPORT */
#if defined(BCM_TRIDENT_SUPPORT)
#include <soc/trident.h>
#endif /* BCM_TRIDENT_SUPPORT */
#if defined(BCM_TRIDENT2_SUPPORT)
#include <soc/trident2.h>
#endif /* BCM_TRIDENT2_SUPPORT */
#if defined(BCM_TRIUMPH3_SUPPORT)
#include <soc/triumph3.h>
#endif /* BCM_TRIUMPH3_SUPPORT */
#if defined(BCM_HELIX4_SUPPORT)
#include <soc/helix4.h>
#endif /* BCM_HELIX4_SUPPORT */
#if defined(BCM_KATANA_SUPPORT)
#include <soc/katana.h>
#endif /* BCM_KATANA_SUPPORT */
#if defined(BCM_KATANA2_SUPPORT)
#include <soc/katana2.h>
#endif /* BCM_KATANA2_SUPPORT */
#if defined(BCM_ENDURO_SUPPORT)
#include <soc/enduro.h>
#endif /* BCM_ENDURO_SUPPORT */

#if defined(BCM_XGS_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) 

#define _SOC_DRV_MEM_CHK_L2_MEM(mem) \
        (mem == L2Xm || mem == L2_ENTRY_1m || mem == L2_ENTRY_2m)

/* Keep this size in sync with the items in the 2 lists below */
#define SOC_SER_REG_CACHE_MAX 18

#define _SOC_SER_CACHE_REG_CASES   \
    case DMVOQ_WRED_CONFIGr:       \
    case GLOBAL_SP_WRED_CONFIGr:   \
    case MAXBUCKETCONFIG_64r:      \
    case MINBUCKETCONFIG_64r:      \
    case PG_RESET_FLOOR_CELLr:     \
    case PG_RESET_OFFSET_CELLr:    \
    case PORT_SHARED_LIMIT_CELLr:  \
    case PORT_SP_WRED_CONFIGr:     \
    case S2_MAXBUCKETCONFIG_64r:   \
    case S2_MINBUCKETCONFIG_64r:   \
    case S3_MAXBUCKETCONFIG_64r:   \
    case S3_MINBUCKETCONFIG_64r:   \
    case S2_COSWEIGHTSr:           \
    case S3_COSWEIGHTSr:           \
    case WRED_CONFIGr:             \
    case ING_TRILL_ADJACENCYr:     \
    case EGR_VLAN_CONTROL_2r:      \
    case EGR_PVLAN_EPORT_CONTROLr:

static int _soc_ser_reg_cache_list[SOC_SER_REG_CACHE_MAX] = {
    DMVOQ_WRED_CONFIGr,      /* 1 */ 
    GLOBAL_SP_WRED_CONFIGr,  /* 2 */ 
    MAXBUCKETCONFIG_64r,     /* 3 */ 
    MINBUCKETCONFIG_64r,     /* 4 */ 
    PG_RESET_FLOOR_CELLr,    /* 5 */ 
    PG_RESET_OFFSET_CELLr,   /* 6 */ 
    PORT_SHARED_LIMIT_CELLr, /* 7 */ 
    PORT_SP_WRED_CONFIGr,    /* 8 */ 
    S2_MAXBUCKETCONFIG_64r,  /* 9 */ 
    S2_MINBUCKETCONFIG_64r,  /* 10 */
    S3_MAXBUCKETCONFIG_64r,  /* 11 */
    S3_MINBUCKETCONFIG_64r,  /* 12 */
    S2_COSWEIGHTSr,          /* 13 */
    S3_COSWEIGHTSr,          /* 14 */
    WRED_CONFIGr,            /* 15 */
    ING_TRILL_ADJACENCYr,    /* 16 */
    EGR_VLAN_CONTROL_2r,     /* 17 */
    EGR_PVLAN_EPORT_CONTROLr /* 18 */
};

#define _SOC_SER_CLEAR_REG_CASES    \
    case GLOBAL_SP_WRED_AVG_QSIZEr: \
    case PORT_SP_WRED_AVG_QSIZEr:   \
    case WRED_AVG_QSIZEr:           \
    case VOQ_WRED_AVG_QSIZEr:       \
    case MINBUCKETr:                \
    case MAXBUCKETr:                \
    case S3_MINBUCKETr:             \
    case S3_MAXBUCKETr:             \
    case S2_MINBUCKETr:             \
    case S2_MAXBUCKETr:             \
    case S3_WERRCOUNTr:             \
    case S2_WERRCOUNTr:

#define _SOC_MAX_PORTS_NUMS    170 
#define _SOC_SER_REG_INDEX_MAX  79 

typedef struct _soc_ser_reg_cache_s {
    uint64 data[_SOC_MAX_PORTS_NUMS][_SOC_SER_REG_INDEX_MAX];
} _soc_ser_reg_cache_t;
typedef struct _soc_ser_reg_cache_array_s {
     _soc_ser_reg_cache_t reg_cache[SOC_SER_REG_CACHE_MAX];
} _soc_ser_reg_cache_array_t;
_soc_ser_reg_cache_array_t *_soc_ser_reg_cache[SOC_MAX_NUM_DEVICES] = {NULL};

static soc_ser_functions_t *_soc_ser_functions[SOC_MAX_NUM_DEVICES];

/* scrub_load: 0=scrub, 1=load cache */
int soc_ser_reg_load_scrub(int unit, int scrub_load)
{
    uint64 rval64;
    soc_reg_t reg;
    int r, i, blk, port, mindex;
    soc_block_types_t regblktype;
    
    /* Access all cacheable regs (store in cache if requested) */
    for (r = 0; r < SOC_SER_REG_CACHE_MAX; r++) {
        reg = _soc_ser_reg_cache_list[r];
        if (!SOC_REG_IS_VALID(unit, reg)) {
            continue;
        }
        if (SOC_REG_INFO(unit, reg).regtype == soc_genreg) {
            SOC_IF_ERROR_RETURN
                (soc_reg_get(unit, reg, REG_PORT_ANY, 0, &rval64));
            if (scrub_load) {
                soc_ser_reg_cache_set(unit, reg, 0, 0, rval64);
            }
            soc_cm_debug(DK_VERBOSE, "Reg: %s port:%d index:%d\n", SOC_REG_NAME(unit, reg), 0, 0);
        } else if (SOC_REG_INFO(unit, reg).regtype == soc_cosreg) {
            regblktype = SOC_REG_INFO(unit, reg).block;
            for (i = 0; i < NUM_COS(unit); i++) {
                SOC_BLOCKS_ITER(unit, blk, regblktype) {
                    port = SOC_BLOCK_PORT(unit, blk);
                    SOC_IF_ERROR_RETURN
                        (soc_reg_get(unit, reg, port, i, &rval64));
                    if (scrub_load) {
                        soc_ser_reg_cache_set(unit, reg, port, i, rval64);
                    }
                    soc_cm_debug(DK_VERBOSE, "Reg: %s port:%d index:%d\n", SOC_REG_NAME(unit, reg), port, i);
                }
            }
        } else if (SOC_REG_INFO(unit, reg).regtype == soc_portreg) {
            mindex = 1;
            switch (reg) {
            case DMVOQ_WRED_CONFIGr:
            case S3_COSWEIGHTSr: mindex = 72; break;
            case PG_RESET_OFFSET_CELLr:
            case PORT_SHARED_LIMIT_CELLr:
            case WRED_CONFIGr: mindex = 8; break;
            case PORT_SP_WRED_CONFIGr:
            case S2_MINBUCKETCONFIG_64r:
            case S2_MAXBUCKETCONFIG_64r:
            case S3_MINBUCKETCONFIG_64r:
            case S3_MAXBUCKETCONFIG_64r: mindex = 4; break;
            case S2_COSWEIGHTSr: mindex = 12; break;
            default: break;
            }
            PBMP_ALL_ITER(unit, port) {
                switch (reg) {
                case DMVOQ_WRED_CONFIGr: 
                    if (!SOC_INFO(unit).port_num_ext_cosq[port]) {
                        continue;
                    }
                    break;
                case MINBUCKETCONFIG_64r:
                case MINBUCKETr:
                case MAXBUCKETCONFIG_64r:
                case MAXBUCKETr:
#ifdef BCM_ENDURO_SUPPORT
                    if (SOC_IS_ENDURO(unit)) {
                        if (IS_CPU_PORT(unit, port)) {
                            continue;
                        } else if (IS_XQ_PORT(unit, port)) {
                            mindex = 26;
                        } else {
                            mindex = 8;
                        }
                    } else {
#endif
                    if (IS_CPU_PORT(unit, port)) {
                        mindex = 48;
                    } else if (IS_LB_PORT(unit, port)) {
                        mindex = 5;
                    } else if (!SOC_INFO(unit).port_num_ext_cosq[port]) {
                        mindex = 15;
                    } else {
                        mindex = 79;
                    }
#ifdef BCM_ENDURO_SUPPORT
                    }
#endif
                    break;
                case PORT_SP_WRED_CONFIGr:
                    if (IS_CPU_PORT(unit, port) || IS_LB_PORT(unit, port)) {
                        continue;
                    }
                    break;
                case S2_MINBUCKETCONFIG_64r:
                case S2_MAXBUCKETCONFIG_64r:
                case S3_MINBUCKETCONFIG_64r:
                case S3_MAXBUCKETCONFIG_64r:
                case S2_COSWEIGHTSr:
                    if (IS_CPU_PORT(unit, port)) {
                        continue;
                    }
                    break;
                case S3_COSWEIGHTSr:
                    if (!SOC_INFO(unit).port_num_ext_cosq[port]) {
                        mindex = 8;
                    }
                    break;
                case WRED_CONFIGr:
                    if (IS_CPU_PORT(unit, port) || IS_LB_PORT(unit, port)) {
                        continue;
                    }
                    break;
                default: break;
                }
                for (i = 0; i < mindex; i++) {
                    SOC_IF_ERROR_RETURN
                        (soc_reg_get(unit, reg, port, i, &rval64));
                    if (scrub_load) {
                        soc_ser_reg_cache_set(unit, reg, port, i, rval64);
                    }
                    soc_cm_debug(DK_VERBOSE, "Reg: %s port:%d index:%d\n", SOC_REG_NAME(unit, reg), port, i);
                }
            }
        }
    }
    return SOC_E_NONE;
}

int
soc_ser_reg_cache_clear(int unit, soc_reg_t reg, int port)
{
    int i;

    if (reg == INVALIDr) {
        sal_memset(&_soc_ser_reg_cache[unit]->reg_cache, 0, sizeof(_soc_ser_reg_cache_t));
    } else {
        for (i = 0; i < SOC_SER_REG_CACHE_MAX; i++) {
            if (reg == _soc_ser_reg_cache_list[i]) {
                if (port == REG_PORT_ANY) {
                    sal_memset(&_soc_ser_reg_cache[unit]->reg_cache[i].data[0][0],
                               0, sizeof(_soc_ser_reg_cache_t));
                } else {
                    sal_memset(&_soc_ser_reg_cache[unit]->reg_cache[i].data[port][0],
                               0, sizeof(_soc_ser_reg_cache_t));
                }
                break;
            }
        }
    }
    return SOC_E_NONE;
}

int
soc_ser_reg_cache_init(int unit)
{
    if (_soc_ser_reg_cache[unit] == NULL) {
        _soc_ser_reg_cache[unit] = sal_alloc(sizeof(_soc_ser_reg_cache_array_t), "reg-cache");
    }
    if (SOC_WARM_BOOT(unit) && (SOC_CONTROL(unit)->mem_scache_ptr == NULL)) {
        return soc_ser_reg_load_scrub(unit, 1);
    } else {
        return soc_ser_reg_cache_clear(unit, INVALIDr, REG_PORT_ANY);
    }
}

void
soc_ser_reg_cache_info(int unit, int *count, int *size)
{
    *count = SOC_SER_REG_CACHE_MAX;
    *size = sizeof(_soc_ser_reg_cache_array_t);
}

int 
soc_ser_reg_cache_set(int unit, soc_reg_t reg, int port, int index, uint64 data)
{
    int i;

    if (_soc_ser_reg_cache[unit] == NULL) {
        return SOC_E_NONE;
    }
    switch (reg) {
    _SOC_SER_CACHE_REG_CASES
        break;
    default: 
        return SOC_E_NONE;
    }
    assert(port < _SOC_MAX_PORTS_NUMS);
    assert(index < _SOC_SER_REG_INDEX_MAX);
    if (port >= _SOC_MAX_PORTS_NUMS) {
        return SOC_E_PARAM;
    }
    if (index >= _SOC_SER_REG_INDEX_MAX) {
        return SOC_E_PARAM;
    }
    for (i = 0; i < SOC_SER_REG_CACHE_MAX; i++) {
        if (reg == _soc_ser_reg_cache_list[i]) {
            if (port == REG_PORT_ANY) {
                port = 0;
            }
            if (index < 0) {
                index = 0;
            }
            soc_cm_debug(DK_VERBOSE+DK_REG,
                         "Unit %d: Set cache: reg:%d port:%d index:%d "
                         "data:0x%x%x\n", unit, reg, port, index,
                         COMPILER_64_HI(data), COMPILER_64_LO(data));
            _soc_ser_reg_cache[unit]->reg_cache[i].data[port][index] = data;
            break;
        }
    }
    return SOC_E_NONE;
}

int
soc_ser_reg32_cache_set(int unit, soc_reg_t reg, int port, int index, uint32 data)
{
    uint64 val;

    if (_soc_ser_reg_cache[unit] == NULL) {
        return SOC_E_NONE;
    }
    COMPILER_64_SET(val, 0, data);
    return soc_ser_reg_cache_set(unit, reg, port, index, val);
}

int 
soc_ser_reg_cache_get(int unit, soc_reg_t reg, int port, int index, uint64 *data)
{
    int i;

    if (_soc_ser_reg_cache[unit] == NULL) {
        return SOC_E_UNAVAIL;
    }
    switch (reg) {
    _SOC_SER_CACHE_REG_CASES
        break;
    default: 
        return SOC_E_UNAVAIL;
    }
    assert(port < _SOC_MAX_PORTS_NUMS);
    assert(index < _SOC_SER_REG_INDEX_MAX);
    if (port >= _SOC_MAX_PORTS_NUMS) {
        return SOC_E_PARAM;
    }
    if (index >= _SOC_SER_REG_INDEX_MAX) {
        return SOC_E_PARAM;
    }
    for (i = 0; i < SOC_SER_REG_CACHE_MAX; i++) {
        if (reg == _soc_ser_reg_cache_list[i]) {
            if (port == REG_PORT_ANY) {
                port = 0;
            }
            if (index < 0) {
                index = 0;
            }
            *data = _soc_ser_reg_cache[unit]->reg_cache[i].data[port][index];
            soc_cm_debug(DK_VERBOSE+DK_REG,
                         "Unit %d: Get cache: reg:%d port:%d index:%d "
                         "data:0x%x%x\n", unit, reg, port, index,
                         COMPILER_64_HI(*data), COMPILER_64_LO(*data));
            break;
        }
    }
    return SOC_E_NONE;
}

int 
soc_ser_reg32_cache_get(int unit, soc_reg_t reg, int port, int index, uint32 *data)
{
    uint64 val;
   
    if (_soc_ser_reg_cache[unit] == NULL) {
        return SOC_E_UNAVAIL;
    }
    COMPILER_64_ZERO(val); 
    SOC_IF_ERROR_RETURN(soc_ser_reg_cache_get(unit, reg, port, index, &val));
    *data = COMPILER_64_LO(val);
    return SOC_E_NONE;
}

int SOC_REG_IS_DYNAMIC(int unit, soc_reg_t reg)
{
    switch (reg) {
    _SOC_SER_CLEAR_REG_CASES
        return TRUE;
    default: return FALSE;
    }
}

int
soc_ser_reg_clear(int unit, soc_reg_t reg, int port, int index, uint64 data)
{
    if (SOC_REG_IS_DYNAMIC(unit, reg)) {
        SOC_IF_ERROR_RETURN(soc_reg_set_nocache(unit, reg, port, index, data));
    }
    return SOC_E_NONE;
}

void
soc_ser_function_register(int unit, soc_ser_functions_t *functions)
{
    _soc_ser_functions[unit] = functions;
}

int
soc_ser_stat_error(int unit, int port)
{
    int fixed = 0;
    soc_stat_t *stat = SOC_STAT(unit);

    if ((_soc_ser_functions[unit] != NULL) &&
        (_soc_ser_functions[unit]->_soc_ser_stat_nack_f != NULL)) {
        (*(_soc_ser_functions[unit]->_soc_ser_stat_nack_f))(unit, &fixed);
    } else {
        return SOC_E_UNAVAIL;
    }

    if (!fixed) {
        return SOC_E_INTERNAL;
    }
    stat->ser_err_stat++;
    return SOC_E_NONE;
}

int
soc_process_ser_parity_error(int unit, 
                             _soc_ser_parity_info_t *_ser_parity_info,
                             int parity_err_type)
{
    int info_ix, index_min, index_max, copyno, mem_index;
    uint32 start_addr, end_addr;
    uint32 fail_count, addr;
    uint32 pipe_acc;
    _soc_ser_parity_info_t *cur_spi;
    _soc_ser_correct_info_t spci = {0};
    soc_stat_t *stat = SOC_STAT(unit);

    SOC_IF_ERROR_RETURN(READ_CMIC_SER_FAIL_CNTr(unit, &fail_count));

    if (!fail_count) {
        soc_cm_debug(DK_ERR,
                     "Unit %d: SER parity failure without valid count\n",
                     unit);
    } else {
        SOC_IF_ERROR_RETURN(READ_CMIC_SER_FAIL_ENTRYr(unit, &addr));

        info_ix = 0;
        while (_ser_parity_info[info_ix].mem != INVALIDm) {
            cur_spi = &(_ser_parity_info[info_ix]);
            index_min = soc_mem_index_min(unit, cur_spi->mem);
            index_max = soc_mem_index_max(unit, cur_spi->mem);
            pipe_acc = cur_spi->ser_flags & _SOC_SER_FLAG_ACC_TYPE_MASK;

            SOC_MEM_BLOCK_ITER(unit, cur_spi->mem, copyno) {
                break;
            }
            start_addr =
                soc_mem_addr(unit, cur_spi->mem, 0, copyno, index_min);
            end_addr =
                soc_mem_addr(unit, cur_spi->mem, 0, copyno, index_max);

            if (0 != pipe_acc) {
                /* Override ACC_TYPE in addresses */
                start_addr &= ~(_SOC_MEM_ADDR_ACC_TYPE_MASK <<
                                _SOC_MEM_ADDR_ACC_TYPE_SHIFT);
                start_addr |= (pipe_acc & _SOC_MEM_ADDR_ACC_TYPE_MASK) <<
                    _SOC_MEM_ADDR_ACC_TYPE_SHIFT;

                end_addr &= ~(_SOC_MEM_ADDR_ACC_TYPE_MASK <<
                                _SOC_MEM_ADDR_ACC_TYPE_SHIFT);
                end_addr |= (pipe_acc & _SOC_MEM_ADDR_ACC_TYPE_MASK) <<
                    _SOC_MEM_ADDR_ACC_TYPE_SHIFT;
            }

            if ((addr >= start_addr) && (addr <= end_addr)) {
                /* Addr in memory range */
                mem_index = (addr - start_addr) + index_min;
                soc_cm_debug(DK_ERR,
                             "Unit %d: %s entry %d TCAM parity error\n",
                             unit, SOC_MEM_NAME(unit, cur_spi->mem),
                             mem_index);
                soc_event_generate(unit, SOC_SWITCH_EVENT_PARITY_ERROR, 
                                   parity_err_type, addr, 
                                   0);
                stat->ser_err_tcam++;
                spci.flags = SOC_SER_SRC_MEM | SOC_SER_REG_MEM_KNOWN;
                spci.reg = INVALIDr;
                spci.mem = cur_spi->mem;
                spci.blk_type = copyno;
                spci.index = mem_index;
                SOC_IF_ERROR_RETURN(soc_ser_correction(unit, &spci));
                break;
            }
            info_ix++;
        }
    }

    SOC_IF_ERROR_RETURN(WRITE_CMIC_SER_FAIL_ENTRYr(unit, 0));
    SOC_IF_ERROR_RETURN(WRITE_CMIC_SER_FAIL_CNTr(unit, 0));
    return SOC_E_NONE;
}

int
soc_process_cmicm_ser_parity_error(int unit, 
                                   _soc_ser_parity_info_t *_ser_parity_info,
                                   int parity_err_type)
{
    uint8 at, fail = 0;
    int info_ix, index_min, index_max, phys_index_min, phys_index_max;
    int ser_mem_count, copyno, mem_index;
    uint32 start_addr, end_addr;
    uint32 fail_count, addr;
    _soc_ser_parity_info_t *cur_spi;
    _soc_ser_correct_info_t spci = {0};
    soc_stat_t *stat = SOC_STAT(unit);
    
    /* First work on ser0 */
    SOC_IF_ERROR_RETURN(READ_CMIC_SER0_FAIL_CNTr(unit, &fail_count));
    if (fail_count) {
        fail++;
        SOC_IF_ERROR_RETURN(READ_CMIC_SER0_FAIL_ENTRYr(unit, &addr));
        info_ix = 0;
        while (_ser_parity_info[info_ix].mem != INVALIDm) {
            cur_spi = &(_ser_parity_info[info_ix]);
            if (cur_spi->cmic_ser_id == 1) {
                info_ix++;
                continue;
            }
            phys_index_min = index_min =
                soc_mem_index_min(unit, cur_spi->mem);
            phys_index_max = index_max =
                soc_mem_index_max(unit, cur_spi->mem);
            if (0 != (cur_spi->ser_flags & _SOC_SER_FLAG_REMAP_READ)) {
#ifdef BCM_TRIUMPH3_SUPPORT
                if (SOC_IS_TRIUMPH3(unit)) {
                    phys_index_min =
                        soc_tr3_l3_defip_index_map(unit, 
                                                   cur_spi->mem,
                                                   index_min);
                    phys_index_max =
                        soc_tr3_l3_defip_index_map(unit, 
                                                   cur_spi->mem,
                                                   index_max);
                }
#endif /* BCM_TRIUMPH3_SUPPORT */
            }
            ser_mem_count = phys_index_max - phys_index_min + 1;
            if (!ser_mem_count) {
                info_ix++;
                continue;
            }
            SOC_MEM_BLOCK_ITER(unit, cur_spi->mem, copyno) {
                break;
            }
            start_addr =
                soc_mem_addr_get(unit, cur_spi->mem, 0, copyno,
                                 phys_index_min, &at);
            end_addr =
                soc_mem_addr_get(unit, cur_spi->mem, 0, copyno,
                                 phys_index_max, &at);
            soc_cm_debug(DK_VERBOSE, "Mem: %s addr: %x start: %x end: %x\n", 
                         SOC_MEM_NAME(unit, cur_spi->mem), addr,
                         start_addr, end_addr);

            if ((addr >= start_addr) && (addr <= end_addr)) {
                /* Addr in memory range */
                mem_index = (addr - start_addr) + phys_index_min;
                if (0 != (cur_spi->ser_flags & _SOC_SER_FLAG_REMAP_READ)) {
                    /* Now, mem_index is the physical index.
                     * The correction routines expect the logical index.
                     * Reverse the mapping.
                     */
#ifdef BCM_TRIUMPH3_SUPPORT
                    if (SOC_IS_TRIUMPH3(unit)) {
                        mem_index =
                            soc_tr3_l3_defip_index_remap(unit, cur_spi->mem, 
                                                         mem_index);
                    }
#endif /* BCM_TRIUMPH3_SUPPORT */
                }
                soc_cm_debug(DK_ERR,
                             "Unit %d: ser0 %s entry %d TCAM parity error\n",
                             unit, SOC_MEM_NAME(unit, cur_spi->mem),
                             mem_index);
                soc_event_generate(unit, SOC_SWITCH_EVENT_PARITY_ERROR, 
                                   SOC_SWITCH_EVENT_DATA_ERROR_PARITY, 
                                   cur_spi->mem | SOC_SER_ERROR_DATA_ID_OFFSET_SET,
                                   mem_index);
                stat->ser_err_tcam++;
                spci.flags = SOC_SER_SRC_MEM | SOC_SER_REG_MEM_KNOWN;
                spci.reg = INVALIDr;
                spci.mem = cur_spi->mem;
                spci.blk_type = copyno;
                spci.index = mem_index;
                SOC_IF_ERROR_RETURN(soc_ser_correction(unit, &spci));
                break;
            }
            info_ix++;
        }
        SOC_IF_ERROR_RETURN(WRITE_CMIC_SER0_FAIL_ENTRYr(unit, 0));
        SOC_IF_ERROR_RETURN(WRITE_CMIC_SER0_FAIL_CNTr(unit, 0));
    }
    
    /* Then work on ser1 */
    SOC_IF_ERROR_RETURN(READ_CMIC_SER1_FAIL_CNTr(unit, &fail_count));
    if (fail_count) {
        fail++;
        SOC_IF_ERROR_RETURN(READ_CMIC_SER1_FAIL_ENTRYr(unit, &addr));
        info_ix = 0;
        while (_ser_parity_info[info_ix].mem != INVALIDm) {
            cur_spi = &(_ser_parity_info[info_ix]);
            if (cur_spi->cmic_ser_id == 0) {
                info_ix++;
                continue;
            }
            index_min = soc_mem_index_min(unit, cur_spi->mem);
            index_max = soc_mem_index_max(unit, cur_spi->mem);
            SOC_MEM_BLOCK_ITER(unit, cur_spi->mem, copyno) {
                break;
            }
            start_addr =
                soc_mem_addr_get(unit, cur_spi->mem, 0, copyno, index_min, &at);
            end_addr =
                soc_mem_addr_get(unit, cur_spi->mem, 0, copyno, index_max, &at);

            if ((addr >= start_addr) && (addr <= end_addr)) {
                /* Addr in memory range */
                mem_index = (addr - start_addr) + index_min;
                soc_cm_debug(DK_ERR,
                             "Unit %d: ser1 %s entry %d TCAM parity error\n",
                             unit, SOC_MEM_NAME(unit, cur_spi->mem),
                             mem_index);
                soc_event_generate(unit, SOC_SWITCH_EVENT_PARITY_ERROR, 
                                   SOC_SWITCH_EVENT_DATA_ERROR_PARITY, 
                                   cur_spi->mem | SOC_SER_ERROR_DATA_ID_OFFSET_SET,
                                   mem_index);
                stat->ser_err_tcam++;
                sal_memset(&spci, 0, sizeof(spci));
                spci.flags = SOC_SER_SRC_MEM | SOC_SER_REG_MEM_KNOWN;
                spci.mem = cur_spi->mem;
                spci.blk_type = copyno;
                spci.index = mem_index;
                SOC_IF_ERROR_RETURN(soc_ser_correction(unit, &spci));
                break;
            }
            info_ix++;
        }
        SOC_IF_ERROR_RETURN(WRITE_CMIC_SER1_FAIL_ENTRYr(unit, 0));
        SOC_IF_ERROR_RETURN(WRITE_CMIC_SER1_FAIL_CNTr(unit, 0));
    }
    if (!fail) {
        soc_cm_debug(DK_ERR, "Unit %d: SER parity failure without valid count\n",
                     unit);
    }
    return SOC_E_NONE;
}

int
soc_process_cmicd_ser_parity_error(int unit, 
                                   _soc_ser_parity_info_t *_ser_parity_info,
                                   int parity_err_type)
{
    
    return SOC_E_NONE;
}

int
soc_ser_mem_nack(void *unit_vp, void *addr_vp, void *d2,
             void *d3, void *d4)
{
    int unit = PTR_TO_INT(unit_vp);

    if ((_soc_ser_functions[unit] != NULL) &&
        (_soc_ser_functions[unit]->_soc_ser_mem_nack_f != NULL)) {
        sal_dpc(*(_soc_ser_functions[unit]->_soc_ser_mem_nack_f), unit_vp, 
            addr_vp, d2, d3, d4);

        return TRUE;
    }

    return FALSE;
}

int
soc_ser_parity_error_intr(int unit)
{
    if ((_soc_ser_functions[unit] != NULL) &&
        (_soc_ser_functions[unit]->_soc_ser_parity_error_intr_f != NULL)) {
        soc_intr_disable(unit, IRQ_MEM_FAIL);
        sal_dpc(*(_soc_ser_functions[unit]->_soc_ser_parity_error_intr_f), 
            INT_TO_PTR(unit), 0, 0, 0, 0);
        return TRUE;
    }

    return FALSE;
}

int
soc_ser_parity_error_cmicm_intr(void *unit_vp, void *d1, void *d2,
             void *d3, void *d4)
{
    int unit = PTR_TO_INT(unit_vp);

    if ((_soc_ser_functions[unit] != NULL) &&
        (_soc_ser_functions[unit]->_soc_ser_parity_error_cmicm_intr_f != NULL)) {
        sal_dpc(*(_soc_ser_functions[unit]->_soc_ser_parity_error_cmicm_intr_f), 
            unit_vp, d1, d2, d3, d4);
        return TRUE;
    }

    return FALSE;
}

void
soc_ser_fail(void *unit_vp, void *addr_vp, void *d2,
             void *d3, void *d4)

{
    int unit = PTR_TO_INT(unit_vp);

    if ((_soc_ser_functions[unit] != NULL) &&
        (_soc_ser_functions[unit]->_soc_ser_fail_f != NULL)) {
        (*(_soc_ser_functions[unit]->_soc_ser_fail_f))(unit);
    }

    return;
}

STATIC int
_soc_ser_granularity(_soc_ser_parity_mode_t parity_mode)
{
    int ser_mem_granularity;

    switch (parity_mode) {
    case _SOC_SER_PARITY_MODE_2BITS:
        ser_mem_granularity = 2;
        break;
    case _SOC_SER_PARITY_MODE_4BITS: 
        ser_mem_granularity = 4;
        break;
    case _SOC_SER_PARITY_MODE_8BITS:
        ser_mem_granularity = 8;
        break;
    case _SOC_SER_PARITY_MODE_1BIT:
    default:
        ser_mem_granularity = 1;
        break;            
    }
    return ser_mem_granularity;
}

int
soc_ser_mem_clear(int unit, _soc_ser_parity_info_t *_ser_parity_info,
                  soc_mem_t mem)
{
    int info_ix, ser_mem_count, ser_mem_granularity;
    uint32 addr_valid = 0;
    _soc_ser_parity_info_t *cur_spi;

    SOC_IF_ERROR_RETURN
        (READ_CMIC_SER_PROTECT_ADDR_RANGE_VALIDr(unit, &addr_valid));

    if (!addr_valid) {
        /* No SER protection, do nothing */
        return SOC_E_NONE;
    }

    info_ix = 0;
    while (_ser_parity_info[info_ix].mem != INVALIDm) {
        if (_ser_parity_info[info_ix].mem == mem) {
            break;
        }
        info_ix++;
    }

    if ((_ser_parity_info[info_ix].mem != INVALIDm) &&
        (addr_valid & (1 << info_ix))) {
        cur_spi = &(_ser_parity_info[info_ix]);

        addr_valid &= ~(1 << info_ix);
        /* Disable SER protection on this memory */
        SOC_IF_ERROR_RETURN
            (WRITE_CMIC_SER_PROTECT_ADDR_RANGE_VALIDr(unit, addr_valid));

        ser_mem_granularity =
            _soc_ser_granularity(cur_spi->parity_mode);

        /* Flush SER memory segment for the table */
        for (ser_mem_count = cur_spi->ser_section_start;
             ser_mem_count < cur_spi->ser_section_end;
             ser_mem_count += ser_mem_granularity) {
            SOC_IF_ERROR_RETURN
                (WRITE_CMIC_SER_MEM_ADDRr(unit, ser_mem_count));
            SOC_IF_ERROR_RETURN(WRITE_CMIC_SER_MEM_DATAr(unit, 0));
        }

        addr_valid |= (1 << info_ix);
        /* Enable SER protection on this memory */
        SOC_IF_ERROR_RETURN
            (WRITE_CMIC_SER_PROTECT_ADDR_RANGE_VALIDr(unit, addr_valid));

        soc_cm_debug(DK_VERBOSE,
                     "\t%s: SER[%d-%d]\n",
                     SOC_MEM_NAME(unit, cur_spi->mem),
                     cur_spi->ser_section_start, cur_spi->ser_section_end);
    }

    return SOC_E_NONE;
}

int
soc_cmicm_ser_mem_clear(int unit, _soc_ser_parity_info_t *_ser_parity_info,
                        soc_mem_t mem)
{
    int info_ix, ser_mem_count, ser_mem_granularity;
    uint32 addr_valid = 0;
    _soc_ser_parity_info_t *cur_spi;

    /* First work on ser0 */
    SOC_IF_ERROR_RETURN
        (READ_CMIC_SER0_PROTECT_ADDR_RANGE_VALIDr(unit, &addr_valid));

    if (!addr_valid) {
        /* No SER protection, do nothing */
        goto check_ser1;
    }

    info_ix = 0;
    while (_ser_parity_info[info_ix].mem != INVALIDm) {
        if ((_ser_parity_info[info_ix].mem == mem) && 
            (_ser_parity_info[info_ix].cmic_ser_id == 0)) {
            break;
        }
        info_ix++;
    }

    if ((_ser_parity_info[info_ix].mem != INVALIDm) &&
        (addr_valid & (1 << info_ix))) {
        cur_spi = &(_ser_parity_info[info_ix]);

        addr_valid &= ~(1 << info_ix);
        /* Disable SER protection on this memory */
        SOC_IF_ERROR_RETURN
            (WRITE_CMIC_SER0_PROTECT_ADDR_RANGE_VALIDr(unit, addr_valid));

        ser_mem_granularity =
            _soc_ser_granularity(cur_spi->parity_mode);

        /* Flush SER memory segment for the table */
        for (ser_mem_count = cur_spi->ser_section_start;
             ser_mem_count < cur_spi->ser_section_end;
             ser_mem_count += ser_mem_granularity) {
            SOC_IF_ERROR_RETURN
                (WRITE_CMIC_SER0_MEM_ADDRr(unit, ser_mem_count));
            SOC_IF_ERROR_RETURN(WRITE_CMIC_SER0_MEM_DATAr(unit, 0));
        }

        addr_valid |= (1 << info_ix);
        /* Enable SER protection on this memory */
        SOC_IF_ERROR_RETURN
            (WRITE_CMIC_SER0_PROTECT_ADDR_RANGE_VALIDr(unit, addr_valid));

        soc_cm_debug(DK_VERBOSE,
                     "\t%s: SER0[%d-%d]\n",
                     SOC_MEM_NAME(unit, cur_spi->mem),
                     cur_spi->ser_section_start, cur_spi->ser_section_end);
    }

check_ser1:
    /* Then work on ser1 */
    SOC_IF_ERROR_RETURN
        (READ_CMIC_SER1_PROTECT_ADDR_RANGE_VALIDr(unit, &addr_valid));

    if (!addr_valid) {
        /* No SER protection, do nothing */
        return SOC_E_NONE;
    }

    info_ix = 0;
    while (_ser_parity_info[info_ix].mem != INVALIDm) {
        if ((_ser_parity_info[info_ix].mem == mem) && 
            (_ser_parity_info[info_ix].cmic_ser_id == 1)) {
            break;
        }
        info_ix++;
    }

    if ((_ser_parity_info[info_ix].mem != INVALIDm) &&
        (addr_valid & (1 << info_ix))) {
        cur_spi = &(_ser_parity_info[info_ix]);

        addr_valid &= ~(1 << info_ix);
        /* Disable SER protection on this memory */
        SOC_IF_ERROR_RETURN
            (WRITE_CMIC_SER0_PROTECT_ADDR_RANGE_VALIDr(unit, addr_valid));

        ser_mem_granularity =
            _soc_ser_granularity(cur_spi->parity_mode);

        /* Flush SER memory segment for the table */
        for (ser_mem_count = cur_spi->ser_section_start;
             ser_mem_count < cur_spi->ser_section_end;
             ser_mem_count += ser_mem_granularity) {
            SOC_IF_ERROR_RETURN
                (WRITE_CMIC_SER1_MEM_ADDRr(unit, ser_mem_count));
            SOC_IF_ERROR_RETURN(WRITE_CMIC_SER1_MEM_DATAr(unit, 0));
        }

        addr_valid |= (1 << info_ix);
        /* Enable SER protection on this memory */
        SOC_IF_ERROR_RETURN
            (WRITE_CMIC_SER1_PROTECT_ADDR_RANGE_VALIDr(unit, addr_valid));

        soc_cm_debug(DK_VERBOSE,
                     "\t%s: SER0[%d-%d]\n",
                     SOC_MEM_NAME(unit, cur_spi->mem),
                     cur_spi->ser_section_start, cur_spi->ser_section_end);
    }
    return SOC_E_NONE;
}

int
soc_ser_init(int unit, _soc_ser_parity_info_t *_ser_parity_info, int max_mem)
{
    int info_ix, o_info_ix;
    int index_min, index_max, copyno, ser_mem_granularity;
    int i, ser_mem_count, ser_mem_total = 0;
    uint32 start_addr, end_addr, ser_mem_addr = 0, reg_addr;
    uint32 addr_valid0 = 0;
    uint32 acc_type;
#ifdef BCM_CMICM_SUPPORT
    uint8 at;
    uint32 addr_valid1 = 0, ser_mem_addr0 = 0, ser_mem_addr1 = 0;
    uint32 ser_mem_total0 = 0, ser_mem_total1 = 0;
#endif /* BCM_CMICM_SUPPORT */
    _soc_ser_parity_info_t *cur_spi, *multi_pipe_spi;

    soc_cm_debug(DK_VERBOSE | DK_SOCMEM,
                 "Unit %d: SER parity initialization:\n", unit);

    soc_ser_function_register(unit, NULL);

    info_ix = 0;
    while (_ser_parity_info[info_ix].mem != INVALIDm) {
        cur_spi = &(_ser_parity_info[info_ix]);

        if (cur_spi->ser_flags & _SOC_SER_FLAG_SW_COMPARE) {
            if (SOC_MEM_INFO(unit, cur_spi->mem).flags & SOC_MEM_FLAG_BE) {
                soc_cm_debug(DK_ERR,
                         "\tUnit %d: SW SER init of Big Endian TCAMs is not supported: %s\n",
                             unit, SOC_MEM_NAME(unit, cur_spi->mem));
                return SOC_E_INTERNAL;
            }

            soc_cm_debug(DK_VERBOSE | DK_SOCMEM,
                         "\tSkipping HW SER init of manual scan mem: %s\n",
                         SOC_MEM_NAME(unit, cur_spi->mem));
            info_ix++;
            continue;
        }

        index_min = soc_mem_index_min(unit, cur_spi->mem);
        index_max = soc_mem_index_max(unit, cur_spi->mem);
#ifdef BCM_TRIUMPH3_SUPPORT
        if (soc_feature(unit, soc_feature_l3_defip_map) &&
            ((cur_spi->mem == L3_DEFIP_PAIR_128m) ||
             (cur_spi->mem == L3_DEFIPm))) {
            /* Override SOC memory impression for overlay
             * LPM tables. */
            index_min = 0;
            index_max = (SOC_CONTROL(unit)->l3_defip_max_tcams * 
                         SOC_CONTROL(unit)->l3_defip_tcam_size) - 1;
            if (cur_spi->mem == L3_DEFIP_PAIR_128m) {
                index_max /= 2;
            }
        }
#endif /* BCM_TRIUMPH3_SUPPORT */
        ser_mem_count = index_max - index_min + 1;
        if (!ser_mem_count) {
            soc_cm_debug(DK_VERBOSE | DK_SOCMEM, "\tSkipping empty mem: %s\n",
                         SOC_MEM_NAME(unit, cur_spi->mem));
            info_ix++;
            continue;
        }

        ser_mem_granularity = _soc_ser_granularity(cur_spi->parity_mode);
        ser_mem_count *= ser_mem_granularity;

        /* Round up to words for simplicity */
        ser_mem_count = 32 * ((ser_mem_count + 31) / 32);
        if ((ser_mem_count + ser_mem_total) > max_mem) {
            /* Can't fit requested parity bits in SER memory */
            soc_cm_debug(DK_VERBOSE | DK_SOCMEM,
                         "Unit %d: SER mem full: Skipping further config.\n", unit);
            break;
        }

        /* Parity bits fit, so on with the config */
        SOC_MEM_BLOCK_ITER(unit, cur_spi->mem, copyno) {
            break;
        }
#ifdef BCM_CMICM_SUPPORT
        if (soc_feature(unit, soc_feature_cmicm) && !SOC_IS_KATANA(unit)) {
            start_addr = soc_mem_addr_get(unit, cur_spi->mem, 0, copyno,
                                          index_min, &at);
            end_addr = soc_mem_addr_get(unit, cur_spi->mem, 0, copyno,
                                        index_max, &at);
        } else 
#endif /* BCM_CMICM_SUPPORT */
        {
            start_addr = soc_mem_addr(unit, cur_spi->mem, 0, copyno, index_min);
            end_addr = soc_mem_addr(unit, cur_spi->mem, 0, copyno, index_max);
        }

        /* Flush SER memory */
#ifdef BCM_CMICM_SUPPORT
        if (soc_feature(unit, soc_feature_cmicm) && !SOC_IS_KATANA(unit)) {
            if (cur_spi->cmic_ser_id) {
                ser_mem_addr1 = ser_mem_total1; /* Previous total */
                ser_mem_total1 += ser_mem_count; /* New total */
                if (!SOC_IS_RELOADING(unit) && !SOC_WARM_BOOT(unit)) {
                    for (i=ser_mem_addr1; i<ser_mem_total1; i++) {
                        SOC_IF_ERROR_RETURN(WRITE_CMIC_SER1_MEM_ADDRr(unit, i));
                        SOC_IF_ERROR_RETURN(WRITE_CMIC_SER1_MEM_DATAr(unit, 0));
                    }
                }
                /* Record section for mem_clear use */
                cur_spi->ser_section_start = ser_mem_addr1;
                cur_spi->ser_section_end = ser_mem_total1 - ser_mem_granularity;
            } else {
                ser_mem_addr0 = ser_mem_total0; /* Previous total */
                ser_mem_total0 += ser_mem_count; /* New total */
                if (!SOC_IS_RELOADING(unit) && !SOC_WARM_BOOT(unit)) {
                    for (i=ser_mem_addr0; i<ser_mem_total0; i++) {
                        SOC_IF_ERROR_RETURN(WRITE_CMIC_SER0_MEM_ADDRr(unit, i));
                        SOC_IF_ERROR_RETURN(WRITE_CMIC_SER0_MEM_DATAr(unit, 0));
                    }
                }
                /* Record section for mem_clear use */
                cur_spi->ser_section_start = ser_mem_addr0;
                cur_spi->ser_section_end = ser_mem_total0 - ser_mem_granularity;
            }
            ser_mem_total += ser_mem_count; /* New total */
        } else 
#endif /* BCM_CMICM_SUPPORT */
        {
            ser_mem_addr = ser_mem_total; /* Previous total */
            ser_mem_total += ser_mem_count; /* New total */
            if (!SOC_IS_RELOADING(unit) && !SOC_WARM_BOOT(unit)) {
                for (i=ser_mem_addr; i<ser_mem_total; i++) {
                    SOC_IF_ERROR_RETURN(WRITE_CMIC_SER_MEM_ADDRr(unit, i));
                    SOC_IF_ERROR_RETURN(WRITE_CMIC_SER_MEM_DATAr(unit, 0));
                }
            }
            /* Record section for mem_clear use */
            cur_spi->ser_section_start = ser_mem_addr;
            cur_spi->ser_section_end = ser_mem_total - ser_mem_granularity;
        }

        /* For multiple pipeline units, different pipeline's hardware
         * instances are duplicates.  The SER parity bits are thus
         * shared, but the configuration registers in the CMIC must be
         * different to capture the different SBUS ranges of the
         * read accesses.
         */
        multi_pipe_spi = cur_spi;
        o_info_ix = info_ix;

        while (cur_spi->mem == multi_pipe_spi->mem) {
            if (cur_spi != multi_pipe_spi) {
                info_ix++; /* Advance the index */
                if (0 != (multi_pipe_spi->ser_flags &
                          _SOC_SER_FLAG_MULTI_PIPE)) {
                    /* Get the non-default pipeline access type */
                    acc_type = multi_pipe_spi->ser_flags &
                        _SOC_SER_FLAG_ACC_TYPE_MASK;

                    /* Override ACC_TYPE in address */
                    /* Note, this function does not support the
                     * extended sbus addressing for multiple pipeline
                     * devices.
                     * soc_generic_ser_init _must_ be used
                     * for such devices.
                     */
                    start_addr &= ~(_SOC_MEM_ADDR_ACC_TYPE_MASK <<
                                    _SOC_MEM_ADDR_ACC_TYPE_SHIFT);
                    start_addr |=
                        (acc_type & _SOC_MEM_ADDR_ACC_TYPE_MASK) <<
                        _SOC_MEM_ADDR_ACC_TYPE_SHIFT;
                    end_addr &= ~(_SOC_MEM_ADDR_ACC_TYPE_MASK <<
                                    _SOC_MEM_ADDR_ACC_TYPE_SHIFT);
                    end_addr |=
                        (acc_type & _SOC_MEM_ADDR_ACC_TYPE_MASK) <<
                        _SOC_MEM_ADDR_ACC_TYPE_SHIFT;
                } else {
                    soc_cm_debug(DK_VERBOSE | DK_SOCMEM,
                           "SER mem muliple pipeline mismatch %s vs. %s.\n",
                                 SOC_MEM_NAME(unit, cur_spi->mem),
                                 SOC_MEM_NAME(unit, multi_pipe_spi->mem));
                    return SOC_E_INTERNAL;
                }
            }

            reg_addr =
                soc_reg_addr(unit, multi_pipe_spi->start_addr_reg,
                             REG_PORT_ANY, 0);
            SOC_IF_ERROR_RETURN(soc_pci_write(unit, reg_addr, start_addr));
            reg_addr =
                soc_reg_addr(unit, multi_pipe_spi->end_addr_reg,
                             REG_PORT_ANY, 0);
            SOC_IF_ERROR_RETURN(soc_pci_write(unit, reg_addr, end_addr));
            reg_addr =
                soc_reg_addr(unit, multi_pipe_spi->cmic_mem_addr_reg,
                             REG_PORT_ANY, 0);
        

#ifdef BCM_CMICM_SUPPORT
            if (soc_feature(unit, soc_feature_cmicm) &&
                !SOC_IS_KATANA(unit)) {
                if (multi_pipe_spi->cmic_ser_id) {
                    SOC_IF_ERROR_RETURN(soc_pci_write(unit, reg_addr,
                                                      ser_mem_addr1));
                    addr_valid1 |= (1 << info_ix);
                } else {
                    SOC_IF_ERROR_RETURN(soc_pci_write(unit, reg_addr,
                                                      ser_mem_addr0));
                    addr_valid0 |= (1 << info_ix);
                }    
            } else 
#endif /* BCM_CMICM_SUPPORT */
            {
                SOC_IF_ERROR_RETURN(soc_pci_write(unit, reg_addr,
                                                  ser_mem_addr));
                addr_valid0 |= (1 << info_ix);
            }

            if (multi_pipe_spi->bit_offset != -1) {
                SOC_IF_ERROR_RETURN
                    (soc_pci_write(unit, soc_reg_addr(unit, multi_pipe_spi->entry_len_reg,
                                                      REG_PORT_ANY, 0),
                                   multi_pipe_spi->bit_offset));
            }
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, multi_pipe_spi->parity_mode_reg,
                                        REG_PORT_ANY, multi_pipe_spi->parity_mode_field, 
                                        multi_pipe_spi->parity_mode));

            multi_pipe_spi = &(_ser_parity_info[info_ix + 1]);
        }

#ifdef BCM_CMICM_SUPPORT
        if (soc_feature(unit, soc_feature_cmicm) && !SOC_IS_KATANA(unit)) {
            soc_cm_debug(DK_VERBOSE | DK_SOCMEM,
                         "\t%s(%d-%d): Sbus range (0x%08x-0x%08x) SER[%d][%d](0x%03x)\n",
                         SOC_MEM_NAME(unit, cur_spi->mem),
                         index_min, index_max, start_addr, end_addr,
                         cur_spi->cmic_ser_id, o_info_ix,
                         cur_spi->cmic_ser_id ?
                         ser_mem_addr1 : ser_mem_addr0);
        } else 
#endif /* BCM_CMICM_SUPPORT */
        {
            soc_cm_debug(DK_VERBOSE | DK_SOCMEM,
                         "\t%s(%d-%d): Sbus range (0x%08x-0x%08x) SER[%d](0x%03x)\n",
                         SOC_MEM_NAME(unit, cur_spi->mem),
                         index_min, index_max, start_addr, end_addr,
                         o_info_ix, ser_mem_addr);
        }
        if (cur_spi->bit_offset != -1) {
            soc_cm_debug(DK_VERBOSE | DK_SOCMEM,
                         "\tBits protected per entry: %d\n",
                         cur_spi->bit_offset);
        }
        SOC_MEM_INFO(unit, cur_spi->mem).flags |= SOC_MEM_FLAG_SER_ENGINE;
        info_ix++;
    }

#ifdef BCM_CMICM_SUPPORT
    if (soc_feature(unit, soc_feature_cmicm) && !SOC_IS_KATANA(unit)) {
        SOC_IF_ERROR_RETURN(WRITE_CMIC_SER0_FAIL_ENTRYr(unit, 0));
        SOC_IF_ERROR_RETURN(WRITE_CMIC_SER0_FAIL_CNTr(unit, 0));
        SOC_IF_ERROR_RETURN(WRITE_CMIC_SER1_FAIL_ENTRYr(unit, 0));
        SOC_IF_ERROR_RETURN(WRITE_CMIC_SER1_FAIL_CNTr(unit, 0));
    } else 
#endif /* BCM_CMICM_SUPPORT */
    {
        SOC_IF_ERROR_RETURN(WRITE_CMIC_SER_FAIL_ENTRYr(unit, 0));
        SOC_IF_ERROR_RETURN(WRITE_CMIC_SER_FAIL_CNTr(unit, 0));
    }

    /* Enable SER protection last */
#ifdef BCM_CMICM_SUPPORT
    if (soc_feature(unit, soc_feature_cmicm) && !SOC_IS_KATANA(unit)) {
        SOC_IF_ERROR_RETURN
            (WRITE_CMIC_SER0_PROTECT_ADDR_RANGE_VALIDr(unit, addr_valid0));
        SOC_IF_ERROR_RETURN
            (WRITE_CMIC_SER1_PROTECT_ADDR_RANGE_VALIDr(unit, addr_valid1));
    } else 
#endif /* BCM_CMICM_SUPPORT */
    {
        SOC_IF_ERROR_RETURN
            (WRITE_CMIC_SER_PROTECT_ADDR_RANGE_VALIDr(unit, addr_valid0));
    }
    return SOC_E_NONE;
}

STATIC int
_soc_generic_ser_granularity(_soc_ser_protection_type_t prot_type,
                             _soc_ser_protection_mode_t prot_mode,
                             __soc_ser_start_end_bits_t *bits)
{
    int i, words = 0, ser_mem_gran;
    
    for (i = 0; i < SOC_NUM_GENERIC_PROT_SECTIONS; i++) {
        if (bits[i].start_bit >= 0) {
            words++;
        }
    }
    if (prot_type == _SOC_SER_TYPE_PARITY) {
        switch (prot_mode) {
        case _SOC_SER_PARITY_2BITS:
            ser_mem_gran = 2;
            break;
        case _SOC_SER_PARITY_4BITS:
            ser_mem_gran = 4;
            break;
        case _SOC_SER_PARITY_8BITS:
            ser_mem_gran = 8;
            break;
        case _SOC_SER_PARITY_1BIT:
        default:
            ser_mem_gran = 1;
            break;            
        }
        return ser_mem_gran;
    } else {
        switch (prot_mode) {
        case _SOC_SER_ECC_2FLD:
            ser_mem_gran = 2;
            break;
        case _SOC_SER_ECC_4FLD:
            ser_mem_gran = 4;
            break;
        case _SOC_SER_ECC_1FLD:
        default:
            ser_mem_gran = 1;
            break;            
        }
        return ser_mem_gran * words * 9;
    }
}

int
soc_generic_ser_process_error(int unit, _soc_generic_ser_info_t *ser_info,
                              int err_type)
{
    uint8 at;
    int info_ix, hw_ix, acc_type;
    int index_min, index_max, phys_index_min, phys_index_max;
    int ser_mem_count, copyno, mem_index;
    uint32 start_addr, end_addr, count;
    uint32 ser_err[2], addr;
    _soc_generic_ser_info_t *cur_ser_info;
    _soc_ser_correct_info_t spci = {0};
    soc_stat_t *stat = SOC_STAT(unit);
    ser_result_0_entry_t err_result;

    SOC_IF_ERROR_RETURN(READ_SER_ERROR_0r(unit, &ser_err[0]));
    SOC_IF_ERROR_RETURN(READ_SER_ERROR_1r(unit, &ser_err[1]));

    if (!soc_reg_field_get(unit, SER_ERROR_0r, ser_err[0], ERROR_0_VALf) && 
        !soc_reg_field_get(unit, SER_ERROR_1r, ser_err[1], ERROR_1_VALf)) {
        soc_cm_debug(DK_ERR,
                     "Unit %d: SER parity failure without valid error !!\n",
                     unit);
        return SOC_E_NONE;
    } 
    do {
        if (soc_reg_field_get(unit, SER_ERROR_0r, ser_err[0],
                              ERROR_0_VALf)) {
            SOC_IF_ERROR_RETURN
                (soc_mem_read(unit, SER_RESULT_0m, MEM_BLOCK_ANY,
                              0, &err_result));
            hw_ix = soc_mem_field32_get(unit, SER_RESULT_0m,
                                        &err_result, RANGEf);
            addr = soc_mem_field32_get(unit, SER_RESULT_0m,
                                       &err_result, SBUS_ADDRf);
            acc_type = soc_mem_field32_get(unit, SER_RESULT_0m,
                                       &err_result, ACC_TYPEf);
            SOC_IF_ERROR_RETURN(WRITE_SER_ERROR_0r(unit, 0));
        } else {
            SOC_IF_ERROR_RETURN
                (soc_mem_read(unit, SER_RESULT_1m, MEM_BLOCK_ANY,
                              0, &err_result));
            hw_ix = soc_mem_field32_get(unit, SER_RESULT_1m,
                                        &err_result, RANGEf);
            addr = soc_mem_field32_get(unit, SER_RESULT_1m,
                                       &err_result, SBUS_ADDRf);
            acc_type = soc_mem_field32_get(unit, SER_RESULT_1m,
                                       &err_result, ACC_TYPEf);
            SOC_IF_ERROR_RETURN(WRITE_SER_ERROR_1r(unit, 0));
        }

        info_ix = 0;
        while (ser_info[info_ix].mem != INVALIDm) {
            if (0 == (ser_info[info_ix].ser_flags &
                      _SOC_SER_FLAG_SW_COMPARE)) {
                if (ser_info[info_ix].ser_hw_index == hw_ix) {
                    break;
                }
            }
            info_ix++;
        }
        if (ser_info[info_ix].mem == INVALIDm) {
            soc_cm_debug(DK_ERR,
                         "Unit %d: SER parity failure with invalid mem range !!\n",
                         unit);
            return SOC_E_NONE;
        }
        cur_ser_info = &(ser_info[info_ix]);
        phys_index_min = index_min =
            soc_mem_index_min(unit, cur_ser_info->mem);
        phys_index_max = index_max =
            soc_mem_index_max(unit, cur_ser_info->mem);
        if (0 != (cur_ser_info->ser_flags & _SOC_SER_FLAG_REMAP_READ)) {
#ifdef BCM_TRIDENT2_SUPPORT
            if (SOC_IS_TD2_TT2(unit)) {
                phys_index_min =
                    soc_trident2_l3_defip_index_map(unit, 
                                                    cur_ser_info->mem,
                                                    index_min);
                phys_index_max =
                    soc_trident2_l3_defip_index_map(unit, 
                                                    cur_ser_info->mem,
                                                    index_max);
            }
#endif /* BCM_TRIDENT2_SUPPORT */
#ifdef BCM_HELIX4_SUPPORT
            if (SOC_IS_HELIX4(unit)) {
                
            }
#endif /* BCM_HELIX4_SUPPORT */
        }
        ser_mem_count = phys_index_max - phys_index_min + 1;
        if (!ser_mem_count) {
            soc_cm_debug(DK_ERR,
                         "Unit %d: SER parity failure with unavailable mem range !!\n",
                         unit);
            return SOC_E_NONE;
        }
        SOC_MEM_BLOCK_ITER(unit, cur_ser_info->mem, copyno) {
            break;
        }
        start_addr = soc_mem_addr_get(unit, cur_ser_info->mem, 0, copyno, 
                                       phys_index_min, &at);
        end_addr = soc_mem_addr_get(unit, cur_ser_info->mem, 0, copyno, 
                                     phys_index_max, &at);
        if ((addr >= start_addr) && (addr <= end_addr)) {
            /* Addr in memory range */
            mem_index = (addr - start_addr) + phys_index_min;
            if (0 != (cur_ser_info->ser_flags & _SOC_SER_FLAG_REMAP_READ)) {
                /* Now, mem_index is the physical index.
                 * The correction routines expect the logical index.
                 * Reverse the mapping.
                 */
#ifdef BCM_TRIDENT2_SUPPORT
                if (SOC_IS_TD2_TT2(unit)) {
                    mem_index =
                        soc_trident2_l3_defip_index_remap(unit, 
                                                          cur_ser_info->mem,
                                                          mem_index);
                }
#endif /* BCM_TRIDENT2_SUPPORT */
#ifdef BCM_HELIX4_SUPPORT
                if (SOC_IS_HELIX4(unit)) {
                    
                }
#endif /* BCM_HELIX4_SUPPORT */
            }
            soc_cm_debug(DK_ERR,
                         "Unit %d: %s entry %d TCAM parity error\n",
                         unit, SOC_MEM_NAME(unit, cur_ser_info->mem),
                         mem_index);
            soc_event_generate(unit, SOC_SWITCH_EVENT_PARITY_ERROR, 
                               0, addr, 0);
            stat->ser_err_tcam++;
            spci.flags = SOC_SER_SRC_MEM | SOC_SER_REG_MEM_KNOWN;
            spci.reg = INVALIDr;
            spci.mem = cur_ser_info->mem;
            spci.blk_type = copyno;
            spci.index = mem_index;
            spci.acc_type = acc_type;
            spci.pipe_num =
                (_SOC_MEM_ADDR_ACC_TYPE_PIPE_Y == acc_type) ? 1 : 0;
            SOC_IF_ERROR_RETURN(soc_ser_correction(unit, &spci));
        }
        SOC_IF_ERROR_RETURN(READ_SER_ERROR_0r(unit, &ser_err[0]));
        SOC_IF_ERROR_RETURN(READ_SER_ERROR_1r(unit, &ser_err[1]));
    } while (soc_reg_field_get(unit, SER_ERROR_0r, ser_err[0], ERROR_0_VALf) ||
             soc_reg_field_get(unit, SER_ERROR_1r, ser_err[1], ERROR_1_VALf));
    SOC_IF_ERROR_RETURN(READ_SER_MISSED_EVENTr(unit, &count));
    if (count) {
        soc_cm_debug(DK_VERBOSE | DK_SOCMEM,
                     "Unit %d: Overflow events: %d.\n", unit, count);
    }
    return SOC_E_NONE;
}

int
soc_generic_ser_init(int unit, _soc_generic_ser_info_t *ser_info)
{
    int info_ix, hw_ser_ix;
    int index_min, index_max, copyno, ser_mem_gran;
    int i = 0, ser_mem_count;
    uint32 rval, start_addr, end_addr;
    uint32 addr_valid = 0, ser_mem_addr = 0, ser_mem_total = 0;
    uint32 acc_type, at_bmp;
    _soc_generic_ser_info_t *cur_ser_info;
    ser_memory_entry_t ser_mem;
    uint8 at, alias;
    
    static soc_reg_t range_cfg[] = {
        SER_RANGE_0_CONFIGr, SER_RANGE_1_CONFIGr, SER_RANGE_2_CONFIGr,
        SER_RANGE_3_CONFIGr,
        SER_RANGE_4_CONFIGr, SER_RANGE_5_CONFIGr, SER_RANGE_6_CONFIGr,
        SER_RANGE_7_CONFIGr,
        SER_RANGE_8_CONFIGr, SER_RANGE_9_CONFIGr, SER_RANGE_10_CONFIGr,
        SER_RANGE_11_CONFIGr,
        SER_RANGE_12_CONFIGr, SER_RANGE_13_CONFIGr, SER_RANGE_14_CONFIGr,
        SER_RANGE_15_CONFIGr,
        SER_RANGE_16_CONFIGr, SER_RANGE_17_CONFIGr, SER_RANGE_18_CONFIGr,
        SER_RANGE_19_CONFIGr,
        SER_RANGE_20_CONFIGr, SER_RANGE_21_CONFIGr, SER_RANGE_22_CONFIGr,
        SER_RANGE_23_CONFIGr,
        SER_RANGE_24_CONFIGr, SER_RANGE_25_CONFIGr, SER_RANGE_26_CONFIGr,
        SER_RANGE_27_CONFIGr,
        SER_RANGE_28_CONFIGr, SER_RANGE_29_CONFIGr, SER_RANGE_30_CONFIGr,
        SER_RANGE_31_CONFIGr
    };
    static soc_reg_t addr_mask[] = {
        SER_RANGE_0_ADDR_MASKr, SER_RANGE_1_ADDR_MASKr, SER_RANGE_2_ADDR_MASKr,
        SER_RANGE_3_ADDR_MASKr,
        SER_RANGE_4_ADDR_MASKr, SER_RANGE_5_ADDR_MASKr, SER_RANGE_6_ADDR_MASKr, 
        SER_RANGE_7_ADDR_MASKr,
        SER_RANGE_8_ADDR_MASKr, SER_RANGE_9_ADDR_MASKr, SER_RANGE_10_ADDR_MASKr,
        SER_RANGE_11_ADDR_MASKr,
        SER_RANGE_12_ADDR_MASKr, SER_RANGE_13_ADDR_MASKr, SER_RANGE_14_ADDR_MASKr,
        SER_RANGE_15_ADDR_MASKr,
        SER_RANGE_16_ADDR_MASKr, SER_RANGE_17_ADDR_MASKr, SER_RANGE_18_ADDR_MASKr,
        SER_RANGE_19_ADDR_MASKr,
        SER_RANGE_20_ADDR_MASKr, SER_RANGE_21_ADDR_MASKr, SER_RANGE_22_ADDR_MASKr,
        SER_RANGE_23_ADDR_MASKr,
        SER_RANGE_24_ADDR_MASKr, SER_RANGE_25_ADDR_MASKr, SER_RANGE_26_ADDR_MASKr,
        SER_RANGE_27_ADDR_MASKr,
        SER_RANGE_28_ADDR_MASKr, SER_RANGE_29_ADDR_MASKr, SER_RANGE_30_ADDR_MASKr,
        SER_RANGE_31_ADDR_MASKr
    };
    static soc_reg_t range_start[] = {
        SER_RANGE_0_STARTr, SER_RANGE_1_STARTr, SER_RANGE_2_STARTr,
        SER_RANGE_3_STARTr,
        SER_RANGE_4_STARTr, SER_RANGE_5_STARTr, SER_RANGE_6_STARTr,
        SER_RANGE_7_STARTr,
        SER_RANGE_8_STARTr, SER_RANGE_9_STARTr, SER_RANGE_10_STARTr,
        SER_RANGE_11_STARTr,
        SER_RANGE_12_STARTr, SER_RANGE_13_STARTr, SER_RANGE_14_STARTr,
        SER_RANGE_15_STARTr,
        SER_RANGE_16_STARTr, SER_RANGE_17_STARTr, SER_RANGE_18_STARTr,
        SER_RANGE_19_STARTr,
        SER_RANGE_20_STARTr, SER_RANGE_21_STARTr, SER_RANGE_22_STARTr,
        SER_RANGE_23_STARTr,
        SER_RANGE_24_STARTr, SER_RANGE_25_STARTr, SER_RANGE_26_STARTr,
        SER_RANGE_27_STARTr,
        SER_RANGE_28_STARTr, SER_RANGE_29_STARTr, SER_RANGE_30_STARTr,
        SER_RANGE_31_STARTr
    };
    static soc_reg_t range_end[] = {
        SER_RANGE_0_ENDr, SER_RANGE_1_ENDr, SER_RANGE_2_ENDr, SER_RANGE_3_ENDr,
        SER_RANGE_4_ENDr, SER_RANGE_5_ENDr, SER_RANGE_6_ENDr, SER_RANGE_7_ENDr,
        SER_RANGE_8_ENDr, SER_RANGE_9_ENDr, SER_RANGE_10_ENDr, SER_RANGE_11_ENDr,
        SER_RANGE_12_ENDr, SER_RANGE_13_ENDr, SER_RANGE_14_ENDr, SER_RANGE_15_ENDr,
        SER_RANGE_16_ENDr, SER_RANGE_17_ENDr, SER_RANGE_18_ENDr, SER_RANGE_19_ENDr,
        SER_RANGE_20_ENDr, SER_RANGE_21_ENDr, SER_RANGE_22_ENDr, SER_RANGE_23_ENDr,
        SER_RANGE_24_ENDr, SER_RANGE_25_ENDr, SER_RANGE_26_ENDr, SER_RANGE_27_ENDr,
        SER_RANGE_28_ENDr, SER_RANGE_29_ENDr, SER_RANGE_30_ENDr, SER_RANGE_31_ENDr
    };
    static soc_reg_t range_result[] = {
        SER_RANGE_0_RESULTr, SER_RANGE_1_RESULTr, SER_RANGE_2_RESULTr,
        SER_RANGE_3_RESULTr,
        SER_RANGE_4_RESULTr, SER_RANGE_5_RESULTr, SER_RANGE_6_RESULTr,
        SER_RANGE_7_RESULTr,
        SER_RANGE_8_RESULTr, SER_RANGE_9_RESULTr, SER_RANGE_10_RESULTr,
        SER_RANGE_11_RESULTr,
        SER_RANGE_12_RESULTr, SER_RANGE_13_RESULTr, SER_RANGE_14_RESULTr,
        SER_RANGE_15_RESULTr,
        SER_RANGE_16_RESULTr, SER_RANGE_17_RESULTr, SER_RANGE_18_RESULTr,
        SER_RANGE_19_RESULTr,
        SER_RANGE_20_RESULTr, SER_RANGE_21_RESULTr, SER_RANGE_22_RESULTr,
        SER_RANGE_23_RESULTr,
        SER_RANGE_24_RESULTr, SER_RANGE_25_RESULTr, SER_RANGE_26_RESULTr,
        SER_RANGE_27_RESULTr,
        SER_RANGE_28_RESULTr, SER_RANGE_29_RESULTr, SER_RANGE_30_RESULTr,
        SER_RANGE_31_RESULTr
    };
    static soc_reg_t prot_word[][SOC_NUM_GENERIC_PROT_SECTIONS] = {
        { SER_RANGE_0_PROT_WORD_0r, SER_RANGE_0_PROT_WORD_1r,
          SER_RANGE_0_PROT_WORD_2r, SER_RANGE_0_PROT_WORD_3r },
        { SER_RANGE_1_PROT_WORD_0r, SER_RANGE_1_PROT_WORD_1r,
          SER_RANGE_1_PROT_WORD_2r, SER_RANGE_1_PROT_WORD_3r },
        { SER_RANGE_2_PROT_WORD_0r, SER_RANGE_2_PROT_WORD_1r,
          SER_RANGE_2_PROT_WORD_2r, SER_RANGE_2_PROT_WORD_3r },
        { SER_RANGE_3_PROT_WORD_0r, SER_RANGE_3_PROT_WORD_1r,
          SER_RANGE_3_PROT_WORD_2r, SER_RANGE_3_PROT_WORD_3r },
        { SER_RANGE_4_PROT_WORD_0r, SER_RANGE_4_PROT_WORD_1r,
          SER_RANGE_4_PROT_WORD_2r, SER_RANGE_4_PROT_WORD_3r },
        { SER_RANGE_5_PROT_WORD_0r, SER_RANGE_5_PROT_WORD_1r,
          SER_RANGE_5_PROT_WORD_2r, SER_RANGE_5_PROT_WORD_3r },
        { SER_RANGE_6_PROT_WORD_0r, SER_RANGE_6_PROT_WORD_1r,
          SER_RANGE_6_PROT_WORD_2r, SER_RANGE_6_PROT_WORD_3r },
        { SER_RANGE_7_PROT_WORD_0r, SER_RANGE_7_PROT_WORD_1r,
          SER_RANGE_7_PROT_WORD_2r, SER_RANGE_7_PROT_WORD_3r },
        { SER_RANGE_8_PROT_WORD_0r, SER_RANGE_8_PROT_WORD_1r,
          SER_RANGE_8_PROT_WORD_2r, SER_RANGE_8_PROT_WORD_3r },
        { SER_RANGE_9_PROT_WORD_0r, SER_RANGE_9_PROT_WORD_1r,
          SER_RANGE_9_PROT_WORD_2r, SER_RANGE_9_PROT_WORD_3r },
        { SER_RANGE_10_PROT_WORD_0r, SER_RANGE_10_PROT_WORD_1r,
          SER_RANGE_10_PROT_WORD_2r, SER_RANGE_10_PROT_WORD_3r },
        { SER_RANGE_11_PROT_WORD_0r, SER_RANGE_11_PROT_WORD_1r,
          SER_RANGE_11_PROT_WORD_2r, SER_RANGE_11_PROT_WORD_3r },
        { SER_RANGE_12_PROT_WORD_0r, SER_RANGE_12_PROT_WORD_1r,
          SER_RANGE_12_PROT_WORD_2r, SER_RANGE_12_PROT_WORD_3r },
        { SER_RANGE_13_PROT_WORD_0r, SER_RANGE_13_PROT_WORD_1r,
          SER_RANGE_13_PROT_WORD_2r, SER_RANGE_13_PROT_WORD_3r },
        { SER_RANGE_14_PROT_WORD_0r, SER_RANGE_14_PROT_WORD_1r,
          SER_RANGE_14_PROT_WORD_2r, SER_RANGE_14_PROT_WORD_3r },
        { SER_RANGE_15_PROT_WORD_0r, SER_RANGE_15_PROT_WORD_1r,
          SER_RANGE_15_PROT_WORD_2r, SER_RANGE_15_PROT_WORD_3r },
        { SER_RANGE_16_PROT_WORD_0r, SER_RANGE_16_PROT_WORD_1r,
          SER_RANGE_16_PROT_WORD_2r, SER_RANGE_16_PROT_WORD_3r },
        { SER_RANGE_17_PROT_WORD_0r, SER_RANGE_17_PROT_WORD_1r,
          SER_RANGE_17_PROT_WORD_2r, SER_RANGE_17_PROT_WORD_3r },
        { SER_RANGE_18_PROT_WORD_0r, SER_RANGE_18_PROT_WORD_1r,
          SER_RANGE_18_PROT_WORD_2r, SER_RANGE_18_PROT_WORD_3r },
        { SER_RANGE_19_PROT_WORD_0r, SER_RANGE_19_PROT_WORD_1r,
          SER_RANGE_19_PROT_WORD_2r, SER_RANGE_19_PROT_WORD_3r },
        { SER_RANGE_20_PROT_WORD_0r, SER_RANGE_20_PROT_WORD_1r,
          SER_RANGE_20_PROT_WORD_2r, SER_RANGE_20_PROT_WORD_3r },
        { SER_RANGE_21_PROT_WORD_0r, SER_RANGE_21_PROT_WORD_1r,
          SER_RANGE_21_PROT_WORD_2r, SER_RANGE_21_PROT_WORD_3r },
        { SER_RANGE_22_PROT_WORD_0r, SER_RANGE_22_PROT_WORD_1r,
          SER_RANGE_22_PROT_WORD_2r, SER_RANGE_22_PROT_WORD_3r },
        { SER_RANGE_23_PROT_WORD_0r, SER_RANGE_23_PROT_WORD_1r,
          SER_RANGE_23_PROT_WORD_2r, SER_RANGE_23_PROT_WORD_3r },
        { SER_RANGE_24_PROT_WORD_0r, SER_RANGE_24_PROT_WORD_1r,
          SER_RANGE_24_PROT_WORD_2r, SER_RANGE_24_PROT_WORD_3r },
        { SER_RANGE_25_PROT_WORD_0r, SER_RANGE_25_PROT_WORD_1r,
          SER_RANGE_25_PROT_WORD_2r, SER_RANGE_25_PROT_WORD_3r },
        { SER_RANGE_26_PROT_WORD_0r, SER_RANGE_26_PROT_WORD_1r,
          SER_RANGE_26_PROT_WORD_2r, SER_RANGE_26_PROT_WORD_3r },
        { SER_RANGE_27_PROT_WORD_0r, SER_RANGE_27_PROT_WORD_1r,
          SER_RANGE_27_PROT_WORD_2r, SER_RANGE_27_PROT_WORD_3r },
        { SER_RANGE_28_PROT_WORD_0r, SER_RANGE_28_PROT_WORD_1r,
          SER_RANGE_28_PROT_WORD_2r, SER_RANGE_28_PROT_WORD_3r },
        { SER_RANGE_29_PROT_WORD_0r, SER_RANGE_29_PROT_WORD_1r,
          SER_RANGE_29_PROT_WORD_2r, SER_RANGE_29_PROT_WORD_3r },
        { SER_RANGE_30_PROT_WORD_0r, SER_RANGE_30_PROT_WORD_1r,
          SER_RANGE_30_PROT_WORD_2r, SER_RANGE_30_PROT_WORD_3r },
        { SER_RANGE_31_PROT_WORD_0r, SER_RANGE_31_PROT_WORD_1r,
          SER_RANGE_31_PROT_WORD_2r, SER_RANGE_31_PROT_WORD_3r }
    };        

    soc_cm_debug(DK_VERBOSE,
                 "Unit %d: SER engine init:\n", unit);

    info_ix = 0;
    hw_ser_ix = 0;
    sal_memset(&ser_mem, 0, sizeof(ser_mem));
    while (ser_info[info_ix].mem != INVALIDm) {
        alias = FALSE;
        cur_ser_info = &(ser_info[info_ix]);

        if (cur_ser_info->ser_flags & _SOC_SER_FLAG_SW_COMPARE) {
            if (SOC_MEM_INFO(unit, cur_ser_info->mem).flags &
                SOC_MEM_FLAG_BE) {
                soc_cm_debug(DK_ERR,
                         "\tUnit %d: SW SER init of Big Endian TCAMs is not supported: %s\n",
                             unit, SOC_MEM_NAME(unit, cur_ser_info->mem));
                return SOC_E_INTERNAL;
            }

            soc_cm_debug(DK_VERBOSE | DK_SOCMEM,
                         "\tSkipping HW SER init of manual scan mem: %s\n",
                         SOC_MEM_NAME(unit, cur_ser_info->mem));
            info_ix++;
            continue;
        }

        index_min = soc_mem_index_min(unit, cur_ser_info->mem);
        index_max = soc_mem_index_max(unit, cur_ser_info->mem);
#ifdef BCM_TRIUMPH3_SUPPORT
        if (soc_feature(unit, soc_feature_l3_defip_map) &&
            ((cur_ser_info->mem == L3_DEFIP_PAIR_128m) ||
             (cur_ser_info->mem == L3_DEFIPm))) {
            /* Override SOC memory impression for overlay
             * LPM tables. */
            index_min = 0;
            index_max = (SOC_CONTROL(unit)->l3_defip_max_tcams * 
                         SOC_CONTROL(unit)->l3_defip_tcam_size) - 1;
            if (cur_ser_info->mem == L3_DEFIP_PAIR_128m) {
                index_max /= 2;
            }
        }
#endif /* BCM_TRIUMPH3_SUPPORT */

        ser_mem_count = index_max - index_min + 1;
        if (!ser_mem_count) {
            soc_cm_debug(DK_VERBOSE | DK_SOCMEM,
                         "\tSkipping empty mem: %s\n",
                         SOC_MEM_NAME(unit, cur_ser_info->mem));
            info_ix++;
            continue;
        }

        ser_mem_gran = _soc_generic_ser_granularity(cur_ser_info->prot_type,
                                                    cur_ser_info->prot_mode,
                                                    cur_ser_info->start_end_bits);
        ser_mem_count *= ser_mem_gran;
        if (cur_ser_info->prot_type == _SOC_SER_TYPE_PARITY) {
            ser_mem_count += ser_mem_count/8; /* Pad bit */
        }

        /* Round up to entry size for simplicity */
        ser_mem_count = ((ser_mem_count + 35) / 36);
        
        if (cur_ser_info->alias_mem != INVALIDm) {
            /* See if the aliased memory has already been configured */
            for (i = 0; i < info_ix; i++) {
                if (ser_info[i].mem == cur_ser_info->alias_mem) {
                    soc_cm_debug(DK_VERBOSE | DK_SOCMEM,
                                 "\tSER alias mem.\n");
                    alias = TRUE;
                    break;
                }
            }            
        }
        if (!alias &&
            (cur_ser_info->ser_flags & _SOC_SER_FLAG_MULTI_PIPE)) {
            i = info_ix - 1;
            if ((i >= 0) && (ser_info[i].mem == cur_ser_info->mem)) {
                /* For multiple pipeline units, different pipeline's hardware
                 * instances are duplicates.  The SER parity bits are thus
                 * shared, but the configuration registers in the CMIC
                 * must be different to capture the different SBUS ranges
                 * of the read accesses.
                 */
                soc_cm_debug(DK_VERBOSE | DK_SOCMEM,
                             "\tSER multiple pipeline mem.\n");
                /* Get the non-default pipeline access type */
                acc_type = cur_ser_info->ser_flags &
                    _SOC_SER_FLAG_ACC_TYPE_MASK;

                /* Copy info for mem clear */
                cur_ser_info->ser_section_start =
                    ser_info[i].ser_section_start;
                cur_ser_info->ser_section_end =
                    ser_info[i].ser_section_end;
                cur_ser_info->ser_hw_index =
                    ser_info[i].ser_hw_index;

                /* For multi-pipe, we can use the access type bitmap
                 * point multiple SBUS accesses to the same HW
                 * range checker.
                 * The HW index advanced when the first entry was set,
                 * so we need to use the previous valid HW index. */
                if (0 == hw_ser_ix) {
                    return SOC_E_INTERNAL;
                }
                SOC_IF_ERROR_RETURN
                    (soc_reg32_set(unit, range_cfg[hw_ser_ix - 1],
                                   REG_PORT_ANY, 0, rval));
                at_bmp =
                    soc_reg_field_get(unit, SER_RANGE_0_CONFIGr, rval,
                                      ACC_TYPEf);
                at_bmp |= (1 << acc_type);
                soc_reg_field_set(unit, SER_RANGE_0_CONFIGr, &rval,
                                  ACC_TYPEf, at_bmp);
                SOC_IF_ERROR_RETURN
                    (soc_reg32_set(unit, range_cfg[hw_ser_ix - 1],
                                   REG_PORT_ANY, 0, rval));
                info_ix++;
                /* Don't advance hw_ser_ix again in case of designs with
                 * more than two pipes */
                continue;
            }
        }

        if (!alias && ((ser_mem_count + ser_mem_total) > 
                       soc_mem_index_count(unit, SER_MEMORYm))) {
            /* Can't fit requested parity bits in SER memory */
            soc_cm_debug(DK_VERBOSE | DK_SOCMEM,
                         "Unit %d: SER mem full: Skipping further config.\n",
                         unit);
            break;
        }

        /* Parity bits fit, so on with the config */
        SOC_MEM_BLOCK_ITER(unit, cur_ser_info->mem, copyno) {
            break;
        }
        start_addr = soc_mem_addr_get(unit, cur_ser_info->mem, 0, copyno, 
                                      index_min, &at);
        end_addr = soc_mem_addr_get(unit, cur_ser_info->mem, 0, copyno, 
                                    index_max, &at);

        /* Flush SER memory if not aliased */
        ser_mem_addr = alias ? ser_info[i].ser_section_start : 
                               ser_mem_total; /* Previous total */
        if (alias) {
            soc_cm_debug(DK_VERBOSE | DK_SOCMEM,
                         "\tReuse SER_MEM (%d) from index: %d - %d\n",
                         ser_mem_count, ser_info[i].ser_section_start,
                         ser_info[i].ser_section_end);
        } else {
            ser_mem_total += ser_mem_count; /* New total */
            soc_cm_debug(DK_VERBOSE | DK_SOCMEM,
                         "\tClear SER_MEM (%d) from index: %d - %d\n", 
                         ser_mem_count, ser_mem_addr, ser_mem_total-1);
            for (i = ser_mem_addr; i < ser_mem_total; i++) {
                /* mem clear */            
                SOC_IF_ERROR_RETURN
                    (WRITE_SER_MEMORYm(unit, MEM_BLOCK_ALL, i, &ser_mem));
            }
        } 

        /* Record section for mem_clear use */
        cur_ser_info->ser_section_start = ser_mem_addr;
        cur_ser_info->ser_section_end = alias ? ser_info[i].ser_section_end : 
                                                (ser_mem_total-1);
        cur_ser_info->ser_hw_index = hw_ser_ix;

        SOC_IF_ERROR_RETURN
            (soc_reg32_set(unit, range_start[hw_ser_ix], REG_PORT_ANY, 0, 
                           start_addr));
        SOC_IF_ERROR_RETURN
            (soc_reg32_set(unit, range_end[hw_ser_ix], REG_PORT_ANY, 0, 
                           end_addr));
        SOC_IF_ERROR_RETURN
            (soc_reg32_set(unit, addr_mask[hw_ser_ix], REG_PORT_ANY, 0, 
                           cur_ser_info->addr_mask ? 
                           cur_ser_info->addr_mask : 0xffffffff));
        SOC_IF_ERROR_RETURN
            (soc_reg32_set(unit, range_result[hw_ser_ix], REG_PORT_ANY, 0, 
                           ser_mem_addr));
        addr_valid |= (1 << hw_ser_ix);

        for (i = 0; i < SOC_NUM_GENERIC_PROT_SECTIONS; i++) {
            if (cur_ser_info->start_end_bits[i].start_bit != -1) {
                rval = 0;
                soc_reg_field_set(unit, SER_RANGE_0_PROT_WORD_0r, &rval, 
                                  DATA_STARTf, 
                                  cur_ser_info->start_end_bits[i].start_bit);
                soc_reg_field_set(unit, SER_RANGE_0_PROT_WORD_0r, &rval, 
                                  DATA_ENDf, 
                                  cur_ser_info->start_end_bits[i].end_bit);
                SOC_IF_ERROR_RETURN
                    (soc_reg32_set(unit, prot_word[hw_ser_ix][i],
                                   REG_PORT_ANY, 0, rval));
            }
        }
        rval = 0;
        soc_reg_field_set(unit, SER_RANGE_0_CONFIGr, &rval, BLOCKf, 
                          SOC_BLOCK2SCH(unit, copyno));
        soc_reg_field_set(unit, SER_RANGE_0_CONFIGr, &rval, ACC_TYPEf, 
                          1 << at);
        soc_reg_field_set(unit, SER_RANGE_0_CONFIGr, &rval, PROT_TYPEf, 
                          cur_ser_info->prot_type);
        soc_reg_field_set(unit, SER_RANGE_0_CONFIGr, &rval, PROT_MODEf, 
                          cur_ser_info->prot_mode);
        soc_reg_field_set(unit, SER_RANGE_0_CONFIGr, &rval, INTERLEAVE_MODEf, 
                          cur_ser_info->intrlv_mode);
        SOC_IF_ERROR_RETURN
            (soc_reg32_set(unit, range_cfg[hw_ser_ix], REG_PORT_ANY, 0, rval));
        
        soc_cm_debug(DK_VERBOSE | DK_SOCMEM, 
                     "\tRange[%d](0x%04x): %s(%d-%d): Sbus range (0x%08x-0x%08x)\n",
                     hw_ser_ix, ser_mem_addr,
                     SOC_MEM_NAME(unit, cur_ser_info->mem),
                     index_min, index_max, start_addr, end_addr);
        info_ix++;
        hw_ser_ix++;
    }

    SOC_IF_ERROR_RETURN
        (WRITE_SER_RING_ERR_CTRLr(unit, 0xf));
    /* Enable SER protection last */
    SOC_IF_ERROR_RETURN
        (WRITE_SER_RANGE_ENABLEr(unit, addr_valid));
    if (soc_feature(unit, soc_feature_ser_hw_bg_read)) {
        SOC_IF_ERROR_RETURN
            (WRITE_SER_CONFIG_REGr(unit, 0x3));
    } else {
        SOC_IF_ERROR_RETURN
            (WRITE_SER_CONFIG_REGr(unit, 0x1));
    }
    /* Kick off s/w background scan if applicable */
    if (!soc_feature(unit, soc_feature_ser_hw_bg_read)) {
        
    }
    return SOC_E_NONE;
}

int
soc_generic_ser_mem_scan_start(int unit)
{
#ifdef INCLUDE_MEM_SCAN
    /* Kick off s/w background scan */
    if (soc_property_get(unit, spn_MEM_SCAN_ENABLE, SAL_BOOT_SIMULATION ? 0 : 1)) {
        sal_usecs_t interval = 0;
        int rate = 0;
        
        if (soc_mem_scan_running(unit, &rate, &interval)) {
            if (soc_mem_scan_stop(unit)) {
                return SOC_E_INTERNAL;
            }
        }
        rate = rate ? rate : soc_property_get(unit, spn_MEM_SCAN_RATE, 4096);
        interval = interval ? interval : 
                              soc_property_get(unit, spn_MEM_SCAN_INTERVAL,
                              SAL_BOOT_SIMULATION ? 100000000 : 10000000);
        if (soc_mem_scan_start(unit, rate, interval)) {
            return SOC_E_INTERNAL;
        }
    }
#endif /* INCLUDE_MEM_SCAN */
    return SOC_E_NONE;
}

int
soc_generic_ser_mem_scan_stop(int unit)
{
#ifdef INCLUDE_MEM_SCAN
    sal_usecs_t interval = 0;
    int rate = 0;
        
    if (soc_mem_scan_running(unit, &rate, &interval)) {
        if (soc_mem_scan_stop(unit)) {
            return SOC_E_INTERNAL;
        }
    }
#endif /* INCLUDE_MEM_SCAN */
    return SOC_E_NONE;
}

STATIC int
_soc_ser_sync_mac_limits(int unit, soc_mem_t mem)
{
#ifdef BCM_XGS_SWITCH_SUPPORT
    
    SOC_IF_ERROR_RETURN(soc_l2x_freeze(unit));
    if (_SOC_DRV_MEM_CHK_L2_MEM(mem)) {
        /* Read L2 table from hardware and update the */
        /* Decrement port/trunk and vlan/vfi counts */
#if defined(BCM_TRIUMPH3_SUPPORT)
        if (SOC_IS_TRIUMPH3(unit)) {
            soc_tr3_l2_entry_limit_count_update(unit);
        }
#endif
    } else {
        /* Calculate and restore port/trunk and vlan/vfi counts */
        if (mem == VLAN_OR_VFI_MAC_COUNTm) {
#if defined(BCM_TRIUMPH3_SUPPORT)
            if (SOC_IS_TRIUMPH3(unit)) {
                soc_tr3_l2_entry_limit_count_update(unit);
            }
#endif
        }
    }
    SOC_IF_ERROR_RETURN(soc_l2x_thaw(unit));
#endif /* BCM_XGS_SWITCH_SUPPORT */    
    return SOC_E_NONE;
}

STATIC int
_soc_ser_recovery_hw_cache(int unit, int pipe, soc_mem_t mem, int copyno,
                           int index)
{
    int acc_type, cache_acc_type;
    uint32 entry[SOC_MAX_MEM_WORDS];
    soc_stat_t *stat = SOC_STAT(unit);

    if (SOC_IS_TD2_TT2(unit)) {
        if (pipe == SOC_PIPE_ANY) {
            pipe = 0;
        }
        switch (mem) {
        case L3_DEFIP_ALPM_IPV4m:
        case L3_DEFIP_ALPM_IPV4_1m:
        case L3_DEFIP_ALPM_IPV6_64m:
        case L3_DEFIP_ALPM_IPV6_64_1m:
        case L3_DEFIP_ALPM_IPV6_128m:
        case L3_DEFIP_AUX_TABLEm:
        case L3_DEFIP_PAIR_128m:
        case L3_DEFIPm:
        case L3_DEFIP_128_DATA_ONLYm:
        case L3_DEFIP_DATA_ONLYm:
            if (pipe == 0) {
                acc_type = _SOC_MEM_ADDR_ACC_TYPE_PIPE_X;
                cache_acc_type = _SOC_MEM_ADDR_ACC_TYPE_PIPE_Y;
            } else if (pipe == 1) {
                acc_type = _SOC_MEM_ADDR_ACC_TYPE_PIPE_Y;
                cache_acc_type = _SOC_MEM_ADDR_ACC_TYPE_PIPE_X;
            } else {
                return SOC_E_PARAM;
            }
            SOC_IF_ERROR_RETURN
                (soc_mem_pipe_select_read(unit, mem, copyno,
                                          cache_acc_type, index, entry));
            SOC_IF_ERROR_RETURN
                (soc_mem_pipe_select_write(unit, mem, copyno,
                                           acc_type, index, entry));
            soc_cm_debug(DK_ERR,
                         "Unit %d: RESTORE[from %c pipe]: %s[%d] blk: %s index: %d\n",
                         unit, pipe ? 'X':'Y', SOC_MEM_NAME(unit, mem), mem,
                         SOC_BLOCK_NAME(unit, copyno), index);
            stat->ser_err_restor++;
            return SOC_E_NONE;
        default: 
            return SOC_E_UNAVAIL;
        }
    }
    return SOC_E_UNAVAIL;
}

/* hw_cpu: Need special handling for hw vs cpu error on some devices/mems.
           hw=0, cpu=1. 
*/
STATIC int
_soc_ser_overlay_mem_correction(int unit, int hw_cpu, int pipe, int sblk, int addr,
                                soc_mem_t reported_mem, int copyno, int index)
{
#define _SOC_SER_MEM_OVERLAY_SET 6
    soc_mem_t mem = INVALIDm;
    void *null_entry;
    int rv, entry_dw;
    uint8 fix = 0, *vmap;
    int i, m, num_mems, vindex = 0;
    soc_stat_t *stat = SOC_STAT(unit);
    uint32 entry[SOC_MAX_MEM_WORDS], *cache;
    int num_indexes[_SOC_SER_MEM_OVERLAY_SET] = {1, 1, 1, 1, 1, 1};
    int iratio[_SOC_SER_MEM_OVERLAY_SET] = {1, 1, 1, 1, 1, 1};
    soc_mem_t mem_list[_SOC_SER_MEM_OVERLAY_SET] = 
                       {INVALIDm, INVALIDm, INVALIDm, INVALIDm};
    
    if (copyno == MEM_BLOCK_ANY) {
        copyno = SOC_MEM_BLOCK_ANY(unit, reported_mem);
    }
    mem_list[0] = reported_mem;
    switch (reported_mem) {
    case MODPORT_MAP_SWm:
        if (SOC_IS_TRX(unit) && 
            !(SOC_IS_TRIUMPH3(unit) || SOC_IS_TD_TT(unit) || SOC_IS_TD2_TT2(unit))) {
            num_mems = 1;
            mem_list[0] = MODPORT_MAPm;
            break;
        } else {
            return SOC_E_UNAVAIL;
        }
    case MODPORT_MAP_M0m:
    case MODPORT_MAP_M1m:
    case MODPORT_MAP_M2m:
    case MODPORT_MAP_M3m:
        num_mems = 1;
        mem_list[0] = MODPORT_MAP_MIRRORm;
        break;
    case ESM_PKT_TYPE_ID_DATA_ONLYm:
        num_mems = 1;
        mem_list[0] = ESM_PKT_TYPE_IDm;
        break;
    case ING_SNAT_DATA_ONLYm:
        num_mems = 1;
        mem_list[0] = ING_SNATm;
        break;
    case VLAN_SUBNET_DATA_ONLYm:
        num_mems = 1;
        mem_list[0] = VLAN_SUBNETm;
        break;
    case MY_STATION_TCAM_DATA_ONLYm:
        num_mems = 1;
        mem_list[0] = MY_STATION_TCAMm;
        break;
    case L2_USER_ENTRY_DATA_ONLYm:
        num_mems = 1;
        mem_list[0] = L2_USER_ENTRYm;
        break;
    case L2_ENTRY_1m:
        num_mems = 2;
        mem_list[1] = L2_ENTRY_2m; iratio[1] = 2;
        break;
    case MPLS_ENTRYm:
    case MPLS_ENTRY_1m:
        if (soc_feature(unit, soc_feature_ism_memory)) {
            num_mems = 2;
            mem_list[1] = MPLS_ENTRY_EXTDm; iratio[1] = 2;
            break;
        } else {
            return SOC_E_UNAVAIL;
        }
    case VLAN_XLATE_1m:
        num_mems = 2;
        mem_list[1] = VLAN_XLATE_EXTDm; iratio[1] = 2;
        break;
    case L3_ENTRY_1m:
        num_mems = 3;
        mem_list[1] = L3_ENTRY_2m; iratio[1] = 2;
        mem_list[2] = L3_ENTRY_4m; iratio[2] = 4;
        break;
    case EGR_IP_TUNNELm:
        if (SOC_IS_TD_TT(unit)) {
            /* Note: This is an exception case where the wider memories index is 
             *       used for reporting the error on a hw lookup and two indexes 
             *       need to be updated for narrow memories.
             *
             *       On a cpu access error the actual view accessed is used for 
             *       reporting the error. So for a reported index 'n' we will 
             *       need to check/fix index n/2 and n*2 as well. 
             */
            num_mems = (index || hw_cpu) ? 6 : 3;
            index *= 2; /* To keep the index ratio based logic below, consistent */
            num_indexes[0] = 2; num_indexes[1] = 2;
            num_indexes[3] = 2; num_indexes[4] = 2;
            mem_list[0] = EGR_IP_TUNNELm;
            mem_list[1] = EGR_IP_TUNNEL_MPLSm;
            mem_list[2] = EGR_IP_TUNNEL_IPV6m; iratio[2] = 2;
            mem_list[3] = EGR_IP_TUNNELm; iratio[3] = 2;
            mem_list[4] = EGR_IP_TUNNEL_MPLSm; iratio[4] = 2;
            mem_list[5] = EGR_IP_TUNNEL_IPV6m; iratio[5] = 4;
        } else {
            num_mems = 3;
            mem_list[1] = EGR_IP_TUNNEL_MPLSm;
            mem_list[2] = EGR_IP_TUNNEL_IPV6m; iratio[2] = 2;
        }
        break;
    case L3_ENTRY_ONLYm:
        if (SOC_MEM_IS_VALID(unit, L3_ENTRY_IPV4_UNICASTm)) {
            mem_list[0] = L3_ENTRY_IPV4_UNICASTm;
        }
        /* passthru */
        /* coverity[fallthrough: FALSE] */
    case L3_ENTRY_IPV4_UNICASTm:
        num_mems = 4;
        mem_list[1] = L3_ENTRY_IPV4_MULTICASTm; iratio[1] = 2;
        mem_list[2] = L3_ENTRY_IPV6_UNICASTm; iratio[2] = 2;
        mem_list[3] = L3_ENTRY_IPV6_MULTICASTm; iratio[3] = 4;
        break;
    case L3_DEFIP_128_DATA_ONLYm:
        num_mems = 1;
        mem_list[0] = L3_DEFIP_128m;
        break;
    case L3_DEFIP_DATA_ONLYm:
        if (soc_feature(unit, soc_feature_l3_defip_map)) {
            num_mems = 1;
#ifdef BCM_TRIUMPH3_SUPPORT
            if (SOC_IS_TRIUMPH3(unit)) {
                index = soc_tr3_l3_defip_mem_index_get(unit, index,
                                                       &mem_list[0]);
            } else
#endif /* BCM_TRIUMPH3_SUPPORT */
#if defined(BCM_TRIDENT2_SUPPORT)
            if (SOC_IS_TD2_TT2(unit)) {
                index = soc_trident2_l3_defip_mem_index_get(unit, index,
                                                            &mem_list[0]);
                /* Lets only correct the data portion of the TCAM to avoid 
                   getting into deadlock situation in bulk op retry for TD2.
                   Note that mem cache is not used for this memory anyways. */
                if (mem_list[0] == L3_DEFIPm) {
                    mem_list[0] = L3_DEFIP_DATA_ONLYm;
                } else if (mem_list[0] == L3_DEFIP_PAIR_128m) {
                    mem_list[0] = L3_DEFIP_PAIR_128_DATA_ONLYm;
                }
                if (SOC_CONTROL(unit)->alpm_bulk_retry) {
                    sal_sem_give(SOC_CONTROL(unit)->alpm_bulk_retry);
                }
            } else 
#endif /* BCM_TRIDENT2_SUPPORT */
            {
                
                return SOC_E_UNAVAIL;
            }
            break;
        } else {
            return SOC_E_UNAVAIL;
        }
    default:
        return SOC_E_UNAVAIL;
    }
    for (m = 0; m < num_mems; m++) {
        mem = mem_list[m];
        entry_dw = soc_mem_entry_words(unit, mem);

        if ((mem == L2_ENTRY_1m) || (mem == L2_ENTRY_2m)) {
            /* Take lock to ensure that cache is updated in case of a bulk op */
            MEM_LOCK(unit, mem);       
        }

        cache = SOC_MEM_STATE(unit, mem).cache[copyno];
        vmap = SOC_MEM_STATE(unit, mem).vmap[copyno];        
        vindex = index/iratio[m];
        for (i = 0; i < num_indexes[m]; i++) {
            if (cache != NULL && CACHE_VMAP_TST(vmap, vindex+i)) {
                sal_memcpy(entry, cache + ((vindex+i) * entry_dw), entry_dw * 4);
                if ((rv = soc_mem_write(unit, mem, copyno, vindex+i, entry)) < 0) {
                    soc_cm_debug(DK_ERR, "Unit %d: %s: mem write %s.%s[%d] failed: %s\n",
                                 unit, FUNCTION_NAME(), SOC_MEM_NAME(unit, mem),
                                 SOC_BLOCK_NAME(unit, copyno),
                                 vindex+i, soc_errmsg(rv));
                    if ((mem == L2_ENTRY_1m) || (mem == L2_ENTRY_2m)) {
                        MEM_UNLOCK(unit, mem);       
                    }

                    return rv;
                }
                fix++;
                soc_event_generate(unit, SOC_SWITCH_EVENT_PARITY_ERROR,
                                   SOC_SWITCH_EVENT_DATA_ERROR_CORRECTED, mem, 
                                   vindex);
                soc_cm_debug(DK_ERR,
                             "Unit %d: CACHE_RESTORE: %s[%d] blk: %s index: %d : [%d][%x]\n",
                             unit, SOC_MEM_NAME(unit, mem), mem,
                             SOC_BLOCK_NAME(unit, copyno),
                             vindex+i, sblk, addr);
                stat->ser_err_restor++;
            }
            if (cache == NULL) {
                /* Do hw cache (from the other pipe) based correction if s/w mem
                   cache is not available */                   
                rv = _soc_ser_recovery_hw_cache(unit, pipe, mem, copyno, vindex+i);
                if (!(rv == SOC_E_NONE || rv == SOC_E_UNAVAIL)) {
                    return rv;
                } else {
                    fix++;
                }
            }
        }
        if ((mem == L2_ENTRY_1m) || (mem == L2_ENTRY_2m)) {
            MEM_UNLOCK(unit, mem);       
        }
    }
    if (fix == 0) {
        /* Fall back to clear - this is always the widest view */
        null_entry = soc_mem_entry_null(unit, mem);
        if ((rv = soc_mem_write(unit, mem, copyno, vindex, null_entry)) < 0) {
            soc_cm_debug(DK_ERR, "Unit %d: %s: mem write %s.%s[%d] failed: %s\n",
                         unit, FUNCTION_NAME(), SOC_MEM_NAME(unit, mem),
                         SOC_BLOCK_NAME(unit, copyno),
                         vindex, soc_errmsg(rv));
            return rv;
        }
        soc_event_generate(unit, SOC_SWITCH_EVENT_PARITY_ERROR,
                           SOC_SWITCH_EVENT_DATA_ERROR_CORRECTED, mem,
                           vindex);
        soc_cm_debug(DK_ERR,
                     "Unit %d: CLEAR_RESTORE: %s[%d] blk: %s index: %d : [%d][%x]\n",
                     unit, SOC_MEM_NAME(unit, mem), mem,
                     SOC_BLOCK_NAME(unit, copyno),
                     vindex, sblk, addr);
        if (_SOC_DRV_MEM_CHK_L2_MEM(mem) &&
            soc_feature(unit, soc_feature_mac_learn_limit)) {
            if ((rv =_soc_ser_sync_mac_limits(unit, mem)) != SOC_E_NONE) {
                soc_cm_debug(DK_ERR,
                             "Unit %d: L2 mac limit sync failed !!\n", 
                             unit);
                return rv;
            }
        }
        stat->ser_err_clear++;
    }
    return SOC_E_NONE;
}

#define _SOC_SER_SRAM_CHK_RETURN(rv, str) \
            if (rv != SOC_E_NONE) {\
                soc_cm_debug(DK_ERR, \
                             "Unit %d: SER SRAM correction encoutered error(%d) in %s\n", \
                             unit, rv, str);\
                MEM_UNLOCK(unit, mem);\
                return rv;\
            }
STATIC int
_soc_ser_sram_correction(int unit, int pipe, int sblk, int addr, soc_mem_t mem,
                         int copyno, int index, uint8 mode)
{
    void *null_entry;
    uint8 *vmap, at = 0;
    int rv, i, entry_dw, block;
    soc_stat_t *stat = SOC_STAT(unit);
    _soc_ser_sram_info_t sram_info;
    uint32 entry[SOC_MAX_MEM_WORDS], rval, *cache, raddr = 0;
    
    if (!SOC_MEM_IS_VALID(unit, mem) || index < 0 ||
        !(mode == _SOC_SER_SRAM_CORRECTION_MODE_0 ||
          mode == _SOC_SER_SRAM_CORRECTION_MODE_1)) {
        return SOC_E_PARAM;
    }
    /* Determine RAM count and indexes for all RAMs */
#if defined(BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TD2_TT2(unit)) {
        SOC_IF_ERROR_RETURN
            (_soc_trident2_mem_sram_info_get(unit, mem, index, &sram_info));
    } else 
#endif /* BCM_TRIDENT2_SUPPORT */
    {
        return SOC_E_UNAVAIL;
    }

    /* Update all RAM indexes with data from cache or null entry */
    MEM_LOCK(unit, mem);
    cache = SOC_MEM_STATE(unit, mem).cache[copyno];
    vmap = SOC_MEM_STATE(unit, mem).vmap[copyno];
    entry_dw = soc_mem_entry_words(unit, mem);
    if (mode == _SOC_SER_SRAM_CORRECTION_MODE_0) {
        rv = soc_reg32_get(unit, sram_info.disable_reg, REG_PORT_ANY, 0, &rval);
        _SOC_SER_SRAM_CHK_RETURN(rv, "reg get");
        soc_reg_field_set(unit, sram_info.disable_reg, &rval,
                          sram_info.disable_field, 1);
        raddr = soc_reg_addr_get(unit, sram_info.disable_reg, REG_PORT_ANY, 0,
                                 &block, &at);
        rv = _soc_reg32_set(unit, block, 3, raddr, rval);
        _SOC_SER_SRAM_CHK_RETURN(rv, "reg set");
    }
    for (i = 0; i < sram_info.ram_count; i++) {
        index = sram_info.mem_indexes[i];
        if (mem == L3_ENTRY_IPV4_UNICASTm) {
            rv = _soc_ser_overlay_mem_correction(unit, 0, pipe, sblk, addr, mem, 
                                                 copyno, index);
            if (!(rv == SOC_E_NONE || rv == SOC_E_UNAVAIL)) {
                soc_cm_debug(DK_ERR,
                             "Unit %d: SER SRAM correction encoutered error(%d) "
                             "in overlay mem correction\n",
                             unit, rv);
                MEM_UNLOCK(unit, mem);
                return rv;
            }
        } else if (mem == L3_DEFIP_ALPM_IPV4m || mem == L3_DEFIP_ALPM_IPV4_1m ||
                   mem == L3_DEFIP_ALPM_IPV6_64m || mem == L3_DEFIP_ALPM_IPV6_64_1m ||
                   mem == L3_DEFIP_ALPM_IPV6_128m) {            
            rv = _soc_ser_recovery_hw_cache(unit, pipe, mem, copyno, index);
            if (!(rv == SOC_E_NONE || rv == SOC_E_UNAVAIL)) {
                soc_cm_debug(DK_ERR,
                             "Unit %d: SER SRAM correction encoutered error(%d) "
                             "in recovery from hw cache\n",
                             unit, rv);
                MEM_UNLOCK(unit, mem);
                return rv;
            }
        } else {
            if (cache != NULL && CACHE_VMAP_TST(vmap, index)) {
                sal_memcpy(entry, cache + index * entry_dw, entry_dw * 4);
                rv = soc_mem_write(unit, mem, copyno, index, entry);
                _SOC_SER_SRAM_CHK_RETURN(rv, "mem write");
                soc_cm_debug(DK_ERR, "Unit %d: CACHE_RESTORE: %s[%d] index: %d\n",
                             unit, SOC_MEM_NAME(unit, mem), mem, index);
                stat->ser_err_restor++;
            } else { /* Fall back to clear */
                null_entry = soc_mem_entry_null(unit, mem);
                rv = soc_mem_write(unit, mem, copyno, index, null_entry);
                _SOC_SER_SRAM_CHK_RETURN(rv, "mem write");
                soc_cm_debug(DK_ERR, "Unit %d: CLEAR_RESTORE: %s[%d] index: %d\n",
                             unit, SOC_MEM_NAME(unit, mem), mem, index);
                stat->ser_err_clear++;
            }
        }
    }
    if (mode == _SOC_SER_SRAM_CORRECTION_MODE_0) {
        soc_reg_field_set(unit, sram_info.disable_reg, &rval,
                          sram_info.disable_field, 0);
        rv = _soc_reg32_set(unit, block, 3, addr, rval);
        _SOC_SER_SRAM_CHK_RETURN(rv, "reg set");
    }
    MEM_UNLOCK(unit, mem);
    switch (mem) {
    case L3_DEFIP_ALPM_IPV4m:
    case L3_DEFIP_ALPM_IPV4_1m:
    case L3_DEFIP_ALPM_IPV6_64m:
    case L3_DEFIP_ALPM_IPV6_64_1m:
    case L3_DEFIP_ALPM_IPV6_128m:
        if (SOC_CONTROL(unit)->alpm_lookup_retry &&
            SHR_BITGET(&(SOC_CONTROL(unit)->alpm_mem_ops), _SOC_ALPM_LOOKUP)) {
            sal_sem_give(SOC_CONTROL(unit)->alpm_lookup_retry);
        }
        if (SOC_CONTROL(unit)->alpm_insert_retry &&
            SHR_BITGET(&(SOC_CONTROL(unit)->alpm_mem_ops), _SOC_ALPM_INSERT)) {
            sal_sem_give(SOC_CONTROL(unit)->alpm_insert_retry);
        }
        if (SOC_CONTROL(unit)->alpm_delete_retry &&
            SHR_BITGET(&(SOC_CONTROL(unit)->alpm_mem_ops), _SOC_ALPM_DELETE)) {
            sal_sem_give(SOC_CONTROL(unit)->alpm_delete_retry);
        }
        break;
    default: break;
    }

    soc_event_generate(unit, SOC_SWITCH_EVENT_PARITY_ERROR,
                       SOC_SWITCH_EVENT_DATA_ERROR_CORRECTED, mem, index);
    stat->ser_err_corr++;
    return SOC_E_NONE;
}

int
soc_ser_sram_correction(int unit, int pipe, int sblk, int addr, soc_mem_t mem,
                        int copyno, int index)
{
    int rv;
    
    
    switch (mem) {
    case L3_ENTRY_ONLYm:
    case L3_ENTRY_IPV4_UNICASTm:
    case L3_ENTRY_IPV4_MULTICASTm:
    case L3_ENTRY_IPV6_UNICASTm:
    case L3_ENTRY_IPV6_MULTICASTm:
        rv = _soc_ser_sram_correction(unit, pipe, sblk, addr,
                                      L3_ENTRY_IPV4_UNICASTm, copyno, index,
                                      _SOC_SER_SRAM_CORRECTION_MODE_0);
        if (rv == SOC_E_NONE || rv != SOC_E_UNAVAIL) {
            return rv;
        }
        break;       
    case L2Xm:
    case L3_DEFIP_ALPM_IPV4m:
    case L3_DEFIP_ALPM_IPV4_1m:
    case L3_DEFIP_ALPM_IPV6_64m:
    case L3_DEFIP_ALPM_IPV6_64_1m:
    case L3_DEFIP_ALPM_IPV6_128m:
    case L3_DEFIP_ALPM_RAWm:
    case VLAN_XLATEm:
    case EGR_VLAN_XLATEm:
        return _soc_ser_sram_correction(unit, pipe, sblk, addr, mem, copyno,
                                        index, _SOC_SER_SRAM_CORRECTION_MODE_0);
    case ING_L3_NEXT_HOPm:
    case L3_IPMCm:
    case L2MCm:
        return _soc_ser_sram_correction(unit, pipe, sblk, addr, mem, copyno,
                                        index, _SOC_SER_SRAM_CORRECTION_MODE_1);
    default: break;
    }
    return SOC_E_UNAVAIL;
}

int
_soc_ser_lp_mem_correction(int unit, soc_mem_t mem, int index)
{
#ifdef BCM_XGS_SWITCH_SUPPORT
    int rv, rv1;
    soc_mem_t hash_mem;
    uint32 entry[SOC_MAX_MEM_WORDS];

    switch (mem) {
    case L2_ENTRY_LPm: hash_mem = L2Xm; break;
    case L3_ENTRY_LPm: hash_mem = L3_ENTRY_IPV4_UNICASTm; break;
    case VLAN_XLATE_LPm: hash_mem = VLAN_XLATEm; break;
    case EGR_VLAN_XLATE_LPm: hash_mem = EGR_VLAN_XLATEm; break;
    default: return SOC_E_PARAM;
    }

    if (hash_mem == L2Xm) {
        rv1 = soc_l2x_freeze(unit);
        if (SOC_FAILURE(rv1)) {
            soc_cm_debug(DK_WARN, 
                         "Unit %d: L2 freeze failed in LP mem correction\n", unit);
        }
    } else {
        MEM_LOCK(unit, hash_mem);
    }
    rv = soc_mem_read(unit, hash_mem, MEM_BLOCK_ALL, index*4, entry);
    if (SOC_FAILURE(rv)) {
        if (hash_mem == L2Xm) {
            rv1 = soc_l2x_thaw(unit);
            if (SOC_FAILURE(rv1)) {
                soc_cm_debug(DK_WARN, 
                             "Unit %d: L2 thaw failed in LP mem correction\n", unit);
            }
        } else {
            MEM_UNLOCK(unit, hash_mem);
        }
        return rv;
    }
    rv = soc_mem_write(unit, hash_mem, MEM_BLOCK_ALL, index*4, entry);
    if (SOC_FAILURE(rv)) {
        if (hash_mem == L2Xm) {
            rv1 = soc_l2x_thaw(unit);
            if (SOC_FAILURE(rv1)) {
                soc_cm_debug(DK_WARN, 
                             "Unit %d: L2 thaw failed in LP mem correction\n", unit);
            }
        } else {
            MEM_UNLOCK(unit, hash_mem);
        }
        return rv;
    }
    if (hash_mem == L2Xm) {
        rv1 = soc_l2x_thaw(unit);
        if (SOC_FAILURE(rv1)) {
            soc_cm_debug(DK_WARN, 
                         "Unit %d: L2 thaw failed in LP mem correction\n", unit);
        }
    } else {
        MEM_UNLOCK(unit, hash_mem);
    }
#endif /* BCM_XGS_SWITCH_SUPPORT */
    return SOC_E_NONE;
}

STATIC void
_soc_ser_ism_correction(int unit)
{
#define _SOC_SER_ISM_DMA_CHUNK_SIZE 1024
    uint32 *tbl_chnk;
    int rv, m, buf_size, chunksize;
    int chnk_idx, chnk_idx_max, mem_idx_max;
    static soc_mem_t ism_mems[] = {
        VLAN_XLATEm,
        L2_ENTRY_1m,
        L3_ENTRY_1m,
        MPLS_ENTRYm,
        EGR_VLAN_XLATEm
    };
    
    chunksize = _SOC_SER_ISM_DMA_CHUNK_SIZE;
    buf_size = 4 * SOC_MAX_MEM_WORDS * chunksize;
    tbl_chnk = soc_cm_salloc(unit, buf_size, "ism ser correction");
    if (NULL == tbl_chnk) {
        soc_cm_debug(DK_ERR, "Unit %d: Memory allocation failure in ser ism "
                     "correction !!\n", unit);
        return;
    }
    
    for (m=0; m<COUNTOF(ism_mems); m++) {
        if (!soc_mem_index_count(unit, ism_mems[m])) {
            continue;
        }
        mem_idx_max = soc_mem_index_max(unit, ism_mems[m]);
        MEM_LOCK(unit, ism_mems[m]);
        for (chnk_idx = soc_mem_index_min(unit, ism_mems[m]); 
             chnk_idx <= mem_idx_max; chnk_idx += chunksize) {
            sal_memset((void *)tbl_chnk, 0, buf_size);
            chnk_idx_max = ((chnk_idx + chunksize) <= mem_idx_max) ? 
                (chnk_idx + chunksize - 1) : mem_idx_max;
            rv = soc_mem_read_range(unit, ism_mems[m], MEM_BLOCK_ANY, chnk_idx,
                                    chnk_idx_max, tbl_chnk);
            if (SOC_FAILURE(rv)) {
                soc_cm_debug(DK_ERR, "Unit %d: DMA failure in ser ism "
                             "correction for %s mem !!\n", unit, 
                             SOC_MEM_NAME(unit, ism_mems[m]));
                MEM_UNLOCK(unit, ism_mems[m]);
                soc_cm_sfree(unit, tbl_chnk);
                return;
            }
        }
        MEM_UNLOCK(unit, ism_mems[m]);
    }
    soc_cm_sfree(unit, tbl_chnk);
}

STATIC int check_parity(uint32 *p32, int n32)
{
    uint32 parity = 0;
    uint32 tmpI   = 0;

    for (n32--; n32>=0; n32--) {

        tmpI=p32[n32];
        tmpI=tmpI^(tmpI>>1);
        tmpI=tmpI^(tmpI>>2);
        tmpI=tmpI^(tmpI>>4);
        tmpI=tmpI^(tmpI>>8);
        tmpI=tmpI^(tmpI>>16);

        parity = parity ^ (tmpI&1);
    }

    return parity&1;
}
#ifdef _SER_TIME_STAMP
extern sal_usecs_t ser_time_1;
sal_usecs_t ser_time_5;
#endif


int
_soc_oam_ser_correction(int unit, soc_mem_t mem, int index)
{
#ifdef BCM_TRIUMPH3_SUPPORT
    if (SOC_IS_TRIUMPH3(unit)) {
        return soc_tr3_oam_ser_process(unit, mem, index);
    }
#endif
    soc_cm_debug(DK_ERR, "Unit %d: SER Handling routine not avaialable\n",
                 unit);
    return SOC_E_UNAVAIL;
}


STATIC int
_soc_ser_ipfix_correction(int unit, soc_mem_t mem)
{
    int port;
    int ingress_stage = 0;
    int rv;

    /*
     * Also need to clear the 
     * 1. ingress/egress fifo counter
     * 2. ingress/egress port record count
     * 3. ingress/egress port sampling count
     */
    ingress_stage = (mem == ING_IPFIX_SESSION_TABLEm) ? 1 : 0; 

    if (ingress_stage) {
        rv = WRITE_ING_IPFIX_EXPORT_FIFO_COUNTERr(unit, 0);
        if (rv < 0) {
            soc_cm_debug(DK_ERR, "Unit %d: %s: "
                         "ING_IPFIX_EXPORT_FIFO_COUNTERr clear failed: %s\n",
                         unit, FUNCTION_NAME(), soc_errmsg(rv));
            return rv;
        }
    } else {
        rv = WRITE_EGR_IPFIX_EXPORT_FIFO_COUNTERr(unit, 0);
        if (rv < 0) {
            soc_cm_debug(DK_ERR, "Unit %d: %s: "
                         "EGR_IPFIX_EXPORT_FIFO_COUNTERr clear failed: %s\n",
                         unit, FUNCTION_NAME(), soc_errmsg(rv));
            return rv;
        }
    }

    PBMP_PORT_ITER(unit, port) {
        if (ingress_stage) {

            rv = WRITE_ING_IPFIX_PORT_RECORD_COUNTr(unit, port, 0);
            if (rv < 0) {
                soc_cm_debug(DK_ERR, "Unit %d: %s: "
                             "ING_IPFIX_PORT_RECORD_COUNTr clear for "
                             "port(%d) failed: %s\n",
                             unit, FUNCTION_NAME(), port, soc_errmsg(rv));
                return rv;
            }

            rv = WRITE_ING_IPFIX_PORT_SAMPLING_COUNTERr(unit, port, 0);
            if (rv < 0) {
                soc_cm_debug(DK_ERR, "Unit %d: %s: "
                             "ING_IPFIX_PORT_SAMPLING_COUNTERr clear for "
                             "port(%d) failed: %s\n",
                             unit, FUNCTION_NAME(), port, soc_errmsg(rv));
                return rv;
            }
        } else {

            rv = WRITE_EGR_IPFIX_PORT_RECORD_COUNTr(unit, port, 0);
            if (rv < 0) {
                soc_cm_debug(DK_ERR, "Unit %d: %s: "
                             "EGR_IPFIX_PORT_RECORD_COUNTr clear for "
                             "port(%d) failed: %s\n",
                             unit, FUNCTION_NAME(), port, soc_errmsg(rv));
                return rv;
            }

            rv = WRITE_EGR_IPFIX_PORT_SAMPLING_COUNTERr(unit, port, 0);
            if (rv < 0) {
                soc_cm_debug(DK_ERR, "Unit %d: %s: "
                             "EGR_IPFIX_PORT_SAMPLING_COUNTERr clear for "
                             "port(%d) failed: %s\n",
                             unit, FUNCTION_NAME(), port, soc_errmsg(rv));
                return rv;
            }
        }
    }
    return SOC_E_NONE;
}


int
soc_ser_correction(int unit, _soc_ser_correct_info_t *si) 
{
    soc_mem_t mem = si->mem;
    int rv, index = si->index, entry_dw, copyno = 0;
    soc_reg_t reg = si->reg;
    soc_port_t port = si->port;
    void *null_entry;
    uint32 entry[SOC_MAX_MEM_WORDS], *cache;
    uint8 *vmap;
    soc_stat_t *stat = SOC_STAT(unit);
    
    if (!SOC_SER_CORRECTION_SUPPORT(unit)) {
        return SOC_E_NONE;
    }
    soc_cm_debug(DK_VERBOSE, "Unit %d: SER_CORRECTION: reg/mem:%d btype:%d sblk:%d at:%d "
                 "stage:%d addr:0x%08x port: %d index: %d\n", unit,
                 (si->flags & SOC_SER_SRC_MEM) ? si->mem : si->reg, si->blk_type, 
                 si->sblk, si->acc_type, si->stage, si->addr, si->port, si->index);
    
    if (si->flags & SOC_SER_SRC_MEM) { /* Handle mems */
        if (!(si->flags & SOC_SER_REG_MEM_KNOWN)) {
            /* Decode memory from address details */
            mem = soc_addr_to_mem_extended(unit, si->sblk, si->acc_type, si->addr);
            if (mem == INVALIDm) {
                if (soc_feature(unit, soc_feature_two_ingress_pipes) &&
                    soc_feature(unit, soc_feature_sbusdma)) {
                    soc_cm_debug(DK_VERBOSE, "Unit %d: %s: memory not decoded [%d %d 0x%8x].\n", 
                                 unit, FUNCTION_NAME(), si->sblk, si->acc_type, si->addr);
                    return SOC_E_NOT_FOUND;
                } else {
                    soc_cm_debug(DK_ERR, "Unit %d: %s: memory not decoded [%d %d 0x%8x].\n", 
                                 unit, FUNCTION_NAME(), si->sblk, si->acc_type, si->addr);
                    stat->ser_err_sw++;
                    return SOC_E_INTERNAL;
                }
            }            
        }        
        if (mem == INVALIDm) {
            return SOC_E_PARAM;
        }
        if (si->sblk) {
            SOC_MEM_BLOCK_ITER(unit, mem, copyno) {
                if (SOC_BLOCK2OFFSET(unit, copyno) == si->sblk) {
                    break;
                }
            }
        } else {
            copyno = SOC_MEM_BLOCK_ANY(unit, mem);
        }
        stat->ser_err_mem++;
        soc_cm_debug(DK_ERR, "Unit %d: mem: %d=%s blkoffset:%d\n", unit, mem, 
                     SOC_MEM_NAME(unit, mem), copyno);
        entry_dw = soc_mem_entry_words(unit, mem);
        if (soc_feature(unit, soc_feature_shared_hash_mem)) {
            rv = soc_ser_sram_correction(unit, si->pipe_num, si->sblk, si->addr,
                                         mem, copyno, index);
            if (rv == SOC_E_NONE || rv != SOC_E_UNAVAIL) {
                return rv;
            }
        }
        if (soc_feature(unit, soc_feature_fp_meter_ser_verify) &&
            (FP_METER_TABLEm == mem)) {
            int i=0;
            uint8 scer = SOC_MEM_INFO(unit, mem).flags & SOC_MEM_FLAG_SCHAN_ERROR_RETURN;

            /* Get data even on parity error */
            SOC_MEM_INFO(unit, mem).flags |= SOC_MEM_FLAG_SCHAN_ERROR_RETURN;
            if (SOC_E_NONE == soc_mem_read(unit, mem, copyno, index, entry)) {
                if (!check_parity(entry, entry_dw)) {
                    /* No error case. Print and return */
                    soc_cm_debug(DK_WARN,
                                 "NO PARITY ERROR: reg/mem:%d btype:%d sblk:%d at:%d "
                                 "stage:%d addr:0x%08x index: %d\n",
                                 (si->flags & SOC_SER_SRC_MEM) ? si->mem : si->reg,
                                 si->blk_type, si->sblk, si->acc_type,
                                 si->stage, si->addr, si->index);

                    for (i=0;i<entry_dw;i++) {
                        soc_cm_debug(DK_WARN, "NO PARITY ERROR: Entry[%d]:%08X\n", i, entry[i]);
                    }
                    /* Restore old state */
                    if (scer == 0) {
                        SOC_MEM_INFO(unit, mem).flags &= ~SOC_MEM_FLAG_SCHAN_ERROR_RETURN;
                    }
                    return SOC_E_NONE;
                }
            }
            /* Restore old state */
            if (scer == 0) {
                SOC_MEM_INFO(unit, mem).flags &= ~SOC_MEM_FLAG_SCHAN_ERROR_RETURN;
            }
        } else if (soc_feature(unit, soc_feature_ism_memory) &&
                   (mem == RAW_ENTRY_TABLEm)) {
            /* Packet lookup encountered an error - dump mems from cpu and do 
               normal correction */
            _soc_ser_ism_correction(unit);
            return SOC_E_NONE;
        }
        switch (SOC_MEM_SER_CORRECTION_TYPE(unit, mem)) {
        case SOC_MEM_FLAG_SER_ENTRY_CLEAR:
            null_entry = soc_mem_entry_null(unit, mem);
            if ((rv = soc_mem_write(unit, mem, copyno, index, null_entry)) < 0) {
                soc_cm_debug(DK_ERR, "Unit %d: %s: mem write %s.%s[%d] failed: %s\n",
                             unit, FUNCTION_NAME(), SOC_MEM_NAME(unit, mem),
                             SOC_BLOCK_NAME(unit, copyno), index, soc_errmsg(rv));
                return rv;
            }
            soc_event_generate(unit, SOC_SWITCH_EVENT_PARITY_ERROR,
                               SOC_SWITCH_EVENT_DATA_ERROR_CORRECTED, mem, index);
            soc_cm_debug(DK_ERR, "Unit %d: ENTRY_CLEAR: %s[%d] blk: %s index: %d : [%d][%x]\n",
                         unit, SOC_MEM_NAME(unit, mem), mem,
                         SOC_BLOCK_NAME(unit, copyno), index, si->sblk, si->addr);

            /* Handle IPFIX ser */
            if (mem == ING_IPFIX_SESSION_TABLEm || 
                mem == EGR_IPFIX_SESSION_TABLEm) {
                if ((rv = _soc_ser_ipfix_correction(unit, mem)) < 0) {
                    return rv;
                }
            }

            stat->ser_err_clear++;
            return SOC_E_NONE;
        case SOC_MEM_FLAG_SER_ECC_CORRECTABLE:
            stat->ser_err_ecc++;
            /* passthru */
            /* coverity[fallthrough: FALSE] */
        case SOC_MEM_FLAG_SER_CACHE_RESTORE:
            /* passthru */
            /* coverity[fallthrough: FALSE] */
        case SOC_MEM_FLAG_SER_WRITE_CACHE_RESTORE:
            rv = _soc_ser_overlay_mem_correction(unit, si->flags & SOC_SER_ERR_CPU ? 1 : 0, 
                                                 si->pipe_num, si->sblk, si->addr, mem,
                                                 copyno, index);
            if (rv == SOC_E_NONE || rv != SOC_E_UNAVAIL) {
                return rv;
            }
            if (mem == L2Xm) {
                /* Take lock to ensure that cache is updated in case of a bulk op */
                MEM_LOCK(unit, mem);
            }
            cache = SOC_MEM_STATE(unit, mem).cache[copyno];
            vmap = SOC_MEM_STATE(unit, mem).vmap[copyno];
            if (cache == NULL) {
                rv = _soc_ser_recovery_hw_cache(unit, si->pipe_num, mem, copyno, index);
                if (rv == SOC_E_NONE || rv != SOC_E_UNAVAIL) {
                    if (rv == SOC_E_NONE && mem == L3_DEFIP_AUX_TABLEm &&
                        SOC_IS_TD2_TT2(unit)) {
                        if (SOC_CONTROL(unit)->alpm_bulk_retry) {
                            sal_sem_give(SOC_CONTROL(unit)->alpm_bulk_retry);
                        }
                    }
                    return rv;
                }
            }
            if (cache != NULL && CACHE_VMAP_TST(vmap, index)) {
                sal_memcpy(entry, cache + index * entry_dw, entry_dw * 4);
                if ((rv = soc_mem_write(unit, mem, copyno, index, entry)) < 0) {
                    soc_cm_debug(DK_ERR, "Unit %d: %s: mem write %s.%s[%d] failed: %s\n",
                                 unit, FUNCTION_NAME(), SOC_MEM_NAME(unit, mem),
                                 SOC_BLOCK_NAME(unit, copyno),
                                 index, soc_errmsg(rv));

                    if (mem == L2Xm) {
                        MEM_UNLOCK(unit, mem);
                    }

                    return rv;
                }
                if (mem == L2Xm) {
                    MEM_UNLOCK(unit, mem);       
                }
                soc_event_generate(unit, SOC_SWITCH_EVENT_PARITY_ERROR,
                                   SOC_SWITCH_EVENT_DATA_ERROR_CORRECTED, mem, 
                                   index);
                soc_cm_debug(DK_ERR,
                             "Unit %d: CACHE_RESTORE: %s[%d] blk: %s index: %d : [%d][%x]\n",
                             unit, SOC_MEM_NAME(unit, mem), mem,
                             SOC_BLOCK_NAME(unit, copyno),
                             index, si->sblk, si->addr);
                stat->ser_err_restor++;
#ifdef _SER_TIME_STAMP
                ser_time_5 = SAL_USECS_SUB(sal_time_usecs(), ser_time_1);                
                soc_cm_print("Total: %d\n", ser_time_5);
#endif
                return SOC_E_NONE;
            } else { /* Fall back to clear */
                null_entry = soc_mem_entry_null(unit, mem);
                if ((rv = soc_mem_write(unit, mem, copyno, index, null_entry)) < 0) {
                    soc_cm_debug(DK_ERR, "Unit %d: %s: mem write %s.%s[%d] failed: %s\n",
                                 unit, FUNCTION_NAME(), SOC_MEM_NAME(unit, mem),
                                 SOC_BLOCK_NAME(unit, copyno),
                                 index, soc_errmsg(rv));

                    if (mem == L2Xm) {
                        MEM_UNLOCK(unit, mem);       
                    }

                    return rv;
                }

                if (mem == L2Xm) {
                    MEM_UNLOCK(unit, mem);       
                }
                soc_event_generate(unit, SOC_SWITCH_EVENT_PARITY_ERROR,
                                   SOC_SWITCH_EVENT_DATA_ERROR_CORRECTED, mem,
                                   index);
                soc_cm_debug(DK_ERR,
                             "Unit %d: CLEAR_RESTORE: %s[%d] blk: %s index: %d : [%d][%x]\n",
                             unit, SOC_MEM_NAME(unit, mem), mem,
                             SOC_BLOCK_NAME(unit, copyno),
                             index, si->sblk, si->addr);
                if (_SOC_DRV_MEM_CHK_L2_MEM(mem) &&
                    soc_feature(unit, soc_feature_mac_learn_limit)) {
                    if ((rv =_soc_ser_sync_mac_limits(unit, mem)) != SOC_E_NONE) {
                        soc_cm_debug(DK_ERR,
                                     "Unit %d: L2 mac limit sync failed !!\n", 
                                     unit);
                        return rv;
                    }
                }
                stat->ser_err_clear++;
#ifdef _SER_TIME_STAMP
                ser_time_5 = SAL_USECS_SUB(sal_time_usecs(), ser_time_1);
                soc_cm_print("Total: %d\n", ser_time_5);
#endif
                return SOC_E_NONE;
            }
        case SOC_MEM_FLAG_SER_SPECIAL:
            stat->ser_err_spe++;
            
            switch (mem) {
            case L2_ENTRY_LPm:
            case L3_ENTRY_LPm:
            case VLAN_XLATE_LPm:
            case EGR_VLAN_XLATE_LPm:
                soc_cm_debug(DK_ERR,
                             "Unit %d: SER_SPECIAL: %s[%d] blk: %s index: %d : [%d][%x]\n",
                             unit, SOC_MEM_NAME(unit, mem), mem,
                             SOC_BLOCK_NAME(unit, copyno),
                             index, si->sblk, si->addr);
                return _soc_ser_lp_mem_correction(unit, mem, index);
            case PORT_OR_TRUNK_MAC_COUNTm:
            case VLAN_OR_VFI_MAC_COUNTm:
                return _soc_ser_sync_mac_limits(unit, mem);
            case MA_STATEm:
            case RMEPm:
                rv = _soc_oam_ser_correction(unit, mem, index);
                if (rv != SOC_E_NONE) {
                    soc_cm_debug(DK_ERR, 
                                 "Unit %d: Error in SER Handling rv=%d\n", 
                                 unit, rv);
                }
                return rv;
            default: 
                soc_cm_debug(DK_ERR, "Unit %d: Unknown ser correction !!\n", 
                             unit);
                stat->ser_err_sw++;
                return SOC_E_INTERNAL;
            }
        default: stat->ser_err_sw++; return SOC_E_INTERNAL;
        }
    } else { /* Handle regs */
        uint64 rval64;

        stat->ser_err_reg++;
        if (!(si->flags & SOC_SER_REG_MEM_KNOWN)) {
             soc_regaddrinfo_t ainfo;
             
             /* Decode register from address details */
             soc_regaddrinfo_extended_get(unit, &ainfo, si->sblk,
                                          si->acc_type, si->addr);
             reg = ainfo.reg;
             port = ainfo.port;
             if (reg == INVALIDr || port == -1) {
                if (soc_feature(unit, soc_feature_two_ingress_pipes) &&
                    soc_feature(unit, soc_feature_sbusdma)) {
                    soc_cm_debug(DK_VERBOSE, "Unit %d: %s: register not decoded [%d %d 0x%8x].\n", 
                                 unit, FUNCTION_NAME(), si->sblk, si->acc_type, si->addr);
                    return SOC_E_NOT_FOUND;
                } else {
                    soc_cm_debug(DK_ERR, "Unit %d: %s: register not decoded [%d %d 0x%8x].\n", 
                                 unit, FUNCTION_NAME(), si->sblk, si->acc_type, si->addr);
                    stat->ser_err_sw++;
                    return SOC_E_INTERNAL;
                }
             }
             index = ainfo.idx >= 0 ? ainfo.idx : 0;
             soc_cm_debug(DK_ERR, "Unit %d: reg: %d=%s port: %d index: %d\n",
                          unit, reg, SOC_REG_NAME(unit, reg), ainfo.port, index);
        }
        if (reg == INVALIDr || port == -1) {
            return SOC_E_PARAM;
        }
        if (si->blk_type >= 0) {
            SOC_BLOCK_ITER(unit, copyno, si->blk_type) {
                if (SOC_BLOCK_TYPE(unit, copyno) == si->blk_type) {
                    break;
                }
            }
        }
        if (SOC_REG_IS_COUNTER(unit, reg)) {
            if (SOC_SER_COUNTER_CORRECTION(unit) == 0) {
                COMPILER_64_ZERO(rval64);
                /* Clear counter in h/w and s/w */            
                SOC_IF_ERROR_RETURN(soc_counter_set(unit, port, reg, index, rval64));
                soc_cm_debug(DK_ERR,
                             "Unit %d: COUNTER_CLEAR: %s[%d] blk: %s index: %d : port[%d]\n",
                             unit, SOC_REG_NAME(unit, reg), reg,
                             SOC_BLOCK_NAME(unit, copyno),
                             index, port);
                stat->ser_err_clear++;
            } else {
                /* Restore last s/w counter value in h/w */
                SOC_IF_ERROR_RETURN(soc_counter_get(unit, port, reg, index, &rval64));
                SOC_IF_ERROR_RETURN(soc_counter_set(unit, port, reg, index, rval64));
                soc_cm_debug(DK_ERR,
                             "Unit %d: COUNTER_RESTORE: %s[%d] blk: %s index: %d : port[%d]\n",
                             unit, SOC_REG_NAME(unit, reg), reg,
                             SOC_BLOCK_NAME(unit, copyno),
                             index, port);
                stat->ser_err_restor++;
            }
        } else {
            rv = soc_ser_reg_cache_get(unit, reg, port, index, &rval64);
            if (rv == SOC_E_UNAVAIL) {
                SOC_IF_ERROR_RETURN(soc_ser_reg_clear(unit, reg, port, index, rval64));
                soc_event_generate(unit, SOC_SWITCH_EVENT_PARITY_ERROR,
                                   SOC_SWITCH_EVENT_DATA_ERROR_CORRECTED, 
                                   reg | SOC_SER_ERROR_DATA_REG_ID_OFFSET, 
                                   port << SOC_SER_ERROR_PIPE_BP | index);
                soc_cm_debug(DK_ERR,
                             "Unit %d: REG_CLEAR: %s[%d] blk: %s index: %d : port[%d]\n",
                             unit, SOC_REG_NAME(unit, reg), reg,
                             SOC_BLOCK_NAME(unit, copyno),
                             index, port);
                stat->ser_err_clear++;
            } else {
                SOC_IF_ERROR_RETURN(soc_reg_set_nocache(unit, reg, port, index, rval64));
                soc_event_generate(unit, SOC_SWITCH_EVENT_PARITY_ERROR,
                                   SOC_SWITCH_EVENT_DATA_ERROR_CORRECTED, 
                                   reg | SOC_SER_ERROR_DATA_REG_ID_OFFSET, 
                                   port << SOC_SER_ERROR_PIPE_BP | index);
                soc_cm_debug(DK_ERR,
                             "Unit %d: REG_RESTORE: %s[%d] blk: %s index: %d : port[%d]\n",
                             unit, SOC_REG_NAME(unit, reg), reg,
                             SOC_BLOCK_NAME(unit, copyno),
                             index, port);
                stat->ser_err_restor++;
            }
        }
    }
    return SOC_E_NONE;
}

#endif /* BCM_XGS_SUPPORT || BCM_SIRIUS_SUPPORT */
