/* $Id: nd_hw.c 1.21 Broadcom SDK $
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
 * Copyright 2011 BATM
 */

#include "pub/nd_hw.h"
#include "nd_platform.h"
#include "nd_registers.h"
#include "nd_bit.h"
#include "nd_debug.h"
#include "nd_mac.h"
#include <soc/cmic.h>
#include <soc/util.h>
#include "bcm_ces_sal.h"


AgNdChipInfo g_ndChipInfoTable[] = {   
/*    fpga code_id                         mask                       channels    ports     PTP channels */

    { AG_ND_ARCH_TYPE_1_PORT_NEMO,         AG_ND_ARCH_MASK_NEMO,      32,         1,        64 },
    { AG_ND_ARCH_TYPE_2_PORT_NEMO,         AG_ND_ARCH_MASK_NEMO,      64,         2,        64 },
    { AG_ND_ARCH_TYPE_4_PORT_NEMO,         AG_ND_ARCH_MASK_NEMO,      128,        4,        64 },
    { AG_ND_ARCH_TYPE_8_PORT_64_NEMO,      AG_ND_ARCH_MASK_NEMO,      64,         8,        64 },
    { AG_ND_ARCH_TYPE_8_PORT_256_NEMO,     AG_ND_ARCH_MASK_NEMO,      256,        8,        64 },
   #ifndef CES16_BCM_VERSION
    { AG_ND_ARCH_TYPE_16_PORT_NEMO,        AG_ND_ARCH_MASK_NEMO,      512,        16,       64 },
   #else
    { AG_ND_ARCH_TYPE_16_PORT_NEMO,        AG_ND_ARCH_MASK_NEMO,      64,         16,       64 },
   #endif 
    { AG_ND_ARCH_TYPE_T3_NEPTUNE,          AG_ND_ARCH_MASK_NEPTUNE,   768,        0,        768 },
    { AG_ND_ARCH_TYPE_C3_512_NEPTUNE,      AG_ND_ARCH_MASK_NEPTUNE,   512,        0,        512 },
    { AG_ND_ARCH_TYPE_C3_2048_NEPTUNE,     AG_ND_ARCH_MASK_NEPTUNE,   2048,       0,        2048 },
    { AG_ND_ARCH_TYPE_NONE,                0,                         0,          0,        0 }
};

#define AG_REG_ADDR_NAME(reg)  reg, #reg

/* */
/* this mask designates that a register is common for both nemo and neptune */
/* */
#define AG_ND_ARCH_MASK_NN          (AG_ND_ARCH_MASK_NEMO | AG_ND_ARCH_MASK_NEPTUNE)

/* */
/*  */
/* */
#define AG_ND_BIT_UNIMPLEMENTED

AgNdRegProperties g_ndRegTable[] = {
/*                                                                               width  type                  test                     format                       mask        reset       default     size  chip */
                                                                                                                                                                   
/*                                                                                                                                                                  */
/* Global/host inetrface                                                                                                                                            */
/*                                                                                                                                                                  */
{ AG_REG_ADDR_NAME (AG_REG_FPGA_ID),                                             16,    AG_ND_REG_TYPE_RO,    ag_nd_bit_ro_reg16,      AG_ND_REG_FMT_GLOBAL,        0x0000ffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },       
{ AG_REG_ADDR_NAME (AG_REG_GLOBAL_CONTROL),                                      16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x00000fff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_CHANNELIZER_LOOPBACK_CHANNEL_SELECT),                 16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x000007ff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_GLOBAL_CLK_ACTIVITY_MONITORING),                      16,    AG_ND_REG_TYPE_RCLR,  NULL,                    AG_ND_REG_FMT_GLOBAL,        0x0000007f, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_TEST_BUS_CONTROL),                                    16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x000000ff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_EXTENDED_ADDRESS),									 16,	AG_ND_REG_TYPE_RW,	  ag_nd_bit_rw_reg16,	   AG_ND_REG_FMT_GLOBAL,		0x0000800f, 0x00000000, 0x00000000, 1,	  AG_ND_ARCH_MASK_NN	  },
#ifndef CES16_BCM_VERSION
{ AG_REG_ADDR_NAME (AG_REG_MEMORY_CONFIGURATION_ADDRESS),						 16,	AG_ND_REG_TYPE_RW,	  ag_nd_bit_rw_reg16,	   AG_ND_REG_FMT_GLOBAL,		0x0000ffff, 0x00000000, 0x00000000, 1,	  AG_ND_ARCH_MASK_NN	  },
#endif                                                                                                                                                                   
/*                                                                                                                                                                  */
/* Interrupt handling                                                                                                                                               */
/*                                                                                                                                                                  */
{ AG_REG_ADDR_NAME (AG_REG_GLOBAL_INTERRUPT_ENABLE),                             16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x0000001f, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_GLOBAL_INTERRUPT_STATUS),                             16,    AG_ND_REG_TYPE_W1CLR, ag_nd_bit_rw1clr_reg16,  AG_ND_REG_FMT_GLOBAL,        0x0000001f, 0x0000001f, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
                                                                                                                                                                   
/*                                                                                                                                                                  */
/* TDM Nemo                                                                                                                                                         */
/*                                                                                                                                                                  */
{ AG_REG_ADDR_NAME (AG_REG_T1_E1_CCLK_MUX_1),                                    16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x0000ffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },

/*#ifdef AG_ND_PTP_SUPPORT */
{ AG_REG_ADDR_NAME (AG_REG_T1_E1_CCLK_MUX_2),                                    16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x0000ffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
/*#else */
/*{ AG_REG_ADDR_NAME (AG_REG_T1_E1_CCLK_MUX_2),                                    16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x0000fff0, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    }, */
/*#endif */

{ AG_REG_ADDR_NAME (AG_REG_TDM_PORT_ACTIVITY_MONITORING),                        16,    AG_ND_REG_TYPE_RO,    NULL,                    AG_ND_REG_FMT_GLOBAL,        0x0000ffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },

{ AG_REG_ADDR_NAME (AG_REG_PAYLOAD_REPLACEMENT_POLICY),                          16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x0000ffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },                                                                                                                                                                  
{ AG_REG_ADDR_NAME (AG_REG_PACKET_PAYLOAD_REPLACEMENT_BYTE_PATTERN),             16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x0000ffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },

/* CAS TEMP: change mask to 0x000083ff when implemented in Nemo */
{ AG_REG_ADDR_NAME (AG_REG_T1_E1_PORT_CONFIGURATION(0)),                         16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_CIRCUIT,       0x0000827f, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },

{ AG_REG_ADDR_NAME (AG_REG_PORT_CLK_BAUD_RATE_GENERATOR1(0)),                    16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_CIRCUIT,       0x0000ffff, 0x00000000, 0x00000000, 2,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_RTP_TIMESTAMP_GENERATOR_CONFIGURATION),               16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x0000efff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_DCR_COMMON_REF_CLK_SOURCE_SELECTION),                 16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x000000ff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_RTP_TIMESTAMP_STEP_SIZE_1),                           16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x00001fff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_RTP_TIMESTAMP_STEP_SIZE_2),                           16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x0000ffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_RTP_TIMESTAMP_FAST_PHASE_CONFIGURATION),              16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x000003ff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_RTP_TIMESTAMP_ERROR_CORRECTION_PERIOD),               16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x0000ffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_RECOVERED_CLK_TIMESTAMP_SAMPLING_PERIOD_1(0)),        16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_CIRCUIT,       0x00000007, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_RECOVERED_CLK_TIMESTAMP_SAMPLING_PERIOD_2(0)),        16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_CIRCUIT,       0x0000ffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_RECOVERED_CLK_LOCAL_TIMESTAMP_1(0)),                  16,    AG_ND_REG_TYPE_RO,    ag_nd_bit_ro_reg16,      AG_ND_REG_FMT_CIRCUIT,       0x0000ffff, 0x00000000, 0x00000000, 2,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_RECOVERED_CLK_FAST_PHASE_TIMESTAMP(0)),               16,    AG_ND_REG_TYPE_RO,    ag_nd_bit_ro_reg16,      AG_ND_REG_FMT_CIRCUIT,       0x000000ff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
                                                                                                                                                                       
/*                                                                                                                                                                  */
/* TDM Neptune                                                                                                                                                      */
/*                                                                                                                                                                  */
{ AG_REG_ADDR_NAME (AG_REG_TDM_BUS_INTERFACE_CONFIGURATION),                     16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x00000000, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_TDM_BUS_HIERARCHY_CONFIGURATION),                     16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x00003f00, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_TRIBUTARY_CIRCUIT_TYPE_PER_VC_2_1),                   16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x0000007f, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE }, 
{ AG_REG_ADDR_NAME (AG_REG_TRIBUTARY_CIRCUIT_TYPE_PER_VC_2_2),                   16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x00007f7f, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE }, 

{ AG_REG_ADDR_NAME (AG_REG_TRIBUTARY_CIRCUIT_CONFIGURATION(0)),                  16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_CIRCUIT,       0x0000e038, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_BIT_STUFFING_GENERATOR_1(0)),                         16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_CIRCUIT,       0x0000ffff, 0x00000000, 0x00000000, 2,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_JUSTIFICATION_REQUEST_CONTROL(0)),                    16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_CIRCUIT,       0x0000fc00, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_RTP_TIMESTAMP_GENERATOR_CONFIGURATION_OC3),           16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x00008fff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_RTP_TIMESTAMP_FRAME_STEP_SIZE),                       16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x00000fff, 0x0000097e, 0x0000097e, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_RTP_TIMESTAMP_BIT_STEP_SIZE),                         16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x0000ffff, 0x00004bf0, 0x00004bf0, 1,    AG_ND_ARCH_MASK_NEPTUNE },
                                                                                                                                                                   
/*                                                                               width  type                  test                     format                       mask        reset       default     size  chip */
                                                                                                                                                                   
/*                                                                                                                                                                  */
/* CAS                                                                                                                                                              */
/*                                                                                                                                                                  */
/* CAS TEMP: change chip to NN for 5 next registers when implemented in Nemo */
#ifndef CES16_BCM_VERSION
{ AG_REG_ADDR_NAME (AG_REG_CAS_INGRESS_DATA_BUFFER(0,0)),                        16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_CIRCUIT,       0x0000ffff, 0x00000000, 0x00000000, 8,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_CAS_EGRESS_DATA_BUFFER(0,0)),                         16,    AG_ND_REG_TYPE_RO,    ag_nd_bit_ro_reg16,      AG_ND_REG_FMT_CIRCUIT,       0x0000ffff, 0x00000000, 0x00000000, 8,    AG_ND_ARCH_MASK_NEPTUNE },
#endif
{ AG_REG_ADDR_NAME (AG_REG_CAS_INGRESS_CHANGE_DETECTION(0,0)),                   16,    AG_ND_REG_TYPE_W1CLR, ag_nd_bit_rw1clr_reg16,  AG_ND_REG_FMT_CIRCUIT,       0x0000ffff, 0x0000ffff, 0x00000000, 2,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_CAS_DATA_REPLACEMENT(0,0)),                           16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_CIRCUIT,       0x0000ffff, 0x0000ffff, 0x0000ffff, 2,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_CAS_IDLE_PATTERN),									 16,	AG_ND_REG_TYPE_RW,	  ag_nd_bit_rw_reg16,	   AG_ND_REG_FMT_GLOBAL,		0x000000ff, 0x00000000, 0x00000000, 1,	  AG_ND_ARCH_MASK_NEPTUNE },

/*{ AG_REG_ADDR_NAME (AG_REG_DCR_SYSTEM_REF_CLK_SOURCE_SELECTION),				 16,	AG_ND_REG_TYPE_RW,	  ag_nd_bit_rw_reg16,	   AG_ND_REG_FMT_CIRCUIT,		0x000000ff, 0x00000000, 0x00000000, 1,	  AG_ND_ARCH_MASK_NEMO }, */
/*{ AG_REG_ADDR_NAME (AG_REG_BSG_ONE_SHOT_COUNTER),								 16,	AG_ND_REG_TYPE_RW,	  ag_nd_bit_rw_reg16,	   AG_ND_REG_FMT_CIRCUIT,		0x000000ff, 0x00000000, 0x00000000, 1,	  AG_ND_ARCH_MASK_NEPTUNE }, */
                                                                                                                                                                   
/*                                                                                                                                                                           */
/*  PIF (Nemo)                                                                                                                                                      */
/*                                                                                                                                                                           */
#ifndef CES16_BCM_VERSION
{ AG_REG_ADDR_NAME (AG_REG_PIF_REV),                                             32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_SCRATCH),                                         32,    AG_ND_REG_TYPE_RW,    ag_nd_mac_bit_rw_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_COMMAND_CONFIG_1),                                32,    AG_ND_REG_TYPE_RW,    ag_nd_mac_bit_rw_reg32,  AG_ND_REG_FMT_GLOBAL,        0x079fc3fb, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_MAC_0),                                           32,    AG_ND_REG_TYPE_RW,    ag_nd_mac_bit_rw_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_MAC_1),                                           32,    AG_ND_REG_TYPE_RW,    ag_nd_mac_bit_rw_reg32,  AG_ND_REG_FMT_GLOBAL,        0x0000ffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_FRM_LENGTH),                                      32,    AG_ND_REG_TYPE_RW,    ag_nd_mac_bit_rw_reg32,  AG_ND_REG_FMT_GLOBAL,        0x00003fff, 0x000005f2, 0x000005f2, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_PAUSE_QUANT),                                     32,    AG_ND_REG_TYPE_RW,    ag_nd_mac_bit_rw_reg32,  AG_ND_REG_FMT_GLOBAL,        0x0000ffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_RX_SECTION_EMPTY),                                32,    AG_ND_REG_TYPE_RW,    ag_nd_mac_bit_rw_reg32,  AG_ND_REG_FMT_GLOBAL,        0x0000007f, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_RX_SECTION_FULL),                                 32,    AG_ND_REG_TYPE_RW,    ag_nd_mac_bit_rw_reg32,  AG_ND_REG_FMT_GLOBAL,        0x0000007f, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_TX_SECTION_EMPTY),                                32,    AG_ND_REG_TYPE_RW,    ag_nd_mac_bit_rw_reg32,  AG_ND_REG_FMT_GLOBAL,        0x0000007f, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_TX_SECTION_FULL),                                 32,    AG_ND_REG_TYPE_RW,    ag_nd_mac_bit_rw_reg32,  AG_ND_REG_FMT_GLOBAL,        0x0000007f, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_RX_ALMOST_EMPTY),                                 32,    AG_ND_REG_TYPE_RW,    ag_nd_mac_bit_rw_reg32,  AG_ND_REG_FMT_GLOBAL,        0x0000007f, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_RX_ALMOST_FULL),                                  32,    AG_ND_REG_TYPE_RW,    ag_nd_mac_bit_rw_reg32,  AG_ND_REG_FMT_GLOBAL,        0x0000007f, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_TX_ALMOST_EMPTY),                                 32,    AG_ND_REG_TYPE_RW,    ag_nd_mac_bit_rw_reg32,  AG_ND_REG_FMT_GLOBAL,        0x0000007f, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_TX_ALMOST_FULL),                                  32,    AG_ND_REG_TYPE_RW,    ag_nd_mac_bit_rw_reg32,  AG_ND_REG_FMT_GLOBAL,        0x0000007f, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_MDIO_ADDR_0),                                     32,    AG_ND_REG_TYPE_RW,    ag_nd_mac_bit_rw_reg32,  AG_ND_REG_FMT_GLOBAL,        0x0000001f, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_MDIO_ADDR_1),                                     32,    AG_ND_REG_TYPE_RW,    ag_nd_mac_bit_rw_reg32,  AG_ND_REG_FMT_GLOBAL,        0x0000001f, 0x00000001, 0x00000001, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_REG_STAT),                                        32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0x00000001, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_TX_IPG_LENGTH),                                   32,    AG_ND_REG_TYPE_RW,    ag_nd_mac_bit_rw_reg32,  AG_ND_REG_FMT_GLOBAL,        0x0000001f, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
                                                                                                                                                                                                       
{ AG_REG_ADDR_NAME (AG_REG_PIF_MACID_0),                                         32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_MACID_1),                                         32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_FRAMES_TRANSMITTED_OK),                           32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_FRAMES_RECEIVED_OK),                              32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_FRAME_CHECK_SEQUENCE_ERRORS),                     32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_ALIGNMENT_ERRORS),                                32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_OCTETS_TRANSMITTED_OK),                           32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_OCTETS_RECEIVED_OK),                              32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_TX_PAUSE_MAC_CTRL_FRAMES),                        32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_RX_PAUSE_MAC_CTRL_FRAMES),                        32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_IN_ERRORS),                                       32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },

{ AG_REG_ADDR_NAME (AG_REG_PIF_OUT_ERRORS),                                      32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_IN_UNICAST_PACKETS),                              32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_IN_MULTICAST_PACKETS),                            32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_IN_BROADCAST_PACKETS),                            32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_OUT_DISCARDS),                                    32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_OUT_UNICAST_PACKETS),                             32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_OUT_MULTICAST_PACKETS),                           32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_OUT_BROADCAST_PACKETS),                           32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_STATS_DROP_EVENTS),                               32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_STATS_OCTETS),                                    32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_STATS_PACKETS),                                   32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_STATS_UNDERSIZE_PACKETS),                         32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_STATS_OVERSIZE_PACKETS),                          32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_STATS_PACKETS_64_OCTETS),                         32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_STATS_PACKETS_65_TO_127_OCTETS),                  32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_STATS_PACKETS_128_TO_255_OCTETS),                 32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_STATS_PACKETS_256_TO_511_OCTETS),                 32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_STATS_PACKETS_512_TO_1023_OCTETS),                32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PIF_STATS_PACKETS_1024_TO_1518_OCTETS),               32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },

/*                                                                                                                                                                           */
/*  PIF (Neptune)                                                                                                                                                   */
/*                                                                                                                                                                           */
/*                                                                               width  type                  test                     format                       mask        reset       default     size  chip                                                                                                                                                                      */
{ AG_REG_ADDR_NAME (AG_REG_PIF_REV),                                             32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_SCRATCH),                                         32,    AG_ND_REG_TYPE_RW,    ag_nd_mac_bit_rw_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_COMMAND_CONFIG_1),                                32,    AG_ND_REG_TYPE_RW,    ag_nd_mac_bit_rw_reg32,  AG_ND_REG_FMT_GLOBAL,        0x079fc3fb, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_MAC_0),                                           32,    AG_ND_REG_TYPE_RW,    ag_nd_mac_bit_rw_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_MAC_1),                                           32,    AG_ND_REG_TYPE_RW,    ag_nd_mac_bit_rw_reg32,  AG_ND_REG_FMT_GLOBAL,        0x0000ffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_FRM_LENGTH),                                      32,    AG_ND_REG_TYPE_RW,    ag_nd_mac_bit_rw_reg32,  AG_ND_REG_FMT_GLOBAL,        0x00003fff, 0x000005f2, 0x000005f2, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_PAUSE_QUANT),                                     32,    AG_ND_REG_TYPE_RW,    ag_nd_mac_bit_rw_reg32,  AG_ND_REG_FMT_GLOBAL,        0x0000ffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_RX_SECTION_EMPTY),                                32,    AG_ND_REG_TYPE_RW,    ag_nd_mac_bit_rw_reg32,  AG_ND_REG_FMT_GLOBAL,        0x0000003f, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_RX_SECTION_FULL),                                 32,    AG_ND_REG_TYPE_RW,    ag_nd_mac_bit_rw_reg32,  AG_ND_REG_FMT_GLOBAL,        0x0000003f, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_TX_SECTION_EMPTY),                                32,    AG_ND_REG_TYPE_RW,    ag_nd_mac_bit_rw_reg32,  AG_ND_REG_FMT_GLOBAL,        0x0000003f, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_TX_SECTION_FULL),                                 32,    AG_ND_REG_TYPE_RW,    ag_nd_mac_bit_rw_reg32,  AG_ND_REG_FMT_GLOBAL,        0x0000003f, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_RX_ALMOST_EMPTY),                                 32,    AG_ND_REG_TYPE_RW,    ag_nd_mac_bit_rw_reg32,  AG_ND_REG_FMT_GLOBAL,        0x0000003f, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_RX_ALMOST_FULL),                                  32,    AG_ND_REG_TYPE_RW,    ag_nd_mac_bit_rw_reg32,  AG_ND_REG_FMT_GLOBAL,        0x0000003f, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_TX_ALMOST_EMPTY),                                 32,    AG_ND_REG_TYPE_RW,    ag_nd_mac_bit_rw_reg32,  AG_ND_REG_FMT_GLOBAL,        0x0000003f, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_TX_ALMOST_FULL),                                  32,    AG_ND_REG_TYPE_RW,    ag_nd_mac_bit_rw_reg32,  AG_ND_REG_FMT_GLOBAL,        0x0000003f, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_MDIO_ADDR_0),                                     32,    AG_ND_REG_TYPE_RW,    ag_nd_mac_bit_rw_reg32,  AG_ND_REG_FMT_GLOBAL,        0x0000001f, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_MDIO_ADDR_1),                                     32,    AG_ND_REG_TYPE_RW,    ag_nd_mac_bit_rw_reg32,  AG_ND_REG_FMT_GLOBAL,        0x0000001f, 0x00000001, 0x00000001, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_REG_STAT),                                        32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0x00000001, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_TX_IPG_LENGTH),                                   32,    AG_ND_REG_TYPE_RW,    ag_nd_mac_bit_rw_reg32,  AG_ND_REG_FMT_GLOBAL,        0x0000001f, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
                                                                                                                                                                                                       
{ AG_REG_ADDR_NAME (AG_REG_PIF_MACID_0),                                         32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_MACID_1),                                         32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_FRAMES_TRANSMITTED_OK),                           32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_FRAMES_RECEIVED_OK),                              32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_FRAME_CHECK_SEQUENCE_ERRORS),                     32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_ALIGNMENT_ERRORS),                                32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_OCTETS_TRANSMITTED_OK),                           32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_OCTETS_RECEIVED_OK),                              32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_TX_PAUSE_MAC_CTRL_FRAMES),                        32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_RX_PAUSE_MAC_CTRL_FRAMES),                        32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_IN_ERRORS),                                       32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_OUT_ERRORS),                                      32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_IN_UNICAST_PACKETS),                              32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_IN_MULTICAST_PACKETS),                            32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_IN_BROADCAST_PACKETS),                            32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_OUT_DISCARDS),                                    32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_OUT_UNICAST_PACKETS),                             32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_OUT_MULTICAST_PACKETS),                           32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_OUT_BROADCAST_PACKETS),                           32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_STATS_DROP_EVENTS),                               32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_STATS_OCTETS),                                    32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_STATS_PACKETS),                                   32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_STATS_UNDERSIZE_PACKETS),                         32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_STATS_OVERSIZE_PACKETS),                          32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_STATS_PACKETS_64_OCTETS),                         32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_STATS_PACKETS_65_TO_127_OCTETS),                  32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_STATS_PACKETS_128_TO_255_OCTETS),                 32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_STATS_PACKETS_256_TO_511_OCTETS),                 32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_STATS_PACKETS_512_TO_1023_OCTETS),                32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_PIF_STATS_PACKETS_1024_TO_1518_OCTETS),               32,    AG_ND_REG_TYPE_RO,    ag_nd_mac_bit_ro_reg32,  AG_ND_REG_FMT_GLOBAL,        0xffffffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
#endif /*CES16_BCM_VERSION*/
                                                                                                                                                                       

/*                                                                                                                                                                           */
/*  CHI                                                                                                                                                                      */
/*                                                                                                                                                                           */
{ AG_REG_ADDR_NAME (AG_REG_INGRESS_CIRCUIT_TO_CHANNEL_MAP(0,0)),                 16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_CIRCUIT,       0x0000c7ff, 0x00000000, 0x00000000, 32,   AG_ND_ARCH_MASK_NN      },       
{ AG_REG_ADDR_NAME (AG_REG_INGRESS_CHANNEL_CONTROL(0)),                          16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_CHANNEL,       0x00008000, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },       
#ifndef CES16_BCM_VERSION
{ AG_REG_ADDR_NAME (AG_REG_INGRESS_TIMESLOT_ALLOCATION_CONTROL(0)),              16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_CHANNEL,       0x0000b000, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
#endif /*CES16_BCM_VERSION*/
                                                                                                                                                                                                    
/*                                                                                                                                                                           */
/*  CHE                                                                                                                                                                      */
/*                                                                                                                                                                           */
/*                                                                               width  type                  test                     format                       mask        reset       default     size  chip                                                                                                                                                                      */
{ AG_REG_ADDR_NAME (AG_REG_EGRESS_CIRCUIT_TO_CHANNEL_MAP(0,0)),                  16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_CIRCUIT,       0x0000c7ff, 0x00000000, 0x00000000, 32,   AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_EGRESS_CHANNEL_CONTROL(0)),                           16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_CHANNEL,       0x00008000, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },       
#ifndef CES16_BCM_VERSION
{ AG_REG_ADDR_NAME (AG_REG_EGRESS_TIMESLOT_ALLOCATION_CONTROL(0)),               16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_CHANNEL,       0x0000b000, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
#endif /*CES16_BCM_VERSION*/
                                                                                                                                                                       
                                                                                                                                                                   
/*                                                                               width  type                  test                     format                       mask        reset       default     size  chip */
                                                                                                                                                                   
/*                                                                                                                                                                           */
/*  PBF                                                                                                                                                                      */
/*                                                                                                                                                                           */
{ AG_REG_ADDR_NAME (AG_REG_PAYLOAD_BUFFER_CHANNEL_CONFIGURATION(0)),             16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_CHANNEL,       0x0000ffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
#ifndef CES16_BCM_VERSION
{ AG_REG_ADDR_NAME (AG_REG_PAYLOAD_BUFFER_QUEUE_BASE_ADDRESS(0)),                16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_CHANNEL,       0x0000ffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_PAYLOAD_BUFFER_STATUS(0)),                            16,    AG_ND_REG_TYPE_RCLR,  ag_nd_bit_rclr_reg16,    AG_ND_REG_FMT_CHANNEL,       0x00009fff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
#endif /*CES16_BCM_VERSION*/

{ AG_REG_ADDR_NAME (AG_REG_TRANSMIT_HEADER_CONFIGURATION(0)),                    16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_CHANNEL,       0x000003ff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
#ifndef CES16_BCM_VERSION
{ AG_REG_ADDR_NAME (AG_REG_TRANSMIT_HEADER_ACCESS_CONTROL),                      16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x00003fff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
#endif /*CES16_BCM_VERSION*/

/* CAS TEMP: change chip to NN for 5 next registers when implemented in Nemo */
{ AG_REG_ADDR_NAME (AG_REG_PBF_CAS_CONFIGURATION(0)),                            16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_CHANNEL,       0x000087ff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
/* although AG_REG_PAYLOAD_CAS_BUFFER_STATUS register is writable we handle it like RO */
{ AG_REG_ADDR_NAME (AG_REG_PAYLOAD_CAS_BUFFER_STATUS(0)),                        16,    AG_ND_REG_TYPE_RO,    NULL,                    AG_ND_REG_FMT_CHANNEL,       0x0000c000, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_TRANSMIT_CAS_HEADER_CONFIGURATION(0)),                16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_CHANNEL,       0x0000007f, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
#ifndef CES16_BCM_VERSION
{ AG_REG_ADDR_NAME (AG_REG_TRANSMIT_CAS_HEADER_ACCESS_CONTROL),                  16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x00003fff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
#endif /*CES16_BCM_VERSION*/

{ AG_REG_ADDR_NAME (AG_REG_TRANSMIT_CAS_PACKET_DELAY),                           16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x0000fc00, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },

#ifndef CES16_BCM_VERSION
/*/#ifdef AG_ND_PTP_SUPPORT */
{ AG_REG_ADDR_NAME (AG_REG_PBF_PTP_CONFIGURATION_REGISTER(0)),                   16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_CHANNEL,       0x00003fff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PTP_TRANSMIT_HEADER_ACCESS_CONTROL),                  16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x00003fff, 0x00000000, 0x00000000, 2,    AG_ND_ARCH_MASK_NEMO    },
/*/#endif */
#endif /*CES16_BCM_VERSION*/
                                                                                                                                                                   
/*                                                                                                                                                                           */
/*  JBF                                                                                                                                                                      */
/*                                                                                                                                                                           */
{ AG_REG_ADDR_NAME (AG_REG_JITTER_BUFFER_CHANNEL_CONFIGURATION_1(0)),            16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_CHANNEL,       0x0000ffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_JITTER_BUFFER_CHANNEL_CONFIGURATION_2(0)),            16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_CHANNEL,       0x0000f7ff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
#ifndef CES16_BCM_VERSION
{ AG_REG_ADDR_NAME (AG_REG_JITTER_BUFFER_QUEUE_BASE_ADDRESS(0)),                 16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_CHANNEL,       0x00000fff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_JITTER_BUFFER_VALID_BIT_ARRAY_CONFIGURATION(0)),      16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_CHANNEL,       0x00009fff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
#endif /*CES16_BCM_VERSION*/


/* CAS TEMP: change chip to NN when implemented in Nemo */
{ AG_REG_ADDR_NAME (AG_REG_CAS_MISORDERED_DROPPED_PACKET_COUNT(0)),              16,    AG_ND_REG_TYPE_RCLR,  ag_nd_bit_rclr_reg16,    AG_ND_REG_FMT_CHANNEL,       0x0000ffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },

{ AG_REG_ADDR_NAME (AG_REG_PWE3_IN_BAD_LENGTH_PACKET_COUNT(0)),                  16,    AG_ND_REG_TYPE_RCLR,  ag_nd_bit_rclr_reg16,    AG_ND_REG_FMT_CHANNEL,       0x00003fff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_MISORDERED_DROPPED_PACKET_COUNT(0)),                  16,    AG_ND_REG_TYPE_RCLR,  ag_nd_bit_rclr_reg16,    AG_ND_REG_FMT_CHANNEL,       0x00003fff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_REORDERED_GOOD_PACKET_COUNT(0)),                      16,    AG_ND_REG_TYPE_RCLR,  ag_nd_bit_rclr_reg16,    AG_ND_REG_FMT_CHANNEL,       0x00003fff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_MINIMUM_JITTER_BUFFER_DEPTH_1(0)),                    16,    AG_ND_REG_TYPE_RCLR,  ag_nd_bit_rclr_reg16,    AG_ND_REG_FMT_CHANNEL,       0x0000ffff, 0x00000000, 0x0000ffff, 2,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_MAXIMUM_JITTER_BUFFER_DEPTH_1(0)),                    16,    AG_ND_REG_TYPE_RCLR,  ag_nd_bit_rclr_reg16,    AG_ND_REG_FMT_CHANNEL,       0x0000ffff, 0x00000000, 0x00000000, 2,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_JITTER_BUFFER_MISSING_PACKET_COUNT(0)),               16,    AG_ND_REG_TYPE_RCLR,  ag_nd_bit_rclr_reg16,    AG_ND_REG_FMT_CHANNEL,       0x00003fff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_JITTER_BUFFER_UNDERRUN_PACKET_COUNT(0)),              16,    AG_ND_REG_TYPE_RCLR,  ag_nd_bit_rclr_reg16,    AG_ND_REG_FMT_CHANNEL,       0x00003fff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_JITTER_BUFFER_RESTART_COUNT(0)),                      16,    AG_ND_REG_TYPE_RCLR,  ag_nd_bit_rclr_reg16,    AG_ND_REG_FMT_CHANNEL,       0x0000000f, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_JITTER_BUFFER_CHANNEL_STATUS(0)),                     16,    AG_ND_REG_TYPE_W1CLR, ag_nd_bit_rw1clr_reg16,  AG_ND_REG_FMT_CHANNEL,       0x000033ff, 0x00000400, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_JITTER_BUFFER_SLIP_COMMAND),                          16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x000037ff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
#ifndef CES16_BCM_VERSION
{ AG_REG_ADDR_NAME (AG_REG_JITTER_BUFFER_SLIP_CONFIGURATION),                    16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x0000ffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
#endif /*CES16_BCM_VERSION*/

{ AG_REG_ADDR_NAME (AG_REG_PACKET_SYNCHRONIZATION_INTERRUPT_EVENT_QUEUE),        16,    AG_ND_REG_TYPE_RO,    ag_nd_bit_ro_reg16,      AG_ND_REG_FMT_GLOBAL,        0x0000c7ff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_PACKET_SYNCHRONIZATION_INTERRUPT_QUEUE_STATUS),       16,    AG_ND_REG_TYPE_RO,    ag_nd_bit_ro_reg16,      AG_ND_REG_FMT_GLOBAL,        0x000007ff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },

/* CAS TEMP: change chip to NN when implemented in Nemo */
{ AG_REG_ADDR_NAME (AG_REG_JITTER_BUFFER_CAS_CONFIGURATION(0)),                  16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_CHANNEL,       0x000007ff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_JITTER_BUFFER_CAS_SEQUENCE_NUMBER_WINDOW),            16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x000000ff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },

/* */
/* There is a problem with packet sync status register: need to wait 125 usec */
/* after clearing the PSYN bit, so ag_nd_bit_rw1clr_reg16 isn't good enough. */
/* Also it seems that port(s) should be enabled  */
/* meanwhile PSYN is masked */
/*  */
{ AG_REG_ADDR_NAME (AG_REG_PACKET_SYNC_STATUS(0)),                               16,    AG_ND_REG_TYPE_W1CLR, ag_nd_bit_rw1clr_reg16,  AG_ND_REG_FMT_CHANNEL,       0x0000c000, 0x00008000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
                                                                                                                                                                   
{ AG_REG_ADDR_NAME (AG_REG_LOSS_OF_PACKET_SYNC_THRESHOLD_TABLE(0)),              16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x00001fff, 0x000003ff, 0x000003ff, AG_ND_SYNC_THRESHOLD_TABLE_SIZE,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_ACQUISITION_OF_PACKET_SYNC_THRESHOLD_TABLE(0)),       16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x00001fff, 0x00000003, 0x00000003, AG_ND_SYNC_THRESHOLD_TABLE_SIZE,    AG_ND_ARCH_MASK_NN      },
                                                                                                                                                                       
                                                                                                                                                                   
/*                                                                               width  type                  test                     format                       mask        reset       default     size  chip */
                                                                                                                                                                   
/*                                                                                                                                                                           */
/*  TPE                                                                                                                                                                      */
/*                                                                                                                                                                  */
{ AG_REG_ADDR_NAME (AG_REG_TPE_GLOBAL_CONFIGURATION),                            16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x0000c000, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_TRANSMITTED_RTP_TIMESTAMP_1(0)),                      16,    AG_ND_REG_TYPE_RO,    ag_nd_bit_ro_reg16,      AG_ND_REG_FMT_CHANNEL,       0x0000ffff, 0x00000000, 0x00000000, 2,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_PWE_OUT_TRANSMITTED_BYTES_1(0)),                      16,    AG_ND_REG_TYPE_RCLR,  ag_nd_bit_rclr_reg16,    AG_ND_REG_FMT_CHANNEL,       0x000000ff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_PWE_OUT_TRANSMITTED_BYTES_2(0)),                      16,    AG_ND_REG_TYPE_RCLR,  ag_nd_bit_rclr_reg16,    AG_ND_REG_FMT_CHANNEL,       0x0000ffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_PWE_OUT_TRANSMITTED_PACKETS(0)),                      16,    AG_ND_REG_TYPE_RCLR,  ag_nd_bit_rclr_reg16,    AG_ND_REG_FMT_CHANNEL,       0x0000ffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },

/* CAS TEMP: change chip to NN when implemented in Nemo */
{ AG_REG_ADDR_NAME (AG_REG_CAS_OUT_TRANSMITTED_PACKETS(0)),                      16,    AG_ND_REG_TYPE_RCLR,  ag_nd_bit_rclr_reg16,    AG_ND_REG_FMT_CHANNEL,       0x0000ffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },

{ AG_REG_ADDR_NAME (AG_REG_PW_OUT_SEQUENCE_NUMBER(0)),                           16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_CHANNEL,       0x0000ffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },

/* CAS TEMP: change chip to NN when implemented in Nemo */
{ AG_REG_ADDR_NAME (AG_REG_CAS_OUT_SEQUENCE_NUMBER(0)),                          16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_CHANNEL,       0x0000ffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },

/*                                                                                                                                                                     */
/*  RPC                                                                                                                                                                */
/*                                                                                                                                                                     */
/*                                                                               width  type                  test                     format                       mask        reset       default     size  chip                                                                                                                                                                      */
{ AG_REG_ADDR_NAME (AG_REG_RECEIVE_CHANNEL_CONFIGURATION(0)),                    16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_CHANNEL,       0x0000c002, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
#ifndef CES16_BCM_VERSION
{ AG_REG_ADDR_NAME (AG_REG_LABEL_SEARCH_TABLE_BASE_ADDRESS),                     16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x0000007f, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_STRICT_CHECK_TABLE_BASE_ADDRESS),                     16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x0000007f, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
#endif /*CES16_BCM_VERSION*/


#ifndef CES16_BCM_VERSION
/*/#ifdef AG_ND_PTP_SUPPORT                                                                                                                                            */
{ AG_REG_ADDR_NAME (AG_REG_PTP_CHANNELS_STRICT_CHECK_TABLE_BASE_ADDRESS),        16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x0000007f, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
/*#endif                                                                                                                                                              */
#endif /*CES16_BCM_VERSION*/

{ AG_REG_ADDR_NAME (AG_REG_CES_CONTROL_WORD_INTERRUPT_MASK),                     16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x00000fc0, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_CES_CONTROL_WORD_INTERRUPT_STATUS(0)),                16,    AG_ND_REG_TYPE_W1CLR, ag_nd_bit_rw1clr_reg16,  AG_ND_REG_FMT_CHANNEL,       0x00000fc0, 0x00000fc0, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_RECEIVE_CES_CONTROL_WORD(0)),                         16,    AG_ND_REG_TYPE_RO,    ag_nd_bit_ro_reg16,      AG_ND_REG_FMT_CHANNEL,       0x00000fc0, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_PWE_IN_RECEIVED_BYTES_1(0)),                          16,    AG_ND_REG_TYPE_RCLR,  ag_nd_bit_rclr_reg16,    AG_ND_REG_FMT_CHANNEL,       0x000000ff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_PWE_IN_RECEIVED_BYTES_2(0)),                          16,    AG_ND_REG_TYPE_RCLR,  ag_nd_bit_rclr_reg16,    AG_ND_REG_FMT_CHANNEL,       0x0000ffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_PWE_IN_RECEIVED_PACKETS(0)),                          16,    AG_ND_REG_TYPE_RCLR,  ag_nd_bit_rclr_reg16,    AG_ND_REG_FMT_CHANNEL,       0x0000ffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_CLASSIFIER_DROPPED_PACKET_COUNT),                     16,    AG_ND_REG_TYPE_RCLR,  ag_nd_bit_rclr_reg16,    AG_ND_REG_FMT_GLOBAL,        0x0000ffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
#ifndef CES16_BCM_VERSION
{ AG_REG_ADDR_NAME (AG_REG_HIGH_PRIORITY_HOST_PACKET_COUNT),                     16,    AG_ND_REG_TYPE_RCLR,  ag_nd_bit_rclr_reg16,    AG_ND_REG_FMT_GLOBAL,        0x0000ffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_LOW_PRIORITY_HOST_PACKET_COUNT),                      16,    AG_ND_REG_TYPE_RCLR,  ag_nd_bit_rclr_reg16,    AG_ND_REG_FMT_GLOBAL,        0x0000ffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
#endif /*CES16_BCM_VERSION*/
{ AG_REG_ADDR_NAME (AG_REG_RAI_CONFIGURATION),                                   16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x00000003, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_CES_CONTROL_WORD_INTERRUPT_EVENT_QUEUE),              16,    AG_ND_REG_TYPE_RO,    ag_nd_bit_ro_reg16,      AG_ND_REG_FMT_GLOBAL,        0x0000c7ff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_CES_CONTROL_WORD_INTERRUPT_QUEUE_STATUS),             16,    AG_ND_REG_TYPE_RO,    ag_nd_bit_ro_reg16,      AG_ND_REG_FMT_GLOBAL,        0x000007ff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },

#ifndef CES16_BCM_VERSION
/*/#ifdef AG_ND_PTP_SUPPORT */
{ AG_REG_ADDR_NAME (AG_REG_RECEIVE_PTP_CHANNEL_CONFIGURATION(0)),                16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_CHANNEL,       0x0000c000, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_POLICY_PTP(0)),                                       16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x0000ffff, 0x00000000, 0x00000000, 2,    AG_ND_ARCH_MASK_NEMO    },
/*/#endif */
#endif

{ AG_REG_ADDR_NAME (AG_REG_PRIORITIZED_COUNTER_SELECTION_MASK),                  16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x00000fff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },               
{ AG_REG_ADDR_NAME (AG_REG_GLOBAL_COUNTER_POLICY_SELECTION),                     16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x0000ffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },               
{ AG_REG_ADDR_NAME (AG_REG_POLICY_STATUS_POLARITY(0)),                           16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x0000ffff, 0x00000000, 0x00000000, 2,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_POLICY_DROP_UNCONDITIONAL(0)),                        16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x0000ffff, 0x00000000, 0x00000000, 2,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_POLICY_DROP_IF_NOT_FORWARD(0)),                       16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x0000ffff, 0x00000000, 0x00000000, 2,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_POLICY_FORWARD_HIGH(0)),                              16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x0000ffff, 0x00000000, 0x00000000, 2,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_POLICY_FORWARD_LOW(0)),                               16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x0000ffff, 0x00000000, 0x00000000, 2,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_POLICY_PER_CHANNEL_COUNTERS(0)),                      16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x0000ffff, 0x00000000, 0x00000000, 8,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_POLICY_GLOBAL_COUNTERS(0)),                           16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x0000ffff, 0x00000000, 0x00000000, 16,   AG_ND_ARCH_MASK_NN      },

{ AG_REG_ADDR_NAME (AG_REG_CLASSIFIER_GLOBAL_PM_COUNTER(0)),                     16,    AG_ND_REG_TYPE_RCLR,  ag_nd_bit_rclr_reg16,    AG_ND_REG_FMT_GLOBAL,        0x0000ffff, 0x00000000, 0x00000000, 4,    AG_ND_ARCH_MASK_NN      },               
{ AG_REG_ADDR_NAME (AG_REG_CLASSIFIER_CHANNEL_SPECIFIC_PM_COUNTER(0,0)),         16,    AG_ND_REG_TYPE_RCLR,  ag_nd_bit_rclr_reg16,    AG_ND_REG_FMT_CHANNEL,       0x0000ffff, 0x00000000, 0x00000000, 4,    AG_ND_ARCH_MASK_NN      },               
                                                                                      
/*                                                                                     */
/* TPP                                                                                 */
/*                                                                                     */
{ AG_REG_ADDR_NAME (AG_REG_TPP_CHANNEL_STATUS(0)),                               16,    AG_ND_REG_TYPE_W1CLR, ag_nd_bit_rw1clr_reg16,  AG_ND_REG_FMT_GLOBAL,        0x0000ffff, 0x0000ffff, 0x00000000, 6,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_TPP_CONFIG_TIMESTAMP_RATE),                           16,    AG_ND_REG_TYPE_W1CLR, ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x0000f000, 0x0000f000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEPTUNE },
{ AG_REG_ADDR_NAME (AG_REG_TPP_CONFIG_TIMESTAMP_RATE),                           16,    AG_ND_REG_TYPE_W1CLR, ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x0000c000, 0x0000c000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_CLK_SOURCE_SELECTION(0)),                             16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_CHANNEL,       0x000000ff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      }, 
{ AG_REG_ADDR_NAME (AG_REG_TIMING_CHANNEL_CONFIGURATION(0)),                     16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_CHANNEL,       0x000007ff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      }, 
{ AG_REG_ADDR_NAME (AG_REG_PHASE_SLOPE_LIMIT_THRESHOLD(0)),                      16,    AG_ND_REG_TYPE_RO,    ag_nd_bit_ro_reg16,      AG_ND_REG_FMT_CHANNEL,       0x0000ffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      }, 
{ AG_REG_ADDR_NAME (AG_REG_PHASE_SLOPE_LIMIT_PM_COUNTER(0)),                     16,    AG_ND_REG_TYPE_RCLR,  ag_nd_bit_rclr_reg16,    AG_ND_REG_FMT_CHANNEL,       0x0000ffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_PEAK_DETECTOR_PM_COUNTER(0)),                         16,    AG_ND_REG_TYPE_RCLR,  ag_nd_bit_rclr_reg16,    AG_ND_REG_FMT_CHANNEL,       0x0000ffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      }, 
{ AG_REG_ADDR_NAME (AG_REG_ARRIVAL_TIMESTAMP_BASE_1(0)),                         16,    AG_ND_REG_TYPE_RO,    ag_nd_bit_ro_reg16,      AG_ND_REG_FMT_CHANNEL,       0x0000ffff, 0x00000000, 0x00000000, 2,    AG_ND_ARCH_MASK_NN      }, 
{ AG_REG_ADDR_NAME (AG_REG_ARRIVAL_TIMESTAMP_SIGMA_1(0)),                        16,    AG_ND_REG_TYPE_RO,    ag_nd_bit_ro_reg16,      AG_ND_REG_FMT_CHANNEL,       0x0000ffff, 0x00000000, 0x00000000, 2,    AG_ND_ARCH_MASK_NN      },  
{ AG_REG_ADDR_NAME (AG_REG_ARRIVAL_TIMESTAMP_COUNT(0)),                          16,    AG_ND_REG_TYPE_RO,    ag_nd_bit_ro_reg16,      AG_ND_REG_FMT_CHANNEL,       0x0000ffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      }, 
{ AG_REG_ADDR_NAME (AG_REG_SQN_FILLER_SIGMA_1(0)),                               16,    AG_ND_REG_TYPE_RO,    ag_nd_bit_ro_reg16,      AG_ND_REG_FMT_CHANNEL,       0x0000ffff, 0x00000000, 0x00000000, 2,    AG_ND_ARCH_MASK_NN      }, 
{ AG_REG_ADDR_NAME (AG_REG_JITTER_BUFFER_DEPTH_PACKET_COUNT(0)),                 16,    AG_ND_REG_TYPE_RCLR,  ag_nd_bit_rclr_reg16,    AG_ND_REG_FMT_CHANNEL,       0x0000ffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      }, 
{ AG_REG_ADDR_NAME (AG_REG_JITTER_BUFFER_DEPTH_ACCUMULATION_1(0)),               16,    AG_ND_REG_TYPE_RO,    ag_nd_bit_ro_reg16,      AG_ND_REG_FMT_CHANNEL,       0x0000ffff, 0x00000000, 0x00000000, 2,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_TIMING_INTERFACE_JITTER_BUFFER_MAXIMUM_DEPTH_1(0)),   16,    AG_ND_REG_TYPE_RCLR,  ag_nd_bit_rclr_reg16,    AG_ND_REG_FMT_CHANNEL,       0x0000ffff, 0x00000000, 0x00000000, 2,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_RECEIVED_PACKET_RTP_TIMESTAMP_1(0)),                  16,    AG_ND_REG_TYPE_RO,    ag_nd_bit_ro_reg16,      AG_ND_REG_FMT_CHANNEL,       0x0000ffff, 0x00000000, 0x00000000, 2,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_125MHZ_ORIGINATED_TPP_TIMESTAMP_1),                   16,    AG_ND_REG_TYPE_RO,    ag_nd_bit_ro_reg16,      AG_ND_REG_FMT_GLOBAL,        0x0000ffff, 0x00000000, 0x00000000, 2,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_77MHZ_ORIGINATED_TPP_TIMESTAMP_1),                    16,    AG_ND_REG_TYPE_RO,    ag_nd_bit_ro_reg16,      AG_ND_REG_FMT_GLOBAL,        0x0000ffff, 0x00000000, 0x00000000, 2,    AG_ND_ARCH_MASK_NN      },

/* */
/* PTP */
/*  */
#ifndef CES16_BCM_VERSION
/*/#ifdef AG_ND_PTP_SUPPORT */
{ AG_REG_ADDR_NAME (AG_REG_PTP_TIMESTAMP_GENERATORS_CONFIGURATION),              16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x0000e000, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PTP_TIMESTAMP_CLOCK_SOURCE_SELECTION(0)),             16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_PTP_IDX,       0x000000ff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PTP_TIMESTAMP_STEP_SIZE_INTEGER),                     16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x000003ff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PTP_TIMESTAMP_STEP_SIZE_FRACTION(0)),                 16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x0000007f, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PTP_TIMESTAMP_STEP_SIZE_FRACTION(1)),                 16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x0000ffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PTP_TIMESTAMP_FAST_PHASE_CONFIGURATION),              16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x00000fff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PTP_TIMESTAMP_ERROR_CORRECTION_PERIOD),               16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x0000ffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PTP_TIMESTAMP_CORRECTION_VALUE(0,0)),                 16,    AG_ND_REG_TYPE_WO,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_PTP_IDX,       0x0000ffff, 0x00000000, 0x00000000, 5,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PTP_BAUD_RATE_GENERATOR(0,0)),                        16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_PTP_IDX,       0x0000ffff, 0x00000000, 0x00000000, 2,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PTP_BAUD_RATE_GENERATOR_CORRECTION(0)),               16,    AG_ND_REG_TYPE_WO,    NULL,                    AG_ND_REG_FMT_GLOBAL,        0x00000003, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PTP_CONFIGURATION(0)),                                16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_CHANNEL,       0x0000fc07, 0x00000006, 0x00000006, 1,    AG_ND_ARCH_MASK_NEMO    },
{ AG_REG_ADDR_NAME (AG_REG_PTP_FILTER),                                          16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x000000fc, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NEMO    },
/*/#endif */

{ AG_REG_ADDR_NAME (AG_REG_HOST_PACKET_LINK_CONFIGURATION),                      16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x0000efff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_RECEIVED_HOST_PACKET_DROP_COUNT),                     16,    AG_ND_REG_TYPE_RCLR,  ag_nd_bit_rclr_reg16,    AG_ND_REG_FMT_GLOBAL,        0x0000ffff, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_PM_EXPORT_STRUCTURE_ASSEMBLY_REGISTER(0)),            16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_GLOBAL,        0x0000cf97, 0x00000000, 0x00000000, 3,    AG_ND_ARCH_MASK_NN      },
{ AG_REG_ADDR_NAME (AG_REG_PM_COUNTER_EXPORT_CHANNEL_SELECTION(0)),              16,    AG_ND_REG_TYPE_RW,    ag_nd_bit_rw_reg16,      AG_ND_REG_FMT_CHANNEL,       0x0000e000, 0x00000000, 0x00000000, 1,    AG_ND_ARCH_MASK_NN      },
                                                                                                                                                                   
#endif /*CES16_BCM_VERSION*/
{ (AG_U32)-1,                                                                    0,     0,                    0,                       0,                           0,          0,          0,          0,    0                       },

};


void    
ag_nd_reg_read(AgNdDevice *p_device, AG_U32 n_offset, AG_U16 *p_data)
{
    assert(p_device);           
    assert(p_data);
    assert(n_offset < 0x800000);

    ag_nd_hw_kiss(p_device, AG_ND_HW_ACCESS_READ, n_offset, p_data);
}



void    
ag_nd_reg_write(AgNdDevice *p_device, AG_U32 n_offset, AG_U16 n_data)
{
    assert(p_device);
    assert(n_offset < 0x800000);

    ag_nd_hw_kiss(p_device, AG_ND_HW_ACCESS_WRITE, n_offset, &n_data);
}


void
ag_nd_reg_read_32bit(AgNdDevice *p_device, AG_U32 n_offset, AG_U32 *n_32bit_data)
{
    AG_U16 n_16bit_data;

    ag_nd_reg_read(p_device, n_offset, &n_16bit_data);
    *n_32bit_data = (AG_U32)n_16bit_data;
    ag_nd_reg_read(p_device, n_offset + 2, &n_16bit_data);
    *n_32bit_data = (*n_32bit_data << 16) | (AG_U32)n_16bit_data;
}


void
ag_nd_reg_write_32bit(AgNdDevice *p_device, AG_U32 n_offset, AG_U32 n_32bit_data)
{
    AG_U16 n_16bit_data;

    n_16bit_data = (AG_U16)(n_32bit_data >> 16);
    ag_nd_reg_write(p_device, n_offset, n_16bit_data);
    n_16bit_data = (AG_U16)(n_32bit_data & 0xFFFF);
    ag_nd_reg_write(p_device, n_offset + 2, n_16bit_data);
}

#ifdef CES16_BCM_VERSION
unsigned int ces16Debug_bank = 0x1000;   /*> 0x100, do not trace, else */
#endif
void    
ag_nd_mem_read(AgNdDevice *p_device, AgNdMemUnit *p_mu, AG_U32 n_offset, void *p_data, AG_U32 n_word_count)
{  
    AG_U16 n_rdata, *p16_data = (AG_U16 *)p_data;
    AgNdRegExtendedAddress x_eaddr;
    /*    AgNdRegExtendedAddress x_eaddrtemp;*/
    assert(p_device);
    assert(p_data);
    assert(p_mu);

    assert(0 == (n_offset & 1));
    assert(n_offset < p_mu->n_size);

    n_offset += p_mu->n_start;
    n_offset |= 0x800000;

    x_eaddr.n_reg = 0;
    x_eaddr.x_fields.b_internal_memory = (field_t) p_mu->b_internal;
    x_eaddr.x_fields.n_bank = (field_t) p_mu->n_bank;
    ag_nd_reg_write(p_device, AG_REG_EXTENDED_ADDRESS, x_eaddr.n_reg);
    p_device->n_current_bank_pointer = p_mu->n_bank; /*BCM: 22.9.11*/


    #ifdef CES16_BCM_VERSION
    if(ces16Debug_bank == x_eaddr.x_fields.n_bank)
        AG_ND_TRACE(p_device, 0xffffffff, AG_ND_TRACE_DEBUG, "w: offset=0x%06lX count=0x%04hX\n", n_offset, n_word_count);
    #endif
    for (; n_word_count--; n_offset += 2)
    {
        ag_nd_hw_kiss(p_device, AG_ND_HW_ACCESS_READ, n_offset, &n_rdata);
        *p16_data++ = soc_htons(n_rdata);  /*BCM write word, need to take care about Endianess*/
    }
}


void    
ag_nd_mem_write(AgNdDevice *p_device, AgNdMemUnit *p_mu, AG_U32 n_offset, void *p_data, AG_U32 n_word_count)
{  
    AG_U16 n_wdata, *p16_data = (AG_U16 *)p_data;
    AgNdRegExtendedAddress x_eaddr;

    assert(p_device);
    assert(p_data);
    assert(p_mu);

    assert(0 == (n_offset & 1));
    assert(n_offset < p_mu->n_size);

    n_offset += p_mu->n_start;
    n_offset |= 0x800000;

    x_eaddr.n_reg = 0;
    x_eaddr.x_fields.b_internal_memory = (field_t) p_mu->b_internal;
    x_eaddr.x_fields.n_bank = (field_t) p_mu->n_bank;
    
    {   /*BCMprotect*/
    
     /*int mask; */
     /*mask = agos_disable_interrupts(); */
    
    ag_nd_reg_write(p_device, AG_REG_EXTENDED_ADDRESS, x_eaddr.n_reg);
    p_device->n_current_bank_pointer = p_mu->n_bank; /*BCM: 22.9.11*/

   
    #ifdef CES16_BCM_VERSION
    if(ces16Debug_bank == x_eaddr.x_fields.n_bank)
      AG_ND_TRACE(p_device, 0xffffffff, AG_ND_TRACE_DEBUG, "w: offset=0x%06lX count=0x%04hX\n", n_offset, n_word_count);
    #endif

    for (; n_word_count--; n_offset += 2)
    {
	n_wdata = soc_htons(*p16_data++);
        ag_nd_hw_kiss(p_device, AG_ND_HW_ACCESS_WRITE, n_offset, &n_wdata);
      
     }

    }
      /*agos_enable_interrupts(mask); */
}


void    
ag_nd_mem_unit_set(AgNdDevice *p_device, AgNdMemUnit *p_mu)
{
    AgNdRegExtendedAddress x_eaddr;

    assert(p_device);
    assert(p_mu);
    /*fprintf(stderr,"\n\r%s: internal=%d,bank=%d,start=%d,size=%d\n\r",p_mu->a_name,p_mu->b_internal,p_mu->n_bank,p_mu->n_start,p_mu->n_size);*/
    x_eaddr.n_reg = 0;
    x_eaddr.x_fields.b_internal_memory = (field_t) p_mu->b_internal;
    x_eaddr.x_fields.n_bank = (field_t) p_mu->n_bank;
    ag_nd_reg_write(p_device, AG_REG_EXTENDED_ADDRESS, x_eaddr.n_reg);
    p_device->n_current_bank_pointer = p_mu->n_bank; /*BCM: 22.9.11*/
}


#define AG_ND_VERIFY_LOOP 50
AG_U32 ag_mem_access_work_around = 1;
AG_U32 ag_mem_access_read_errs = 0;
void 
ag_nd_hw_kiss(AgNdDevice    *p_device,
              AgNdHwAccess  e_action,
              AG_U32        n_offset,
              AG_U16        *p_data)
{
    AG_U32      n_addr;
   #ifdef AG_ND_ENABLE_TRACE
    AG_BOOL     b_trace = AG_FALSE;
    AG_S16      i;
   #endif
    uint32 tempData,verifyData,verifyLoop;
    AG_U32      memory_verify = 0; /*if 0, do not verify "read" memory access*/
    int rv;

    assert(p_device);
    assert(p_data);
    assert(!(n_offset & 1));
    assert(!((AG_U32)p_data & 1));

    /* soc_cm_print("n_offset:0x%08lx\n", n_offset); */

    /*  */
    /* perform r/w */
    /* */
    
    if (n_offset & 0x800000)
    {   /*THis is a memory access*/
        n_offset &= ~(0x800000);
        n_offset |= 0x40000;
        
        /*BCM:21.9.11*/              
        switch(p_device->n_current_bank_pointer)
        {
        case 0: /*Micro code*/
        case 1: /*Classification table*/
        case 2: /*Straict check table*/
               memory_verify = 1; /*Verify memory access*/
               break; 
        case 4:
                /*Payload buffer           40000-4ffff */
                /*Header template memory   50000-57fff */
                if(n_offset >= 0x50000)
                   memory_verify = 1; /*Verify memory access only to header template*/
                break; 

        case 5: /*Jitter buffer memory, do not verify*/
        default:
                 break;
        }
    }
    n_addr = (n_offset | p_device->n_base);

    
    if (((n_addr & ~0x40000) & ~0x40280000) > 0x3fc4a) {
	soc_cm_print("%s: Bad offset:0x%08lx\n", __func__, n_addr);
	return;
    }

    if (AG_ND_HW_ACCESS_READ == e_action)
    {
        p_device->n_bus_access_read_count++;

        if (p_device->b_use_hw)
	    {
        rv = soc_reg32_read(0, n_addr, &tempData);
        if (rv < 0) {
            soc_cm_print("\n\rnd_kiss soc read failed");
            return;
        }
        if(ag_mem_access_work_around && memory_verify)
        {
          /*Verify read*/
           for(verifyLoop=0; verifyLoop<AG_ND_VERIFY_LOOP;verifyLoop++)
           {
            rv = soc_reg32_read(0, n_addr, &verifyData);
            if (rv < 0) {
                soc_cm_print("\n\rnd_kiss soc read failed");
                return;
            }
            if(verifyData == tempData)
               break;  /*Found same value twice*/
            ag_mem_access_read_errs++;
            tempData = verifyData;
           }
           if(verifyLoop >= AG_ND_VERIFY_LOOP)
             soc_cm_print("\n\rnd_kiss failed to read memory");
        }
        *p_data = (AG_U16)tempData;

	    }/*Hw*/
    }/*Read*/

    else if (AG_ND_HW_ACCESS_WRITE == e_action)
    {
        p_device->n_bus_access_write_count++;
        if (p_device->b_use_hw)
	    {
	      rv = soc_reg32_write(0, n_addr, (AG_U32)*p_data);
              if (rv < 0) {
                  soc_cm_print("\n\rnd_kiss soc write failed");
                  return;
              }
	    }
    }/*Write*/


#ifdef AG_ND_ENABLE_TRACE
    /* */
    /* bus monitoring */
    /*  */
    b_trace = AG_FALSE;

    if (p_device->b_trace)
    {
        if (g_ndTraceMask & AG_ND_TRACE_BUS_ALL)
            b_trace = AG_TRUE;
        else 
            if (g_ndTraceMask  & AG_ND_TRACE_BUS_SPECIFIC)
                for (i = 0; i < g_ndTraceBusAddrSize; i++)
                    if (g_ndTraceBusAddr[i] == n_addr)
                        b_trace = AG_TRUE;      
    }

    if (b_trace)
    {
      /*        AG_U16 level1 = AG_ND_TRACE_DEBUG; */
        if (AG_ND_HW_ACCESS_WRITE == e_action)
            AG_ND_TRACE(p_device, 0xffffffff, level1, "w: addr=0x%06lX data=0x%04hX\n", n_addr, *p_data);

        if (AG_ND_HW_ACCESS_READ == e_action)
            AG_ND_TRACE(p_device, 0xffffffff, AG_ND_TRACE_DEBUG, "r: addr=0x%06lX data=0x%04hX\n", n_addr, *p_data);
    }
#endif /* AG_ND_ENABLE_TRACE  */
#if 0
    g_force_kiss_out = 1;
    if (g_force_kiss_out) /*BCM*/
    {
      if (AG_ND_HW_ACCESS_WRITE == e_action)
          soc_cm_print("w: addr=0x%06lX data=0x%04hX\n", n_addr, *p_data);

      if (AG_ND_HW_ACCESS_READ == e_action)
          soc_cm_print("r: addr=0x%06lX data=0x%04hX\n", n_addr, *p_data);
    }
#endif
}

/********************************************************************************/
/* Enumerate channel/circuit ids */
/* */
/* Returns (AG_U32)-1 in the case given chid/cid reached the maximum value */
/* */
/* Refer to the Nemo/Neptune spec: channel/circuit id interpretation and register address format */
/* Remember that the port(nemo) and channel id is just a running indexes, and the neptune */
/* circuit id is constructed from 3 indexes:  */
/*      bits 0-1: VT    0..3 */
/*      bits 2-4: VTG   0..6 */
/*      bits 5-6: SPE   0..3 */
/* */
AG_U32
ag_nd_next_reg_id(AgNdDevice *p_device, AgNdRegProperties *p_reg, AG_U32 current_id)
{
    assert(p_device);
    assert(p_reg);



    switch (p_reg->e_format)
    {

    case AG_ND_REG_FMT_PTP_IDX:

        if (current_id >= AG_ND_REG_FMT_PTP_IDX - 1)
            return (AG_U32)-1;

        return current_id + 1;



    case AG_ND_REG_FMT_CHANNEL:

        if (current_id >= p_device->n_total_channels - 1)
            return (AG_U32)-1;

        return current_id + 1;



    case AG_ND_REG_FMT_CIRCUIT:

        if (AG_ND_ARCH_MASK_NEMO == p_device->n_chip_mask)
        {
            if (current_id >= p_device->n_total_ports - 1)
                return (AG_U32)-1;
            
            return current_id + 1;
        }
        else
        {
            current_id++;

            /* */
            /* if VTG index reached 7, move to next SPE */
            /*  */
            if ((current_id & 0x1c) == 0x1c)
                current_id += 0x4;

            /* */
            /* if SPE index reached 4, we done */
            /* */
            if ((current_id & 0x60) == 0x60)
                return (AG_U32)-1;

            return current_id;
        }


    default:

        return (AG_U32)-1;
    }
}


/********************************************************************************/
/* sets all registers to default value */
/* */
void
ag_nd_regs_reset(AgNdDevice *p_device)
{
    AG_S32 i = 0;
    AG_U32 n_id = 0; /* circuit/channel id */


    assert(p_device);


    for (i = 0; (AG_U32)-1 != g_ndRegTable[i].n_address; i++)
    {
        if (!(g_ndRegTable[i].n_chip & p_device->n_chip_mask)) 
            continue;


        n_id = 0;

        do
        {
            if (g_ndRegTable[i].test)
                g_ndRegTable[i].test(
                    p_device, 
                    &(g_ndRegTable[i]), 
                    n_id << 12, 
                    AG_FALSE);  /* reset only */

            n_id = ag_nd_next_reg_id(p_device, &(g_ndRegTable[i]), n_id);
        }
        while (n_id != (AG_U32)-1);
    }

}

/*******************************************************************************/
/* ag_nd_get_ts_info */
/* */
AgNdTsInfo*
ag_nd_get_ts_info(
    AgNdDevice  *p_device,
    AgNdCircuit n_circuit_id,
    AG_U32      n_ts_idx,
    AgNdPath    e_path)
{
    AG_U32  n_circuit_idx;

    n_circuit_idx = p_device->p_circuit_id_to_idx(n_circuit_id);

    return & ( p_device->a_ts_info_table[n_circuit_idx][n_ts_idx][e_path] );
}
/*BCM-CLS: add functions to read/write lstree memory without assuming memory bank was already set*/
void    
ag_nd_lstree_mem_read(AgNdDevice *p_device, AG_U32 n_offset, void *p_data, AG_U32 n_word_count)
{  
    AG_U16 n_rdata, *p16_data = (AG_U16 *)p_data;
    AgNdMemUnit *p_mu;

    assert(p_device);
    assert(p_data);

    assert(0 == (n_offset & 1));

    p_mu = p_device->p_mem_lstree;

    assert(p_mu);
    assert(n_offset < p_mu->n_size);

    n_offset += p_mu->n_start;
    n_offset |= 0x800000;


    for (; n_word_count--; n_offset += 2)
    {
        ag_nd_hw_kiss(p_device, AG_ND_HW_ACCESS_READ, n_offset, &n_rdata);
        *p16_data++ = soc_htons(n_rdata);  /*BCM write word, need to take care about Endianess*/
    }
}


void    
ag_nd_lstree_mem_write(AgNdDevice *p_device, AG_U32 n_offset, void *p_data, AG_U32 n_word_count)
{  
    AG_U16  n_wdata, *p16_data = (AG_U16 *)p_data;
    AgNdMemUnit *p_mu;
    assert(p_device);
    assert(p_data);

    assert(0 == (n_offset & 1));
    
    p_mu = p_device->p_mem_lstree;
    assert(p_mu);
    assert(n_offset < p_mu->n_size);

    n_offset += p_mu->n_start;
    n_offset |= 0x800000;

    
    {   /*BCMprotect*/
    
     /*int mask; */
     /*mask = agos_disable_interrupts(); */
       

    for (; n_word_count--; n_offset += 2)
    {
        n_wdata = soc_htons(*p16_data++);
        ag_nd_hw_kiss(p_device, AG_ND_HW_ACCESS_WRITE, n_offset, &n_wdata);
        
     }

    }
      /*agos_enable_interrupts(mask); */
}
