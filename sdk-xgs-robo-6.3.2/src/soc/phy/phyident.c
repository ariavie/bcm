/*
 * $Id: phyident.c 1.471.2.3 Broadcom SDK $
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
 * File:        phyident.c
 * Purpose:     These routines and structures are related to
 *              figuring out phy identification and correlating
 *              addresses to drivers
 */

#include <sal/types.h>
#include <sal/core/boot.h>

#include <soc/drv.h>
#include <soc/debug.h>
#include <soc/error.h>
#include <soc/phyreg.h>

#include <soc/phy.h>
#include <soc/phyctrl.h>
#include <soc/phy/phyctrl.h>
#include <soc/phy/drv.h>
#ifdef BCM_SBX_SUPPORT
#include <soc/sbx/sbx_drv.h>
#ifdef BCM_CALADAN3_SUPPORT
#include <soc/sbx/caladan3/port.h>
#endif
#endif

#ifdef BCM_ARAD_SUPPORT
#include <soc/dpp/NIF/port_sw_db.h>
#include <soc/dpp/drv.h>
#endif

#ifdef BCM_KATANA2_SUPPORT
#include <soc/katana2.h>
#endif

#include "phydefs.h"  /* Must include before other phy related includes */
#include "phyident.h"
#include "phyaddr.h"
#include "physr.h"

#define _MAX_PHYS       128

typedef struct {
    uint16 int_addr;
    uint16 ext_addr;
} port_phy_addr_t;

/* Per port phy address map. */
static port_phy_addr_t *port_phy_addr[SOC_MAX_NUM_DEVICES];

static soc_phy_table_t  *phy_table[_MAX_PHYS];
static int              _phys_in_table = -1;

static int _chk_phy(int unit, soc_port_t port, soc_phy_table_t *my_entry,
                    uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi);

static int _chk_null(int unit, soc_port_t port, soc_phy_table_t *my_entry,
                     uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi);

static int _chk_sfp_phy(int unit, soc_port_t port, soc_phy_table_t *my_entry,
                    uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi);

#if defined(INCLUDE_PHY_SIMUL)
static int _chk_simul(int unit, soc_port_t port, soc_phy_table_t *my_entry,
                      uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi);
#endif /* INCLUDE_PHY_SIMUL */

#if defined(INCLUDE_PHY_5690)
static int _chk_fiber5690(int unit, soc_port_t port,
                          soc_phy_table_t *my_entry,
                          uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi);
#endif /* INCLUDE_PHY_5690 */

#if defined(INCLUDE_XGXS_QSGMII65)
static int _chk_qsgmii53314(int unit, soc_port_t port,
                          soc_phy_table_t *my_entry,
                          uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi);
#endif /* INCLUDE_XGXS_QSGMII65 */

#if defined(INCLUDE_PHY_54680)
static int _chk_qgphy_5332x(int unit, soc_port_t port, 
                          soc_phy_table_t *my_entry,
                          uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi);
#endif /* INCLUDE_PHY_54680 */

#if defined(INCLUDE_PHY_56XXX)
static int _chk_fiber56xxx(int unit, soc_port_t port,
                          soc_phy_table_t *my_entry,
                          uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi);
#endif /* INCLUDE_PHY_56XXX */

#if defined(INCLUDE_PHY_56XXX)
static int _chk_fiber56xxx_5601x(int unit, soc_port_t port,
                          soc_phy_table_t *my_entry,
                          uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi);
#endif /* INCLUDE_PHY_56XXX */

#if defined(INCLUDE_SERDES_COMBO)
static int _chk_serdes_combo_5601x(int unit, soc_port_t port,
                          soc_phy_table_t *my_entry,
                          uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi);
#endif /* INCLUDE_SERDES_COMBO */

#if defined(INCLUDE_PHY_XGXS6)
static int _chk_unicore(int unit, soc_port_t port,
                        soc_phy_table_t *my_entry,
                        uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi);
#endif /* INCLUDE_PHY_XGXS6 */

#if defined(INCLUDE_PHY_8706)
static int _chk_8706(int unit, soc_port_t port, soc_phy_table_t *my_entry,
                     uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi);
#endif /* INCLUDE_PHY_8706 */

#if defined(INCLUDE_PHY_8072)
static int _chk_8072(int unit, soc_port_t port, soc_phy_table_t *my_entry,
                     uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi);
#endif /* INCLUDE_PHY_8072 */

#if defined(INCLUDE_PHY_8040)
static int _chk_8040(int unit, soc_port_t port, soc_phy_table_t *my_entry,
                     uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi);
#endif /* INCLUDE_PHY_8040 */

#if defined(INCLUDE_PHY_8481)
static int _chk_8481(int unit, soc_port_t port, soc_phy_table_t *my_entry,
                     uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi);
#endif /* INCLUDE_PHY_8481 */

#if defined(INCLUDE_SERDES_COMBO65)
static int
_chk_serdescombo65(int unit, soc_port_t port, soc_phy_table_t *my_entry,
         uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi);

#endif /* INCLUDE_SERDES_COMBO65 */

#if defined(INCLUDE_XGXS_16G)
static int
_chk_xgxs16g1l(int unit, soc_port_t port, soc_phy_table_t *my_entry,
         uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi);
#endif /* INCLUDE_XGXS_16G */

#if defined(INCLUDE_XGXS_WCMOD)
static int
_chk_wcmod(int unit, soc_port_t port, soc_phy_table_t *my_entry,
         uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi);
#endif  /* INCLUDE_XGXS_WCMOD */

#if defined(INCLUDE_PHY_53XXX)
static int _chk_fiber53xxx(int unit, soc_port_t port,
                          soc_phy_table_t *my_entry,
                          uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi);
#endif /* INCLUDE_PHY_53XXX */

static int _chk_default(int unit, soc_port_t port, soc_phy_table_t *my_entry,
                        uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi);

static soc_known_phy_t
    _phy_ident_type_get(uint16 phy_id0, uint16 phy_id1);

static soc_phy_table_t _null_phy_entry =
    {_chk_null, _phy_id_NULL,   "Null",    &phy_null,          NULL};

#if defined(INCLUDE_PHY_SIMUL)
static soc_phy_table_t _simul_phy_entry =
    {_chk_simul, _phy_id_SIMUL, "Simulation", &phy_simul,      NULL};
#endif /* INCLUDE_PHY_SIMUL */

#if defined(INCLUDE_PHY_5690)
static soc_phy_table_t _fiber5690_phy_entry =
    {_chk_fiber5690, _phy_id_NULL, "Internal SERDES", &phy_5690drv_ge, NULL };
#endif /* INCLUDE_PHY_5690 */

#if defined(INCLUDE_PHY_56XXX)
static soc_phy_table_t _fiber56xxx_phy_entry =
    {_chk_fiber56xxx, _phy_id_NULL, "Internal SERDES", &phy_56xxxdrv_ge, NULL };
#endif /* INCLUDE_PHY_56XXX */

#if defined(INCLUDE_PHY_53XXX)
static soc_phy_table_t _fiber53xxx_phy_entry =
    {_chk_fiber53xxx, _phy_id_NULL, "Internal  SERDES", &phy_53xxxdrv_ge, NULL};
#endif /* INCLUDE_PHY_53XXX */

static soc_phy_table_t _default_phy_entry =
    {_chk_default, _phy_id_NULL, "Unknown", &phy_drv_fe, NULL};

#ifdef BCM_ROBO_SUPPORT
static soc_phy_table_t _default_phy_entry_ge =
    {_chk_default, _phy_id_NULL, "Unknown", &phy_drv_ge, NULL};
#endif /* BCM_ROBO_SUPPORT */

/*
 * Variable:
 *      _standard_phy_table
 * Purpose:
 *      Defines the standard supported Broadcom PHYs, and the corresponding
 *      driver.
 */

static soc_phy_table_t _standard_phy_table[] = {

#ifdef INCLUDE_PHY_522X
    {_chk_phy, _phy_id_BCM5218, "BCM5218",     &phy_522xdrv_fe, NULL},
    {_chk_phy, _phy_id_BCM5220, "BCM5220/21",  &phy_522xdrv_fe, NULL},
    {_chk_phy, _phy_id_BCM5226, "BCM5226",     &phy_522xdrv_fe, NULL},
    {_chk_phy, _phy_id_BCM5228, "BCM5228",     &phy_522xdrv_fe, NULL},
    {_chk_phy, _phy_id_BCM5238, "BCM5238",     &phy_522xdrv_fe, NULL},
    {_chk_phy, _phy_id_BCM5248, "BCM5248",     &phy_522xdrv_fe, NULL},
    {_chk_phy, _phy_id_BCM5324, "BCM5324/FE",  &phy_522xdrv_fe, NULL},
    {_chk_phy, _phy_id_BCM5348, "BCM5348/FE",  &phy_522xdrv_fe, NULL},
    {_chk_phy, _phy_id_BCM53242, "BCM53242/FE",  &phy_522xdrv_fe, NULL},
    {_chk_phy, _phy_id_BCM53262, "BCM53262/FE",  &phy_522xdrv_fe, NULL},
    {_chk_phy, _phy_id_BCM53101, "BCM53101/FE",  &phy_522xdrv_fe, NULL},
    {_chk_phy, _phy_id_BCM53280, "BCM53280/FE",  &phy_522xdrv_fe, NULL},
    {_chk_phy, _phy_id_BCM53600, "BCM53600/FE",  &phy_522xdrv_fe, NULL},
#endif /* INCLUDE_PHY_522X */

    {_chk_phy, _phy_id_BCM5400, "BCM5400",     &phy_drv_ge, NULL},

#ifdef INCLUDE_PHY_54XX
    {_chk_phy, _phy_id_BCM5401, "BCM5401",     &phy_5401drv_ge, NULL},
    {_chk_phy, _phy_id_BCM5402, "BCM5402",     &phy_5402drv_ge, NULL},
    {_chk_phy, _phy_id_BCM5404, "BCM5404",     &phy_5404drv_ge, NULL},
    {_chk_phy, _phy_id_BCM5424, "BCM5424/34",  &phy_5424drv_ge, NULL},
    {_chk_phy, _phy_id_BCM5411, "BCM5411",     &phy_5411drv_ge, NULL},
#endif /* INCLUDE_PHY_54XX */
#if defined(INCLUDE_PHY_5464_ROBO)
    {_chk_phy, _phy_id_BCM5461, "BCM5461",     &phy_5464robodrv_ge, NULL},
    {_chk_phy, _phy_id_BCM5464, "BCM5464",     &phy_5464robodrv_ge, NULL},
    {_chk_phy, _phy_id_BCM5466, "BCM5466",     &phy_5464robodrv_ge, NULL},
    {_chk_phy, _phy_id_BCM5478, "BCM5478",     &phy_5464robodrv_ge, NULL},
    {_chk_phy, _phy_id_BCM5488, "BCM5488",     &phy_5464robodrv_ge, NULL},
#endif /* INCLUDE_PHY_5464_ROBO */

#if defined(INCLUDE_PHY_5482_ROBO)
    {_chk_phy, _phy_id_BCM5482, "BCM5482",     &phy_5482robodrv_ge, NULL},
    {_chk_phy, _phy_id_BCM5481, "BCM5481",     &phy_5482robodrv_ge, NULL},
#endif /* INCLUDE_PHY_5482_ROBO */

#if defined(INCLUDE_PHY_5464_ESW)
    {_chk_phy, _phy_id_BCM5461, "BCM5461",     &phy_5464drv_ge, NULL},
    {_chk_phy, _phy_id_BCM5464, "BCM5464",     &phy_5464drv_ge, NULL},
    {_chk_phy, _phy_id_BCM5466, "BCM5466",     &phy_5464drv_ge, NULL},
    {_chk_phy, _phy_id_BCM5478, "BCM5478",     &phy_5464drv_ge, NULL},
    {_chk_phy, _phy_id_BCM5488, "BCM5488",     &phy_5464drv_ge, NULL},
    {_chk_phy, _phy_id_BCM54980, "BCM54980",   &phy_5464drv_ge, NULL},
    {_chk_phy, _phy_id_BCM54980C, "BCM54980",  &phy_5464drv_ge, NULL},
    {_chk_phy, _phy_id_BCM54980V, "BCM54980",  &phy_5464drv_ge, NULL},
    {_chk_phy, _phy_id_BCM54980VC, "BCM54980", &phy_5464drv_ge, NULL},
    {_chk_phy, _phy_id_BCM53314, "BCM53314",   &phy_5464drv_ge, NULL},
#endif /* INCLUDE_PHY_5464 */

#if defined(INCLUDE_PHY_5464_ROBO)
    {_chk_phy, _phy_id_BCM5398, "BCM5398",     &phy_5464robodrv_ge, NULL},
    {_chk_phy, _phy_id_BCM5395, "BCM5395",     &phy_5464robodrv_ge, NULL},
    {_chk_phy, _phy_id_BCM53115, "BCM53115",     &phy_5464robodrv_ge, NULL},
    {_chk_phy, _phy_id_BCM53118, "BCM53118",     &phy_5464robodrv_ge, NULL},
#endif /* BCM_ROBO_SUPPORT && INCLUDE_PHY_5464 */

#if defined(INCLUDE_PHY_5482_ESW)
    {_chk_phy, _phy_id_BCM5482, "BCM5482/801x",     &phy_5482drv_ge, NULL},
#endif /* INCLUDE_PHY_5482 */

#if defined(INCLUDE_PHY_54684)
    {_chk_phy, _phy_id_BCM54684, "BCM54684", &phy_54684drv_ge, NULL},
#endif /* defined(INCLUDE_PHY_54684) */

#if defined(INCLUDE_PHY_54640)
    {_chk_phy, _phy_id_BCM54640, "BCM54640", &phy_54640drv_ge, NULL},
#endif /* defined(INCLUDE_PHY_54640) */

#if defined(INCLUDE_PHY_54682)
    {_chk_phy, _phy_id_BCM54682, "BCM54682E", &phy_54682drv_ge, NULL},
    {_chk_phy, _phy_id_BCM54684E, "BCM54684E", &phy_54682drv_ge, NULL},
    {_chk_phy, _phy_id_BCM54685, "BCM54685", &phy_54682drv_ge, NULL},
#endif /* defined(INCLUDE_PHY_54682) */

#ifdef INCLUDE_PHY_54616
    {_chk_phy, _phy_id_BCM54616, "BCM54616", &phy_54616drv_ge, NULL},
#endif /* INCLUDE_PHY_54616 */

#ifdef INCLUDE_PHY_84728 
    {_chk_phy, _phy_id_BCM84707, "BCM84707",  &phy_84728drv_xe,  NULL}, 
    {_chk_phy, _phy_id_BCM84073, "BCM84073",  &phy_84728drv_xe,  NULL}, 
    {_chk_phy, _phy_id_BCM84074, "BCM84074",  &phy_84728drv_xe,  NULL}, 
    {_chk_phy, _phy_id_BCM84728, "BCM84728",  &phy_84728drv_xe,  NULL}, 
    {_chk_phy, _phy_id_BCM84748, "BCM84748",  &phy_84728drv_xe,  NULL}, 
    {_chk_phy, _phy_id_BCM84727, "BCM84727",  &phy_84728drv_xe,  NULL}, 
    {_chk_phy, _phy_id_BCM84747, "BCM84747",  &phy_84728drv_xe,  NULL}, 
    {_chk_phy, _phy_id_BCM84762, "BCM84762",  &phy_84728drv_xe,  NULL}, 
    {_chk_phy, _phy_id_BCM84764, "BCM84764",  &phy_84728drv_xe,  NULL}, 
    {_chk_phy, _phy_id_BCM84042, "BCM84042",  &phy_84728drv_xe,  NULL}, 
    {_chk_phy, _phy_id_BCM84044, "BCM84044",  &phy_84728drv_xe,  NULL}, 
#endif /* INCLUDE_PHY_84728 */ 

#ifdef INCLUDE_MACSEC
#if defined(INCLUDE_PHY_54580)
    {_chk_phy, _phy_id_BCM54584, "BCM54584", &phy_54580drv_ge, NULL},
    {_chk_phy, _phy_id_BCM54580, "BCM54580", &phy_54580drv_ge, NULL},
    {_chk_phy, _phy_id_BCM54540, "BCM54540", &phy_54580drv_ge, NULL},
    {_chk_phy, _phy_id_BCM54585, "BCM54584", &phy_54580drv_ge, NULL},
#endif /* defined(INCLUDE_PHY_54580) */
#if defined(INCLUDE_PHY_54380)
    {_chk_phy, _phy_id_BCM54380, "BCM54380", &phy_54380drv_ge, NULL},
    {_chk_phy, _phy_id_BCM54382, "BCM54382", &phy_54380drv_ge, NULL},
    {_chk_phy, _phy_id_BCM54340, "BCM54340", &phy_54380drv_ge, NULL},
    {_chk_phy, _phy_id_BCM54385, "BCM54385", &phy_54380drv_ge, NULL},
#endif /* defined(INCLUDE_PHY_54380) */
#ifdef INCLUDE_PHY_8729
    {_chk_phy, _phy_id_BCM8729, "BCM8729",  &phy_8729drv_gexe,  NULL},
#endif /* INCLUDE_PHY_8729 */
#ifdef INCLUDE_PHY_84756 
    {_chk_phy, _phy_id_BCM84756, "BCM84756",  &phy_84756drv_xe,  NULL},
    {_chk_phy, _phy_id_BCM84757, "BCM84757",  &phy_84756drv_xe,  NULL},
    {_chk_phy, _phy_id_BCM84759, "BCM84759",  &phy_84756drv_xe,  NULL},
#endif /* INCLUDE_PHY_84756 */
#ifdef INCLUDE_PHY_84334
    {_chk_phy, _phy_id_BCM84334, "BCM84334",  &phy_84334drv_xe,  NULL},
    {_chk_phy, _phy_id_BCM84336, "BCM84336",  &phy_84334drv_xe,  NULL},
    {_chk_phy, _phy_id_BCM84844, "BCM84844",  &phy_84334drv_xe,  NULL},
    {_chk_phy, _phy_id_BCM84846, "BCM84846",  &phy_84334drv_xe,  NULL},
    {_chk_phy, _phy_id_BCM84848, "BCM84848",  &phy_84334drv_xe,  NULL},
#endif /* INCLUDE_PHY_84334 */
#ifdef INCLUDE_PHY_84749 
    {_chk_phy, _phy_id_BCM84749, "BCM84749",  &phy_84749drv_xe,  NULL},
    {_chk_phy, _phy_id_BCM84729, "BCM84729",  &phy_84749drv_xe,  NULL},
#endif /* INCLUDE_PHY_84749 */
#endif  /* INCLUDE_MACSEC */
#if defined(INCLUDE_PHY_542XX)
    {_chk_phy, _phy_id_BCM54280, "BCM54280", &phy_542xxdrv_ge, NULL},
    {_chk_phy, _phy_id_BCM54282, "BCM54282", &phy_542xxdrv_ge, NULL},
    {_chk_phy, _phy_id_BCM54240, "BCM54240", &phy_542xxdrv_ge, NULL},
    {_chk_phy, _phy_id_BCM54285, "BCM54285", &phy_542xxdrv_ge, NULL},
    {_chk_phy, _phy_id_BCM5428x, "BCM5428X", &phy_542xxdrv_ge, NULL},
    {_chk_phy, _phy_id_BCM54290, "BCM54290", &phy_542xxdrv_ge, NULL},
    {_chk_phy, _phy_id_BCM54292, "BCM54292", &phy_542xxdrv_ge, NULL},
    {_chk_phy, _phy_id_BCM54294, "BCM54294", &phy_542xxdrv_ge, NULL},
    {_chk_phy, _phy_id_BCM54295, "BCM54295", &phy_542xxdrv_ge, NULL},
#endif /* defined(INCLUDE_PHY_542XX) */
#ifdef INCLUDE_PHY_84756
#if defined(INCLUDE_FCMAP) || defined(INCLUDE_MACSEC)
    {_chk_phy, _phy_id_BCM84756, "BCM84756",  &phy_84756drv_fcmap_xe,  NULL},
    {_chk_phy, _phy_id_BCM84757, "BCM84757",  &phy_84756drv_fcmap_xe,  NULL},
    {_chk_phy, _phy_id_BCM84759, "BCM84759",  &phy_84756drv_fcmap_xe,  NULL},
#endif /* INCLUDE_FCMAP  || INCLUDE_MACSEC */
#endif /* INCLUDE_PHY_84756 */

#ifdef INCLUDE_PHY_5421S
    {_chk_phy, _phy_id_BCM5421, "BCM5421S",    &phy_5421Sdrv_ge, NULL},
#endif /* INCLUDE_PHY_5421S */

#ifdef INCLUDE_PHY_54680
    {_chk_phy, _phy_id_BCM54680, "BCM54680", &phy_54680drv_ge, NULL},
    {_chk_qgphy_5332x, _phy_id_BCM53324, "BCM53324", &phy_54680drv_ge, NULL},
    {_chk_phy, _phy_id_BCM53125, "BCM53125", &phy_54680drv_ge, NULL},
    {_chk_phy, _phy_id_BCM53128, "BCM53128", &phy_54680drv_ge, NULL},
    {_chk_phy, _phy_id_BCM53010, "BCM53010", &phy_54680drv_ge, NULL},
    {_chk_phy, _phy_id_BCM53018, "BCM53018", &phy_54680drv_ge, NULL},
    {_chk_phy, _phy_id_BCM53020, "BCM5302X", &phy_54680drv_ge, NULL},
#endif /* INCLUDE_PHY_54680 */

#ifdef INCLUDE_PHY_54880
    {_chk_phy, _phy_id_BCM54880, "BCM54880", &phy_54880drv_ge, NULL},
    {_chk_phy, _phy_id_BCM54881, "BCM54881", &phy_54880drv_ge, NULL},
    {_chk_phy, _phy_id_BCM54810, "BCM54810", &phy_54880drv_ge, NULL},
    {_chk_phy, _phy_id_BCM54811, "BCM54811", &phy_54880drv_ge, NULL},
    {_chk_phy, _phy_id_BCM89500, "BCM89500", &phy_54880drv_ge, NULL},
#endif /* INCLUDE_PHY_54880 */

#ifdef INCLUDE_PHY_54640E
    {_chk_phy, _phy_id_BCM54680E, "BCM54640E", &phy_54640drv_ge, NULL},
#endif /* INCLUDE_PHY_54640E */

#ifdef INCLUDE_PHY_54880E
    {_chk_phy, _phy_id_BCM54880E, "BCM54880E", &phy_54680drv_ge, NULL},
#endif /* INCLUDE_PHY_54880E */

#ifdef INCLUDE_PHY_54680E
    {_chk_phy, _phy_id_BCM54680E, "BCM54680E", &phy_54680drv_ge, NULL},
#endif /* INCLUDE_PHY_54680E */

#ifdef INCLUDE_PHY_52681E
    {_chk_phy, _phy_id_BCM52681E, "BCM52681E", &phy_54680drv_ge, NULL},
#endif /* INCLUDE_PHY_52681E */

#ifdef INCLUDE_PHY_8703
    {_chk_phy, _phy_id_BCM8703, "BCM8703",  &phy_8703drv_xe,  NULL},
    {_chk_phy, _phy_id_BCM8704, "BCM8704",  &phy_8703drv_xe,  NULL},
#endif /* INCLUDE_PHY_8703 */
#ifdef INCLUDE_PHY_8705
    {_chk_phy, _phy_id_BCM8705, "BCM8705/24/25",  &phy_8705drv_xe,  NULL},
#endif /* INCLUDE_PHY_8705 */
#if defined(INCLUDE_PHY_8706)
    /* BCM8706_A0 and BCM8705 has the same device ID. Therefore, the probe must
     * check for 8706 before 8705 to correctly attach BCM8706. For 8706,
     * phy_8706 config must be set.
     */
    {_chk_8706, _phy_id_BCM8706, "BCM8706/8726", &phy_8706drv_xe, NULL},
    {_chk_8706, _phy_id_BCM8727, "BCM8727", &phy_8706drv_xe, NULL},
    {_chk_8706, _phy_id_BCM8747, "BCM8728/8747", &phy_8706drv_xe, NULL},
#endif /* INCLUDE_PHY_8706 */
#if defined(INCLUDE_PHY_8072)
    {_chk_8072, _phy_id_BCM8072, "BCM8072", &phy_8072drv_xe, NULL},
    {_chk_8072, _phy_id_BCM8073, "BCM8073", &phy_8072drv_xe, NULL},
    {_chk_8072, _phy_id_BCM8074, "BCM8074", &phy_8074drv_xe, NULL},
#endif /* INCLUDE_PHY_8072 */

#if defined(INCLUDE_PHY_8040)
    {_chk_8040, _phy_id_BCM8040, "BCM8040", &phy_8040drv_xe, NULL},
#endif /* INCLUDE_PHY_8040 */

#if defined(INCLUDE_PHY_8481)
    {_chk_8481, _phy_id_BCM8481x, "BCM8481X", &phy_8481drv_xe, NULL},
    {_chk_8481, _phy_id_BCM84812ce, "BCM84812", &phy_8481drv_xe, NULL},
    {_chk_8481, _phy_id_BCM84821, "BCM84821", &phy_8481drv_xe, NULL},
    {_chk_8481, _phy_id_BCM84822, "BCM84822", &phy_8481drv_xe, NULL},
    {_chk_8481, _phy_id_BCM84823, "BCM84823", &phy_8481drv_xe, NULL},
    {_chk_8481, _phy_id_BCM84833, "BCM84833", &phy_8481drv_xe, NULL},
    {_chk_8481, _phy_id_BCM84834, "BCM84834", &phy_8481drv_xe, NULL},
    {_chk_8481, _phy_id_BCM84835, "BCM84835", &phy_8481drv_xe, NULL},
    {_chk_8481, _phy_id_BCM84836, "BCM84836", &phy_8481drv_xe, NULL},
    {_chk_8481, _phy_id_BCM84844, "BCM84844", &phy_8481drv_xe, NULL},
    {_chk_8481, _phy_id_BCM84846, "BCM84846", &phy_8481drv_xe, NULL},
    {_chk_8481, _phy_id_BCM84848, "BCM84848", &phy_8481drv_xe, NULL},
#endif /* INCLUDE_PHY_8481 */
#ifdef INCLUDE_PHY_8750
    {_chk_phy, _phy_id_BCM8750, "BCM8750",  &phy_8750drv_xe,  NULL},
    {_chk_phy, _phy_id_BCM8752, "BCM8752",  &phy_8750drv_xe,  NULL},
    {_chk_phy, _phy_id_BCM8754, "BCM8754",  &phy_8750drv_xe,  NULL},
#endif /* INCLUDE_PHY_8750 */
#ifdef INCLUDE_PHY_84740
    {_chk_phy, _phy_id_BCM84740, "BCM84740",  &phy_84740drv_xe,  NULL},
    {_chk_phy, _phy_id_BCM84164, "BCM84164",  &phy_84740drv_xe,  NULL},
    {_chk_phy, _phy_id_BCM84758, "BCM84758",  &phy_84740drv_xe,  NULL},
    {_chk_phy, _phy_id_BCM84780, "BCM84780",  &phy_84740drv_xe,  NULL},
    {_chk_phy, _phy_id_BCM84784, "BCM84784",  &phy_84740drv_xe,  NULL},
#endif /* INCLUDE_PHY_84740 */
#ifdef INCLUDE_PHY_84328
    {_chk_phy, _phy_id_BCM84328, "BCM84328",  &phy_84328drv_xe,  NULL},
#endif /* INCLUDE_PHY_84328 */
#ifdef INCLUDE_PHY_84793
    {_chk_phy, _phy_id_BCM84793, "BCM84793",  &phy_84793drv_ce,  NULL},
#endif
#ifdef INCLUDE_PHY_82328
    {_chk_phy, _phy_id_BCM82328, "BCM82328",  &phy_82328drv_xe,  NULL},
#endif

    {_chk_sfp_phy,0xffff,"copper sfp",&phy_copper_sfp_drv,NULL},
};

/* Internal PHY table */
static soc_phy_table_t _int_phy_table[] = {
#if defined(INCLUDE_PHY_56XXX)
    {_chk_fiber56xxx, _phy_id_NULL, "Internal SERDES",
     &phy_56xxxdrv_ge, NULL},
#endif /* INCLUDE_PHY_56XXX */

#if defined(INCLUDE_PHY_53XXX)
    {_chk_fiber53xxx, _phy_id_NULL, "Internal SERDES",
     &phy_53xxxdrv_ge, NULL},
#endif /* INCLUDE_PHY_53XXX */

#if defined(INCLUDE_SERDES_COMBO)
     {_chk_phy, _phy_id_SERDESCOMBO, "COMBO", &phy_serdescombo_ge, NULL},
#endif /* INCLUDE_SERDES_COMBO */

#if defined(INCLUDE_SERDES_100FX)
    {_chk_phy, _phy_id_SERDES100FX, "1000X/100FX", &phy_serdes100fx_ge, NULL},
#endif /* INCLUDE_SERDES_100FX */

#if defined(INCLUDE_SERDES_65LP)
    {_chk_phy, _phy_id_SERDES65LP, "65LP", &phy_serdes65lp_ge, NULL},
#endif /* INCLUDE_SERDES_65LP */

#ifdef INCLUDE_XGXS_QSGMII65
    {_chk_qsgmii53314, _phy_id_NULL, "QSGMII65", &phy_qsgmii65_ge,
     NULL },
#endif /* INCLUDE_XGXS_QSGMII65 */

#if defined(INCLUDE_SERDES_COMBO65)
    {_chk_serdescombo65, _phy_id_SERDESCOMBO65, "COMBO65", 
     &phy_serdescombo65_ge, NULL},
#endif /* INCLUDE_SERDES_COMBO65 */

#if defined(INCLUDE_PHY_XGXS1)
    {_chk_phy, _phy_id_BCMXGXS1, "XGXS1",      &phy_xgxs1_hg, NULL},
    {_chk_phy, _phy_id_BCMXGXS2, "XGXS2",      &phy_xgxs1_hg, NULL},
#endif /* INCLUDE_PHY_XGXS1 */

#if defined(INCLUDE_PHY_XGXS5)
    {_chk_phy, _phy_id_BCMXGXS5, "XGXS5",      &phy_xgxs5_hg, NULL},
#endif /* INCLUDE_PHY_XGXS5 */

#if defined(INCLUDE_PHY_XGXS6)
    {_chk_phy,     _phy_id_BCMXGXS6, "XGXS6",  &phy_xgxs6_hg, NULL},
    {_chk_unicore, _phy_id_BCMXGXS2, "XGXS6",  &phy_xgxs6_hg, NULL},
#endif /* INCLUDE_PHY_XGXS6 */

    /* Must probe for newer internal SerDes/XAUI first before probing for
     * older devices. Newer devices reuse the same device ID and introduce
     * a new mechanism to differentiate betwee devices. Therefore, newer
     * PHY drivers implement probe funtion to check for correct device.
     */
#if defined(INCLUDE_XGXS_WC40)
    {_chk_phy, _phy_id_XGXS_WC40, "WC40/4",    &phy_wc40_hg, NULL},
#endif /* INCLUDE_XGXS_HL65 */

#if defined(INCLUDE_XGXS_HL65)
    {_chk_phy, _phy_id_XGXS_HL65, "HL65/4",    &phy_hl65_hg, NULL},
#endif /* INCLUDE_XGXS_HL65 */

#if defined(INCLUDE_PHY_56XXX)
    {_chk_fiber56xxx_5601x, _phy_id_NULL, "Internal SERDES",
     &phy_56xxx_5601x_drv_ge, NULL},
#endif /* INCLUDE_PHY_56XXX */

#if defined(INCLUDE_SERDES_COMBO)
    {_chk_serdes_combo_5601x, _phy_id_NULL, "COMBO SERDES",
     &phy_serdescombo_5601x_ge, NULL},
#endif /* INCLUDE_SERDES_COMBO */

#if defined(INCLUDE_XGXS_16G)
    {_chk_phy, _phy_id_XGXS_16G, "XGXS16G",    &phy_xgxs16g_hg, NULL},
    {_chk_xgxs16g1l, _phy_id_XGXS_16G, "XGXS16G/1", &phy_xgxs16g1l_ge, NULL},
#endif /* INCLUDE_XGXS_16G */

#if defined(INCLUDE_XGXS_WCMOD)
    {_chk_wcmod, _phy_id_XGXS_WL, "WCMOD/4",    &phy_wcmod_hg, NULL},
#endif  /* INCLUDE_XGXS_WL */

#if defined(INCLUDE_XGXS_TSCMOD)
    {_chk_phy, _phy_id_XGXS_TSC, "TSCMOD/4",    &phy_tscmod_hg, NULL}
#endif  /* INCLUDE_XGXS_TSC */


};
#if defined (INCLUDE_PHY_56XXX) || defined (INCLUDE_SERDES_COMBO)
/*
 * Check corrupted registers by writing zeroes
 * to block address register and making sure zeroes
 * are read back.
 */
STATIC INLINE int 
_is_corrupted_reg(int unit, uint8 phy_addr)
{
    int         rv;
    uint16      data;

    rv = soc_miim_write(unit, phy_addr, 0x1f, 0);
    if (rv != SOC_E_NONE) {
        return FALSE;
    }
    rv = soc_miim_read(unit, phy_addr, 0x1f, &data);
    if (rv != SOC_E_NONE) {
        return FALSE;
    }

    return (data != 0);
}
#endif /* (INCLUDE_PHY_56XXX) || (INCLUDE_SERDES_COMBO)*/

/*
 * Function:
 *      _init_phy_table(void)
 * Purpose:
 *      Initialize the phy table with known phys.
 * Parameters:
 *      None
 */

static void
_init_phy_table(void)
{
    uint32      i;

    for (i = 0; i < COUNTOF(_standard_phy_table) && i < _MAX_PHYS; i++) {
        phy_table[i] = &_standard_phy_table[i];
    }

    _phys_in_table = i;
}

#if defined(BCM_ENDURO_SUPPORT)
static void
_enduro_phy_addr_default(int unit, int port,
                          uint16 *phy_addr, uint16 *phy_addr_int)
{
    static const uint16 _soc_enduro_int_phy_addr[] = {
        0x00, /* Port  0 (cmic)         N/A */
        0x00, /* Port  1 (N/A)                */
        0x81, /* Port  2 (8SERDES_0)    IntBus=0 Addr=0x01 */
        0x82, /* Port  3 (8SERDES_0)    IntBus=0 Addr=0x02 */
        0x83, /* Port  4 (8SERDES_0)    IntBus=0 Addr=0x03 */
        0x84, /* Port  5 (8SERDES_0)    IntBus=0 Addr=0x04 */
        0x85, /* Port  6 (8SERDES_0)    IntBus=0 Addr=0x05 */
        0x86, /* Port  7 (8SERDES_0)    IntBus=0 Addr=0x06 */
        0x87, /* Port  8 (8SERDES_0)    IntBus=0 Addr=0x07 */
        0x88, /* Port  9 (8SERDES_0)    IntBus=0 Addr=0x08 */
        0x89, /* Port 10 (8SERDES_1)    IntBus=0 Addr=0x09 */
        0x8a, /* Port 11 (8SERDES_1)    IntBus=0 Addr=0x0a */
        0x8b, /* Port 12 (8SERDES_1)    IntBus=0 Addr=0x0b */
        0x8c, /* Port 13 (8SERDES_1)    IntBus=0 Addr=0x0c */
        0x8d, /* Port 14 (8SERDES_1)    IntBus=0 Addr=0x0d */
        0x8e, /* Port 15 (8SERDES_1)    IntBus=0 Addr=0x0e */
        0x8f, /* Port 16 (8SERDES_1)    IntBus=0 Addr=0x0f */
        0x90, /* Port 17 (8SERDES_1)    IntBus=0 Addr=0x10 */
        0x91, /* Port 18 (9SERDES)      IntBus=0 Addr=0x11 */
        0x92, /* Port 19 (9SERDES)      IntBus=0 Addr=0x12 */
        0x93, /* Port 20 (9SERDES)      IntBus=0 Addr=0x13 */
        0x94, /* Port 21 (9SERDES)      IntBus=0 Addr=0x14 */
        0x95, /* Port 22 (9SERDES)      IntBus=0 Addr=0x15 */
        0x96, /* Port 23 (9SERDES)      IntBus=0 Addr=0x16 */
        0x97, /* Port 24 (9SERDES)      IntBus=0 Addr=0x17 */
        0x98, /* Port 25 (9SERDES)      IntBus=0 Addr=0x18 */
        0x99, /* Port 26 (HC0)          IntBus=0 Addr=0x19 */
        0x9a, /* Port 27 (HC1)          IntBus=0 Addr=0x1a */
        0x9b, /* Port 28 (HC2)          IntBus=0 Addr=0x1b */
        0x9c, /* Port 29 (HC3)          IntBus=0 Addr=0x1c */
    };

    static const uint16 _soc_phy_addr_bcm56334[] = {
        0x00, /* Port  0 (cmic) N/A */
        0x00, /* Port  1 ( N/A)                    */
        0x01, /* Port  2 ( ge0) ExtBus=0 Addr=0x01 */
        0x02, /* Port  3 ( ge1) ExtBus=0 Addr=0x02 */
        0x03, /* Port  4 ( ge2) ExtBus=0 Addr=0x03 */
        0x04, /* Port  5 ( ge3) ExtBus=0 Addr=0x04 */
        0x05, /* Port  6 ( ge4) ExtBus=0 Addr=0x05 */
        0x06, /* Port  7 ( ge5) ExtBus=0 Addr=0x06 */
        0x07, /* Port  8 ( ge6) ExtBus=0 Addr=0x07 */
        0x08, /* Port  9 ( ge7) ExtBus=0 Addr=0x08 */
        0x09, /* Port 10 ( ge8) ExtBus=0 Addr=0x09 */
        0x0a, /* Port 11 ( ge9) ExtBus=0 Addr=0x0a */
        0x0b, /* Port 12 (ge10) ExtBus=0 Addr=0x0b */
        0x0c, /* Port 13 (ge11) ExtBus=0 Addr=0x0c */
        0x0d, /* Port 14 (ge12) ExtBus=0 Addr=0x0d */
        0x0e, /* Port 15 (ge13) ExtBus=0 Addr=0x0e */
        0x0f, /* Port 16 (ge14) ExtBus=0 Addr=0x0f */
        0x10, /* Port 17 (ge15) ExtBus=0 Addr=0x10 */
        0x11, /* Port 18 (ge16) ExtBus=0 Addr=0x11 */
        0x12, /* Port 19 (ge17) ExtBus=0 Addr=0x12 */
        0x13, /* Port 20 (ge18) ExtBus=0 Addr=0x13 */
        0x14, /* Port 21 (ge19) ExtBus=0 Addr=0x14 */
        0x15, /* Port 22 (ge20) ExtBus=0 Addr=0x15 */
        0x16, /* Port 23 (ge21) ExtBus=0 Addr=0x16 */
        0x17, /* Port 24 (ge22) ExtBus=0 Addr=0x17 */
        0x18, /* Port 25 (ge23) ExtBus=0 Addr=0x18 */
        0x19, /* Port 26 ( hg0) ExtBus=0 Addr=0x19 */
        0x1a, /* Port 27 ( hg1) ExtBus=0 Addr=0x1a */
        0x1b, /* Port 28 ( hg2) ExtBus=0 Addr=0x1b */
        0x1c, /* Port 29 ( hg3) ExtBus=0 Addr=0x1c */
    };

    uint16 dev_id;
    uint8 rev_id;

    soc_cm_get_id(unit, &dev_id, &rev_id);

    *phy_addr_int = _soc_enduro_int_phy_addr[port];
    switch (dev_id) {
    case BCM56331_DEVICE_ID:
    case BCM56333_DEVICE_ID:
    case BCM56334_DEVICE_ID:
    case BCM56338_DEVICE_ID:
    case BCM56320_DEVICE_ID:
    case BCM56321_DEVICE_ID:
    case BCM56132_DEVICE_ID:
    case BCM56134_DEVICE_ID:
    case BCM56230_DEVICE_ID:
    case BCM56231_DEVICE_ID:
        *phy_addr = _soc_phy_addr_bcm56334[port];
        break;
    }
}
#endif /* BCM_ENDURO_SUPPORT */

#if defined(BCM_HURRICANE_SUPPORT)
static void
_hurricane_phy_addr_default(int unit, int port,
                          uint16 *phy_addr, uint16 *phy_addr_int)
{
    static const uint16 _soc_hurricane_int_phy_addr[] = {
        0x00, /* Port  0 (cmic)         N/A */
        0x00, /* Port  1 (N/A)                */
        0x81, /* Port  2 (8SERDES_0)    IntBus=0 Addr=0x01 */
        0x81, /* Port  3 (8SERDES_0)    IntBus=0 Addr=0x02 */
        0x81, /* Port  4 (8SERDES_0)    IntBus=0 Addr=0x03 */
        0x81, /* Port  5 (8SERDES_0)    IntBus=0 Addr=0x04 */
        0x81, /* Port  6 (8SERDES_0)    IntBus=0 Addr=0x05 */
        0x81, /* Port  7 (8SERDES_0)    IntBus=0 Addr=0x06 */
        0x81, /* Port  8 (8SERDES_0)    IntBus=0 Addr=0x07 */
        0x81, /* Port  9 (8SERDES_0)    IntBus=0 Addr=0x08 */
        0x89, /* Port 10 (8SERDES_1)    IntBus=0 Addr=0x09 */
        0x89, /* Port 11 (8SERDES_1)    IntBus=0 Addr=0x0a */
        0x89, /* Port 12 (8SERDES_1)    IntBus=0 Addr=0x0b */
        0x89, /* Port 13 (8SERDES_1)    IntBus=0 Addr=0x0c */
        0x89, /* Port 14 (8SERDES_1)    IntBus=0 Addr=0x0d */
        0x89, /* Port 15 (8SERDES_1)    IntBus=0 Addr=0x0e */
        0x89, /* Port 16 (8SERDES_1)    IntBus=0 Addr=0x0f */
        0x89, /* Port 17 (8SERDES_1)    IntBus=0 Addr=0x10 */
        0x91, /* Port 18 (9SERDES)      IntBus=0 Addr=0x11 */
        0x91, /* Port 19 (9SERDES)      IntBus=0 Addr=0x12 */
        0x91, /* Port 20 (9SERDES)      IntBus=0 Addr=0x13 */
        0x91, /* Port 21 (9SERDES)      IntBus=0 Addr=0x14 */
        0x91, /* Port 22 (9SERDES)      IntBus=0 Addr=0x15 */
        0x91, /* Port 23 (9SERDES)      IntBus=0 Addr=0x16 */
        0x91, /* Port 24 (9SERDES)      IntBus=0 Addr=0x17 */
        0x91, /* Port 25 (9SERDES)      IntBus=0 Addr=0x18 */
        0x99, /* Port 26 (HC0)          IntBus=0 Addr=0x19 */
        0x99, /* Port 27 (HC0)          IntBus=0 Addr=0x19 */
        0x9a, /* Port 28 (HC1)          IntBus=0 Addr=0x1a */
        0x9a, /* Port 29 (HC1)          IntBus=0 Addr=0x1a */
    };

    static const uint16 _soc_phy_addr_bcm56142[] = {
        0x00, /* Port  0 (cmic) N/A */
        0x00, /* Port  1 ( N/A)                    */
        0x01, /* Port  2 ( ge0) ExtBus=0 Addr=0x01 */
        0x02, /* Port  3 ( ge1) ExtBus=0 Addr=0x02 */
        0x03, /* Port  4 ( ge2) ExtBus=0 Addr=0x03 */
        0x04, /* Port  5 ( ge3) ExtBus=0 Addr=0x04 */
        0x05, /* Port  6 ( ge4) ExtBus=0 Addr=0x05 */
        0x06, /* Port  7 ( ge5) ExtBus=0 Addr=0x06 */
        0x07, /* Port  8 ( ge6) ExtBus=0 Addr=0x07 */
        0x08, /* Port  9 ( ge7) ExtBus=0 Addr=0x08 */
        0x0a, /* Port 10 ( ge8) ExtBus=0 Addr=0x0a */
        0x0b, /* Port 11 ( ge9) ExtBus=0 Addr=0x0b */
        0x0c, /* Port 12 (ge10) ExtBus=0 Addr=0x0c */
        0x0d, /* Port 13 (ge11) ExtBus=0 Addr=0x0d */
        0x0e, /* Port 14 (ge12) ExtBus=0 Addr=0x0e */
        0x0f, /* Port 15 (ge13) ExtBus=0 Addr=0x0f */
        0x10, /* Port 16 (ge14) ExtBus=0 Addr=0x10 */
        0x11, /* Port 17 (ge15) ExtBus=0 Addr=0x11 */
        0x13, /* Port 18 (ge16) ExtBus=0 Addr=0x13 */
        0x14, /* Port 19 (ge17) ExtBus=0 Addr=0x14 */
        0x15, /* Port 20 (ge18) ExtBus=0 Addr=0x15 */
        0x16, /* Port 21 (ge19) ExtBus=0 Addr=0x16 */
        0x17, /* Port 22 (ge20) ExtBus=0 Addr=0x17 */
        0x18, /* Port 23 (ge21) ExtBus=0 Addr=0x18 */
        0x19, /* Port 24 (ge22) ExtBus=0 Addr=0x19 */
        0x1a, /* Port 25 (ge23) ExtBus=0 Addr=0x1a */
        0x1c, /* Port 26 ( hg0) ExtBus=0/1 Addr=0x1c */
        0,    /* Port 27 ( hg1) */
        0x1d, /* Port 28 ( hg2) ExtBus=0/1 Addr=0x1d */
        0,    /* Port 29 ( hg3) */
    };

    uint16 dev_id;
    uint8 rev_id;

    soc_cm_get_id(unit, &dev_id, &rev_id);

    *phy_addr_int = _soc_hurricane_int_phy_addr[port];
    switch (dev_id) {
    case BCM56140_DEVICE_ID:
    case BCM56142_DEVICE_ID:
    case BCM56143_DEVICE_ID:
    case BCM56144_DEVICE_ID:
    case BCM56146_DEVICE_ID:
    case BCM56147_DEVICE_ID:
    case BCM56149_DEVICE_ID:
        *phy_addr = _soc_phy_addr_bcm56142[port];
        break;
    }
    
}
#endif /* BCM_HURRICANE_SUPPORT */

#if defined(BCM_HURRICANE2_SUPPORT)
static void
_hurricane2_phy_addr_default(int unit, int port,
                          uint16 *phy_addr, uint16 *phy_addr_int)
{
    static const uint16 _soc_hurricane2_int_phy_addr[] = {
        0x00, /* Port  0 (cmic)         N/A */
        0x00, /* Port  1 (N/A)                */
        0x81, /* Port  2 (QSGMII2X_0)    IntBus=0 Addr=0x01 */
        0x81, /* Port  3 (QSGMII2X_0)    IntBus=0 Addr=0x02 */
        0x81, /* Port  4 (QSGMII2X_0)    IntBus=0 Addr=0x03 */
        0x81, /* Port  5 (QSGMII2X_0)    IntBus=0 Addr=0x04 */
        0x81, /* Port  6 (QSGMII2X_0)    IntBus=0 Addr=0x05 */
        0x81, /* Port  7 (QSGMII2X_0)    IntBus=0 Addr=0x06 */
        0x81, /* Port  8 (QSGMII2X_0)    IntBus=0 Addr=0x07 */
        0x81, /* Port  9 (QSGMII2X_0)    IntBus=0 Addr=0x08 */
        0x89, /* Port 10 (QSGMII2X_1)    IntBus=0 Addr=0x09 */
        0x89, /* Port 11 (QSGMII2X_1)    IntBus=0 Addr=0x0a */
        0x89, /* Port 12 (QSGMII2X_1)    IntBus=0 Addr=0x0b */
        0x89, /* Port 13 (QSGMII2X_1)    IntBus=0 Addr=0x0c */
        0x89, /* Port 14 (QSGMII2X_1)    IntBus=0 Addr=0x0d */
        0x89, /* Port 15 (QSGMII2X_1)    IntBus=0 Addr=0x0e */
        0x89, /* Port 16 (QSGMII2X_1)    IntBus=0 Addr=0x0f */
        0x89, /* Port 17 (QSGMII2X_1)    IntBus=0 Addr=0x10 */
        0xb1, /* Port 18 (QSGMII2X_2)    IntBus=1 Addr=0x11 */
        0xb1, /* Port 19 (QSGMII2X_2)    IntBus=1 Addr=0x12 */
        0xb1, /* Port 20 (QSGMII2X_2)    IntBus=1 Addr=0x13 */
        0xb1, /* Port 21 (QSGMII2X_2)    IntBus=1 Addr=0x14 */
        0xb1, /* Port 22 (QSGMII2X_2)    IntBus=1 Addr=0x15 */
        0xb1, /* Port 23 (QSGMII2X_2)    IntBus=1 Addr=0x16 */
        0xb1, /* Port 24 (QSGMII2X_2)    IntBus=1 Addr=0x17 */
        0xb1, /* Port 25 (QSGMII2X_2)    IntBus=1 Addr=0x18 */
        0xa1, /* Port 26 (TSC4_0)        IntBus=1 Addr=0x01 */
        0xa2, /* Port 27 (TSC4_0)        IntBus=1 Addr=0x02 */
        0xa3, /* Port 28 (TSC4_0)        IntBus=1 Addr=0x03 */
        0xa4, /* Port 29 (TSC4_0)        IntBus=1 Addr=0x04 */
        0xa5, /* Port 30 (TSC4_1)        IntBus=1 Addr=0x05 */
        0xa6, /* Port 31 (TSC4_1)        IntBus=1 Addr=0x06 */
        0xa7, /* Port 32 (TSC4_1)        IntBus=1 Addr=0x09 */
        0xa8  /* Port 33 (TSC4_1)        IntBus=1 Addr=0x08 */
    };

    static const uint16 _soc_phy_addr_bcm56150[] = {
        0x00, /* Port  0 (cmic) N/A */
        0x00, /* Port  1 ( N/A)                    */
        0x02, /* Port  2 ( ge0) ExtBus=0 Addr=0x02 */
        0x03, /* Port  3 ( ge1) ExtBus=0 Addr=0x03 */
        0x04, /* Port  4 ( ge2) ExtBus=0 Addr=0x04 */
        0x05, /* Port  5 ( ge3) ExtBus=0 Addr=0x05 */
        0x06, /* Port  6 ( ge4) ExtBus=0 Addr=0x06 */
        0x07, /* Port  7 ( ge5) ExtBus=0 Addr=0x07 */
        0x08, /* Port  8 ( ge6) ExtBus=0 Addr=0x08 */
        0x09, /* Port  9 ( ge7) ExtBus=0 Addr=0x09 */
        0x0b, /* Port 10 ( ge8) ExtBus=0 Addr=0x0b */
        0x0c, /* Port 11 ( ge9) ExtBus=0 Addr=0x0c */
        0x0d, /* Port 12 (ge10) ExtBus=0 Addr=0x0d */
        0x0e, /* Port 13 (ge11) ExtBus=0 Addr=0x0e */
        0x0f, /* Port 14 (ge12) ExtBus=0 Addr=0x0f */
        0x10, /* Port 15 (ge13) ExtBus=0 Addr=0x10 */
        0x11, /* Port 16 (ge14) ExtBus=0 Addr=0x11 */
        0x12, /* Port 17 (ge15) ExtBus=0 Addr=0x12 */
        0x14, /* Port 18 (ge16) ExtBus=0 Addr=0x14 */
        0x15, /* Port 19 (ge17) ExtBus=0 Addr=0x15 */
        0x16, /* Port 20 (ge18) ExtBus=0 Addr=0x16 */
        0x17, /* Port 21 (ge19) ExtBus=0 Addr=0x17 */
        0x18, /* Port 22 (ge20) ExtBus=0 Addr=0x18 */
        0x19, /* Port 23 (ge21) ExtBus=0 Addr=0x19 */
        0x1a, /* Port 24 (ge22) ExtBus=0 Addr=0x1a */
        0x1b, /* Port 25 (ge23) ExtBus=0 Addr=0x1b */
        0x21, /* Port 26 ( hg0) ExtBus=1 Addr=0x01 */
        0x22, /* Port 27 ( hg1) ExtBus=1 Addr=0x02 */
        0x23, /* Port 28 ( hg2) ExtBus=1 Addr=0x03 */
        0x24, /* Port 29 ( hg3) ExtBus=1 Addr=0x04 */
        0x25, /* Port 30 ( hg4) ExtBus=1 Addr=0x05 */
        0x26, /* Port 31 ( hg5) ExtBus=1 Addr=0x06 */
        0x27, /* Port 32 ( hg6) ExtBus=1 Addr=0x07 */
        0x28  /* Port 33 ( hg7) ExtBus=1 Addr=0x08 */
    };

    static const uint16 _soc_phy_addr_bcm5339x[] = {
        0x00, /* Port  0 (cmic) N/A */
        0x00, /* Port  1 ( N/A)                    */
        0x02, /* Port  2 ( ge0) ExtBus=0 Addr=0x02 */
        0x00, /* Port  3 ( N/A) */
        0x00, /* Port  4 ( N/A) */
        0x00, /* Port  5 ( N/A) */
        0x03, /* Port  6 ( ge1) ExtBus=0 Addr=0x03 */
        0x00, /* Port  7 ( N/A) */
        0x00, /* Port  8 ( N/A) */
        0x00, /* Port  9 ( N/A) */
        0x04, /* Port 10 ( ge2) ExtBus=0 Addr=0x04 */
        0x00, /* Port 11 ( N/A) */
        0x00, /* Port 12 ( N/A) */
        0x00, /* Port 13 ( N/A) */
        0x05, /* Port 14 ( ge3) ExtBus=0 Addr=0x05 */
        0x00, /* Port 15 ( N/A) */
        0x00, /* Port 16 ( N/A) */
        0x00, /* Port 17 ( N/A) */
        0x06, /* Port 18 ( ge4) ExtBus=0 Addr=0x06 */
        0x00, /* Port 19 ( N/A) */
        0x00, /* Port 20 ( N/A) */
        0x00, /* Port 21 ( N/A) */
        0x07, /* Port 22 ( ge5) ExtBus=0 Addr=0x07 */
        0x00, /* Port 23 ( N/A) */
        0x00, /* Port 24 ( N/A) */
        0x00, /* Port 25 ( N/A) */
        0x00, /* Port 26 ( N/A) */
        0x00, /* Port 27 ( N/A) */
        0x00, /* Port 28 ( N/A) */
        0x00, /* Port 29 ( N/A) */
        0x00, /* Port 30 ( N/A) */
        0x00, /* Port 31 ( N/A) */
        0x00, /* Port 32 ( N/A) */
        0x00  /* Port 33 ( N/A) */
    };

    static const uint16 _soc_hurricane2_gphy_addr[] = {
        0x00, /* Port  0 (cmic)         N/A */
        0x00, /* Port  1 (N/A)                */
        0x81, /* Port  2 (QGPHY_0)    IntBus=0 Addr=0x01 */
        0x82, /* Port  3 (QGPHY_0)    IntBus=0 Addr=0x02 */
        0x83, /* Port  4 (QGPHY_0)    IntBus=0 Addr=0x03 */
        0x84, /* Port  5 (QGPHY_0)    IntBus=0 Addr=0x04 */
        0x86, /* Port  6 (QGPHY_1)    IntBus=0 Addr=0x06 */
        0x87, /* Port  7 (QGPHY_1)    IntBus=0 Addr=0x07 */
        0x88, /* Port  8 (QGPHY_1)    IntBus=0 Addr=0x08 */
        0x89, /* Port  9 (QGPHY_1)    IntBus=0 Addr=0x09 */
        0x8b, /* Port 10 (QGPHY_2)    IntBus=0 Addr=0x0b */
        0x8c, /* Port 11 (QGPHY_2)    IntBus=0 Addr=0x0c */
        0x8d, /* Port 12 (QGPHY_2)    IntBus=0 Addr=0x0d */
        0x8e, /* Port 13 (QGPHY_2)    IntBus=0 Addr=0x0e */
        0x90, /* Port 14 (QGPHY_3)    IntBus=0 Addr=0x10 */
        0x91, /* Port 15 (QGPHY_3)    IntBus=0 Addr=0x11 */
        0x92, /* Port 16 (QGPHY_3)    IntBus=0 Addr=0x12 */
        0x93  /* Port 17 (QGPHY_3)    IntBus=0 Addr=0x13 */
    };

    uint16 dev_id;
    uint8 rev_id;
    int phy_port;

    soc_cm_get_id(unit, &dev_id, &rev_id);

    phy_port = SOC_INFO(unit).port_l2p_mapping[port];
    *phy_addr_int = _soc_hurricane2_int_phy_addr[phy_port];
    switch (dev_id) {
        case BCM56150_DEVICE_ID:
        case BCM56151_DEVICE_ID:
        case BCM56152_DEVICE_ID:
        case BCM53342_DEVICE_ID:
        case BCM53343_DEVICE_ID:
        case BCM53344_DEVICE_ID:
        case BCM53346_DEVICE_ID:
        case BCM53347_DEVICE_ID:
        case BCM53333_DEVICE_ID:
        case BCM53334_DEVICE_ID:
        *phy_addr = _soc_phy_addr_bcm56150[phy_port];
        break;
        case BCM53393_DEVICE_ID:
        case BCM53394_DEVICE_ID:
        *phy_addr = _soc_phy_addr_bcm5339x[phy_port];
        break;
    }
    if ( soc_feature(unit, soc_feature_gphy) && (phy_port < 18)) {
        *phy_addr_int = 0;
        *phy_addr = _soc_hurricane2_gphy_addr[phy_port];
    }
}
#endif /* BCM_HURRICANE2_SUPPORT */


#if defined(BCM_HELIX4_SUPPORT) 

static void
_helix4_phy_addr_default(int unit, int port,
                          uint16 *phy_addr, uint16 *phy_addr_int)
{
    static const uint16 _soc_helix4_int_phy_addr[] = {
        0x80, /* Port   0 (cmic) N/A */

        0x81, /* Port   1 (QC0) IntBus=0 Addr=0x01 */
        0x81, /* Port   2 (QC0) IntBus=0 Addr=0x02 */
        0x81, /* Port   3 (QC0) IntBus=0 Addr=0x03 */
        0x81, /* Port   4 (QC0) IntBus=0 Addr=0x04 */
        0x81, /* Port   5 (QC0) IntBus=0 Addr=0x05 */
        0x81, /* Port   6 (QC0) IntBus=0 Addr=0x06 */
        0x81, /* Port   7 (QC0) IntBus=0 Addr=0x07 */
        0x81, /* Port   8 (QC0) IntBus=0 Addr=0x08 */

        0x89, /* Port   9 (QC1) IntBus=0 Addr=0x09 */
        0x89, /* Port  10 (QC1) IntBus=0 Addr=0x0A */
        0x89, /* Port  11 (QC1) IntBus=0 Addr=0x0B */
        0x89, /* Port  12 (QC1) IntBus=0 Addr=0x0C */
        0x89, /* Port  13 (QC1) IntBus=0 Addr=0x0D */
        0x89, /* Port  14 (QC1) IntBus=0 Addr=0x0E */
        0x89, /* Port  15 (QC1) IntBus=0 Addr=0x0F */
        0x89, /* Port  16 (QC1) IntBus=0 Addr=0x10 */

        0x91, /* Port  17 (QC2) IntBus=0 Addr=0x11 */
        0x91, /* Port  18 (QC2) IntBus=0 Addr=0x12 */
        0x91, /* Port  19 (QC2) IntBus=0 Addr=0x13 */
        0x91, /* Port  20 (QC2) IntBus=0 Addr=0x14 */
        0x91, /* Port  21 (QC2) IntBus=0 Addr=0x15 */
        0x91, /* Port  22 (QC2) IntBus=0 Addr=0x16 */
        0x91, /* Port  23 (QC2) IntBus=0 Addr=0x17 */
        0x91, /* Port  24 (QC2) IntBus=0 Addr=0x18 */

        0xA1, /* Port  25 (QC3) IntBus=1 Addr=0x01 */
        0xA1, /* Port  26 (QC3) IntBus=1 Addr=0x02 */
        0xA1, /* Port  27 (QC3) IntBus=1 Addr=0x03 */
        0xA1, /* Port  28 (QC3) IntBus=1 Addr=0x04 */
        0xA1, /* Port  29 (QC3) IntBus=1 Addr=0x05 */
        0xA1, /* Port  30 (QC3) IntBus=1 Addr=0x06 */
        0xA1, /* Port  31 (QC3) IntBus=1 Addr=0x07 */
        0xA1, /* Port  32 (QC3) IntBus=1 Addr=0x08 */

        0xA9, /* Port  33 (QC4) IntBus=1 Addr=0x09 */
        0xA9, /* Port  34 (QC4) IntBus=1 Addr=0x0a */
        0xA9, /* Port  35 (QC4) IntBus=1 Addr=0x0b */
        0xA9, /* Port  36 (QC4) IntBus=1 Addr=0x0c */
        0xA9, /* Port  37 (QC4) IntBus=1 Addr=0x0d */
        0xA9, /* Port  38 (QC4) IntBus=1 Addr=0x0e */
        0xA9, /* Port  39 (QC4) IntBus=1 Addr=0x0f */
        0xA9, /* Port  40 (QC4) IntBus=1 Addr=0x10 */

        0xB1, /* Port  41 (QC5) IntBus=1 Addr=0x11 */
        0xB1, /* Port  42 (QC5) IntBus=1 Addr=0x12 */
        0xB1, /* Port  43 (QC5) IntBus=1 Addr=0x13 */
        0xB1, /* Port  44 (QC5) IntBus=1 Addr=0x14 */
        0xB1, /* Port  45 (QC5) IntBus=1 Addr=0x15 */
        0xB1, /* Port  46 (QC5) IntBus=1 Addr=0x16 */
        0xB1, /* Port  47 (QC5) IntBus=1 Addr=0x17 */
        0xB1, /* Port  48 (QC5) IntBus=1 Addr=0x18 */

        0xE3, /* Port  49 (QSG) IntBus=3 Addr=0x03 */
        0xE3, /* Port  50 (---) IntBus=3 Addr=0x04 */
        0xE3, /* Port  51 (---) IntBus= Addr=0x */
        0xE3, /* Port  52 (---) IntBus= Addr=0x */

        0xC1, /* Port  53 (WC0) IntBus=2 Addr=0x01 */
        0xC1, /* Port  54 (WC0) IntBus=2 Addr=0x02 */
        0xC1, /* Port  55 (WC0) IntBus=2 Addr=0x03 */
        0xC1, /* Port  56 (WC0) IntBus=2 Addr=0x04 */

        0xC5, /* Port  57 (WC1) IntBus=2 Addr=0x05 */
        0xC5, /* Port  58 (WC1) IntBus=2 Addr=0x06 */
        0xC5, /* Port  59 (WC1) IntBus=2 Addr=0x07 */
        0xC5, /* Port  60 (WC1) IntBus=2 Addr=0x08 */

        0xC9, /* Port  61 (WC2) IntBus=2 Addr=0x09 */
        0xC9, /* Port  62 (WC2) IntBus=2 Addr=0x0A */
        0xC9, /* Port  63 (WC2) IntBus=2 Addr=0x0B */
        0xC9, /* Port  64 (WC2) IntBus=2 Addr=0x0C */
    };

    static const uint16 _soc_phy_addr_bcm56340[] = {

        0x00, /* Port   0 (cmic) N/A */

        0x01, /* Port   1 (QC0) ExtBus=0 Addr=0x01 */
        0x02, /* Port   2 (QC0) ExtBus=0 Addr=0x02 */
        0x03, /* Port   3 (QC0) ExtBus=0 Addr=0x03 */
        0x04, /* Port   4 (QC0) ExtBus=0 Addr=0x04 */
        0x05, /* Port   5 (QC0) ExtBus=0 Addr=0x05 */
        0x06, /* Port   6 (QC0) ExtBus=0 Addr=0x06 */
        0x07, /* Port   7 (QC0) ExtBus=0 Addr=0x07 */
        0x08, /* Port   8 (QC0) ExtBus=0 Addr=0x08 */

        0x0a, /* Port   9 (QC1) ExtBus=0 Addr=0x09 */
        0x0b, /* Port  10 (QC1) ExtBus=0 Addr=0x0A */
        0x0c, /* Port  11 (QC1) ExtBus=0 Addr=0x0B */
        0x0d, /* Port  12 (QC1) ExtBus=0 Addr=0x0C */
        0x0e, /* Port  13 (QC1) ExtBus=0 Addr=0x0D */
        0x0f, /* Port  14 (QC1) ExtBus=0 Addr=0x0E */
        0x10, /* Port  15 (QC1) ExtBus=0 Addr=0x0F */
        0x11, /* Port  16 (QC1) ExtBus=0 Addr=0x10 */

        0x13, /* Port  17 (QC2) ExtBus=0 Addr=0x11 */
        0x14, /* Port  18 (QC2) ExtBus=0 Addr=0x12 */
        0x15, /* Port  19 (QC2) ExtBus=0 Addr=0x13 */
        0x16, /* Port  20 (QC2) ExtBus=0 Addr=0x14 */
        0x17, /* Port  21 (QC2) ExtBus=0 Addr=0x15 */
        0x18, /* Port  22 (QC2) ExtBus=0 Addr=0x16 */
        0x19, /* Port  23 (QC2) ExtBus=0 Addr=0x17 */
        0x1a, /* Port  24 (QC2) ExtBus=0 Addr=0x18 */

        0x21, /* Port  25 (QC3) ExtBus=1 Addr=0x01 */
        0x22, /* Port  26 (QC3) ExtBus=1 Addr=0x02 */
        0x23, /* Port  27 (QC3) ExtBus=1 Addr=0x03 */
        0x24, /* Port  28 (QC3) ExtBus=1 Addr=0x04 */
        0x25, /* Port  29 (QC3) ExtBus=1 Addr=0x05 */
        0x26, /* Port  30 (QC3) ExtBus=1 Addr=0x06 */
        0x27, /* Port  31 (QC3) ExtBus=1 Addr=0x07 */
        0x28, /* Port  32 (QC3) ExtBus=1 Addr=0x08 */

        0x2a, /* Port  33 (QC4) ExtBus=1 Addr=0x09 */
        0x2b, /* Port  34 (QC4) ExtBus=1 Addr=0x0a */
        0x2c, /* Port  35 (QC4) ExtBus=1 Addr=0x0b */
        0x2d, /* Port  36 (QC4) ExtBus=1 Addr=0x0c */
        0x2e, /* Port  37 (QC4) ExtBus=1 Addr=0x0d */
        0x2f, /* Port  38 (QC4) ExtBus=1 Addr=0x0e */
        0x30, /* Port  39 (QC4) ExtBus=1 Addr=0x0f */
        0x31, /* Port  40 (QC4) ExtBus=1 Addr=0x10 */

        0x33, /* Port  41 (QC5) ExtBus=1 Addr=0x11 */
        0x34, /* Port  42 (QC5) ExtBus=1 Addr=0x12 */
        0x35, /* Port  43 (QC5) ExtBus=1 Addr=0x13 */
        0x36, /* Port  44 (QC5) ExtBus=1 Addr=0x14 */
        0x37, /* Port  45 (QC5) ExtBus=1 Addr=0x15 */
        0x38, /* Port  46 (QC5) ExtBus=1 Addr=0x16 */
        0x39, /* Port  47 (QC5) ExtBus=1 Addr=0x17 */
        0x3a, /* Port  48 (QC5) ExtBus=1 Addr=0x18 */

        0x63, /* Port  49 (QSG) ExtBus=3 Addr=0x03 */
        0x63, /* Port  50 (---) ExtBus=3 Addr=0x04 */
        0x63, /* Port  51 (---) ExtBus= Addr=0x */
        0x63, /* Port  52 (---) ExtBus= Addr=0x */

        0x41, /* Port  53 (WC0) ExtBus=2 Addr=0x01 */
        0x42, /* Port  54 (WC0) ExtBus=2 Addr=0x02 */
        0x43, /* Port  55 (WC0) ExtBus=2 Addr=0x03 */
        0x44, /* Port  56 (WC0) ExtBus=2 Addr=0x04 */

        0x45, /* Port  57 (WC1) ExtBus=2 Addr=0x05 */
        0x46, /* Port  58 (WC1) ExtBus=2 Addr=0x06 */
        0x47, /* Port  59 (WC1) ExtBus=2 Addr=0x07 */
        0x48, /* Port  60 (WC1) ExtBus=2 Addr=0x08 */

        0x49, /* Port  61 (WC2) ExtBus=2 Addr=0x09 */
        0x4a, /* Port  62 (WC2) ExtBus=2 Addr=0x0A */
        0x4b, /* Port  63 (WC2) ExtBus=2 Addr=0x0B */
        0x4c, /* Port  64 (WC2) ExtBus=2 Addr=0x0C */
    };
    uint16 dev_id;
    uint8 rev_id;
    int phy_port;

    soc_cm_get_id(unit, &dev_id, &rev_id);

    phy_port = SOC_INFO(unit).port_l2p_mapping[port];

    *phy_addr_int = _soc_helix4_int_phy_addr[phy_port];

    switch (dev_id) {
    case BCM56548_DEVICE_ID:
    case BCM56547_DEVICE_ID:
    case BCM56344_DEVICE_ID:
    case BCM56342_DEVICE_ID:
    case BCM56340_DEVICE_ID:
    case BCM56049_DEVICE_ID:
    case BCM56048_DEVICE_ID:
    case BCM56047_DEVICE_ID:
    case BCM56042_DEVICE_ID:
    case BCM56041_DEVICE_ID:
    case BCM56040_DEVICE_ID:
        *phy_addr = _soc_phy_addr_bcm56340[phy_port];
        break;
    default:
        *phy_addr = _soc_phy_addr_bcm56340[phy_port];
        break;
    }
}
#endif /* BCM_HELIX4_SUPPORT */

#if defined(BCM_TRIUMPH3_SUPPORT) 

static void
_triumph3_phy_addr_default(int unit, int port,
                          uint16 *phy_addr, uint16 *phy_addr_int)
{
    static const uint16 _soc_triumph3_int_phy_addr[] = {
        0x80, /* Port   0 (cmic) N/A */
        0x81, /* Port   1 (XC0) IntBus=0 Addr=0x01 */
        0x82, /* Port   2 (XC0) IntBus=0 Addr=0x02 */
        0x83, /* Port   3 (XC0) IntBus=0 Addr=0x03 */
        0x84, /* Port   4 (XC0) IntBus=0 Addr=0x04 */
        0x85, /* Port   5 (XC1) IntBus=0 Addr=0x05 */
        0x86, /* Port   6 (XC1) IntBus=0 Addr=0x06 */
        0x87, /* Port   7 (XC1) IntBus=0 Addr=0x07 */
        0x88, /* Port   8 (XC1) IntBus=0 Addr=0x08 */
        0x89, /* Port   9 (XC2) IntBus=0 Addr=0x09 */
        0x8A, /* Port  10 (XC2) IntBus=0 Addr=0x0A */
        0x8B, /* Port  11 (XC2) IntBus=0 Addr=0x0B */
        0x8C, /* Port  12 (XC2) IntBus=0 Addr=0x0C */
        0x8D, /* Port  13 (XC3) IntBus=0 Addr=0x0D */
        0x8E, /* Port  14 (XC3) IntBus=0 Addr=0x0E */
        0x8F, /* Port  15 (XC3) IntBus=0 Addr=0x0F */
        0x90, /* Port  16 (XC3) IntBus=0 Addr=0x10 */
        0x91, /* Port  17 (XC4) IntBus=0 Addr=0x11 */
        0x92, /* Port  18 (XC4) IntBus=0 Addr=0x12 */
        0x93, /* Port  19 (XC4) IntBus=0 Addr=0x13 */
        0x94, /* Port  20 (XC4) IntBus=0 Addr=0x14 */
        0x95, /* Port  21 (XC5) IntBus=0 Addr=0x15 */
        0x96, /* Port  22 (XC5) IntBus=0 Addr=0x16 */
        0x97, /* Port  23 (XC5) IntBus=0 Addr=0x17 */
        0x98, /* Port  24 (XC5) IntBus=0 Addr=0x18 */
        0x99, /* Port  25 (XC6) IntBus=0 Addr=0x19 */
        0x9A, /* Port  26 (XC6) IntBus=0 Addr=0x1A */
        0x9B, /* Port  27 (XC6) IntBus=0 Addr=0x1B */
        0x9C, /* Port  28 (XC6) IntBus=0 Addr=0x1C */

        0xA1, /* Port  29 (XC7) IntBus=1 Addr=0x01 */
        0xA2, /* Port  30 (XC7) IntBus=1 Addr=0x02 */
        0xA3, /* Port  31 (XC7) IntBus=1 Addr=0x03 */
        0xA4, /* Port  32 (XC7) IntBus=1 Addr=0x04 */
        0xA5, /* Port  33 (XC8) IntBus=1 Addr=0x05 */
        0xA6, /* Port  34 (XC8) IntBus=1 Addr=0x06 */
        0xA7, /* Port  35 (XC8) IntBus=1 Addr=0x07 */
        0xA8, /* Port  36 (XC8) IntBus=1 Addr=0x08 */
        0xA9, /* Port  37 (XC9) IntBus=1 Addr=0x09 */
        0,    /* Port  38 (---) IntBus=1 Addr=0 */
        0,    /* Port  39 (---) IntBus=1 Addr=0 */
        0,    /* Port  40 (---) IntBus=1 Addr=0 */
        0xAD, /* Port  41 (XC10) IntBus=1 Addr=0x0D */
        0xAE, /* Port  42 (XC10) IntBus=1 Addr=0x0E */
        0xAF, /* Port  43 (XC10) IntBus=1 Addr=0x0F */
        0xB0, /* Port  44 (XC10) IntBus=1 Addr=0x10 */
        0xB1, /* Port  45 (XC11) IntBus=1 Addr=0x11 */
        0xB2, /* Port  46 (XC11) IntBus=1 Addr=0x12 */
        0xB3, /* Port  47 (XC11) IntBus=1 Addr=0x13 */
        0xB4, /* Port  48 (XC11) IntBus=1 Addr=0x14 */
        0xB5, /* Port  49 (XC12) IntBus=1 Addr=0x15 */
        0xB6, /* Port  50 (XC12) IntBus=1 Addr=0x16 */
        0xB7, /* Port  51 (XC12) IntBus=1 Addr=0x17 */
        0xB8, /* Port  52 (XC12) IntBus=1 Addr=0x18 */

#ifdef TR3_WCMOD_MULTIPORT_SUPPORT
        
        0xC1, /* Port  53 (WC0) IntBus=2 Addr=0x01 */
        0xC2, /* Port  54 (WC0) IntBus=2 Addr=0x02 */
        0xC3, /* Port  55 (WC0) IntBus=2 Addr=0x03 */
        0xC4, /* Port  56 (WC0) IntBus=2 Addr=0x04 */
        0xC5, /* Port  57 (WC1) IntBus=2 Addr=0x05 */
        0xC6, /* Port  58 (WC1) IntBus=2 Addr=0x06 */
        0xC7, /* Port  59 (WC1) IntBus=2 Addr=0x07 */
        0xC8, /* Port  60 (WC1) IntBus=2 Addr=0x08 */
        0xC9, /* Port  61 (WC2) IntBus=2 Addr=0x09 */
        0xCA, /* Port  62 (WC2) IntBus=2 Addr=0x0A */
        0xCB, /* Port  63 (WC2) IntBus=2 Addr=0x0B */
        0xCC, /* Port  64 (WC2) IntBus=2 Addr=0x0C */
        0xCD, /* Port  65 (WC3) IntBus=2 Addr=0x0D */
        0xCE, /* Port  66 (WC3) IntBus=2 Addr=0x0E */
        0xCF, /* Port  67 (WC3) IntBus=2 Addr=0x0F */
        0xD0, /* Port  68 (WC3) IntBus=2 Addr=0x10 */
        0xD1, /* Port  69 (WC4) IntBus=2 Addr=0x11 */
        0xD2, /* Port  70 (WC4) IntBus=2 Addr=0x12 */
        0xD3, /* Port  71 (WC4) IntBus=2 Addr=0x13 */
        0xD4, /* Port  72 (WC4) IntBus=2 Addr=0x14 */
        0xD5, /* Port  73 (WC5) IntBus=2 Addr=0x15 */
        0xD6, /* Port  74 (WC5) IntBus=2 Addr=0x16 */
        0xD7, /* Port  75 (WC5) IntBus=2 Addr=0x17 */
        0xD8, /* Port  76 (WC5) IntBus=2 Addr=0x18 */
        0xD9, /* Port  77 (WC6) IntBus=2 Addr=0x19 */
        0xDA, /* Port  78 (WC6) IntBus=2 Addr=0x1A */
        0xDB, /* Port  79 (WC6) IntBus=2 Addr=0x1B */
        0xDC, /* Port  80 (WC6) IntBus=2 Addr=0x1C */
#else
        0xC1, /* Port  53 (WC0) IntBus=2 Addr=0x01 */
        0xC1, /* Port  54 (WC0) IntBus=2 Addr=0x02 */
        0xC1, /* Port  55 (WC0) IntBus=2 Addr=0x03 */
        0xC1, /* Port  56 (WC0) IntBus=2 Addr=0x04 */
        0xC5, /* Port  57 (WC1) IntBus=2 Addr=0x05 */
        0xC5, /* Port  58 (WC1) IntBus=2 Addr=0x06 */
        0xC5, /* Port  59 (WC1) IntBus=2 Addr=0x07 */
        0xC5, /* Port  60 (WC1) IntBus=2 Addr=0x08 */
        0xC9, /* Port  61 (WC2) IntBus=2 Addr=0x09 */
        0xC9, /* Port  62 (WC2) IntBus=2 Addr=0x0A */
        0xC9, /* Port  63 (WC2) IntBus=2 Addr=0x0B */
        0xC9, /* Port  64 (WC2) IntBus=2 Addr=0x0C */
        0xCD, /* Port  65 (WC3) IntBus=2 Addr=0x0D */
        0xCD, /* Port  66 (WC3) IntBus=2 Addr=0x0E */
        0xCD, /* Port  67 (WC3) IntBus=2 Addr=0x0F */
        0xCD, /* Port  68 (WC3) IntBus=2 Addr=0x10 */
        0xD1, /* Port  69 (WC4) IntBus=2 Addr=0x11 */
        0xD1, /* Port  70 (WC4) IntBus=2 Addr=0x12 */
        0xD1, /* Port  71 (WC4) IntBus=2 Addr=0x13 */
        0xD1, /* Port  72 (WC4) IntBus=2 Addr=0x14 */
        0xD5, /* Port  73 (WC5) IntBus=2 Addr=0x15 */
        0xD5, /* Port  74 (WC5) IntBus=2 Addr=0x16 */
        0xD5, /* Port  75 (WC5) IntBus=2 Addr=0x17 */
        0xD5, /* Port  76 (WC5) IntBus=2 Addr=0x18 */
        0xD9, /* Port  77 (WC6) IntBus=2 Addr=0x19 */
        0xD9, /* Port  78 (WC6) IntBus=2 Addr=0x1A */
        0xD9, /* Port  79 (WC6) IntBus=2 Addr=0x1B */
        0xD9, /* Port  80 (WC6) IntBus=2 Addr=0x1C */
#endif /* TR3_WCMOD_MULTIPORT_SUPPORT */
    };

    static const uint16 _soc_phy_addr_bcm56640[] = {    
        0x00, /* Port   0 (cmic) N/A */
        0x01, /* Port   1 (XC0) ExtBus=0 Addr=0x01 */
        0x02, /* Port   2 (XC0) ExtBus=0 Addr=0x02 */
        0x03, /* Port   3 (XC0) ExtBus=0 Addr=0x03 */
        0x04, /* Port   4 (XC0) ExtBus=0 Addr=0x04 */
        0x05, /* Port   5 (XC1) ExtBus=0 Addr=0x05 */
        0x06, /* Port   6 (XC1) ExtBus=0 Addr=0x06 */
        0x07, /* Port   7 (XC1) ExtBus=0 Addr=0x07 */
        0x08, /* Port   8 (XC1) ExtBus=0 Addr=0x08 */
        0x09, /* Port   9 (XC2) ExtBus=0 Addr=0x09 */
        0x0A, /* Port  10 (XC2) ExtBus=0 Addr=0x0A */
        0x0B, /* Port  11 (XC2) ExtBus=0 Addr=0x0B */
        0x0C, /* Port  12 (XC2) ExtBus=0 Addr=0x0C */
        0x0D, /* Port  13 (XC3) ExtBus=0 Addr=0x0D */
        0x0E, /* Port  14 (XC3) ExtBus=0 Addr=0x0E */
        0x0F, /* Port  15 (XC3) ExtBus=0 Addr=0x0F */
        0x10, /* Port  16 (XC3) ExtBus=0 Addr=0x10 */
        0x11, /* Port  17 (XC4) ExtBus=0 Addr=0x11 */
        0x12, /* Port  18 (XC4) ExtBus=0 Addr=0x12 */
        0x13, /* Port  19 (XC4) ExtBus=0 Addr=0x13 */
        0x14, /* Port  20 (XC4) ExtBus=0 Addr=0x14 */
        0x15, /* Port  21 (XC5) ExtBus=0 Addr=0x15 */
        0x16, /* Port  22 (XC5) ExtBus=0 Addr=0x16 */
        0x17, /* Port  23 (XC5) ExtBus=0 Addr=0x17 */
        0x18, /* Port  24 (XC5) ExtBus=0 Addr=0x18 */

        0x21, /* Port  25 (XC6) ExtBus=1 Addr=0x01 */
        0x22, /* Port  26 (XC6) ExtBus=1 Addr=0x02 */
        0x23, /* Port  27 (XC6) ExtBus=1 Addr=0x03 */
        0x24, /* Port  28 (XC6) ExtBus=1 Addr=0x04 */
        0x25, /* Port  29 (XC7) ExtBus=1 Addr=0x05 */
        0x26, /* Port  30 (XC7) ExtBus=1 Addr=0x06 */
        0x27, /* Port  31 (XC7) ExtBus=1 Addr=0x07 */
        0x28, /* Port  32 (XC7) ExtBus=1 Addr=0x08 */
        0x29, /* Port  33 (XC8) ExtBus=1 Addr=0x09 */
        0x2A, /* Port  34 (XC8) ExtBus=1 Addr=0x0A */
        0x2B, /* Port  35 (XC8) ExtBus=1 Addr=0x0B */
        0x2C, /* Port  36 (XC8) ExtBus=1 Addr=0x0C */
        0x1D, /* Port  37 (XC9) ExtBus=0 Addr=0x1d */
        0,    /* Port  38 (---) ExtBus=1 Addr=0 */
        0,    /* Port  39 (---) ExtBus=1 Addr=0 */
        0,    /* Port  40 (---) ExtBus=1 Addr=0 */
        0x2D, /* Port  41 (XC10) ExtBus=1 Addr=0x0D */
        0x2E, /* Port  42 (XC10) ExtBus=1 Addr=0x0E */
        0x2F, /* Port  43 (XC10) ExtBus=1 Addr=0x0F */
        0x30, /* Port  44 (XC10) ExtBus=1 Addr=0x10 */
        0x31, /* Port  45 (XC11) ExtBus=1 Addr=0x11 */
        0x32, /* Port  46 (XC11) ExtBus=1 Addr=0x12 */
        0x33, /* Port  47 (XC11) ExtBus=1 Addr=0x13 */
        0x34, /* Port  48 (XC11) ExtBus=1 Addr=0x14 */
        0x35, /* Port  49 (XC12) ExtBus=1 Addr=0x15 */
        0x36, /* Port  50 (XC12) ExtBus=1 Addr=0x16 */
        0x37, /* Port  51 (XC12) ExtBus=1 Addr=0x17 */
        0x38, /* Port  52 (XC12) ExtBus=1 Addr=0x18 */

        0x41, /* Port  53 (WC0) ExtBus=2 Addr=0x01 */
        0x42, /* Port  54 (WC0) ExtBus=2 Addr=0x02 */
        0x43, /* Port  55 (WC0) ExtBus=2 Addr=0x03 */
        0x44, /* Port  56 (WC0) ExtBus=2 Addr=0x04 */
        0x45, /* Port  57 (WC1) ExtBus=2 Addr=0x05 */
        0x46, /* Port  58 (WC1) ExtBus=2 Addr=0x06 */
        0x47, /* Port  59 (WC1) ExtBus=2 Addr=0x07 */
        0x48, /* Port  60 (WC1) ExtBus=2 Addr=0x08 */
        0x49, /* Port  61 (WC2) ExtBus=2 Addr=0x09 */
        0x4A, /* Port  62 (WC2) ExtBus=2 Addr=0x0A */
        0x4B, /* Port  63 (WC2) ExtBus=2 Addr=0x0B */
        0x4C, /* Port  64 (WC2) ExtBus=2 Addr=0x0C */
        0x4D, /* Port  65 (WC3) ExtBus=2 Addr=0x0D */
        0x4E, /* Port  66 (WC3) ExtBus=2 Addr=0x0E */
        0x4F, /* Port  67 (WC3) ExtBus=2 Addr=0x0F */
        0x50, /* Port  68 (WC3) ExtBus=2 Addr=0x10 */
        0x51, /* Port  69 (WC4) ExtBus=2 Addr=0x11 */
        0x52, /* Port  70 (WC4) ExtBus=2 Addr=0x12 */

        0x53, /* Port  71 (WC4) ExtBus=2 Addr=0x13 */
        0x54, /* Port  72 (WC4) ExtBus=2 Addr=0x14 */
        0x55, /* Port  73 (WC5) ExtBus=2 Addr=0x15 */
        0x56, /* Port  74 (WC5) ExtBus=2 Addr=0x16 */
        0x57, /* Port  75 (WC5) ExtBus=2 Addr=0x17 */
        0x58, /* Port  76 (WC5) ExtBus=2 Addr=0x18 */
        0x59, /* Port  77 (WC6) ExtBus=2 Addr=0x19 */
        0x5A, /* Port  78 (WC6) ExtBus=2 Addr=0x1A */
        0x5B, /* Port  79 (WC6) ExtBus=2 Addr=0x1B */
        0x5C, /* Port  80 (WC6) ExtBus=2 Addr=0x1C */
    };
    uint16 dev_id;
    uint8 rev_id;
    int phy_port;

    soc_cm_get_id(unit, &dev_id, &rev_id);

    phy_port = SOC_INFO(unit).port_l2p_mapping[port];

    *phy_addr_int = _soc_triumph3_int_phy_addr[phy_port];

    switch (dev_id) {
    case BCM56640_DEVICE_ID:
    
    case BCM56643_DEVICE_ID: 
    case BCM56644_DEVICE_ID:
    case BCM56649_DEVICE_ID:
    case BCM56540_DEVICE_ID:
    case BCM56541_DEVICE_ID:
    case BCM56542_DEVICE_ID:
    case BCM56544_DEVICE_ID:
    case BCM56545_DEVICE_ID:
    case BCM56546_DEVICE_ID:
        *phy_addr = _soc_phy_addr_bcm56640[phy_port];
        break;
    default:
        
        *phy_addr = _soc_phy_addr_bcm56640[phy_port];
        break;
    }
}
#endif /* BCM_TRIUMPH3_SUPPORT */

#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_APOLLO_SUPPORT) || \
    defined(BCM_VALKYRIE2_SUPPORT)
static void
_triumph2_phy_addr_default(int unit, int port,
                          uint16 *phy_addr, uint16 *phy_addr_int)
{
    static const uint16 _soc_triumph2_int_phy_addr[] = {
        0x00, /* Port  0 (cmic)         N/A */
        0x99, /* Port  1 (9SERDES)      IntBus=0 Addr=0x19 */
        0x81, /* Port  2 (8SERDES_0)    IntBus=0 Addr=0x01 */
        0x82, /* Port  3 (8SERDES_0)    IntBus=0 Addr=0x02 */
        0x83, /* Port  4 (8SERDES_0)    IntBus=0 Addr=0x03 */
        0x84, /* Port  5 (8SERDES_0)    IntBus=0 Addr=0x04 */
        0x85, /* Port  6 (8SERDES_0)    IntBus=0 Addr=0x05 */
        0x86, /* Port  7 (8SERDES_0)    IntBus=0 Addr=0x06 */
        0x87, /* Port  8 (8SERDES_0)    IntBus=0 Addr=0x07 */
        0x88, /* Port  9 (8SERDES_0)    IntBus=0 Addr=0x08 */
        0x89, /* Port 10 (8SERDES_1)    IntBus=0 Addr=0x09 */
        0x8a, /* Port 11 (8SERDES_1)    IntBus=0 Addr=0x0a */
        0x8b, /* Port 12 (8SERDES_1)    IntBus=0 Addr=0x0b */
        0x8c, /* Port 13 (8SERDES_1)    IntBus=0 Addr=0x0c */
        0x8d, /* Port 14 (8SERDES_1)    IntBus=0 Addr=0x0d */
        0x8e, /* Port 15 (8SERDES_1)    IntBus=0 Addr=0x0e */
        0x8f, /* Port 16 (8SERDES_1)    IntBus=0 Addr=0x0f */
        0x90, /* Port 17 (8SERDES_1)    IntBus=0 Addr=0x10 */
        0x91, /* Port 18 (9SERDES)      IntBus=0 Addr=0x11 */
        0x92, /* Port 19 (9SERDES)      IntBus=0 Addr=0x12 */
        0x93, /* Port 20 (9SERDES)      IntBus=0 Addr=0x13 */
        0x94, /* Port 21 (9SERDES)      IntBus=0 Addr=0x14 */
        0x95, /* Port 22 (9SERDES)      IntBus=0 Addr=0x15 */
        0x96, /* Port 23 (9SERDES)      IntBus=0 Addr=0x16 */
        0x97, /* Port 24 (9SERDES)      IntBus=0 Addr=0x17 */
        0x98, /* Port 25 (9SERDES)      IntBus=0 Addr=0x18 */
        0xd9, /* Port 26 (HC0)          IntBus=2 Addr=0x19 */
        0xda, /* Port 27 (HC1)          IntBus=2 Addr=0x1a */
        0xdb, /* Port 28 (HC2)          IntBus=2 Addr=0x1b */
        0xdc, /* Port 29 (HC3)          IntBus=2 Addr=0x1c */
        0xc1, /* Port 30 (HL0)          IntBus=2 Addr=0x01 */
        0xc2, /* Port 31 (HL0)          IntBus=2 Addr=0x02 */
        0xc3, /* Port 32 (HL0)          IntBus=2 Addr=0x03 */
        0xc4, /* Port 33 (HL0)          IntBus=2 Addr=0x04 */
        0xc5, /* Port 34 (HL1)          IntBus=2 Addr=0x05 */
        0xc6, /* Port 35 (HL1)          IntBus=2 Addr=0x06 */
        0xc7, /* Port 36 (HL1)          IntBus=2 Addr=0x07 */
        0xc8, /* Port 37 (HL1)          IntBus=2 Addr=0x08 */
        0xc9, /* Port 38 (HL2)          IntBus=2 Addr=0x09 */
        0xca, /* Port 39 (HL2)          IntBus=2 Addr=0x0a */
        0xcb, /* Port 40 (HL2)          IntBus=2 Addr=0x0b */
        0xcc, /* Port 41 (HL2)          IntBus=2 Addr=0x0c */
        0xcd, /* Port 42 (HL3)          IntBus=2 Addr=0x0d */
        0xce, /* Port 43 (HL3)          IntBus=2 Addr=0x0e */
        0xcf, /* Port 44 (HL3)          IntBus=2 Addr=0x0f */
        0xd0, /* Port 45 (HL3)          IntBus=2 Addr=0x10 */
        0xd1, /* Port 46 (HL4)          IntBus=2 Addr=0x11 */
        0xd2, /* Port 47 (HL4)          IntBus=2 Addr=0x12 */
        0xd3, /* Port 48 (HL4)          IntBus=2 Addr=0x13 */
        0xd4, /* Port 49 (HL4)          IntBus=2 Addr=0x14 */
        0xd5, /* Port 50 (HL5)          IntBus=2 Addr=0x15 */
        0xd6, /* Port 51 (HL5)          IntBus=2 Addr=0x16 */
        0xd7, /* Port 52 (HL5)          IntBus=2 Addr=0x17 */
        0xd8, /* Port 53 (HL5)          IntBus=2 Addr=0x18 */
    };

    static const uint16 _soc_phy_addr_bcm56630[] = {
        0x00, /* Port  0 (cmic) N/A */
        0x19, /* Port  1 ( ge0) ExtBus=0 Addr=0x19 */
        0x00, /* Port  2 ( N/A)                    */
        0x00, /* Port  3 ( N/A)                    */
        0x00, /* Port  4 ( N/A)                    */
        0x00, /* Port  5 ( N/A)                    */
        0x00, /* Port  6 ( N/A)                    */
        0x00, /* Port  7 ( N/A)                    */
        0x00, /* Port  8 ( N/A)                    */
        0x00, /* Port  9 ( N/A)                    */
        0x00, /* Port 10 ( N/A)                    */
        0x00, /* Port 11 ( N/A)                    */
        0x00, /* Port 12 ( N/A)                    */
        0x00, /* Port 13 ( N/A)                    */
        0x00, /* Port 14 ( N/A)                    */
        0x00, /* Port 15 ( N/A)                    */
        0x00, /* Port 16 ( N/A)                    */
        0x00, /* Port 17 ( N/A)                    */
        0x00, /* Port 18 ( N/A)                    */
        0x00, /* Port 19 ( N/A)                    */
        0x00, /* Port 20 ( N/A)                    */
        0x00, /* Port 21 ( N/A)                    */
        0x00, /* Port 22 ( N/A)                    */
        0x00, /* Port 23 ( N/A)                    */
        0x00, /* Port 24 ( N/A)                    */
        0x00, /* Port 25 ( N/A)                    */
        0x41, /* Port 26 ( hg0) ExtBus=2 Addr=0x01 */
        0x42, /* Port 27 ( hg1) ExtBus=2 Addr=0x02 */
        0x43, /* Port 28 ( hg2) ExtBus=2 Addr=0x03 */
        0x44, /* Port 29 ( hg3) ExtBus=2 Addr=0x04 */
        0x01, /* Port 30 ( ge1) ExtBus=0 Addr=0x01 */
        0x02, /* Port 31 ( ge2) ExtBus=0 Addr=0x02 */
        0x03, /* Port 32 ( ge3) ExtBus=0 Addr=0x03 */
        0x04, /* Port 33 ( ge4) ExtBus=0 Addr=0x04 */
        0x05, /* Port 34 ( ge5) ExtBus=0 Addr=0x05 */
        0x06, /* Port 35 ( ge6) ExtBus=0 Addr=0x06 */
        0x07, /* Port 36 ( ge7) ExtBus=0 Addr=0x07 */
        0x08, /* Port 37 ( ge8) ExtBus=0 Addr=0x08 */
        0x09, /* Port 38 ( ge9) ExtBus=0 Addr=0x09 */
        0x0a, /* Port 39 (ge10) ExtBus=0 Addr=0x0a */
        0x0b, /* Port 40 (ge11) ExtBus=0 Addr=0x0b */
        0x0c, /* Port 41 (ge12) ExtBus=0 Addr=0x0c */
        0x0d, /* Port 42 (ge13) ExtBus=0 Addr=0x0d */
        0x0e, /* Port 43 (ge14) ExtBus=0 Addr=0x0e */
        0x0f, /* Port 44 (ge15) ExtBus=0 Addr=0x0f */
        0x10, /* Port 45 (ge16) ExtBus=0 Addr=0x10 */
        0x11, /* Port 46 (ge17) ExtBus=0 Addr=0x11 */
        0x12, /* Port 47 (ge18) ExtBus=0 Addr=0x12 */
        0x13, /* Port 48 (ge19) ExtBus=0 Addr=0x13 */
        0x14, /* Port 49 (ge20) ExtBus=0 Addr=0x14 */
        0x15, /* Port 50 (ge21) ExtBus=0 Addr=0x15 */
        0x16, /* Port 51 (ge22) ExtBus=0 Addr=0x16 */
        0x17, /* Port 52 (ge23) ExtBus=0 Addr=0x17 */
        0x18, /* Port 53 (ge24) ExtBus=0 Addr=0x18 */
    };

static const uint16 _soc_phy_addr_bcm56634[] = {
        0x00, /* Port  0 (cmic) N/A */
        0x19, /* Port  1 ( ge0) ExtBus=0 Addr=0x19 */
        0x09, /* Port  2 ( ge1) ExtBus=0 Addr=0x09 */
        0x0a, /* Port  3 ( ge2) ExtBus=0 Addr=0x0a */
        0x0b, /* Port  4 ( ge3) ExtBus=0 Addr=0x0b */
        0x0c, /* Port  5 ( ge4) ExtBus=0 Addr=0x0c */
        0x0d, /* Port  6 ( ge5) ExtBus=0 Addr=0x0d */
        0x0e, /* Port  7 ( ge6) ExtBus=0 Addr=0x0e */
        0x0f, /* Port  8 ( ge7) ExtBus=0 Addr=0x0f */
        0x10, /* Port  9 ( ge8) ExtBus=0 Addr=0x10 */
        0x15, /* Port 10 ( ge9) ExtBus=0 Addr=0x15 */ 
        0x16, /* Port 11 (ge10) ExtBus=0 Addr=0x16 */ 
        0x17, /* Port 12 (ge11) ExtBus=0 Addr=0x17 */
        0x18, /* Port 13 (ge12) ExtBus=0 Addr=0x18 */ 
        0x21, /* Port 14 (ge13) ExtBus=1 Addr=0x01 */
        0x22, /* Port 15 (ge14) ExtBus=1 Addr=0x02 */
        0x23, /* Port 16 (ge15) ExtBus=1 Addr=0x03 */
        0x24, /* Port 17 (ge16) ExtBus=1 Addr=0x04 */
        0x29, /* Port 18 (ge17) ExtBus=1 Addr=0x09 */
        0x2a, /* Port 19 (ge18) ExtBus=1 Addr=0x0a */
        0x2b, /* Port 20 (ge19) ExtBus=1 Addr=0x0b */
        0x2c, /* Port 21 (ge20) ExtBus=1 Addr=0x0c */
        0x2d, /* Port 22 (ge21) ExtBus=1 Addr=0x0d */
        0x2e, /* Port 23 (ge22) ExtBus=1 Addr=0x0e */
        0x2f, /* Port 24 (ge23) ExtBus=1 Addr=0x0f */
        0x30, /* Port 25 (ge24) ExtBus=1 Addr=0x10 */
        0x41, /* Port 26 ( hg0) ExtBus=2 Addr=0x01 */
        0x42, /* Port 27 ( hg1) ExtBus=2 Addr=0x02 */
        0x43, /* Port 28 ( hg2) ExtBus=2 Addr=0x03 */
        0x44, /* Port 29 ( hg3) ExtBus=2 Addr=0x04 */
        0x01, /* Port 30 (ge25) ExtBus=0 Addr=0x01 */
        0x02, /* Port 31 (ge26) ExtBus=0 Addr=0x02 */
        0x03, /* Port 32 (ge27) ExtBus=0 Addr=0x03 */
        0x04, /* Port 33 (ge28) ExtBus=0 Addr=0x04 */
        0x05, /* Port 34 (ge29) ExtBus=0 Addr=0x05 */
        0x06, /* Port 35 (ge30) ExtBus=0 Addr=0x06 */
        0x07, /* Port 36 (ge31) ExtBus=0 Addr=0x07 */
        0x08, /* Port 37 (ge32) ExtBus=0 Addr=0x08 */
        0x11, /* Port 38 (ge33) ExtBus=0 Addr=0x11 */
        0x12, /* Port 39 (ge34) ExtBus=0 Addr=0x12 */
        0x13, /* Port 40 (ge35) ExtBus=0 Addr=0x13 */
        0x14, /* Port 41 (ge36) ExtBus=0 Addr=0x14 */
        0x25, /* Port 42 (ge37) ExtBus=1 Addr=0x05 */
        0x26, /* Port 43 (ge38) ExtBus=1 Addr=0x06 */
        0x27, /* Port 44 (ge39) ExtBus=1 Addr=0x07 */
        0x28, /* Port 45 (ge40) ExtBus=1 Addr=0x08 */
        0x31, /* Port 46 (ge41) ExtBus=1 Addr=0x11 */
        0x32, /* Port 47 (ge42) ExtBus=1 Addr=0x12 */
        0x33, /* Port 48 (ge43) ExtBus=1 Addr=0x13 */
        0x34, /* Port 49 (ge44) ExtBus=1 Addr=0x14 */
        0x35, /* Port 50 (ge45) ExtBus=1 Addr=0x15 */
        0x36, /* Port 51 (ge46) ExtBus=1 Addr=0x16 */
        0x37, /* Port 52 (ge47) ExtBus=1 Addr=0x17 */
        0x38, /* Port 53 (ge48) ExtBus=1 Addr=0x18 */
    };


    static const uint16 _soc_phy_addr_bcm56636[] = {
        0x00, /* Port  0 (cmic) N/A */
        0x19, /* Port  1 ( ge0) ExtBus=1 Addr=0x19 */
        0x09, /* Port  2 ( ge1) ExtBus=0 Addr=0x09 */
        0x0a, /* Port  3 ( ge2) ExtBus=0 Addr=0x0a */
        0x0b, /* Port  4 ( ge3) ExtBus=0 Addr=0x0b */
        0x0c, /* Port  5 ( ge4) ExtBus=0 Addr=0x0c */
        0x0d, /* Port  6 ( ge5) ExtBus=0 Addr=0x0d */
        0x0e, /* Port  7 ( ge6) ExtBus=0 Addr=0x0e */
        0x0f, /* Port  8 ( ge7) ExtBus=0 Addr=0x0f */
        0x10, /* Port  9 ( ge8) ExtBus=0 Addr=0x10 */
        0x15, /* Port 10 ( ge9) ExtBus=0 Addr=0x15 */
        0x16, /* Port 11 (ge10) ExtBus=0 Addr=0x16 */
        0x17, /* Port 12 (ge11) ExtBus=0 Addr=0x17 */
        0x18, /* Port 13 (ge12) ExtBus=0 Addr=0x18 */
        0x00, /* Port 14 ( N/A)                    */
        0x00, /* Port 15 ( N/A)                    */
        0x00, /* Port 16 ( N/A)                    */
        0x00, /* Port 17 ( N/A)                    */
        0x00, /* Port 18 ( N/A)                    */
        0x00, /* Port 19 ( N/A)                    */
        0x00, /* Port 20 ( N/A)                    */
        0x00, /* Port 21 ( N/A)                    */
        0x00, /* Port 22 ( N/A)                    */
        0x00, /* Port 23 ( N/A)                    */
        0x00, /* Port 24 ( N/A)                    */
        0x00, /* Port 25 ( N/A)                    */
        0x41, /* Port 26 ( hg0) ExtBus=2 Addr=0x01 */
        0x42, /* Port 27 ( hg1) ExtBus=2 Addr=0x02 */
        0x43, /* Port 28 ( hg2) ExtBus=2 Addr=0x03 */
        0x44, /* Port 29 ( hg3) ExtBus=2 Addr=0x04 */
        0x01, /* Port 30 (ge25) ExtBus=0 Addr=0x01 */
        0x02, /* Port 31 (ge26) ExtBus=0 Addr=0x02 */
        0x03, /* Port 32 (ge27) ExtBus=0 Addr=0x03 */
        0x04, /* Port 33 (ge28) ExtBus=0 Addr=0x04 */
        0x05, /* Port 34 (ge29) ExtBus=0 Addr=0x05 */
        0x06, /* Port 35 (ge30) ExtBus=0 Addr=0x06 */
        0x07, /* Port 36 (ge31) ExtBus=0 Addr=0x07 */
        0x08, /* Port 37 (ge32) ExtBus=0 Addr=0x08 */
        0x11, /* Port 38 (ge33) ExtBus=0 Addr=0x11 */
        0x12, /* Port 39 (ge34) ExtBus=0 Addr=0x12 */
        0x13, /* Port 40 (ge35) ExtBus=0 Addr=0x13 */
        0x14, /* Port 41 (ge36) ExtBus=0 Addr=0x14 */
        0x45, /* Port 42 (ge37) ExtBus=2 Addr=0x05 */
        0x00, /* Port 43 ( N/A)                    */
        0x00, /* Port 44 ( N/A)                    */
        0x00, /* Port 45 ( N/A)                    */
        0x00, /* Port 46 ( N/A)                    */
        0x00, /* Port 47 ( N/A)                    */
        0x00, /* Port 48 ( N/A)                    */
        0x00, /* Port 49 ( N/A)                    */
        0x46, /* Port 50 (ge45) ExtBus=2 Addr=0x06 */
        0x00, /* Port 51 ( N/A)                    */
        0x00, /* Port 52 ( N/A)                    */
        0x00, /* Port 53 ( N/A)                    */
    };

    static const uint16 _soc_phy_addr_bcm56638[] = {
        0x00, /* Port  0 (cmic) N/A */
        0x19, /* Port  1 ( ge0) ExtBus=1 Addr=0x19 */
        0x00, /* Port  2 ( N/A)                    */
        0x00, /* Port  3 ( N/A)                    */
        0x00, /* Port  4 ( N/A)                    */
        0x00, /* Port  5 ( N/A)                    */
        0x00, /* Port  6 ( N/A)                    */
        0x00, /* Port  7 ( N/A)                    */
        0x00, /* Port  8 ( N/A)                    */
        0x00, /* Port  9 ( N/A)                    */
        0x00, /* Port 10 ( N/A)                    */
        0x00, /* Port 11 ( N/A)                    */
        0x00, /* Port 12 ( N/A)                    */
        0x00, /* Port 13 ( N/A)                    */
        0x00, /* Port 14 ( N/A)                    */
        0x00, /* Port 15 ( N/A)                    */
        0x00, /* Port 16 ( N/A)                    */
        0x00, /* Port 17 ( N/A)                    */
        0x00, /* Port 18 ( N/A)                    */
        0x00, /* Port 19 ( N/A)                    */
        0x00, /* Port 20 ( N/A)                    */
        0x00, /* Port 21 ( N/A)                    */
        0x00, /* Port 22 ( N/A)                    */
        0x00, /* Port 23 ( N/A)                    */
        0x00, /* Port 24 ( N/A)                    */
        0x00, /* Port 25 ( N/A)                    */
        0x41, /* Port 26 ( hg0) ExtBus=2 Addr=0x01 */
        0x42, /* Port 27 ( hg1) ExtBus=2 Addr=0x02 */
        0x43, /* Port 28 ( hg2) ExtBus=2 Addr=0x03 */
        0x44, /* Port 29 ( hg3) ExtBus=2 Addr=0x04 */
        0x45, /* Port 30 ( xe0) ExtBus=2 Addr=0x05 */
        0x00, /* Port 31 ( N/A)                    */
        0x00, /* Port 32 ( N/A)                    */
        0x00, /* Port 33 ( N/A)                    */
        0x00, /* Port 34 ( N/A)                    */
        0x00, /* Port 35 ( N/A)                    */
        0x00, /* Port 36 ( N/A)                    */
        0x00, /* Port 37 ( N/A)                    */
        0x46, /* Port 38 ( xe1) ExtBus=2 Addr=0x06 */
        0x00, /* Port 39 ( N/A)                    */
        0x00, /* Port 40 ( N/A)                    */
        0x00, /* Port 41 ( N/A)                    */
        0x47, /* Port 42 ( xe2) ExtBus=2 Addr=0x07 */
        0x00, /* Port 43 ( N/A)                    */
        0x00, /* Port 44 ( N/A)                    */
        0x00, /* Port 45 ( N/A)                    */
        0x00, /* Port 46 ( N/A)                    */
        0x00, /* Port 47 ( N/A)                    */
        0x00, /* Port 48 ( N/A)                    */
        0x00, /* Port 49 ( N/A)                    */
        0x48, /* Port 50 ( xe3) ExtBus=2 Addr=0x08 */
        0x00, /* Port 51 ( N/A)                    */
        0x00, /* Port 52 ( N/A)                    */
        0x00, /* Port 53 ( N/A)                    */
    };

    static const uint16 _soc_phy_addr_bcm56639[] = {
        0x00, /* Port  0 (cmic) N/A */
        0x1d, /* Port  1 ( ge0) ExtBus=0 Addr=0x1d */
        0x01, /* Port  2 ( ge1) ExtBus=0 Addr=0x01 */
        0x02, /* Port  3 ( ge2) ExtBus=0 Addr=0x02 */
        0x03, /* Port  4 ( ge3) ExtBus=0 Addr=0x03 */
        0x04, /* Port  5 ( ge4) ExtBus=0 Addr=0x04 */
        0x05, /* Port  6 ( ge5) ExtBus=0 Addr=0x05 */
        0x06, /* Port  7 ( ge6) ExtBus=0 Addr=0x06 */
        0x07, /* Port  8 ( ge7) ExtBus=0 Addr=0x07 */
        0x08, /* Port  9 ( ge8) ExtBus=0 Addr=0x08 */
        0x09, /* Port 10 ( ge9) ExtBus=0 Addr=0x09 */
        0x0a, /* Port 11 (ge10) ExtBus=0 Addr=0x0a */
        0x0b, /* Port 12 (ge11) ExtBus=0 Addr=0x0b */
        0x0c, /* Port 13 (ge12) ExtBus=0 Addr=0x0c */
        0x0d, /* Port 14 (ge13) ExtBus=0 Addr=0x0d */
        0x0e, /* Port 15 (ge14) ExtBus=0 Addr=0x0e */
        0x0f, /* Port 16 (ge15) ExtBus=0 Addr=0x0f */
        0x10, /* Port 17 (ge16) ExtBus=0 Addr=0x10 */
        0x11, /* Port 18 (ge17) ExtBus=0 Addr=0x11 */
        0x12, /* Port 19 (ge18) ExtBus=0 Addr=0x12 */
        0x13, /* Port 20 (ge19) ExtBus=0 Addr=0x13 */
        0x14, /* Port 21 (ge20) ExtBus=0 Addr=0x14 */
        0x15, /* Port 22 (ge21) ExtBus=0 Addr=0x15 */
        0x16, /* Port 23 (ge22) ExtBus=0 Addr=0x16 */
        0x17, /* Port 24 (ge23) ExtBus=0 Addr=0x17 */
        0x18, /* Port 25 (ge24) ExtBus=0 Addr=0x18 */
        0x45, /* Port 26 ( hg0) ExtBus=2 Addr=0x05 */
        0x46, /* Port 27 ( hg1) ExtBus=2 Addr=0x06 */
        0x47, /* Port 28 ( hg2) ExtBus=2 Addr=0x07 */
        0x48, /* Port 29 ( hg3) ExtBus=2 Addr=0x08 */
        0x41, /* Port 30 ( xe0) ExtBus=2 Addr=0x01 */
        0x00, /* Port 31 ( N/A)                    */
        0x00, /* Port 32 ( N/A)                    */
        0x00, /* Port 33 ( N/A)                    */
        0x00, /* Port 34 ( N/A)                    */
        0x00, /* Port 35 ( N/A)                    */
        0x00, /* Port 36 ( N/A)                    */
        0x00, /* Port 37 ( N/A)                    */
        0x42, /* Port 38 ( xe1) ExtBus=2 Addr=0x02 */
        0x00, /* Port 39 ( N/A)                    */
        0x00, /* Port 40 ( N/A)                    */
        0x00, /* Port 41 ( N/A)                    */
        0x43, /* Port 42 ( xe2) ExtBus=2 Addr=0x03 */
        0x00, /* Port 43 ( N/A)                    */
        0x00, /* Port 44 ( N/A)                    */
        0x00, /* Port 45 ( N/A)                    */
        0x19, /* Port 46 (ge25) ExtBus=0 Addr=0x19 */
        0x1a, /* Port 47 (ge26) ExtBus=0 Addr=0x1a */
        0x1b, /* Port 48 (ge27) ExtBus=0 Addr=0x1b */
        0x1c, /* Port 49 (ge28) ExtBus=0 Addr=0x1c */
        0x44, /* Port 50 ( xe3) ExtBus=2 Addr=0x04 */
        0x00, /* Port 51 ( N/A)                    */
        0x00, /* Port 52 ( N/A)                    */
        0x00, /* Port 53 ( N/A)                    */
    };

    static const uint16 _soc_phy_addr_bcm56521[] = {
        0x00, /* Port  0 (cmic) N/A */
        0x00, /* Port  1 ( N/A)                    */
        0x01, /* Port  2 ( ge0) ExtBus=0 Addr=0x01 */
        0x02, /* Port  3 ( ge1) ExtBus=0 Addr=0x02 */
        0x03, /* Port  4 ( ge2) ExtBus=0 Addr=0x03 */
        0x04, /* Port  5 ( ge3) ExtBus=0 Addr=0x04 */
        0x05, /* Port  6 ( ge4) ExtBus=0 Addr=0x05 */
        0x06, /* Port  7 ( ge5) ExtBus=0 Addr=0x06 */
        0x07, /* Port  8 ( ge6) ExtBus=0 Addr=0x07 */
        0x08, /* Port  9 ( ge7) ExtBus=0 Addr=0x08 */
        0x09, /* Port 10 ( ge8) ExtBus=0 Addr=0x09 */
        0x0a, /* Port 11 ( ge9) ExtBus=0 Addr=0x0a */
        0x0b, /* Port 12 (ge10) ExtBus=0 Addr=0x0b */
        0x0c, /* Port 13 (ge11) ExtBus=0 Addr=0x0c */
        0x0d, /* Port 14 (ge12) ExtBus=0 Addr=0x0d */
        0x0e, /* Port 15 (ge13) ExtBus=0 Addr=0x0e */
        0x0f, /* Port 16 (ge14) ExtBus=0 Addr=0x0f */
        0x10, /* Port 17 (ge15) ExtBus=0 Addr=0x10 */
        0x11, /* Port 18 (ge16) ExtBus=0 Addr=0x11 */
        0x12, /* Port 19 (ge17) ExtBus=0 Addr=0x12 */
        0x13, /* Port 20 (ge18) ExtBus=0 Addr=0x13 */
        0x14, /* Port 21 (ge19) ExtBus=0 Addr=0x14 */
        0x15, /* Port 22 (ge20) ExtBus=0 Addr=0x15 */
        0x16, /* Port 23 (ge21) ExtBus=0 Addr=0x16 */
        0x17, /* Port 24 (ge22) ExtBus=0 Addr=0x17 */
        0x18, /* Port 25 (ge23) ExtBus=0 Addr=0x18 */
        0x41, /* Port 26 ( hg0) ExtBus=2 Addr=0x01 */
        0x42, /* Port 27 ( hg1) ExtBus=2 Addr=0x02 */
        0x43, /* Port 28 ( hg2) ExtBus=2 Addr=0x03 */
        0x44, /* Port 29 ( hg3) ExtBus=2 Addr=0x04 */
        0x00, /* Port 30 ( N/A)                    */
        0x00, /* Port 31 ( N/A)                    */
        0x00, /* Port 32 ( N/A)                    */
        0x00, /* Port 33 ( N/A)                    */
        0x00, /* Port 34 ( N/A)                    */
        0x00, /* Port 35 ( N/A)                    */
        0x00, /* Port 36 ( N/A)                    */
        0x00, /* Port 37 ( N/A)                    */
        0x00, /* Port 38 ( N/A)                    */
        0x00, /* Port 39 ( N/A)                    */
        0x00, /* Port 40 ( N/A)                    */
        0x00, /* Port 41 ( N/A)                    */
        0x00, /* Port 42 ( N/A)                    */
        0x00, /* Port 43 ( N/A)                    */
        0x00, /* Port 44 ( N/A)                    */
        0x00, /* Port 45 ( N/A)                    */
        0x00, /* Port 46 ( N/A)                    */
        0x00, /* Port 47 ( N/A)                    */
        0x00, /* Port 48 ( N/A)                    */
        0x00, /* Port 49 ( N/A)                    */
        0x00, /* Port 50 ( N/A)                    */
        0x00, /* Port 51 ( N/A)                    */
        0x00, /* Port 52 ( N/A)                    */
        0x00, /* Port 53 ( N/A)                    */
    };

    static const uint16 _soc_phy_addr_bcm56526[] = {
        0x00, /* Port  0 (cmic) N/A */
        0x1d, /* Port  1 ( ge0) ExtBus=0 Addr=0x19 */
        0x01, /* Port  2 ( ge1) ExtBus=0 Addr=0x01 */
        0x02, /* Port  3 ( ge2) ExtBus=0 Addr=0x02 */
        0x03, /* Port  4 ( ge3) ExtBus=0 Addr=0x03 */
        0x04, /* Port  5 ( ge4) ExtBus=0 Addr=0x04 */
        0x05, /* Port  6 ( ge5) ExtBus=0 Addr=0x05 */
        0x06, /* Port  7 ( ge6) ExtBus=0 Addr=0x06 */
        0x07, /* Port  8 ( ge7) ExtBus=0 Addr=0x07 */
        0x08, /* Port  9 ( ge8) ExtBus=0 Addr=0x08 */
        0x0d, /* Port 10 (ge13) ExtBus=0 Addr=0x0d */
        0x0e, /* Port 11 (ge14) ExtBus=0 Addr=0x0e */
        0x0f, /* Port 12 (ge15) ExtBus=0 Addr=0x0f */
        0x10, /* Port 13 (ge16) ExtBus=0 Addr=0x10 */
        0x11, /* Port 14 (ge17) ExtBus=0 Addr=0x11 */
        0x12, /* Port 15 (ge18) ExtBus=0 Addr=0x12 */
        0x13, /* Port 16 (ge19) ExtBus=0 Addr=0x13 */
        0x14, /* Port 17 (ge20) ExtBus=0 Addr=0x14 */
        0x15, /* Port 18 (ge21) ExtBus=0 Addr=0x15 */
        0x16, /* Port 19 (ge22) ExtBus=0 Addr=0x16 */
        0x17, /* Port 20 (ge23) ExtBus=0 Addr=0x17 */
        0x18, /* Port 21 (ge24) ExtBus=0 Addr=0x18 */
        0x19, /* Port 22 (ge25) ExtBus=0 Addr=0x19 */
        0x1a, /* Port 23 (ge26) ExtBus=0 Addr=0x1a */
        0x1b, /* Port 24 (ge27) ExtBus=0 Addr=0x1b */
        0x1c, /* Port 25 (ge28) ExtBus=0 Addr=0x1c */
        0x41, /* Port 26 ( hg0) ExtBus=2 Addr=0x01 */
        0x42, /* Port 27 ( hg1) ExtBus=2 Addr=0x02 */
        0x43, /* Port 28 ( hg2) ExtBus=2 Addr=0x03 */
        0x44, /* Port 29 ( hg3) ExtBus=2 Addr=0x04 */
        0x00, /* Port 30 ( N/A)                    */
        0x00, /* Port 31 ( N/A)                    */
        0x00, /* Port 32 ( N/A)                    */
        0x00, /* Port 33 ( N/A)                    */
        0x00, /* Port 34 ( N/A)                    */
        0x00, /* Port 35 ( N/A)                    */
        0x00, /* Port 36 ( N/A)                    */
        0x00, /* Port 37 ( N/A)                    */
        0x09, /* Port 38 ( ge9) ExtBus=0 Addr=0x09 */
        0x0a, /* Port 39 (ge10) ExtBus=0 Addr=0x0a */
        0x0b, /* Port 40 (ge11) ExtBus=0 Addr=0x0b */
        0x0c, /* Port 41 (ge12) ExtBus=0 Addr=0x0c */
        0x00, /* Port 42 ( N/A)                    */
        0x00, /* Port 43 ( N/A)                    */
        0x00, /* Port 44 ( N/A)                    */
        0x00, /* Port 45 ( N/A)                    */
        0x45, /* Port 46 ( xe0) ExtBus=2 Addr=0x05 */
        0x00, /* Port 47 ( N/A)                    */
        0x00, /* Port 48 ( N/A)                    */
        0x00, /* Port 49 ( N/A)                    */
        0x46, /* Port 50 ( xe1) ExtBus=2 Addr=0x06 */
        0x00, /* Port 51 ( N/A)                    */
        0x00, /* Port 52 ( N/A)                    */
        0x00, /* Port 53 ( N/A)                    */
    };

    static const uint16 _soc_phy_addr_bcm56685[] = {
        0x00, /* Port  0 (cmic) N/A */
        0x39, /* Port  1 ( ge0) ExtBus=1 Addr=0x19 */
        0x00, /* Port  2 ( N/A)                    */
        0x00, /* Port  3 ( N/A)                    */
        0x00, /* Port  4 ( N/A)                    */
        0x00, /* Port  5 ( N/A)                    */
        0x00, /* Port  6 ( N/A)                    */
        0x00, /* Port  7 ( N/A)                    */
        0x00, /* Port  8 ( N/A)                    */
        0x00, /* Port  9 ( N/A)                    */
        0x00, /* Port 10 ( N/A)                    */
        0x00, /* Port 11 ( N/A)                    */
        0x00, /* Port 12 ( N/A)                    */
        0x00, /* Port 13 ( N/A)                    */
        0x00, /* Port 14 ( N/A)                    */
        0x00, /* Port 15 ( N/A)                    */
        0x00, /* Port 16 ( N/A)                    */
        0x00, /* Port 17 ( N/A)                    */
        0x00, /* Port 18 ( N/A)                    */
        0x00, /* Port 19 ( N/A)                    */
        0x00, /* Port 20 ( N/A)                    */
        0x00, /* Port 21 ( N/A)                    */
        0x00, /* Port 22 ( N/A)                    */
        0x00, /* Port 23 ( N/A)                    */
        0x00, /* Port 24 ( N/A)                    */
        0x00, /* Port 25 ( N/A)                    */
        0x41, /* Port 26 ( hg0) ExtBus=2 Addr=0x01 */
        0x42, /* Port 27 ( hg1) ExtBus=2 Addr=0x02 */
        0x43, /* Port 28 ( hg2) ExtBus=2 Addr=0x03 */
        0x44, /* Port 29 ( hg3) ExtBus=2 Addr=0x04 */
        0x01, /* Port 30 ( ge1) ExtBus=0 Addr=0x01 */
        0x02, /* Port 31 ( ge2) ExtBus=0 Addr=0x02 */
        0x03, /* Port 32 ( ge3) ExtBus=0 Addr=0x03 */
        0x04, /* Port 33 ( ge4) ExtBus=0 Addr=0x04 */
        0x05, /* Port 34 ( ge5) ExtBus=0 Addr=0x05 */
        0x06, /* Port 35 ( ge6) ExtBus=0 Addr=0x06 */
        0x07, /* Port 36 ( ge7) ExtBus=0 Addr=0x07 */
        0x08, /* Port 37 ( ge8) ExtBus=0 Addr=0x08 */
        0x09, /* Port 38 ( ge9) ExtBus=0 Addr=0x09 */
        0x0a, /* Port 39 (ge10) ExtBus=0 Addr=0x0a */
        0x0b, /* Port 40 (ge11) ExtBus=0 Addr=0x0b */
        0x0c, /* Port 41 (ge12) ExtBus=0 Addr=0x0c */
        0x0d, /* Port 42 (ge13) ExtBus=0 Addr=0x0d */
        0x0e, /* Port 43 (ge14) ExtBus=0 Addr=0x0e */
        0x0f, /* Port 44 (ge15) ExtBus=0 Addr=0x0f */
        0x10, /* Port 45 (ge16) ExtBus=0 Addr=0x10 */
        0x11, /* Port 46 (ge17) ExtBus=0 Addr=0x11 */
        0x12, /* Port 47 (ge18) ExtBus=0 Addr=0x12 */
        0x13, /* Port 48 (ge19) ExtBus=0 Addr=0x13 */
        0x14, /* Port 49 (ge20) ExtBus=0 Addr=0x14 */
        0x15, /* Port 50 (ge21) ExtBus=0 Addr=0x15 */
        0x16, /* Port 51 (ge22) ExtBus=0 Addr=0x16 */
        0x17, /* Port 52 (ge23) ExtBus=0 Addr=0x17 */
        0x18, /* Port 53 (ge24) ExtBus=0 Addr=0x18 */
    };

    uint16 dev_id;
    uint8 rev_id;

    soc_cm_get_id(unit, &dev_id, &rev_id);

    *phy_addr_int = _soc_triumph2_int_phy_addr[port];
    switch (dev_id) {
    case BCM56630_DEVICE_ID:
    case BCM56520_DEVICE_ID:
    case BCM56522_DEVICE_ID:
    case BCM56524_DEVICE_ID:
    case BCM56534_DEVICE_ID:
        *phy_addr = _soc_phy_addr_bcm56630[port];
        break;
    case BCM56636_DEVICE_ID:
        *phy_addr = _soc_phy_addr_bcm56636[port];
        break;
    case BCM56638_DEVICE_ID:
        *phy_addr = _soc_phy_addr_bcm56638[port];
        break;
    case BCM56639_DEVICE_ID:
        *phy_addr = _soc_phy_addr_bcm56639[port];
        break;
    case BCM56521_DEVICE_ID:
        *phy_addr = _soc_phy_addr_bcm56521[port];
        break;
    case BCM56526_DEVICE_ID:
        *phy_addr = _soc_phy_addr_bcm56526[port];
        break;
    case BCM56685_DEVICE_ID:
    case BCM56689_DEVICE_ID:
        *phy_addr = _soc_phy_addr_bcm56685[port];
        break;
    case BCM56634_DEVICE_ID:
    case BCM56538_DEVICE_ID:
    default:
        *phy_addr = _soc_phy_addr_bcm56634[port];
        break;
    }
}
#endif /* BCM_TRIUMPH2_SUPPORT || BCM_APOLLO_SUPPORT || BCM_VALKYRIE2_SUPPORT */

#ifdef BCM_TRIUMPH_SUPPORT
static void
_triumph_phy_addr_default(int unit, int port,
                          uint16 *phy_addr, uint16 *phy_addr_int)
{
    static const uint16 _soc_triumph_int_phy_addr[] = {
        0x00, /* Port  0 (cmic)         N/A */
        0x99, /* Port  1 (serdes2:8)    IntBus=0 Addr=0x19 */
        0xc1, /* Port  2 (hyperlite0:0) IntBus=2 Addr=0x01 */
        0xc2, /* Port  3 (hyperlite0:1) IntBus=2 Addr=0x02 */
        0xc3, /* Port  4 (hyperlite0:2) IntBus=2 Addr=0x03 */
        0xc4, /* Port  5 (hyperlite0:3) IntBus=2 Addr=0x04 */
        0xc5, /* Port  6 (hyperlite1:0) IntBus=2 Addr=0x05 */
        0xc6, /* Port  7 (hyperlite1:1) IntBus=2 Addr=0x06 */
        0x81, /* Port  8 (serdes0:0)    IntBus=0 Addr=0x01 */
        0x82, /* Port  9 (serdes0:1)    IntBus=0 Addr=0x02 */
        0x83, /* Port 10 (serdes0:2)    IntBus=0 Addr=0x03 */
        0x84, /* Port 11 (serdes0:3)    IntBus=0 Addr=0x04 */
        0x85, /* Port 12 (serdes0:4)    IntBus=0 Addr=0x05 */
        0x86, /* Port 13 (serdes0:5)    IntBus=0 Addr=0x06 */
        0xc9, /* Port 14 (hyperlite2:0) IntBus=2 Addr=0x09 */
        0xca, /* Port 15 (hyperlite2:1) IntBus=2 Addr=0x0a */
        0xcb, /* Port 16 (hyperlite2:2) IntBus=2 Addr=0x0b */
        0xcc, /* Port 17 (hyperlite2:3) IntBus=2 Addr=0x0c */
        0xc7, /* Port 18 (hyperlite1:2) IntBus=2 Addr=0x07 */
        0xc8, /* Port 19 (hyperlite1:3) IntBus=2 Addr=0x08 */
        0x87, /* Port 20 (serdes0:6)    IntBus=0 Addr=0x07 */
        0x88, /* Port 21 (serdes0:7)    IntBus=0 Addr=0x08 */
        0x89, /* Port 22 (serdes1:0)    IntBus=0 Addr=0x09 */
        0x8a, /* Port 23 (serdes1:1)    IntBus=0 Addr=0x0a */
        0x8b, /* Port 24 (serdes1:2)    IntBus=0 Addr=0x0b */
        0x8c, /* Port 25 (serdes1:3)    IntBus=0 Addr=0x0c */
        0xcd, /* Port 26 (hyperlite3:0) IntBus=2 Addr=0x0d */
        0xd5, /* Port 27 (hyperlite5:0) IntBus=2 Addr=0x15 */
        0xd9, /* Port 28 (unicore0)     IntBus=2 Addr=0x19 */
        0xda, /* Port 29 (unicore1)     IntBus=2 Addr=0x1a */
        0xdb, /* Port 30 (unicore2)     IntBus=2 Addr=0x1b */
        0xdc, /* Port 31 (unicore3)     IntBus=2 Addr=0x1c */
        0xce, /* Port 32 (hyperlite3:1) IntBus=2 Addr=0x0e */
        0xcf, /* Port 33 (hyperlite3:2) IntBus=2 Addr=0x0f */
        0xd0, /* Port 34 (hyperlite3:3) IntBus=2 Addr=0x10 */
        0xd1, /* Port 35 (hyperlite4:0) IntBus=2 Addr=0x11 */
        0xd2, /* Port 36 (hyperlite4:1) IntBus=2 Addr=0x12 */
        0x8d, /* Port 37 (serdes1:4)    IntBus=0 Addr=0x0d */
        0x8e, /* Port 38 (serdes1:5)    IntBus=0 Addr=0x0e */
        0x8f, /* Port 39 (serdes1:6)    IntBus=0 Addr=0x0f */
        0x90, /* Port 40 (serdes1:7)    IntBus=0 Addr=0x10 */
        0x91, /* Port 41 (serdes2:0)    IntBus=0 Addr=0x11 */
        0x92, /* Port 42 (serdes2:1)    IntBus=0 Addr=0x12 */
        0xd6, /* Port 43 (hyperlite5:1) IntBus=2 Addr=0x16 */
        0xd7, /* Port 44 (hyperlite5:2) IntBus=2 Addr=0x17 */
        0xd8, /* Port 45 (hyperlite5:3) IntBus=2 Addr=0x18 */
        0xd3, /* Port 46 (hyperlite4:2) IntBus=2 Addr=0x13 */
        0xd4, /* Port 47 (hyperlite4:3) IntBus=2 Addr=0x14 */
        0x93, /* Port 48 (serdes2:2)    IntBus=0 Addr=0x13 */
        0x94, /* Port 49 (serdes2:3)    IntBus=0 Addr=0x14 */
        0x95, /* Port 50 (serdes2:4)    IntBus=0 Addr=0x15 */
        0x96, /* Port 51 (serdes2:5)    IntBus=0 Addr=0x16 */
        0x97, /* Port 52 (serdes2:6)    IntBus=0 Addr=0x17 */
        0x98, /* Port 53 (serdes2:7)    IntBus=0 Addr=0x18 */
    };

    static const uint16 _soc_phy_addr_bcm56624[] = {
        0x00, /* Port  0 (cmic) N/A */
        0x19, /* Port  1 ( ge0) ExtBus=1 Addr=0x19 */
        0x01, /* Port  2 ( ge1) ExtBus=0 Addr=0x01 */
        0x02, /* Port  3 ( ge2) ExtBus=0 Addr=0x02 */
        0x03, /* Port  4 ( ge3) ExtBus=0 Addr=0x03 */
        0x04, /* Port  5 ( ge4) ExtBus=0 Addr=0x04 */
        0x05, /* Port  6 ( ge5) ExtBus=0 Addr=0x05 */
        0x06, /* Port  7 ( ge6) ExtBus=0 Addr=0x06 */
        0x09, /* Port  8 ( ge7) ExtBus=0 Addr=0x09 */
        0x0a, /* Port  9 ( ge8) ExtBus=0 Addr=0x0a */
        0x0b, /* Port 10 ( ge9) ExtBus=0 Addr=0x0b */
        0x0c, /* Port 11 (ge10) ExtBus=0 Addr=0x0c */
        0x0d, /* Port 12 (ge11) ExtBus=0 Addr=0x0d */
        0x0e, /* Port 13 (ge12) ExtBus=0 Addr=0x0e */
        0x11, /* Port 14 (ge13) ExtBus=0 Addr=0x11 */
        0x12, /* Port 15 (ge14) ExtBus=0 Addr=0x12 */
        0x13, /* Port 16 (ge15) ExtBus=0 Addr=0x13 */
        0x14, /* Port 17 (ge16) ExtBus=0 Addr=0x14 */
        0x07, /* Port 18 (ge17) ExtBus=0 Addr=0x07 */
        0x08, /* Port 19 (ge18) ExtBus=0 Addr=0x08 */
        0x0f, /* Port 20 (ge19) ExtBus=0 Addr=0x0f */
        0x10, /* Port 21 (ge20) ExtBus=0 Addr=0x10 */
        0x15, /* Port 22 (ge21) ExtBus=0 Addr=0x15 */
        0x16, /* Port 23 (ge22) ExtBus=0 Addr=0x16 */
        0x17, /* Port 24 (ge23) ExtBus=0 Addr=0x17 */
        0x18, /* Port 25 (ge24) ExtBus=0 Addr=0x18 */
        0x25, /* Port 26 (ge25) ExtBus=1 Addr=0x05 */
        0x35, /* Port 27 (ge26) ExtBus=1 Addr=0x15 */
        0x41, /* Port 28 ( xg0) ExtBus=2 Addr=0x01 */
        0x42, /* Port 29 ( xg1) ExtBus=2 Addr=0x02 */
        0x43, /* Port 30 ( xg2) ExtBus=2 Addr=0x03 */
        0x44, /* Port 31 ( xg3) ExtBus=2 Addr=0x04 */
        0x26, /* Port 32 (ge27) ExtBus=1 Addr=0x06 */
        0x27, /* Port 33 (ge28) ExtBus=1 Addr=0x07 */
        0x28, /* Port 34 (ge29) ExtBus=1 Addr=0x08 */
        0x31, /* Port 35 (ge30) ExtBus=1 Addr=0x11 */
        0x32, /* Port 36 (ge31) ExtBus=1 Addr=0x12 */
        0x21, /* Port 37 (ge32) ExtBus=1 Addr=0x01 */
        0x22, /* Port 38 (ge33) ExtBus=1 Addr=0x02 */
        0x23, /* Port 39 (ge34) ExtBus=1 Addr=0x03 */
        0x24, /* Port 40 (ge35) ExtBus=1 Addr=0x04 */
        0x29, /* Port 41 (ge36) ExtBus=1 Addr=0x09 */
        0x2a, /* Port 42 (ge37) ExtBus=1 Addr=0x0a */
        0x36, /* Port 43 (ge38) ExtBus=1 Addr=0x16 */
        0x37, /* Port 44 (ge39) ExtBus=1 Addr=0x17 */
        0x38, /* Port 45 (ge40) ExtBus=1 Addr=0x18 */
        0x33, /* Port 46 (ge41) ExtBus=1 Addr=0x13 */
        0x34, /* Port 47 (ge42) ExtBus=1 Addr=0x14 */
        0x2b, /* Port 48 (ge43) ExtBus=1 Addr=0x0b */
        0x2c, /* Port 49 (ge44) ExtBus=1 Addr=0x0c */
        0x2d, /* Port 50 (ge45) ExtBus=1 Addr=0x0d */
        0x2e, /* Port 51 (ge46) ExtBus=1 Addr=0x0e */
        0x2f, /* Port 52 (ge47) ExtBus=1 Addr=0x0f */
        0x30, /* Port 53 (ge48) ExtBus=1 Addr=0x10 */
    };

    static const uint16 _soc_phy_addr_bcm56626[] = {
        0x00, /* Port  0 (cmic) N/A */
        0x19, /* Port  1 ( ge0) ExtBus=1 Addr=0x19 */
        0x01, /* Port  2 ( ge1) ExtBus=0 Addr=0x01 */
        0x02, /* Port  3 ( ge2) ExtBus=0 Addr=0x02 */
        0x03, /* Port  4 ( ge3) ExtBus=0 Addr=0x03 */
        0x04, /* Port  5 ( ge4) ExtBus=0 Addr=0x04 */
        0x05, /* Port  6 ( ge5) ExtBus=0 Addr=0x05 */
        0x06, /* Port  7 ( ge6) ExtBus=0 Addr=0x06 */
        0x09, /* Port  8 ( ge7) ExtBus=0 Addr=0x09 */
        0x0a, /* Port  9 ( ge8) ExtBus=0 Addr=0x0a */
        0x0b, /* Port 10 ( ge9) ExtBus=0 Addr=0x0b */
        0x0c, /* Port 11 (ge10) ExtBus=0 Addr=0x0c */
        0x0d, /* Port 12 (ge11) ExtBus=0 Addr=0x0d */
        0x0e, /* Port 13 (ge12) ExtBus=0 Addr=0x0e */
        0x11, /* Port 14 (ge13) ExtBus=0 Addr=0x11 */
        0x12, /* Port 15 (ge14) ExtBus=0 Addr=0x12 */
        0x13, /* Port 16 (ge15) ExtBus=0 Addr=0x13 */
        0x14, /* Port 17 (ge16) ExtBus=0 Addr=0x14 */
        0x07, /* Port 18 (ge17) ExtBus=0 Addr=0x07 */
        0x08, /* Port 19 (ge18) ExtBus=0 Addr=0x08 */
        0x0f, /* Port 20 (ge19) ExtBus=0 Addr=0x0f */
        0x10, /* Port 21 (ge20) ExtBus=0 Addr=0x10 */
        0x15, /* Port 22 (ge21) ExtBus=0 Addr=0x15 */
        0x16, /* Port 23 (ge22) ExtBus=0 Addr=0x16 */
        0x17, /* Port 24 (ge23) ExtBus=0 Addr=0x17 */
        0x18, /* Port 25 (ge24) ExtBus=0 Addr=0x18 */
        0x45, /* Port 26 ( xe0) ExtBus=2 Addr=0x05 */
        0x46, /* Port 27 ( xe1) ExtBus=2 Addr=0x06 */
        0x41, /* Port 28 ( hg0) ExtBus=2 Addr=0x01 */
        0x42, /* Port 29 ( hg1) ExtBus=2 Addr=0x02 */
        0x43, /* Port 30 ( hg2) ExtBus=2 Addr=0x03 */
        0x44, /* Port 31 ( hg3) ExtBus=2 Addr=0x04 */
    };

    static const uint16 _soc_phy_addr_bcm56628[] = {
        0x00, /* Port  0 (cmic) N/A */
        0x19, /* Port  1 ( ge0) ExtBus=1 Addr=0x01 */
        0x45, /* Port  2 ( xe0) ExtBus=2 Addr=0x05 */
        0x00, /* Port  3 ( N/A) */
        0x00, /* Port  4 ( N/A) */
        0x00, /* Port  5 ( N/A) */
        0x00, /* Port  6 ( N/A) */
        0x00, /* Port  7 ( N/A) */
        0x00, /* Port  8 ( N/A) */
        0x00, /* Port  9 ( N/A) */
        0x00, /* Port 10 ( N/A) */
        0x00, /* Port 11 ( N/A) */
        0x00, /* Port 12 ( N/A) */
        0x00, /* Port 13 ( N/A) */
        0x46, /* Port 14 ( xe1) ExtBus=2 Addr=0x06 */
        0x00, /* Port 15 ( N/A) */
        0x00, /* Port 16 ( N/A) */
        0x00, /* Port 17 ( N/A) */
        0x00, /* Port 18 ( N/A) */
        0x00, /* Port 19 ( N/A) */
        0x00, /* Port 20 ( N/A) */
        0x00, /* Port 21 ( N/A) */
        0x00, /* Port 22 ( N/A) */
        0x00, /* Port 23 ( N/A) */
        0x00, /* Port 24 ( N/A) */
        0x00, /* Port 25 ( N/A) */
        0x47, /* Port 26 ( xe2) ExtBus=2 Addr=0x07 */
        0x48, /* Port 27 ( xe3) ExtBus=2 Addr=0x08 */
        0x41, /* Port 28 ( hg0) ExtBus=2 Addr=0x01 */
        0x42, /* Port 29 ( hg1) ExtBus=2 Addr=0x02 */
        0x43, /* Port 30 ( hg2) ExtBus=2 Addr=0x03 */
        0x44, /* Port 31 ( hg3) ExtBus=2 Addr=0x04 */
    };

    static const uint16 _soc_phy_addr_bcm56629[] = {
        0x00, /* Port  0 (cmic) N/A */
        0x19, /* Port  1 ( ge0) ExtBus=0 Addr=0x19 */
        0x41, /* Port  2 ( xe0) ExtBus=2 Addr=0x01 */
        0x00, /* Port  3 ( N/A) */
        0x00, /* Port  4 ( N/A) */
        0x00, /* Port  5 ( N/A) */
        0x00, /* Port  6 ( N/A) */
        0x00, /* Port  7 ( N/A) */
        0x01, /* Port  8 ( ge1) ExtBus=0 Addr=0x01 */
        0x02, /* Port  9 ( ge2) ExtBus=0 Addr=0x02 */
        0x03, /* Port 10 ( ge3) ExtBus=0 Addr=0x03 */
        0x04, /* Port 11 ( ge4) ExtBus=0 Addr=0x04 */
        0x05, /* Port 12 ( ge5) ExtBus=0 Addr=0x05 */
        0x06, /* Port 13 ( ge6) ExtBus=0 Addr=0x06 */
        0x42, /* Port 14 ( xe1) ExtBus=2 Addr=0x02 */
        0x00, /* Port 15 ( N/A) */
        0x00, /* Port 16 ( N/A) */
        0x00, /* Port 17 ( N/A) */
        0x00, /* Port 18 ( N/A) */
        0x00, /* Port 19 ( N/A) */
        0x07, /* Port 20 ( ge7) ExtBus=0 Addr=0x07 */
        0x08, /* Port 21 ( ge8) ExtBus=0 Addr=0x08 */
        0x09, /* Port 22 ( ge9) ExtBus=0 Addr=0x09 */
        0x0a, /* Port 23 (ge10) ExtBus=0 Addr=0x0a */
        0x0b, /* Port 24 (ge11) ExtBus=0 Addr=0x0b */
        0x0c, /* Port 25 (ge12) ExtBus=0 Addr=0x0c */
        0x43, /* Port 26 ( xe2) ExtBus=2 Addr=0x03 */
        0x44, /* Port 27 ( xe3) ExtBus=2 Addr=0x04 */
        0x45, /* Port 28 ( hg0) ExtBus=2 Addr=0x05 */
        0x46, /* Port 29 ( hg1) ExtBus=2 Addr=0x06 */
        0x47, /* Port 30 ( hg2) ExtBus=2 Addr=0x07 */
        0x48, /* Port 31 ( hg3) ExtBus=2 Addr=0x08 */
        0x00, /* Port 32 ( N/A) */
        0x00, /* Port 33 ( N/A) */
        0x00, /* Port 34 ( N/A) */
        0x00, /* Port 35 ( N/A) */
        0x00, /* Port 36 ( N/A) */
        0x0d, /* Port 37 (ge13) ExtBus=0 Addr=0x0d */
        0x0e, /* Port 38 (ge14) ExtBus=0 Addr=0x0e */
        0x0f, /* Port 39 (ge15) ExtBus=0 Addr=0x0f */
        0x10, /* Port 40 (ge16) ExtBus=0 Addr=0x10 */
        0x11, /* Port 41 (ge17) ExtBus=0 Addr=0x11 */
        0x12, /* Port 42 (ge18) ExtBus=0 Addr=0x12 */
        0x00, /* Port 43 ( N/A) */
        0x00, /* Port 44 ( N/A) */
        0x00, /* Port 45 ( N/A) */
        0x00, /* Port 46 ( N/A) */
        0x00, /* Port 47 ( N/A) */
        0x13, /* Port 48 (ge19) ExtBus=0 Addr=0x13 */
        0x14, /* Port 49 (ge20) ExtBus=0 Addr=0x14 */
        0x15, /* Port 50 (ge21) ExtBus=0 Addr=0x15 */
        0x16, /* Port 51 (ge22) ExtBus=0 Addr=0x16 */
        0x17, /* Port 52 (ge23) ExtBus=0 Addr=0x17 */
        0x18, /* Port 53 (ge24) ExtBus=0 Addr=0x18 */
    };

    uint16 dev_id;
    uint8 rev_id;

    soc_cm_get_id(unit, &dev_id, &rev_id);

    *phy_addr_int = _soc_triumph_int_phy_addr[port];
    switch (dev_id) {
    case BCM56626_DEVICE_ID:
        *phy_addr = _soc_phy_addr_bcm56626[port];
        break;
    case BCM56628_DEVICE_ID:
        *phy_addr = _soc_phy_addr_bcm56628[port];
        break;
    case BCM56629_DEVICE_ID:
        if (soc_feature(unit, soc_feature_xgport_one_xe_six_ge)) {
            if (soc_property_get(unit, spn_BCM56629_40GE, 0)) {
                *phy_addr = _soc_phy_addr_bcm56624[port];
            } else {
                *phy_addr = _soc_phy_addr_bcm56629[port];
            }
        } else {
            *phy_addr = _soc_phy_addr_bcm56626[port];
        }
        break;
    case BCM56624_DEVICE_ID:
    default:
        *phy_addr = _soc_phy_addr_bcm56624[port];
        break;
    }
}
#endif /* BCM_TRIUMPH_SUPPORT */

#ifdef BCM_VALKYRIE_SUPPORT
static void
_valkyrie_phy_addr_default(int unit, int port,
                           uint16 *phy_addr, uint16 *phy_addr_int)
{
    static const uint16 _soc_valkyrie_int_phy_addr[] = {
        0x00, /* Port  0 (cmic)         N/A */
        0x99, /* Port  1 (serdes2:8)    IntBus=0 Addr=0x19 */
        0xc1, /* Port  2 (hyperlite0:0) IntBus=2 Addr=0x01 */
        0xc2, /* Port  3 (hyperlite0:1) IntBus=2 Addr=0x02 */
        0xc3, /* Port  4 (hyperlite0:2) IntBus=2 Addr=0x03 */
        0xc4, /* Port  5 (hyperlite0:3) IntBus=2 Addr=0x04 */
        0xc5, /* Port  6 (hyperlite1:0) IntBus=2 Addr=0x05 */
        0xc6, /* Port  7 (hyperlite1:1) IntBus=2 Addr=0x06 */
        0x81, /* Port  8 (N/A) */
        0x82, /* Port  9 (N/A) */
        0x83, /* Port 10 (N/A) */
        0x84, /* Port 11 (N/A) */
        0x85, /* Port 12 (N/A) */
        0x86, /* Port 13 (N/A) */
        0xc9, /* Port 14 (hyperlite2:0) IntBus=2 Addr=0x09 */
        0xca, /* Port 15 (hyperlite2:1) IntBus=2 Addr=0x0a */
        0xcb, /* Port 16 (hyperlite2:2) IntBus=2 Addr=0x0b */
        0xcc, /* Port 17 (hyperlite2:3) IntBus=2 Addr=0x0c */
        0xc7, /* Port 18 (hyperlite1:2) IntBus=2 Addr=0x07 */
        0xc8, /* Port 19 (hyperlite1:3) IntBus=2 Addr=0x08 */
        0x87, /* Port 20 (N/A) */
        0x88, /* Port 21 (N/A) */
        0x89, /* Port 22 (N/A) */
        0x8a, /* Port 23 (N/A) */
        0x8b, /* Port 24 (N/A) */
        0x8c, /* Port 25 (N/A) */
        0xcd, /* Port 26 (hyperlite3:0) IntBus=2 Addr=0x0d */
        0xd5, /* Port 27 (hyperlite5:0) IntBus=2 Addr=0x15 */
        0xd9, /* Port 28 (unicore0)     IntBus=2 Addr=0x19 */
        0xda, /* Port 29 (unicore1)     IntBus=2 Addr=0x1a */
        0xdb, /* Port 30 (unicore2)     IntBus=2 Addr=0x1b */
        0xdc, /* Port 31 (unicore3)     IntBus=2 Addr=0x1c */
        0xce, /* Port 32 (hyperlite3:1) IntBus=2 Addr=0x0e */
        0xcf, /* Port 33 (hyperlite3:2) IntBus=2 Addr=0x0f */
        0xd0, /* Port 34 (hyperlite3:3) IntBus=2 Addr=0x10 */
        0xd1, /* Port 35 (hyperlite4:0) IntBus=2 Addr=0x11 */
        0xd2, /* Port 36 (hyperlite4:1) IntBus=2 Addr=0x12 */
        0x8d, /* Port 37 (N/A) */
        0x8e, /* Port 38 (N/A) */
        0x8f, /* Port 39 (N/A) */
        0x90, /* Port 40 (N/A) */
        0x91, /* Port 41 (N/A) */
        0x92, /* Port 42 (N/A) */
        0xd6, /* Port 43 (hyperlite5:1) IntBus=2 Addr=0x16 */
        0xd7, /* Port 44 (hyperlite5:2) IntBus=2 Addr=0x17 */
        0xd8, /* Port 45 (hyperlite5:3) IntBus=2 Addr=0x18 */
        0xd3, /* Port 46 (hyperlite4:2) IntBus=2 Addr=0x13 */
        0xd4, /* Port 47 (hyperlite4:3) IntBus=2 Addr=0x14 */
        0x93, /* Port 48 (N/A) */
        0x94, /* Port 49 (N/A) */
        0x95, /* Port 50 (N/A) */
        0x96, /* Port 51 (N/A) */
        0x97, /* Port 52 (N/A) */
        0x98, /* Port 53 (N/A) */
    };

    static const uint16 _soc_phy_addr_bcm56680[] = {
        0x00, /* Port  0 (cmic) N/A */
        0x39, /* Port  1 ( ge0) ExtBus=1 Addr=0x19 */
        0x01, /* Port  2 ( ge1) ExtBus=0 Addr=0x01 */
        0x02, /* Port  3 ( ge2) ExtBus=0 Addr=0x02 */
        0x03, /* Port  4 ( ge3) ExtBus=0 Addr=0x03 */
        0x04, /* Port  5 ( ge4) ExtBus=0 Addr=0x04 */
        0x05, /* Port  6 ( ge5) ExtBus=0 Addr=0x05 */
        0x06, /* Port  7 ( ge6) ExtBus=0 Addr=0x06 */
        0x00, /* Port  8 ( N/A) */
        0x00, /* Port  9 ( N/A) */
        0x00, /* Port 10 ( N/A) */
        0x00, /* Port 11 ( N/A) */
        0x00, /* Port 12 ( N/A) */
        0x00, /* Port 13 ( N/A) */
        0x07, /* Port 14 ( ge7) ExtBus=0 Addr=0x07 */
        0x08, /* Port 15 ( ge8) ExtBus=0 Addr=0x08 */
        0x09, /* Port 16 ( ge9) ExtBus=0 Addr=0x09 */
        0x0a, /* Port 17 (ge10) ExtBus=0 Addr=0x0a */
        0x0b, /* Port 18 (ge11) ExtBus=0 Addr=0x0b */
        0x0c, /* Port 19 (ge12) ExtBus=0 Addr=0x0c */
        0x00, /* Port 20 ( N/A) */
        0x00, /* Port 21 ( N/A) */
        0x00, /* Port 22 ( N/A) */
        0x00, /* Port 23 ( N/A) */
        0x00, /* Port 24 ( N/A) */
        0x00, /* Port 25 ( N/A) */
        0x0d, /* Port 26 (ge13) ExtBus=0 Addr=0x0d */
        0x13, /* Port 27 (ge19) ExtBus=0 Addr=0x13 */
        0x41, /* Port 28 ( hg0) ExtBus=2 Addr=0x01 */
        0x42, /* Port 29 ( hg1) ExtBus=2 Addr=0x02 */
        0x43, /* Port 30 ( hg2) ExtBus=2 Addr=0x03 */
        0x44, /* Port 31 ( hg3) ExtBus=2 Addr=0x04 */
        0x0e, /* Port 32 (ge14) ExtBus=0 Addr=0x0e */
        0x0f, /* Port 33 (ge15) ExtBus=0 Addr=0x0f */
        0x10, /* Port 34 (ge16) ExtBus=0 Addr=0x10 */
        0x11, /* Port 35 (ge17) ExtBus=0 Addr=0x11 */
        0x12, /* Port 36 (ge18) ExtBus=0 Addr=0x12 */
        0x00, /* Port 37 ( N/A) */
        0x00, /* Port 38 ( N/A) */
        0x00, /* Port 39 ( N/A) */
        0x00, /* Port 40 ( N/A) */
        0x00, /* Port 41 ( N/A) */
        0x00, /* Port 42 ( N/A) */
        0x14, /* Port 43 (ge20) ExtBus=0 Addr=0x14 */
        0x15, /* Port 44 (ge21) ExtBus=0 Addr=0x15 */
        0x16, /* Port 45 (ge22) ExtBus=0 Addr=0x16 */
        0x17, /* Port 46 (ge23) ExtBus=0 Addr=0x17 */
        0x18, /* Port 47 (ge24) ExtBus=0 Addr=0x18 */
        0x00, /* Port 48 ( N/A) */
        0x00, /* Port 49 ( N/A) */
        0x00, /* Port 50 ( N/A) */
        0x00, /* Port 51 ( N/A) */
        0x00, /* Port 52 ( N/A) */
        0x00, /* Port 53 ( N/A) */
    };

    static const uint16 _soc_phy_addr_bcm56684[] = {
        0x00, /* Port  0 (cmic) N/A */
        0x00, /* Port  1 ( N/A) */
        0x01, /* Port  2 ( ge0) ExtBus=0 Addr=0x01 */
        0x02, /* Port  3 ( ge1) ExtBus=0 Addr=0x02 */
        0x03, /* Port  4 ( ge2) ExtBus=0 Addr=0x03 */
        0x04, /* Port  5 ( ge3) ExtBus=0 Addr=0x04 */
        0x05, /* Port  6 ( ge4) ExtBus=0 Addr=0x05 */
        0x06, /* Port  7 ( ge5) ExtBus=0 Addr=0x06 */
        0x00, /* Port  8 ( N/A) */
        0x00, /* Port  9 ( N/A) */
        0x00, /* Port 10 ( N/A) */
        0x00, /* Port 11 ( N/A) */
        0x00, /* Port 12 ( N/A) */
        0x00, /* Port 13 ( N/A) */
        0x09, /* Port 14 ( ge6) ExtBus=0 Addr=0x09 */
        0x0a, /* Port 15 ( ge7) ExtBus=0 Addr=0x0a */
        0x0b, /* Port 16 ( ge8) ExtBus=0 Addr=0x0b */
        0x0c, /* Port 17 ( ge9) ExtBus=0 Addr=0x0c */
        0x07, /* Port 18 (ge10) ExtBus=0 Addr=0x07 */
        0x08, /* Port 19 (ge11) ExtBus=0 Addr=0x08 */
        0x00, /* Port 20 ( N/A) */
        0x00, /* Port 21 ( N/A) */
        0x00, /* Port 22 ( N/A) */
        0x00, /* Port 23 ( N/A) */
        0x00, /* Port 24 ( N/A) */
        0x00, /* Port 25 ( N/A) */
        0x0d, /* Port 26 (ge12) ExtBus=0 Addr=0x0d */
        0x15, /* Port 27 (ge13) ExtBus=0 Addr=0x15 */
        0x41, /* Port 28 ( hg0) ExtBus=2 Addr=0x01 */
        0x42, /* Port 29 ( hg1) ExtBus=2 Addr=0x02 */
        0x43, /* Port 30 ( hg2) ExtBus=2 Addr=0x03 */
        0x44, /* Port 31 ( hg3) ExtBus=2 Addr=0x04 */
        0x0e, /* Port 32 (ge14) ExtBus=0 Addr=0x0e */
        0x0f, /* Port 33 (ge15) ExtBus=0 Addr=0x0f */
        0x10, /* Port 34 (ge16) ExtBus=0 Addr=0x10 */
        0x11, /* Port 35 (ge17) ExtBus=0 Addr=0x11 */
        0x12, /* Port 36 (ge18) ExtBus=0 Addr=0x12 */
        0x00, /* Port 37 ( N/A) */
        0x00, /* Port 38 ( N/A) */
        0x00, /* Port 39 ( N/A) */
        0x00, /* Port 40 ( N/A) */
        0x00, /* Port 41 ( N/A) */
        0x00, /* Port 42 ( N/A) */
        0x16, /* Port 43 (ge19) ExtBus=0 Addr=0x16 */
        0x17, /* Port 44 (ge20) ExtBus=0 Addr=0x17 */
        0x18, /* Port 45 (ge21) ExtBus=0 Addr=0x18 */
        0x13, /* Port 46 (ge22) ExtBus=0 Addr=0x13 */
        0x14, /* Port 47 (ge23) ExtBus=0 Addr=0x14 */
        0x00, /* Port 48 ( N/A) */
        0x00, /* Port 49 ( N/A) */
        0x00, /* Port 50 ( N/A) */
        0x00, /* Port 51 ( N/A) */
        0x00, /* Port 52 ( N/A) */
        0x00, /* Port 53 ( N/A) */
    };

    static const uint16 _soc_phy_addr_bcm56686[] = {
        0x00, /* Port  0 (cmic) N/A */
        0x00, /* Port  1 ( N/A) */
        0x45, /* Port  2 ( xe0) ExtBus=2 Addr=0x05 */
        0x00, /* Port  3 ( N/A) */
        0x00, /* Port  4 ( N/A) */
        0x00, /* Port  5 ( N/A) */
        0x00, /* Port  6 ( N/A) */
        0x00, /* Port  7 ( N/A) */
        0x00, /* Port  8 ( N/A) */
        0x00, /* Port  9 ( N/A) */
        0x00, /* Port 10 ( N/A) */
        0x00, /* Port 11 ( N/A) */
        0x00, /* Port 12 ( N/A) */
        0x00, /* Port 13 ( N/A) */
        0x00, /* Port 14 ( N/A) */
        0x00, /* Port 15 ( N/A) */
        0x00, /* Port 16 ( N/A) */
        0x00, /* Port 17 ( N/A) */
        0x00, /* Port 18 ( N/A) */
        0x00, /* Port 19 ( N/A) */
        0x00, /* Port 20 ( N/A) */
        0x00, /* Port 21 ( N/A) */
        0x00, /* Port 22 ( N/A) */
        0x00, /* Port 23 ( N/A) */
        0x00, /* Port 24 ( N/A) */
        0x00, /* Port 25 ( N/A) */
        0x00, /* Port 26 ( N/A) */
        0x46, /* Port 27 ( xe1) ExtBus=2 Addr=0x06 */
        0x41, /* Port 28 ( hg0) ExtBus=2 Addr=0x01 */
        0x42, /* Port 29 ( hg1) ExtBus=2 Addr=0x02 */
        0x43, /* Port 30 ( hg2) ExtBus=2 Addr=0x03 */
        0x44, /* Port 31 ( hg3) ExtBus=2 Addr=0x04 */
    };

    static const uint16 _soc_phy_addr_bcm56620[] = {
        0x00, /* Port  0 (cmic) N/A */
        0x00, /* Port  1 ( N/A) */
        0x01, /* Port  2 ( ge0) ExtBus=0 Addr=0x01 */
        0x02, /* Port  3 ( ge1) ExtBus=0 Addr=0x02 */
        0x03, /* Port  4 ( ge2) ExtBus=0 Addr=0x03 */
        0x04, /* Port  5 ( ge3) ExtBus=0 Addr=0x04 */
        0x05, /* Port  6 ( ge4) ExtBus=0 Addr=0x05 */
        0x06, /* Port  7 ( ge5) ExtBus=0 Addr=0x06 */
        0x00, /* Port  8 ( N/A) */
        0x00, /* Port  9 ( N/A) */
        0x00, /* Port 10 ( N/A) */
        0x00, /* Port 11 ( N/A) */
        0x00, /* Port 12 ( N/A) */
        0x00, /* Port 13 ( N/A) */
        0x09, /* Port 14 ( ge6) ExtBus=0 Addr=0x09 */
        0x0a, /* Port 15 ( ge7) ExtBus=0 Addr=0x0a */
        0x0b, /* Port 16 ( ge8) ExtBus=0 Addr=0x0b */
        0x0c, /* Port 17 ( ge9) ExtBus=0 Addr=0x0c */
        0x07, /* Port 18 (ge10) ExtBus=0 Addr=0x07 */
        0x08, /* Port 19 (ge11) ExtBus=0 Addr=0x08 */
        0x00, /* Port 20 ( N/A) */
        0x00, /* Port 21 ( N/A) */
        0x00, /* Port 22 ( N/A) */
        0x00, /* Port 23 ( N/A) */
        0x00, /* Port 24 ( N/A) */
        0x00, /* Port 25 ( N/A) */
        0x0d, /* Port 26 (ge12) ExtBus=0 Addr=0x0d */
        0x15, /* Port 27 (ge13) ExtBus=0 Addr=0x15 */
        0x41, /* Port 28 ( hg0) ExtBus=2 Addr=0x01 */
        0x42, /* Port 29 ( hg1) ExtBus=2 Addr=0x02 */
        0x43, /* Port 30 ( hg2) ExtBus=2 Addr=0x03 */
        0x44, /* Port 31 ( hg3) ExtBus=2 Addr=0x04 */
        0x0e, /* Port 32 (ge14) ExtBus=0 Addr=0x0e */
        0x0f, /* Port 33 (ge15) ExtBus=0 Addr=0x0f */
        0x10, /* Port 34 (ge16) ExtBus=0 Addr=0x10 */
        0x11, /* Port 35 (ge17) ExtBus=0 Addr=0x11 */
        0x12, /* Port 36 (ge18) ExtBus=0 Addr=0x12 */
        0x00, /* Port 37 ( N/A) */
        0x00, /* Port 38 ( N/A) */
        0x00, /* Port 39 ( N/A) */
        0x00, /* Port 40 ( N/A) */
        0x00, /* Port 41 ( N/A) */
        0x00, /* Port 42 ( N/A) */
        0x16, /* Port 43 (ge19) ExtBus=0 Addr=0x16 */
        0x17, /* Port 44 (ge20) ExtBus=0 Addr=0x17 */
        0x18, /* Port 45 (ge21) ExtBus=0 Addr=0x18 */
        0x13, /* Port 46 (ge22) ExtBus=0 Addr=0x13 */
        0x14, /* Port 47 (ge23) ExtBus=0 Addr=0x14 */
        0x00, /* Port 48 ( N/A) */
        0x00, /* Port 49 ( N/A) */
        0x00, /* Port 50 ( N/A) */
        0x00, /* Port 51 ( N/A) */
        0x00, /* Port 52 ( N/A) */
        0x00, /* Port 53 ( N/A) */
    };
    uint16 dev_id;
    uint8 rev_id;

    soc_cm_get_id(unit, &dev_id, &rev_id);

    *phy_addr_int = _soc_valkyrie_int_phy_addr[port];
    switch (dev_id) {
    case BCM56620_DEVICE_ID:
        *phy_addr = _soc_phy_addr_bcm56620[port];
        break;
    case BCM56684_DEVICE_ID:
        *phy_addr = _soc_phy_addr_bcm56684[port];
        break;
    case BCM56686_DEVICE_ID:
        *phy_addr = _soc_phy_addr_bcm56686[port];
        break;
    case BCM56680_DEVICE_ID:
    default:
        *phy_addr = _soc_phy_addr_bcm56680[port];
        break;
    }
}
#endif /* BCM_VALKYRIE_SUPPORT */

#ifdef BCM_PETRA_SUPPORT
int
_dpp_phy_addr_multi_get(int unit, soc_port_t port, int is_logical, int array_max,
                       int *array_size, phyident_core_info_t *core_info)
{
    static const uint16 _soc_phy_addr_arad[] = {
        (0x00),                                 /* Phy Port  0 (first phy port is CMIC)*/
        ((3 << PHY_ID_BUS_LOWER_SHIFT) | 0x00 | 0x80), /* Phy Port  1 (WC0 lane 0)  Bus=3 Phy=0  */
        ((3 << PHY_ID_BUS_LOWER_SHIFT) | 0x00 | 0x80), /* Phy Port  2 (WC0 lane 1)  Bus=3 Phy=0  */
        ((3 << PHY_ID_BUS_LOWER_SHIFT) | 0x00 | 0x80), /* Phy Port  3 (WC0 lane 2)  Bus=3 Phy=0  */
        ((3 << PHY_ID_BUS_LOWER_SHIFT) | 0x00 | 0x80), /* Phy Port  4 (WC0 lane 3)  Bus=3 Phy=0  */
        ((3 << PHY_ID_BUS_LOWER_SHIFT) | 0x04 | 0x80), /* Phy Port  5 (WC1 lane 0)  Bus=3 Phy=4  */
        ((3 << PHY_ID_BUS_LOWER_SHIFT) | 0x04 | 0x80), /* Phy Port  6 (WC1 lane 1)  Bus=3 Phy=4  */
        ((3 << PHY_ID_BUS_LOWER_SHIFT) | 0x04 | 0x80), /* Phy Port  7 (WC1 lane 2)  Bus=3 Phy=4  */
        ((3 << PHY_ID_BUS_LOWER_SHIFT) | 0x04 | 0x80), /* Phy Port  8 (WC1 lane 3)  Bus=3 Phy=4  */
        ((3 << PHY_ID_BUS_LOWER_SHIFT) | 0x08 | 0x80), /* Phy Port  9 (WC2 lane 0)  Bus=3 Phy=8  */
        ((3 << PHY_ID_BUS_LOWER_SHIFT) | 0x08 | 0x80), /* Phy Port 10 (WC2 lane 1)  Bus=3 Phy=8  */
        ((3 << PHY_ID_BUS_LOWER_SHIFT) | 0x08 | 0x80), /* Phy Port 11 (WC2 lane 2)  Bus=3 Phy=8  */
        ((3 << PHY_ID_BUS_LOWER_SHIFT) | 0x08 | 0x80), /* Phy Port 12 (WC2 lane 3)  Bus=3 Phy=8  */
        ((3 << PHY_ID_BUS_LOWER_SHIFT) | 0x0c | 0x80), /* Phy Port 13 (WC3 lane 0)  Bus=3 Phy=12 */
        ((3 << PHY_ID_BUS_LOWER_SHIFT) | 0x0c | 0x80), /* Phy Port 14 (WC3 lane 1)  Bus=3 Phy=12 */
        ((3 << PHY_ID_BUS_LOWER_SHIFT) | 0x0c | 0x80), /* Phy Port 15 (WC3 lane 2)  Bus=3 Phy=12 */
        ((3 << PHY_ID_BUS_LOWER_SHIFT) | 0x0c | 0x80), /* Phy Port 16 (WC3 lane 3)  Bus=3 Phy=12 */
        ((3 << PHY_ID_BUS_LOWER_SHIFT) | 0x10 | 0x80), /* Phy Port 17 (WC4 lane 0)  Bus=3 Phy=16 */
        ((3 << PHY_ID_BUS_LOWER_SHIFT) | 0x10 | 0x80), /* Phy Port 18 (WC4 lane 1)  Bus=3 Phy=16 */
        ((3 << PHY_ID_BUS_LOWER_SHIFT) | 0x10 | 0x80), /* Phy Port 19 (WC4 lane 2)  Bus=3 Phy=16 */
        ((3 << PHY_ID_BUS_LOWER_SHIFT) | 0x10 | 0x80), /* Phy Port 20 (WC4 lane 3)  Bus=3 Phy=16 */
        ((3 << PHY_ID_BUS_LOWER_SHIFT) | 0x14 | 0x80), /* Phy Port 21 (WC5 lane 0)  Bus=3 Phy=20 */
        ((3 << PHY_ID_BUS_LOWER_SHIFT) | 0x14 | 0x80), /* Phy Port 22 (WC5 lane 1)  Bus=3 Phy=20 */
        ((3 << PHY_ID_BUS_LOWER_SHIFT) | 0x14 | 0x80), /* Phy Port 23 (WC5 lane 2)  Bus=3 Phy=20 */
        ((3 << PHY_ID_BUS_LOWER_SHIFT) | 0x14 | 0x80), /* Phy Port 24 (WC5 lane 3)  Bus=3 Phy=20 */
        ((3 << PHY_ID_BUS_LOWER_SHIFT) | 0x18 | 0x80), /* Phy Port 25 (WC6 lane 0)  Bus=3 Phy=24 */
        ((3 << PHY_ID_BUS_LOWER_SHIFT) | 0x18 | 0x80), /* Phy Port 26 (WC6 lane 1)  Bus=3 Phy=24 */
        ((3 << PHY_ID_BUS_LOWER_SHIFT) | 0x18 | 0x80), /* Phy Port 27 (WC6 lane 2)  Bus=3 Phy=24 */
        ((3 << PHY_ID_BUS_LOWER_SHIFT) | 0x18 | 0x80), /* Phy Port 28 (WC6 lane 3)  Bus=3 Phy=24 */
        ((2 << PHY_ID_BUS_LOWER_SHIFT) | 0x0c | 0x80), /* Phy Port 29 (WC7 lane 0)  Bus=2 Phy=12 */
        ((2 << PHY_ID_BUS_LOWER_SHIFT) | 0x0c | 0x80), /* Phy Port 30 (WC7 lane 1)  Bus=2 Phy=12 */
        ((2 << PHY_ID_BUS_LOWER_SHIFT) | 0x0c | 0x80), /* Phy Port 31 (WC7 lane 2)  Bus=2 Phy=12 */
        ((2 << PHY_ID_BUS_LOWER_SHIFT) | 0x0c | 0x80), /* Phy Port 32 (WC7 lane 3)  Bus=2 Phy=12 */
        ((3 << PHY_ID_BUS_LOWER_SHIFT) | 0x1c | 0x80), /* Phy Port 33 (MLD 0)       Bus=3 Phy=26 */
        ((3 << PHY_ID_BUS_LOWER_SHIFT) | 0x1d | 0x80), /* Phy Port 34 (MLD 1)       Bus=3 Phy=27 */
        0x00, /* Phy Port 35 (N/A) */
        0x00, /* Phy Port 36 (N/A) */
        0x00, /* Phy Port 37 (N/A) */
        0x00, /* Phy Port 38 (N/A) */
        0x00, /* Phy Port 39 (N/A) */
        ((1 << PHY_ID_BUS_LOWER_SHIFT) | 0x00 | 0x80), /* Phy Port 40 (Fabric Link0)  Bus=1 Phy=0  */
        ((1 << PHY_ID_BUS_LOWER_SHIFT) | 0x00 | 0x80), /* Phy Port 41 (Fabric Link1)  Bus=1 Phy=0  */
        ((1 << PHY_ID_BUS_LOWER_SHIFT) | 0x00 | 0x80), /* Phy Port 42 (Fabric Link2)  Bus=1 Phy=0  */
        ((1 << PHY_ID_BUS_LOWER_SHIFT) | 0x00 | 0x80), /* Phy Port 43 (Fabric Link3)  Bus=1 Phy=0  */
        ((1 << PHY_ID_BUS_LOWER_SHIFT) | 0x04 | 0x80), /* Phy Port 44 (Fabric Link4)  Bus=1 Phy=4  */
        ((1 << PHY_ID_BUS_LOWER_SHIFT) | 0x04 | 0x80), /* Phy Port 45 (Fabric Link5)  Bus=1 Phy=4  */
        ((1 << PHY_ID_BUS_LOWER_SHIFT) | 0x04 | 0x80), /* Phy Port 46 (Fabric Link6)  Bus=1 Phy=4  */
        ((1 << PHY_ID_BUS_LOWER_SHIFT) | 0x04 | 0x80), /* Phy Port 47 (Fabric Link7)  Bus=1 Phy=4  */
        ((1 << PHY_ID_BUS_LOWER_SHIFT) | 0x08 | 0x80), /* Phy Port 48 (Fabric Link8)  Bus=1 Phy=8  */
        ((1 << PHY_ID_BUS_LOWER_SHIFT) | 0x08 | 0x80), /* Phy Port 49 (Fabric Link9)  Bus=1 Phy=8  */
        ((1 << PHY_ID_BUS_LOWER_SHIFT) | 0x08 | 0x80), /* Phy Port 50 (Fabric Link10) Bus=1 Phy=8  */
        ((1 << PHY_ID_BUS_LOWER_SHIFT) | 0x08 | 0x80), /* Phy Port 51 (Fabric Link11) Bus=1 Phy=8  */
        ((1 << PHY_ID_BUS_LOWER_SHIFT) | 0x10 | 0x80), /* Phy Port 52 (Fabric Link12) Bus=1 Phy=16 */
        ((1 << PHY_ID_BUS_LOWER_SHIFT) | 0x10 | 0x80), /* Phy Port 53 (Fabric Link13) Bus=1 Phy=16 */
        ((1 << PHY_ID_BUS_LOWER_SHIFT) | 0x10 | 0x80), /* Phy Port 54 (Fabric Link14) Bus=1 Phy=16 */
        ((1 << PHY_ID_BUS_LOWER_SHIFT) | 0x10 | 0x80), /* Phy Port 55 (Fabric Link15) Bus=1 Phy=16 */
        ((1 << PHY_ID_BUS_LOWER_SHIFT) | 0x14 | 0x80), /* Phy Port 56 (Fabric Link16) Bus=1 Phy=20 */
        ((1 << PHY_ID_BUS_LOWER_SHIFT) | 0x14 | 0x80), /* Phy Port 57 (Fabric Link17) Bus=1 Phy=20 */
        ((1 << PHY_ID_BUS_LOWER_SHIFT) | 0x14 | 0x80), /* Phy Port 58 (Fabric Link18) Bus=1 Phy=20 */
        ((1 << PHY_ID_BUS_LOWER_SHIFT) | 0x14 | 0x80), /* Phy Port 59 (Fabric Link19) Bus=1 Phy=20 */
        ((1 << PHY_ID_BUS_LOWER_SHIFT) | 0x18 | 0x80), /* Phy Port 60 (Fabric Link20) Bus=1 Phy=24 */
        ((1 << PHY_ID_BUS_LOWER_SHIFT) | 0x18 | 0x80), /* Phy Port 61 (Fabric Link21) Bus=1 Phy=24 */
        ((1 << PHY_ID_BUS_LOWER_SHIFT) | 0x18 | 0x80), /* Phy Port 62 (Fabric Link22) Bus=1 Phy=24 */
        ((1 << PHY_ID_BUS_LOWER_SHIFT) | 0x18 | 0x80), /* Phy Port 63 (Fabric Link23) Bus=1 Phy=24 */
        ((2 << PHY_ID_BUS_LOWER_SHIFT) | 0x00 | 0x80), /* Phy Port 64 (Fabric Link24) Bus=2 Phy=0  */
        ((2 << PHY_ID_BUS_LOWER_SHIFT) | 0x00 | 0x80), /* Phy Port 65 (Fabric Link25) Bus=2 Phy=0  */
        ((2 << PHY_ID_BUS_LOWER_SHIFT) | 0x00 | 0x80), /* Phy Port 66 (Fabric Link26) Bus=2 Phy=0  */
        ((2 << PHY_ID_BUS_LOWER_SHIFT) | 0x00 | 0x80), /* Phy Port 67 (Fabric Link27) Bus=2 Phy=0  */
        ((2 << PHY_ID_BUS_LOWER_SHIFT) | 0x04 | 0x80), /* Phy Port 68 (Fabric Link28) Bus=2 Phy=4  */
        ((2 << PHY_ID_BUS_LOWER_SHIFT) | 0x04 | 0x80), /* Phy Port 69 (Fabric Link29) Bus=2 Phy=4  */
        ((2 << PHY_ID_BUS_LOWER_SHIFT) | 0x04 | 0x80), /* Phy Port 70 (Fabric Link30) Bus=2 Phy=4  */
        ((2 << PHY_ID_BUS_LOWER_SHIFT) | 0x04 | 0x80), /* Phy Port 71 (Fabric Link31) Bus=2 Phy=4  */
        ((2 << PHY_ID_BUS_LOWER_SHIFT) | 0x08 | 0x80), /* Phy Port 72 (Fabric Link32) Bus=2 Phy=8  */
        ((2 << PHY_ID_BUS_LOWER_SHIFT) | 0x08 | 0x80), /* Phy Port 73 (Fabric Link33) Bus=2 Phy=8  */
        ((2 << PHY_ID_BUS_LOWER_SHIFT) | 0x08 | 0x80), /* Phy Port 74 (Fabric Link34) Bus=2 Phy=8  */
        ((2 << PHY_ID_BUS_LOWER_SHIFT) | 0x08 | 0x80), /* Phy Port 75 (Fabric Link35) Bus=2 Phy=8  */
    };

    uint32 phy_port;
	
    *array_size = 0;

    if(!is_logical) {
		phy_port = port;
		if(array_max != 1) {
			return SOC_E_PARAM;
		}
    }
	else{
		phy_port = SOC_INFO(unit).port_l2p_mapping[port];
	}

    if(phy_port > 75){
        return SOC_E_PARAM;
    }
	/* array of one element return just the mdio address and type*/
	if(array_max == 1){
		core_info[0].mdio_addr = _soc_phy_addr_arad[phy_port];
		core_info[0].core_type = phyident_core_type_wc;
		core_info[0].mld_index = PHYIDENT_INFO_NOT_SET; 
		core_info[0].index_in_mld = PHYIDENT_INFO_NOT_SET;
		core_info[0].first_phy_in_core = PHYIDENT_INFO_NOT_SET;
		core_info[0].nof_lanes_in_core = PHYIDENT_INFO_NOT_SET;
		*array_size = 1;
		return SOC_E_NONE;
	}

#ifdef BCM_ARAD_SUPPORT
	if (SOC_IS_ARAD(unit)){
		uint32 required_mdio_addr;
		soc_port_if_t interface_type;
		soc_pbmp_t quads_in_use, phys_in_use;
		uint32 lanes_in_quad;
		uint32 quad_id;
		uint32 mlds_in_use = 0;
		uint8 is_elk_ilkn_no_mld = FALSE;
		uint8 is_caui_ten_lanes_allowed = FALSE;
		int i;
		int rv;

		rv = soc_port_sw_db_interface_type_get(unit, port, &interface_type);
		if (rv != SOC_E_NONE) {
			return SOC_E_FAIL;
		}
#if defined(INCLUDE_KBP) && !defined(BCM_88030) && defined(BCM_88660)
		if(SOC_IS_ARADPLUS(unit)&& (SOC_DPP_CONFIG(unit)->arad->init.elk.ext_interface_mode)){
			soc_ilkn_config_t ilkn_config;

			is_caui_ten_lanes_allowed = TRUE;
			if(interface_type == SOC_PORT_IF_ILKN){
				rv = soc_port_sw_db_ilkn_config_get(unit, port, &ilkn_config);
				if (rv != SOC_E_NONE) {
					return SOC_E_FAIL;
				}
				/* ILKN1 in ext_interface_mode of ARAD PLUS does not use MLDs*/
				is_elk_ilkn_no_mld = (ilkn_config.ilkn_id == 1);
			}
		}
#endif
		/* calaculate the required mdio entries (WCs+MLDs)*/
		rv = soc_port_sw_db_serdes_quads_in_use_get(unit, port, &quads_in_use);
		if (rv != SOC_E_NONE) {
			return SOC_E_FAIL;
		}
		SOC_PBMP_COUNT(quads_in_use, required_mdio_addr);
		if (!is_elk_ilkn_no_mld)
		{
			
			required_mdio_addr += (required_mdio_addr - 1)/ 3 + 1;
		}

		if(array_max < required_mdio_addr) {
			return SOC_E_PARAM;
		}

		/* add the cores to the array*/
		rv = soc_port_sw_db_phy_ports_get(unit, port, &phys_in_use);
		if (rv != SOC_E_NONE) {
			return SOC_E_FAIL;
		}
		SOC_PBMP_ITER(quads_in_use, quad_id)
		{
			if(quad_id > 7){
				return SOC_E_FAIL;
			}
			phy_port = (quad_id * LANES_IN_QUAD) + 1;
			/*add core_info*/
			core_info[*array_size].mdio_addr = _soc_phy_addr_arad[phy_port];
			core_info[*array_size].core_type = phyident_core_type_wc;
#if defined(INCLUDE_KBP) && !defined(BCM_88030) && defined(BCM_88660)
			if (is_elk_ilkn_no_mld){
				core_info[*array_size].mld_index = PHYIDENT_INFO_NOT_SET; 
				core_info[*array_size].index_in_mld = PHYIDENT_INFO_NOT_SET;
			} else
#endif /* defined(INCLUDE_KBP) && !defined(BCM_88030) && defined(BCM_88660) */
			{
				core_info[*array_size].mld_index = quad_id < 4 ? 0 : 1; 
				core_info[*array_size].index_in_mld = quad_id - (quad_id / 4) * 4;
				SHR_BITSET(&mlds_in_use, core_info[*array_size].mld_index);
			}		
			if((interface_type == SOC_PORT_IF_ILKN) || (is_caui_ten_lanes_allowed))
			{
				lanes_in_quad = 0;
				for ( i = 0 ; i < LANES_IN_QUAD; i++)
				{
					if(SOC_PBMP_MEMBER(phys_in_use, phy_port + i)){
						if(lanes_in_quad == 0){
							core_info[*array_size].first_phy_in_core = i;
						}
						lanes_in_quad++;
					}
				}
				core_info[*array_size].nof_lanes_in_core = lanes_in_quad;
			}
			(*array_size)++;
		}

		/*add the MLDs*/
		for(i = 0 ; i < 2 ; i++)
		{
			if(!SHR_BITGET(&mlds_in_use, i)){
				continue;
			}
			core_info[*array_size].mdio_addr = _soc_phy_addr_arad[33+i];
			core_info[*array_size].core_type = phyident_core_type_mld; 
			core_info[*array_size].mld_index = i;  
			core_info[*array_size].index_in_mld = 0;
			core_info[*array_size].first_phy_in_core = PHYIDENT_INFO_NOT_SET;
			core_info[*array_size].nof_lanes_in_core = PHYIDENT_INFO_NOT_SET;
			(*array_size)++;
		}
	}
#endif
    return SOC_E_NONE;
}

STATIC void
_dpp_phy_addr_default(int unit, int port,
                          uint16 *phy_addr, uint16 *phy_addr_int)
{
    int array_size, rv;
    phyident_core_info_t core_info;

    phyident_core_info_t_init(&core_info);

    *phy_addr = 0;
    rv = _dpp_phy_addr_multi_get(unit, port, 1, 1, &array_size, &core_info);
    *phy_addr_int = core_info.mdio_addr;

    if(SOC_FAILURE(rv) || array_size==0){
        SOC_DEBUG_PRINT((DK_ERR, "_dpp_phy_addr_default: failed to get address for port %d\n", port));
    }
}
#endif /* BCM_PETRA_SUPPORT */

#ifdef BCM_DFE_SUPPORT
STATIC void
_dfe_phy_addr_default(int unit, int port,
                          uint16 *phy_addr, uint16 *phy_addr_int)
{
    uint32 bus_id;

    /* Physical port number starts from 0
     * Internal/external MDIO bus number = port number / 32
     * Internal MDIO address = (port number % 32) / 4
     */
    bus_id = port / 32;
    bus_id = ((bus_id & 0x3)<<PHY_ID_BUS_LOWER_SHIFT) | ((bus_id & 0xc)<<PHY_ID_BUS_UPPER_SHIFT);
    *phy_addr_int = 0x80 | bus_id | ((port % 32)/4);

    /* external MDIO address */
    *phy_addr = bus_id | ((port % 32)/4);
}
#endif /* BCM_DFE_SUPPORT */

#ifdef BCM_TRIDENT_SUPPORT
STATIC void
_trident_phy_addr_default(int unit, int port,
                          uint16 *phy_addr, uint16 *phy_addr_int)
{
    soc_info_t *si;
    int phy_port;

    si = &SOC_INFO(unit);

    phy_port = si->port_l2p_mapping[port];
    if (phy_port == -1) {
        *phy_addr_int = 0;
        *phy_addr = 0;
        return;
    }

    /* Physical port number starts from 1
     * Internal/external MDIO bus number = (physical port number - 1) / 24
     * Internal MDIO address = (physical port number - 1) % 24 + 1
     */
    *phy_addr_int = 0x80 | (((phy_port - 1) / 24) << 5) | 
                           ((((phy_port - 1) % 24)/4)*4 + 1);

    /* external MDIO address */
    *phy_addr = (((phy_port - 1) / 24) << 5) | 
                ((phy_port - 1) % 24 + 4);

    soc_cm_debug(DK_ERR, "_trident_phy_addr_default:port:%d, phy_port:%d, phy_addr_int:%d, phy_addr:%d\n",
				         port, phy_port, *phy_addr_int, *phy_addr);
}
#endif /* BCM_TRIDENT_SUPPORT */

#ifdef BCM_TRIDENT2_SUPPORT
STATIC void
_trident2_phy_addr_default(int unit, int port,
                           uint16 *phy_addr, uint16 *phy_addr_int)
{
    soc_info_t *si;
    int phy_port, bus_id_encoding, phy_id;

    si = &SOC_INFO(unit);

    phy_port = si->port_l2p_mapping[port];
    if (phy_port == -1) {
        *phy_addr_int = 0;
        *phy_addr = 0;
        return;
    }

    /*
     * Internal phy address:
     * bus 0 phy 1 to 20 are mapped to Physical port 1 to 20
     * bus 1 phy 1 to 24 are mapped to Physical port 21 to 44
     * bus 2 phy 1 to 20 are mapped to Physical port 45 to 64
     * bus 3 phy 1 to 20 are mapped to Physical port 65 to 84
     * bus 4 phy 1 to 24 are mapped to Physical port 85 to 108
     * bus 5 phy 1 to 20 are mapped to Physical port 109 to 128
     */
    if (phy_port < 65) { /* X pipe */
        if (phy_port < 21) {
            bus_id_encoding = 0;     /* bus 0, bit {8,6,5} = 0 */
            phy_id = phy_port;
        } else if (phy_port < 45) {
            bus_id_encoding = 0x020; /* bus 1, bit {8,6,5} = 1 */
            phy_id = phy_port - 20;
        } else {
            bus_id_encoding = 0x040; /* bus 2, bit {8,6,5} = 2 */
            phy_id = phy_port - 44;
        }
    } else {
        if (phy_port < 85) {
            bus_id_encoding = 0x060; /* bus 3, bit {8,6,5} = 3 */
            phy_id = phy_port - 64;
        } else if (phy_port < 109) {
            bus_id_encoding = 0x100; /* bus 4, bit {8,6,5} = 4 */
            phy_id = phy_port - 84;
        } else {
            bus_id_encoding = 0x120; /* bus 5, bit {8,6,5} = 5 */
            phy_id = phy_port - 108;
        }
    }
    *phy_addr_int = 0x80 | bus_id_encoding | phy_id;

    /*
     * External phy address:
     * bus 0 phy 1 to 31 are mapped to Physical port 1 to 32
     * bus 1 phy 1 to 31 are mapped to Physical port 33 to 64
     * bus 2 phy 1 to 31 are mapped to Physical port 65 to 96
     * bus 3 phy 1 to 31 are mapped to Physical port 97 to 128
     */
    bus_id_encoding = (phy_port - 1) & 0x060;
    phy_id = (phy_port - 1) & 0x1f;
    /* external phy address encoding */
    *phy_addr = bus_id_encoding | phy_id;
}
#endif /* BCM_TRIDENT2_SUPPORT */

#ifdef BCM_SHADOW_SUPPORT
STATIC void
_shadow_phy_addr_default(int unit, int port,
                          uint16 *phy_addr, uint16 *phy_addr_int)
{
    soc_info_t *si;
    int phy_port;
    static const uint16 _soc_phy_addr_bcm88732_a0[] = {
        0,
        1, 1, 1, 1,       /* Radian port 1-4 => WC0 MDIO Port Address 1 */ 
        5, 5, 5, 5,       /* Radian port 5-8 => WC0 MDIO Port Address 5 */
        9, 9, 9, 9,       /* Radian port 9-10 => WC0 MDIO Port Address 9 */ 
        13, 13, 13, 13,   /* Radian port 11-12 => WC0 MDIO Port Address 13 */ 
        17, 17, 17, 17,   /* Radian port 13-14 => WC0 MDIO Port Address 17 */
        21, 21, 21, 21    /* Radian port 15-16 => WC0 MDIO Port Address 21 */
    };
    static const uint16 _soc_phy_addr_switch_link_xaui[] = {
        9,  9,       /* Radian port 9-10 => WC0 MDIO Port Address 9 */ 
        13, 13,    /* Radian port 11-12 => WC0 MDIO Port Address 13 */ 
        17, 17,    /* Radian port 13-14 => WC0 MDIO Port Address 17 */
        21, 21    /* Radian port 15-16 => WC0 MDIO Port Address 21 */
    };

    static const uint16 _soc_phy_addr_switch_link_r2[] = {
        9,  9,       /* Radian port 9-10 => WC0 MDIO Port Address 9 */ 
        13, 13,    /* Radian port 11-12 => WC0 MDIO Port Address 13 */ 
        17, 17,    /* Radian port 13-14 => WC0 MDIO Port Address 17 */
        21, 21    /* Radian port 15-16 => WC0 MDIO Port Address 21 */
    };
    si = &SOC_INFO(unit);

    if (soc_feature(unit, soc_feature_logical_port_num)) {
        phy_port = si->port_l2p_mapping[port];
    } else {
        phy_port = port;
    }
    if (phy_port < 1 || phy_port > 16) {
        *phy_addr_int = 0;
        *phy_addr = 0;
        return;
    }
    if (phy_port < 9) {
       *phy_addr_int = 0x80 | (_soc_phy_addr_bcm88732_a0[phy_port]);
       *phy_addr = *phy_addr_int; /* no external PHYs */
    } else {  /* switch link side */
        if (soc_property_get(unit, spn_BCM88732_1X40_4X10, 0) || 
            soc_property_get(unit, spn_BCM88732_4X10_4X10, 0)) {
            *phy_addr_int = 0x80 | (_soc_phy_addr_switch_link_xaui[phy_port-9]);
            *phy_addr = *phy_addr_int; /* no external PHYs */
        } else if (soc_property_get(unit, spn_BCM88732_2X40_8X12, 0) || 
            soc_property_get(unit, spn_BCM88732_1X40_4X10_8X12, 0) ||
            soc_property_get(unit, spn_BCM88732_4X10_1X40_8X12, 0) ||
            soc_property_get(unit, spn_BCM88732_8X10_8X12, 0) ||
            soc_property_get(unit, spn_BCM88732_8X10_4X12, 0) ||
            soc_property_get(unit, spn_BCM88732_6X10_2X12, 0) ||
            soc_property_get(unit, spn_BCM88732_8X10_2X12, 0) ||
            soc_property_get(unit, spn_BCM88732_2X40_8X10, 0) ||
            soc_property_get(unit, spn_BCM88732_8X10_8X10, 0)) {
            *phy_addr_int = 0x80 | (_soc_phy_addr_switch_link_r2[phy_port-9]);
            *phy_addr = *phy_addr_int; /* no external PHYs */
        } else {
            *phy_addr_int = 0x80 | (_soc_phy_addr_bcm88732_a0[phy_port]);
            if (soc_property_get(unit, spn_BCM88732_2X40_2X40, 0) ||
                soc_property_get(unit, spn_BCM88732_8X10_2X40, 0) ||
                (soc_property_get(unit, "IL3125", 0)))  { /* for 16Lanes */
                /* 
                 * Use the MDIO address assignment similar to that of 
                 * Switch link port configuration. 
                 */
                *phy_addr_int = 0x80 | (_soc_phy_addr_switch_link_r2[phy_port-9]);
            } else { 
                 *phy_addr_int = 0x80 | (_soc_phy_addr_bcm88732_a0[phy_port]);
            }
            *phy_addr = *phy_addr_int; /* no external PHYs */
        }
    }
}
#endif /* BCM_SHADOW_SUPPORT */

#ifdef BCM_SCORPION_SUPPORT
static int
_scorpion_phy_addr_default(int unit, int port,
                           uint16 *phy_addr, uint16 *phy_addr_int)
{
    static const uint16 _soc_phy_addr_bcm56820_a0[] = {
 /* extPhy intPhy          */
    0x00, 0x00, /* Port  0 (cmic) N/A */
    0x42, 0xc1, /* Port  1 ( xe0) */
    0x43, 0xc2, /* Port  2 ( xe1) */
    0x44, 0xc3, /* Port  3 ( xe2) */
    0x45, 0xc4, /* Port  4 ( xe3) */
    0x46, 0xc5, /* Port  5 ( xe4) */
    0x47, 0xc6, /* Port  6 ( xe5) */
    0x48, 0xc7, /* Port  7 ( xe6) */
    0x49, 0xc8, /* Port  8 ( xe7) */
    0x4a, 0xc9, /* Port  9 ( xe8) */
    0x4b, 0xca, /* Port 10 ( xe9) */
    0x4c, 0xcb, /* Port 11 (xe10) */
    0x4d, 0xcc, /* Port 12 (xe11) */
    0x4e, 0xcd, /* Port 13 (xe12) */
    0x4f, 0xce, /* Port 14 (xe13) */
    0x50, 0xcf, /* Port 15 (xe14) */
    0x51, 0xd0, /* Port 16 (xe15) */
    0x52, 0xd1, /* Port 17 (xe16) */
    0x53, 0xd2, /* Port 18 (xe17) */
    0x54, 0xd3, /* Port 19 (xe18) */
    0x55, 0xd4, /* Port 20 (xe19) */
    0x21, 0xd5, /* Port 21 (xe20) */
    0x26, 0xd6, /* Port 22 (xe21) */
    0x2b, 0xd7, /* Port 23 (xe22) */
    0x30, 0xd8, /* Port 24 (xe23) */
    0x59, 0xd9, /* Port 25 (ge0) */
    0x5a, 0xda, /* Port 26 (ge1) */
    0x5b, 0xdb, /* Port 27 (ge2) */
    0x5c, 0xdc  /* Port 28 (ge3) */
    };
    int rv = TRUE;
    uint16 dev_id;
    uint8 rev_id;
                                                                                
    soc_cm_get_id(unit, &dev_id, &rev_id);

    switch (dev_id) {
    case BCM56820_DEVICE_ID:
        *phy_addr = _soc_phy_addr_bcm56820_a0[port * 2];
        *phy_addr_int = _soc_phy_addr_bcm56820_a0[port * 2 + 1];
        break;
    default:
        rv = FALSE;
        break;
    }
    return rv;
}
#endif /* BCM_SCORPION_SUPPORT */

#ifdef BCM_XGS3_SWITCH_SUPPORT
static void
_xgs3_phy_addr_default(int unit, int port,
                      uint16 *phy_addr, uint16 *phy_addr_int)
{
    int phy_addr_adjust = 0;

    if (SOC_IS_GOLDWING(unit)) {
        /* In Goldwing due to the hardware port remapping, we need to adjust
         * the PHY addresses. The PHY addresses are remapped as follow.
         * Original : 0  ... 13 14 15 16 17 18 19
         * Remapped : 0  ... 13 16 17 18 19 14 15
         */
        if (port == 14 || port == 15) {
            phy_addr_adjust = 4;
        } else if (port > 15) {
            phy_addr_adjust = -2;
        }
    }
    if (IS_HG_PORT(unit, port) || IS_XE_PORT(unit, port) ||
        IS_GX_PORT(unit, port)) {
        pbmp_t pbm;
        int    temp_port;
        int    found     = 0;
        int    mdio_addr = 1;
        /*
         * Internal XAUI (Internal sel bit 0x80) on XPORT MDIO bus(0x40)
         * External Phy  (Internal sel bit 0x00) on XPORT MDIO bus(0x40)
         * Assume External MDIO address starts at 1 as this is on a
         * seperate BUS.
         */

        /*
         * First, assign lowest addresses to GE ports which are also
         * GX ports.
         */
        pbm = PBMP_GX_ALL(unit);
        SOC_PBMP_AND(pbm, PBMP_GE_ALL(unit));

        PBMP_ITER(pbm, temp_port) {
            if (temp_port == port) {
                found = 1;
                break;
            }
            mdio_addr++;
        }

        /*
         * Second, assign external addresses for XE ports.
         */
        if (!found) {
            PBMP_XE_ITER(unit, temp_port) {
                if  (temp_port == port) {
                    found = 1;
                    break;
                }
                mdio_addr++;
            }
        }

        /*
         * Finally, assign external adddresses for HG ports.
         */
        if (!found) {
            PBMP_HG_ITER(unit, temp_port) {
                if  (temp_port == port) {
                    found = 1;
                    break;
                }
                mdio_addr++;
            }
        }

        *phy_addr = mdio_addr + 0x40;
        *phy_addr_int = port + 0xc0 + phy_addr_adjust;

        if (SOC_IS_SC_CQ(unit)) {
            *phy_addr_int = port + 0xc0;
            *phy_addr = port + 0x41;
        }
        if (SAL_BOOT_QUICKTURN) {
            *phy_addr = port + 0x41;
            if (SOC_IS_SCORPION(unit)) {
                /* Skip over CMIC at port 0 */
                *phy_addr -= 1;
            }
        }
    } else {
        /*
         * Internal Serdes (Internal sel bit 0x80) on GPORT MDIO bus(0x00)
         * External Phy    (Internal sel bit 0x00) on GPORT MDIO bus(0x00)
         */
        *phy_addr = port + 1 + phy_addr_adjust;
        *phy_addr_int = port + 0x80 + phy_addr_adjust;
        if (SOC_IS_SC_CQ(unit)) {
            *phy_addr_int = port + 0xc0;
            *phy_addr = port + 0x40;
        }
    }
}
#endif /* BCM_XGS3_SWITCH_SUPPORT */

#ifdef BCM_SBX_SUPPORT
typedef struct _block_phy_addr_s {
    int     block_type;
    int     block_number;
    int     block_index;
    uint16  addr;
    uint16  addr_int;
} _block_phy_addr_t;

#ifdef BCM_BM9600_SUPPORT
static _block_phy_addr_t   _soc_phy_addr_bcm88130_a0[] = {
    { SOC_BLK_GXPORT,    0x0,    0,   0x0,    0x0 },  /* sfi0  */
    { SOC_BLK_GXPORT,    0x0,    1,   0x0,    0x0 },  /* sfi1  */
    { SOC_BLK_GXPORT,    0x0,    2,   0x0,    0x0 },  /* sfi2  */
    { SOC_BLK_GXPORT,    0x0,    3,   0x0,    0x0 },  /* sfi3  */
    { SOC_BLK_GXPORT,    0x1,    4,   0x1,    0x1 },  /* sfi4  */
    { SOC_BLK_GXPORT,    0x1,    5,   0x1,    0x1 },  /* sfi5  */
    { SOC_BLK_GXPORT,    0x1,    6,   0x1,    0x1 },  /* sfi6  */
    { SOC_BLK_GXPORT,    0x1,    7,   0x1,    0x1 },  /* sfi7  */
    { SOC_BLK_GXPORT,    0x2,    8,   0x2,    0x2 },  /* sfi8  */
    { SOC_BLK_GXPORT,    0x2,    9,   0x2,    0x2 },  /* sfi9  */
    { SOC_BLK_GXPORT,    0x2,   10,   0x2,    0x2 },  /* sfi10 */
    { SOC_BLK_GXPORT,    0x2,   11,   0x2,    0x2 },  /* sfi11 */
    { SOC_BLK_GXPORT,    0x3,   12,   0x3,    0x3 },  /* sfi12 */
    { SOC_BLK_GXPORT,    0x3,   13,   0x3,    0x3 },  /* sfi13 */
    { SOC_BLK_GXPORT,    0x3,   14,   0x3,    0x3 },  /* sfi14 */
    { SOC_BLK_GXPORT,    0x3,   15,   0x3,    0x3 },  /* sfi15 */
    { SOC_BLK_GXPORT,    0x4,   16,   0x4,    0x4 },  /* sfi16 */
    { SOC_BLK_GXPORT,    0x4,   17,   0x4,    0x4 },  /* sfi17 */
    { SOC_BLK_GXPORT,    0x4,   18,   0x4,    0x4 },  /* sfi18 */
    { SOC_BLK_GXPORT,    0x4,   19,   0x4,    0x4 },  /* sfi19 */
    { SOC_BLK_GXPORT,    0x5,   20,   0x5,    0x5 },  /* sfi20 */
    { SOC_BLK_GXPORT,    0x5,   21,   0x5,    0x5 },  /* sfi21 */
    { SOC_BLK_GXPORT,    0x5,   22,   0x5,    0x5 },  /* sfi22 */
    { SOC_BLK_GXPORT,    0x5,   23,   0x5,    0x5 },  /* sfi23 */
    { SOC_BLK_GXPORT,    0x6,   24,   0x6,    0x6 },  /* sfi24  */
    { SOC_BLK_GXPORT,    0x6,   25,   0x6,    0x6 },  /* sfi25  */
    { SOC_BLK_GXPORT,    0x6,   26,   0x6,    0x6 },  /* sfi26  */
    { SOC_BLK_GXPORT,    0x6,   27,   0x6,    0x6 },  /* sfi27  */
    { SOC_BLK_GXPORT,    0x7,   28,   0x7,    0x7 },  /* sfi28 */
    { SOC_BLK_GXPORT,    0x7,   29,   0x7,    0x7 },  /* sfi29  */
    { SOC_BLK_GXPORT,    0x7,   30,   0x7,    0x7 },  /* sfi30  */
    { SOC_BLK_GXPORT,    0x7,   31,   0x7,    0x7 },  /* sfi31  */
    { SOC_BLK_GXPORT,    0x8,   32,   0x8,    0x8 },  /* sfi32  */
    { SOC_BLK_GXPORT,    0x8,   33,   0x8,    0x8 },  /* sfi33  */
    { SOC_BLK_GXPORT,    0x8,   34,   0x8,    0x8 },  /* sfi34  */
    { SOC_BLK_GXPORT,    0x8,   35,   0x8,    0x8 },  /* sfi35  */
    { SOC_BLK_GXPORT,    0x9,   36,   0x9,    0x9 },  /* sfi36 */
    { SOC_BLK_GXPORT,    0x9,   37,   0x9,    0x9 },  /* sfi37 */
    { SOC_BLK_GXPORT,    0x9,   38,   0x9,    0x9 },  /* sfi38 */
    { SOC_BLK_GXPORT,    0x9,   39,   0x9,    0x9 },  /* sfi39 */
    { SOC_BLK_GXPORT,    0xa,   40,   0xa,    0xa },  /* sfi40 */
    { SOC_BLK_GXPORT,    0xa,   41,   0xa,    0xa },  /* sfi41 */
    { SOC_BLK_GXPORT,    0xa,   42,   0xa,    0xa },  /* sfi42 */
    { SOC_BLK_GXPORT,    0xa,   43,   0xa,    0xa },  /* sfi43 */
    { SOC_BLK_GXPORT,    0xb,   44,   0xb,    0xb },  /* sfi44 */
    { SOC_BLK_GXPORT,    0xb,   45,   0xb,    0xb },  /* sfi45 */
    { SOC_BLK_GXPORT,    0xb,   46,   0xb,    0xb },  /* sfi46 */
    { SOC_BLK_GXPORT,    0xb,   47,   0xb,    0xb },  /* sfi47 */
    { SOC_BLK_GXPORT,    0xc,   48,   0xc,    0xc },  /* sfi48 */
    { SOC_BLK_GXPORT,    0xc,   49,   0xc,    0xc },  /* sfi49 */
    { SOC_BLK_GXPORT,    0xc,   50,   0xc,    0xc },  /* sfi50  */
    { SOC_BLK_GXPORT,    0xc,   51,   0xc,    0xc },  /* sfi51  */
    { SOC_BLK_GXPORT,    0xd,   52,   0xd,    0xd },  /* sfi52  */
    { SOC_BLK_GXPORT,    0xd,   53,   0xd,    0xd },  /* sfi53  */
    { SOC_BLK_GXPORT,    0xd,   54,   0xd,    0xd },  /* sfi54  */
    { SOC_BLK_GXPORT,    0xd,   55,   0xd,    0xd },  /* sfi55  */
    { SOC_BLK_GXPORT,    0xe,   56,   0xe,    0xe },  /* sfi56  */
    { SOC_BLK_GXPORT,    0xe,   57,   0xe,    0xe },  /* sfi57  */
    { SOC_BLK_GXPORT,    0xe,   58,   0xe,    0xe },  /* sfi58  */
    { SOC_BLK_GXPORT,    0xe,   59,   0xe,    0xe },  /* sfi59  */
    { SOC_BLK_GXPORT,    0xf,   60,   0xf,    0xf },  /* sfi60  */
    { SOC_BLK_GXPORT,    0xf,   61,   0xf,    0xf },  /* sfi61  */
    { SOC_BLK_GXPORT,    0xf,   62,   0xf,    0xf },  /* sfi62 */
    { SOC_BLK_GXPORT,    0xf,   63,   0xf,    0xf },  /* sfi63 */
    { SOC_BLK_GXPORT,    0x10,  64,   0x10,   0x10 },  /* sfi64 */
    { SOC_BLK_GXPORT,    0x10,  65,   0x10,   0x10 },  /* sfi65 */
    { SOC_BLK_GXPORT,    0x10,  66,   0x10,   0x10 },  /* sfi66 */
    { SOC_BLK_GXPORT,    0x10,  67,   0x10,   0x10 },  /* sfi67 */
    { SOC_BLK_GXPORT,    0x11,  68,   0x11,   0x11 },  /* sfi68 */
    { SOC_BLK_GXPORT,    0x11,  69,   0x11,   0x11 },  /* sfi69 */
    { SOC_BLK_GXPORT,    0x11,  70,   0x11,   0x11 },  /* sfi70 */
    { SOC_BLK_GXPORT,    0x11,  71,   0x11,   0x11 },  /* sfi71 */
    { SOC_BLK_GXPORT,    0x12,  72,   0x12,   0x12 },  /* sfi72 */
    { SOC_BLK_GXPORT,    0x12,  73,   0x12,   0x12 },  /* sfi73 */
    { SOC_BLK_GXPORT,    0x12,  74,   0x12,   0x12 },  /* sfi74 */
    { SOC_BLK_GXPORT,    0x12,  75,   0x12,   0x12 },  /* sfi75 */
    { SOC_BLK_GXPORT,    0x13,  76,   0x13,   0x13 },  /* sfi76  */
    { SOC_BLK_GXPORT,    0x13,  77,   0x13,   0x13 },  /* sfi77  */
    { SOC_BLK_GXPORT,    0x13,  78,   0x13,   0x13 },  /* sfi78  */
    { SOC_BLK_GXPORT,    0x13,  79,   0x13,   0x13 },  /* sfi79  */
    { SOC_BLK_GXPORT,    0x14,  80,   0x14,   0x14 },  /* sfi80  */
    { SOC_BLK_GXPORT,    0x14,  81,   0x14,   0x14 },  /* sfi81  */
    { SOC_BLK_GXPORT,    0x14,  82,   0x14,   0x14 },  /* sfi82  */
    { SOC_BLK_GXPORT,    0x14,  83,   0x14,   0x14 },  /* sfi83  */
    { SOC_BLK_GXPORT,    0x15,  84,   0x15,   0x15 },  /* sfi84  */
    { SOC_BLK_GXPORT,    0x15,  85,   0x15,   0x15 },  /* sfi85  */
    { SOC_BLK_GXPORT,    0x15,  86,   0x15,   0x15 },  /* sfi86  */
    { SOC_BLK_GXPORT,    0x15,  87,   0x15,   0x15 },  /* sfi87  */
    { SOC_BLK_GXPORT,    0x16,  88,   0x16,   0x16 },  /* sfi88 */
    { SOC_BLK_GXPORT,    0x16,  89,   0x16,   0x16 },  /* sfi89 */
    { SOC_BLK_GXPORT,    0x16,  90,   0x16,   0x16 },  /* sfi90 */
    { SOC_BLK_GXPORT,    0x16,  91,   0x16,   0x16 },  /* sfi91 */
    { SOC_BLK_GXPORT,    0x17,  92,   0x17,   0x17 },  /* sfi92 */
    { SOC_BLK_GXPORT,    0x17,  93,   0x17,   0x17 },  /* sfi93 */
    { SOC_BLK_GXPORT,    0x17,  94,   0x17,   0x17 },  /* sfi94 */
    { SOC_BLK_GXPORT,    0x17,  95,   0x17,   0x17 },  /* sfi95 */
    {            -1,   -1,   -1,   0x0,   0x00 }   /* Last */
};

static void
_bm9600_phy_addr_default(int unit, int port,
                         uint16 *phy_addr, uint16 *phy_addr_int)
{
  _block_phy_addr_t  *phy_info;

  *phy_addr = *phy_addr_int = 0x0;
  phy_info = &_soc_phy_addr_bcm88130_a0[0];

  while (phy_info->block_type != -1) {
    if (phy_info->block_index == port) {
      /* Found */
      *phy_addr     = phy_info->addr;
      *phy_addr_int = phy_info->addr_int;
      break;
    }

    phy_info++;
  }
}
#endif /* BCM_BM9600_SUPPORT */

static _block_phy_addr_t   _soc_phy_addr_bcm88020_a0[] = {
    { SOC_BLK_GPORT,    0,    0,   0x0,    0x80 },  /* ge0  */
    { SOC_BLK_GPORT,    0,    1,   0x1,    0x81 },  /* ge1  */
    { SOC_BLK_GPORT,    0,    2,   0x2,    0x82 },  /* ge2  */
    { SOC_BLK_GPORT,    0,    3,   0x3,    0x83 },  /* ge3  */
    { SOC_BLK_GPORT,    0,    4,   0x4,    0x84 },  /* ge4  */
    { SOC_BLK_GPORT,    0,    5,   0x5,    0x85 },  /* ge5  */
    { SOC_BLK_GPORT,    0,    6,   0x6,    0x86 },  /* ge6  */
    { SOC_BLK_GPORT,    0,    7,   0x7,    0x87 },  /* ge7  */
    { SOC_BLK_GPORT,    0,    8,   0x8,    0x88 },  /* ge8  */
    { SOC_BLK_GPORT,    0,    9,   0x9,    0x89 },  /* ge9  */
    { SOC_BLK_GPORT,    0,   10,   0xa,    0x8a },  /* ge10 */
    { SOC_BLK_GPORT,    0,   11,   0xb,    0x8b },  /* ge11 */
    { SOC_BLK_GPORT,    1,    0,   0xc,    0x8c },  /* ge12 */
    { SOC_BLK_GPORT,    1,    1,   0xd,    0x8d },  /* ge13 */
    { SOC_BLK_GPORT,    1,    2,   0xe,    0x8e },  /* ge14 */
    { SOC_BLK_GPORT,    1,    3,   0xf,    0x8f },  /* ge15 */
    { SOC_BLK_GPORT,    1,    4,  0x10,    0x90 },  /* ge16 */
    { SOC_BLK_GPORT,    1,    5,  0x11,    0x91 },  /* ge17 */
    { SOC_BLK_GPORT,    1,    6,  0x12,    0x92 },  /* ge18 */
    { SOC_BLK_GPORT,    1,    7,  0x13,    0x93 },  /* ge19 */
    { SOC_BLK_GPORT,    1,    8,  0x14,    0x94 },  /* ge20 */
    { SOC_BLK_GPORT,    1,    9,  0x15,    0x95 },  /* ge21 */
    { SOC_BLK_GPORT,    1,   10,  0x16,    0x96 },  /* ge22 */
    { SOC_BLK_GPORT,    1,   11,  0x17,    0x97 },  /* ge23 */
    { SOC_BLK_XPORT,    0,    0,  0x58,    0xd8 },  /* xe0  */
    { SOC_BLK_XPORT,    1,    0,  0x59,    0xd9 },  /* xe1  */
    {            -1,   -1,   -1,   0x0,    0x00 }   /* Last */
};

/*  Note:
 *  ge8-ge11 share the same internal phy as xe2
 *  ge20-ge23 share the same internal phy as xe3 
 */

static _block_phy_addr_t   _soc_phy_addr_bcm88025_a0[] = {
    { SOC_BLK_GPORT,    0,    0,   0x0,    0x80 },   /* ge0  */
    { SOC_BLK_GPORT,    0,    1,   0x1,    0x81 },   /* ge1  */
    { SOC_BLK_GPORT,    0,    2,   0x2,    0x82 },   /* ge2  */
    { SOC_BLK_GPORT,    0,    3,   0x3,    0x83 },   /* ge3  */
    { SOC_BLK_GPORT,    0,    4,   0x4,    0x84 },   /* ge4  */
    { SOC_BLK_GPORT,    0,    5,   0x5,    0x85 },   /* ge5  */
    { SOC_BLK_GPORT,    0,    6,   0x6,    0x86 },   /* ge6  */
    { SOC_BLK_GPORT,    0,    7,   0x7,    0x87 },   /* ge7  */
    { SOC_BLK_GPORT,    0,    8,   0x8,    0xd2 },   /* ge8  */
    { SOC_BLK_GPORT,    0,    9,   0x9,    0xd2 },   /* ge9  */
    { SOC_BLK_GPORT,    0,   10,   0xa,    0xd2 },   /* ge10 */
    { SOC_BLK_GPORT,    0,   11,   0xb,    0xd2 },   /* ge11 */
    { SOC_BLK_XPORT,    0,    0,  0x50,    0xd0 },   /* xe0  */
    { SOC_BLK_XPORT,    1,    0,  0x51,    0xd1 },   /* xe1  */
    { SOC_BLK_XPORT,    2,    0,  0x52,    0xd2 },   /* xe2  */
    { SOC_BLK_XPORT,    3,    0,  0x53,    0xd3 },   /* xe3  */
    { SOC_BLK_GPORT,    1,    0,   0xc,    0x88 },   /* ge12 */
    { SOC_BLK_GPORT,    1,    1,   0xd,    0x89 },   /* ge13 */
    { SOC_BLK_GPORT,    1,    2,   0xe,    0x8a },   /* ge14 */
    { SOC_BLK_GPORT,    1,    3,   0xf,    0x8b },   /* ge15 */
    { SOC_BLK_GPORT,    1,    4,  0x10,    0x8c },   /* ge16 */
    { SOC_BLK_GPORT,    1,    5,  0x11,    0x8d },   /* ge17 */
    { SOC_BLK_GPORT,    1,    6,  0x12,    0x8e },   /* ge18 */
    { SOC_BLK_GPORT,    1,    7,  0x13,    0x8f },   /* ge19 */
    { SOC_BLK_GPORT,    1,    8,  0x14,    0xd3 },   /* ge20 */
    { SOC_BLK_GPORT,    1,    9,  0x15,    0xd3 },   /* ge21 */
    { SOC_BLK_GPORT,    1,   10,  0x16,    0xd3 },   /* ge22 */
    { SOC_BLK_GPORT,    1,   11,  0x17,    0xd3 },   /* ge23 */
    {            -1,   -1,   -1,   0x0,    0x00 }    /* Last */
};

static void
_fe2000_phy_addr_default(int unit, int port,
                         uint16 *phy_addr, uint16 *phy_addr_int)
{
    _block_phy_addr_t  *phy_info;
    int                block_type;
    int                block_number;
    int                block_index;

    *phy_addr = *phy_addr_int = 0x0;

    if (SOC_IS_SBX_FE2KXT(unit)) {
      phy_info = &_soc_phy_addr_bcm88025_a0[0];
    } else {
      phy_info = &_soc_phy_addr_bcm88020_a0[0];
    }

    /* Get physical block number and port offset */
    block_type   = SOC_PORT_BLOCK_TYPE(unit, port);
    block_number = SOC_PORT_BLOCK_NUMBER(unit, port);
    block_index  = SOC_PORT_BLOCK_INDEX(unit, port);

    while (phy_info->block_type != -1) {
        if ((phy_info->block_type   == block_type)   &&
            (phy_info->block_number == block_number) ) {
            if ((block_type == SOC_BLK_XPORT) ||
                (phy_info->block_index == block_index)) {
                /* Found */
                *phy_addr     = phy_info->addr;
                *phy_addr_int = phy_info->addr_int;
                break;
            }
        }

        phy_info++;
    }
}
#ifdef BCM_CALADAN3_SUPPORT

static void
_bcm88030_wc_phy_addr(int unit, int port, uint16 *phy_addr_int )
{
    int phyid = 0, rv = 0;
    
    static const uint16 _soc_bcm88030_int_phy_addr_wc[] = {
        0x80, /*    0 (cmic) N/A */
        0x81, /*    1 (WC0) IntBus=0 Addr=0x01 */
        0x81, /*    2 (WC0) IntBus=0 Addr=0x02 */
        0x81, /*    3 (WC0) IntBus=0 Addr=0x03 */
        0x81, /*    4 (WC0) IntBus=0 Addr=0x04 */
        0x81, /*    5 (QSGMII0_0) IntBus=0 Addr=0x05 */
        0x81, /*    6 (QSGMII0_1) IntBus=0 Addr=0x06 */
        0x81, /*    7 (QSGMII0_2) IntBus=0 Addr=0x07 */
        0x81, /*    8 (QSGMII0_3) IntBus=0 Addr=0x08 */
        0x81, /*    9 (QSGMII0_4) IntBus=0 Addr=0x09 */
        0x81, /*   10 (QSGMII0_5) IntBus=0 Addr=0x0A */
        0x81, /*   11 (QSGMII0_6) IntBus=0 Addr=0x0B */
        0x81, /*   12 (QSGMII0_7) IntBus=0 Addr=0x0C */
        0x81, /*   13 (QSGMII1_0) IntBus=0 Addr=0x0D */
        0x81, /*   14 (QSGMII1_1) IntBus=0 Addr=0x0E */
        0x81, /*   15 (QSGMII1_2) IntBus=0 Addr=0x0F */
        0x81, /*   16 (QSGMII1_3) IntBus=0 Addr=0x10 */
        0x81, /*   17 (QSGMII1_4) IntBus=0 Addr=0x11 */
        0x81, /*   18 (QSGMII1_5) IntBus=0 Addr=0x12 */
        0x81, /*   19 (QSGMII1_6) IntBus=0 Addr=0x13 */
        0x81, /*   20 (QSGMII1_7) IntBus=0 Addr=0x14 */
        0x95, /*   21 (WC3) IntBus=0 Addr=0x15 */
        0x95, /*   22 (WC3) IntBus=0 Addr=0x16 */
        0x95, /*   23 (WC3) IntBus=0 Addr=0x17 */
        0x95, /*   24 (WC3) IntBus=0 Addr=0x18 */
        0x99, /*   25 (WC4) IntBus=0 Addr=0x19 */
        0x99, /*   26 (WC4) IntBus=0 Addr=0x1A */
        0x99, /*   27 (WC4) IntBus=0 Addr=0x1B */
        0x99, /*   28 (WC4) IntBus=0 Addr=0x1C */

        0xA1, /*   29 (WC1) IntBus=1 Addr=0x01 */
        0xA1, /*   30 (WC1) IntBus=1 Addr=0x02 */
        0xA1, /*   31 (WC1) IntBus=1 Addr=0x03 */
        0xA1, /*   32 (WC1) IntBus=1 Addr=0x04 */
        0xA1, /*   33 (QSGMII2_0) IntBus=1 Addr=0x05 */
        0xA1, /*   34 (QSGMII2_1) IntBus=1 Addr=0x06 */
        0xA1, /*   35 (QSGMII2_2) IntBus=1 Addr=0x07 */
        0xA1, /*   36 (QSGMII2_3) IntBus=1 Addr=0x08 */
        0xA1, /*   37 (QSGMII2_4) IntBus=1 Addr=0x09 */
        0xA1, /*   38 (QSGMII2_5) IntBus=1 Addr=0x0A */
        0xA1, /*   39 (QSGMII2_6) IntBus=1 Addr=0x0B */
        0xA1, /*   40 (QSGMII2_7) IntBus=1 Addr=0x0C */
        0xA1, /*   41 (QSGMII3_0) IntBus=1 Addr=0x0D */
        0xA1, /*   42 (QSGMII3_1) IntBus=1 Addr=0x0E */
        0xA1, /*   43 (QSGMII3_2) IntBus=1 Addr=0x0F */
        0xA1, /*   44 (QSGMII3_3) IntBus=1 Addr=0x10 */
        0xA1, /*   45 (QSGMII3_4) IntBus=1 Addr=0x11 */
        0xA1, /*   46 (QSGMII3_5) IntBus=1 Addr=0x12 */
        0xA1, /*   47 (QSGMII3_6) IntBus=1 Addr=0x13 */
        0xA1, /*   48 (QSGMII3_7) IntBus=1 Addr=0x14 */
        0xB5, /*   49 (WC5) IntBus=1 Addr=0x15 */
        0xB5, /*   50 (WC5) IntBus=1 Addr=0x16 */
        0xB5, /*   51 (WC5) IntBus=1 Addr=0x17 */
        0xB5, /*   52 (WC5) IntBus=1 Addr=0x18 */

        0xC1, /*   53 (WC2) IntBus=2 Addr=0x01 */
        0xC1, /*   54 (WC2) IntBus=2 Addr=0x02 */
        0xC1, /*   55 (WC2) IntBus=2 Addr=0x03 */
        0xC1, /*   56 (WC2) IntBus=2 Addr=0x04 */
        0xC1, /*   57 (QSGMII4_0) IntBus=2 Addr=0x05 */
        0xC1, /*   58 (QSGMII4_1) IntBus=2 Addr=0x06 */
        0xC1, /*   59 (QSGMII4_2) IntBus=2 Addr=0x07 */
        0xC1, /*   60 (QSGMII4_3) IntBus=2 Addr=0x08 */
        0xC1, /*   61 (QSGMII4_4) IntBus=2 Addr=0x09 */
        0xC1, /*   62 (QSGMII4_5) IntBus=2 Addr=0x0A */
        0xC1, /*   63 (QSGMII4_6) IntBus=2 Addr=0x0B */
        0xC1, /*   64 (QSGMII4_7) IntBus=2 Addr=0x0C */
        0xC1, /*   65 (QSGMII5_0) IntBus=2 Addr=0x0D */
        0xC1, /*   66 (QSGMII5_1) IntBus=2 Addr=0x0E */
        0xC1, /*   67 (QSGMII5_2) IntBus=2 Addr=0x0F */
        0xC1, /*   68 (QSGMII5_3) IntBus=2 Addr=0x10 */
        0xC1, /*   69 (QSGMII5_4) IntBus=2 Addr=0x11 */
        0xC1, /*   70 (QSGMII5_5) IntBus=2 Addr=0x12 */
        0xC1, /*   71 (QSGMII5_6) IntBus=2 Addr=0x13 */
        0xC1, /*   72 (QSGMII5_7) IntBus=2 Addr=0x14 */
        0xD5, /*   73 (MLD0) IntBus=2 Addr=0x15 */
        0xD6, /*   74 (MLD1) IntBus=2 Addr=0x16 */
        0xD7, /*   75 (XC0) IntBus=2 Addr=0x17 */
        0xD7, /*   76 (XC0) IntBus=2 Addr=0x18 */
        0xD7, /*   77 (XC0) IntBus=2 Addr=0x17 */
        0xD7, /*   78 (XC0) IntBus=2 Addr=0x18 */
    };

    rv = soc_sbx_caladan3_port_to_phyid(unit, port, &phyid);
    if (SOC_FAILURE(rv)) {
        SOC_DEBUG_PRINT((DK_ERR, 
              "soc_sbx_caladan3_phy_addr_multi_get: failed to get address for port %d\n", 
              port));
        return ;
    }
    *phy_addr_int = _soc_bcm88030_int_phy_addr_wc[phyid];
}

static void
_bcm88030_phy_addr_default(int unit, int port,
                         uint16 *phy_addr, uint16 *phy_addr_int)
{
    static const uint16 _soc_bcm88030_int_phy_addr[] = {
        0x80, /*    0 (cmic) N/A */
        0x81, /*    1 (WC0) IntBus=0 Addr=0x01 */
        0x81, /*    2 (WC0) IntBus=0 Addr=0x02 */
        0x81, /*    3 (WC0) IntBus=0 Addr=0x03 */
        0x81, /*    4 (WC0) IntBus=0 Addr=0x04 */
        0x85, /*    5 (QSGMII0_0) IntBus=0 Addr=0x05 */
        0x85, /*    6 (QSGMII0_1) IntBus=0 Addr=0x06 */
        0x85, /*    7 (QSGMII0_2) IntBus=0 Addr=0x07 */
        0x85, /*    8 (QSGMII0_3) IntBus=0 Addr=0x08 */
        0x85, /*    9 (QSGMII0_4) IntBus=0 Addr=0x09 */
        0x85, /*   10 (QSGMII0_5) IntBus=0 Addr=0x0A */
        0x85, /*   11 (QSGMII0_6) IntBus=0 Addr=0x0B */
        0x85, /*   12 (QSGMII0_7) IntBus=0 Addr=0x0C */
        0x8D, /*   13 (QSGMII1_0) IntBus=0 Addr=0x0D */
        0x8D, /*   14 (QSGMII1_1) IntBus=0 Addr=0x0E */
        0x8D, /*   15 (QSGMII1_2) IntBus=0 Addr=0x0F */
        0x8D, /*   16 (QSGMII1_3) IntBus=0 Addr=0x10 */
        0x8D, /*   17 (QSGMII1_4) IntBus=0 Addr=0x11 */
        0x8D, /*   18 (QSGMII1_5) IntBus=0 Addr=0x12 */
        0x8D, /*   19 (QSGMII1_6) IntBus=0 Addr=0x13 */
        0x8D, /*   20 (QSGMII1_7) IntBus=0 Addr=0x14 */
        0x95, /*   21 (WC3) IntBus=0 Addr=0x15 */
        0x95, /*   22 (WC3) IntBus=0 Addr=0x16 */
        0x95, /*   23 (WC3) IntBus=0 Addr=0x17 */
        0x95, /*   24 (WC3) IntBus=0 Addr=0x18 */
        0x99, /*   25 (WC4) IntBus=0 Addr=0x19 */
        0x99, /*   26 (WC4) IntBus=0 Addr=0x1A */
        0x99, /*   27 (WC4) IntBus=0 Addr=0x1B */
        0x99, /*   28 (WC4) IntBus=0 Addr=0x1C */

        0xA1, /*   29 (WC1) IntBus=1 Addr=0x01 */
        0xA1, /*   30 (WC1) IntBus=1 Addr=0x02 */
        0xA1, /*   31 (WC1) IntBus=1 Addr=0x03 */
        0xA1, /*   32 (WC1) IntBus=1 Addr=0x04 */
        0xA5, /*   33 (QSGMII2_0) IntBus=1 Addr=0x05 */
        0xA5, /*   34 (QSGMII2_1) IntBus=1 Addr=0x06 */
        0xA5, /*   35 (QSGMII2_2) IntBus=1 Addr=0x07 */
        0xA5, /*   36 (QSGMII2_3) IntBus=1 Addr=0x08 */
        0xA5, /*   37 (QSGMII2_4) IntBus=1 Addr=0x09 */
        0xA5, /*   38 (QSGMII2_5) IntBus=1 Addr=0x0A */
        0xA5, /*   39 (QSGMII2_6) IntBus=1 Addr=0x0B */
        0xA5, /*   40 (QSGMII2_7) IntBus=1 Addr=0x0C */
        0xAD, /*   41 (QSGMII3_0) IntBus=1 Addr=0x0D */
        0xAD, /*   42 (QSGMII3_1) IntBus=1 Addr=0x0E */
        0xAD, /*   43 (QSGMII3_2) IntBus=1 Addr=0x0F */
        0xAD, /*   44 (QSGMII3_3) IntBus=1 Addr=0x10 */
        0xAD, /*   45 (QSGMII3_4) IntBus=1 Addr=0x11 */
        0xAD, /*   46 (QSGMII3_5) IntBus=1 Addr=0x12 */
        0xAD, /*   47 (QSGMII3_6) IntBus=1 Addr=0x13 */
        0xAD, /*   48 (QSGMII3_7) IntBus=1 Addr=0x14 */
        0xB5, /*   49 (WC5) IntBus=1 Addr=0x15 */
        0xB5, /*   50 (WC5) IntBus=1 Addr=0x16 */
        0xB5, /*   51 (WC5) IntBus=1 Addr=0x17 */
        0xB5, /*   52 (WC5) IntBus=1 Addr=0x18 */

        0xC1, /*   53 (WC2) IntBus=2 Addr=0x01 */
        0xC1, /*   54 (WC2) IntBus=2 Addr=0x02 */
        0xC1, /*   55 (WC2) IntBus=2 Addr=0x03 */
        0xC1, /*   56 (WC2) IntBus=2 Addr=0x04 */
        0xC5, /*   57 (QSGMII4_0) IntBus=2 Addr=0x05 */
        0xC5, /*   58 (QSGMII4_1) IntBus=2 Addr=0x06 */
        0xC5, /*   59 (QSGMII4_2) IntBus=2 Addr=0x07 */
        0xC5, /*   60 (QSGMII4_3) IntBus=2 Addr=0x08 */
        0xC5, /*   61 (QSGMII4_4) IntBus=2 Addr=0x09 */
        0xC5, /*   62 (QSGMII4_5) IntBus=2 Addr=0x0A */
        0xC5, /*   63 (QSGMII4_6) IntBus=2 Addr=0x0B */
        0xC5, /*   64 (QSGMII4_7) IntBus=2 Addr=0x0C */
        0xCD, /*   65 (QSGMII5_0) IntBus=2 Addr=0x0D */
        0xCD, /*   66 (QSGMII5_1) IntBus=2 Addr=0x0E */
        0xCD, /*   67 (QSGMII5_2) IntBus=2 Addr=0x0F */
        0xCD, /*   68 (QSGMII5_3) IntBus=2 Addr=0x10 */
        0xCD, /*   69 (QSGMII5_4) IntBus=2 Addr=0x11 */
        0xCD, /*   70 (QSGMII5_5) IntBus=2 Addr=0x12 */
        0xCD, /*   71 (QSGMII5_6) IntBus=2 Addr=0x13 */
        0xCD, /*   72 (QSGMII5_7) IntBus=2 Addr=0x14 */
        0xD5, /*   73 (MLD0) IntBus=2 Addr=0x15 */
        0xD6, /*   74 (MLD1) IntBus=2 Addr=0x16 */
        0xD7, /*   75 (XC0) IntBus=2 Addr=0x17 */
        0xD7, /*   76 (XC0) IntBus=2 Addr=0x18 */
        0xD7, /*   77 (XC0) IntBus=2 Addr=0x17 */
        0xD7, /*   78 (XC0) IntBus=2 Addr=0x18 */
    };
    static const uint16 _soc_bcm88030_phy_addr[] = {
        0x00, /* CMIC */
        0x00,  
        0x00,  
        0x00,  
        0x00,  
        0x04, /* ge0, external bus 0, addr 0x4 */
        0x05, /* ge1, external bus 0, addr 0x5 */
        0x06, /* ge2, external bus 0, addr 0x6 */
        0x07, /* ge3, external bus 0, addr 0x7 */
        0x08, /* ge4, external bus 0, addr 0x8 */
        0x09, /* ge5, external bus 0, addr 0x9 */
        0x0A, /* ge6, external bus 0, addr 0xA */
        0x0B, /* ge7, external bus 0, addr 0xB */
        0x0D, /* ge8, external bus 0, addr 0xD */
        0x0E, /* ge9, external bus 0, addr 0xE */
        0x0F, /* ge10, external bus 0, addr 0xF */
        0x10, /* ge11, external bus 0, addr 0x10 */
        0x11, /* ge12, external bus 0, addr 0x11 */
        0x12, /* ge13, external bus 0, addr 0x12 */
        0x13, /* ge14, external bus 0, addr 0x13 */
        0x14, /* ge15, external bus 0, addr 0x14 */
        0x00,  
        0x00,  
        0x00,  
        0x00,  
        0x00,  
        0x00,  
        0x00,  
        0x00,  
        0x00,  
        0x00,  
        0x00,  
        0x00,  
        0x16, /* ge16, external bus 0, addr 0x16 */
        0x17, /* ge17, external bus 0, addr 0x17 */
        0x18, /* ge18, external bus 0, addr 0x18 */
        0x19, /* ge19, external bus 0, addr 0x19 */
        0x1A, /* ge20, external bus 0, addr 0x1A */
        0x1B, /* ge21, external bus 0, addr 0x1B */
        0x1C, /* ge22, external bus 0, addr 0x1C */
        0x1D, /* ge23, external bus 0, addr 0x1D */

        0x24, /* ge24, external bus 1, addr 0x4 */
        0x25, /* ge25, external bus 1, addr 0x5 */
        0x26, /* ge26, external bus 1, addr 0x6 */
        0x27, /* ge27, external bus 1, addr 0x7 */
        0x28, /* ge28, external bus 1, addr 0x8 */
        0x29, /* ge29, external bus 1, addr 0x9 */
        0x2A, /* ge30, external bus 1, addr 0xA */
        0x2B, /* ge31, external bus 1, addr 0xB */

        0x0,  
        0x0,  
        0x0,  
        0x0, 
        0x0, 
        0x0, 
        0x0, 
        0x0, 
        0x2D, /* ge32, external bus 1, addr 0xD */
        0x2E, /* ge33, external bus 1, addr 0xE */
        0x2F, /* ge34, external bus 1, addr 0xF */
        0x30, /* ge35, external bus 1, addr 0x10 */
        0x31, /* ge36, external bus 1, addr 0x11 */
        0x32, /* ge37, external bus 1, addr 0x12 */
        0x33, /* ge38, external bus 1, addr 0x13 */
        0x34, /* ge39, external bus 1, addr 0x14 */
        0x36, /* ge40, external bus 1, addr 0x16 */
        0x37, /* ge41, external bus 1, addr 0x17 */
        0x38, /* ge42, external bus 1, addr 0x18 */
        0x39, /* ge43, external bus 1, addr 0x19 */
        0x3A, /* ge44, external bus 1, addr 0x1A */
        0x3B, /* ge45, external bus 1, addr 0x1B */
        0x3C, /* ge46, external bus 1, addr 0x1C */
        0x3D, /* ge47, external bus 1, addr 0x1D */
        0x0, 
        0x0, 
        0x0, 
        0x0, 
        0x0, 
        0x0, 
    };

    uint16 dev_id;
    uint8 rev_id;
    int phyid;
    int rv;

    soc_cm_get_id(unit, &dev_id, &rev_id);

    rv = soc_sbx_caladan3_port_to_phyid(unit, port, &phyid);
    if (SOC_FAILURE(rv)) {
        SOC_DEBUG_PRINT((DK_ERR, 
                "soc_sbx_caladan3_phy_addr_multi_get: failed to get address for port %d\n", port));
        return ;
    }

    *phy_addr_int = _soc_bcm88030_int_phy_addr[phyid];

    /* Variations ?? */
    switch (dev_id) {
    case BCM88030_DEVICE_ID:
        *phy_addr = _soc_bcm88030_phy_addr[phyid];
        break;
    default:
        *phy_addr = _soc_bcm88030_phy_addr[phyid];
        break;
    }
}
#endif

#ifdef BCM_SIRIUS_SUPPORT
/* Hypercores for hg port are on internal mdio bus only 
 *   Hg0-3 use Hc0-3
 *   Hg4-7 use Hc6-9
 *   SCI0/1 use Hc4, lane0/1
 *   SFI0/21 use Hc4, Lan2/3 to Hc9
 */
static _block_phy_addr_t   _soc_phy_addr_bcm88230[] = {
    { SOC_BLK_GXPORT,   0,    0,   0x0,    0x80 },  /* hg0  */
    { SOC_BLK_GXPORT,   1,    0,   0x0,    0x81 },  /* hg1  */
    { SOC_BLK_GXPORT,   2,    0,   0x0,    0x82 },  /* hg2  */
    { SOC_BLK_GXPORT,   3,    0,   0x0,    0x83 },  /* hg3  */
    { SOC_BLK_GXPORT,   4,    0,   0x0,    0x86 },  /* hg4  */
    { SOC_BLK_GXPORT,   5,    0,   0x0,    0x87 },  /* hg5  */
    { SOC_BLK_GXPORT,   6,    0,   0x0,    0x88 },  /* hg6  */
    { SOC_BLK_GXPORT,   7,    0,   0x0,    0x89 },  /* hg7  */
    { SOC_BLK_CMIC,     0,    0,   0x0,    0x00 },  /* cpu N/A */
    { SOC_BLK_SC_TOP,   0,    2,   0x0,    0x84 },  /* sfi0 */
    { SOC_BLK_SC_TOP,   0,    3,   0x0,    0x84 },  /* sfi1 */
    { SOC_BLK_SC_TOP,   0,    4,   0x0,    0x85 },  /* sfi2 */
    { SOC_BLK_SC_TOP,   0,    5,   0x0,    0x85 },  /* sfi3 */
    { SOC_BLK_SC_TOP,   0,    6,   0x0,    0x85 },  /* sfi4 */
    { SOC_BLK_SC_TOP,   0,    7,   0x0,    0x85 },  /* sfi5 */
    { SOC_BLK_SC_TOP,   0,    8,   0x0,    0x86 },  /* sfi6 */
    { SOC_BLK_SC_TOP,   0,    9,   0x0,    0x86 },  /* sfi7 */
    { SOC_BLK_SC_TOP,   0,   10,   0x0,    0x86 },  /* sfi8 */
    { SOC_BLK_SC_TOP,   0,   11,   0x0,    0x86 },  /* sfi9 */
    { SOC_BLK_SF_TOP,   0,    0,   0x0,    0x87 },  /* sfi10*/
    { SOC_BLK_SF_TOP,   0,    1,   0x0,    0x87 },  /* sfi11*/
    { SOC_BLK_SF_TOP,   0,    2,   0x0,    0x87 },  /* sfi12*/
    { SOC_BLK_SF_TOP,   0,    3,   0x0,    0x87 },  /* sfi13*/
    { SOC_BLK_SF_TOP,   0,    4,   0x0,    0x88 },  /* sfi14*/
    { SOC_BLK_SF_TOP,   0,    5,   0x0,    0x88 },  /* sfi15*/
    { SOC_BLK_SF_TOP,   0,    6,   0x0,    0x88 },  /* sfi16*/
    { SOC_BLK_SF_TOP,   0,    7,   0x0,    0x88 },  /* sfi17*/
    { SOC_BLK_SF_TOP,   0,    8,   0x0,    0x89 },  /* sfi18*/
    { SOC_BLK_SF_TOP,   0,    9,   0x0,    0x89 },  /* sfi19*/
    { SOC_BLK_SF_TOP,   0,   10,   0x0,    0x89 },  /* sfi20*/
    { SOC_BLK_SF_TOP,   0,   11,   0x0,    0x89 },  /* sfi21*/
    { SOC_BLK_SC_TOP,   0,    0,   0x0,    0x84 },  /* sci0 */
    { SOC_BLK_SC_TOP,   0,    1,   0x0,    0x84 },  /* sci1 */
    { SOC_BLK_EP,       0,    0,   0x0,    0x00 },  /* req0 N/A */
    { SOC_BLK_EP,       0,    1,   0x0,    0x00 },  /* req1 N/A */
    {             -1,   -1,    -1,   0x0,    0x0 }   /* Last */
};

static void
_sirius_phy_addr_default(int unit, int port,
                         uint16 *phy_addr, uint16 *phy_addr_int)
{
    _block_phy_addr_t  *phy_info;
    int                block_type;
    int                block_number;
    int                block_index;

    *phy_addr = *phy_addr_int = 0x0;

    phy_info = &_soc_phy_addr_bcm88230[0];

    /* Get physical block number and port offset */
    block_type   = SOC_PORT_BLOCK_TYPE(unit, port);
    block_number = SOC_PORT_BLOCK_NUMBER(unit, port);
    block_index  = SOC_PORT_BLOCK_INDEX(unit, port);

    soc_cm_debug(DK_PHY | DK_VERBOSE, "block_type %d, block_num %d, block_index %d\n",
		 block_type, block_number, block_index);

    while (phy_info->block_type != -1) {
        if ((phy_info->block_type   == block_type)   &&
            (phy_info->block_number == block_number) &&
            (phy_info->block_index  == block_index)) {
            /* Found */
            *phy_addr     = phy_info->addr;
            *phy_addr_int = phy_info->addr_int;
            break;
        }

        phy_info++;
    }
}
#endif /* BCM_SIRIUS_SUPPORT */

#endif /* BCM_SBX_SUPPORT */

#ifdef BCM_HAWKEYE_SUPPORT
static void
_hawkeye_phy_addr_default(int unit, int port,
                           uint16 *phy_addr, uint16 *phy_addr_int)
{
    static const uint16 _soc_phy_addr_bcm53314_a0[] = {
    0x80, 0x0, /* Port  0 (cmic) N/A */
    0x81, 0x0, /* Port  1 ( ge0) ExtBus=0 ExtAddr=0x01 */
    0x82, 0x0, /* Port  2 ( ge1) ExtBus=0 ExtAddr=0x02 */
    0x83, 0x0, /* Port  3 ( ge2) ExtBus=0 ExtAddr=0x03 */
    0x84, 0x0, /* Port  4 ( ge3) ExtBus=0 ExtAddr=0x04 */
    0x85, 0x0, /* Port  5 ( ge4) ExtBus=0 ExtAddr=0x05 */
    0x86, 0x0, /* Port  6 ( ge5) ExtBus=0 ExtAddr=0x06 */
    0x87, 0x0, /* Port  7 ( ge6) ExtBus=0 ExtAddr=0x07 */
    0x88, 0x0, /* Port  8 ( ge7) ExtBus=0 ExtAddr=0x08 */
    0x09, 0x89, /* Port  9 ( ge8) ExtBus=0 ExtAddr=0x09 IntBus=0 IntAddr=0x09 */
    0x0a, 0x89, /* Port 10 ( ge9) ExtBus=0 ExtAddr=0x0a IntBus=0 IntAddr=0x09 */
    0x0b, 0x89, /* Port 11 (ge10) ExtBus=0 ExtAddr=0x0b IntBus=0 IntAddr=0x09 */
    0x0c, 0x89, /* Port 12 (ge11) ExtBus=0 ExtAddr=0x0c IntBus=0 IntAddr=0x09*/
    0x0d, 0x89, /* Port 13 (ge12) ExtBus=0 ExtAddr=0x0d IntBus=0 IntAddr=0x09 */
    0x0e, 0x89, /* Port 14 (ge13) ExtBus=0 ExtAddr=0x0e IntBus=0 IntAddr=0x09 */
    0x0f, 0x89, /* Port 15 (ge14) ExtBus=0 ExtAddr=0x0f IntBus=0 IntAddr=0x09 */
    0x10, 0x89, /* Port 16 (ge15) ExtBus=0 ExtAddr=0x10 IntBus=0 IntAddr=0x09 */
    0x12, 0x91, /* Port 17 (ge16) ExtBus=0 ExtAddr=0x12 IntBus=0 IntAddr=0x11 */
    0x13, 0x91, /* Port 18 (ge17) ExtBus=0 ExtAddr=0x13 IntBus=0 IntAddr=0x11 */
    0x14, 0x91, /* Port 19 (ge18) ExtBus=0 ExtAddr=0x14 IntBus=0 IntAddr=0x11 */
    0x15, 0x91, /* Port 20 (ge19) ExtBus=0 ExtAddr=0x15 IntBus=0 IntAddr=0x11 */
    0x16, 0x91, /* Port 21 (ge20) ExtBus=0 ExtAddr=0x16 IntBus=0 IntAddr=0x11 */
    0x17, 0x91, /* Port 22 (ge21) ExtBus=0 ExtAddr=0x17 IntBus=0 IntAddr=0x11 */
    0x18, 0x91, /* Port 23 (ge22) ExtBus=0 ExtAddr=0x18 IntBus=0 IntAddr=0x11 */
    0x19, 0x91, /* Port 24 (ge23) ExtBus=0 ExtAddr=0x19 IntBus=0 IntAddr=0x11 */
    };

    uint16 dev_id;
    uint8 rev_id;

    soc_cm_get_id(unit, &dev_id, &rev_id);

    switch (dev_id) {
    case BCM53312_DEVICE_ID:
    case BCM53313_DEVICE_ID:
    case BCM53314_DEVICE_ID:
    case BCM53322_DEVICE_ID:
    case BCM53323_DEVICE_ID:
    case BCM53324_DEVICE_ID:
        *phy_addr = _soc_phy_addr_bcm53314_a0[port * 2];
        *phy_addr_int = _soc_phy_addr_bcm53314_a0[port * 2 + 1];
        break;
    }
}
#endif /* BCM_HAWKEYE_SUPPORT */

#ifdef BCM_KATANA_SUPPORT
STATIC void
_katana_phy_addr_default(int unit, int port,
                          uint16 *phy_addr, uint16 *phy_addr_int)
{
    static const uint16 _katana_phy_int_addr_mxqport[] = {
        0xa0, /* Port  25 Int XG Bus=1 IntAddr=0x80 */
        0xa1, /* Port  26 Int XG Bus=1 IntAddr=0x81 */
        0xa2, /* Port  27 Int XG Bus=1 IntAddr=0x82 */
        0xa6, /* Port  28 Int XG Bus=1 IntAddr=0x86 */
        0xa7, /* Port  29 Int XG Bus=1 IntAddr=0x87 */
        0xa8, /* Port  30 Int XG Bus=1 IntAddr=0x88 */
        0xa9, /* Port  31 Int XG Bus=1 IntAddr=0x89 */
        0xa3, /* Port  32 Int XG Bus=1 IntAddr=0x83 */
        0xa4, /* Port  33 Int XG Bus=1 IntAddr=0x84 */
        0xa5, /* Port  34 Int XG Bus=1 IntAddr=0x85 */
    };
    static const uint16 _katana_phy_ext_addr_mxqport[] = {
        0x2c, /* Port  25 Ext Bus=1 ExtAddr=0x0c */
        0x30, /* Port  26 Ext Bus=1 ExtAddr=0x10 */
        0x34, /* Port  27 Ext Bus=1 ExtAddr=0x14 */
        0x38, /* Port  28 Ext Bus=1 ExtAddr=0x18 */
        0x39, /* Port  29 Ext Bus=1 ExtAddr=0x19 */
        0x3a, /* Port  30 Ext Bus=1 ExtAddr=0x1a */
        0x3b, /* Port  31 Ext Bus=1 ExtAddr=0x1b */
        0x35, /* Port  32 Ext Bus=1 ExtAddr=0x15 */
        0x36, /* Port  33 Ext Bus=1 ExtAddr=0x16 */
        0x37, /* Port  34 Ext Bus=1 ExtAddr=0x17 */
    };

    uint16 dev_id;
    uint8 rev_id;

    soc_cm_get_id(unit, &dev_id, &rev_id);

    if(port < 25) {
        *phy_addr_int = (0x80 | port);
        *phy_addr = port;
    } else {
        *phy_addr_int = _katana_phy_int_addr_mxqport[port-25];
        switch (dev_id) {
            case BCM56440_DEVICE_ID:
            case BCM55440_DEVICE_ID:
            case BCM55441_DEVICE_ID:
            case BCM56445_DEVICE_ID:
            case BCM56448_DEVICE_ID:
            case BCM56449_DEVICE_ID:
                /* Valid ports 25-28 only */
                *phy_addr = (0x20 | port); /* Bus-1 */
                break;
            case BCM56441_DEVICE_ID:
            case BCM56446_DEVICE_ID:
            case BCM56443_DEVICE_ID:
            case BCM56240_DEVICE_ID:
            case BCM56241_DEVICE_ID:
            case BCM56242_DEVICE_ID:
            case BCM56243_DEVICE_ID:
                *phy_addr = _katana_phy_ext_addr_mxqport[port-25];
                break;
            case BCM56442_DEVICE_ID:
            case BCM56447_DEVICE_ID:
                /* Shouldn't be here.. 442 only has 16 ports */
                break;
            default:
                
                break;
        }
    }
}
#endif /* BCM_KATANA_SUPPORT */

#ifdef BCM_KATANA2_SUPPORT
void
_katana2_phy_addr_default(int unit, int port,
                          uint16 *phy_addr, uint16 *phy_addr_int)
{
   uint8 mdio_bus_ring=0;
   uint8 mdio_phy_addr=0;
   uint8 mxqblock=0;
   bcmMxqConnection_t connection_mode = bcmMqxConnectionUniCore;
#if 0
   /* Actual Addressing scheme but Looks like WARP Core Not happy with
      independent lane mode and works with AER scheme only*/
   /* Keeping this block for JUST REFERENCE */
   static int8 mapped_phy_addr[40+1]={0,
   /* Bus=0 PhySical Port versus Phy Addresses mapping */
   /* MXQ0=1..4(0x1..0x4)    MXQ1=5..8(0x5..0x8)    MXQ2=9..12(0x9..0xc)      */
   /* MXQ3=13..16(0xd..0x10) MXQ4=17..20(0x11..0x14) MXQ5=21..24(0x15..0x18)  */
                                     0x01, 0x02, 0x03,0x04, /* MXQ 0=1.. 4    */
                                     0x05, 0x06, 0x07,0x08, /* MXQ 1=5.. 8    */
                                     0x09, 0x0a, 0x0b,0x0c, /* MXQ 2=9.. 12   */
                                     0x0d, 0x0e, 0x0f,0x10, /* MXQ 3=13..16   */
                                     0x11, 0x12, 0x13,0x14, /* MXQ 4=17..20   */
                                     0x15, 0x16, 0x17,0x18, /* MXQ 5=21..24   */
  /* Bus=1 PhySical Port versus Phy Addresses mapping */
  /* MXQ6=25,35..37(0x1..0x4) MXQ7=26,38..40(0x5..0x8)                        */
  /* MXQ8=27,32..34(0x9..0xb) MXQ9=28,35..37(0xc..0x10)                       */
                                     0x01, 0x05, 0x09,0x0d, /* 25..28 */
                                     0x0e, 0x0f, 0x10,0x0a, /* 29..32 */
                                     0x0b, 0x0c, 0x02,0x03 ,/* 33..36 */
                                     0x04, 0x06, 0x07,0x08 ,/* 37..40 */
                                    };
#else
   static int8 mapped_phy_addr[40+1]={0,
   /* Bus=0 PhySical Port versus Phy Addresses mapping */
   /* MXQ0=1..4(0x1..0x4)    MXQ1=5..8(0x5..0x8)    MXQ2=9..12(0x9..0xc)      */
   /* MXQ3=13..16(0xd..0x10) MXQ4=17..20(0x11..0x14) MXQ5=21..24(0x15..0x18)  */
                                     0x01, 0x02, 0x03,0x04, /* MXQ 0=1.. 4    */
                                     0x05, 0x06, 0x07,0x08, /* MXQ 1=5.. 8    */
                                     0x09, 0x0a, 0x0b,0x0c, /* MXQ 2=9.. 12   */
                                     0x0d, 0x0e, 0x0f,0x10, /* MXQ 3=13..16   */
                                     0x11, 0x12, 0x13,0x14, /* MXQ 4=17..20   */
                                     0x15, 0x16, 0x17,0x18, /* MXQ 5=21..24   */
  /* Bus=1 PhySical Port versus Phy Addresses mapping */
  /* MXQ6=25,35..37(0x1..0x4) MXQ7=26,38..40(0x5..0x8)                        */
  /* MXQ8=27,32..34(0x9..0xb) MXQ9=28,35..37(0xc..0x10)                       */
                                     0x01, 0x05, /* 25..26 */
                                     0x09, /* WC0:27 */
                                     0x0d, /* WC1:28 */
                                     0x0d, 0x0d, 0x0d, /* WC1:29..31 */
                                     0x09, 0x09, 0x09, /* WC0:32..34 */
                                     0x02,0x03 ,/* 35..36 */
                                     0x04, 0x06, 0x07,0x08 ,/* 37..40 */
                                    };

#endif
   if ((port < 0 ) || (port >  (sizeof(mapped_phy_addr)-1))) {
        soc_cm_print("Internal Error: Invalid port=%d \n",port);
        return;
   }
   if (SOC_INFO(unit).olp_port && (port == KT2_OLP_PORT)) {
       mdio_bus_ring = 2;
       mdio_phy_addr = 3;
   } else { 
       if (soc_katana2_get_port_mxqblock( unit,port,&mxqblock) != SOC_E_NONE) {
           soc_cm_print("Unable to get mxqblock : Invalid port=%d \n",port);
           return;
       }
       if (soc_katana2_get_phy_connection_mode(
           unit, port,mxqblock,&connection_mode) != SOC_E_NONE) {
           soc_cm_print("Unable to get connection mode:Invalid port=%d\n",port);
           return;
       }
       if ((mxqblock == 6) &&
           (connection_mode == bcmMqxConnectionWarpCore)) {
           if (port == 25) {
               port = 32;
           }
           if (port == 36) {
               port = 34;
           }
       }
       if ((mxqblock == 7) &&
           (connection_mode == bcmMqxConnectionWarpCore)) {
           if (port == 26) {
               port = 29;
           }
           if (port == 39) {
               port = 31;
           }
       }
       mdio_bus_ring = (port - 1) / 24;
       mdio_phy_addr = mapped_phy_addr[port] ;
   }

   /* As per CMIC_CMCx_MIIM_PARAM */
   *phy_addr_int =    (1<<7)   | (mdio_bus_ring << 5) | mdio_phy_addr;
   *phy_addr     = /* (0<<7) */  (mdio_bus_ring << 5) | mdio_phy_addr;
}
#endif /* BCM_KATANA2_SUPPORT */


#ifdef BCM_ROBO_SUPPORT

static void
_robo_phy_addr_default(int unit, int port,
                         uint16 *phy_addr, uint16 *phy_addr_int)
{
    uint32 reg_val, temp;
    int rv;

    /* assigning the initial phy_addr */
    _ROBO_PHY_ADDR_DEFAULT(unit, port, phy_addr, phy_addr_int);

    /* Assign the external PHY address */
    reg_val = 0;
    temp = 0;
    if (port == 5) {
        if(SOC_IS_VULCAN(unit)) {
            rv = REG_READ_MDIO_ADDR_WANr(unit, &reg_val);
            if (rv != SOC_E_NONE) {
                return;
            }
            soc_MDIO_ADDR_WANr_field_get(unit, &reg_val, 
                ADDR_WANf, &temp);
            *phy_addr = temp;
        } else if (SOC_IS_LOTUS(unit) ||SOC_IS_STARFIGHTER(unit) || 
            SOC_IS_POLAR(unit)) {
            rv = REG_READ_MDIO_WAN_ADDRr(unit, &reg_val);
            if (rv != SOC_E_NONE) {
                return;
            }
            soc_MDIO_WAN_ADDRr_field_get(unit, &reg_val, 
                ADDR_WANf, &temp);
            *phy_addr = temp;
        } else if (SOC_IS_NORTHSTAR(unit)) {
            /* Northstar Port5 */
            if (port == 5) {
                /* Port 5 external PHY id default 0 */
                *phy_addr = 0;
                *phy_addr |= PHY_ADDR_ROBO_CPU_MDIOBUS;
            }
        }
    } else if ((port == 7) && (SOC_IS_BLACKBIRD2_BOND_V(unit))) {
        rv = REG_READ_MDIO_PORT7_ADDRr(unit, &reg_val);
        if (rv != SOC_E_NONE) {
            return;
        }
        soc_MDIO_PORT7_ADDRr_field_get(unit, &reg_val, 
            ADDR_PORT7f, &temp);
        *phy_addr = temp;
    }
	
    /* Specific assinging section for those ROBO devices within built-in 
     * SerDes design. (like, bcm5396/5348/5347/53115/polar)
     *  - For Robo device, here we assign a specifal flag at the highest bit
     *      to indicate that there is a internal SerDes built-in on this port.
     *      And the physical phy_addr on ext/int site will be retrived again 
     *      when doing miim_read/write.
     * 
     * Note : 
     *  1. The usage for internal phy_addr flag causes the max available 
     *      phy_addr been reduced from 255 to 127. In case a ROBO device might 
     *      consist of more than 127 ports on a single unit than this design 
     *      for internal phy_add flag have to change.
     *  2. The phy_addr is a logical phy_addr reflect to the PHY-ID of a port 
     *      on a device.
     */
    if (IS_ROBO_SPECIFIC_INT_SERDES(unit, port)){
        *phy_addr_int = (*phy_addr | PHY_ADDR_ROBO_INT_SERDES); 
    }
}

#ifdef BCM_NORTHSTARPLUS_SUPPORT


#define NSP_CORE_APB_ADDR               (0x3F000) /* offset from 0x18000000 */
#define NSP_P5_MUX_CONFIG               (0x308)
#define NSP_MUX_CONFIG_MODE_MASK        (0x7)
#define NSP_P5_MUX_CONFIG_MODE_SGMII    (0)
#define NSP_P5_MUX_CONFIG_MODE_MII      (1)
#define NSP_P5_MUX_CONFIG_MODE_RGMII    (2)
#define NSP_P5_MUX_CONFIG_MODE_GMII     (3)
#define NSP_P5_MUX_CONFIG_MODE_GPHY3    (4)
#define NSP_P5_MUX_CONFIG_MODE_INT_MAC  (5)
#define NSP_P5_MUX_CONFIG_MODE_DEFAULT  (5)

#define NSP_P4_MUX_CONFIG               (0x30C)
#define NSP_P4_MUX_CONFIG_MODE_SHIFT    (0)
#define NSP_P4_MUX_CONFIG_MODE_SGMII    (0)
#define NSP_P4_MUX_CONFIG_MODE_MII      (1)
#define NSP_P4_MUX_CONFIG_MODE_RGMII    (2)
#define NSP_P4_MUX_CONFIG_MODE_GPHY4    (4)

static void
_nsp_phy_addr_default(int unit, int port,
                         uint16 *phy_addr, uint16 *phy_addr_int)
{
    uint32 reg_val, temp = 0;
    int rv;
    int     phy_addr_base;
    uint32  mux_reg_addr, mux_reg_val, sys_if; 

    phy_addr_base = 6;
    if (port < 6) {
        *phy_addr_int = port + phy_addr_base;
        *phy_addr = port + phy_addr_base;
    }

    /* Reset the P5 MUX to default internal MAC */
    if (port == 4) {
        /* 
         * Since if the port 5 is configured as internal MAC, 
         * this port will be removed from valid_pbmp.
         * It will have no chance t modfiy the register of Port 5 MUX.
         * Reset the default value of port 5 mux register when process port 4.
         */
        mux_reg_addr = NSP_CORE_APB_ADDR + NSP_P5_MUX_CONFIG;
        mux_reg_val = CMREAD(soc_eth_unit, mux_reg_addr);
        mux_reg_val &= ~(NSP_MUX_CONFIG_MODE_MASK);
        mux_reg_val |= NSP_P5_MUX_CONFIG_MODE_DEFAULT;
        CMWRITE(soc_eth_unit, mux_reg_addr, mux_reg_val);
    }
    


    /* Check the P5  and P4 Mux config value */
    if ((port == 5) || (port == 4)) {
        /* mux register address */
        if (port == 5) {
            /* port 5 */
            mux_reg_addr = NSP_CORE_APB_ADDR + NSP_P5_MUX_CONFIG;
        } else {
            /* port 4 */
            mux_reg_addr = NSP_CORE_APB_ADDR + NSP_P4_MUX_CONFIG;
        }
        
        mux_reg_val = CMREAD(soc_eth_unit, mux_reg_addr);
        
        /* Check if port 4/5 mux is specified */
        sys_if = soc_property_port_get(unit, port, spn_PHY_SYS_INTERFACE, 0);
        if (sys_if) {
            if (port == 5) {
                switch (sys_if) {
                    case SOC_PORT_IF_SGMII: /* 4 */
                        mux_reg_val &= ~(NSP_MUX_CONFIG_MODE_MASK);
                        mux_reg_val |= NSP_P5_MUX_CONFIG_MODE_SGMII;
                        break;
                    case SOC_PORT_IF_RGMII: /* 7 */
                        mux_reg_val &= ~(NSP_MUX_CONFIG_MODE_MASK);
                        mux_reg_val |= NSP_P5_MUX_CONFIG_MODE_RGMII;
                        break;
                    case SOC_PORT_IF_GMII: /* 3 */
                        mux_reg_val &= ~(NSP_MUX_CONFIG_MODE_MASK);
                        mux_reg_val |= NSP_P5_MUX_CONFIG_MODE_GMII;
                        break;
                    case SOC_PORT_IF_MII: /* 2 */
                        mux_reg_val &= ~(NSP_MUX_CONFIG_MODE_MASK);
                        mux_reg_val |= NSP_P5_MUX_CONFIG_MODE_MII;
                        break;
                    case SOC_PORT_IF_CPU: /* 0x21 */
                        mux_reg_val &= ~(NSP_MUX_CONFIG_MODE_MASK);
                        mux_reg_val |= NSP_P5_MUX_CONFIG_MODE_INT_MAC;
                        break;
                    case SOC_PORT_IF_NULL: /* 1 */
                        mux_reg_val &= ~(NSP_MUX_CONFIG_MODE_MASK);
                        mux_reg_val |= NSP_P5_MUX_CONFIG_MODE_GPHY3;
                        break;
                    default:
                        break;
                }
            } else if (port == 4) {
                switch (sys_if) {
                    case SOC_PORT_IF_SGMII: /* 4 */
                        mux_reg_val &= ~(NSP_MUX_CONFIG_MODE_MASK);
                        mux_reg_val |= NSP_P4_MUX_CONFIG_MODE_SGMII;
                        break;
                    case SOC_PORT_IF_RGMII: /* 7 */
                        mux_reg_val &= ~(NSP_MUX_CONFIG_MODE_MASK);
                        mux_reg_val |= NSP_P4_MUX_CONFIG_MODE_RGMII;
                        break;
                    case SOC_PORT_IF_MII: /* 2 */
                        mux_reg_val &= ~(NSP_MUX_CONFIG_MODE_MASK);
                        mux_reg_val |= NSP_P4_MUX_CONFIG_MODE_MII;
                        break;
                    case SOC_PORT_IF_NULL: /* 1 */
                        mux_reg_val &= ~(NSP_MUX_CONFIG_MODE_MASK);
                        mux_reg_val |= NSP_P4_MUX_CONFIG_MODE_GPHY4;
                        break;
                    default:
                        break;
                }
            }
            CMWRITE(soc_eth_unit, mux_reg_addr, mux_reg_val);
        }
        
        mux_reg_val &= NSP_MUX_CONFIG_MODE_MASK;

        /* Determine the default phy address */
        if (port == 5) {
            switch (mux_reg_val) {
                case NSP_P5_MUX_CONFIG_MODE_SGMII:
                    *phy_addr = 12;
                    *phy_addr_int = 12 | PHY_ADDR_ROBO_INT_SERDES;
                    SOC_PBMP_PORT_REMOVE(SOC_INFO(unit).gmii_pbm, port);
                    break;
                case NSP_P5_MUX_CONFIG_MODE_RGMII:
                case NSP_P5_MUX_CONFIG_MODE_GMII:
                case NSP_P5_MUX_CONFIG_MODE_MII:
                    rv = REG_READ_MDIO_WAN_ADDRr(unit, &reg_val);
                    if (rv != SOC_E_NONE) {
                        break;
                    }
                    soc_MDIO_WAN_ADDRr_field_get(unit, &reg_val, 
                        ADDR_WANf, &temp);
                    *phy_addr = temp | PHY_ADDR_ROBO_CPU_MDIOBUS;
                    *phy_addr_int = port + phy_addr_base;
                    break;
                case NSP_P5_MUX_CONFIG_MODE_GPHY3:
                    *phy_addr = phy_addr_base + 3;
                    *phy_addr_int = 0;
                    break;
                case NSP_P5_MUX_CONFIG_MODE_INT_MAC:
                    /* means no MACSEC attached at Port 5 */
                    rv = DRV_DEV_PROP_GET(unit, 
                            DRV_DEV_PROP_SWITCHMACSEC_ATTACH_PBMP, &temp);
                    if (rv != SOC_E_NONE) {
                        break;
                    }
                    /* remove it from bmp */
                    temp &= ~(0x1 << port);
                    rv = DRV_DEV_PROP_SET(unit, 
                            DRV_DEV_PROP_SWITCHMACSEC_ATTACH_PBMP, temp);
                    if (rv != SOC_E_NONE) {
                        break;
                    }
                    break;
                default:
                    break;
            }
        } else if (port == 4) {
            switch (mux_reg_val) {
                case NSP_P4_MUX_CONFIG_MODE_SGMII:
                    *phy_addr = 13;
                    *phy_addr_int = 13 | PHY_ADDR_ROBO_INT_SERDES;
                    SOC_PBMP_PORT_REMOVE(SOC_INFO(unit).gmii_pbm, port);
                    break;
                case NSP_P4_MUX_CONFIG_MODE_RGMII:
                case NSP_P4_MUX_CONFIG_MODE_MII:
                    rv = REG_READ_MDIO_PORT4_ADDRr(unit, &reg_val);
                    if (rv != SOC_E_NONE) {
                        break;
                    }
                    soc_MDIO_PORT4_ADDRr_field_get(unit, &reg_val, 
                        ADDR_PORT4f, &temp);
                    *phy_addr = temp | PHY_ADDR_ROBO_CPU_MDIOBUS;
                    *phy_addr_int = port + phy_addr_base;
                    break;
                case NSP_P4_MUX_CONFIG_MODE_GPHY4:
                    break;
                default:
                    break;
            }
        }
    }

    
}
#endif /* BCM_NORTHSTARPLUS_SUPPORT */

#endif /* BCM_ROBO_SUPPORT */

/*
 * Function:
 *      _soc_phy_addr_default
 * Purpose:
 *      Return the default PHY addresses used to initialize the PHY map
 *      for a port.
 * Parameters:
 *      unit - StrataSwitch unit number
 *      phy_addr - (OUT) Outer PHY address
 *      phy_addr_int - (OUT) Intermediate PHY address, 0xff if none
 */

static void
_soc_phy_addr_default(int unit, int port,
                      uint16 *phy_addr, uint16 *phy_addr_int)
{

#ifdef BCM_XGS12_FABRIC_SUPPORT
    if (SOC_IS_XGS12_FABRIC(unit)) {
        _XGS12_FABRIC_PHY_ADDR_DEFAULT(unit, port, phy_addr, phy_addr_int);
    } else
#endif /* BCM_XGS12_FABRIC_SUPPORT */

#ifdef BCM_RAPTOR_SUPPORT
    if (SOC_IS_RAPTOR(unit) || SOC_IS_RAVEN(unit)) {
        _RAPTOR_PHY_ADDR_DEFAULT(unit, port, phy_addr, phy_addr_int);
    } else
#endif /* BCM_RAPTOR_SUPPORT */

#if defined(BCM_ENDURO_SUPPORT)
    if (SOC_IS_ENDURO(unit)) {
        _enduro_phy_addr_default(unit, port, phy_addr, phy_addr_int);
    } else
#endif /* BCM_ENDURO_SUPPORT */

#if defined(BCM_HURRICANE_SUPPORT)
    if (SOC_IS_HURRICANE(unit)) {
        _hurricane_phy_addr_default(unit, port, phy_addr, phy_addr_int);
    } else
#endif /* BCM_HURRICANE_SUPPORT */

#if defined(BCM_HURRICANE2_SUPPORT)
    if (SOC_IS_HURRICANE2(unit)) {
        _hurricane2_phy_addr_default(unit, port, phy_addr, phy_addr_int);
    } else
#endif /* BCM_HURRICANE2_SUPPORT */

#if defined(BCM_HELIX4_SUPPORT) 
    if (SOC_IS_HELIX4(unit)) {
        _helix4_phy_addr_default(unit, port, phy_addr, phy_addr_int);
    } else
#endif /* BCM_HELIX4_SUPPORT */

#if defined(BCM_TRIUMPH3_SUPPORT) 
    if (SOC_IS_TRIUMPH3(unit)) {
        _triumph3_phy_addr_default(unit, port, phy_addr, phy_addr_int);
    } else
#endif /* BCM_TRIUMPH3_SUPPORT */

#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_APOLLO_SUPPORT) || \
    defined(BCM_VALKYRIE2_SUPPORT)
    if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) || 
        SOC_IS_VALKYRIE2(unit)) {
        _triumph2_phy_addr_default(unit, port, phy_addr, phy_addr_int);
    } else
#endif /* BCM_TRIUMPH2_SUPPORT */

#ifdef BCM_TRIUMPH_SUPPORT
    if (SOC_IS_TRIUMPH(unit)) {
        _triumph_phy_addr_default(unit, port, phy_addr, phy_addr_int);
    } else
#endif /* BCM_TRIUMPH_SUPPORT */

#ifdef BCM_VALKYRIE_SUPPORT
    if (SOC_IS_VALKYRIE(unit)) {
        _valkyrie_phy_addr_default(unit, port, phy_addr, phy_addr_int);
    } else
#endif /* BCM_VALKYRIE_SUPPORT */

#ifdef BCM_DFE_SUPPORT
    if (SOC_IS_DFE(unit)) {
        _dfe_phy_addr_default(unit, port, phy_addr, phy_addr_int);
    } else
#endif /* BCM_DFE_SUPPORT */

#ifdef BCM_PETRA_SUPPORT
    if (SOC_IS_DPP(unit)) {
        _dpp_phy_addr_default(unit, port, phy_addr, phy_addr_int);
    } else
#endif /* BCM_PETRA_SUPPORT */

#ifdef BCM_TRIDENT_SUPPORT
#ifdef BCM_TRIDENT2_SUPPORT
    if (SOC_IS_TD2_TT2(unit)) {
        _trident2_phy_addr_default(unit, port, phy_addr, phy_addr_int);
    } else
#endif /* BCM_TRIDENT2_SUPPORT */
    if (SOC_IS_TD_TT(unit)) {
        _trident_phy_addr_default(unit, port, phy_addr, phy_addr_int);
    } else
#endif /* BCM_TRIDENT_SUPPORT */

#ifdef BCM_SHADOW_SUPPORT
    if (SOC_IS_SHADOW(unit)) {
        _shadow_phy_addr_default(unit, port, phy_addr, phy_addr_int);
    } else
#endif /* BCM_SHADOW_SUPPORT */

#ifdef BCM_SCORPION_SUPPORT
    if (SOC_IS_SCORPION(unit) &&
        _scorpion_phy_addr_default(unit, port, phy_addr, phy_addr_int)) {
    } else
#endif /* BCM_SCORPION_SUPPORT */

#ifdef BCM_HAWKEYE_SUPPORT
    if (SOC_IS_HAWKEYE(unit)) {
        _hawkeye_phy_addr_default(unit, port, phy_addr, phy_addr_int);
    } else
#endif /* BCM_HAWKEYE_SUPPORT */

#ifdef BCM_KATANA_SUPPORT
    if (SOC_IS_KATANA(unit)) {
        _katana_phy_addr_default(unit, port, phy_addr, phy_addr_int);
    } else
#endif /* BCM_KATANA_SUPPORT */

#ifdef BCM_KATANA2_SUPPORT
    if (SOC_IS_KATANA2(unit)) {
        _katana2_phy_addr_default(unit, port, phy_addr, phy_addr_int);
    } else
#endif /* BCM_KATANA2_SUPPORT */

#ifdef BCM_XGS3_SWITCH_SUPPORT
    if (SOC_IS_XGS3_SWITCH(unit)) {
        _xgs3_phy_addr_default(unit, port, phy_addr, phy_addr_int);
    } else
#endif /* BCM_XGS3_SWITCH_SUPPORT */

#ifdef BCM_ROBO_SUPPORT
#ifdef BCM_NORTHSTARPLUS_SUPPORT
    if (SOC_IS_NORTHSTARPLUS(unit)) {
        _nsp_phy_addr_default(unit, port, phy_addr, phy_addr_int);
    } else
#endif /* BCM_NORTHSTARPLUS_SUPPORT */
    if (SOC_IS_ROBO(unit)) {
        _robo_phy_addr_default(unit, port, phy_addr, phy_addr_int);
    } else
#endif /* BCM_ROBO_SUPPORT */

#ifdef BCM_SBX_SUPPORT
    if (SOC_IS_SBX_FE2000(unit) ||
	SOC_IS_SBX_FE2KXT(unit) ) {
        _fe2000_phy_addr_default(unit, port, phy_addr, phy_addr_int);
    } else 
#endif /* BCM_FE2000_SUPPORT */

#ifdef BCM_CALADAN3_SUPPORT
    if (SOC_IS_CALADAN3(unit)) {
        _bcm88030_phy_addr_default(unit, port, phy_addr, phy_addr_int);
    } else 
#endif /* BCM_CALADAN3_SUPPORT */

#ifdef BCM_BM9600_SUPPORT
    if (SOC_IS_SBX_BM9600(unit)) {
        _bm9600_phy_addr_default(unit, port, phy_addr, phy_addr_int);
    } else
#endif /* BCM_BM9600_SUPPORT */

#ifdef BCM_SIRIUS_SUPPORT
    if (SOC_IS_SIRIUS(unit)) {
        _sirius_phy_addr_default(unit, port, phy_addr, phy_addr_int);
    } else
#endif /* BCM_SIRIUS_SUPPORT */

    {
        *phy_addr     = port + 1;
        *phy_addr_int = 0xff;
    }

    /*
     * Override the calculated address(es) with the per-port properties
     */
    *phy_addr = soc_property_port_get(unit, port,
                                      spn_PORT_PHY_ADDR,
                                      *phy_addr);
}

int
soc_phy_deinit(int unit)
{
    sal_free(port_phy_addr[unit]);
    port_phy_addr[unit] = NULL;
    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_phy_init
 * Purpose:
 *      Initialize PHY software subsystem.
 * Parameters:
 *      unit - StrataSwitch unit number
 * Returns:
 *      SOC_E_XXX
 */
int
soc_phy_init(int unit)
{
    uint16              phy_addr=0, phy_addr_int=0;
    soc_port_t          port;

    if (_phys_in_table < 0) {
        _init_phy_table();
    }

    if (port_phy_addr[unit] == NULL) {
        port_phy_addr[unit] = sal_alloc(sizeof(port_phy_addr_t) *
                                        SOC_MAX_NUM_PORTS,
                                        "port_phy_addr");
        if (port_phy_addr[unit] == NULL) {
            return SOC_E_MEMORY;
        }
    }

    sal_memset(port_phy_addr[unit], 0,
               sizeof(port_phy_addr_t) * SOC_MAX_NUM_PORTS);

    PBMP_PORT_ITER(unit, port) {
        _soc_phy_addr_default(unit, port, &phy_addr, &phy_addr_int);

        SOC_IF_ERROR_RETURN
            (soc_phy_cfg_addr_set(unit,port,0, phy_addr));
        SOC_IF_ERROR_RETURN
            (soc_phy_cfg_addr_set(unit,port,SOC_PHY_INTERNAL, phy_addr_int));

        PHY_ADDR(unit, port)     = phy_addr;
        PHY_ADDR_INT(unit, port) = phy_addr_int;
    }

    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_phy_port_init
 * Purpose:
 *      Initialize PHY software for a single port.
 * Parameters:
 *      unit - unit number
 *      port - port number
 * Returns:
 *      SOC_E_XXX
 */
int
soc_phy_port_init(int unit, soc_port_t port)
{
    uint16              phy_addr=0, phy_addr_int=0;

    _soc_phy_addr_default(unit, port, &phy_addr, &phy_addr_int);

    SOC_IF_ERROR_RETURN
        (soc_phy_cfg_addr_set(unit,port,0, phy_addr));
    SOC_IF_ERROR_RETURN
        (soc_phy_cfg_addr_set(unit,port,SOC_PHY_INTERNAL, phy_addr_int));

    PHY_ADDR(unit, port)     = phy_addr;
    PHY_ADDR_INT(unit, port) = phy_addr_int;
    

    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_phy_add_entry
 * Purpose:
 *      Add an entry to the PHY table
 * Parameters:
 *      entry - pointer to the entry
 * Returns:
 *      SOC_E_NONE - no error
 *      SOC_E_INIT - not initialized
 *      SOC_E_MEMORY - not no more space in table.
 */

int
soc_phy_add_entry(soc_phy_table_t *entry)
{
    assert(_phys_in_table >= 0);        /* Fatal if not already inited */

    if (_phys_in_table >= _MAX_PHYS) {
        return SOC_E_MEMORY;
    }

    phy_table[_phys_in_table++] = entry;

    return SOC_E_NONE;
}

#if defined(INCLUDE_SERDES_COMBO65)
/*
 * Function:
 *      _chk_serdescombo65
 * Purpose:
 *      Check function for Raven Combo SerDes PHYs
 * Parameters:
 *      See soc_phy_ident_f
 *      *pi is an output parameter filled with PHY info.
 * Returns:
 *      TRUE if this PHY matches.
 *      FALSE otherwise.
 * Notes:
 *      Sets the phy_info structure for this port.
 */

static int
_chk_serdescombo65(int unit, soc_port_t port, soc_phy_table_t *my_entry,
         uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi)
{
    COMPILER_REFERENCE(unit);

    if (SOC_IS_RAVEN(unit)) {
        if (port == 1 || port == 2 || port == 4 || port == 5) {
            pi->phy_name = my_entry->phy_name;
            return TRUE;
        }
    }

#ifdef BCM_ROBO_SUPPORT
    if (SOC_IS_ROBO(unit)) {
        if(SOC_IS_TB(unit) && IS_S_PORT(unit, port)){
            pi->phy_name = my_entry->phy_name;
            return TRUE;
        }
    }
#endif /* BCM_ROBO_SUPPORT */

    return FALSE;
}

#endif /* INCLUDE_SERDES_COMBO65 */

#if defined(INCLUDE_XGXS_16G)
/*
 * Function:
 *      _chk_xgxs16g1l
 * Purpose:
 *      Standard check function for PHYs (see soc_phy_ident_f)
 * Parameters:
 *      See soc_phy_ident_f
 *      *pi is an output parameter filled with PHY info.
 * Returns:
 *      TRUE if this PHY matches.
 *      FALSE otherwise.
 * Notes:
 *      Sets the phy_info structure for this port.
 */

static int
_chk_xgxs16g1l(int unit, soc_port_t port, soc_phy_table_t *my_entry,
         uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi)
{
    if (my_entry->myNum == _phy_ident_type_get(phy_id0, phy_id1)) {
#ifdef BCM_KATANA_SUPPORT
         if (SOC_IS_KATANAX(unit) && IS_GE_PORT(unit,port)) {
             uint32 rval;
             int port_mode = 2;

             if (IS_MXQ_PORT(unit,port)) {
                 SOC_IF_ERROR_RETURN(READ_XPORT_MODE_REGr(unit, port, &rval));
                 port_mode = soc_reg_field_get(unit, XPORT_MODE_REGr, rval, PHY_PORT_MODEf);
             }
             if (port_mode) {
                 return TRUE;
             } else {
                 return FALSE;
             }
         } else
#endif /* BCM_KATANA_SUPPORT */

#ifdef BCM_TRIUMPH3_SUPPORT
        if (SOC_IS_TRIUMPH3(unit) && IS_GE_PORT(unit, port)) {
            pi->phy_name = my_entry->phy_name;
            return TRUE;
        }
#endif

        if (SOC_IS_SCORPION(unit) && !IS_GX_PORT(unit, port)) {
            pi->phy_name = my_entry->phy_name;
            return TRUE;
        }

#ifdef BCM_SBX_SUPPORT
	if (SOC_IS_SBX_FE2KXT(unit) && IS_GE_PORT(unit,port)) {
            pi->phy_name = my_entry->phy_name;
            return TRUE;	  
	}
#endif /* BCM_SBX_SUPPORT */

#ifdef BCM_ENDURO_SUPPORT
	if (SOC_IS_ENDURO(unit) && (port > 25) && IS_GE_PORT(unit,port)) {
            pi->phy_name = my_entry->phy_name;
            return TRUE;	  
	}
#endif /* BCM_ENDURO_SUPPORT */

#ifdef BCM_NORTHSTARPLUS_SUPPORT
        if (SOC_IS_NORTHSTARPLUS(unit) && IS_GE_PORT(unit,port)) {
                pi->phy_name = my_entry->phy_name;
                return TRUE;      
        }
#endif /* BCM_NORTHSTARPLUS_SUPPORT */


    }
    return FALSE;
}
#endif /* INCLUDE_XGXS_16G */

#if defined(INCLUDE_XGXS_WCMOD)
/*
 * Function:
 *      _chk_wcmod
 * Purpose:
 *      Standard check function for PHYs (see soc_phy_ident_f)
 * Parameters:
 *      See soc_phy_ident_f
 *      *pi is an output parameter filled with PHY info.
 * Returns:
 *      TRUE if this PHY matches.
 *      FALSE otherwise.
 * Notes:
 *      Sets the phy_info structure for this port.
 */

static int
_chk_wcmod(int unit, soc_port_t port, soc_phy_table_t *my_entry,
           uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi)
{
    if (SOC_IS_TRIUMPH3(unit)) {
        if (IS_GE_PORT(unit, port) &&
            !soc_property_port_get(unit, port, "phy_wcmod", 1)) {
            /* Allow debug fallback to legacy XGXS16g1l PHY driver */
            return FALSE;
        }
    }

    return _chk_phy(unit, port, my_entry, phy_id0, phy_id1, pi);
}
#endif  /* INCLUDE_XGXS_WCMOD */

/*
 * Function:
 *      _chk_phy
 * Purpose:
 *      Standard check function for PHYs (see soc_phy_ident_f)
 * Parameters:
 *      See soc_phy_ident_f
 *      *pi is an output parameter filled with PHY info.
 * Returns:
 *      TRUE if this PHY matches.
 *      FALSE otherwise.
 * Notes:
 *      Sets the phy_info structure for this port.
 */

static int
_chk_phy(int unit, soc_port_t port, soc_phy_table_t *my_entry,
         uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi)
{
    COMPILER_REFERENCE(unit);

    if (my_entry->myNum == _phy_ident_type_get(phy_id0, phy_id1)) {
        pi->phy_name = my_entry->phy_name;
        return TRUE;
    }

    return FALSE;
}

#if defined(INCLUDE_PHY_SIMUL)
#if defined(SIM_ALL_PHYS)
#define USE_SIMULATION_PHY(unit, port)  (TRUE)
#else
#define USE_SIMULATION_PHY(unit, port) \
     (soc_property_port_get(unit, port, spn_PHY_SIMUL, 0))
#endif
#else
#define USE_SIMULATION_PHY(unit, port)  (FALSE)
#endif

/*
 * Function:
 *      _chk_null
 * Purpose:
 *      Check function for NULL phys.
 * Returns:
 *      True if this phy matches.
 *      False otherwise.
 */

static int
_chk_null(int unit, soc_port_t port,  soc_phy_table_t *my_entry,
          uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi)
{
#if defined(INCLUDE_CES)
    uint16              dev_id;
    uint8               rev_id;
#endif

    if ((SAL_BOOT_PLISIM && (!SAL_BOOT_RTLSIM &&
        !USE_SIMULATION_PHY(unit, port))) ||
        !soc_property_get(unit, spn_PHY_ENABLE, 1) ||
        soc_property_port_get(unit, port, spn_PHY_NULL, 0) ||
        (SOC_IS_TRIUMPH3(unit) &&
         SOC_PBMP_MEMBER(SOC_PORT_DISABLED_BITMAP(unit, all), port))) {
        pi->phy_name = my_entry->phy_name;

        return TRUE;
    }

#if defined(INCLUDE_CES)
/*
 * For CES on the 56441/2/3 where the CES mii port 
 * is internal and there is no physical phy set the
 * phy device to the null phy.
 */
    if (soc_feature(unit, soc_feature_ces) && port == 1) {
	soc_cm_get_id(unit, &dev_id, &rev_id);

	if (dev_id == BCM56441_DEVICE_ID ||
	    dev_id == BCM56442_DEVICE_ID ||
	    dev_id == BCM56443_DEVICE_ID) {
	    pi->phy_name = my_entry->phy_name;
	    return TRUE;
	}
    }
#endif

#if defined(INCLUDE_RCPU)
    if (SOC_IS_RCPU_ONLY(unit)) {
        pi->phy_name = my_entry->phy_name;
        return TRUE;
    }
#endif /* INCLUDE_RCPU */

    return FALSE;
}

#if defined(BCM_XGS3_SWITCH_SUPPORT) || defined(BCM_ROBO_SUPPORT)
/*
 * Function:
 *      _chk_gmii
 * Purpose:
 *      Check function for GMII port.
 * Returns:
 *      True if this phy matches.
 *      False otherwise.
 */

static int
_chk_gmii(int unit, soc_port_t port,  soc_phy_table_t *my_entry,
          uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi)
{
    if (IS_GMII_PORT(unit, port)) {
        pi->phy_name = my_entry->phy_name;

        return TRUE;
    }

    return FALSE;
}
#endif /* BCM_XGS3_SWITCH_SUPPORT || BCM_ROBO_SUPPORT */

#if defined(INCLUDE_PHY_SIMUL)

/*
 * Function:
 *      _chk_simul
 * Purpose:
 *      Check function for simulation phys.
 * Returns:
 *      True if this phy matches.
 *      False otherwise.
 */

static int
_chk_simul(int unit, soc_port_t port,  soc_phy_table_t *my_entry,
           uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi)
{
    if (USE_SIMULATION_PHY(unit, port)) {
        pi->phy_name = my_entry->phy_name;

        return TRUE;
    }

    return FALSE;
}
#endif /* include phy sim */

#undef USE_SIMULATION_PHY

#ifdef INCLUDE_XGXS_QSGMII65
/*
 * Function:
 *      _chk_qsgmii53314
 * Purpose:
 *      Check for using Internal 53314 SERDES Device 
 * Returns:
 *      TRUE if this PHY matches.
 *      FALSE otherwise.
 * Notes:
 *      This routine should be called after checking for external
 *      PHYs because it will default the PHY driver to the internal
 *      SERDES in the absense of any other PHY.
 */

static int
_chk_qsgmii53314(int unit, soc_port_t port, soc_phy_table_t *my_entry,
               uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi)
{
    COMPILER_REFERENCE(my_entry);
    COMPILER_REFERENCE(phy_id0);
    COMPILER_REFERENCE(phy_id1);
    COMPILER_REFERENCE(pi);

    if (SOC_IS_HAWKEYE(unit) && port >= 9) {
        pi->phy_name = "Phy53314";
        return TRUE;
    }

    if (IS_GE_PORT(unit, port) && (SOC_IS_HURRICANE(unit) && (port >= 2) && (port <= 25))) {
        pi->phy_name = "Phy53314";
        return TRUE;
    }

    return FALSE;
}
#endif /* INCLUDE_XGXS_QSGMII65 */

#ifdef INCLUDE_PHY_54680
/*
 * Function:
 *      _chk_qgphy_5332x
 * Purpose:
 *      Check for using 5332x QGPHY Device 
 * Returns:
 *      TRUE if this PHY matches.
 *      FALSE otherwise.
 * Notes:
 *      Because The Id of QGPHY device of HKEEE is same as BCM54682E,
 *      the _chk_phy can be used for QGPHY device of HKEEE
 */

static int
_chk_qgphy_5332x(int unit, soc_port_t port, soc_phy_table_t *my_entry,
               uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi)
{
    COMPILER_REFERENCE(my_entry);
    COMPILER_REFERENCE(phy_id0);
    COMPILER_REFERENCE(phy_id1);
    COMPILER_REFERENCE(pi);

    if (SOC_IS_HAWKEYE(unit) && 
        soc_feature (unit, soc_feature_eee) && 
        (port <= 8) && 
        (port != 0)) {
        pi->phy_name = "BCM53324";
        return TRUE;
    }

    return FALSE;
}
#endif /* INCLUDE_PHY_54680 */

#if defined(BCM_XGS3_SWITCH_SUPPORT)
/*
 * Function:
 *      _chk_fiber56xxx
 * Purpose:
 *      Check for using Internal 56XXX SERDES Device for fiber
 * Returns:
 *      TRUE if this PHY matches.
 *      FALSE otherwise.
 * Notes:
 *      This routine should be called after checking for external
 *      PHYs because it will default the PHY driver to the internal
 *      SERDES in the absense of any other PHY.
 */

static int
_chk_fiber56xxx(int unit, soc_port_t port, soc_phy_table_t *my_entry,
               uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi)
{
    COMPILER_REFERENCE(my_entry);
    COMPILER_REFERENCE(phy_id0);
    COMPILER_REFERENCE(phy_id1);

    if (IS_GE_PORT(unit, port) && !IS_GMII_PORT(unit, port) &&
        !SOC_IS_HAWKEYE(unit) && !SOC_IS_HURRICANE2(unit) &&
        soc_feature(unit, soc_feature_dodeca_serdes)) {
        pi->phy_name = "Phy56XXX";

        return TRUE;
    }

    return FALSE;
}
#endif /* BCM_XGS3_SWITCH_SUPPORT */

#if defined(INCLUDE_PHY_XGXS6)
/*
 * Function:
 *      _chk_unicore
 * Purpose:
 *      Check for Unicore, which may return two different PHY IDs
 *      depending on the current IEEE register mapping. One of
 *      these PHY IDs conflicts with the BCM5673/74 PHY ID, so
 *      we need to check the port type here as well.
 * Parameters:
 *      See soc_phy_ident_f
 *      *pi is an output parameter filled with PHY info.
 * Returns:
 *      TRUE if this PHY matches.
 *      FALSE otherwise.
 * Notes:
 *      Sets the phy_info structure for this port.
 */

static int
_chk_unicore(int unit, soc_port_t port, soc_phy_table_t *my_entry,
             uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi)
{
    COMPILER_REFERENCE(unit);

    if (my_entry->myNum == _phy_ident_type_get(phy_id0, phy_id1) &&
        soc_feature(unit, soc_feature_xgxs_v6)) {
        pi->phy_name = my_entry->phy_name;
        return TRUE;
    }

    return FALSE;
}
#endif /* INCLUDE_PHY_XGXS6 */

static int
_chk_sfp_phy(int unit, soc_port_t port, soc_phy_table_t *my_entry,
         uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi)
{
    COMPILER_REFERENCE(unit);

    if (soc_property_port_get(unit, port, spn_PHY_COPPER_SFP, 0)) {
        if (!(phy_id0 == (uint16)0xFFFF && phy_id1 == (uint16)0xFFFF)) {
            SOC_DEBUG_PRINT((DK_PHY,
                "_chk_sfp_phy: u=%d p=%d id0=0x%x, id1=0x%x,"
                " oui=0x%x,model=0x%x,rev=0x%x\n",
                         unit, port,phy_id0,phy_id1,PHY_OUI(phy_id0, phy_id1),
                         PHY_MODEL(phy_id0, phy_id1),PHY_REV(phy_id0, phy_id1)));
            pi->phy_name = my_entry->phy_name;
            return TRUE;
        }
    }
    return FALSE;
}

#if defined(INCLUDE_PHY_8706)
static int
_chk_8706(int unit, soc_port_t port, soc_phy_table_t *my_entry,
             uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi)
{
    if (my_entry->myNum == _phy_ident_type_get(phy_id0, phy_id1) ||
        soc_property_port_get(unit, port, spn_PHY_8706, FALSE)) {
        pi->phy_name = my_entry->phy_name;
        return TRUE;
    }
    return FALSE;
}

#endif /* INCLUDE_PHY_8706 */

#if defined(INCLUDE_PHY_8040)
static int
_chk_8040(int unit, soc_port_t port, soc_phy_table_t *my_entry,
             uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi)
{
    if (my_entry->myNum == _phy_ident_type_get(phy_id0, phy_id1)) {
        pi->phy_name = my_entry->phy_name;
        return TRUE;
    }
    return FALSE;
}
#endif /* INCLUDE_PHY_8040 */

#if defined(INCLUDE_PHY_8072)
static int
_chk_8072(int unit, soc_port_t port, soc_phy_table_t *my_entry,
             uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi)
{
    if (my_entry->myNum == _phy_ident_type_get(phy_id0, phy_id1) ||
        soc_property_port_get(unit, port, spn_PHY_8072, FALSE)) {
        pi->phy_name = my_entry->phy_name;
        return TRUE;
    }
    return FALSE;
}

#endif /* INCLUDE_PHY_8072 */

#if defined(INCLUDE_PHY_8481)
static int
_chk_8481(int unit, soc_port_t port, soc_phy_table_t *my_entry,
             uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi)
{

    if (!IS_XE_PORT(unit, port) && !IS_HG_PORT(unit, port)) {
        return FALSE;
    }

    if (my_entry->myNum == _phy_ident_type_get(phy_id0, phy_id1)) {
        pi->phy_name = my_entry->phy_name;
        return TRUE;
    }

    return FALSE;

}
#endif /* INCLUDE_PHY_8481 */

#if defined(INCLUDE_PHY_56XXX)
/*
 * Function:
 *      _chk_fiber56xxx_5601x
 * Purpose:
 *      Check for using Internal SERDES Device for fiber with shadow registers
 * Returns:
 *      TRUE if this PHY matches.
 *      FALSE otherwise.
 * Notes:
 *      This routine should be called after checking for external
 *      PHYs because it will default the PHY driver to the internal
 *      SERDES in the absense of any other PHY.
 */

static int
_chk_fiber56xxx_5601x(int unit, soc_port_t port, soc_phy_table_t *my_entry,
               uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi)
{
    uint8       phy_addr;

    COMPILER_REFERENCE(my_entry);
    COMPILER_REFERENCE(phy_id0);
    COMPILER_REFERENCE(phy_id1);

    phy_addr = PORT_TO_PHY_ADDR_INT(unit, port);

    if (SOC_IS_RAPTOR(unit) && 
        soc_feature(unit, soc_feature_fe_ports) &&
        ((port == 4) || (port == 5)) &&
        (soc_property_port_get(unit, port, spn_SERDES_SHADOW_DRIVER, FALSE) ||
         _is_corrupted_reg(unit, phy_addr))) {
        pi->phy_name = "Phy56XXX5601x";
        return TRUE;
    }

    return FALSE;
}
#endif /* INCLUDE_PHY_56XXX */

#if defined(INCLUDE_SERDES_COMBO)
/*
 * Function:
 *      _chk_serdes_combo_5601x
 * Purpose:
 *      Check for using Internal SERDES Device for fiber with shadow registers
 * Returns:q
 *      TRUE if this PHY matches.
 *      FALSE otherwise.
 * Notes:
 *      This routine should be called after checking for external
 *      PHYs because it will default the PHY driver to the internal
 *      SERDES in the absense of any other PHY.
 */

static int
_chk_serdes_combo_5601x(int unit, soc_port_t port, soc_phy_table_t *my_entry,
               uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi)
{
    uint8       phy_addr;

    COMPILER_REFERENCE(my_entry);
    COMPILER_REFERENCE(phy_id0);
    COMPILER_REFERENCE(phy_id1);

    phy_addr = PORT_TO_PHY_ADDR_INT(unit, port);

    if (SOC_IS_RAPTOR(unit) && 
        soc_feature(unit, soc_feature_fe_ports) &&
        ((port == 1) || (port == 2)) &&
        (soc_property_port_get(unit, port, spn_SERDES_SHADOW_DRIVER, FALSE) ||
         _is_corrupted_reg(unit, phy_addr))) {
        pi->phy_name = "COMBO5601x";
        return TRUE;
    }    

    return FALSE;
}
#endif /* INCLUDE_SERDES_COMBO */

#if defined(INCLUDE_PHY_53XXX)
/*
 * Function:
 *      _chk_fiber53xxx
 * Purpose:
 *      Check for using Internal 53XXX SERDES Device for fiber
 * Returns:
 *      TRUE if this PHY matches.
 *      FALSE otherwise.
 * Notes:
 *      This routine should be called after checking for external
 *      PHYs because it will default the PHY driver to the internal
 *      SERDES in the absense of any other PHY.
 */

static int
_chk_fiber53xxx(int unit, soc_port_t port, soc_phy_table_t *my_entry,
               uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi)
{
    COMPILER_REFERENCE(my_entry);
    COMPILER_REFERENCE(phy_id0);
    COMPILER_REFERENCE(phy_id1);

    if (IS_GE_PORT(unit, port) && !IS_GMII_PORT(unit, port) &&
        soc_feature(unit, soc_feature_dodeca_serdes) && 
        SOC_IS_ROBO(unit)) {
        pi->phy_name = "Phy53XXX";

        return TRUE;
    }

    return FALSE;
}
#endif /* INCLUDE_PHY_53XXX */
/*
 * Function:
 *      _chk_default
 * Purpose:
 *      Select a default PHY driver.
 * Returns:
 *      TRUE
 * Notes:
 *      This routine always "finds" a default PHY driver and can
 *      be the last entry in the PHY table (or called explicitly).
 */

static int
_chk_default(int unit, soc_port_t port, soc_phy_table_t *my_entry,
             uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi)
{
    pi->phy_name = my_entry->phy_name;

    return TRUE;
}

STATIC int
_forced_phy_probe(int unit, soc_port_t port,
                  soc_phy_info_t *pi, phy_ctrl_t *ext_pc)
{
    phy_driver_t   *phyd;
    char *s;

    phyd = NULL;
#if defined(INCLUDE_PHY_SIMUL)
    /* Similarly, check for simulation driver */
    if (phyd == NULL &&
        _chk_simul(unit, port, &_simul_phy_entry, 0xffff, 0xffff, pi)) {
        ext_pc->pd  = _simul_phy_entry.driver;
        pi->phy_id0 = 0xffff;
        pi->phy_id1 = 0xffff;
    }
#endif

#if defined(BCM_XGS3_SWITCH_SUPPORT)
    /* Check for property forcing fiber mode on XGS3 switch */
    if (phyd == NULL &&
        soc_property_port_get(unit, port, spn_PHY_56XXX, FALSE) &&
        _chk_fiber56xxx(unit, port, &_fiber56xxx_phy_entry,
                        0xffff, 0xffff, pi)) {
        /* Correct internal SerDes PHY driver is already attached by
         * internal PHY probe. Therefore, just assign NULL to external PHY
         * driver.
         */
        ext_pc->pd   = NULL;
        pi->phy_id0  = 0xffff;
        pi->phy_id1  = 0xffff;
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT */
    s = soc_property_get_str(unit, "board_name");
    if( (s != NULL) && (sal_strcmp(s, "bcm53280_fpga") == 0)) {
        /* force the external PHY driver on TB's FE port in FPGA board. 
         *  - MAC is fe ports but GE PHY ports designed in FPGA board
         */
        if (phyd == NULL && IS_FE_PORT(unit, port)){
            soc_cm_print("_forced_phy_probe(),[FPGA]:port %d, ", port);
            if (ext_pc->pd == NULL){
                soc_cm_print("No external PHY connected!\n");
            } 
#if defined(INCLUDE_PHY_5464_ROBO) && defined(INCLUDE_PHY_522X)
            else if (ext_pc->pd == &phy_5464robodrv_ge){
                ext_pc->pd   = &phy_522xdrv_fe;
                soc_cm_print("FE driver for bcm5464 PHY!\n");
            } 
#endif /* defined(INCLUDE_PHY_5464_ROBO) && defined(INCLUDE_PHY_522X) */
            else {
                soc_cm_print("Unexpected PHY connected!\n");
            }
        }
    }
    /* Forced PHY will have internal PHY device ID */
    return SOC_E_NONE;
}

STATIC int
_int_phy_probe(int unit, soc_port_t port,
               soc_phy_info_t *pi, phy_ctrl_t *int_pc)
{
    uint16               phy_addr;
    uint16               phy_id0, phy_id1;
    int                  i;
    int                  rv;
    phy_driver_t         *int_phyd;
#if defined(BCM_KATANA2_SUPPORT)
    uint8 lane_num = 0;
    uint8 phy_mode = 0;
    uint8 chip_num = 0;
#endif

    phy_addr = int_pc->phy_id;
    int_phyd = NULL;
    i = sizeof(_int_phy_table) / sizeof(_int_phy_table[0]);

    /* Make sure page 0 is mapped before reading for PHY dev ID */

#if defined(BCM_TRIUMPH3_SUPPORT) 
    if (SOC_IS_TRIUMPH3(unit) &&
        (SOC_PBMP_MEMBER(SOC_PORT_DISABLED_BITMAP(unit,all), port))) {
        
        phy_id0 = phy_id1 = 0;
    } else 
#endif /* BCM_TRIUMPH3_SUPPORT */
    {
    (void)int_pc->write(unit, phy_addr, 0x1f, 0);

    (void)int_pc->read(unit, phy_addr, MII_PHY_ID0_REG, &phy_id0);
    (void)int_pc->read(unit, phy_addr, MII_PHY_ID1_REG, &phy_id1);
    }

    
    pi->phy_id0       = phy_id0;
    pi->phy_id1       = phy_id1;
    pi->phy_addr_int  = phy_addr;
    int_pc->phy_id0   = phy_id0;
    int_pc->phy_id1   = phy_id1;
    int_pc->phy_oui   = PHY_OUI(phy_id0, phy_id1);
    int_pc->phy_model = PHY_MODEL(phy_id0, phy_id1);
    int_pc->phy_rev   = PHY_REV(phy_id0, phy_id1);

    for (i = i - 1; i >= 0; i--) {
        if ((_int_phy_table[i].checkphy)(unit, port, &_int_phy_table[i],
                                         phy_id0, phy_id1, pi)) {
            /* Device ID matches. Calls driver probe routine to confirm
             * that the driver is the appropriate one.
             * Many PHY devices has the same device ID but they are
             * actually different.
             */
            rv = PHY_PROBE(_int_phy_table[i].driver, unit, int_pc);
            if ((rv == SOC_E_NONE) || (rv == SOC_E_UNAVAIL)) {
                SOC_DEBUG_PRINT((DK_PHY, "<%d> int Index = %d Mynum = %d %s\n",
                    rv, i, _int_phy_table[i].myNum, _int_phy_table[i].phy_name));
                int_phyd = _int_phy_table[i].driver;
                break;
            }
        }
    }

#if defined(INCLUDE_XGXS_16G)
    if (IS_GE_PORT(unit, port) && (&phy_xgxs16g1l_ge == int_phyd)) {
        /* using XGXS16G in independent lane mode on GE port.  */
        PHY_FLAGS_SET(unit, port, PHY_FLAGS_INDEPENDENT_LANE);
#if defined(BCM_FE2000_SUPPORT)
        if (SOC_IS_SBX_FE2KXT(unit)) {
	  int_pc->flags |= PHYCTRL_MDIO_ADDR_SHARE;
            switch(SOC_PORT_BINDEX(unit, port)) {
            case 8:
                int_pc->lane_num = 0;
                break;
	    case 9:
                int_pc->lane_num = 1;
                break;
	    case 10:
                int_pc->lane_num = 2;
                break;
	    case 11:
                int_pc->lane_num = 3;
                break;
            default:
                break;
            }
        }
#endif
#if defined(BCM_SCORPION_SUPPORT)
        if (SOC_IS_SCORPION(unit)) {
            switch(port) {
            case 25:
                pi->phy_name = "XGXS16G/1/0";
                int_pc->lane_num = 0;
                break;
            case 26:
                pi->phy_name = "XGXS16G/1/1";
                int_pc->lane_num = 1;      
                break;
            case 27:
                pi->phy_name = "XGXS16G/1/2";
                int_pc->lane_num = 2;
                break;
            case 28:
                pi->phy_name = "XGXS16G/1/3";
                int_pc->lane_num = 3;
                break;
            default:
                break;
            }
        }
#endif
    }
#endif

#if defined(INCLUDE_XGXS_HL65)
    if (IS_GE_PORT(unit, port) && (&phy_hl65_hg == int_phyd)) {
        /* If using HyperLite on GE port, use the HyperLite in independent
         * lane mode.
         */
        PHY_FLAGS_SET(unit, port, PHY_FLAGS_INDEPENDENT_LANE);
        pi->phy_name = "HL65/1";
#if defined(BCM_HURRICANE_SUPPORT)
        if (SOC_IS_HURRICANE(unit)) {
            int_pc->phy_mode = PHYCTRL_DUAL_LANE_PORT;
            int_pc->flags |= PHYCTRL_MDIO_ADDR_SHARE;
            switch (port) {
                case 26:
                    pi->phy_name = "HL65/0/0";
                    int_pc->lane_num = 0;
                    break;
                case 27:
                    pi->phy_name = "HL65/0/2";
                    int_pc->lane_num = 2;
                    break;
                case 28:
                    pi->phy_name = "HL65/1/0";
                    int_pc->lane_num = 0;
                    break;
                case 29:
                    pi->phy_name = "HL65/1/2";
                    int_pc->lane_num = 2;
                    break;
                default:
                    break;
            }
        }
#endif
#if defined(BCM_VALKYRIE_SUPPORT) || defined(BCM_TRIUMPH_SUPPORT)
        if (SOC_IS_VALKYRIE(unit) || SOC_IS_TRIUMPH(unit)) {
            switch(port) {
            case 2:
            case 6:
            case 14:
            case 26:
            case 27:
            case 35:
                pi->phy_name = "HL65/1/0";
                int_pc->lane_num = 0;
                break;

            case 3:
            case 7:
            case 15:
            case 32:
            case 36:
            case 43:
                pi->phy_name = "HL65/1/1";
                int_pc->lane_num = 1;
                break;

            case 4:
            case 16:
            case 18:
            case 33:
            case 44:
            case 46:
                pi->phy_name = "HL65/1/2";
                int_pc->lane_num = 2;
                break;

            case 5:
            case 17:
            case 19:
            case 34:
            case 45:
            case 47:
                pi->phy_name = "HL65/1/3";
                int_pc->lane_num = 3;
                break;

            default:
                break;

            }
        }
#endif /* BCM_VALKYRIE_SUPPORT */
#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_APOLLO_SUPPORT) || \
    defined(BCM_VALKYRIE2_SUPPORT)
        if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) || 
            SOC_IS_VALKYRIE2(unit)) {
            switch(port) {
            case 30:
            case 34:
            case 38:
            case 42:
            case 46:
            case 50:
                pi->phy_name = "HL65/1/0";
                int_pc->lane_num = 0;
                break;

            case 31:
            case 35:
            case 39:
            case 43:
            case 47:
            case 51:
                pi->phy_name = "HL65/1/1";
                int_pc->lane_num = 1;
                break;

            case 32:
            case 36:
            case 40:
            case 44:
            case 48:
            case 52:
                pi->phy_name = "HL65/1/2";
                int_pc->lane_num = 2;
                break;

            case 33:
            case 37:
            case 41:
            case 45:
            case 49:
            case 53:
                pi->phy_name = "HL65/1/3";
                int_pc->lane_num = 3;
                break;

            default:
                break;

            }
        }
#endif /* BCM_TRIUMPH2_SUPPORT */
    }

#if defined(BCM_HURRICANE_SUPPORT)
        if ((&phy_hl65_hg == int_phyd) && SOC_IS_HURRICANE(unit) && ((IS_HG_PORT(unit, port)) || IS_XE_PORT(unit, port))) {
            if ((port == 26) && (!SOC_PORT_VALID(unit, 27))) {
                SOC_DEBUG_PRINT((DK_PHY, "Port 26 in combo mode\n"));
                pi->phy_name = "HL65/0";
            } else if ((port == 28) && (!SOC_PORT_VALID(unit, 29))) {
                SOC_DEBUG_PRINT((DK_PHY, "Port 28 in combo mode\n"));
                pi->phy_name = "HL65/1";
            } else {
                SOC_DEBUG_PRINT((DK_PHY, "Port %d in HGd mode\n", port));
                PHY_FLAGS_SET(unit, port, PHY_FLAGS_INDEPENDENT_LANE);
                int_pc->phy_mode = PHYCTRL_DUAL_LANE_PORT;
                int_pc->flags |= PHYCTRL_MDIO_ADDR_SHARE;
                switch (port) {
                    case 26:
                        pi->phy_name = "HL65/0/0";
                        int_pc->lane_num = 0;
                        break;
                    case 27:
                        pi->phy_name = "HL65/0/2";
                        int_pc->lane_num = 2;
                        break;
                    case 28:
                        pi->phy_name = "HL65/1/0";
                        int_pc->lane_num = 0;
                        break;
                    case 29:
                        pi->phy_name = "HL65/1/2";
                        int_pc->lane_num = 2;
                        break;
                    default:
                        break;
                }
            }
        }
#endif /* BCM_HURRICANE_SUPPORT */

#if defined(BCM_BM9600_SUPPORT) || defined(BCM_SIRIUS_SUPPORT)
    if (&phy_hl65_hg == int_phyd) {
        if (SOC_IS_SBX_BM9600(unit)) {
            static char *bm9600_phy_names[] = {
                "HC65/0/0",  "HC65/0/1",  "HC65/0/2",  "HC65/0/3",  
                "HC65/1/0",  "HC65/1/1",  "HC65/1/2",  "HC65/1/3",  
                "HC65/2/0",  "HC65/2/1",  "HC65/2/2",  "HC65/2/3",  
                "HC65/3/0",  "HC65/3/1",  "HC65/3/2",  "HC65/3/3",  
                "HC65/4/0",  "HC65/4/1",  "HC65/4/2",  "HC65/4/3",  
                "HC65/5/0",  "HC65/5/1",  "HC65/5/2",  "HC65/5/3",  
                "HC65/6/0",  "HC65/6/1",  "HC65/6/2",  "HC65/6/3",  
                "HC65/7/0",  "HC65/7/1",  "HC65/7/2",  "HC65/7/3",  
                "HC65/8/0",  "HC65/8/1",  "HC65/8/2",  "HC65/8/3",  
                "HC65/9/0",  "HC65/9/1",  "HC65/9/2",  "HC65/9/3",  
                "HC65/10/0", "HC65/10/1", "HC65/10/2", "HC65/10/3", 
                "HC65/11/0", "HC65/11/1", "HC65/11/2", "HC65/11/3", 
                "HC65/12/0", "HC65/12/1", "HC65/12/2", "HC65/12/3", 
                "HC65/13/0", "HC65/13/1", "HC65/13/2", "HC65/13/3", 
                "HC65/14/0", "HC65/14/1", "HC65/14/2", "HC65/14/3", 
                "HC65/15/0", "HC65/15/1", "HC65/15/2", "HC65/15/3", 
                "HC65/16/0", "HC65/16/1", "HC65/16/2", "HC65/16/3", 
                "HC65/17/0", "HC65/17/1", "HC65/17/2", "HC65/17/3", 
                "HC65/18/0", "HC65/18/1", "HC65/18/2", "HC65/18/3", 
                "HC65/19/0", "HC65/19/1", "HC65/19/2", "HC65/19/3", 
                "HC65/20/0", "HC65/20/1", "HC65/20/2", "HC65/20/3", 
                "HC65/21/0", "HC65/21/1", "HC65/21/2", "HC65/21/3", 
                "HC65/22/0", "HC65/22/1", "HC65/22/2", "HC65/22/3", 
                "HC65/23/0", "HC65/23/1", "HC65/23/2", "HC65/23/3" 
            }; 
            pi->phy_name = bm9600_phy_names[port];
            int_pc->lane_num = port % 4;
            int_pc->flags |= PHYCTRL_MDIO_ADDR_SHARE;
            PHY_FLAGS_SET(unit, port, PHY_FLAGS_HC65_FABRIC);
        } else {
            if (SOC_IS_SIRIUS(unit)) {
                static struct {
                    char  *name;
                    int    lane;
                } _sirius_phy_port_info[] = {
                    { "HC65/4/2", 2 },  /* port 9 , sfi0,  lane 2 */
                    { "HC65/4/3", 3 },  /* port 10, sfi1,  lane 3 */
                    { "HC65/5/0", 0 },  /* port 11, sfi2,  lane 0 */
                    { "HC65/5/1", 1 },  /* port 12, sfi3,  lane 1 */
                    { "HC65/5/2", 2 },  /* port 13, sfi4,  lane 2 */
                    { "HC65/5/3", 3 },  /* port 14, sfi5,  lane 3 */
                    { "HC65/6/0", 0 },  /* port 15, sfi6,  lane 0 */
                    { "HC65/6/1", 1 },  /* port 16, sfi7,  lane 1 */
                    { "HC65/6/2", 2 },  /* port 17, sfi8,  lane 2 */
                    { "HC65/6/3", 3 },  /* port 18, sfi9,  lane 3 */
                    { "HC65/7/0", 0 },  /* port 19, sfi10, lane 0 */
                    { "HC65/7/1", 1 },  /* port 20, sfi11, lane 1 */
                    { "HC65/7/2", 2 },  /* port 21, sfi12, lane 2 */
                    { "HC65/7/3", 3 },  /* port 22, sfi13, lane 3 */
                    { "HC65/8/0", 0 },  /* port 23, sfi14, lane 0 */
                    { "HC65/8/1", 1 },  /* port 24, sfi15, lane 1 */
                    { "HC65/8/2", 2 },  /* port 25, sfi16, lane 2 */
                    { "HC65/8/3", 3 },  /* port 26, sfi17, lane 3 */
                    { "HC65/9/0", 0 },  /* port 27, sfi18, lane 0 */
                    { "HC65/9/1", 1 },  /* port 28, sfi19, lane 1 */
                    { "HC65/9/2", 2 },  /* port 29, sfi20, lane 2 */
                    { "HC65/9/3", 3 },  /* port 30, sfi21, lane 3 */
                    { "HC65/4/0", 0 },  /* port 31, sci0,  lane 0 */
                    { "HC65/4/1", 1 }   /* port 32, sci1,  lane 1 */
                };
                /*
                 * Make sure port is SFI or SCI. 
                 *  0..7  : Higig 
                 *  8     : CPU
                 *  9..30 : SFI
                 *  31-32 : SCI
                 */
                if (port >= 9 && port <= 32) {
                    int port_idx = port - 9;
                    pi->phy_name = _sirius_phy_port_info[port_idx].name;
                    int_pc->lane_num = _sirius_phy_port_info[port_idx].lane;
                    int_pc->flags |= PHYCTRL_MDIO_ADDR_SHARE;
                    PHY_FLAGS_SET(unit, port, PHY_FLAGS_HC65_FABRIC);
                }
            }
        }
    }
#endif /* BCM_BM9600_SUPPORT || BCM_SIRIUS_SUPPORT */
#endif /* INCLUDE_XGXS_HL65 */

#if defined(INCLUDE_XGXS_QSGMII65)
    if (IS_GE_PORT(unit, port) && (&phy_qsgmii65_ge == int_phyd)) {
        pi->phy_name = "QSGMII65";
#if defined(BCM_HAWKEYE_SUPPORT)
        if (SOC_IS_HAWKEYE(unit)) {
            switch(port) {
            case 9:
            case 17:
                pi->phy_name = "QSGMII65/0";
                int_pc->lane_num = 0;
                break;

            case 10:
            case 18:
                pi->phy_name = "QSGMII65/1";
                int_pc->lane_num = 1;
                break;

            case 11:
            case 19:
                pi->phy_name = "QSGMII65/2";
                int_pc->lane_num = 2;
                break;

            case 12:
            case 20:
                pi->phy_name = "QSGMII65/3";
                int_pc->lane_num = 3;
                break;

            case 13:
            case 21:
                pi->phy_name = "QSGMII65/4";
                int_pc->lane_num = 4;
                break;

            case 14:
            case 22:
                pi->phy_name = "QSGMII65/5";
                int_pc->lane_num = 5;
                break;

            case 15:
            case 23:
                pi->phy_name = "QSGMII65/6";
                int_pc->lane_num = 6;
                break;

            case 16:
            case 24:
                pi->phy_name = "QSGMII65/7";
                int_pc->lane_num = 7;
                break;

            default:
                break;

            }
        }
#endif /* SOC_IS_HAWKEYE */
#if defined(BCM_HURRICANE_SUPPORT)
        if (SOC_IS_HURRICANE(unit)) {
            switch(port) {
            case 2:
            case 10:
            case 18:
                pi->phy_name = "QSGMII65/0";
                int_pc->lane_num = 0;
                break;

            case 3:
            case 11:
            case 19:
                pi->phy_name = "QSGMII65/1";
                int_pc->lane_num = 1;
                break;

            case 4:
            case 12:
            case 20:
                pi->phy_name = "QSGMII65/2";
                int_pc->lane_num = 2;
                break;

            case 5:
            case 13:
            case 21:
                pi->phy_name = "QSGMII65/3";
                int_pc->lane_num = 3;
                break;

            case 6:
            case 14:
            case 22:
                pi->phy_name = "QSGMII65/4";
                int_pc->lane_num = 4;
                break;

            case 7:
            case 15:
            case 23:
                pi->phy_name = "QSGMII65/5";
                int_pc->lane_num = 5;
                break;

            case 8:
            case 16:
            case 24:
                pi->phy_name = "QSGMII65/6";
                int_pc->lane_num = 6;
                break;

            case 9:
            case 17:
            case 25:
                pi->phy_name = "QSGMII65/7";
                int_pc->lane_num = 7;
                break;

            default:
                break;

            }
        }
#endif /* BCM_HURRICANE_SUPPORT */
    }
#endif /* INCLUDE_XGXS_QSGMII65 */

#if defined(BCM_KATANA2_SUPPORT)
    if (SOC_IS_KATANA2(unit)) {
        if (katana2_get_wc_phy_info(unit,port,&lane_num, &phy_mode, &chip_num) == SOC_E_NONE) {
            int_pc->lane_num = lane_num;
            int_pc->phy_mode = phy_mode;
            int_pc->chip_num = chip_num;
            int_pc->flags |= PHYCTRL_MDIO_ADDR_SHARE;
            /* PHY_FLAGS_SET(unit, port, PHY_FLAGS_INDEPENDENT_LANE); */
        }
        if ((SOC_INFO(unit).olp_port) && (port == KT2_OLP_PORT)) {
            int_pc->lane_num = 2;
        }
    }

#endif

#if defined(INCLUDE_PHY_56XXX) && defined(INCLUDE_PHY_XGXS6)
    /* If we detetcted a Unicore driver for a GE port, attach internal SerDes
     * driver.
     * Current Unicore driver does not support external GE PHY.
     */
    if (IS_GE_PORT(unit, port) &&  (&phy_xgxs6_hg == int_phyd)) {
        if (_chk_fiber56xxx(unit, port, &_fiber56xxx_phy_entry,
                       phy_id0, phy_id1, pi)) {
            int_phyd = &phy_56xxxdrv_ge;
        }
    }
#endif /* INCLUDE_PHY_56XXX && INCLUDE_PHY_XGXS6 */


#if defined(INCLUDE_PHY_56XXX) 
    /* If we detected a shadow register driver, allocate driver data */
    if (&phy_56xxx_5601x_drv_ge == int_phyd) {
        serdes_5601x_sregs_t *sr;

        /* Allocate shadow registers */
        sr = sal_alloc(sizeof(serdes_5601x_sregs_t), 
                       "SERDES_COMBO shadow regs");
        if (sr == NULL) {
            return SOC_E_MEMORY;
        }
        int_pc->driver_data = sr;
    }
#endif /* INCLUDE_PHY_56XXX */

#if defined(INCLUDE_SERDES_COMBO)
    /* If we detected a shadow register driver, allocate driver data */
    if (&phy_serdescombo_5601x_ge == int_phyd) {
        serdescombo_5601x_sregs_t *sr;

        /* Allocate shadow registers */
        sr = sal_alloc(sizeof(serdescombo_5601x_sregs_t), 
                       "SERDES_COMBO shadow regs");
        if (sr == NULL) {
            return SOC_E_MEMORY;
        }
        int_pc->driver_data = sr;
    }
#endif /* INCLUDE_SERDES_COMBO */

#if defined(INCLUDE_PHY_53XXX)
    /* If we detetcted a Unicore driver for a GE port, attach internal SerDes
     * driver.
     * Current Unicore driver does not support external GE PHY.
     */
    if (IS_GE_PORT(unit, port)) {
        if (_chk_fiber53xxx(unit, port, &_fiber53xxx_phy_entry,
                       phy_id0, phy_id1, pi)) {
            int_phyd = &phy_53xxxdrv_ge;
        }
    }
#endif /* INCLUDE_PHY_53XXX */

#ifdef BCM_XGS_SUPPORT
    if (int_phyd == NULL) {
        if (IS_XE_PORT(unit, port) || IS_HG_PORT(unit, port)) {
            /* If no appropriate driver is installed in the phy driver table
             * use a default higig driver for XE port */
#if defined (INCLUDE_PHY_XGXS6)
            if (soc_feature(unit, soc_feature_xgxs_v6)) {
                int_phyd = &phy_xgxs6_hg;
            } else
#endif /* INCLUDE_PHY_XGXS6*/
#if defined(INCLUDE_PHY_XGXS5)
            if (soc_feature(unit, soc_feature_xgxs_v5)) {
                int_phyd = &phy_xgxs5_hg;
            } else
#endif /* INCLUDE_PHY_XGXS5 */
            {
                int_phyd = &phy_xgxs1_hg;
            }
        }
    }
#endif /* BCM_XGS_SUPPORT */

#if defined(BCM_XGS3_SWITCH_SUPPORT) || defined(BCM_ROBO_SUPPORT)
    /* If GMII port, attach NULL PHY driver to
     * internal PHY driver. GMII port does not have SerDes.
     */
    if (_chk_gmii(unit, port, &_null_phy_entry, 0xffff, 0xffff, pi)) {
        int_phyd = _null_phy_entry.driver;
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT || BCM_ROBO_SUPPORT */

    int_pc->pd = int_phyd;

    return SOC_E_NONE;
}



int
_ext_phy_probe(int unit, soc_port_t port,
               soc_phy_info_t *pi, phy_ctrl_t *ext_pc)
{
    uint16               phy_addr;
    uint32               id0_addr, id1_addr;
    uint16               phy_id0=0, phy_id1=0;
    int                  i;
    phy_driver_t        *phyd;
    int                  rv;
    int                  cl45_override = 0;
    char                 *propval;
#ifdef BCM_NORTHSTAR_SUPPORT
    uint16               dev_id = 0;
    uint8                rev_id = 0;
#endif /* BCM_NORTHSTAR_SUPPORT */

    phy_addr = ext_pc->phy_id;
    phyd     = NULL;

    /* Clause 45 instead of Clause 22 MDIO access */
    if (soc_property_port_get(unit, port, spn_PORT_PHY_CLAUSE, 22) == 45) {
        cl45_override = 1;
    }

    if (IS_XE_PORT(unit, port) || IS_HG_PORT(unit, port) || IS_CE_PORT(unit, port) || SOC_IS_ARAD(unit)) {
        if (SOC_IS_CALADAN3(unit) || SOC_IS_ARAD(unit)) {
            if (cl45_override) {
                id0_addr = SOC_PHY_CLAUSE45_ADDR(PHY_C45_DEV_PMA_PMD,
                                         MII_PHY_ID0_REG);
                id1_addr = SOC_PHY_CLAUSE45_ADDR(PHY_C45_DEV_PMA_PMD,
                                         MII_PHY_ID1_REG);
            } else {
                id0_addr = MII_PHY_ID0_REG;
                id1_addr = MII_PHY_ID1_REG;
            }
        } else {
            id0_addr = SOC_PHY_CLAUSE45_ADDR(PHY_C45_DEV_PMA_PMD,
                                         MII_PHY_ID0_REG);
            id1_addr = SOC_PHY_CLAUSE45_ADDR(PHY_C45_DEV_PMA_PMD,
                                         MII_PHY_ID1_REG);
        }
    } else {
#ifdef BCM_SIRIUS_SUPPORT
        if (SOC_IS_SIRIUS(unit)) {
            /* Do not probe GE ports in Sirius */
            ext_pc->pd  = NULL;
            return SOC_E_NONE;
        }
#endif /* BCM_SIRIUS_SUPPORT */
        id0_addr = MII_PHY_ID0_REG;
        id1_addr = MII_PHY_ID1_REG;
    }
    propval = soc_property_port_get_str(unit, port, spn_PORT_PHY_ID0);
    if (propval != NULL){
        phy_id0 = soc_property_port_get(unit, port, spn_PORT_PHY_ID0, 0xFFFF);
    }
    else{
        (void)ext_pc->read(unit, phy_addr, id0_addr, &phy_id0);
    }
    propval = soc_property_port_get_str(unit, port, spn_PORT_PHY_ID1);
    if (propval != NULL){
        phy_id1 = soc_property_port_get(unit, port, spn_PORT_PHY_ID1, 0xFFFF);
    }
    else{
        (void)ext_pc->read(unit, phy_addr, id1_addr, &phy_id1);
    }

    /* Look through table for match */
    for (i = _phys_in_table - 1; i >= 0; i--) {
        if ((phy_table[i]->checkphy)(unit, port, phy_table[i],
                                     phy_id0, phy_id1, pi)) {



            /* Device ID matches. Calls driver probe routine to confirm
             * that the driver is the appropriate one.
             * Many PHY devices has the same device ID but they are
             * actually different.
             */
            rv = PHY_PROBE(phy_table[i]->driver, unit, ext_pc);
            if ((rv == SOC_E_NONE) || (rv == SOC_E_UNAVAIL)) {

                SOC_DEBUG_PRINT((DK_PHY, "<%d> ext Index = %d Mynum = %d %s\n",
                    rv, i, phy_table[i]->myNum, phy_table[i]->phy_name));
            phyd = phy_table[i]->driver;
            pi->phy_id0       = phy_id0;
            pi->phy_id1       = phy_id1;
            pi->phy_addr      = phy_addr;
            if (ext_pc->dev_name) {
                pi->phy_name      = ext_pc->dev_name;
            }
            ext_pc->phy_id0   = phy_id0;
            ext_pc->phy_id1   = phy_id1;
            ext_pc->phy_oui   = PHY_OUI(phy_id0, phy_id1);
            ext_pc->phy_model = PHY_MODEL(phy_id0, phy_id1);
            ext_pc->phy_rev   = PHY_REV(phy_id0, phy_id1);

#ifdef BCM_NORTHSTAR_SUPPORT
            if (SOC_IS_NORTHSTAR(unit)) {
                soc_cm_get_id(unit, &dev_id, &rev_id);        
                ext_pc->phy_rev = rev_id;
            }
#endif /* BCM_NORTHSTAR_SUPPORT */

            PHY_FLAGS_SET(unit, port, PHY_FLAGS_EXTERNAL_PHY);

            break;
            }
        }
    }

#if defined(INCLUDE_PHY_5464_ESW) && defined(INCLUDE_PHY_5464_ROBO)
    
    if (SOC_IS_ROBO(unit) && (phyd == &phy_5464drv_ge)) {
        phyd = &phy_5464robodrv_ge;
    }
#endif /* INCLUDE_PHY_5464_ESW && INCLUDE_PHY_5464_ROBO */

#if defined(INCLUDE_PHY_5482_ESW) && defined(INCLUDE_PHY_5482_ROBO)
    
    if (SOC_IS_ROBO(unit) && (phyd == &phy_5482drv_ge)) {
        phyd = &phy_5482robodrv_ge;
    }
#endif /* INCLUDE_PHY_5464_ESW && INCLUDE_PHY_5464_ROBO */

#if defined(INCLUDE_PHY_54684)
    if (IS_GE_PORT(unit, port) && (phyd == &phy_54684drv_ge)) {
#if defined(BCM_HAWKEYE_SUPPORT)
        if (SOC_IS_HAWKEYE(unit)) {
            switch(port) {
            case 9:
            case 17:
                ext_pc->lane_num = 0;
                break;

            case 10:
            case 18:
                ext_pc->lane_num = 1;
                break;

            case 11:
            case 19:
                ext_pc->lane_num = 2;
                break;

            case 12:
            case 20:
                ext_pc->lane_num = 3;
                break;

            case 13:
            case 21:
                ext_pc->lane_num = 4;
                break;

            case 14:
            case 22:
                ext_pc->lane_num = 5;
                break;

            case 15:
            case 23:
                ext_pc->lane_num = 6;
                break;

            case 16:
            case 24:
                ext_pc->lane_num = 7;
                break;

            default:
                break;

            }
        }
#endif /* SOC_IS_HAWKEYE */
#if defined(BCM_HURRICANE_SUPPORT)
		if (SOC_IS_HURRICANE(unit)) {
			switch(port) {
			case 2:
			case 10:
			case 18:
				ext_pc->lane_num = 0;
				break;

			case 3:
			case 11:
			case 19:
				ext_pc->lane_num = 1;
				break;

			case 4:
			case 12:
			case 20:
				ext_pc->lane_num = 2;
				break;

			case 5:
			case 13:
			case 21:
				ext_pc->lane_num = 3;
				break;

			case 6:
			case 14:
			case 22:
				ext_pc->lane_num = 4;
				break;

			case 7:
			case 15:
			case 23:
				ext_pc->lane_num = 5;
				break;

			case 8:
			case 16:
			case 24:
				ext_pc->lane_num = 6;
				break;

			case 9:
			case 17:
			case 25:
				ext_pc->lane_num = 7;
				break;

			default:
				break;

			}
		}
#endif /* BCM_HURRICANE_SUPPORT */
    }
#endif /* INCLUDE_PHY_54684 */

    ext_pc->pd = phyd;

    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_phy_probe
 * Purpose:
 *      Probe the PHY on the specified port and return a pointer to the
 *      drivers for the device found.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      phyd_ptr - (OUT) Pointer to PHY driver (NULL on error)
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      Loop thru table making callback for each known PHY.
 *      We loop from the table from top to bottom so that user additions
 *      take precedence over default values.  The first checkphy function
 *      returning TRUE is used as the driver.
 */
int
soc_phy_probe(int unit, soc_port_t port, phy_ctrl_t *ext_pc,
              phy_ctrl_t *int_pc)
{
    soc_phy_info_t      *pi;
    uint16               phy_addr;
    uint16               phy_addr_int;
#ifdef BCM_ROBO_SUPPORT
    uint16               phy_id0 = 0, phy_id1 = 0;
    uint32               id0_addr = 0, id1_addr = 0;
#endif /* BCM_ROBO_SUPPORT */

    /* Always use default addresses for probing.
     * This make sure that the external PHY probe works correctly even
     * when the device is hot plugged or the external PHY address is
     * overriden from previous probe.
     */
    SOC_IF_ERROR_RETURN
        (soc_phy_cfg_addr_get(unit,port,0,&phy_addr));
    SOC_IF_ERROR_RETURN
        (soc_phy_cfg_addr_get(unit,port,SOC_PHY_INTERNAL,&phy_addr_int));
    int_pc->phy_id = phy_addr_int;
    ext_pc->phy_id = phy_addr;

    /*
     * Characterize PHY by reading MII registers.
     */
    pi       = &SOC_PHY_INFO(unit, port);

    /* Probe for null PHY configuration first to avoid MII timeouts */
    if (_chk_null(unit, port, &_null_phy_entry, 0xffff, 0xffff, pi)) {
        ext_pc->pd     = _null_phy_entry.driver;
        int_pc->pd     = _null_phy_entry.driver;
    }

    /* Search for internal phy */
    if (NULL == int_pc->pd) {
        SOC_IF_ERROR_RETURN
            (_int_phy_probe(unit, port, pi, int_pc));
    }

    /* Search for external PHY */
    if (NULL == ext_pc->pd) {
        SOC_IF_ERROR_RETURN
            (_ext_phy_probe(unit, port, pi, ext_pc));
    }
    /* Override external PHY driver according to config settings */
    SOC_IF_ERROR_RETURN
        (_forced_phy_probe(unit, port, pi, ext_pc));

#ifdef BCM_ROBO_SUPPORT
    if (SOC_IS_ROBO(unit)) {
        if (IS_GMII_PORT(unit, port) && (ext_pc->pd == NULL)) {
            id0_addr = MII_PHY_ID0_REG;
            id1_addr = MII_PHY_ID1_REG;
            phy_id0 = 0xFFFF;
            phy_id1 = 0xFFFF;
	
            (void)ext_pc->read(unit, phy_addr, id0_addr, &phy_id0);
            (void)ext_pc->read(unit, phy_addr, id1_addr, &phy_id1);

            if ((phy_id0 != 0) && (phy_id0 != 0xFFFF) &&
                (phy_id1 != 0) && (phy_id1 != 0xFFFF) &&
                _chk_default(unit, port, &_default_phy_entry_ge, 0xffff, 0xffff, pi)) {
                ext_pc->pd = _default_phy_entry_ge.driver;
            }
        }
    }
#endif /* BCM_ROBO_SUPPORT */

    if (ext_pc->pd != NULL) {
        if (IS_GMII_PORT(unit, port)) {
            /* If GMII port has external PHY, remove the NULL PHY driver
             * attached to internal PHY in _int_phy_probe().
             */
            int_pc->pd = NULL;
        }
        if ((int_pc->pd == _null_phy_entry.driver) &&
            (ext_pc->pd == _null_phy_entry.driver)) {
            /* Attach NULL PHY driver as external PHY driver */
            int_pc->pd = NULL;
        }
    }

   if ((ext_pc->pd == NULL) && (int_pc->pd == NULL) &&
        _chk_default(unit, port, &_default_phy_entry, 0xffff, 0xffff, pi)) {
        ext_pc->pd = _default_phy_entry.driver;
    }

    assert((ext_pc->pd != NULL) || (int_pc->pd != NULL));

    if (ext_pc->pd == NULL ||        /* No external PHY */
        ext_pc->pd == int_pc->pd) {  /* Forced PHY */
        /* Use internal address when application trying to access
         * external PHY.
         */
        pi->phy_addr = pi->phy_addr_int;

        /* If there is no external PHY, the internal PHY must be in
         * fiber mode.
         */
        if (soc_property_port_get(unit, port,
                                      spn_SERDES_FIBER_PREF, 1)) {
            PHY_FLAGS_SET(unit, port, PHY_FLAGS_FIBER);
        } else {
            PHY_FLAGS_CLR(unit, port, PHY_FLAGS_FIBER);
        }
    }

    /*
     * The property if_tbi_port<X> can be used to force TBI mode on a
     * port.  The individual PHY drivers should key off this flag.
     */
    if (soc_property_port_get(unit, port, spn_IF_TBI, 0)) {
        PHY_FLAGS_SET(unit, port, PHY_FLAGS_10B);
    }

    pi->an_timeout =
        soc_property_port_get(unit, port,
                              spn_PHY_AUTONEG_TIMEOUT, 250000);

    SOC_DEBUG_PRINT((DK_PHY,
                     "soc_phy_probe: port=%d addr=0x%x "
                     "id1=0x%x id0=0x%x flg=0x%x driver=\"%s\"\n",
                     port,
                     pi->phy_addr, pi->phy_id0, pi->phy_id1,
                     pi->phy_flags, pi->phy_name));

    return SOC_E_NONE;
}

#ifdef BCM_CALADAN3_SUPPORT
/*
 * Function:
 *      soc_caladan3_phy_addr_multi_get
 * Purpose:
 *      Provide a list of internal MDIO addresses corresponding to a port.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      array_max - maximum number of elements in mdio_addr array.
 *      array_size - (OUT) number of valid elements returned in mdio_addr.
 *      mdio_addr - (OUT) list of internal mdio addresses for the port.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      Return the list of relevant internal MDIO addresses connected
 *      to a given port.
 *      Currently, only 100G+ ports require multiple MDIO addresses.
 */
STATIC int
soc_sbx_caladan3_phy_addr_multi_get(int unit, soc_port_t port, int array_max,
                       int *array_size, phyident_core_info_t *core_info)
{
    soc_info_t *si;
    int idx = 0;

    si = &SOC_INFO(unit);

    if ((si->port_num_lanes[port] >= 10) || (si->port_type[port] == SOC_BLK_IL)) {
        if (array_max < 3) {
            return SOC_E_PARAM;
        }
        if (soc_sbx_caladan3_is_line_port(unit, port)) {
            core_info[idx].core_type = phyident_core_type_wc;
            core_info[idx].mld_index = 0;
            core_info[idx].index_in_mld = 0;
            core_info[idx].mdio_addr = 0x81;
            idx++;
           
            core_info[idx].core_type = phyident_core_type_wc;
            core_info[idx].mld_index = 0;
            core_info[idx].index_in_mld = 1;
            core_info[idx].mdio_addr = 0xA1;
            idx++;

            core_info[idx].core_type = phyident_core_type_wc;
            core_info[idx].mld_index = 0;
            core_info[idx].index_in_mld = 2;
            core_info[idx].mdio_addr = 0xC1;
            idx++;

            core_info[idx].core_type = phyident_core_type_mld;
            core_info[idx].mld_index = 0;
            core_info[idx].index_in_mld = 0;
            core_info[idx].mdio_addr = 0xD5;
            idx++;
        } else {
            core_info[idx].core_type = phyident_core_type_wc;
            core_info[idx].mld_index = 1;
            core_info[idx].index_in_mld = 0;
            core_info[idx].mdio_addr = 0x95;
            idx++;

            core_info[idx].core_type = phyident_core_type_wc;
            core_info[idx].mld_index = 1;
            core_info[idx].index_in_mld = 1;
            core_info[idx].mdio_addr = 0x99;
            idx++;

            core_info[idx].core_type = phyident_core_type_wc;
            core_info[idx].mld_index = 1;
            core_info[idx].index_in_mld = 2;
            core_info[idx].mdio_addr = 0xB5;
            idx++;

            core_info[idx].core_type = phyident_core_type_mld;
            core_info[idx].mld_index = 1;
            core_info[idx].index_in_mld = 0;
            core_info[idx].mdio_addr = 0xD6;
            idx++;
        }
    } else if (si->port_speed_max[port] == 1000) {
        /* QGMII mode, format is QSMII mdio, WC mdio, QSGMII lane */
        core_info[idx].core_type = phyident_core_type_qsmii;
        core_info[idx].mdio_addr = port_phy_addr[unit][port].int_addr;
        core_info[idx].qsmii_lane = (port % 16) >> 3;
        idx++;

        core_info[idx].core_type = phyident_core_type_wc;
        _bcm88030_wc_phy_addr(unit, port, &(core_info[idx].mdio_addr));
        idx++;

    } else {
        core_info[idx].core_type = phyident_core_type_wc;
        core_info[idx].mdio_addr = port_phy_addr[unit][port].int_addr;
        idx++;
    }

    *array_size = idx;
    return SOC_E_NONE;
}
#endif

void 
phyident_core_info_t_init(phyident_core_info_t* core_info) {
    core_info->mdio_addr = 0xFFFF;
    core_info->core_type = phyident_core_types_count;
    core_info->mld_index = PHYIDENT_INFO_NOT_SET;
    core_info->index_in_mld = PHYIDENT_INFO_NOT_SET;
    core_info->qsmii_lane = PHYIDENT_INFO_NOT_SET;
	core_info->first_phy_in_core = 0;
	core_info->nof_lanes_in_core = 4;
}

/*
 * Function:
 *      soc_phy_addr_multi_get
 * Purpose:
 *      Provide a list of internal MDIO addresses corresponding to a port.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      array_max - maximum number of elements in mdio_addr array.
 *      array_size - (OUT) number of valid elements returned in mdio_addr.
 *      mdio_addr - (OUT) list of internal mdio addresses for the port.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      Return the list of relevant internal MDIO addresses connected
 *      to a given port.
 *      Currently, only 100G+ ports require multiple MDIO addresses.
 */
int
soc_phy_addr_multi_get(int unit, soc_port_t port, int array_max,
                       int *array_size, phyident_core_info_t *core_info)
{
    int addr_num = 0;
    int i;

    if ((0 >= array_max) || (NULL == array_size) || (NULL == core_info)) {
        return SOC_E_PARAM;
    }

    for(i=0 ; i<array_max ; i++) {
        phyident_core_info_t_init(&(core_info[i]));
    }

#ifdef BCM_PETRA_SUPPORT
    if(SOC_IS_DPP(unit)) {
        return _dpp_phy_addr_multi_get(unit, port, 1, array_max, array_size, core_info);
    }
#endif
#ifdef BCM_CALADAN3_SUPPORT
    if (SOC_IS_CALADAN3(unit)) {
        return soc_sbx_caladan3_phy_addr_multi_get(unit, port, array_max, array_size, core_info);
    }
#endif

    if (IS_CL_PORT(unit, port)) {
        if (SOC_IS_TRIUMPH3(unit)) {
            uint32 mld_index = (0 == SOC_BLOCK_NUMBER(unit, SOC_PORT_BLOCK(unit, 
                          SOC_INFO(unit).port_l2p_mapping[port]))) ? 0 : 1;
            if (array_max > 1) {
                core_info[addr_num].mdio_addr = port_phy_addr[unit][port].int_addr;
                core_info[addr_num].core_type = phyident_core_type_wc;
                core_info[addr_num].mld_index = mld_index;
                core_info[addr_num].index_in_mld = 0;
                addr_num++;
            }
            if (array_max > 2) {
                core_info[addr_num].mdio_addr = core_info[addr_num - 1].mdio_addr + 4;
                core_info[addr_num].core_type = phyident_core_type_wc;
                core_info[addr_num].mld_index = mld_index;
                core_info[addr_num].index_in_mld = 1;
                addr_num++;
            }
            if (array_max > 3) {
                core_info[addr_num].mdio_addr = core_info[addr_num - 1].mdio_addr + 4;
                core_info[addr_num].core_type = phyident_core_type_wc;
                core_info[addr_num].mld_index = mld_index;
                core_info[addr_num].index_in_mld = 2;
                addr_num++;
            }
            if (array_max > 4) {
                core_info[addr_num].mdio_addr = mld_index ? 0xde : 0xdd;
                core_info[addr_num].core_type = phyident_core_type_mld;
                core_info[addr_num].mld_index = mld_index;
                core_info[addr_num].index_in_mld = 0;
                addr_num++;
            }
            *array_size = addr_num;
        } else {
            
            return SOC_E_UNAVAIL;
        }
    } else {
        SOC_IF_ERROR_RETURN
            (soc_phy_cfg_addr_get(unit, port, SOC_PHY_INTERNAL,
                                  &(core_info[0].mdio_addr)));
        core_info[0].core_type = phyident_core_type_wc;
        *array_size = 1;
    }
    return SOC_E_NONE;
}

/*
 * Variable:
 *      phy_id_map
 * Purpose:
 *      Map the PHY identifier register (OUI and device ID) into
 *      enumerated PHY type for prototypical devices.
 */

typedef struct phy_id_map_s {
    soc_known_phy_t     phy_num;        /* Enumerated PHY type */
    uint32              oui;            /* Device OUI */
    uint16              model;          /* Device Model */
    uint16              rev_map;        /* Device Revision */
} phy_id_map_t;

#define PHY_REV_ALL   (0xffff)
#define PHY_REV_0_3   (0x000f)
#define PHY_REV_4_7   (PHY_REV_0_3 << 4)
#define PHY_REV_8_11  (PHY_REV_0_3 << 8)
#define PHY_REV_12_15 (PHY_REV_0_3 << 12)
#define PHY_REV_0     (1 << 0)
#define PHY_REV_1     (1 << 1)
#define PHY_REV_2     (1 << 2)
#define PHY_REV_3     (1 << 3)
#define PHY_REV_4     (1 << 4)
#define PHY_REV_5     (1 << 5)
#define PHY_REV_6     (1 << 6)
#define PHY_REV_7     (1 << 7)
#define PHY_REV_8     (1 << 8)
#define PHY_REV_9     (1 << 9)
#define PHY_REV_10    (1 << 10)
#define PHY_REV_11    (1 << 11)
#define PHY_REV_12    (1 << 12)

STATIC phy_id_map_t phy_id_map[] = {
    { _phy_id_BCM5218,      PHY_BCM5218_OUI,        PHY_BCM5218_MODEL,
      PHY_REV_ALL },
    { _phy_id_BCM5220,      PHY_BCM5220_OUI,        PHY_BCM5220_MODEL,
      PHY_REV_ALL }, /* & 5221 */
    { _phy_id_BCM5226,      PHY_BCM5226_OUI,        PHY_BCM5226_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM5228,      PHY_BCM5228_OUI,        PHY_BCM5228_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM5238,      PHY_BCM5238_OUI,        PHY_BCM5238_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM5248,      PHY_BCM5248_OUI,        PHY_BCM5248_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM5324,      PHY_BCM5324_OUI,        PHY_BCM5324_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM5400,      PHY_BCM5400_OUI,        PHY_BCM5400_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM5401,      PHY_BCM5401_OUI,        PHY_BCM5401_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM5402,      PHY_BCM5402_OUI,        PHY_BCM5402_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM5404,      PHY_BCM5404_OUI,        PHY_BCM5404_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM5411,      PHY_BCM5411_OUI,        PHY_BCM5411_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM5421,      PHY_BCM5421_OUI,        PHY_BCM5421_MODEL,
      PHY_REV_ALL}, /* & 5421S */
    { _phy_id_BCM5424,      PHY_BCM5424_OUI,        PHY_BCM5424_MODEL,
      PHY_REV_ALL}, /* & 5434 */
    { _phy_id_BCM5464,      PHY_BCM5464_OUI,        PHY_BCM5464_MODEL,
      PHY_REV_ALL}, /* & 5464S */
    { _phy_id_BCM5466,       PHY_BCM5466_OUI,       PHY_BCM5466_MODEL,
      PHY_REV_ALL}, /* & 5466S */
    { _phy_id_BCM5461,       PHY_BCM5461_OUI,       PHY_BCM5461_MODEL,
      PHY_REV_ALL}, /* & 5461S */
    { _phy_id_BCM5461,       PHY_BCM5462_OUI,       PHY_BCM5462_MODEL,
      PHY_REV_ALL}, /* & 5461D */
    { _phy_id_BCM5478,       PHY_BCM5478_OUI,       PHY_BCM5478_MODEL,
      PHY_REV_ALL}, /*   5478 */
    { _phy_id_BCM5488,       PHY_BCM5488_OUI,       PHY_BCM5488_MODEL,
      PHY_REV_ALL}, /*   5488 */
    { _phy_id_BCM5482,       PHY_BCM5482_OUI,       PHY_BCM5482_MODEL,
      PHY_REV_ALL}, /* & 5482S */
    { _phy_id_BCM5481,       PHY_BCM5481_OUI,       PHY_BCM5481_MODEL,
      PHY_REV_ALL}, /* & 5481 */
    { _phy_id_BCM54980,      PHY_BCM54980_OUI,      PHY_BCM54980_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54980C,     PHY_BCM54980C_OUI,     PHY_BCM54980C_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54980V,     PHY_BCM54980V_OUI,     PHY_BCM54980V_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54980VC,    PHY_BCM54980VC_OUI,    PHY_BCM54980VC_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54680,      PHY_BCM54680_OUI,      PHY_BCM54680_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54880,      PHY_BCM54880_OUI,      PHY_BCM54880_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54880E,     PHY_BCM54880E_OUI,     PHY_BCM54880E_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54680E,     PHY_BCM54680E_OUI,     PHY_BCM54680E_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM52681E,     PHY_BCM52681E_OUI,     PHY_BCM52681E_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54881,      PHY_BCM54881_OUI,      PHY_BCM54881_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54810,      PHY_BCM54810_OUI,      PHY_BCM54810_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54811,      PHY_BCM54811_OUI,      PHY_BCM54811_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54684,      PHY_BCM54684_OUI,      PHY_BCM54684_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54684E,     PHY_BCM54684E_OUI,     PHY_BCM54684E_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54682,      PHY_BCM54682E_OUI,     PHY_BCM54682E_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54685,      PHY_BCM54685_OUI,      PHY_BCM54685_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54640,      PHY_BCM54640_OUI,      PHY_BCM54640_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54616,      PHY_BCM54616_OUI,      PHY_BCM54616_MODEL,
      PHY_REV_ALL},

#ifdef INCLUDE_MACSEC
#if defined(INCLUDE_PHY_54380)
    { _phy_id_BCM54380,      PHY_BCM54380_OUI,      PHY_BCM54380_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54382,      PHY_BCM54380_OUI,      PHY_BCM54382_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54340,      PHY_BCM54380_OUI,      PHY_BCM54340_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54385,      PHY_BCM54380_OUI,      PHY_BCM54385_MODEL,
      PHY_REV_ALL},
#endif
#endif    

#if defined(INCLUDE_PHY_542XX)
    { _phy_id_BCM54280,      PHY_BCM54280_OUI,      PHY_BCM54280_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54282,      PHY_BCM54280_OUI,      PHY_BCM54282_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54240,      PHY_BCM54280_OUI,      PHY_BCM54240_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54285,      PHY_BCM54280_OUI,      PHY_BCM54285_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54290,      PHY_BCM54290_OUI,      PHY_BCM54290_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54292,      PHY_BCM54292_OUI,      PHY_BCM54292_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54294,      PHY_BCM54294_OUI,      PHY_BCM54294_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54295,      PHY_BCM54295_OUI,      PHY_BCM54295_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM5428x,      PHY_BCM5428X_OUI,      PHY_BCM5428X_MODEL,
      PHY_REV_ALL},
#endif

#ifdef INCLUDE_MACSEC
#if defined(INCLUDE_PHY_54580)
    { _phy_id_BCM54584,      PHY_BCM54580_OUI,      PHY_BCM54584_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54580,      PHY_BCM54580_OUI,      PHY_BCM54580_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54540,      PHY_BCM54580_OUI,      PHY_BCM54540_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54585,      PHY_BCM54580_OUI,      PHY_BCM54585_MODEL,
      PHY_REV_ALL},
#endif
#endif /* INCLUDE_MACSEC */
    { _phy_id_BCM8011,       PHY_BCM8011_OUI,       PHY_BCM8011_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM8040,       PHY_BCM8040_OUI,       PHY_BCM8040_MODEL,
      PHY_REV_0 | PHY_REV_1 | PHY_REV_2},
    { _phy_id_BCM8703,       PHY_BCM8703_OUI,       PHY_BCM8703_MODEL,
      PHY_REV_0 | PHY_REV_1 | PHY_REV_2},
    { _phy_id_BCM8704,       PHY_BCM8704_OUI,       PHY_BCM8704_MODEL,
      PHY_REV_3},
    { _phy_id_BCM8705,       PHY_BCM8705_OUI,       PHY_BCM8705_MODEL,
      PHY_REV_4},
    { _phy_id_BCM8706,       PHY_BCM8706_OUI,       PHY_BCM8706_MODEL,
      PHY_REV_5},
    { _phy_id_BCM8750,       PHY_BCM8750_OUI,       PHY_BCM8750_MODEL,
      PHY_REV_0},
    { _phy_id_BCM8752,       PHY_BCM8752_OUI,       PHY_BCM8752_MODEL,
      PHY_REV_0},
    { _phy_id_BCM8754,       PHY_BCM8754_OUI,       PHY_BCM8754_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM8072,       PHY_BCM8072_OUI,       PHY_BCM8072_MODEL,
      PHY_REV_5},
    { _phy_id_BCM8727,       PHY_BCM8727_OUI,       PHY_BCM8727_MODEL,
      PHY_REV_6},
    { _phy_id_BCM8073,       PHY_BCM8073_OUI,       PHY_BCM8073_MODEL,
      PHY_REV_6},
    { _phy_id_BCM8747,       PHY_BCM8747_OUI,       PHY_BCM8747_MODEL,
      PHY_REV_7},
    { _phy_id_BCM84740,      PHY_BCM84740_OUI,      PHY_BCM84740_MODEL,
      PHY_REV_0},
    { _phy_id_BCM84164,      PHY_BCM84164_OUI,      PHY_BCM84164_MODEL,
      PHY_REV_0_3},
    { _phy_id_BCM84758,      PHY_BCM84758_OUI,      PHY_BCM84758_MODEL,
      PHY_REV_0_3},
    { _phy_id_BCM84780,      PHY_BCM84780_OUI,      PHY_BCM84780_MODEL,
      PHY_REV_4_7},
    { _phy_id_BCM84784,      PHY_BCM84784_OUI,      PHY_BCM84784_MODEL,
      PHY_REV_8_11},
#ifdef INCLUDE_MACSEC
#ifdef INCLUDE_PHY_8729
    { _phy_id_BCM8729,       PHY_BCM5927_OUI,       PHY_BCM5927_MODEL,
      PHY_REV_4},
    { _phy_id_BCM8729,       PHY_BCM8729_OUI,       PHY_BCM8729_MODEL,
      PHY_REV_12},
#endif
#ifdef INCLUDE_PHY_84756
    { _phy_id_BCM84756,      PHY_BCM84756_OUI,      PHY_BCM84756_MODEL,
      PHY_REV_0_3},
    { _phy_id_BCM84757,      PHY_BCM84756_OUI,      PHY_BCM84756_MODEL,
      PHY_REV_8_11},
    { _phy_id_BCM84759,      PHY_BCM84756_OUI,      PHY_BCM84756_MODEL,
      PHY_REV_4_7},
#endif
#ifdef INCLUDE_PHY_84334
    { _phy_id_BCM84334,      PHY_BCM84334_OUI,      PHY_BCM84334_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM84336,      PHY_BCM84336_OUI,      PHY_BCM84336_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM84844,      PHY_BCM84844_OUI,      PHY_BCM84844_MODEL,
      PHY_REV_ALL},
#endif
#ifdef INCLUDE_PHY_84749
    { _phy_id_BCM84749,      PHY_BCM84749_OUI,      PHY_BCM84749_MODEL,
      PHY_REV_0},
    { _phy_id_BCM84729,      PHY_BCM84729_OUI,      PHY_BCM84729_MODEL,
      PHY_REV_0},
#endif
#endif  /* INCLUDE_MACSEC */
#if defined(INCLUDE_FCMAP) || defined(INCLUDE_MACSEC)
#ifdef INCLUDE_PHY_84756
    { _phy_id_BCM84756,      PHY_BCM84756_OUI,      PHY_BCM84756_MODEL,
      PHY_REV_0_3},
    { _phy_id_BCM84757,      PHY_BCM84756_OUI,      PHY_BCM84756_MODEL,
      PHY_REV_8_11},
    { _phy_id_BCM84759,      PHY_BCM84756_OUI,      PHY_BCM84756_MODEL,
      PHY_REV_4_7},
#endif  /*INCLUDE_PHY_84756 */
#endif  /* INCLUDE_FCMAP || INCLUDE_MACSEC */
#ifdef INCLUDE_PHY_84728 
    { _phy_id_BCM84707,      PHY_BCM84707_OUI,      PHY_BCM84707_MODEL, 
      PHY_REV_4_7}, 
    { _phy_id_BCM84073,      PHY_BCM84073_OUI,      PHY_BCM84073_MODEL, 
      PHY_REV_8_11}, 
    { _phy_id_BCM84074,      PHY_BCM84074_OUI,      PHY_BCM84074_MODEL, 
      PHY_REV_12_15}, 
    { _phy_id_BCM84728,      PHY_BCM84728_OUI,      PHY_BCM84728_MODEL, 
      PHY_REV_0_3}, 
    { _phy_id_BCM84748,      PHY_BCM84748_OUI,      PHY_BCM84748_MODEL, 
      PHY_REV_4_7}, 
    { _phy_id_BCM84727,      PHY_BCM84727_OUI,      PHY_BCM84727_MODEL, 
      PHY_REV_8_11}, 
    { _phy_id_BCM84747,      PHY_BCM84747_OUI,      PHY_BCM84747_MODEL, 
      PHY_REV_12_15}, 
    { _phy_id_BCM84762,      PHY_BCM84762_OUI,      PHY_BCM84762_MODEL, 
      PHY_REV_0_3}, 
    { _phy_id_BCM84764,      PHY_BCM84764_OUI,      PHY_BCM84764_MODEL, 
      PHY_REV_4_7}, 
    { _phy_id_BCM84042,      PHY_BCM84042_OUI,      PHY_BCM84042_MODEL, 
      PHY_REV_8_11}, 
    { _phy_id_BCM84044,      PHY_BCM84044_OUI,      PHY_BCM84044_MODEL, 
      PHY_REV_12_15}, 
#endif 
    { _phy_id_BCM8481x,      PHY_BCM8481X_OUI,      PHY_BCM8481X_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM84812ce,    PHY_BCM84812CE_OUI,    PHY_BCM84812CE_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM84821,      PHY_BCM84821_OUI,      PHY_BCM84821_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM84822,      PHY_BCM84822_OUI,      PHY_BCM84822_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM84823,      PHY_BCM84823_OUI,      PHY_BCM84823_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM84833,      PHY_BCM84833_OUI,      PHY_BCM84833_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM84834,      PHY_BCM84834_OUI,      PHY_BCM84834_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM84835,      PHY_BCM84835_OUI,      PHY_BCM84835_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM84836,      PHY_BCM84836_OUI,      PHY_BCM84836_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM84844,      PHY_BCM84844_OUI,      PHY_BCM84844_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM84846,      PHY_BCM84846_OUI,      PHY_BCM84846_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM84848,      PHY_BCM84848_OUI,      PHY_BCM84848_MODEL,
      PHY_REV_ALL},
#ifdef INCLUDE_PHY_84328
    { _phy_id_BCM84328,      PHY_BCM84328_OUI,      PHY_BCM84328_MODEL,
      PHY_REV_ALL},
#endif
#ifdef INCLUDE_PHY_84793
    { _phy_id_BCM84793,      PHY_BCM84793_OUI,      PHY_BCM84793_MODEL,
      PHY_REV_ALL},
#endif
#ifdef INCLUDE_PHY_82328
    { _phy_id_BCM82328,      PHY_BCM82328_OUI,      PHY_BCM82328_MODEL,
      PHY_REV_ALL},
#endif

    { _phy_id_BCMXGXS1,      PHY_BCMXGXS1_OUI,      PHY_BCMXGXS1_MODEL,
      PHY_REV_ALL},
    /*
     * HL65 has the same device ID as XGXS1.
     *{ _phy_id_XGXS_HL65,   PHY_XGXS_HL65_OUI,     PHY_XGXS_HL65_MODEL,
     * PHY_REV_ALL }
     */
    { _phy_id_BCMXGXS2,      PHY_BCMXGXS2_OUI,      PHY_BCMXGXS2_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCMXGXS5,      PHY_BCMXGXS5_OUI,      PHY_BCMXGXS5_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCMXGXS6,      PHY_BCMXGXS6_OUI,      PHY_BCMXGXS6_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM5398,       PHY_BCM5398_OUI,       PHY_BCM5398_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM5348,       PHY_BCM5348_OUI,       PHY_BCM5348_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM5395,       PHY_BCM5395_OUI,       PHY_BCM5395_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM53242,      PHY_BCM53242_OUI,     PHY_BCM53242_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM53262,      PHY_BCM53262_OUI,     PHY_BCM53262_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM53115,      PHY_BCM53115_OUI,     PHY_BCM53115_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM53118,      PHY_BCM53118_OUI,     PHY_BCM53118_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM53280,      PHY_BCM53280_OUI,     PHY_BCM53280_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM53101,      PHY_BCM53101_OUI,     PHY_BCM53101_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM53314,      PHY_BCM53314_OUI,     PHY_BCM53314_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM53324,      PHY_BCM53324_OUI,     PHY_BCM53324_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM53125,      PHY_BCM53125_OUI,     PHY_BCM53125_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM53128,      PHY_BCM53128_OUI,     PHY_BCM53128_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM53600,      PHY_BCM53600_OUI,     PHY_BCM53600_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM89500,      PHY_BCM89500_OUI,     PHY_BCM89500_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM53010,      PHY_BCM53010_OUI,     PHY_BCM53010_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM53018,      PHY_BCM53018_OUI,     PHY_BCM53018_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM53020,      PHY_BCM53020_OUI,     PHY_BCM53020_MODEL,
      PHY_REV_ALL},  
    { _phy_id_SERDES100FX,   PHY_SERDES100FX_OUI,   PHY_SERDES100FX_MODEL,
      PHY_REV_4 | PHY_REV_5},
    { _phy_id_SERDES65LP,    PHY_SERDES65LP_OUI,    PHY_SERDES65LP_MODEL,
      PHY_REV_ALL},
    { _phy_id_SERDESCOMBO,   PHY_SERDESCOMBO_OUI,   PHY_SERDESCOMBO_MODEL,
      PHY_REV_8 },
    { _phy_id_XGXS_16G,      PHY_XGXS_16G_OUI,      PHY_XGXS_16G_MODEL,
      PHY_REV_0 },
    { _phy_id_XGXS_TSC,      PHY_XGXS_TSC_OUI,      PHY_XGXS_TSC_MODEL,
      PHY_REV_0 },
};

/*
 * Function:
 *      _phy_ident_type_get
 * Purpose:
 *      Check the PHY ID and return an enumerated value indicating
 *      the PHY.  This looks very redundant, but in the future, more
 *      complicated PHY detection may be necessary.  In addition, the
 *      enum value could be used as an index.
 * Parameters:
 *      phy_id0 - PHY ID register 0 (MII register 2)
 *      phy_id1 - PHY ID register 1 (MII register 3)
 */

static soc_known_phy_t
_phy_ident_type_get(uint16 phy_id0, uint16 phy_id1)
{
    int                 i;
    phy_id_map_t        *pm;
    uint32              oui;
    uint16              model, rev_map;

    oui       = PHY_OUI(phy_id0, phy_id1);
    model     = PHY_MODEL(phy_id0, phy_id1);
    rev_map   = 1 << PHY_REV(phy_id0, phy_id1);

    SOC_DEBUG_PRINT((DK_PHY,
        "phy_id0 = %04x phy_id1 %04x oui = %04x model = %04x rev_map = %04x\n",
        phy_id0, phy_id1, oui, model, rev_map));
    for (i = 0; i < COUNTOF(phy_id_map); i++) {
        pm = &phy_id_map[i];
        if ((pm->oui == oui) && (pm->model == model)) {
            if (pm->rev_map & rev_map) {
                return pm->phy_num;
            }
        }
    }

    return _phy_id_unknown;
}

/*
 * Function:
 *      soc_phy_nocxn
 * Purpose:
 *      Return the no_cxn PHY driver
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      phyd_ptr - (OUT) Pointer to PHY driver.
 * Returns:
 *      SOC_E_XXX
 */

int
soc_phy_nocxn(int unit, phy_driver_t **phyd_ptr)
{
    *phyd_ptr = &phy_nocxn;

    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_phy_info_get
 * Purpose:
 *      Accessor function to copy out PHY info structure
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      pi - (OUT) Pointer to output structure.
 * Returns:
 *      SOC_E_XXX
 */

int
soc_phy_info_get(int unit, soc_port_t port, soc_phy_info_t *pi)
{
    soc_phy_info_t *source;

    source = &SOC_PHY_INFO(unit, port);

    sal_memcpy(pi, source, sizeof(soc_phy_info_t));
    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_phy_an_timeout_get
 * Purpose:
 *      Return autonegotiation timeout for a specific port
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 * Returns:
 *      Timeout in usec
 */

sal_usecs_t
soc_phy_an_timeout_get(int unit, soc_port_t port)
{
    return PHY_AN_TIMEOUT(unit, port);
}

/*
 * Function:
 *      soc_phy_addr_of_port
 * Purpose:
 *      Return PHY ID of the PHY attached to a specified port.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 * Returns:
 *      PHY ID
 */

uint16
soc_phy_addr_of_port(int unit, soc_port_t port)
{
    return PHY_ADDR(unit, port);
}

/*
 * Function:
 *      soc_phy_addr_int_of_port
 * Purpose:
 *      Return PHY ID of a intermediate PHY on specified port.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 * Returns:
 *      PHY ID
 * Notes:
 *      Only applies to chip ports that have an intermediate PHY.
 */

uint16
soc_phy_addr_int_of_port(int unit, soc_port_t port)
{
    return PHY_ADDR_INT(unit, port);
}

/*
 * Function:
 *      soc_phy_cfg_addr_get
 * Purpose:
 *      Get the configured PHY address for this port.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      flag - internal phy or external phy
 *      addr_ptr - hold the retrieved address
 * Returns:
 *      SOC_E_NONE 
 */

int
soc_phy_cfg_addr_get(int unit, soc_port_t port, int flags, uint16 *addr_ptr)
{
    if (flags & SOC_PHY_INTERNAL) {
        *addr_ptr = port_phy_addr[unit][port].int_addr;
    } else {
        *addr_ptr = soc_property_port_get(unit, port,
                                      spn_PORT_PHY_ADDR,
                                      port_phy_addr[unit][port].ext_addr);
    }
    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_phy_cfg_addr_set
 * Purpose:
 *      Configure the port with the given PHY address.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      flag - internal phy or external phy
 *      addr - the address to set
 * Returns:
 *      SOC_E_NONE
 */

int
soc_phy_cfg_addr_set(int unit, soc_port_t port, int flags, uint16 addr)
{
    if (flags & SOC_PHY_INTERNAL) {
        port_phy_addr[unit][port].int_addr = addr;
	if(!((port >= 48 && port <= 55) || (port >= 56 && port <= 63 ))){
	    port_phy_addr[unit][port].ext_addr = addr;
	    soc_cm_debug(DK_ERR, "port:%d, ext_addr=int_addr=addr:%d, SOC_PORT_VALID(unit, 49):%d\n", port,  addr, SOC_PORT_VALID(unit, 49));
	}
    } else {
        port_phy_addr[unit][port].ext_addr = addr;
	if(port >= 48 && port <= 55){
	    if(!SOC_PORT_VALID(unit, 49)) //?
		port_phy_addr[unit][port].ext_addr = port - 48 + 1;
	    else
		port_phy_addr[unit][port].ext_addr = port - 48 + 0;
	}
	if(port >= 56 && port <= 63){
	    if(!SOC_PORT_VALID(unit, 57)) //?
		port_phy_addr[unit][port].ext_addr = port - 56 + 33;
	    else
		port_phy_addr[unit][port].ext_addr = port - 56 + 32;
	}

    }
    soc_cm_debug(DK_ERR, "--int %d, unit:%d,port%d, addr %d\n",
			flags, unit, port, port_phy_addr[unit][port].ext_addr);

    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_phy_addr_to_port
 * Purpose:
 *      Return the port to which a given PHY ID corresponds.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 * Returns:
 *      Port number
 */

soc_port_t
soc_phy_addr_to_port(int unit, uint16 phy_id)
{
    return PHY_ADDR_TO_PORT(unit, phy_id);
}

/*
 * Function:
 *      soc_phy_id1reg_get
 * Purpose:
 *      Return the PHY ID1 field from the PHY on a specified port.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 * Returns:
 *      PHY ID
 */

uint16
soc_phy_id1reg_get(int unit, soc_port_t port)
{
    return PHY_ID1_REG(unit, port);
}

/*
 * Function:
 *      soc_phy_id1reg_get
 * Purpose:
 *      Return the PHY ID0 field from the PHY on a specified port.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 * Returns:
 *      PHY ID
 */

uint16
soc_phy_id0reg_get(int unit, soc_port_t port)
{
    return PHY_ID0_REG(unit, port);
}

/*
 * Function:
 *      soc_phy_is_c45_miim
 * Purpose:
 *      Return TRUE  if Phy uses Clause 45 MIIM, FALSE otherwise
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 * Returns:
 *      Return TRUE  if Phy uses Clause 45 MIIM, FALSE otherwise
 */

int
soc_phy_is_c45_miim(int unit, soc_port_t port)
{
    return PHY_CLAUSE45_MODE(unit, port);
}

/*
 * Function:
 *      soc_phy_name_get
 * Purpose:
 *      Return name of PHY driver corresponding to specified port.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 * Returns:
 *      Static pointer to string.
 */

char *
soc_phy_name_get(int unit, soc_port_t port)
{
    if (PHY_NAME(unit, port) != NULL) {
        return PHY_NAME(unit, port);
    } else {
        return "<Unnamed PHY>";
    }
}

#ifdef BCM_ESW_SUPPORT
#ifdef BROADCOM_DEBUG
/*
 * Function:
 *      soc_phy_dump
 * Purpose:
 *      Display the phy dirvers that are compiled into the driver.
 * Parameters:
 *      unit - StrataSwitch unit #.
 * Returns:
 *      none
 */

void soc_phy_dump(void)
{
    int idx1, idx2;

    for(idx1 = 0; idx1 < _phys_in_table; idx1 += 4) {
        if ( idx1 == 0 ) {
            soc_cm_print("PHYs: ");
        } else {
            soc_cm_print("      ");
        }
        for(idx2 = idx1; idx2 < idx1 + 4 && idx2 < _phys_in_table; idx2++) {
            soc_cm_print("\t%s%s", phy_table[idx2]->phy_name,
                         idx2 < _phys_in_table? "," : "");
        }
        soc_cm_print("\n");
    }

    return;
}
#endif /* BROADCOM_DEBUG */


int
soc_phy_list_get(char *phy_list[],int phy_list_size,int *phys_in_list)
{
    int       idx, base;

    if (phy_list == NULL) {
        return SOC_E_PARAM;
    }

    if (phys_in_list == NULL) {
        return SOC_E_PARAM;
    }

    base = *phys_in_list;

    /* Check invalid param */
    if ((base < 0) || (base > _MAX_PHYS)) {
        return SOC_E_PARAM;
    }

    /* Check invalid table  */
    if ((_phys_in_table < 0) || (_phys_in_table > _MAX_PHYS)) {
        return SOC_E_INTERNAL;
    }

    if ((phy_list_size + base) < _phys_in_table) {
        /*  If phy_list can be completely filled 
           i.e the phy_table has more elements starting from base
           than the size of phy_list */
        *phys_in_list = phy_list_size;
    } else {
        /* If phy_list cannot be completely filled 
           i.e the phy_table has less elements starting from base
           than the size of phy_list*/
        *phys_in_list = _phys_in_table - base;
    }

    /* At this point phys_in_list has the number of items(phys)
       that are going to be set in phy_list starting from the base
       in phy_table[] 
    */
    for (idx = 0; (idx < *phys_in_list) && (idx + base < _MAX_PHYS); idx++) {
        /* coverity[overrun-local : FALSE] */
        phy_list[idx] = phy_table[idx + base]->phy_name;
    }

    /* Returns the status to the calling function if
        there are still more entries in the list */
    if ((idx + base) < _phys_in_table)  {
        return SOC_E_FULL;
    }
    return SOC_E_NONE;
}


#endif /* BCM_ESW_SUPPORT */
