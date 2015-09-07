/*
 * $Id: ism.c,v 1.75 Broadcom SDK $
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
 * ISM management routines
 */

#include <shared/bsl.h>

#include <assert.h>

#include <sal/types.h>

#include <soc/drv.h>
#include <soc/error.h>
#include <soc/util.h>
#include <soc/mem.h>
#include <soc/debug.h>
#include <soc/cm.h>

#ifdef BCM_ISM_SUPPORT

_soc_ism_t _soc_ism_info[SOC_MAX_NUM_DEVICES];
_soc_ism_hash_t _soc_ism_hash_info[SOC_MAX_NUM_DEVICES];
/* track bank usage by mem, set dynamically - used by pcid as well */
uint32 _soc_ism_bank_avail[SOC_MAX_NUM_DEVICES][_SOC_ISM_MAX_BANKS];

/* Physical stage/bank constructs */ 
uint32 _soc_ism_bank_raw_sizes[] = {
    _SOC_ISM_BANK0_SIZE, _SOC_ISM_BANK1_SIZE,
    _SOC_ISM_BANK2_SIZE, _SOC_ISM_BANK3_SIZE, 
    _SOC_ISM_BANK4_SIZE,
    _SOC_ISM_BANK0_SIZE, _SOC_ISM_BANK1_SIZE,
    _SOC_ISM_BANK2_SIZE, _SOC_ISM_BANK3_SIZE, 
    _SOC_ISM_BANK4_SIZE,
    _SOC_ISM_BANK0_SIZE, _SOC_ISM_BANK1_SIZE,
    _SOC_ISM_BANK2_SIZE, _SOC_ISM_BANK3_SIZE, 
    _SOC_ISM_BANK4_SIZE,
    _SOC_ISM_BANK0_SIZE, _SOC_ISM_BANK1_SIZE,
    _SOC_ISM_BANK2_SIZE, _SOC_ISM_BANK3_SIZE, 
    _SOC_ISM_BANK4_SIZE
};

uint32 _soc_ism_bank_raw_sizes_256[] = {
    _SOC_ISM_256_BANK0_SIZE, _SOC_ISM_256_BANK1_SIZE,
    _SOC_ISM_256_BANK2_SIZE, _SOC_ISM_256_BANK3_SIZE, 
    _SOC_ISM_256_BANK4_SIZE,
    _SOC_ISM_256_BANK0_SIZE, _SOC_ISM_256_BANK1_SIZE,
    _SOC_ISM_256_BANK2_SIZE, _SOC_ISM_256_BANK3_SIZE, 
    _SOC_ISM_256_BANK4_SIZE,
    _SOC_ISM_256_BANK0_SIZE, _SOC_ISM_256_BANK1_SIZE,
    _SOC_ISM_256_BANK2_SIZE, _SOC_ISM_256_BANK3_SIZE, 
    _SOC_ISM_256_BANK4_SIZE,
    _SOC_ISM_256_BANK0_SIZE, _SOC_ISM_256_BANK1_SIZE,
    _SOC_ISM_256_BANK2_SIZE, _SOC_ISM_256_BANK3_SIZE, 
    _SOC_ISM_256_BANK4_SIZE
};

uint32 _soc_ism_bank_raw_sizes_256_176[] = {
    _SOC_ISM_256_176_BANK0_SIZE, _SOC_ISM_256_176_BANK1_SIZE,
    _SOC_ISM_256_176_BANK2_SIZE, _SOC_ISM_256_176_BANK3_SIZE,
    _SOC_ISM_256_176_BANK0_SIZE, _SOC_ISM_256_176_BANK1_SIZE,
    _SOC_ISM_256_176_BANK2_SIZE, _SOC_ISM_256_176_BANK3_SIZE,
    _SOC_ISM_256_176_BANK0_SIZE, _SOC_ISM_256_176_BANK1_SIZE,
    _SOC_ISM_256_176_BANK2_SIZE, _SOC_ISM_256_176_BANK3_SIZE,
    _SOC_ISM_256_176_BANK0_SIZE, _SOC_ISM_256_176_BANK1_SIZE,
    _SOC_ISM_256_176_BANK2_SIZE, _SOC_ISM_256_176_BANK3_SIZE
};

uint32 _soc_ism_bank_raw_sizes_256_96[] = {
    _SOC_ISM_256_96_BANK1_SIZE, _SOC_ISM_256_96_BANK2_SIZE, 
    _SOC_ISM_256_96_BANK3_SIZE, _SOC_ISM_256_96_BANK4_SIZE,
    _SOC_ISM_256_96_BANK1_SIZE, _SOC_ISM_256_96_BANK2_SIZE, 
    _SOC_ISM_256_96_BANK3_SIZE, _SOC_ISM_256_96_BANK4_SIZE,
    _SOC_ISM_256_96_BANK1_SIZE, _SOC_ISM_256_96_BANK2_SIZE, 
    _SOC_ISM_256_96_BANK3_SIZE, _SOC_ISM_256_96_BANK4_SIZE,
    _SOC_ISM_256_96_BANK1_SIZE, _SOC_ISM_256_96_BANK2_SIZE, 
    _SOC_ISM_256_96_BANK3_SIZE, _SOC_ISM_256_96_BANK4_SIZE
};

uint32 _soc_ism_bank_raw_sizes_256_80[] = {
    _SOC_ISM_256_80_BANK1_SIZE, _SOC_ISM_256_80_BANK2_SIZE, 
    _SOC_ISM_256_80_BANK3_SIZE, _SOC_ISM_256_80_BANK4_SIZE,
    _SOC_ISM_256_80_BANK1_SIZE, _SOC_ISM_256_80_BANK2_SIZE, 
    _SOC_ISM_256_80_BANK3_SIZE, _SOC_ISM_256_80_BANK4_SIZE,
    _SOC_ISM_256_80_BANK1_SIZE, _SOC_ISM_256_80_BANK2_SIZE, 
    _SOC_ISM_256_80_BANK3_SIZE, _SOC_ISM_256_80_BANK4_SIZE,
    _SOC_ISM_256_80_BANK1_SIZE, _SOC_ISM_256_80_BANK2_SIZE, 
    _SOC_ISM_256_80_BANK3_SIZE, _SOC_ISM_256_80_BANK4_SIZE
};

/* raw to real bank map */
_soc_ism_real_bank_map_t _soc_ism_real_bank_map[] = {
        {8, {0, 1, 2, 3, 4, 5, 6, 7}},
        {4, {8, 9, 10, 11}},
        {2, {12, 13}},
        {1, {14}},
        {1, {15}},
        {8, {16, 17, 18, 19, 20, 21, 22, 23}},
        {4, {24, 25, 26, 27}},
        {2, {28, 29}},
        {1, {30}},
        {1, {31}},
        {8, {32, 33, 34, 35, 36, 37, 38, 39}},
        {4, {40, 41, 42, 43}},
        {2, {44, 45}},
        {1, {46}},
        {1, {47}},
        {8, {48, 49, 50, 51, 52, 53, 54, 55}},
        {4, {56, 57, 58, 59}},
        {2, {60, 61}},
        {1, {62}},
        {1, {63}}
};

_soc_ism_real_bank_map_t _soc_ism_real_bank_map_176[] = {
        {4, {0, 1, 2, 3}},
        {4, {8, 9, 10, 11}},
        {2, {12, 13}},
        {1, {14}},
        {4, {16, 17, 18, 19}},
        {4, {24, 25, 26, 27}},
        {2, {28, 29}},
        {1, {30}},
        {4, {32, 33, 34, 35}},
        {4, {40, 41, 42, 43}},
        {2, {44, 45}},
        {1, {46}},
        {4, {48, 49, 50, 51}},
        {4, {56, 57, 58, 59}},
        {2, {60, 61}},
        {1, {62}}
};

_soc_ism_real_bank_map_t _soc_ism_real_bank_map_96[] = {
        {2, {8, 9}},
        {2, {12, 13}},
        {1, {14}},
        {1, {15}},
        {2, {24, 25}},
        {2, {28, 29}},
        {1, {30}},
        {1, {31}},
        {2, {40, 41}},
        {2, {44, 45}},
        {1, {46}},
        {1, {47}},
        {2, {56, 57}},
        {2, {60, 61}},
        {1, {62}},
        {1, {63}}
};

_soc_ism_real_bank_map_t _soc_ism_real_bank_map_80[] = {
        {2, {8, 9}},
        {1, {12}},
        {1, {14}},
        {1, {15}},
        {2, {24, 25}},
        {1, {28}},
        {1, {30}},
        {1, {31}},
        {2, {40, 41}},
        {1, {44}},
        {1, {46}},
        {1, {47}},
        {2, {56, 57}},
        {1, {60}},
        {1, {62}},
        {1, {63}}
};

STATIC _soc_ism_table_index_t mem_info_entry[] = {
    { "VLAN_XLATE", SOC_ISM_MEM_VLAN_XLATE, 0, 2 },
    { "L2_ENTRY", SOC_ISM_MEM_L2_ENTRY, 1, 2 },
    { "L3_ENTRY", SOC_ISM_MEM_L3_ENTRY, 2, 1 },
    { "EP_VLAN_XLATE", SOC_ISM_MEM_EP_VLAN_XLATE, 3, 4 },
    { "MPLS", SOC_ISM_MEM_MPLS, 4, 2 },
    { "ESM_L2", SOC_ISM_MEM_ESM_L2, 5, 1 },
    { "ESM_L3", SOC_ISM_MEM_ESM_L3, 6, 1 },
    { "ESM_ACL", SOC_ISM_MEM_ESM_ACL, 7, 1 }
};

/* Populated dynamically */
STATIC uint32 _soc_ism_table_bank_count[SOC_MAX_NUM_DEVICES]
                                       [_SOC_ISM_MAX_TABLES];

/* Populated dynamically */
STATIC uint8 _soc_ism_table_bank_config[SOC_MAX_NUM_DEVICES]
                  [_SOC_ISM_MAX_TABLES][_SOC_ISM_MAX_BANKS];

/* Populated dynamically */
STATIC uint32 _soc_ism_table_raw_bank_count[SOC_MAX_NUM_DEVICES]
                                           [_SOC_ISM_MAX_TABLES];

/* Populated dynamically */
STATIC uint32 _soc_ism_log_to_phy_map[SOC_MAX_NUM_DEVICES]
                [_SOC_ISM_MAX_TABLES][_SOC_ISM_TOTAL_BANKS];

/* Power Down structures */
_soc_ism_pd_t _soc_ism_pd[_SOC_ISM_MAX_STAGES][_SOC_ISM_BANKS_PER_STAGE] = {
    {
        {
            {
                { STAGE0_MEMORY_CONTROL_0r, { BANK0_PDA0f, BANK0_PDA1f, INVALIDf } },
                { STAGE0_MEMORY_CONTROL_1r, { BANK0_PDA2f, BANK0_PDA3f, INVALIDf } },
                { STAGE0_MEMORY_CONTROL_2r, { BANK0_PDA4f, BANK0_PDA5f, INVALIDf } },
                { STAGE0_MEMORY_CONTROL_3r, { BANK0_PDA6f, BANK0_PDA7f, INVALIDf } },
                { STAGE0_MEMORY_CONTROL_4r, { BANK0_PDA8f, BANK0_PDA9f, INVALIDf } },
                { STAGE0_MEMORY_CONTROL_5r, { BANK0_PDA10f, BANK0_PDA11f, INVALIDf } },
                { INVALIDr }
            },
            {
                { STAGE0_MEMORY_CONTROL_23r, { BANK0_HIT_PDA0f, BANK0_HIT_PDA1f, INVALIDf } },
                { INVALIDr }
            }
        },   
        {
            {    
                { STAGE0_MEMORY_CONTROL_10r, { BANK1_PDA0f, BANK1_PDA1f, INVALIDf } },
                { STAGE0_MEMORY_CONTROL_11r, { BANK1_PDA2f, BANK1_PDA3f, INVALIDf } },
                { STAGE0_MEMORY_CONTROL_12r, { BANK1_PDA4f, BANK1_PDA5f, INVALIDf } },
                { INVALIDr }
            },
            {
                { STAGE0_MEMORY_CONTROL_25r, { BANK1_HIT_PDA0f, INVALIDf } },
                { INVALIDr }
            }
        },
        {
            {
                { STAGE0_MEMORY_CONTROL_15r, { BANK2_PDA0f, BANK2_PDA1f, BANK2_PDA2f, 
                                               BANK2_PDA3f, INVALIDf } },
                { STAGE0_MEMORY_CONTROL_16r, { BANK2_PDA4f, BANK2_PDA5f, INVALIDf } },
                { INVALIDr }
            },
            {
                { STAGE0_MEMORY_CONTROL_26r, { BANK2_HIT_PDA0f, INVALIDf } },
                { INVALIDr }
            }
        },
        {
            {
                { STAGE0_MEMORY_CONTROL_19r, { BANK3_PDA0f, BANK3_PDA1f, INVALIDf } },
                { INVALIDr }
            },
            {
                { STAGE0_MEMORY_CONTROL_27r, { BANK3_HIT_PDA0f, INVALIDf } },
                { INVALIDr }
            }
        },
        {
            {
                { STAGE0_MEMORY_CONTROL_21r, { BANK4_PDA0f, BANK4_PDA1f, INVALIDf } },
                { INVALIDr }
            },
            {
                { STAGE0_MEMORY_CONTROL_27r, { BANK4_HIT_PDA0f, INVALIDf } },
                { INVALIDr }
            }
        }
    },
    {
        {
            {
                { STAGE1_MEMORY_CONTROL_0r, { BANK0_PDA0f, BANK0_PDA1f, INVALIDf } },
                { STAGE1_MEMORY_CONTROL_1r, { BANK0_PDA2f, BANK0_PDA3f, INVALIDf } },
                { STAGE1_MEMORY_CONTROL_2r, { BANK0_PDA4f, BANK0_PDA5f, INVALIDf } },
                { STAGE1_MEMORY_CONTROL_3r, { BANK0_PDA6f, BANK0_PDA7f, INVALIDf } },
                { STAGE1_MEMORY_CONTROL_4r, { BANK0_PDA8f, BANK0_PDA9f, INVALIDf } },
                { STAGE1_MEMORY_CONTROL_5r, { BANK0_PDA10f, BANK0_PDA11f, INVALIDf } },
                { INVALIDr }
            },
            {
                { STAGE1_MEMORY_CONTROL_23r, { BANK0_HIT_PDA0f, BANK0_HIT_PDA1f, INVALIDf } },
                { INVALIDr }
            }
        },   
        {
            {    
                { STAGE1_MEMORY_CONTROL_10r, { BANK1_PDA0f, BANK1_PDA1f, INVALIDf } },
                { STAGE1_MEMORY_CONTROL_11r, { BANK1_PDA2f, BANK1_PDA3f, INVALIDf } },
                { STAGE1_MEMORY_CONTROL_12r, { BANK1_PDA4f, BANK1_PDA5f, INVALIDf } },
                { INVALIDr }
            },
            {
                { STAGE1_MEMORY_CONTROL_25r, { BANK1_HIT_PDA0f, INVALIDf } },
                { INVALIDr }
            }
        },
        {
            {
                { STAGE1_MEMORY_CONTROL_15r, { BANK2_PDA0f, BANK2_PDA1f, BANK2_PDA2f, 
                                               BANK2_PDA3f, INVALIDf } },
                { STAGE1_MEMORY_CONTROL_16r, { BANK2_PDA4f, BANK2_PDA5f, INVALIDf } },
                { INVALIDr }
            },
            {
                { STAGE1_MEMORY_CONTROL_26r, { BANK2_HIT_PDA0f, INVALIDf } },
                { INVALIDr }
            }
        },
        {
            {
                { STAGE1_MEMORY_CONTROL_19r, { BANK3_PDA0f, BANK3_PDA1f, INVALIDf } },
                { INVALIDr }
            },
            {
                { STAGE1_MEMORY_CONTROL_27r, { BANK3_HIT_PDA0f, INVALIDf } },
                { INVALIDr }
            }
        },
        {
            {
                { STAGE1_MEMORY_CONTROL_21r, { BANK4_PDA0f, BANK4_PDA1f, INVALIDf } },
                { INVALIDr }
            },
            {
                { STAGE1_MEMORY_CONTROL_27r, { BANK4_HIT_PDA0f, INVALIDf } },
                { INVALIDr }
            }
        }
    },
    {
        {
            {
                { STAGE2_MEMORY_CONTROL_0r, { BANK0_PDA0f, BANK0_PDA1f, INVALIDf } },
                { STAGE2_MEMORY_CONTROL_1r, { BANK0_PDA2f, BANK0_PDA3f, INVALIDf } },
                { STAGE2_MEMORY_CONTROL_2r, { BANK0_PDA4f, BANK0_PDA5f, INVALIDf } },
                { STAGE2_MEMORY_CONTROL_3r, { BANK0_PDA6f, BANK0_PDA7f, INVALIDf } },
                { STAGE2_MEMORY_CONTROL_4r, { BANK0_PDA8f, BANK0_PDA9f, INVALIDf } },
                { STAGE2_MEMORY_CONTROL_5r, { BANK0_PDA10f, BANK0_PDA11f, INVALIDf } },
                { INVALIDr }
            },
            {
                { STAGE2_MEMORY_CONTROL_23r, { BANK0_HIT_PDA0f, BANK0_HIT_PDA1f, INVALIDf } },
                { INVALIDr }
            }
        },   
        {
            {    
                { STAGE2_MEMORY_CONTROL_10r, { BANK1_PDA0f, BANK1_PDA1f, INVALIDf } },
                { STAGE2_MEMORY_CONTROL_11r, { BANK1_PDA2f, BANK1_PDA3f, INVALIDf } },
                { STAGE2_MEMORY_CONTROL_12r, { BANK1_PDA4f, BANK1_PDA5f, INVALIDf } },
                { INVALIDr }
            },
            {
                { STAGE2_MEMORY_CONTROL_25r, { BANK1_HIT_PDA0f, INVALIDf } },
                { INVALIDr }
            }
        },
        {
            {
                { STAGE2_MEMORY_CONTROL_15r, { BANK2_PDA0f, BANK2_PDA1f, BANK2_PDA2f, 
                                               BANK2_PDA3f, INVALIDf } },
                { STAGE2_MEMORY_CONTROL_16r, { BANK2_PDA4f, BANK2_PDA5f, INVALIDf } },
                { INVALIDr }
            },
            {
                { STAGE2_MEMORY_CONTROL_26r, { BANK2_HIT_PDA0f, INVALIDf } },
                { INVALIDr }
            }
        },
        {
            {
                { STAGE2_MEMORY_CONTROL_19r, { BANK3_PDA0f, BANK3_PDA1f, INVALIDf } },
                { INVALIDr }
            },
            {
                { STAGE2_MEMORY_CONTROL_27r, { BANK3_HIT_PDA0f, INVALIDf } },
                { INVALIDr }
            }
        },
        {
            {
                { STAGE2_MEMORY_CONTROL_21r, { BANK4_PDA0f, BANK4_PDA1f, INVALIDf } },
                { INVALIDr }
            },
            {
                { STAGE2_MEMORY_CONTROL_27r, { BANK4_HIT_PDA0f, INVALIDf } },
                { INVALIDr }
            }
        }
    },
    {
        {
            {
                { STAGE3_MEMORY_CONTROL_0r, { BANK0_PDA0f, BANK0_PDA1f, INVALIDf } },
                { STAGE3_MEMORY_CONTROL_1r, { BANK0_PDA2f, BANK0_PDA3f, INVALIDf } },
                { STAGE3_MEMORY_CONTROL_2r, { BANK0_PDA4f, BANK0_PDA5f, INVALIDf } },
                { STAGE3_MEMORY_CONTROL_3r, { BANK0_PDA6f, BANK0_PDA7f, INVALIDf } },
                { STAGE3_MEMORY_CONTROL_4r, { BANK0_PDA8f, BANK0_PDA9f, INVALIDf } },
                { STAGE3_MEMORY_CONTROL_5r, { BANK0_PDA10f, BANK0_PDA11f, INVALIDf } },
                { INVALIDr }
            },
            {
                { STAGE3_MEMORY_CONTROL_23r, { BANK0_HIT_PDA0f, BANK0_HIT_PDA1f, INVALIDf } },
                { INVALIDr }
            }
        },   
        {
            {    
                { STAGE3_MEMORY_CONTROL_10r, { BANK1_PDA0f, BANK1_PDA1f, INVALIDf } },
                { STAGE3_MEMORY_CONTROL_11r, { BANK1_PDA2f, BANK1_PDA3f, INVALIDf } },
                { STAGE3_MEMORY_CONTROL_12r, { BANK1_PDA4f, BANK1_PDA5f, INVALIDf } },
                { INVALIDr }
            },
            {
                { STAGE3_MEMORY_CONTROL_25r, { BANK1_HIT_PDA0f, INVALIDf } },
                { INVALIDr }
            }
        },
        {
            {
                { STAGE3_MEMORY_CONTROL_15r, { BANK2_PDA0f, BANK2_PDA1f, BANK2_PDA2f, 
                                               BANK2_PDA3f, INVALIDf } },
                { STAGE3_MEMORY_CONTROL_16r, { BANK2_PDA4f, BANK2_PDA5f, INVALIDf } },
                { INVALIDr }
            },
            {
                { STAGE3_MEMORY_CONTROL_26r, { BANK2_HIT_PDA0f, INVALIDf } },
                { INVALIDr }
            }
        },
        {
            {
                { STAGE3_MEMORY_CONTROL_19r, { BANK3_PDA0f, BANK3_PDA1f, INVALIDf } },
                { INVALIDr }
            },
            {
                { STAGE3_MEMORY_CONTROL_27r, { BANK3_HIT_PDA0f, INVALIDf } },
                { INVALIDr }
            }
        },
        {
            {
                { STAGE3_MEMORY_CONTROL_21r, { BANK4_PDA0f, BANK4_PDA1f, INVALIDf } },
                { INVALIDr }
            },
            {
                { STAGE3_MEMORY_CONTROL_27r, { BANK4_HIT_PDA0f, INVALIDf } },
                { INVALIDr }
            }
        }
    }
};

/*
 * Macro used by memory accessor functions to fix order
 */
#define FIX_MEM_ORDER_E(v,m) (((m)->flags & SOC_MEM_FLAG_BE) ? \
                                BYTES2WORDS((m)->bytes)-1-(v) : \
                                (v))

/* helper routine */
int
soc_ism_get_hash_mem_idx(int unit, soc_mem_t mem)
{
    int8 i;
    for (i = 0; i < _SOC_ISM_MAX_ISM_MEMS; i++) {
         if (mem == _SOC_ISM_MEMS(unit)[i].mem) {
             return i;
         }
    }
    return SOC_E_PARAM;
}

/* helper routine */
int 
soc_ism_table_to_index(uint32 mem) 
{
    int i, idx = -1;
    for (i = 0; i < COUNTOF(mem_info_entry); i++) {
        if (mem == mem_info_entry[i].mem) {
            idx = mem_info_entry[i].index;
            break;
        }
    }
    return idx;
}

/* helper routine */
char *
soc_ism_table_to_name(uint32 mem)
{
    int i;
    for (i = 0; i < COUNTOF(mem_info_entry); i++) {
        if (mem == mem_info_entry[i].mem) {
            return mem_info_entry[i].name;
            break;
        }
    }
    return NULL;
}

/* Fill _soc_ism_log_to_phy_map based upon _soc_ism_table_bank_config */
STATIC int
soc_ism_log_to_phy_fill(int unit)
{
    int32 tab, b, c, r;
    int8 bank;
    for (tab = 0; tab < _SOC_ISM_MAX_TABLES; tab++) {
        r = 0;
        for (b = 0; b < SOC_ISM_INFO(unit)->max_banks; b++) {
            bank = ((b%_SOC_ISM_MAX_STAGES) * 
                    SOC_ISM_INFO(unit)->banks_per_stage) + 
                   (b/_SOC_ISM_MAX_STAGES);
            if (_soc_ism_table_bank_config[unit][tab][bank]) {
                for (c = 0; c < 
                     SOC_ISM_INFO(unit)->real_bank_map[bank].count; c++) {
                    _soc_ism_log_to_phy_map[unit][tab][r] = 
                       SOC_ISM_INFO(unit)->real_bank_map[bank].index[c];
                    r++;
                }
                _soc_ism_table_raw_bank_count[unit][tab] += 
                    SOC_ISM_INFO(unit)->real_bank_map[bank].count;
                if (_soc_ism_table_raw_bank_count[unit][tab] >
                    SOC_ISM_INFO(unit)->total_banks) {
                    return SOC_E_PARAM;
                } 
            }
        }
    }
    return SOC_E_NONE;
}

STATIC soc_mem_t _ism_log_to_phy_mem[] = {
    TABLE0_LOG_TO_PHY_MAPm, TABLE1_LOG_TO_PHY_MAPm,
    TABLE2_LOG_TO_PHY_MAPm, TABLE3_LOG_TO_PHY_MAPm,
    TABLE4_LOG_TO_PHY_MAPm
};

/* Write _soc_ism_log_to_phy_map to TABLEx_LOG_TO_PHY_MAPm */
STATIC int
soc_ism_log_to_phy_set(int unit)
{
    int tab, bank;
    uint32 entry[SOC_MAX_MEM_WORDS];
    for (tab = 0; tab < _SOC_ISM_MAX_TABLES; tab++) {
        LOG_INFO(BSL_LS_SOC_SOCMEM,
                 (BSL_META_U(unit,
                             "Table: %d\n"), tab));
        for (bank = 0; bank < SOC_ISM_INFO(unit)->total_banks; bank++) {
            LOG_INFO(BSL_LS_SOC_SOCMEM,
                     (BSL_META_U(unit,
                                 "[%d]-%d "), bank, 
                      _soc_ism_log_to_phy_map[unit][tab][bank]));
            sal_memset(&entry, 0, sizeof(entry));
            soc_mem_field32_set(unit, _ism_log_to_phy_mem[tab], entry,
                                LOG_TO_PHY_MAPf, 
                                _soc_ism_log_to_phy_map[unit][tab][bank]);
            /* Write to TABLEx_LOG_TO_PHY_MAPm memory */
            SOC_IF_ERROR_RETURN
                (soc_mem_write(unit, _ism_log_to_phy_mem[tab], 
                               MEM_BLOCK_ALL, bank, &entry));
        }        
        LOG_INFO(BSL_LS_SOC_SOCMEM,
                 (BSL_META_U(unit,
                             "\n")));
    }
    return SOC_E_NONE;
}

soc_reg_t _ism_table_bank_cfg_reg[] = {
    TABLE0_BANK_CONFIGr, TABLE1_BANK_CONFIGr,
    TABLE2_BANK_CONFIGr, TABLE3_BANK_CONFIGr,
    TABLE4_BANK_CONFIGr
};

soc_field_t _ism_table_bank_cfg_fld[] = {
    STAGE0_BANKSf, STAGE1_BANKSf,
    STAGE2_BANKSf, STAGE3_BANKSf
};

/* Write _soc_ism_table_bank_config to TABLEx_BANK_CONFIG registers */
STATIC int 
soc_ism_table_bank_set(int unit)
{
    int tab, stg, bank, offset;
    uint32 rval, val;
    for (tab = 0; tab < _SOC_ISM_MAX_TABLES; tab++) {
        if (!_soc_ism_table_bank_count[unit][tab]) {
            continue;
        }
        LOG_INFO(BSL_LS_SOC_SOCMEM,
                 (BSL_META_U(unit,
                             "Table: %d\n"), tab));
        SOC_IF_ERROR_RETURN
        (soc_reg32_get(unit, _ism_table_bank_cfg_reg[tab], REG_PORT_ANY, 
                       0, &rval));
        for (stg = 0; stg < _SOC_ISM_MAX_STAGES; stg++) {
            val = 0;
            /* Take disabled bank into account */
            offset = ((SOC_ISM_INFO(unit)->ism_mode == _ISM_SIZE_MODE_96) || 
                      (SOC_ISM_INFO(unit)->ism_mode == _ISM_SIZE_MODE_80)) ? 
                      ((SOC_ISM_INFO(unit)->ism_mode == _ISM_SIZE_MODE_64) ? 2 : 1) : 0;
            for (bank = 0; bank < SOC_ISM_INFO(unit)->banks_per_stage; bank++) {
                val |= _soc_ism_table_bank_config[unit][tab]
                      [(stg*SOC_ISM_INFO(unit)->banks_per_stage)+bank] << (bank + offset);
            }
            LOG_INFO(BSL_LS_SOC_SOCMEM,
                     (BSL_META_U(unit,
                                 "stage: %d - bmask: %x "), stg, val));
            
            soc_reg_field_set(unit, _ism_table_bank_cfg_reg[tab], 
                              &rval, _ism_table_bank_cfg_fld[stg],
                              val);
        }
        if (SOC_ISM_INFO(unit)->ism_mode != _ISM_SIZE_MODE_512) {
            soc_reg_field_set(unit, _ism_table_bank_cfg_reg[tab], 
                              &rval, MAPPING_MODEf, 1);
        }
        /* Write to TABLEx_BANK_CONFIG register */
        SOC_IF_ERROR_RETURN
            (soc_reg32_set(unit, _ism_table_bank_cfg_reg[tab], 
                           REG_PORT_ANY, 0, rval));
        LOG_INFO(BSL_LS_SOC_SOCMEM,
                 (BSL_META_U(unit,
                             "\n")));
    }
    return SOC_E_NONE;
}

/* helper routine to sort and prioritize mem types based upon allocation rules */
STATIC void
_soc_ism_sort_mems(soc_ism_mem_size_config_t *i_mem_cfg, int count,
                      soc_ism_mem_size_config_t *o_mem_cfg)
{
    int i, j, esm = 0, ism = 0;
    soc_ism_mem_size_config_t tmp;
    soc_ism_mem_size_config_t arr[SOC_ISM_MEM_TOTAL];
    
    sal_memset(o_mem_cfg, 0, 
               sizeof(soc_ism_mem_size_config_t) * SOC_ISM_MEM_TOTAL);
    sal_memcpy(o_mem_cfg, i_mem_cfg, 
               sizeof(soc_ism_mem_size_config_t) * count);
    if (count == 1) {
        return;
    }
    /* Search for and fetch ESM mems */
    for (i = 0; i < count; i++) {
        if (i_mem_cfg[i].mem >= SOC_ISM_MEM_MAX) {
            arr[esm] = i_mem_cfg[i];
            esm++;
        }
    }
    if (esm) {
        /* sort esm mems */
        for (i = 0; i < esm; i++) {
            for (j = 0; j < esm - i - 1; j++) {
                if (arr[j].size < arr[j+1].size) {
                    sal_memcpy(&tmp, &arr[j], 
                               sizeof(soc_ism_mem_size_config_t));
                    sal_memcpy(&arr[j], &arr[j+1],
                               sizeof(soc_ism_mem_size_config_t));
                    sal_memcpy(&arr[j+1], &tmp,
                               sizeof(soc_ism_mem_size_config_t));
                }
            }
        }
        /* copy back sorted esm mems */
        sal_memcpy(o_mem_cfg, arr, 
                   sizeof(soc_ism_mem_size_config_t) * esm);
        if ((count - esm) == 0) {
            return;
        }
    }
    /* Search for and fetch ISM mems */
    for (i = 0; i < count; i++) {
        if (i_mem_cfg[i].mem < SOC_ISM_MEM_MAX) {
            arr[ism] = i_mem_cfg[i];
            ism++;
        }
    }
    if (ism) {
        /* sort ism mems */
        for (i = 0; i < ism; i++) {
            for (j = 0; j < ism - i - 1; j++) {
                if (arr[j].size < arr[j+1].size) {
                    sal_memcpy(&tmp, &arr[j], 
                               sizeof(soc_ism_mem_size_config_t));
                    sal_memcpy(&arr[j], &arr[j+1],
                               sizeof(soc_ism_mem_size_config_t));
                    sal_memcpy(&arr[j+1], &tmp,
                               sizeof(soc_ism_mem_size_config_t));
                }
            }
        }
        /* copy back sorted esm mems */
        sal_memcpy(&o_mem_cfg[esm], arr, 
                   sizeof(soc_ism_mem_size_config_t) * ism);
    }
}

uint32
_soc_ism_bank_total(int unit)
{
    uint8 i;
    uint32 total = 0;
    for (i = 0; i < SOC_ISM_INFO(unit)->max_banks; i++) {
        total += SOC_ISM_INFO(unit)->bank_raw_sizes[i];
    }
    return total;
}

/* Support simple, sequential configuration for now and
   fill _soc_ism_table_bank_config based upon driver memory config */ 
int
soc_ism_mem_config(int unit, soc_ism_mem_size_config_t *mem_cfg, int count)
{
    int rv, tab, idx, bank;
    int midx, size;
    uint16 dev_id;
    uint8 rev_id;
    uint32 mem, _ism_total = 0;
    soc_ism_mem_size_config_t ism_hash_tables[SOC_ISM_MEM_TOTAL];
    soc_persist_t *sop = SOC_PERSIST(unit);
    SOC_ISM_INFO(unit) = &_soc_ism_info[unit];
    SOC_ISM_HASH_INFO(unit) = &_soc_ism_hash_info[unit];
    
    sal_memset(&_soc_ism_bank_avail[unit], 0, 
                sizeof(uint32) * _SOC_ISM_MAX_BANKS);
    sal_memset(&_soc_ism_table_bank_count[unit], 0, 
                sizeof(uint32) * _SOC_ISM_MAX_TABLES);
    sal_memset(&_soc_ism_table_bank_config[unit], 0, 
                sizeof(uint8) * _SOC_ISM_MAX_TABLES * _SOC_ISM_MAX_BANKS);
    sal_memset(&_soc_ism_table_raw_bank_count[unit], 0, 
                sizeof(uint32) * _SOC_ISM_MAX_TABLES);
    sal_memset(&_soc_ism_log_to_phy_map[unit], 0, 
                sizeof(uint32) * _SOC_ISM_MAX_TABLES * _SOC_ISM_TOTAL_BANKS);
    
    soc_cm_get_id(unit, &dev_id, &rev_id);
    switch (dev_id) {
    case BCM56640_DEVICE_ID:
    case BCM56643_DEVICE_ID:
    case BCM56644_DEVICE_ID:
        SOC_ISM_INFO(unit)->ism_mode = _ISM_SIZE_MODE_512;
        break;
    case BCM56648_DEVICE_ID:
    case BCM56649_DEVICE_ID:
    case BCM56540_DEVICE_ID:
    case BCM56541_DEVICE_ID:
    case BCM56542_DEVICE_ID:
    case BCM56543_DEVICE_ID:
    case BCM56544_DEVICE_ID:
    case BCM56545_DEVICE_ID:
    case BCM56546_DEVICE_ID:
        SOC_ISM_INFO(unit)->ism_mode = _ISM_SIZE_MODE_176;
        break;
    case BCM56044_DEVICE_ID:
    case BCM56045_DEVICE_ID:
    case BCM56046_DEVICE_ID:
        SOC_ISM_INFO(unit)->ism_mode = _ISM_SIZE_MODE_80;
        break;
#ifdef BCM_HELIX4_SUPPORT
    case BCM56548_DEVICE_ID:
    case BCM56547_DEVICE_ID:
    case BCM56347_DEVICE_ID:
    case BCM56346_DEVICE_ID:
    case BCM56344_DEVICE_ID:
    case BCM56342_DEVICE_ID:
    case BCM56340_DEVICE_ID:
        SOC_ISM_INFO(unit)->ism_mode = _ISM_SIZE_MODE_96;
        break;
    case BCM56042_DEVICE_ID:
    case BCM56041_DEVICE_ID:
    case BCM56040_DEVICE_ID:
    case BCM56049_DEVICE_ID:
    case BCM56048_DEVICE_ID:
    case BCM56047_DEVICE_ID:        
        SOC_ISM_INFO(unit)->ism_mode = _ISM_SIZE_MODE_80;
        break;
#endif
    default:
         return SOC_E_PARAM;
    }
    
    switch (SOC_ISM_INFO(unit)->ism_mode) {
    case _ISM_SIZE_MODE_512:
        SOC_ISM_INFO(unit)->bank_raw_sizes = _soc_ism_bank_raw_sizes;
        SOC_ISM_INFO(unit)->banks_per_stage = _SOC_ISM_BANKS_PER_STAGE;
        SOC_ISM_INFO(unit)->max_banks = _SOC_ISM_MAX_BANKS;
        SOC_ISM_INFO(unit)->total_banks = _SOC_ISM_TOTAL_BANKS;
        SOC_ISM_INFO(unit)->total_entries = _SOC_ISM_ENTRIES_PER_BKT *
                                            _soc_ism_bank_total(unit);
        SOC_ISM_INFO(unit)->max_raw_banks = _SOC_ISM_MAX_RAW_BANKS;
        SOC_ISM_INFO(unit)->real_bank_map = _soc_ism_real_bank_map;
        break;
    case _ISM_SIZE_MODE_256:
        SOC_ISM_INFO(unit)->bank_raw_sizes = _soc_ism_bank_raw_sizes_256;
        SOC_ISM_INFO(unit)->banks_per_stage = _SOC_ISM_BANKS_PER_STAGE;
        SOC_ISM_INFO(unit)->max_banks = _SOC_ISM_MAX_BANKS;
        SOC_ISM_INFO(unit)->total_banks = _SOC_ISM_TOTAL_BANKS;
        SOC_ISM_INFO(unit)->total_entries = _SOC_ISM_ENTRIES_PER_BKT *
                                            _soc_ism_bank_total(unit);
        SOC_ISM_INFO(unit)->max_raw_banks = _SOC_ISM_MAX_RAW_BANKS;
        SOC_ISM_INFO(unit)->real_bank_map = _soc_ism_real_bank_map;
        break;
    case _ISM_SIZE_MODE_176:
        SOC_ISM_INFO(unit)->bank_raw_sizes = _soc_ism_bank_raw_sizes_256_176;
        SOC_ISM_INFO(unit)->banks_per_stage = _SOC_ISM_BANKS_PER_STAGE_176;
        SOC_ISM_INFO(unit)->max_banks = _SOC_ISM_MAX_BANKS_176;
        SOC_ISM_INFO(unit)->total_banks = _SOC_ISM_TOTAL_BANKS_176;
        SOC_ISM_INFO(unit)->total_entries = _SOC_ISM_ENTRIES_PER_BKT *
                                            _soc_ism_bank_total(unit);
        SOC_ISM_INFO(unit)->max_raw_banks = _SOC_ISM_MAX_RAW_BANKS;
        SOC_ISM_INFO(unit)->real_bank_map = _soc_ism_real_bank_map_176;
        break;
    case _ISM_SIZE_MODE_96:
        SOC_ISM_INFO(unit)->bank_raw_sizes = _soc_ism_bank_raw_sizes_256_96;
        SOC_ISM_INFO(unit)->banks_per_stage = _SOC_ISM_BANKS_PER_STAGE_96;
        SOC_ISM_INFO(unit)->max_banks = _SOC_ISM_MAX_BANKS_96;
        SOC_ISM_INFO(unit)->total_banks = _SOC_ISM_TOTAL_BANKS_96;
        SOC_ISM_INFO(unit)->total_entries = _SOC_ISM_ENTRIES_PER_BKT *
                                            _soc_ism_bank_total(unit);
        SOC_ISM_INFO(unit)->max_raw_banks = _SOC_ISM_MAX_RAW_BANKS;
        SOC_ISM_INFO(unit)->real_bank_map = _soc_ism_real_bank_map_96;
        break;
    case _ISM_SIZE_MODE_80:
        SOC_ISM_INFO(unit)->bank_raw_sizes = _soc_ism_bank_raw_sizes_256_80;
        SOC_ISM_INFO(unit)->banks_per_stage = _SOC_ISM_BANKS_PER_STAGE_80;
        SOC_ISM_INFO(unit)->max_banks = _SOC_ISM_MAX_BANKS_80;
        SOC_ISM_INFO(unit)->total_banks = _SOC_ISM_TOTAL_BANKS_80;
        SOC_ISM_INFO(unit)->total_entries = _SOC_ISM_ENTRIES_PER_BKT *
                                            _soc_ism_bank_total(unit);
        SOC_ISM_INFO(unit)->max_raw_banks = _SOC_ISM_MAX_RAW_BANKS;
        SOC_ISM_INFO(unit)->real_bank_map = _soc_ism_real_bank_map_80;
        break;
    default:
         return SOC_E_PARAM;
    }
    
    /* Attach the hash cfg structures based upon chip type 
       (currently only one type; will change to use if-else in the future) */
    _SOC_ISM_BANKS(unit) = _soc_ism_shb[unit];
    _SOC_ISM_SETS(unit) = _soc_ism_shms;
    _SOC_ISM_MEMS(unit) = _soc_ism_shm;
    _SOC_ISM_VIEWS(unit) = _soc_ism_shmv;
    _SOC_ISM_KEYS(unit) = _soc_ism_shk;
    
    /* Sort the memories in decreasing order of sizes and allocation priority */
    _soc_ism_sort_mems(mem_cfg, count, ism_hash_tables);

    for (tab = 0; tab < count && tab < SOC_ISM_MEM_TOTAL; tab++) {
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "ISM mem: [%s], size: %d\n"), 
                     soc_ism_table_to_name(ism_hash_tables[tab].mem),
                     ism_hash_tables[tab].size));
    }
    sop->memState[VLAN_XLATEm].index_max = -1;
    sop->memState[VLAN_XLATE_1m].index_max = -1;
    sop->memState[VLAN_XLATE_1_HIT_ONLYm].index_max = -1;
    sop->memState[VLAN_XLATE_EXTDm].index_max = -1;
    sop->memState[VLAN_XLATE_2_HIT_ONLYm].index_max = -1;
    sop->memState[L2_ENTRY_1m].index_max = -1;
    sop->memState[L2_ENTRY_1_HIT_ONLYm].index_max = -1;
    sop->memState[L2_ENTRY_2m].index_max = -1;
    sop->memState[L2_ENTRY_2_HIT_ONLYm].index_max = -1;
    sop->memState[L3_ENTRY_1m].index_max = -1;
    sop->memState[L3_ENTRY_1_HIT_ONLYm].index_max = -1;
    sop->memState[L3_ENTRY_2m].index_max = -1;
    sop->memState[L3_ENTRY_2_HIT_ONLYm].index_max = -1;
    sop->memState[L3_ENTRY_4m].index_max = -1;
    sop->memState[L3_ENTRY_4_HIT_ONLYm].index_max = -1;
    sop->memState[EGR_VLAN_XLATEm].index_max = -1;
    sop->memState[EP_VLAN_XLATE_1m].index_max = -1;
    sop->memState[EP_VLAN_XLATE_1_HIT_ONLYm].index_max = -1;
    sop->memState[MPLS_ENTRYm].index_max = -1;
    sop->memState[MPLS_ENTRY_1m].index_max = -1;
    sop->memState[MPLS_ENTRY_1_HIT_ONLYm].index_max = -1;
    sop->memState[MPLS_ENTRY_EXTDm].index_max = -1;
    sop->memState[MPLS_ENTRY_2_HIT_ONLYm].index_max = -1;
    for (tab = 0; tab < count && tab < SOC_ISM_MEM_TOTAL; tab++) {
        uint8 avail;
        int rem;
        mem = ism_hash_tables[tab].mem;
        midx = soc_ism_table_to_index(mem);
        if (midx == -1) {
            return SOC_E_PARAM;
        }
        if (ism_hash_tables[tab].size <= 0 ) {
            continue;
        }
        size = ism_hash_tables[tab].size;
        rem = size % _SOC_ISM_ENTRY_SIZE_QUANTA;
        if (rem) {
            size = size + (_SOC_ISM_ENTRY_SIZE_QUANTA - rem);
            LOG_VERBOSE(BSL_LS_SOC_COMMON,
                        (BSL_META_U(unit,
                                    "Updated size %s: %d\n"), 
                         soc_ism_table_to_name(mem), size));
        }
        ism_hash_tables[tab].size = size;
        do {
            /* convert to num of buckets */ 
            size = size / mem_info_entry[midx].epb; 
            avail = 0;
            for (idx = 0; idx < SOC_ISM_INFO(unit)->max_banks; idx++) {
                bank = ((idx%_SOC_ISM_MAX_STAGES) * 
                        SOC_ISM_INFO(unit)->banks_per_stage) +
                       (idx/_SOC_ISM_MAX_STAGES);
                if (_soc_ism_bank_avail[unit][bank]) {
                    continue;
                }
                avail++;
                /* Rule: ESM stuff can only be in banks >= 4k in size */
                if (mem >= SOC_ISM_MEM_MAX && 
                    SOC_ISM_INFO(unit)->bank_raw_sizes[bank] < 1024*4) {
                    continue;
                }
                if (size >= SOC_ISM_INFO(unit)->bank_raw_sizes[bank]) {
                    _ism_total += SOC_ISM_INFO(unit)->bank_raw_sizes[bank] * \
                                  _SOC_ISM_ENTRIES_PER_BKT;
                    size -= SOC_ISM_INFO(unit)->bank_raw_sizes[bank];
                    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                                (BSL_META_U(unit,
                                            "bank: %d\n"), bank));
                    _soc_ism_bank_avail[unit][bank] = mem;
                    if (mem < SOC_ISM_MEM_MAX) {
                        _soc_ism_table_bank_count[unit][midx]++;
                        if (_soc_ism_table_bank_count[unit][midx] > 
                            SOC_ISM_INFO(unit)->max_banks) {
                            return SOC_E_PARAM;
                        } 
                        _soc_ism_table_bank_config[unit][midx][bank] = 1; 
                    }
                }
                if (!size) {
                    goto next_mem;
                }
            }
            if (size) {
                /* Bump up num of entries to allocate the next bigger bank */
                size = size * mem_info_entry[midx].epb + _SOC_ISM_ENTRY_SIZE_QUANTA;
                ism_hash_tables[tab].size += _SOC_ISM_ENTRY_SIZE_QUANTA;
            } 
        } while (size && avail); /* Keep trying */
        if (size) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "Could not allocate banks for mem: %s\n"),
                       soc_ism_table_to_name(mem)));
            return SOC_E_PARAM;
        }
next_mem:
        size = ism_hash_tables[tab].size;
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "Final size %s: %d\n"), 
                     soc_ism_table_to_name(mem), size));
        switch (mem) {
        case SOC_ISM_MEM_VLAN_XLATE:
            sop->memState[VLAN_XLATEm].index_max += (size * 2);
            sop->memState[VLAN_XLATE_1m].index_max += (size * 2);
            sop->memState[VLAN_XLATE_1_HIT_ONLYm].index_max += (size * 2);
            sop->memState[VLAN_XLATE_EXTDm].index_max += size;
            sop->memState[VLAN_XLATE_2_HIT_ONLYm].index_max += size;
            break;
        case SOC_ISM_MEM_L2_ENTRY:
            sop->memState[L2_ENTRY_1m].index_max += (size * 2) ;
            sop->memState[L2_ENTRY_1_HIT_ONLYm].index_max += (size * 2) ;
            sop->memState[L2_ENTRY_2m].index_max += size ;
            sop->memState[L2_ENTRY_2_HIT_ONLYm].index_max += size ;
            break;
        case SOC_ISM_MEM_L3_ENTRY:
            sop->memState[L3_ENTRY_1m].index_max += (size * 4);
            sop->memState[L3_ENTRY_1_HIT_ONLYm].index_max += (size * 4);
            sop->memState[L3_ENTRY_2m].index_max += (size * 2);
            sop->memState[L3_ENTRY_2_HIT_ONLYm].index_max += (size * 2);
            sop->memState[L3_ENTRY_4m].index_max += size;
            sop->memState[L3_ENTRY_4_HIT_ONLYm].index_max += size;
            break;
        case SOC_ISM_MEM_EP_VLAN_XLATE:
            sop->memState[EGR_VLAN_XLATEm].index_max += size;
            sop->memState[EP_VLAN_XLATE_1m].index_max += size;
            sop->memState[EP_VLAN_XLATE_1_HIT_ONLYm].index_max += size;
            break;
        case SOC_ISM_MEM_MPLS:
            sop->memState[MPLS_ENTRYm].index_max += (size * 2);
            sop->memState[MPLS_ENTRY_1m].index_max += (size * 2);
            sop->memState[MPLS_ENTRY_1_HIT_ONLYm].index_max += (size * 2);
            sop->memState[MPLS_ENTRY_EXTDm].index_max += size;
            sop->memState[MPLS_ENTRY_2_HIT_ONLYm].index_max += size;
            break;
        case SOC_ISM_MEM_ESM_L2:
        case SOC_ISM_MEM_ESM_L3:
        case SOC_ISM_MEM_ESM_ACL:
            break;
        default: return SOC_E_PARAM;
        } 
        continue;
    }
    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "ISM used total: %d k of %d k\n"), 
                 _ism_total/_SOC_ISM_ENTRY_SIZE_QUANTA, 
                 SOC_ISM_INFO(unit)->total_entries/_SOC_ISM_ENTRY_SIZE_QUANTA));
    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "ISM remaining : %d k\n"), 
                 SOC_ISM_INFO(unit)->total_entries/_SOC_ISM_ENTRY_SIZE_QUANTA - 
                 (_ism_total/_SOC_ISM_ENTRY_SIZE_QUANTA)));

    if (_ism_total/_SOC_ISM_ENTRY_SIZE_QUANTA > 
        SOC_ISM_INFO(unit)->total_entries/_SOC_ISM_ENTRY_SIZE_QUANTA) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Over-allocation of ISM resources !!\n")));
        return SOC_E_PARAM;
    }

    rv = soc_ism_log_to_phy_fill(unit);
    if (SOC_FAILURE(rv)) {
        return rv; 
    }
    rv = soc_ism_hash_init(unit);
    if (SOC_FAILURE(rv)) {
        return rv; 
    }
    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "ISM configured.\n")));
    return SOC_E_NONE;
}

/* Internel helper routine */
STATIC int
_soc_ism_get_sorted_bank_list(int unit, soc_mem_t *mems, int *banks) 
{
    int32 i, j, count = 0;
    uint8 done, bank, tmp;
    /* go through banks to get sorted base_entry's */ 
    for (i = 0; i < SOC_ISM_INFO(unit)->max_banks; i++) {
        bank = ((i%_SOC_ISM_MAX_STAGES) * SOC_ISM_INFO(unit)->banks_per_stage) + 
               (i/_SOC_ISM_MAX_STAGES);
        if (!_soc_ism_bank_avail[unit][bank]) {
            continue;
        }
        done = 0; 
        for (j = 0; j < count; j++) {
            if (mems[j] == _soc_ism_bank_avail[unit][bank]) {
                done = 1;
                break;
            } 
        }
        if (done) {
            continue;
        } else {
            banks[count] = bank;
            mems[count++] = _soc_ism_bank_avail[unit][bank];
        }
        for (j = i+1; j < SOC_ISM_INFO(unit)->max_banks; j++) {
            tmp = ((j%_SOC_ISM_MAX_STAGES) * 
                   SOC_ISM_INFO(unit)->banks_per_stage) + 
                  (j/_SOC_ISM_MAX_STAGES);
            if (_soc_ism_bank_avail[unit][bank] == 
                _soc_ism_bank_avail[unit][tmp]) {
                banks[count] = tmp;
                mems[count++] = _soc_ism_bank_avail[unit][tmp];
            }
        } 
    }
    return count;
}

/* Initializes and links various hash structures */
int
soc_ism_hash_init(int unit)
{
    int i, j, k, count = 0;
    soc_mem_t mems[_SOC_ISM_MAX_BANKS];
    int banks[_SOC_ISM_MAX_BANKS];
    
    
    
    sal_memset(_SOC_ISM_BANKS(unit), 0, sizeof(soc_hash_bank_t));
    sal_memset(mems, 0, COUNTOF(mems) * sizeof(soc_mem_t));
    for (i = 0; i < SOC_MEM_SET_MAX; i++) {
        _SOC_ISM_SETS(unit)[i].num_banks = 0;
    }
    count = _soc_ism_get_sorted_bank_list(unit, mems, banks);    
    LOG_INFO(BSL_LS_SOC_SOCMEM,
             (BSL_META_U(unit,
                         "Used banks: %d\n"), count));
    for (i = 0; i < count; i++) {
        if (mems[i] >= SOC_ISM_MEM_MAX) {
            continue;
        }
        _SOC_ISM_BANKS(unit)[i].num_bkts = 
                                   SOC_ISM_INFO(unit)->bank_raw_sizes[banks[i]];
        _SOC_ISM_BANKS(unit)[i].bkt_size = 4; /* could be configurable */
        _SOC_ISM_BANKS(unit)[i].num_entries = _SOC_ISM_BANKS(unit)[i].num_bkts * 
                                              _SOC_ISM_BANKS(unit)[i].bkt_size;
        _SOC_ISM_BANKS(unit)[i].my_id = banks[i];
        for (j = 0; j < SOC_MEM_SET_MAX; j++) {
            if (mems[i] == _SOC_ISM_SETS(unit)[j].mem_set) {
               _SOC_ISM_BANKS(unit)[i].hms = &_SOC_ISM_SETS(unit)[j];
               if (_SOC_ISM_SETS(unit)[j].num_banks == 0) {
                   _SOC_ISM_SETS(unit)[j].shb = &_SOC_ISM_BANKS(unit)[i];
                   _SOC_ISM_BANKS(unit)[i].base_entry = 0;
               } else {
                   _SOC_ISM_BANKS(unit)[i].base_entry = 
                   _SOC_ISM_SETS(unit)[j].shb[_SOC_ISM_SETS(unit)[j].num_banks-1].base_entry 
                   + _SOC_ISM_SETS(unit)[j].shb[_SOC_ISM_SETS(unit)[j].num_banks-1].num_entries;
               }
               _SOC_ISM_SETS(unit)[j].num_banks++;               
               break;
            }
        }
        LOG_INFO(BSL_LS_SOC_SOCMEM,
                 (BSL_META_U(unit,
                             "Bank: %d, base: %d, mem: [%s], buckets: %d, entries: %d "
                             "hash offset: %d\n"), banks[i], 
                  _SOC_ISM_BANKS(unit)[i].base_entry, 
                  soc_ism_table_to_name(_SOC_ISM_BANKS(unit)[i].hms->mem_set),
                  _SOC_ISM_BANKS(unit)[i].num_bkts, 
                  _SOC_ISM_BANKS(unit)[i].num_entries,
                  _SOC_ISM_BANKS(unit)[i].hash_offset));
    }
    /* Just for sanity */
    for (i = 0; i < SOC_MEM_SET_MAX; i++) {
        LOG_INFO(BSL_LS_SOC_SOCMEM,
                 (BSL_META_U(unit,
                             "Set: %s, num mems: %d, banks: %d, zero_lsb: %d, \n"), 
                  soc_ism_table_to_name(_SOC_ISM_SETS(unit)[i].mem_set),
                  _SOC_ISM_SETS(unit)[i].num_mems, 
                  _SOC_ISM_SETS(unit)[i].num_banks,
                  _SOC_ISM_SETS(unit)[i].zero_lsb));
        for (j = 0; j < _SOC_ISM_SETS(unit)[i].num_mems; j++) {
            LOG_INFO(BSL_LS_SOC_SOCMEM,
                     (BSL_META_U(unit,
                                 "Num views: %d\n"), 
                      _SOC_ISM_SETS(unit)[i].shm[j].num_views));
            for (k = 0; k < _SOC_ISM_SETS(unit)[i].shm[j].num_views; k++) {
                LOG_INFO(BSL_LS_SOC_SOCMEM,
                         (BSL_META_U(unit,
                                     "key width: %d\n"), 
                          _SOC_ISM_SETS(unit)[i].shm[j].hmv[k].key_size));
            }
        }
    }
    return SOC_E_NONE;
}

/* write to h/w */
int
soc_ism_hw_config(int unit) 
{
    int i, r, f, stg, bank, len;
    int j, offset = 0, lbpd = 0, rv = SOC_E_NONE;
    uint32 pd_val, rval;
    _soc_ism_pd_t *pd;

    assert(SOC_ISM_INFO(unit));
    if (SAL_BOOT_SIMULATION) {
        uint32 rval = 0;
        if (SOC_ISM_INFO(unit)->ism_mode == _ISM_SIZE_MODE_176) {
            soc_reg_field_set(unit, STAGE0_BANK_SIZEr, &rval, BANK0_SIZE_LIMITf,
                              _ISM_BANK_SIZE_QUARTER);
            soc_reg_field_set(unit, STAGE0_BANK_SIZEr, &rval, BANK1_SIZE_LIMITf,
                              _ISM_BANK_SIZE_HALF);
            soc_reg_field_set(unit, STAGE0_BANK_SIZEr, &rval, BANK2_SIZE_LIMITf,
                              _ISM_BANK_SIZE_HALF);
            soc_reg_field_set(unit, STAGE0_BANK_SIZEr, &rval, BANK3_SIZE_LIMITf,
                              _ISM_BANK_SIZE_HALF);
            soc_reg_field_set(unit, STAGE0_BANK_SIZEr, &rval, BANK4_SIZE_LIMITf,
                              _ISM_BANK_SIZE_DISABLED);
            SOC_IF_ERROR_RETURN(WRITE_STAGE0_BANK_SIZEr(unit, rval));
            SOC_IF_ERROR_RETURN(WRITE_STAGE1_BANK_SIZEr(unit, rval));
            SOC_IF_ERROR_RETURN(WRITE_STAGE2_BANK_SIZEr(unit, rval));
            SOC_IF_ERROR_RETURN(WRITE_STAGE3_BANK_SIZEr(unit, rval));
        } else if (SOC_ISM_INFO(unit)->ism_mode == _ISM_SIZE_MODE_96) {
            soc_reg_field_set(unit, STAGE0_BANK_SIZEr, &rval, BANK0_SIZE_LIMITf,
                              _ISM_BANK_SIZE_DISABLED);
            soc_reg_field_set(unit, STAGE0_BANK_SIZEr, &rval, BANK1_SIZE_LIMITf,
                              _ISM_BANK_SIZE_QUARTER);
            soc_reg_field_set(unit, STAGE0_BANK_SIZEr, &rval, BANK2_SIZE_LIMITf,
                              _ISM_BANK_SIZE_HALF);
            soc_reg_field_set(unit, STAGE0_BANK_SIZEr, &rval, BANK3_SIZE_LIMITf,
                              _ISM_BANK_SIZE_HALF);
            soc_reg_field_set(unit, STAGE0_BANK_SIZEr, &rval, BANK4_SIZE_LIMITf,
                              _ISM_BANK_SIZE_HALF);
            SOC_IF_ERROR_RETURN(WRITE_STAGE0_BANK_SIZEr(unit, rval));
            SOC_IF_ERROR_RETURN(WRITE_STAGE1_BANK_SIZEr(unit, rval));
            SOC_IF_ERROR_RETURN(WRITE_STAGE2_BANK_SIZEr(unit, rval));
            SOC_IF_ERROR_RETURN(WRITE_STAGE3_BANK_SIZEr(unit, rval));
        } else if (SOC_ISM_INFO(unit)->ism_mode == _ISM_SIZE_MODE_80) {
            soc_reg_field_set(unit, STAGE0_BANK_SIZEr, &rval, BANK0_SIZE_LIMITf,
                              _ISM_BANK_SIZE_DISABLED);
            soc_reg_field_set(unit, STAGE0_BANK_SIZEr, &rval, BANK1_SIZE_LIMITf,
                              _ISM_BANK_SIZE_QUARTER);
            soc_reg_field_set(unit, STAGE0_BANK_SIZEr, &rval, BANK2_SIZE_LIMITf,
                              _ISM_BANK_SIZE_QUARTER);
            soc_reg_field_set(unit, STAGE0_BANK_SIZEr, &rval, BANK3_SIZE_LIMITf,
                              _ISM_BANK_SIZE_HALF);
            soc_reg_field_set(unit, STAGE0_BANK_SIZEr, &rval, BANK4_SIZE_LIMITf,
                              _ISM_BANK_SIZE_HALF);
            SOC_IF_ERROR_RETURN(WRITE_STAGE0_BANK_SIZEr(unit, rval));
            SOC_IF_ERROR_RETURN(WRITE_STAGE1_BANK_SIZEr(unit, rval));
            SOC_IF_ERROR_RETURN(WRITE_STAGE2_BANK_SIZEr(unit, rval));
            SOC_IF_ERROR_RETURN(WRITE_STAGE3_BANK_SIZEr(unit, rval));
        }
        /* Note: handle other modes as needed in the future */
    }
    rv = soc_ism_table_bank_set(unit);
    if (SOC_FAILURE(rv)) {
        return rv; 
    }
    rv = soc_ism_log_to_phy_set(unit);
    if (SOC_FAILURE(rv)) {
        return rv; 
    }
    if ((SOC_ISM_INFO(unit)->ism_mode == _ISM_SIZE_MODE_96) ||
        (SOC_ISM_INFO(unit)->ism_mode == _ISM_SIZE_MODE_80)) {
        offset = 1; /* First bank is not available */
    } else if (SOC_ISM_INFO(unit)->ism_mode == _ISM_SIZE_MODE_176) {
        lbpd = 1; /* Last bank is not available */
    }
    /* Power down unused banks.
     * Note: This works on the actual banks of the base device not the banks 
     *       based upon the ISM mode.
     */
    for (i = 0; i < _SOC_ISM_MAX_BANKS; i++) {
        if (!_soc_ism_bank_avail[unit][i]) {
            if (offset || lbpd) {
                j = i + offset + i/4;
                if (j >= _SOC_ISM_MAX_BANKS) {
                    return SOC_E_NONE;
                }
            } else {
                j = i;
            }
            stg = j / _SOC_ISM_BANKS_PER_STAGE;
            bank = j % _SOC_ISM_BANKS_PER_STAGE;
            LOG_VERBOSE(BSL_LS_SOC_COMMON,
                        (BSL_META_U(unit,
                                    "Powering down stage[%d] bank[%d]\n"),
                         stg, bank));
            pd = &_soc_ism_pd[stg][bank];
            r = 0;
            while (pd->reg_field[r].pdr != INVALIDr) {
                SOC_IF_ERROR_RETURN
                    (soc_reg32_get(unit, pd->reg_field[r].pdr, REG_PORT_ANY, 
                                   0, &rval));
                f = 0;
                while (pd->reg_field[r].pdf[f] != INVALIDf) {
                    len = soc_reg_field_length(unit, pd->reg_field[r].pdr, 
                                               pd->reg_field[r].pdf[f]);
                    pd_val = ((1 << len) - 1);
                    soc_reg_field_set(unit, pd->reg_field[r].pdr, &rval, 
                                      pd->reg_field[r].pdf[f], pd_val);
                    f++;
                }
                SOC_IF_ERROR_RETURN
                    (soc_reg32_set(unit, pd->reg_field[r].pdr, REG_PORT_ANY, 
                                   0, rval));
                r++;
            }
        } else if (_soc_ism_bank_avail[unit][i] >= SOC_ISM_MEM_ESM_L2) {
            if (offset || lbpd) {
                j = i + offset + i/4;
                if (j >= _SOC_ISM_MAX_BANKS) {
                    return SOC_E_NONE;
                }
            } else {
                j = i;
            }
            stg = j / _SOC_ISM_BANKS_PER_STAGE;
            bank = j % _SOC_ISM_BANKS_PER_STAGE;
            LOG_VERBOSE(BSL_LS_SOC_COMMON,
                        (BSL_META_U(unit,
                                    "Powering down hit-bit rams for stage[%d] bank[%d]\n"),
                         stg, bank));
            pd = &_soc_ism_pd[stg][bank];
            r = 0;
            while (pd->hit_reg_field[r].pdr != INVALIDr) {
                SOC_IF_ERROR_RETURN
                    (soc_reg32_get(unit, pd->hit_reg_field[r].pdr, REG_PORT_ANY, 
                                   0, &rval));
                f = 0;
                while (pd->hit_reg_field[r].pdf[f] != INVALIDf) {
                    len = soc_reg_field_length(unit, pd->hit_reg_field[r].pdr, 
                                               pd->hit_reg_field[r].pdf[f]);
                    pd_val = ((1 << len) - 1);
                    soc_reg_field_set(unit, pd->hit_reg_field[r].pdr, &rval, 
                                      pd->hit_reg_field[r].pdf[f], pd_val);
                    f++;
                }
                SOC_IF_ERROR_RETURN
                    (soc_reg32_set(unit, pd->hit_reg_field[r].pdr, REG_PORT_ANY, 
                                   0, rval));
                r++;
            }
        }
    }
    return SOC_E_NONE;
}

soc_reg_t _ism_stage_hash_cfg_reg[] = {
    STAGE0_HASH_OFFSETr, STAGE1_HASH_OFFSETr,
    STAGE2_HASH_OFFSETr, STAGE3_HASH_OFFSETr
};

soc_field_t _ism_stage_hash_cfg_fld[] = {
    BANK0_HASH_OFFSETf, BANK1_HASH_OFFSETf,
    BANK2_HASH_OFFSETf, BANK3_HASH_OFFSETf,
    BANK4_HASH_OFFSETf
};

/* Configure hash offset per bank */
int 
soc_ism_hash_offset_config(int unit, uint8 bank, uint8 offset)
{
    uint8 i, b, stage, set_idx, found = 0;
    uint32 rval;
    int disabled_banks = 0;

    if ((bank >= SOC_ISM_INFO(unit)->max_banks) || (offset > 63)) {
        return SOC_E_PARAM;
    }
    if ((!_soc_ism_bank_avail[unit][bank]) || 
        (_soc_ism_bank_avail[unit][bank] >= SOC_ISM_MEM_MAX)) {
        return SOC_E_PARAM;
    }
    stage = bank/SOC_ISM_INFO(unit)->banks_per_stage;
    disabled_banks = ((SOC_ISM_INFO(unit)->ism_mode == _ISM_SIZE_MODE_96) ||
              (SOC_ISM_INFO(unit)->ism_mode == _ISM_SIZE_MODE_80)) ?
              ((SOC_ISM_INFO(unit)->ism_mode == _ISM_SIZE_MODE_64) ? 2 : 1) : 0;
    b = (bank%SOC_ISM_INFO(unit)->banks_per_stage) + disabled_banks;
    
    SOC_IF_ERROR_RETURN
        (soc_reg32_get(unit, _ism_stage_hash_cfg_reg[stage], REG_PORT_ANY, 
                       0, &rval));
    soc_reg_field_set(unit, _ism_stage_hash_cfg_reg[stage], 
                      &rval, _ism_stage_hash_cfg_fld[b], offset);
    SOC_IF_ERROR_RETURN
        (soc_reg32_set(unit, _ism_stage_hash_cfg_reg[stage], REG_PORT_ANY, 
                       0, rval));
    set_idx = _soc_ism_bank_avail[unit][bank] - 1;
    for (i = 0; i < _SOC_ISM_SETS(unit)[set_idx].num_banks; i++) {
        if (_SOC_ISM_SETS(unit)[set_idx].shb[i].my_id == bank) {
            _SOC_ISM_SETS(unit)[set_idx].shb[i].hash_offset = offset;
            found = 1;
            break;
        }
    }
    if (!found) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "No memory mapped to bank: %d\n"), bank));
        return SOC_E_INTERNAL;
    }
    return SOC_E_NONE;
}

/* Get max allowed offset value that can be used for any bank */
int
soc_ism_hash_max_offset_get(int unit, int *offset)
{
    if (offset) {
        *offset = 64;
    }
    return SOC_E_NONE;
}

/* Configure hash offset per mem */
int 
soc_ism_hash_mem_offset_config(int unit, soc_mem_t mem, uint8 count, 
                               uint8 *offset)
{
    return SOC_E_NONE;
}

/* Get hash offset per bank */
int 
soc_ism_hash_offset_config_get(int unit, uint8 bank, uint8 *offset)
{
    uint8 i, set_idx, found = 0;

    if (bank >= SOC_ISM_INFO(unit)->max_banks) {
        return SOC_E_PARAM;
    }    
    set_idx = _soc_ism_bank_avail[unit][bank] - 1;
    
#ifdef BCM_WARM_BOOT_SUPPORT
    if (SOC_WARM_BOOT(unit)) {
        uint8 b, stage;
        uint32 rval, val;
        int disabled_banks = 0;
    
        if (bank >= SOC_ISM_INFO(unit)->max_banks) {
            return SOC_E_PARAM;
        }
        if ((!_soc_ism_bank_avail[unit][bank]) || 
            (_soc_ism_bank_avail[unit][bank] >= SOC_ISM_MEM_MAX)) {
            return SOC_E_PARAM;
        }
        stage = bank/SOC_ISM_INFO(unit)->banks_per_stage;
        disabled_banks = ((SOC_ISM_INFO(unit)->ism_mode == _ISM_SIZE_MODE_96) ||
                   (SOC_ISM_INFO(unit)->ism_mode == _ISM_SIZE_MODE_80)) ?
                   ((SOC_ISM_INFO(unit)->ism_mode == 
                                            _ISM_SIZE_MODE_64) ? 2 : 1) : 0;
        b = (bank%SOC_ISM_INFO(unit)->banks_per_stage) + disabled_banks;
        
        SOC_IF_ERROR_RETURN
            (soc_reg32_get(unit, _ism_stage_hash_cfg_reg[stage], REG_PORT_ANY, 
                           0, &rval));
        val = soc_reg_field_get(unit, _ism_stage_hash_cfg_reg[stage], 
                                rval, _ism_stage_hash_cfg_fld[b]);
        for (i = 0; i < _SOC_ISM_SETS(unit)[set_idx].num_banks; i++) {
            if (_SOC_ISM_SETS(unit)[set_idx].shb[i].my_id == bank) {
                *offset = val;
                _SOC_ISM_SETS(unit)[set_idx].shb[i].hash_offset = val;
                found = 1;
                break;
            }
        }
    } else 
#endif /* BCM_WARM_BOOT_SUPPORT */
    {    
        for (i = 0; i < _SOC_ISM_SETS(unit)[set_idx].num_banks; i++) {
            if (_SOC_ISM_SETS(unit)[set_idx].shb[i].my_id == bank) {
                *offset = _SOC_ISM_SETS(unit)[set_idx].shb[i].hash_offset;
                found = 1;
                break;
            }
        }
    }
    if (!found) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "No memory mapped to bank: %d\n"), bank));
        return SOC_E_INTERNAL;
    }
    return SOC_E_NONE;
}
 
/* Get hash offset per table */
int 
soc_ism_hash_table_offset_config_get(int unit, soc_ism_mem_type_t table, 
                                     uint8 *offset, uint8 *count)
{
    int i, rv;
    uint8 banks[_SOC_ISM_MAX_BANKS];
    uint32 bank_size[_SOC_ISM_MAX_BANKS];
    
    rv = soc_ism_get_banks(unit, table, banks, bank_size, count);
    for (i = 0; i < *count; i++) {
        rv |= soc_ism_hash_offset_config_get(unit, banks[i], &offset[i]);
    }
    return SOC_E_NONE;
}

/* Get hash offset per mem */
int 
soc_ism_hash_mem_offset_config_get(int unit, soc_mem_t mem, uint8 *offset, 
                                   uint8 *count)
{
    int8 memidx;
    
    if ((memidx = soc_ism_get_hash_mem_idx(unit, mem)) < 0) {
        return SOC_E_PARAM;
    }
    return soc_ism_hash_table_offset_config_get(unit, _SOC_ISM_MEMS(unit)[memidx].shms->mem_set,
                                                offset, count);
}

/* Configure hash type per table */
int 
soc_ism_table_hash_config(int unit, soc_ism_mem_type_t table, uint8 zero_lsb)
{
    uint32 rval;
    if (!table || table >= SOC_ISM_MEM_MAX) {
        return SOC_E_PARAM;
    }
    SOC_IF_ERROR_RETURN
        (soc_reg32_get(unit, _ism_table_bank_cfg_reg[table-1], REG_PORT_ANY, 
                       0, &rval));
    soc_reg_field_set(unit, _ism_table_bank_cfg_reg[table-1], 
                      &rval, HASH_ZERO_OR_LSBf, zero_lsb);
    SOC_IF_ERROR_RETURN
        (soc_reg32_set(unit, _ism_table_bank_cfg_reg[table-1], REG_PORT_ANY, 
                       0, rval));
    _SOC_ISM_SETS(unit)[table-1].zero_lsb = zero_lsb;
    return SOC_E_NONE;
}

/* Configure hash type per mem */
int 
soc_ism_mem_hash_config(int unit, soc_mem_t mem, uint8 zero_lsb)
{
    int8 memidx;
    if ((memidx = soc_ism_get_hash_mem_idx(unit, mem)) < 0) {
        return SOC_E_PARAM;
    }
    return soc_ism_table_hash_config(unit, _SOC_ISM_MEMS(unit)[memidx].shms->mem_set, 
                                     zero_lsb);
}

/* Get hash type per table */
int 
soc_ism_table_hash_config_get(int unit, soc_ism_mem_type_t table, uint8 *zero_lsb)
{
    if (!table || table >= SOC_ISM_MEM_MAX) {
        return SOC_E_PARAM;
    }
#ifdef BCM_WARM_BOOT_SUPPORT
    if (SOC_WARM_BOOT(unit)) {
        uint32 rval, val;
        SOC_IF_ERROR_RETURN
        (soc_reg32_get(unit, _ism_table_bank_cfg_reg[table-1], REG_PORT_ANY, 
                       0, &rval));
        val = soc_reg_field_get(unit, _ism_table_bank_cfg_reg[table-1], 
                                rval, HASH_ZERO_OR_LSBf);
        *zero_lsb = val;
        _SOC_ISM_SETS(unit)[table-1].zero_lsb = val;
    } else 
#endif /* BCM_WARM_BOOT_SUPPORT */
    {
        *zero_lsb = _SOC_ISM_SETS(unit)[table-1].zero_lsb;
    }
    return SOC_E_NONE;
}

/* Get hash type per mem */
int 
soc_ism_mem_hash_config_get(int unit, soc_mem_t mem, uint8 *zero_lsb)
{
    int8 memidx;
    if ((memidx = soc_ism_get_hash_mem_idx(unit, mem)) < 0) {
        return SOC_E_PARAM;
    }
    return soc_ism_table_hash_config_get(unit, _SOC_ISM_MEMS(unit)[memidx].shms->mem_set, 
                                         zero_lsb);
}

/* Get key length per mem */
int 
soc_ism_mem_max_key_bits_get(int unit, soc_mem_t mem)
{
    int8 memidx;
    if ((memidx = soc_ism_get_hash_mem_idx(unit, mem)) < 0) {
        return SOC_E_PARAM;
    }
    return _SOC_ISM_MEMS(unit)[memidx].shms->max_key_bits; 
}

int
soc_ism_get_total_banks(int unit, int *count)
{
    if (count) {
        *count = SOC_ISM_INFO(unit)->max_banks;
    }
    return SOC_E_NONE;
}

/* Get configured banks and count per table */
int 
soc_ism_get_banks(int unit, soc_ism_mem_type_t table, uint8 *banks, 
                  uint32 *bank_size, uint8 *count)
{
    uint8 i;
    if (count) {
        *count = 0;
    } else {
        return SOC_E_PARAM;
    }
    for (i = 0; i < SOC_ISM_INFO(unit)->max_banks; i++) {
        if (_soc_ism_bank_avail[unit][i] == table) {
            if (banks) {
                banks[*count] = i;
            }
            if (bank_size) {
                bank_size[*count] = SOC_ISM_INFO(unit)->bank_raw_sizes[i];
            }
            (*count)++;
        }
    }
    return SOC_E_NONE;
}

/* Get configured banks and count per mem */
int
soc_ism_get_banks_for_mem(int unit, soc_mem_t mem, uint8 *banks, 
                          uint32 *bank_size, uint8 *count)
{
    int8 memidx;
    if ((memidx = soc_ism_get_hash_mem_idx(unit, mem)) < 0) {
        return SOC_E_PARAM;
    }
    if (_SOC_ISM_MEMS(unit)[memidx].shms->num_banks) {
        return soc_ism_get_banks(unit, _SOC_ISM_MEMS(unit)[memidx].shms->mem_set, 
                                 banks, bank_size, count);
    } else {
        *count = 0;
        return SOC_E_NONE;
    }
}

uint32
soc_ism_get_phy_bank_mask(int unit, uint32 bank_mask)
{
    uint32 offset = 1, bank = 0;
    if (-1 == bank_mask || 0 == bank_mask) {
        return bank_mask;
    }
    
    if ((SOC_ISM_INFO(unit)->ism_mode == _ISM_SIZE_MODE_96) ||
        (SOC_ISM_INFO(unit)->ism_mode == _ISM_SIZE_MODE_80) ||
        (SOC_ISM_INFO(unit)->ism_mode == _ISM_SIZE_MODE_176)) {
        int i;
        
        for (i=0; i<32; i++) {
            if (bank_mask & (1<<i)) {
                bank = i;
                break;
            }
        }
        if ((SOC_ISM_INFO(unit)->ism_mode == _ISM_SIZE_MODE_96) ||
            (SOC_ISM_INFO(unit)->ism_mode == _ISM_SIZE_MODE_80)) {
            /* First bank is not available */
        } else if (SOC_ISM_INFO(unit)->ism_mode == _ISM_SIZE_MODE_176) {
            /* Last bank is not available */
            offset = 0;
        }
        return 1 << (bank + offset + bank/4);
    } else {
        return bank_mask;
    }
}

/* Get key and lsb fields and count from a mem entry */
int 
soc_generic_get_hash_key(int unit, soc_mem_t mem, void *entry, 
                         soc_field_t *keyf, soc_field_t *lsbf, uint8 *num_flds)
{
    int i, j, f = 0, found;
    int key_type; 
    if (SOC_MEM_FIELD_VALID(unit, mem, KEY_TYPEf)) {
        key_type = soc_mem_field32_get(unit, mem, entry, KEY_TYPEf);
    } else {
        key_type = soc_mem_field32_get(unit, mem, entry, KEY_TYPE_0f);
    }
    i = soc_ism_get_hash_mem_idx(unit, mem);
    if (i < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Invalid hash memory !!\n")));
        return SOC_E_PARAM;
    }
    found = 0;
    for (j = 0; j < _SOC_ISM_MEMS(unit)[i].shms->num_keys; j++) {
        if (key_type == _SOC_ISM_MEMS(unit)[i].shms->shk[j].key_type) {
            LOG_VERBOSE(BSL_LS_SOC_SOCMEM,
                        (BSL_META_U(unit,
                                    "Retreived key_type: %d for mem: %s\n"), 
                         key_type, SOC_MEM_NAME(unit, mem)));
            found = 1;
            break;
        }  
    }
    if (!found) {
        return SOC_E_INTERNAL;
    }
    while (_SOC_ISM_MEMS(unit)[i].shms->shk[j].hmv->key_fields[f] != -1) { 
        keyf[f] = _SOC_ISM_MEMS(unit)[i].shms->shk[j].hmv->key_fields[f];
        f++;
        *num_flds = f; 
    }
    *lsbf = _SOC_ISM_MEMS(unit)[i].shms->shk[j].hmv->lsb_field;
    return SOC_E_NONE;
}

/* helper routine */
STATIC uint32
_soc_crc32b(uint8 *data, int data_nbits)
{
    uint32 rv;
    rv = _shr_crc32b(0, data, data_nbits);
    rv = _shr_bit_rev_by_byte_word32(rv);
    return rv;
}

/* helper routine */
STATIC uint16
_soc_crc16b(uint8 *data, int data_nbits)
{
    uint16 rv;
    rv = _shr_crc16b(0, data, data_nbits);
    rv = _shr_bit_rev16(rv);
    return rv;
}

/* Generate hash value from key using lsb or crc based upon config and offset */
int
soc_generic_gen_hash(int unit, uint32 zero_lsb, uint32 num_bits, 
                     uint32 offset, uint32 mask, uint8 *key, uint16 lsb)
{
    uint64 val, tmp;
    uint32 crc_lo;
    uint16 crc_hi;
    int32 i, j = 0;
    LOG_VERBOSE(BSL_LS_SOC_SOCMEM,
                (BSL_META_U(unit,
                            "Num bits: %d, zero_lsb: %d, lsb: %x, offset: %d, "
                            "mask: %x\n"), num_bits, zero_lsb, lsb, offset, mask));
    /* mask bit 0 of key */
    key[0] &= 0xfe;
    LOG_VERBOSE(BSL_LS_SOC_SOCMEM,
                (BSL_META_U(unit,
                            "Key: [")));
    for (i = num_bits; i > 0; i-=8) {
        LOG_VERBOSE(BSL_LS_SOC_SOCMEM,
                    (BSL_META_U(unit,
                                "%0x"), key[j++]));
    }
    LOG_VERBOSE(BSL_LS_SOC_SOCMEM,
                (BSL_META_U(unit,
                            "]\n")));
    if (offset >= 48) {
        if (!zero_lsb) {
            LOG_VERBOSE(BSL_LS_SOC_SOCMEM,
                        (BSL_META_U(unit,
                                    "Hash(zero)\n")));
            return 0;
        } else {
            if (offset > 48) {
                lsb = lsb >> (offset-48);
            }
            lsb &= mask;
            LOG_VERBOSE(BSL_LS_SOC_SOCMEM,
                        (BSL_META_U(unit,
                                    "Hash(lsb): %d\n"), lsb));
            return lsb & mask;
        }
    } else {
        crc_lo = _soc_crc32b(key, num_bits);
        crc_hi = _soc_crc16b(key, num_bits);
        LOG_VERBOSE(BSL_LS_SOC_SOCMEM,
                    (BSL_META_U(unit,
                                "crc32: %x\n"), crc_lo));
        LOG_VERBOSE(BSL_LS_SOC_SOCMEM,
                    (BSL_META_U(unit,
                                "crc16: %x\n"), crc_hi));
        COMPILER_64_SET(val, crc_hi, crc_lo);
        if (offset) {
            COMPILER_64_SHR(val, offset);
        }
        COMPILER_64_SET(tmp, 0, mask);
        COMPILER_64_AND(val, tmp);
        COMPILER_64_TO_32_LO(crc_lo, val);
        LOG_VERBOSE(BSL_LS_SOC_SOCMEM,
                    (BSL_META_U(unit,
                                "Hash(crc): %d\n"), crc_lo));
        return crc_lo & mask;
    }
    return SOC_E_NONE;
}

/* return mask based upon bank size */
int
soc_ism_get_hash_bucket_mask(int buckets)
{
    switch (buckets) {
    case 1024: return 0x3ff;
    case 2048: return 0x7ff;
    case 4096: return 0xfff;
    case 8192: return 0x1fff;
    case 16384: return 0x3fff;
    default: return SOC_E_PARAM;
    }
}

/* return num of bits based upon bank size */
int 
soc_ism_get_hash_bits(int buckets)
{
    switch (buckets) {
    case 1024: return 10;
    case 2048: return 11;
    case 4096: return 12;
    case 8192: return 13;
    case 16384: return 14;
    default: return SOC_E_PARAM;
    }
}

/* helper routine */
STATIC void
_soc_hash_check_and_swap(_soc_ism_sbo_t *lx, _soc_ism_sbo_t *ly, int x, int y)
{
    _soc_ism_sbo_t t;
    if (x > y) {
        t = *lx;
        *lx = *ly;
        *ly = t;
    }
}

/* helper routine */
STATIC void
_soc_sort_bank_on_criteria(_soc_ism_sbo_t *list, uint8 size, uint8 criteria)
{
    uint8 x, y;
    for (x=0; x < size-1; x++) {
        for (y=0; y < size-x-1; y++) {
            switch (criteria) {
            case _CRITERIA_ENTRY: _soc_hash_check_and_swap(&list[y], &list[y+1], 
                                                           list[y].entry, 
                                                           list[y+1].entry);
                break;
            case _CRITERIA_STAGE: _soc_hash_check_and_swap(&list[y], &list[y+1], 
                                                           list[y].stage, 
                                                           list[y+1].stage);
                break;
            case _CRITERIA_BANK: _soc_hash_check_and_swap(&list[y], &list[y+1], 
                                                          list[y].bank, 
                                                          list[y+1].bank);
                break;
            }
        }
    }
}

/* determine the best entry index from multiple banks using various criterias */
void
soc_ism_resolve_entry_index(_soc_ism_sbo_t *sbo, uint8 num_banks)
{
    uint8 b, tie=0;
    _soc_sort_bank_on_criteria(sbo, num_banks, _CRITERIA_ENTRY);
    /* determine if there is a tie and sort further */
    for (b = 0; b < num_banks - 1; b++) {
        if (sbo[b].entry == sbo[b+1].entry) {
            tie++;
        } else {
            break;
        }
    }
    if (!tie) {
        return;
    }
    _soc_sort_bank_on_criteria(sbo, tie, _CRITERIA_BANK);
    num_banks = tie;
    tie=0;
    /* determine if there is a tie and sort further */
    for (b = 0; b < num_banks - 1; b++) {
        if (sbo[b].bank == sbo[b+1].bank) {
            tie++;
        } else {
            break;
        }
    }
    if (!tie) {
        return;
    }
    num_banks = tie;
    tie=0;
    /* determine if there is a tie and sort further */
    _soc_sort_bank_on_criteria(sbo, num_banks, _CRITERIA_STAGE);
}

/* helper function: It has specific memory names and logic but that is only to 
   make it work as fast as possible */
uint8
soc_ism_get_bucket_offset(int unit, soc_mem_t mem, int8 midx, void *new_entry, 
                          void *existing_entry)
{
    uint8 i, incr = 1, kts;
    uint32 new_kt, existing_kt;
    soc_hash_mem_set_t *shms;
    soc_hash_mem_t *shm;
    
    if (midx < 0) {
        midx = soc_ism_get_hash_mem_idx(unit, mem);
    }
    shms = _SOC_ISM_MEMS(unit)[midx].shms;
    if (shms->num_mems == 1) {
        return incr;
    }
    if ((mem == L2_ENTRY_1m) || (mem == L2_ENTRY_2m)) {
        if (soc_mem_field32_get(unit, L2_ENTRY_1m, existing_entry, WIDEf)) {
            return 2;
        } else {
            if (soc_mem_field32_get(unit, L2_ENTRY_1m, new_entry, WIDEf)) {
                return 2;
            }
            return incr;
        }
    }
    if (SOC_MEM_FIELD_VALID(unit, mem, KEY_TYPEf)) {
        new_kt = soc_mem_field32_get(unit, mem, new_entry, KEY_TYPEf);
    } else {
        new_kt = soc_mem_field32_get(unit, mem, new_entry, KEY_TYPE_0f);
    }
    if (SOC_MEM_FIELD_VALID(unit, mem, KEY_TYPEf)) {
        existing_kt = soc_mem_field32_get(unit, mem, existing_entry, KEY_TYPEf);
    } else {
        existing_kt = soc_mem_field32_get(unit, mem, existing_entry, KEY_TYPE_0f);
    }
    kts = shms->num_keys;
    for (i=0; i<kts; i++) {
        if (shms->shk[i].key_type == existing_kt) {
            shm = shms->shk[i].hmv->shm;
            if (shm->mem == VLAN_XLATEm || shm->mem == L3_ENTRY_1m || 
                shm->mem == MPLS_ENTRYm) {
                break;
            } else if (shm->mem == VLAN_XLATE_EXTDm || shm->mem == L3_ENTRY_2m || 
                       shm->mem == MPLS_ENTRY_EXTDm) {
                incr = 2;
                break;
            } else {
                return 4;
            }
        }
    }
    /* Need to check new entry's size and return index increment appropriately */
    for (i=0; i<kts; i++) {
        if (shms->shk[i].key_type == new_kt) {
            shm = shms->shk[i].hmv->shm;
            if (shm->mem == VLAN_XLATEm || shm->mem == L3_ENTRY_1m || 
                shm->mem == MPLS_ENTRYm) {
                return (incr > 1) ? incr : 1;
            } else if (shm->mem == VLAN_XLATE_EXTDm || shm->mem == L3_ENTRY_2m || 
                       shm->mem == MPLS_ENTRY_EXTDm) {
                return 2;
            } else {
                return 4;
            }
        }
    }
    return incr;
}

/* helper function */
STATIC void
_soc_append_mem_field_to_data(soc_mem_info_t *meminfo, uint8 *key, uint16 offset, 
                              uint32 *fldbuf, uint16 size, uint8 lendian)
{
    int32 len;
    uint32 mask, i, wp, bp;
    uint32 *entbuf = (uint32 *)key;

    LOG_VERBOSE(BSL_LS_SOC_SOCMEM,
                (BSL_META("offset: %d, size: %d\n"), offset, size));
    if (lendian) {
        wp = offset / 32;
        bp = offset & (32 - 1);
        i = 0;
        for (len = size; len > 0; len -= 32) {
            if (bp) {
                if (len < 32) {
                    mask = (1 << len) - 1;
                } else {
                    mask = -1;
                }

                entbuf[FIX_MEM_ORDER_E(wp, meminfo)] &= ~(mask << bp);
                entbuf[FIX_MEM_ORDER_E(wp++, meminfo)] |= fldbuf[i] << bp;
                if (len > (32 - bp)) {
                    entbuf[FIX_MEM_ORDER_E(wp, meminfo)] &= ~(mask >> (32 - bp));
                    entbuf[FIX_MEM_ORDER_E(wp, meminfo)] |=
                        fldbuf[i] >> (32 - bp) & ((1 << bp) - 1);
                }
            } else {
                if (len < 32) {
                    mask = (1 << len) - 1;
                    entbuf[FIX_MEM_ORDER_E(wp, meminfo)] &= ~mask;
                    entbuf[FIX_MEM_ORDER_E(wp++, meminfo)] |= fldbuf[i] << bp;
                } else {
                    entbuf[FIX_MEM_ORDER_E(wp++, meminfo)] = fldbuf[i];
                }
            }
            i++;
        }
    } else {
        bp = offset;
        len = size;
        while (len > 0) {
            len--;
            entbuf[FIX_MEM_ORDER_E(bp / 32, meminfo)] &= ~(1 << (bp & (32-1)));
            entbuf[FIX_MEM_ORDER_E(bp / 32, meminfo)] |=
                (fldbuf[len / 32] >> (len & (32-1)) & 1) << (bp & (32-1));
            bp++;
        }
    }
}

/* Create final key by concatenating various key fields from a mem entry */
void
soc_ism_gen_key_from_keyfields(int unit, soc_mem_t mem, void *entry, 
                               soc_field_t *keyflds, uint8 *key, uint8 num_flds)
{
    int16 i;
    uint8 j = 0;
    uint32 val[SOC_MAX_MEM_WORDS];
    uint16 offset = 0, len;
    soc_mem_info_t *meminfo;
    soc_field_info_t *fieldinfo;
    meminfo = &SOC_MEM_INFO(unit, mem);

    for (i = 0; i < num_flds; i++) {
        SOC_FIND_FIELD(keyflds[i], meminfo->fields, meminfo->nFields,
                       fieldinfo);
        if (NULL == fieldinfo) {
            LOG_CLI((BSL_META_U(unit,
                                "mem %s field %s is invalid\n"),
                     SOC_MEM_NAME(unit, mem), SOC_FIELD_NAME(unit, keyflds[i])));
            assert(fieldinfo);
        }
        soc_mem_field_get(unit, mem, entry, keyflds[i], val);
        len = soc_mem_field_length(unit, mem, keyflds[i]);
        _soc_append_mem_field_to_data(meminfo, key, offset, val, len,
                                      fieldinfo->flags & SOCF_LE);
        offset += len;
    }
    LOG_VERBOSE(BSL_LS_SOC_SOCMEM,
                (BSL_META_U(unit,
                            "Combined Key: ")));
    for (i = offset; i > 0; i-=8) {
        LOG_VERBOSE(BSL_LS_SOC_SOCMEM,
                    (BSL_META_U(unit,
                                "%0x "), key[j++]));
    }
    LOG_VERBOSE(BSL_LS_SOC_SOCMEM,
                (BSL_META_U(unit,
                            "\n")));
}

/* Create key to be used in crc calculation by concatenating various key fields 
   from a mem entry and taking care of padding, alignment etc */
void
soc_ism_gen_crc_key_from_keyfields(int unit, soc_mem_t mem, void *entry, 
                                   soc_field_t *keyflds, uint8 *key, 
                                   uint8 num_flds, uint16 *bit_count)
{
    uint8 i;
    soc_field_t field;
    uint16 key_index, val_index, fval_index;
    uint16 right_shift_count, left_shift_count;
    uint32 val[SOC_MAX_MEM_WORDS], fval[SOC_MAX_MEM_WORDS];
    int16 val_bits = *bit_count, fval_bits;
    uint16 bits, field_length[16];
    
    for (i = 0; i < num_flds; i++) {
        field = keyflds[i];
        field_length[i] = soc_mem_field_length(unit, mem, field);
    }
    
    bits = (val_bits + 7) & ~0x7;
    sal_memset(val, 0, sizeof(val));
    val_bits = bits - val_bits;
    for (i = 0; i < num_flds; i++) {
        field = keyflds[i];
        soc_mem_field_get(unit, mem, entry, field, fval);
        
        /* mask out 0th bit of key type */
        fval[0] &= ~1;
        
        fval_bits = field_length[i];

        val_index = val_bits >> 5;
        fval_index = 0;
        left_shift_count = val_bits & 0x1f;
        right_shift_count = 32 - left_shift_count;
        val_bits += fval_bits;

        if (left_shift_count) {
            for (; fval_bits > 0; fval_bits -= 32) {
                val[val_index++] |= fval[fval_index] << left_shift_count;
                val[val_index] |= fval[fval_index++] >> right_shift_count;
            }
        } else {
            for (; fval_bits > 0; fval_bits -= 32) {
                val[val_index++] = fval[fval_index++];
            }
        }
    }

    key_index = 0;
    for (val_index = 0; val_bits > 0; val_index++) {
        for (right_shift_count = 0; right_shift_count < 32;
             right_shift_count += 8) {
            if (val_bits <= 0) {
                break;
            }
            key[key_index++] = (val[val_index] >> right_shift_count) & 0xff;
            val_bits -= 8;
        }
    }

    if ((bits + 7) / 8 > key_index) {
        sal_memset(&key[key_index], 0, (bits + 7) / 8 - key_index);
    }
    *bit_count = bits;
}

/* Create a valid memory entry from a compacted key */
int
soc_gen_entry_from_key(int unit, soc_mem_t mem, uint8 *key, void *entry)
{
    int8 i, j, f = 0, found = 0;
    uint16 len, num_flds = 0;
    uint32 keyf[4] = {0};
    uint16 minb = 0, maxb;
    uint32 fvalue[SOC_MAX_MEM_WORDS] = {0};
    int key_type; 
    
    if (SOC_MEM_FIELD_VALID(unit, mem, KEY_TYPEf)) {
        len = soc_mem_field_length(unit, mem, KEY_TYPEf);
        soc_bits_get((uint32*)&key[0], 0, len-1, fvalue);
        key_type = fvalue[0];
    } else {
        len = soc_mem_field_length(unit, mem, KEY_TYPE_0f);
        soc_bits_get((uint32*)&key[0], 0, len-1, fvalue);
        key_type = fvalue[0];
    }
    i = soc_ism_get_hash_mem_idx(unit, mem);
    if (i < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Invalid hash memory !!\n")));
        return SOC_E_PARAM;
    }
    for (j = 0; j < _SOC_ISM_MEMS(unit)[i].shms->num_keys; j++) {
        if (_SOC_ISM_MEMS(unit)[i].shms->shk[j].hmv->shm->mem == mem) {
            if (key_type == _SOC_ISM_MEMS(unit)[i].shms->shk[j].key_type) {
                found = 1;
                LOG_VERBOSE(BSL_LS_SOC_SOCMEM,
                            (BSL_META_U(unit,
                                        "Input key_type: %d found for mem: %s\n"), 
                             key_type, SOC_MEM_NAME(unit, mem)));
                break;
            }
        }  
    }
    if (!found) {
        LOG_CLI((BSL_META_U(unit,
                            "Key type not found for this memory !!\n")));
        return SOC_E_INTERNAL;
    }
    while (_SOC_ISM_MEMS(unit)[i].shms->shk[j].hmv->key_fields[f] != -1) { 
        keyf[f] = _SOC_ISM_MEMS(unit)[i].shms->shk[j].hmv->key_fields[f];
        f++;
        num_flds = f; 
    }
    for (i=0; i<num_flds; i++) {
        len = soc_mem_field_length(unit, mem, keyf[i]);
        maxb = minb+len-1;
        /*LOG_CLI((BSL_META_U(unit,
                              "Min: %d, Max: %d\n"), minb, maxb));*/
        soc_bits_get((uint32*)&key[0], minb, maxb, fvalue);
        soc_mem_field_set(unit, mem, entry, keyf[i], fvalue);
        minb = maxb+1;
    }
    /* Make the entry valid and properly sized */
    if (SOC_MEM_FIELD_VALID(unit, mem, VALIDf)) {
        soc_mem_field32_set(unit, mem, entry, VALIDf, 1);
    } else {
        soc_mem_field32_set(unit, mem, entry, VALID_0f, 1);
        soc_mem_field32_set(unit, mem, entry, VALID_1f, 1);
        if (SOC_MEM_FIELD_VALID(unit, mem, VALID_2f)) {
            soc_mem_field32_set(unit, mem, entry, VALID_2f, 1);
            soc_mem_field32_set(unit, mem, entry, VALID_3f, 1);
        }
    }
    if (SOC_MEM_FIELD_VALID(unit, mem, WIDE_0f)) {
        soc_mem_field32_set(unit, mem, entry, WIDE_0f, 1);
        soc_mem_field32_set(unit, mem, entry, WIDE_1f, 1);
    }
    return SOC_E_NONE;
}

/* Generate entry with key fields set, used for direct comparision */
int
soc_gen_key_from_entry(int unit, soc_mem_t mem, void *entry, void *key)
{
    uint8 i, num_flds;
    uint32 val[SOC_MAX_MEM_WORDS];
    soc_field_t keyflds[MAX_FIELDS], lsbfld;
    soc_mem_info_t *meminfo;
    soc_field_info_t *fieldinfo;    
    int32 memidx = soc_ism_get_hash_mem_idx(unit, mem);
    
    if (memidx < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Invalid hash memory: %s !!\n"), 
                   SOC_MEM_NAME(unit, mem)));
        return SOC_E_PARAM;
    }
    meminfo = &SOC_MEM_INFO(unit, mem);
    
    if (soc_generic_get_hash_key(unit, mem, entry, keyflds, &lsbfld, 
                                 &num_flds) == SOC_E_NONE) {
        for (i = 0; i < num_flds; i++) {
            SOC_FIND_FIELD(keyflds[i], meminfo->fields, meminfo->nFields,
                           fieldinfo);
            if (NULL == fieldinfo) {
                LOG_CLI((BSL_META_U(unit,
                                    "Unit %d: mem %s field %s is invalid\n"), unit,
                         SOC_MEM_NAME(unit, mem), SOC_FIELD_NAME(unit, keyflds[i])));
                assert(fieldinfo);
            }
            soc_mem_field_get(unit, mem, entry, keyflds[i], val);
            soc_mem_field_set(unit, mem, key, keyflds[i], val);
            /* Make the entry valid and properly sized */
            if (SOC_MEM_FIELD_VALID(unit, mem, VALIDf)) {
                soc_mem_field32_set(unit, mem, key, VALIDf, 1);
            } else {
                soc_mem_field32_set(unit, mem, key, VALID_0f, 1);
                soc_mem_field32_set(unit, mem, key, VALID_1f, 1);
                if (SOC_MEM_FIELD_VALID(unit, mem, VALID_2f)) {
                    soc_mem_field32_set(unit, mem, key, VALID_2f, 1);
                    soc_mem_field32_set(unit, mem, key, VALID_3f, 1);
                }
            }
            if (SOC_MEM_FIELD_VALID(unit, mem, WIDE_0f)) {
                soc_mem_field32_set(unit, mem, key, WIDE_0f, 1);
                soc_mem_field32_set(unit, mem, key, WIDE_1f, 1);
            }
        }
        return SOC_E_NONE;
    }
    return SOC_E_INTERNAL;
}

/* 
 * Main hash execution routine. 
 * NOTE: Heavily overloaded, modify with care.
 * Returns index, result and optionally the last accessed bucket,
 * number of entries in a bucket for the memory.
 */
int
soc_generic_hash(int unit, soc_mem_t mem, void *entry, int32 banks, 
                 uint8 op, int *index, uint32 *result, uint32 *base_idx, 
                 uint8 *num_entries)
{
    int rv;
    soc_hash_bank_t *shbank;
    uint16 lsb, midx, num_bits;
    int32 bits, memidx, bucket = 0;
    uint32 tmp_hs[SOC_MAX_MEM_WORDS];
    uint32 idx, bidx = 0, zero_lsb, offset, mask;
    uint8 idxinc = 1, num_flds, num_banks = 0;
    uint8 found = 0, key[32], tmp_key[32], crc_key[32];
    _soc_ism_sbo_t sbo[_SOC_ISM_MAX_BANKS]; /* Dynamically created bank info based upon
                                               requested banks used for final
                                               bucket/index calculation */
    soc_field_t keyflds[MAX_FIELDS], lsbfld, vf = VALIDf;

    memidx = soc_ism_get_hash_mem_idx(unit, mem);
    if (memidx < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Invalid hash memory !!\n")));
        return SOC_E_PARAM;
    }
    if (!_SOC_ISM_MEMS(unit)[memidx].shms->num_banks) {
        return SOC_E_PARAM;
    }
    if (soc_generic_get_hash_key(unit, mem, entry, keyflds, &lsbfld, 
                                 &num_flds) == SOC_E_NONE) {
        LOG_VERBOSE(BSL_LS_SOC_SOCMEM,
                    (BSL_META_U(unit,
                                "Key field(s): ")));
        for (idx = 0; idx < num_flds; idx++) {
            LOG_VERBOSE(BSL_LS_SOC_SOCMEM,
                        (BSL_META_U(unit,
                                    "%d, "), keyflds[idx]));
        }
        LOG_VERBOSE(BSL_LS_SOC_SOCMEM,
                    (BSL_META_U(unit,
                                "\nLsb field: %d\n"), lsbfld));
        sal_memset(key, 0, sizeof(key));
        sal_memset(crc_key, 0, sizeof(crc_key));
        soc_ism_gen_key_from_keyfields(unit, mem, entry, keyflds, key, num_flds);
        num_bits = _SOC_ISM_MEMS(unit)[memidx].shms->max_key_bits;
        soc_ism_gen_crc_key_from_keyfields(unit, mem, entry, keyflds, crc_key, 
                                           num_flds, &num_bits);
        lsb = soc_mem_field32_get(unit, mem, entry, lsbfld);
    } else {
        LOG_CLI((BSL_META_U(unit,
                            "Key field not found !!\n")));
        return -1;
    } 
    shbank = _SOC_ISM_MEMS(unit)[memidx].shms->shb;
    bits = soc_mem_entry_bits(unit, mem);
    if (bits > (_SOC_ISM_ENTRY_BITS * 2)) {
        idxinc = 4;
    } else if (bits > _SOC_ISM_ENTRY_BITS) {
        idxinc = 2;
    }
    zero_lsb = _SOC_ISM_SETS(unit)[_SOC_ISM_MEMS(unit)[memidx].shms->mem_set-1].zero_lsb;
    sal_memset(sbo, 0, sizeof(sbo));
    for (idx = 0; idx < _SOC_ISM_MEMS(unit)[memidx].shms->num_banks; idx++) {
        if (banks != -1) {
            /* only use the indicated bank */
            if (!((((int32)1) << shbank[idx].my_id) & banks)) {
                continue;
            }
            num_banks++;
        } else {
            num_banks++;
        }
        sbo[bidx].mode = -1;
        sbo[bidx].bidx = bidx;
        mask = soc_ism_get_hash_bucket_mask(shbank[idx].num_bkts);
        offset = shbank[idx].hash_offset;
        bucket = soc_generic_gen_hash(unit, zero_lsb, num_bits, offset, 
                                      mask, crc_key, lsb);
        sbo[bidx].index = bucket;
        LOG_VERBOSE(BSL_LS_SOC_SOCMEM,
                    (BSL_META_U(unit,
                                "Bank[%d]: bucket:%d\n"), bidx, bucket));
        if (base_idx) {
            *base_idx = sbo[bidx].index;
        }
        if (num_entries) {
            *num_entries = shbank[idx].bkt_size/idxinc;
        }
        sbo[bidx].index = (shbank[idx].base_entry/idxinc) + 
                         (sbo[bidx].index * (shbank[idx].bkt_size/idxinc));
        if (op == 0) { /* Special bucket seek op */
            *index = sbo[bidx].index;
            return SOC_E_NONE;
        }
        for (midx = 0; midx < shbank[idx].bkt_size/idxinc;) {
            rv = soc_mem_read(unit, mem, MEM_BLOCK_ANY, sbo[bidx].index+midx, 
                              tmp_hs);
            if (SOC_FAILURE(rv)) {
                return rv;
            } 
            if (!SOC_MEM_FIELD_VALID(unit, mem, VALIDf)) {
                vf = VALID_0f; 
            }
            if (soc_mem_field32_get(unit, mem, tmp_hs, vf)) {
                sal_memset(tmp_key, 0, sizeof(tmp_key));
                /* compare key */
                soc_ism_gen_key_from_keyfields(unit, mem, tmp_hs, keyflds, 
                                               tmp_key, num_flds);
                if (!sal_memcmp(key, tmp_key, sizeof(key))) {
                    found++;
                    sbo[bidx].entry = midx;
                    sbo[bidx].stage = shbank[idx].my_id / SOC_ISM_INFO(unit)->banks_per_stage;
                    sbo[bidx].bank = shbank[idx].my_id % SOC_ISM_INFO(unit)->banks_per_stage;
                    sbo[bidx].mode = 1;
                    LOG_VERBOSE(BSL_LS_SOC_SOCMEM,
                                (BSL_META_U(unit,
                                            "Existing mem index: %d\n"), 
                                 sbo[bidx].index+sbo[bidx].entry));
                    if (op != TABLE_INSERT_CMD_MSG) {
                        *index = sbo[bidx].index + midx;
                        if (op == TABLE_LOOKUP_CMD_MSG) { 
                            *result = SCHAN_GEN_RESP_TYPE_FOUND;
                        } else {
                            *result = SCHAN_GEN_RESP_TYPE_DELETED;
                        }
                        return SOC_E_NONE;
                    }
                    break;
                }
            } else if (op == TABLE_INSERT_CMD_MSG) {
                found++;
                sbo[bidx].entry = midx;
                sbo[bidx].stage = shbank[idx].my_id / SOC_ISM_INFO(unit)->banks_per_stage;
                sbo[bidx].bank = shbank[idx].my_id % SOC_ISM_INFO(unit)->banks_per_stage;
                sbo[bidx].mode = 0;
                LOG_VERBOSE(BSL_LS_SOC_SOCMEM,
                            (BSL_META_U(unit,
                                        "New mem index: %d\n"), 
                             sbo[bidx].index+sbo[bidx].entry));
                break;
            }
            /* Determine bucket offset increment based upon existing entry type/size */
            midx += soc_ism_get_bucket_offset(unit, mem, memidx, entry, tmp_hs);
        }
        bidx++;
    }
    if (!found) {
        if (op == TABLE_INSERT_CMD_MSG) {
            *result = SCHAN_GEN_RESP_TYPE_FULL;
        } else {
            *result = SCHAN_GEN_RESP_TYPE_NOT_FOUND;
            return SOC_E_NONE;
        }
    }
    /* Get the lowest valid/empty index of the lowest bank-id of the lowest stage-id */
    if (found > 1) {
        soc_ism_resolve_entry_index(sbo, num_banks); 
    }
    for (idx = 0; idx < num_banks; idx++) {
        if (sbo[idx].mode >= 0) {
            *index = sbo[idx].index + sbo[idx].entry;
            if (sbo[idx].mode) {
                *result = SCHAN_GEN_RESP_TYPE_REPLACED;
            } else {
                *result = SCHAN_GEN_RESP_TYPE_INSERTED;
            }
            break;
        }
    }
    return SOC_E_NONE;
}

/* 
 * Normalize memory based on the key type:
 *
 * Move entry may be of different Key type and may have been inserted with
 * different memory view. Hence the need to normalize the memory view name
 * with respect to move_entry. The mem field is not the view used to insert
 * the entry, to make space for the incoming entry, existing entries are moved.
 * The move need to happen with respect to the memory view used to insert the 
 * the already existing entry.
 */
STATIC void 
soc_mem_multi_hash_norm_mem(int unit, soc_mem_t mem, void *entry, soc_mem_t *norm_mem)
{
    int s, k, key_type;
    *norm_mem = mem;
    
    if (SOC_MEM_FIELD_VALID(unit, mem, KEY_TYPEf)) {
        key_type = soc_mem_field32_get(unit, mem, entry, KEY_TYPEf);
    } else {
        key_type = soc_mem_field32_get(unit, mem, entry, KEY_TYPE_0f);
    }
    s = soc_ism_get_hash_mem_idx(unit, mem);
    for (k = 0; k < _SOC_ISM_MEMS(unit)[s].shms->num_keys; k++) {
        if (key_type == _SOC_ISM_MEMS(unit)[s].shms->shk[k].key_type) {
            *norm_mem = _SOC_ISM_MEMS(unit)[s].shms->shk[k].hmv->shm->mem;
            LOG_VERBOSE(BSL_LS_SOC_SOCMEM,
                        (BSL_META_U(unit,
                                    "Normalized for key_type: %d mem: %s\n"), 
                         key_type, SOC_MEM_NAME(unit, *norm_mem)));
            break;
        }  
    }
}

int
soc_mem_multi_hash_move(int unit, soc_mem_t mem, int32 banks, int copyno,
                        void *entry, SHR_BITDCL *bucket_trace, 
                        _soc_ism_mem_banks_t *banks_info, int recurse_depth)
{
    static int mc = 0, ic=0;
    SHR_BITDCL *trace;
    int32 nbix;
    int rv = SOC_E_NONE, index, dest_index;
    _soc_ism_mem_banks_t *mem_banks;
    uint8 i, bix, num_ent, found = 0;
    uint32 cb=0, db, dest_bucket_index, result, trace_size = 0;
    uint32 bucket_index, move_entry[SOC_MAX_MEM_WORDS];
    soc_mem_t orig_mem = INVALIDm;
    soc_mem_t norm_mem = INVALIDm;
    static uint32 recurse, numb = 0;
    
    if (recurse_depth < 0) {
        return SOC_E_FULL;
    }
    /* Stack variables initialization & memory allocations */
    if (NULL == bucket_trace) {
        /* For simplicity, allocate same/max number of bits for buckets for all banks */
        numb = _SOC_ISM_BANK0_SIZE;
        /* Keep back trace of all buckets affected by recursion. */
        trace_size = SHR_BITALLOCSIZE(numb * _SOC_ISM_MAX_BANKS);
        trace =  sal_alloc(trace_size, "N hash");
        if (NULL == trace) {
            return (SOC_E_MEMORY);
        }
        sal_memset(trace, 0, trace_size);
        mem_banks = sal_alloc(sizeof(_soc_ism_mem_banks_t), "N hash banks");
        if (NULL == mem_banks) {
            sal_free(trace);
            return (SOC_E_MEMORY);
        }
        sal_memset(mem_banks, 0, sizeof(_soc_ism_mem_banks_t));
        /* Get mems bank info */
        rv = soc_ism_get_banks_for_mem(unit, mem, mem_banks->banks, 
                                       mem_banks->bank_size, &mem_banks->count);
        if (SOC_FAILURE(rv)) {
            sal_free(trace);
            sal_free(mem_banks);
            return rv;
        }
        if (mem_banks->count == 1) {
            sal_free(trace);
            sal_free(mem_banks);
            return SOC_E_FULL;
        }
    } else {
        trace = bucket_trace;
        mem_banks = banks_info;
    }
    /* Iterate over banks. */
    for (bix = 0; bix < mem_banks->count; bix++) {
        cb = (uint32)1 << mem_banks->banks[bix]; /* current bank */
        if ((banks != SOC_MEM_HASH_BANK_ALL) && (banks == cb)) {
            /* Not this bank */
            continue;
        }
        nbix = (bix+1 >= mem_banks->count) ? 0 : bix+1; /* next bank index */
        db = (uint32)1 << mem_banks->banks[nbix]; /* next destination bank */
        rv = soc_generic_hash(unit, mem, entry, cb, 0, &index, &result, 
                              &bucket_index, &num_ent);
        if (SOC_FAILURE(rv)) {
            break;
        }
        SHR_BITSET(trace, (numb * bix) + bucket_index);
        /* Iterate over un-visited entries in the bucket */
        for (i = 0; i < num_ent;) {
            rv = soc_mem_read(unit, mem, copyno, index+i,
                              move_entry);
            if (SOC_FAILURE(rv)) {
                rv = SOC_E_MEMORY;
                break;
            }
            /* Normalize memory based on the key type */
            orig_mem = mem;
            soc_mem_multi_hash_norm_mem(unit, mem, move_entry, &norm_mem);
            if (mem != norm_mem) {
                mem = norm_mem;
            }

            /* Are we already in recursion or will we be in recursion */
            if (recurse || recurse_depth) {
                /* Calculate destination entry hash value. */
                rv = soc_generic_hash(unit, mem, move_entry, db,
                                      0, &dest_index, &result, 
                                      &dest_bucket_index, NULL);
                if (SOC_FAILURE(rv)) {
                    mem = orig_mem;
                    break;
                }
                /* Make sure we are not touching buckets in bucket trace. */
                if(SHR_BITGET(trace, (numb * nbix) + dest_bucket_index)) {
                    /* Determine bucket offset increment based upon existing entry type/size */
                    i += soc_ism_get_bucket_offset(unit, mem, -1, entry, move_entry);
                    LOG_VERBOSE(BSL_LS_SOC_SOCMEM,
                                (BSL_META_U(unit,
                                            "Skip bucket: %d\n"), 
                                 (numb * nbix) + dest_bucket_index));
                    mem = orig_mem;
                    continue;
                }
            }
            /* Attempt to insert it into the other bank. */
            rv = soc_mem_bank_insert(unit, mem, db, copyno, move_entry, NULL);
            if (SOC_FAILURE(rv)) {
                if (rv != SOC_E_FULL) {
                    mem = orig_mem;
                    break;
                }
                /* Recursive call - attempt to create a slot
                   in another bank's bucket. */
                rv = soc_mem_multi_hash_move(unit, mem, cb, copyno, 
                                             move_entry, trace, mem_banks, 
                                             recurse_depth - 1);
                if (SOC_FAILURE(rv)) {
                    mem = orig_mem;
                    if (rv != SOC_E_FULL) {
                        break;
                    }
                    /* Determine bucket offset increment based upon existing 
                       entry type/size */
                    i += soc_ism_get_bucket_offset(unit, mem, -1, entry, 
                                                   move_entry);
                    continue;
                }
            }
            /* Entry was moved successfully. */
            found = TRUE;
            /* Delete old entry from original location */ 
            rv = soc_mem_generic_delete(unit, mem, MEM_BLOCK_ANY, cb, move_entry, 
                                        NULL, NULL);
            LOG_VERBOSE(BSL_LS_SOC_COMMON,
                        (BSL_META_U(unit,
                                    "Delete moved entry [%d]: %d\n"), mc++, rv));
            mem = orig_mem;
            break;
        }  /* Bucket iteration loop. */
        if (found || ((rv < 0) && (rv != SOC_E_FULL))) {
            break;
        }
    } /* Bank iteration loop. */
    if ((rv < 0) && (rv != SOC_E_FULL)) {
        if (NULL == bucket_trace) { 
            sal_free(trace);
            sal_free(mem_banks);
        }
        return rv;
    }
    if (!found) {
        if (NULL == bucket_trace) {
            sal_free(trace);
            sal_free(mem_banks);
        }
        return SOC_E_FULL;
    }
    rv = soc_mem_generic_insert(unit, mem, copyno, cb, entry, entry, NULL);
    if (rv) {
        LOG_VERBOSE(BSL_LS_SOC_SOCMEM,
                    (BSL_META_U(unit,
                                "Insert entry: %d\n"), rv));
    } else {
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "Insert entry [%d]\n"), ic++));
    }
    if (NULL == bucket_trace) {
        sal_free(trace);
        sal_free(mem_banks);
    }
    return rv;
}

#endif /* BCM_ISM_SUPPORT */

