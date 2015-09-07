/*
 * $Id: chip.h 1.65 Broadcom SDK $
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
 * File:        chip.h
 * Purpose:     Defines for chip types, etc.
 * Requires:    soctypes.h, memory, register and feature defs.
 *
 * System on Chip (SOC) basic structures and typedefs
 * Each chip consists of a number of blocks.  Each block can be
 * a port interface controller (PIC) that contains ports.
 * The mcm/bcm*.i files contain definitions of each block and port
 * in the associated chip.  The structures used in those files
 * are defined here.  They are used to build internal use data
 * structures defined in soc/drv.h
 */

#ifndef _SOC_CHIP_H
#define _SOC_CHIP_H

#include <soc/types.h>
#include <soc/memory.h>
#include <soc/register.h>
#include <soc/feature.h>

/*
 * Arrays of soc_block_info_t are built by the registers program.
 * Each array is indexed by block number (per chip).
 * The last entry has type -1.
 * Used in bcm*.i files.
 */
typedef struct {
    int    type;    /* SOC_BLK_* */
    int    number;  /* instance of type */
    int    schan;   /* pic number for schan commands */
    int    cmic;    /* pic number for cmic r/w commands */
} soc_block_info_t;

/*
 * Arrays of soc_port_info_t are built by the registers program.
 * Each array is indexed by port number (per chip).
 * Unused ports have blk -1 and bindex 0
 * The last entry has blk -1 and bindex -1
 * Used in bcm*.i files.
 */
typedef struct {
    int     blk;    /* index into soc_block_info array */
    int     bindex; /* index of port within block */
} soc_port_info_t;

/*
 * Block types
 */
typedef enum soc_block_type_e {
    SOC_BLK_NONE=0,
    SOC_BLK_EPIC,
    SOC_BLK_GPIC,
    SOC_BLK_HPIC,
    SOC_BLK_IPIC,
    SOC_BLK_XPIC,
    SOC_BLK_CMIC,
    SOC_BLK_CPIC,
    SOC_BLK_ARL,
    SOC_BLK_MMU,
    SOC_BLK_MCU, /* 10 */
    SOC_BLK_GPORT,
    SOC_BLK_XPORT,
    SOC_BLK_IPIPE,
    SOC_BLK_IPIPE_HI,
    SOC_BLK_EPIPE,
    SOC_BLK_EPIPE_HI,
    SOC_BLK_IGR,
    SOC_BLK_EGR,
    SOC_BLK_BSE,
    SOC_BLK_CSE, /* 20 */
    SOC_BLK_HSE,
    SOC_BLK_BSAFE,
    SOC_BLK_GXPORT,
    SOC_BLK_SPI,
    SOC_BLK_EXP,
    SOC_BLK_SYS,
    SOC_BLK_XGPORT,
    SOC_BLK_SPORT,
    SOC_BLK_INTER,
    SOC_BLK_EXTER, /* 30 */
    SOC_BLK_ESM,
    SOC_BLK_OTPC,
    SOC_BLK_QGPORT,
    SOC_BLK_XQPORT,
    SOC_BLK_XLPORT,
    SOC_BLK_LBPORT,
    SOC_BLK_PORT_GROUP4,
    SOC_BLK_PORT_GROUP5,
    SOC_BLK_PGW_CL,
    SOC_BLK_CPORT, /* 40 */
    SOC_BLK_CLPORT,
    SOC_BLK_XTPORT,
    SOC_BLK_MXQPORT,
    SOC_BLK_TOP,
    SOC_BLK_SER,
    SOC_BLK_AXP,
    SOC_BLK_ISM,
    SOC_BLK_ETU,
    SOC_BLK_IBOD,
    SOC_BLK_LLS, /* 50 */
    SOC_BLK_CES,
    SOC_BLK_CI,
    SOC_BLK_TXLP,
    SOC_BLK_RXLP,
    SOC_BLK_IL,
    SOC_BLK_MS_ISEC,
    SOC_BLK_MS_ESEC,
    /* Sirius regs */
    SOC_BLK_BP,
    SOC_BLK_CS,
    SOC_BLK_EB,  /* 60 */
    SOC_BLK_EP,
    SOC_BLK_ES,
    SOC_BLK_FD,
    SOC_BLK_FF,
    SOC_BLK_FR, 
    SOC_BLK_TX, 
    SOC_BLK_QMA,
    SOC_BLK_QMB,
    SOC_BLK_QMC, 
    SOC_BLK_QSA,  /* 70 */
    SOC_BLK_QSB,
    SOC_BLK_RB,
    SOC_BLK_SC_TOP,
    SOC_BLK_SF_TOP,
    SOC_BLK_TS, 
    SOC_BLK_CW,
    /* Caladan blocks */
    SOC_BLK_CM,
    SOC_BLK_CO,
    SOC_BLK_CX,
    SOC_BLK_ETU_WRAP,  /* 80 */
    SOC_BLK_LRA,
    SOC_BLK_LRB,
    SOC_BLK_OC,
    SOC_BLK_PB,
    SOC_BLK_PD,
    SOC_BLK_PP,
    SOC_BLK_PR,
    SOC_BLK_PT,
    SOC_BLK_QM,
    SOC_BLK_RC,  /* 90 */
    SOC_BLK_TMA,
    SOC_BLK_TMB,
    SOC_BLK_TP,
    SOC_BLK_TM_QE,
    /* Helix4 */
    SOC_BLK_XWPORT,
    SOC_BLK_IPROC,
    /* Composites: Keeping these at the end */
    SOC_BLK_PORT,
    SOC_BLK_CPU,
    SOC_BLK_ETHER,
    SOC_BLK_HIGIG,
    SOC_BLK_FABRIC,
    SOC_BLK_NET,
    SOC_BLK_HGPORT,
    SOC_BLK_SBX_PORT,
    
    SOC_BLK_FIRST_DPP = 10000,
    /* Petra-B block info */
    SOC_BLK_CFC = SOC_BLK_FIRST_DPP,
    SOC_BLK_DPI,
    SOC_BLK_DRC,
    SOC_BLK_ECI,
    SOC_BLK_EGQ,
    SOC_BLK_EPNI,
    SOC_BLK_FCR,
    SOC_BLK_FCT,
    /* SOC_BLK_MMU, */
    SOC_BLK_FDR,
    SOC_BLK_FDT,
    SOC_BLK_MESH_TOPOLOGY,
    SOC_BLK_IDR,
    SOC_BLK_IHB,
    SOC_BLK_IHP,
    SOC_BLK_IPS,
    SOC_BLK_IPT,
    SOC_BLK_IQM,
    SOC_BLK_IRE,
    SOC_BLK_IRR,
    SOC_BLK_MAC,
    SOC_BLK_MBU,
    SOC_BLK_MCC,
    SOC_BLK_MSW,
    SOC_BLK_NBI,
    SOC_BLK_NIF,
    SOC_BLK_NIF_MAL,
    SOC_BLK_OLP,
    SOC_BLK_QDR,
    SOC_BLK_RTP,
    SOC_BLK_SCH,

    /* ARAD */
    /*SOC_BLK_CFC,*/
    SOC_BLK_OCB,
    SOC_BLK_CRPS,
    /*SOC_BLK_ECI,*/
    /*SOC_BLK_EGQ,*/  
    /*SOC_BLK_CMIC,*/
    /*SOC_BLK_FCR,*/
    /*SOC_BLK_FCT,*/
    /*SOC_BLK_MMU,*/
    /*SOC_BLK_FDR,*/
    /*SOC_BLK_FDT,*/
    /*SOC_BLK_MESH_TOPOLOGY,*/
    /*SOC_BLK_IDR,*/
    /*SOC_BLK_IHB,*/
    /*SOC_BLK_IHP,*/
    /*SOC_BLK_IPS,*/
    /*SOC_BLK_IPT,*/
    /*SOC_BLK_IQM,*/
    /*SOC_BLK_IRE,*/
    /*SOC_BLK_IRR,*/
    SOC_BLK_FMAC,
    /*SOC_BLK_OTPC,*/
    SOC_BLK_XLP,
    SOC_BLK_CLP,
    /*SOC_BLK_NBI,*/
    SOC_BLK_CGM,
    SOC_BLK_OAMP,
    /*SOC_BLK_OLP,*/
    SOC_BLK_FSRD,
    /*SOC_BLK_RTP,*/
    /*SOC_BLK_SCH,*/
    /*SOC_BLK_EPNI,*/
    SOC_BLK_DRCA,
    SOC_BLK_DRCB,
    SOC_BLK_DRCC,
    SOC_BLK_DRCD,
    SOC_BLK_DRCE,
    SOC_BLK_DRCF,
    SOC_BLK_DRCG,
    SOC_BLK_DRCH,
    SOC_BLK_BRDC_FMAC,
    SOC_BLK_BRDC_FSRD,
    SOC_BLK_DRCBROADCAST,
    /* JERICHO-2-P3 */
    SOC_BLK_CFG, 
    SOC_BLK_EM,
    SOC_BLK_TABLE,
    SOC_BLK_TBL,
    SOC_BLK_TCAM,
    /*FE1600*/
    /*SOC_BLK_ECI,*/
    SOC_BLK_OCCG,
    SOC_BLK_DCH,
    /*SOC_BLK_CMIC,*/
    SOC_BLK_DCL,
    SOC_BLK_DCMA,
    SOC_BLK_DCMB,
    SOC_BLK_DCMC,
    SOC_BLK_CCS,
    /*SOC_BLK_RTP,*/
    /*SOC_BLK_MESH_TOPOLOGY,*/
    /*SOC_BLK_FMAC,*/
    /*SOC_BLK_FSRD,*/
    /*SOC_BLK_OTPC,*/
    /*SOC_BLK_BRDC_FSRD,*/
    SOC_BLK_BRDC_FMACH,
    SOC_BLK_BRDC_FMACL,
    SOC_BLK_LAST_DPP = SOC_BLK_BRDC_FMACL,

    SOC_BLOCK_TYPE_INVALID = -1
} soc_block_type_t;

#define SOC_BLK_PORT_INITIALIZER \
    SOC_BLK_EPIC,\
    SOC_BLK_GPIC,\
    SOC_BLK_HPIC,\
    SOC_BLK_IPIC,\
    SOC_BLK_XPIC,\
    SOC_BLK_GPORT,\
    SOC_BLK_XPORT,\
    SOC_BLK_GXPORT,\
    SOC_BLK_XGPORT,\
    SOC_BLK_QGPORT,\
    SOC_BLK_SPORT,\
    SOC_BLK_XLPORT,\
    SOC_BLK_MXQPORT,\
    SOC_BLK_XQPORT,\
    SOC_BLK_CPIC,\
    SOC_BLK_CPORT,\
    SOC_BLK_CLPORT,\
    SOC_BLK_XTPORT,\
    SOC_BLK_XWPORT,\
    SOC_BLK_AXP,\
    SOC_BLK_MXQPORT,\
    SOC_BLK_XLP,\
    SOC_BLK_CLP,\
    SOC_BLK_PGW_CL, \
    SOC_BLK_RXLP,\
    SOC_BLK_TXLP,\
    SOC_BLOCK_TYPE_INVALID

#define SOC_BLK_CPU_INITIALIZER \
    SOC_BLK_CMIC,\
    SOC_BLK_CPIC,\
    SOC_BLOCK_TYPE_INVALID\

#define SOC_BLK_ETHER_INITIALIZER \
    SOC_BLK_EPIC,\
    SOC_BLK_GPIC,\
    SOC_BLK_XPIC,\
    SOC_BLK_GPORT,\
    SOC_BLK_XPORT,\
    SOC_BLK_GXPORT,\
    SOC_BLK_XGPORT,\
    SOC_BLK_XQPORT,\
    SOC_BLK_QGPORT,\
    SOC_BLK_SPORT,\
    SOC_BLK_XLPORT,\
    SOC_BLK_MXQPORT,\
    SOC_BLK_CPORT,\
    SOC_BLK_CLPORT,\
    SOC_BLK_XTPORT,\
    SOC_BLK_XWPORT,\
    SOC_BLOCK_TYPE_INVALID\

#define SOC_BLK_HIGIG_INITIALIZER \
    SOC_BLK_HPIC,\
    SOC_BLK_IPIC,\
    SOC_BLK_XPORT,\
    SOC_BLK_GXPORT,\
    SOC_BLK_XGPORT,\
    SOC_BLK_XQPORT,\
    SOC_BLK_XLPORT,\
    SOC_BLK_MXQPORT,\
    SOC_BLK_QGPORT,\
    SOC_BLK_CPORT,\
    SOC_BLK_CLPORT,\
    SOC_BLK_XTPORT,\
    SOC_BLK_XWPORT,\
    SOC_BLOCK_TYPE_INVALID\

#define SOC_BLK_FABRIC_INITIALIZER \
    SOC_BLK_SC_TOP,\
    SOC_BLK_SF_TOP,\
    SOC_BLK_SPI,\
    SOC_BLOCK_TYPE_INVALID\

#define SOC_BLK_NET_INITIALIZER \
    SOC_BLK_EPIC,\
    SOC_BLK_GPIC,\
    SOC_BLK_XPIC,\
    SOC_BLK_GPORT,\
    SOC_BLK_XPORT,\
    SOC_BLK_GXPORT,\
    SOC_BLK_XGPORT,\
    SOC_BLK_XQPORT,\
    SOC_BLK_QGPORT,\
    SOC_BLK_SPORT,\
    SOC_BLK_XLPORT,\
    SOC_BLK_MXQPORT,\
    SOC_BLK_CPORT,\
    SOC_BLK_CLPORT,\
    SOC_BLK_XTPORT,\
    SOC_BLK_XWPORT,\
    SOC_BLK_HPIC,\
    SOC_BLK_IPIC,\
    SOC_BLK_SC_TOP,\
    SOC_BLK_SF_TOP,\
    SOC_BLK_SPI,\
    SOC_BLK_IL,\
    SOC_BLK_FSRD,\
    SOC_BLK_XLP,\
    SOC_BLK_CLP,\
    SOC_BLOCK_TYPE_INVALID\

#define SOC_BLK_HGPORT_INITIALIZER \
    SOC_BLK_IPIPE_HI,\
    SOC_BLOCK_TYPE_INVALID\

#define SOC_BLK_SBX_PORT_INITIALIZER \
    SOC_BLK_GXPORT,\
    SOC_BLK_CLPORT,\
    SOC_BLK_XTPORT,\
    SOC_BLK_XLPORT,\
    SOC_BLK_IL,\
    SOC_BLOCK_TYPE_INVALID\

/*
 * Naming of blocks (there are two such arrays, one for
 * block based naming and one for port based naming)
 * Last entry has blk of SOC_BLK_NONE.
 */
typedef struct {
    soc_block_t     blk;        /* block bits to match */
    int             isalias;    /* this name is an alias */
    char            *name;      /* printable name */
} soc_block_name_t;

/* used to intialize soc_block_name_t soc_block_port_names[] */
#define SOC_BLOCK_PORT_NAMES_INITIALIZER    {  \
    /*    blk  , isalias, name */      \
    { SOC_BLK_EPIC,     0,  "fe"    }, \
    { SOC_BLK_GPIC,     0,  "ge"    }, \
    { SOC_BLK_GPORT,    0,  "ge"    }, \
    { SOC_BLK_GXPORT,   0,  "hg"    }, \
    { SOC_BLK_XGPORT,   0,  "ge"    }, \
    { SOC_BLK_QGPORT,   0,  "ge"    }, \
    { SOC_BLK_XQPORT,   0,  "ge"    }, \
    { SOC_BLK_SPORT,    0,  "ge"    }, \
    { SOC_BLK_HPIC,     0,  "hg"    }, \
    { SOC_BLK_IPIC,     0,  "hg"    }, \
    { SOC_BLK_XPIC,     0,  "xe"    }, \
    { SOC_BLK_XPORT,    0,  "hg"    }, \
    { SOC_BLK_CMIC,     0,  "cpu"   }, \
    { SOC_BLK_CPIC,     0,  "cpu"   }, \
    { SOC_BLK_SPI,      0,  "spi"   }, \
    { SOC_BLK_EXP,      0,  "exp"   }, \
    { SOC_BLK_LBPORT,   0,  "lb"    }, \
    { SOC_BLK_CPU,      0,  "cpu"   }, \
    { SOC_BLK_AXP,      0,  "ax"    }, \
    { SOC_BLK_XLPORT,   0,  "xl"    }, \
    { SOC_BLK_CPORT,    0,  "ce"    }, \
    { SOC_BLK_CLPORT,   0,  "ce"    }, \
    { SOC_BLK_XTPORT,   0,  "xt"    }, \
    { SOC_BLK_MXQPORT,  0,  "mxq"   }, \
    { SOC_BLK_XWPORT,   0,  "xw"    }, \
    { SOC_BLK_MXQPORT,  0,  "mxq"    }, \
    { SOC_BLK_PGW_CL,   0,  "pgw_cl"}, \
    { SOC_BLK_NONE,     0,  ""  } }

/* used to intialize soc_block_name_t soc_block_names[] */
#define SOC_BLOCK_NAMES_INITIALIZER {  \
    /*    blk  , isalias, name */      \
    { SOC_BLK_EPIC,     0,  "epic"  }, \
    { SOC_BLK_GPIC,     0,  "gpic"  }, \
    { SOC_BLK_HPIC,     0,  "hpic"  }, \
    { SOC_BLK_IPIC,     0,  "ipic"  }, \
    { SOC_BLK_XPIC,     0,  "xpic"  }, \
    { SOC_BLK_CMIC,     0,  "cmic"  }, \
    { SOC_BLK_CPIC,     0,  "cpic"  }, \
    { SOC_BLK_ARL,      0,  "arl"   }, \
    { SOC_BLK_MMU,      0,  "mmu"   }, \
    { SOC_BLK_MCU,      0,  "mcu"   }, \
    { SOC_BLK_GPORT,    0,  "gport" }, \
    { SOC_BLK_XPORT,    0,  "xport" }, \
    { SOC_BLK_GXPORT,   0,  "gxport" }, \
    { SOC_BLK_XLPORT,   0,  "xlport" }, \
    { SOC_BLK_MXQPORT,  0,  "mxqport" }, \
    { SOC_BLK_XGPORT,   0,  "xgport" }, \
    { SOC_BLK_QGPORT,   0,  "qgport" }, \
    { SOC_BLK_XQPORT,   0,  "xqport" }, \
    { SOC_BLK_SPORT,    0,  "sport" }, \
    { SOC_BLK_IPIPE,    0,  "ipipe" }, \
    { SOC_BLK_IPIPE_HI, 0,  "ipipe_hi" }, \
    { SOC_BLK_EPIPE,    0,  "epipe" }, \
    { SOC_BLK_EPIPE_HI, 0,  "epipe_hi" }, \
    { SOC_BLK_IGR,      0,  "igr"  }, \
    { SOC_BLK_EGR,      0,  "egr"  }, \
    { SOC_BLK_BSE,      0,  "bse"  }, \
    { SOC_BLK_IL,       0,  "intl"  }, \
    { SOC_BLK_MS_ISEC,  0,  "ms_isec"  }, \
    { SOC_BLK_MS_ESEC,  0,  "ms_esec"  }, \
    { SOC_BLK_CW,       0,  "cw"  }, \
    { SOC_BLK_CSE,      0,  "cse"  }, \
    { SOC_BLK_HSE,      0,  "hse"  }, \
    { SOC_BLK_BSAFE,    0,  "bsafe" }, \
    { SOC_BLK_ESM,      0,  "esm"}, \
    { SOC_BLK_EPIC,     1,  "e"     }, \
    { SOC_BLK_GPIC,     1,  "g"     }, \
    { SOC_BLK_HPIC,     1,  "h"     }, \
    { SOC_BLK_IPIC,     1,  "i"     }, \
    { SOC_BLK_XPIC,     1,  "x"     }, \
    { SOC_BLK_CMIC,     1,  "cpu"   }, \
    { SOC_BLK_OTPC,     1,  "otpc"  }, \
    { SOC_BLK_SPI,      0,  "spi"   }, \
    { SOC_BLK_LBPORT,   0,  "lb"    }, \
    { SOC_BLK_TOP,      0,  "top"   }, \
    { SOC_BLK_SER,      0,  "ser" }, \
    { SOC_BLK_ISM,      0,  "ism"   }, \
    { SOC_BLK_ETU,      0,  "etu"   }, \
    { SOC_BLK_IBOD,     0,  "ibod"   }, \
    { SOC_BLK_AXP,      0,  "ax"    }, \
    { SOC_BLK_XLPORT,   0,  "xlport" }, \
    { SOC_BLK_CLPORT,   0,  "clport" }, \
    { SOC_BLK_CPORT,    0,  "cport" }, \
    { SOC_BLK_XTPORT,   0,  "xtport" }, \
    { SOC_BLK_XWPORT,   0,  "xwport" }, \
    { SOC_BLK_PORT_GROUP4, 0, "port_group4" }, \
    { SOC_BLK_PORT_GROUP5, 0, "port_group5" }, \
    { SOC_BLK_PGW_CL,   0,  "pgw_cl" }, \
    { SOC_BLK_CPU,      0,  "cpu"   }, \
    { SOC_BLK_CI,       0,  "ci"   }, \
    { SOC_BLK_TXLP,     0,  "txlp" }, \
    { SOC_BLK_RXLP,     0,  "rxlp" }, \
    { SOC_BLK_LLS,      0,  "lls"   }, \
    { SOC_BLK_CES,      0,  "ces"   }, \
    { SOC_BLK_MS_ISEC,  0,  "ms_isec"  }, \
    { SOC_BLK_MS_ESEC,  0,  "ms_esec"  }, \
    { SOC_BLK_PGW_CL,   0,  "pgw_cl" }, \
    { SOC_BLK_IPROC,    0,  "iproc"  }, \
    { SOC_BLK_NONE,     0,  ""      } }

/* used to intialize soc_block_name_t soc_sbx_block_port_names[] */
#define SOC_BLOCK_SBX_PORT_NAMES_INITIALIZER    {  \
    /*    blk  , isalias, name */      \
    { SOC_BLK_CMIC,     0,  "cpu"   }, \
    { SOC_BLK_GXPORT,   0,  "gxport"}, \
    { SOC_BLK_NONE,     0,  ""  } }

/* used to intialize soc_block_name_t soc_sbx_block_names[] */
#define SOC_BLOCK_SBX_NAMES_INITIALIZER {  \
    /*    blk  , isalias, name */      \
    { SOC_BLK_BP,       0,  "bp"  }, \
    { SOC_BLK_CI,       0,  "ci"  }, \
    { SOC_BLK_CS,       0,  "cs"  }, \
    { SOC_BLK_EB,       0,  "eb"  }, \
    { SOC_BLK_EP,       0,  "ep"  }, \
    { SOC_BLK_CMIC,     0,  "cmic"}, \
    { SOC_BLK_ES,       0,  "es"  }, \
    { SOC_BLK_FD,       0,  "fd"   }, \
    { SOC_BLK_FF,       0,  "ff"   }, \
    { SOC_BLK_FR,       0,  "fr"   }, \
    { SOC_BLK_TX,       0,  "tx"   }, \
    { SOC_BLK_GXPORT,   0,  "gxport"}, \
    { SOC_BLK_QMA,      0,  "qma"  }, \
    { SOC_BLK_QMB,      0,  "qmb"  }, \
    { SOC_BLK_QMC,      0,  "qmc"  }, \
    { SOC_BLK_QSA,      0,  "qsa"  }, \
    { SOC_BLK_QSB,      0,  "qsb"  }, \
    { SOC_BLK_RB,       0,  "rb"   }, \
    { SOC_BLK_SC_TOP,   0,  "sc_top" }, \
    { SOC_BLK_SF_TOP,   0,  "sf_top" }, \
    { SOC_BLK_OTPC,     1,  "otpc"  }, \
    { SOC_BLK_TS,       0,  "ts"   }, \
    { SOC_BLK_TMA,      0,  "tmua"  }, \
    { SOC_BLK_TMB,      0,  "tmub"  }, \
    { SOC_BLK_LRA,      0,  "lrpa"  }, \
    { SOC_BLK_LRB,      0,  "lrpb"  }, \
    { SOC_BLK_PB,       0,  "pb"    }, \
    { SOC_BLK_QM,       0,  "qm"    }, \
    { SOC_BLK_PR,       0,  "pr"    }, \
    { SOC_BLK_PT,       0,  "pt"    }, \
    { SOC_BLK_PP,       0,  "pp"    }, \
    { SOC_BLK_PD,       0,  "pd"    }, \
    { SOC_BLK_OC,       0,  "oc"    }, \
    { SOC_BLK_CO,       0,  "cop"   }, \
    { SOC_BLK_CM,       0,  "cm"    }, \
    { SOC_BLK_CX,       0,  "cx"    }, \
    { SOC_BLK_RC,       0,  "rc"    }, \
    { SOC_BLK_TP,       0,  "tp"    }, \
    { SOC_BLK_XTPORT,   0,  "xtport"}, \
    { SOC_BLK_XLPORT,   0,  "xlport"}, \
    { SOC_BLK_CPORT,    0,  "cport"}, \
    { SOC_BLK_CLPORT,   0,  "clport"}, \
    { SOC_BLK_IL,       0,  "intl"  }, \
    { SOC_BLK_ETU,      0,  "etu"   }, \
    { SOC_BLK_ETU_WRAP, 0,  "etu_wrap"}, \
    { SOC_BLK_NONE,     0,  ""      } }



/* used to initialize soc_block_name_t soc_dpp_block_names[] */
#define SOC_BLOCK_DPP_NAMES_INITIALIZER { \
    /*    blk  , isalias, name */      \
    {SOC_BLK_CFC,       0,  "CFC"}, \
    {SOC_BLK_DPI,       0,  "DPI"}, \
    {SOC_BLK_DRC,       0,  "DRC"}, \
    {SOC_BLK_ECI,       0,  "ECI"}, \
    {SOC_BLK_EGQ,       0,  "EGQ"}, \
    {SOC_BLK_EPNI,      0,  "EPNI"}, \
    {SOC_BLK_FCR,       0,  "FCR"}, \
    {SOC_BLK_MMU,       0,  "MMU"}, \
    {SOC_BLK_FCT,       0,  "FCT"}, \
    {SOC_BLK_FDR,       0,  "FDR"}, \
    {SOC_BLK_FDT,       0,  "FDT"}, \
    {SOC_BLK_MESH_TOPOLOGY, 0,  "MESH_TOPOLOGY"}, \
    {SOC_BLK_IDR,       0,  "IDR"}, \
    {SOC_BLK_IHB,       0,  "IHB"}, \
    {SOC_BLK_IHP,       0,  "IHP"}, \
    {SOC_BLK_IPS,       0,  "IPS"}, \
    {SOC_BLK_IPT,       0,  "IPT"}, \
    {SOC_BLK_IQM,       0,  "IQM"}, \
    {SOC_BLK_IRE,       0,  "IRE"}, \
    {SOC_BLK_IRR,       0,  "IRR"}, \
    {SOC_BLK_MAC,       0,  "MAC"}, \
    {SOC_BLK_MBU,       0,  "MBU"}, \
    {SOC_BLK_MCC,       0,  "MCC"}, \
    {SOC_BLK_MSW,       0,  "MSW"}, \
    {SOC_BLK_NBI,       0,  "NBI"}, \
    {SOC_BLK_NIF,       0,  "NIF"}, \
    {SOC_BLK_NIF_MAL,   0,  "NIF_MAL"}, \
    {SOC_BLK_OLP,       0,  "OLP"}, \
    {SOC_BLK_QDR,       0,  "QDR"}, \
    {SOC_BLK_RTP,       0,  "RTP"}, \
    {SOC_BLK_SCH,       0,  "SCH"}, \
    {SOC_BLK_OCB,       0,  "OCB"}, \
    {SOC_BLK_CRPS,      0,  "CRPS"}, \
    {SOC_BLK_FMAC,      0,  "FMAC"}, \
    {SOC_BLK_XLP,       0,  "XLP"}, \
    {SOC_BLK_CLP,       0,  "CLP"}, \
    {SOC_BLK_CGM,       0,  "CGM"}, \
    {SOC_BLK_OAMP,      0,  "OAMP"}, \
    {SOC_BLK_FSRD,      0,  "FSRD"}, \
    {SOC_BLK_DRCA,      0,  "DRCA"}, \
    {SOC_BLK_DRCB,      0,  "DRCB"}, \
    {SOC_BLK_DRCC,      0,  "DRCC"}, \
    {SOC_BLK_DRCD,      0,  "DRCD"}, \
    {SOC_BLK_DRCE,      0,  "DRCE"}, \
    {SOC_BLK_DRCF,      0,  "DRCF"}, \
    {SOC_BLK_DRCG,      0,  "DRCG"}, \
    {SOC_BLK_DRCH,      0,  "DRCH"}, \
    {SOC_BLK_BRDC_FMAC ,0,  "BRDC_FMAC"}, \
    {SOC_BLK_BRDC_FSRD,0,  "BRDC_FSRD"}, \
    {SOC_BLK_DRCBROADCAST, 0,  "DRCBROADCAST"}, \
    /* JERICHO-2-P3 */ {SOC_BLK_CFG,      0,  "CFG"}, \
    {SOC_BLK_EM,       0,  "EM"}, \
    {SOC_BLK_TABLE,    0,  "TBL"}, \
    {SOC_BLK_TCAM,     0,  "TCAM"}, \
    {SOC_BLK_OCCG,      0,  "OCCG"}, \
    {SOC_BLK_DCH,       0,  "DCH"}, \
    {SOC_BLK_DCL,       0,  "DCL"}, \
    {SOC_BLK_DCMA,      0,  "DCMA"}, \
    {SOC_BLK_DCMB,      0,  "DCMB"}, \
    {SOC_BLK_DCMC,      0,  "DCMC"}, \
    {SOC_BLK_CCS,       0,  "CCS"}, \
    {SOC_BLK_BRDC_FMACH,0,  "BRDC_FMACH"}, \
    {SOC_BLK_BRDC_FMACL,0,  "BRDC_FMACL"}, \
    {SOC_BLK_CMIC,      0,  "CMIC"}, \
    {SOC_BLK_NONE,      0,  ""      }}


#define SOC_BLOCK_DPP_PORT_NAMES_INITIALIZER {  \
    /*    blk  , isalias, name */ \
    { SOC_BLK_NONE,     0,  ""  }}

/*
 * soc_feature_fun_t: boolean function indicating if feature is supported
 * Used in bcm*.i files.
 */
typedef int (*soc_feature_fun_t) (int unit, soc_feature_t feature);

/*
 * soc_init_chip_fun_t: chip initialization function
 * Used in bcm*.i files.
 */
typedef void (*soc_init_chip_fun_t) (void);

/* Use macros to access */
extern soc_chip_groups_t soc_chip_type_map[SOC_CHIP_TYPES_COUNT];
extern char *soc_chip_type_names[SOC_CHIP_TYPES_COUNT];
extern char *soc_chip_group_names[SOC_CHIP_GROUPS_COUNT];
extern uint32 soc_ports_list[][6];
extern soc_numelport_set_t soc_numelports_list[][5];

#define SOC_CHIP_NAME(type)    (soc_chip_type_names[(type)])

#endif  /* !_SOC_CHIP_H */

