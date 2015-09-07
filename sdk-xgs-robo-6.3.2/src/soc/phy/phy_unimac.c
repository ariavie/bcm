/*
 * $Id: phy_unimac.c 1.4 Broadcom SDK $
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
 */
int phy_unimac_not_empty;
#if defined(INCLUDE_FCMAP) || defined(INCLUDE_MACSEC)
#include "phy_unimac.h"
#include <buser_sal.h>
#include <blmi_err.h>
#include "phy_mac_ctrl.h"


/***************************************************************
 * MACSEC driver functions.
 **************************************************************/
/*
 * Function:     
 *    phy_unimac_mac_init
 * Purpose:    
 *    Initialize the MAC
 * Parameters:
 *    mmc - mac driver control
 *    dev_port - Port number
 * Notes: 
 * The MAC will be put in Auto config mode at initilization
 * MAX Frame Size = 0x3fff
 * IPG = 12
 * TX and RX are enabled
 */
int 
phy_unimac_mac_init(phy_mac_ctrl_t *mmc, int dev_port)
{

     buint32_t macsec_ctrl, cmd_cfg;

    /* Set MAC in Auto Config Mode */
    BLMI_E_IF_ERROR_RETURN(
        phy_unimac_mac_auto_cfg(mmc, dev_port, 1));

    /* Set MAX Frame length to 0x3fff */
    BLMI_E_IF_ERROR_RETURN(
        phy_unimac_mac_max_frame_set(mmc, dev_port, 
                               UNIMAC_REG_DEFAULT_MAX_FRAME_SIZE));

    /* Set IPG to 12 */
    BLMI_E_IF_ERROR_RETURN(
        phy_unimac_mac_update_ipg(mmc, dev_port, 
                            UNIMAC_REG_DEFAULT_IPG));

    /* Enable TX and RX */
    BLMI_E_IF_ERROR_RETURN(
        phy_unimac_mac_enable_set(mmc, dev_port, 1));

    /* Enable the TX CRC corrupt in MACSEC Control register */ 
    BLMI_E_IF_ERROR_RETURN(
        UNIMAC_READ_MACSEC_CNTL_REG(mmc, dev_port,
                                              &macsec_ctrl));
    macsec_ctrl &= ~(UNIMAC_REG_TX_CRC_ENABLE_MASK);
    macsec_ctrl |= (UNIMAC_REG_TX_CRC_ENABLE << 
                UNIMAC_REG_TX_CRC_ENABLE_SHIFT);
    BLMI_E_IF_ERROR_RETURN(
        UNIMAC_WRITE_MACSEC_CNTL_REG(mmc, dev_port, 
                                               macsec_ctrl));

    /* Set terminate CRC in the MAC and Pass MAC control Frames*/
    BLMI_E_IF_ERROR_RETURN(
        UNIMAC_READ_CMD_CFG_REG(mmc, dev_port, &cmd_cfg));
    cmd_cfg &= ~(UNIMAC_REG_CMD_CFG_CRC_FWD_MASK);
    cmd_cfg &= ~(UNIMAC_REG_CMD_CFG_CTL_FRM_ENA_MASK);
    cmd_cfg |= (UNIMAC_REG_CMD_CFG_CRC_FWD_DISABLE << 
                UNIMAC_REG_CMD_CFG_CRC_FWD_SHIFT);
    cmd_cfg |= (1 << UNIMAC_REG_CMD_CTL_FRM_ENA_SHIFT);
    BLMI_E_IF_ERROR_RETURN(
        UNIMAC_WRITE_CMD_CFG_REG(mmc, dev_port, cmd_cfg));

    
    return BLMI_E_NONE;
}

/*
 * Function:     
 *    phy_unimac_mac_auto_cfg
 * Purpose:    
 *    This function puts the MAC in auto config if auto_cfg is 1.
 * Parameters:
 *    mmc - mac driver control
 *    dev_port - Port number
 *    auto_cfg - Auto configuration flag
 *               1 = Auto configuration
 *               0 = Forced mode
 * Notes: 
 */
int 
phy_unimac_mac_auto_cfg(phy_mac_ctrl_t *mmc, int dev_port, int auto_cfg)
{
    buint32_t cmd_reg, val;

    val = (auto_cfg) ? 1 : 0;

    BLMI_E_IF_ERROR_RETURN(
        UNIMAC_READ_CMD_CFG_REG(mmc, dev_port, &cmd_reg));
    cmd_reg &= ~(UNIMAC_REG_CMD_CFG_ENA_EXA_CONFIG_MASK);
    cmd_reg |= (val << UNIMAC_REG_CMD_CFG_ENA_EXA_CONFIG_SHIFT);
    BLMI_E_IF_ERROR_RETURN(
        UNIMAC_WRITE_CMD_CFG_REG(mmc, dev_port, cmd_reg));

    /* Need to sleep for 400ns (or 256 core clock cycles) */
    BMF_SAL_USLEEP(1);

    return BLMI_E_NONE;
}

/*
 */
/*
 * Function:     
 *    phy_unimac_mac_reset
 * Purpose:    
 *    This function puts the MAC in reset if reset is 1 else takes mac
 *    out of reset 
 * Parameters:
 *    mmc - mac driver control
 *    dev_port - Port number
 *    reset    - Reset MAC
 *               1 = Put in Reset
 *               0 = Take out of reset
 * Notes: 
 */
int 
phy_unimac_mac_reset(phy_mac_ctrl_t *mmc, int dev_port, int reset)
{
    buint32_t cmd_reg, val;

    val = (reset) ? 1 : 0;

    BLMI_E_IF_ERROR_RETURN(
        UNIMAC_READ_CMD_CFG_REG(mmc, dev_port, &cmd_reg));
    cmd_reg &= ~(UNIMAC_REG_CMD_CFG_SW_RESET_MASK);
    cmd_reg |= (val << UNIMAC_REG_CMD_CFG_SW_RESET_SHIFT);
    BLMI_E_IF_ERROR_RETURN(
        UNIMAC_WRITE_CMD_CFG_REG(mmc, dev_port, cmd_reg));

    return BLMI_E_NONE;
}

/*
 * Function:     
 *    phy_unimac_mac_enable_set
 * Purpose:    
 *    Enable/Disable MAC
 * Parameters:
 *    mmc - mac driver control
 *    dev_port - Port number
 *    enable   - Enable flag
 *               1 = Enable MAC
 *               0 = Disable MAC
 * Notes: 
 */
int 
phy_unimac_mac_enable_set(phy_mac_ctrl_t *mmc, int dev_port, int enable)
{
    buint32_t cmd_reg, val;

    val = (enable) ? 1 : 0;
    /* Put the mac in reset */
    phy_unimac_mac_reset(mmc, dev_port, 1);

    BLMI_E_IF_ERROR_RETURN(
        UNIMAC_READ_CMD_CFG_REG(mmc, dev_port, &cmd_reg));
    cmd_reg &= ~(UNIMAC_REG_CMD_CFG_TXEN_MASK);
    cmd_reg |= (val << UNIMAC_REG_CMD_CFG_TXEN_SHIFT);
    cmd_reg &= ~(UNIMAC_REG_CMD_CFG_RXEN_MASK);
    cmd_reg |= (val << UNIMAC_REG_CMD_CFG_RXEN_SHIFT);
    BLMI_E_IF_ERROR_RETURN(
        UNIMAC_WRITE_CMD_CFG_REG(mmc, dev_port, cmd_reg));

    /* Take mac out of reset */
    BLMI_E_IF_ERROR_RETURN(
        phy_unimac_mac_reset(mmc, dev_port, 0));

    return BLMI_E_NONE;
}

/*
 * Function:     
 *    phy_unimac_mac_enable_get
 * Purpose:    
 *    Get the enable status of MAC
 * Parameters:
 *    mmc - mac driver control
 *    dev_port - Port number
 *    enable   - (OUT)Enable flag
 *               1 = Enable MAC
 *               0 = Disable MAC
 * Notes: 
 */
int 
phy_unimac_mac_enable_get(phy_mac_ctrl_t *mmc, int dev_port, int *enable) 
{
    buint32_t   cmd_reg, val ;

    BLMI_E_IF_ERROR_RETURN(
        UNIMAC_READ_CMD_CFG_REG(mmc, dev_port, &cmd_reg));
    val = (cmd_reg & UNIMAC_REG_CMD_CFG_TXEN_MASK) >> 
                     UNIMAC_REG_CMD_CFG_TXEN_SHIFT;

    *(enable) = (val) ? 1 : 0;

    return BLMI_E_NONE;
}

/*
 * Function:     
 *    phy_unimac_mac_speed_set
 * Purpose:    
 *    Set Speed of the MAC
 * Parameters:
 *    mmc - mac driver control
 *    dev_port - Port number
 *    speed    - 1000 - 1000Mbps
 *               100  - 100Mbps
 *               10   - 10Mbps
 * Notes: 
 */
int 
phy_unimac_mac_speed_set(phy_mac_ctrl_t *mmc, int dev_port, int speed) 
{
    buint32_t cmd_reg;
    int      speed_select;

    switch (speed) {
    case 0:     /* No change */
        return BLMI_E_NONE;
    case 10:
        speed_select = 0;
	break;
    case 100:
        speed_select = 1;
	break;
    case 1000:
        speed_select = 2;
        break;
    default:
        return (BLMI_E_CONFIG);
    }

    BLMI_E_IF_ERROR_RETURN(
        phy_unimac_mac_reset(mmc, dev_port, 1));

    BLMI_E_IF_ERROR_RETURN(
        UNIMAC_READ_CMD_CFG_REG(mmc, dev_port, &cmd_reg));

    cmd_reg &= ~(UNIMAC_REG_CMD_CFG_SPEED_MASK);
    cmd_reg |= (speed_select << UNIMAC_REG_CMD_CFG_SPEED_SHIFT);

    BLMI_E_IF_ERROR_RETURN(
        UNIMAC_WRITE_CMD_CFG_REG(mmc, dev_port, cmd_reg));

    BLMI_E_IF_ERROR_RETURN(
        phy_unimac_mac_reset(mmc, dev_port, 0));

    return BLMI_E_NONE;
}

/*
 * Function:     
 *    phy_unimac_mac_speed_get
 * Purpose:    
 *    Get Speed of the MAC
 * Parameters:
 *    mmc - mac driver control
 *    dev_port - Port number
 *    speed    - (OUT) 1000 - 1000Mbps
 *                     100  - 100Mbps
 *                     10   - 10Mbps
 * Notes: 
 */
int 
phy_unimac_mac_speed_get(phy_mac_ctrl_t *mmc, int dev_port, int *speed) 
{
    int speed_select;
    buint32_t cmd_reg;

    BLMI_E_IF_ERROR_RETURN(
        UNIMAC_READ_CMD_CFG_REG(mmc, dev_port, &cmd_reg));
    speed_select = (cmd_reg & UNIMAC_REG_CMD_CFG_SPEED_MASK) >>
                   UNIMAC_REG_CMD_CFG_SPEED_SHIFT; 

    switch (speed_select) {
    case 0:
        *speed = 10;
	break;
    case 1:
        *speed = 100;
	break;
    case 2:
        *speed= 1000;
        break;
    default:
        return (BLMI_E_INTERNAL);
    }
    return BLMI_E_NONE;
}


/*
 * Function:     
 *    phy_unimac_mac_duplex_set
 * Purpose:    
 *    Set Duplex of the MAC
 * Parameters:
 *    mmc - mac driver control
 *    dev_port - Port number
 *    duplex   - 1 - Full Duplex
 *               0 - Half Duplex
 * Notes: 
 */
int 
phy_unimac_mac_duplex_set(phy_mac_ctrl_t *mmc, int dev_port, int duplex)
{

    buint32_t cmd_reg;
    int hd_enable, speed, rv = BLMI_E_NONE;

    hd_enable = (duplex) ? 0 : 1; 

    BLMI_E_IF_ERROR_RETURN(
        phy_unimac_mac_reset(mmc, dev_port, 1));

    BLMI_E_IF_ERROR_RETURN(
        phy_unimac_mac_speed_get(mmc, dev_port, &speed));
    if ((speed >= 1000) && (hd_enable == 1)) {
        /* Half duplex with Gigabit speed not supported */
        rv = BLMI_E_PARAM;
        goto error;
    }

    BLMI_E_IF_ERROR_RETURN(
        UNIMAC_READ_CMD_CFG_REG(mmc, dev_port, &cmd_reg));
    cmd_reg &= ~(UNIMAC_REG_CMD_CFG_DUPLEX_MASK);
    cmd_reg |= (hd_enable << UNIMAC_REG_CMD_CFG_DUPLEX_SHIFT);
    BLMI_E_IF_ERROR_RETURN(
        UNIMAC_WRITE_CMD_CFG_REG(mmc, dev_port, cmd_reg));

error:
    BLMI_E_IF_ERROR_RETURN(
        phy_unimac_mac_reset(mmc, dev_port, 0));
    return rv;
}


/*
 * Function:     
 *    phy_unimac_mac_duplex_get
 * Purpose:    
 *    Get Duplex of the MAC
 * Parameters:
 *    mmc - mac driver control
 *    dev_port - Port number
 *    duplex   - (OUT) 1 - Full Duplex
 *                     0 - Half Duplex
 * Notes: 
 */
int 
phy_unimac_mac_duplex_get(phy_mac_ctrl_t *mmc, int dev_port, int *fd) 
{
    buint32_t cmd_reg;
    int      hd_enable;

    BLMI_E_IF_ERROR_RETURN(
        UNIMAC_READ_CMD_CFG_REG(mmc, dev_port, &cmd_reg));
    hd_enable = (cmd_reg & UNIMAC_REG_CMD_CFG_DUPLEX_MASK) >> 
                                    UNIMAC_REG_CMD_CFG_DUPLEX_SHIFT;

    *fd = (hd_enable) ? 0 : 1; 
    return BLMI_E_NONE;
}



/*
 * Function:     
 *    phy_unimac_mac_pause_set
 * Purpose:    
 *    Set pause of the MAC
 * Parameters:
 *    mmc - mac driver control
 *    dev_port - Port number
 *    pause    - 1 - Pause enable
 *               0 - Pause disable
 * Notes: 
 */
int 
phy_unimac_mac_pause_set(phy_mac_ctrl_t *mmc, int dev_port, int pause)
{
    buint32_t cmd_reg;
    int  tx_pause_ignore, rx_pause_ignore;
    int  duplex, rv = BLMI_E_NONE;

    if(pause == 1) {
        tx_pause_ignore = 0; 
        rx_pause_ignore = 0; 
    } else {
        tx_pause_ignore = 1; 
        rx_pause_ignore = 1; 
    }

    BLMI_E_IF_ERROR_RETURN(
        phy_unimac_mac_reset(mmc, dev_port, 1));

    BLMI_E_IF_ERROR_RETURN(
        phy_unimac_mac_duplex_get(mmc, dev_port, &duplex));

    /* handling of PAUSE is differnet in Half duplex more */
    if(duplex == 1) { /* Full Duplex */
        BLMI_E_IF_ERROR_RETURN(
            UNIMAC_READ_CMD_CFG_REG(mmc, dev_port, &cmd_reg));
        cmd_reg &= ~(UNIMAC_REG_CMD_CFG_TX_PAUSE_IGNORE_MASK);
        cmd_reg &= ~(UNIMAC_REG_CMD_CFG_RX_PAUSE_IGNORE_MASK);
        cmd_reg |= (tx_pause_ignore << 
                    UNIMAC_REG_CMD_CFG_TX_PAUSE_IGNORE_SHIFT);
        cmd_reg |= (rx_pause_ignore << 
                    UNIMAC_REG_CMD_CFG_RX_PAUSE_IGNORE_SHIFT);
        BLMI_E_IF_ERROR_RETURN(
            UNIMAC_WRITE_CMD_CFG_REG(mmc, dev_port, cmd_reg));
    } else {
        
    }

    BLMI_E_IF_ERROR_RETURN(
        phy_unimac_mac_reset(mmc, dev_port, 0));
    return rv;
}


/*
 * Function:     
 *    phy_unimac_mac_update_ipg
 * Purpose:    
 *    Set IPG of the MAC
 * Parameters:
 *    mmc - mac driver control
 *    dev_port - Port number
 *    ipg      - IPG value
 * Notes: 
 */
int 
phy_unimac_mac_update_ipg(phy_mac_ctrl_t *mmc, int dev_port, int ipg)
{
    buint32_t ipg_reg;

    if((ipg < 8) || (ipg > 64)) {
        return BLMI_E_PARAM;
    }

    ipg_reg = ipg;
    BLMI_E_IF_ERROR_RETURN(
        UNIMAC_WRITE_TX_IPG_REG(mmc, dev_port, ipg_reg));

    return BLMI_E_NONE;
}


/*
 * Function:     
 *    phy_unimac_mac_max_frame_set
 * Purpose:    
 *    Set Max frame of the MAC
 * Parameters:
 *    mmc  - MAC control structure pointer
 *    dev_port  - Port number
 *    max_frame - Max frame value
 * Notes: 
 */
int 
phy_unimac_mac_max_frame_set(phy_mac_ctrl_t *mmc, int dev_port, 
                           buint32_t max_frame)
{
    /* 
     * Maximum Frame size shouldn;t be less 48 and cannot be greater than
     * 0x3fff
     */
    if((max_frame < 48) || (max_frame > 0x3fff)) {
        return BLMI_E_PARAM;
    }
    BLMI_E_IF_ERROR_RETURN(
        UNIMAC_WRITE_FRM_LEN_REG(mmc, dev_port, max_frame));

    return BLMI_E_NONE;
}


/*
 * Function:     
 *    phy_unimac_mac_max_frame_get
 * Purpose:    
 *    Get Max frame of the MAC
 * Parameters:
 *    mmc  - MAC control structure pointer
 *    dev_port  - Port number
 *    max_frame - (OUT) Max frame
 * Notes: 
 */
int 
phy_unimac_mac_max_frame_get(phy_mac_ctrl_t *mmc, int dev_port, 
                           buint32_t *max_frame) 
{
    buint32_t frame_len;

    BLMI_E_IF_ERROR_RETURN(
        UNIMAC_READ_FRM_LEN_REG(mmc, dev_port, &frame_len));
    *max_frame = frame_len & UNIMAC_REG_FRM_LENr_FMR_LEN_MASK;

    return BLMI_E_NONE;
}

/*
 * Function:     
 *    phy_unimac_mac_pause_frame_fwd
 * Purpose:    
 *    Enable/Disable Pause Frame Forward
 * Parameters:
 *    mmc  - MAC control structure pointer
 *    dev_port  - Port number
 *    pause_fwd - Pause forward 
 *                1 - Forward pause
 *                0 - Consume PAUSE.
 * Notes: 
 */
int 
phy_unimac_mac_pause_frame_fwd(phy_mac_ctrl_t *mmc, int dev_port, 
                             int pause_fwd)
{
    int pause;
    buint32_t cmd_reg;

    if (pause_fwd == 1) {
        pause = 1;
    } else {
        if (pause_fwd == 0) {
            pause = 0;
        } else {
            return BLMI_E_CONFIG;
        }
    }
    BLMI_E_IF_ERROR_RETURN(
        UNIMAC_READ_CMD_CFG_REG(mmc, dev_port, &cmd_reg));
    cmd_reg &= ~(UNIMAC_REG_CMD_CFG_PAUSE_FWD_MASK);
    cmd_reg |= (pause << UNIMAC_REG_CMD_CFG_PAUSE_FWD_SHIFT);
    BLMI_E_IF_ERROR_RETURN(
        UNIMAC_WRITE_CMD_CFG_REG(mmc, dev_port, cmd_reg));

    return BLMI_E_NONE;
}

phy_mac_drv_t phy_unimac_drv = {
    phy_unimac_mac_init,
    phy_unimac_mac_reset,
    phy_unimac_mac_auto_cfg,
    phy_unimac_mac_enable_set,
    phy_unimac_mac_enable_get,
    phy_unimac_mac_speed_set,
    phy_unimac_mac_speed_get,
    phy_unimac_mac_duplex_set,
    phy_unimac_mac_duplex_get,
    phy_unimac_mac_update_ipg,
    phy_unimac_mac_max_frame_set,
    phy_unimac_mac_max_frame_get,
    phy_unimac_mac_pause_set,
    NULL,
    phy_unimac_mac_pause_frame_fwd,
};

#endif /* defined(INCLUDE_FCMAP) || defined (INCLUDE_MACSEC) */
