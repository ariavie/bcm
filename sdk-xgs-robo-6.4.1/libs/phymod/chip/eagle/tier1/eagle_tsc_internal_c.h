/***********************************************************************************/
/***********************************************************************************/
/*  File Name     :  eagle_tsc_internal.c                                         */
/*  Created On    :  13/02/2014                                                    */
/*  Created By    :  Justin Gaither                                                */
/*  Description   :  APIs for Serdes IPs                                           */
/*  Revision      :  $Id: eagle_tsc_internal.c 535 2014-05-23 23:10:44Z kirand $ */
/*                                                                                 */
/*  Copyright: (c) 2014 Broadcom Corporation All Rights Reserved.                  */
/*  All Rights Reserved                                                            */
/*  No portions of this material may be reproduced in any form without             */
/*  the written permission of:                                                     */
/*      Broadcom Corporation                                                       */
/*      5300 California Avenue                                                     */
/*      Irvine, CA  92617                                                          */
/*                                                                                 */
/*  All information contained in this document is Broadcom Corporation             */
/*  company private proprietary, and trade secret.                                 */
/*                                                                                 */
/***********************************************************************************/
/***********************************************************************************/
/*
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
 *  $Id: 54fea590f13f33075f9225be7ee52ec49c80b4b4 $
*/


/** @file eagle_tsc_internal.c
 * Implementation of API functions
 */

#include "eagle_tsc_internal.h"

SDK_STATIC uint32_t _mult_with_overflow_check(uint32_t a, uint32_t b, uint8_t *of) {
    uint16_t c,d;
    uint32_t r,s;
    if (a > b) return _mult_with_overflow_check(b, a, of);
    *of = 0;
    c = b >> 16;
    d = UINT16_MAX & b;
    r = a * c;
    s = a * d;
    if (r > UINT16_MAX) *of = 1;
    r <<= 16;
    return (s + r);
}

  
  

SDK_STATIC err_code_t _get_p1_threshold( const phymod_access_t *pa, int8_t *val) {
    ESTM(*val = rd_p1_eyediag_bin()); 
 return (ERR_CODE_NONE);
}


/* Setup the P1 slicer vertical level  */
SDK_STATIC err_code_t _set_p1_threshold( const phymod_access_t *pa, int8_t threshold) {

  ESTM(_check_uc_lane_stopped( pa ));                     /* make sure uC is stopped to avoid race conditions */

  wr_dfe_vga_write_tapsel(0xd);                       /* Configure dfe_vga_write_tapsel to p1_eyediag mode  */
  wr_dfe_vga_write_val((threshold)<<3);               /* dfe_vga_write_val[8:3] are used to drive the analog control port. */
  wr_dfe_vga_write_en(0x1);                           /* Enable f/w to write the tap values */
  return (ERR_CODE_NONE);
}

SDK_STATIC err_code_t _move_clkp1_offset( const phymod_access_t *pa, int8_t delta) {
  int8_t cnt;

  ESTM(_check_uc_lane_stopped( pa ));                     /* make sure uC is stopped to avoid race conditions */

  wr_rx_pi_slicers_en(0x2);                           /* Select p1 slicer to adjust */
  wr_rx_pi_phase_step_dir(delta>0);                   /* 1 for positive step   */
  wr_rx_pi_phase_step_cnt(1);                         /* Step set to 1 */
  for (cnt=0; cnt < _abs(delta); cnt++) {
    wr_rx_pi_manual_strobe(1);                        /* Increments/Decrements by 1 every strobe */
  } 
  return(ERR_CODE_NONE);
}

SDK_STATIC int16_t _ladder_setting_to_mV(const phymod_access_t *pa,int8_t ctrl, uint8_t range_250) {
    int8_t absv;                                      /* Absolute value of ctrl */
    int16_t nlmv;                                     /* Non-linear value */
    
    /* Get absolute value */
    absv = _abs(ctrl);

    {
    int16_t nlv;                                      /* Non-linear value */

    /* Convert to linear scale from non-linear*/
    /* First 22 steps are 6.25mV, last 9 steps are 12.5mV   */
    /* store in x4 units (25mV/4) */
    nlv = 25*absv + (absv-22)*(absv>22)*25;     

    if (range_250) {
        /* 250mV range, x/4 to get mV units */
        nlmv = (nlv+2)/4;
    } else {
        /* 150mV range, x*3/20 */
        /* Multiply by 0.6 and add 0.5 for rounding */
        nlmv = (nlv*3+10)/20;
    }
    }
    /* Add sign and return */
    return( (ctrl>=0) ? nlmv : -nlmv);

}


static err_code_t _compute_bin( const phymod_access_t *pa, char var, char bin[]) {

  switch (var) {
    case '0':  USR_STRCPY(bin,"0000");
               break;
    case '1':  USR_STRCPY(bin,"0001");
               break;
    case '2':  USR_STRCPY(bin,"0010");
               break;
    case '3':  USR_STRCPY(bin,"0011");
               break;
    case '4':  USR_STRCPY(bin,"0100");
               break;
    case '5':  USR_STRCPY(bin,"0101");
               break;
    case '6':  USR_STRCPY(bin,"0110");
               break;
    case '7':  USR_STRCPY(bin,"0111");
               break;
    case '8':  USR_STRCPY(bin,"1000");
               break;
    case '9':  USR_STRCPY(bin,"1001");
               break;
    case 'a':
    case 'A':  USR_STRCPY(bin,"1010");
               break;
    case 'b':
    case 'B':  USR_STRCPY(bin,"1011");
               break;
    case 'c':
    case 'C':  USR_STRCPY(bin,"1100");
               break;
    case 'd':
    case 'D':  USR_STRCPY(bin,"1101");
               break;
    case 'e':
    case 'E':  USR_STRCPY(bin,"1110");
               break;
    case 'f':
    case 'F':  USR_STRCPY(bin,"1111");
               break;
    case '_':  USR_STRCPY(bin,"");
               break;
    default :  USR_STRCPY(bin,"");
               USR_PRINTF(("ERROR: Invalid Hexadecimal Pattern\n"));
               return (_error(ERR_CODE_CFG_PATT_INVALID_HEX));
               break;
  }
  return (ERR_CODE_NONE);
}


static err_code_t _compute_hex( const phymod_access_t *pa, char bin[],uint8_t *hex) {
	if(!hex) {
		return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
	}

 
  if (!USR_STRCMP(bin,"0000")) {
    *hex = 0x0;
  } 
  else if (!USR_STRCMP(bin,"0001")) {
    *hex = 0x1;
  } 
  else if (!USR_STRCMP(bin,"0010")) {
    *hex = 0x2;
  } 
  else if (!USR_STRCMP(bin,"0011")) {
    *hex = 0x3;
  } 
  else if (!USR_STRCMP(bin,"0100")) {
    *hex = 0x4;
  } 
  else if (!USR_STRCMP(bin,"0101")) {
    *hex = 0x5;
  } 
  else if (!USR_STRCMP(bin,"0110")) {
    *hex = 0x6;
  } 
  else if (!USR_STRCMP(bin,"0111")) {
    *hex = 0x7;
  } 
  else if (!USR_STRCMP(bin,"1000")) {
    *hex = 0x8;
  } 
  else if (!USR_STRCMP(bin,"1001")) {
    *hex = 0x9;
  } 
  else if (!USR_STRCMP(bin,"1010")) {
    *hex = 0xA;
  } 
  else if (!USR_STRCMP(bin,"1011")) {
    *hex = 0xB;
  } 
  else if (!USR_STRCMP(bin,"1100")) {
    *hex = 0xC;
  } 
  else if (!USR_STRCMP(bin,"1101")) {
    *hex = 0xD;
  } 
  else if (!USR_STRCMP(bin,"1110")) {
    *hex = 0xE;
  } 
  else if (!USR_STRCMP(bin,"1111")) {
    *hex = 0xF;
  } 
  else {
    USR_PRINTF(("ERROR: Invalid Binary to Hex Conversion\n"));
    *hex = 0x0;
	return (_error(ERR_CODE_CFG_PATT_INVALID_BIN2HEX));

  } 
  return (ERR_CODE_NONE);
}

  

/*************************************************/
/*  RAM access through Micro Register Interface  */
/*************************************************/

 
err_code_t _eagle_display_event(const phymod_access_t *pa,uint8_t event_id,
	                uint8_t entry_len,
				    uint8_t prev_cursor,
				    uint8_t curr_cursor,
				    uint8_t post_cursor) {
        char *s1, *s2, *s3;
	switch (event_id) {                                                       /* decode event code */
	case EVENT_CODE_ENTRY_TO_DSC_RESET:
		USR_PRINTF(("  Entry to DSC reset\n"));
		break;
	case EVENT_CODE_RELEASE_USER_RESET:
		USR_PRINTF(("  Release user reset\n"));
		break;
	case EVENT_CODE_EXIT_FROM_DSC_RESET:
		USR_PRINTF(("  Exit from DSC reset\n"));
		break;
	case EVENT_CODE_ENTRY_TO_CORE_RESET:
		USR_PRINTF(("  Entry to core reset\n"));
		break;
	case EVENT_CODE_RELEASE_USER_CORE_RESET:
		USR_PRINTF(("  Release user core reset\n"));
		break;
	case EVENT_CODE_ACTIVE_RESTART_CONDITION:
		USR_PRINTF(("  Active restart condition\n"));
		break;
	case EVENT_CODE_EXIT_FROM_RESTART:
		USR_PRINTF(("  Exit from restart\n"));
		break;
	case EVENT_CODE_WRITE_TR_COARSE_LOCK:
		USR_PRINTF(("  Write transmit coarse lock\n"));
		break;
	case EVENT_CODE_CL72_READY_FOR_COMMAND:
		s1 = _status_val_2_str( pa, prev_cursor);
		s2 = _status_val_2_str( pa, curr_cursor);
		s3 = _status_val_2_str( pa, post_cursor);
		if (entry_len == 4) {
			USR_PRINTF(("  Cl72 ready for command\n"));
		} else {
			USR_PRINTF(("  Cl72 ready for command, prev command returned (%s, %s, %s)\n", s1, s2, s3 ));
		}
		break;
	case EVENT_CODE_EACH_WRITE_TO_CL72_TX_CHANGE_REQUEST:
		s1 = _update_val_2_str( pa, prev_cursor);
		s2 = _update_val_2_str( pa, curr_cursor);
		s3 = _update_val_2_str( pa, post_cursor);
		if (entry_len == 4) {
			USR_PRINTF(("  Write to Cl72 transmit change request\n"));
		} else {
			USR_PRINTF(("  Write to Cl72 transmit change request (%s, %s, %s)\n", s1, s2, s3 ));
		}
		break;
	case EVENT_CODE_REMOTE_RX_READY:
		USR_PRINTF(("  Remote Rx ready\n"));
		break;
	case EVENT_CODE_LOCAL_RX_TRAINED:
		USR_PRINTF(("  Local Rx trained\n"));
		break;
	case EVENT_CODE_DSC_LOCK:
		USR_PRINTF(("  DSC lock\n"));
		break;
	case EVENT_CODE_FIRST_RX_PMD_LOCK:
		USR_PRINTF(("  Rx PMD lock\n"));
		break;
	case EVENT_CODE_PMD_RESTART_FROM_CL72_CMD_INTF_TIMEOUT:
		USR_PRINTF(("  PMD restart due to CL72 ready for command timeout\n"));
		break;
	case EVENT_CODE_LP_RX_READY:
		USR_PRINTF(("  Remote receiver ready in CL72\n"));
		break;
	case EVENT_CODE_STOP_EVENT_LOG:
		USR_PRINTF(("  Start reading event log\n"));
		break;
	case EVENT_CODE_GENERAL_EVENT_0:
		USR_PRINTF(("  General event 0, (0x%x%x)\n",post_cursor,prev_cursor));
		break;
	case EVENT_CODE_GENERAL_EVENT_1:
		USR_PRINTF(("  General event 1, (0x%x%x)\n",post_cursor,prev_cursor));
		break;
	case EVENT_CODE_GENERAL_EVENT_2:
		USR_PRINTF(("  General event 2, (0x%x%x)\n",post_cursor,prev_cursor));
		break;
	default:
		USR_PRINTF(("  UNRECOGNIZED EVENT CODE !!!\n\n"));
		break;
	}
  return(ERR_CODE_NONE);
}


static char* _status_val_2_str( const phymod_access_t *pa, uint8_t val) {
	switch (val) {
	case 3:
		return ("MAX");
	case 2:
		return ("MIN");
	case 1:
		return ("UPDATED");
	case 0:
		return ("NOT_UPDATED");
	default:
		return ("UNDEFINED");
	}
}

static char* _update_val_2_str( const phymod_access_t *pa, uint8_t val) {
	switch (val) {
	case 2:
		return ("DEC");
	case 1:
		return ("INC");
	case 0:
		return ("HOLD");
	default:
		return ("UNDEFINED");
	}
}


#ifdef TO_FLOATS
/* convert uint32_t to float8 */
static float8_t _int32_to_float8(uint32_t input) {
	int8_t cnt;
	uint8_t output;

	if(input == 0) {
		return(0);
	} else if(input == 1) {
		return(0xe0);
	} else {
		cnt = 0;
		input = input & 0x7FFFFFFF; /* get rid of MSB which may be lock indicator */
		do {
			input = input << 1;
			cnt++;
		} while ((input & 0x80000000) == 0);

		output = (uint8_t)((input & 0x70000000)>>23)+(31 - cnt%32);
			return(output);
	}
}
#endif

/* convert float8 to uint32_t */
static uint32_t _float8_to_int32(float8_t input) {
	uint32_t x;
	if(input == 0) return(0);
	x = (input>>5) + 8;
	if((input & 0x1F) < 3) {
		return(x>>(3-(input & 0x1f)));
	} 
	return(x<<((input & 0x1F)-3));
}

/* convert float12 to uint32 */
static uint32_t _float12_to_uint32( const phymod_access_t *pa, uint8_t byte, uint8_t multi) {

	return(((uint32_t)byte)<<multi);
}

#ifdef TO_FLOATS
/* convert uint32 to float12 */
static uint8_t _uint32_to_float12( const phymod_access_t *pa, uint32_t input, uint8_t *multi) {
	int8_t cnt;
	uint8_t output;
	if(!multi) {
		return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
	}

	if((input == 0) || (!multi)) {
		*multi = 0;
		return(0);
	} else {
		cnt = 0;
		if(input > 0x007FFFFF) input = 0x007FFFFF; /* limit to 23bits so multi is 4 bits */
		do {
			input = input << 1;
			cnt++;
		} while ((input & 0x80000000) == 0);

		*multi = (31 - (cnt%32));
		if(*multi < 8) {
			output = (uint8_t)((input & 0xFF000000)>>(24 + (7-*multi)));
			*multi = 0;
		} else {
			output = (uint8_t)((input & 0xFF000000)>>24);
			*multi = *multi - 7;
		}
		return(output);
	}
}
#endif

static err_code_t _set_rx_pf_main( const phymod_access_t *pa, uint8_t val) {
	if (val > 15) {
		return (_error(ERR_CODE_PF_INVALID));
	}
	wr_pf_ctrl(val); 
	return(ERR_CODE_NONE); 
}

static err_code_t _get_rx_pf_main( const phymod_access_t *pa, int8_t *val) {
		ESTM(*val = (int8_t)rd_pf_ctrl()); 
 return (ERR_CODE_NONE);
}

static err_code_t _set_rx_pf2( const phymod_access_t *pa, uint8_t val) {
	if (val > 7) {
		return (_error(ERR_CODE_PF_INVALID));
	}
	wr_pf2_lowp_ctrl(val);
	return(ERR_CODE_NONE); 
}

static err_code_t _get_rx_pf2( const phymod_access_t *pa, int8_t *val) {
		ESTM(*val = (int8_t)rd_pf2_lowp_ctrl()); 
 return (ERR_CODE_NONE);
}

static err_code_t _set_rx_vga( const phymod_access_t *pa, uint8_t val) {

    ESTM(_check_uc_lane_stopped( pa ));                     /* make sure uC is stopped to avoid race conditions */

	if (val > 45) {
		return (_error(ERR_CODE_VGA_INVALID));
	} 
	wr_dfe_vga_write_tapsel(0);                         /* Configure dfe_vga_write_tapsel to VGA */
	wr_dfe_vga_write_val(val<<3);                       /* dfe_vga_write_val[8:3] are used to drive the analog control port */
	wr_dfe_vga_write_en(1);
	return(ERR_CODE_NONE); 
}

static err_code_t _get_rx_vga( const phymod_access_t *pa, int8_t *val) {
		ESTM(*val = (int8_t)rd_vga_bin()); 
 return (ERR_CODE_NONE);
}


static err_code_t _set_rx_dfe1( const phymod_access_t *pa, int8_t val) {

    ESTM(_check_uc_lane_stopped( pa ));                     /* make sure uC is stopped to avoid race conditions */

    {
        int8_t tap_eo;

        if (val > 63) {
            return (_error(ERR_CODE_DFE1_INVALID));  
        }
        /* Compute tap1 even/odd component */
        tap_eo = 0;
        wr_dfe_vga_write_tapsel(2);                         /* Write tap1_odd */
        wr_dfe_vga_write_val(tap_eo);
        wr_dfe_vga_write_en(1);
        wr_dfe_vga_write_tapsel(3);                         /* Write tap1_even */
        wr_dfe_vga_write_val(tap_eo);
        wr_dfe_vga_write_en(1);
        wr_dfe_vga_write_tapsel(1);
        wr_dfe_vga_write_val(val-tap_eo);                 /* Write the common tap */
        wr_dfe_vga_write_en(1);
    }
	return(ERR_CODE_NONE); 
}
static err_code_t _get_rx_dfe1( const phymod_access_t *pa, int8_t *val) {
	ESTM(*val = (int8_t)(rd_dfe_1_e() + rd_dfe_1_cmn()));
 return (ERR_CODE_NONE);
}


static err_code_t _set_rx_dfe2( const phymod_access_t *pa, int8_t val) {
	int8_t tap_eo;

	if ((val > 31) || (val < -31)) {
		return (_error(ERR_CODE_DFE2_INVALID));  
	}

    ESTM(_check_uc_lane_stopped( pa ));                     /* make sure uC is stopped to avoid race conditions */

	/* Compute tap2 odd/even component */
	tap_eo = (_abs(val) > 31) ? (_abs(val)-31) : 0;
	wr_dfe_vga_write_tapsel(5);                         /* Write tap2_odd */
	wr_dfe_vga_write_val(tap_eo);
	wr_dfe_vga_write_en(1);
	wr_dfe_vga_write_tapsel(6);                         /* Write tap2_even */
	wr_dfe_vga_write_val(tap_eo);
	wr_dfe_vga_write_en(1);
	wr_dfe_vga_write_tapsel(4);
	wr_dfe_vga_write_val(_abs(val)-tap_eo);            /* Write the common tap */
	wr_dfe_vga_write_en(1);
	wr_dfe_vga_write_tapsel(10);                        /* Write tap2_even_sign */
	wr_dfe_vga_write_val(val<0?1:0);
	wr_dfe_vga_write_en(1);
	wr_dfe_vga_write_tapsel(11);                        /* Write tap2_odd_sign */
	wr_dfe_vga_write_val(val<0?1:0);
	wr_dfe_vga_write_en(1);
	return(ERR_CODE_NONE); 
}

static err_code_t _get_rx_dfe2( const phymod_access_t *pa, int8_t *val) {
	ESTM(*val = rd_dfe_2_se() ? -(rd_dfe_2_e()+rd_dfe_2_cmn()) : (rd_dfe_2_e()+rd_dfe_2_cmn()));
 return (ERR_CODE_NONE);
}

static err_code_t _set_rx_dfe3( const phymod_access_t *pa, int8_t val) {
	if ((val > 31) || (val < -31)) {
		return (_error(ERR_CODE_DFE3_INVALID));  
	}

    ESTM(_check_uc_lane_stopped( pa ));                     /* make sure uC is stopped to avoid race conditions */

	wr_dfe_vga_write_tapsel(7);                         /* Write tap3 */
	wr_dfe_vga_write_val(val);
	wr_dfe_vga_write_en(1);
	return(ERR_CODE_NONE); 
}

static err_code_t _get_rx_dfe3( const phymod_access_t *pa, int8_t *val) {
	ESTM(*val = rd_dfe_3_cmn());
 return (ERR_CODE_NONE);
}

static err_code_t _set_rx_dfe4( const phymod_access_t *pa, int8_t val) {
	if ((val > 15) || (val < -15)) {
		return (_error(ERR_CODE_DFE4_INVALID));  
	}
	
    ESTM(_check_uc_lane_stopped( pa ));                     /* make sure uC is stopped to avoid race conditions */
    
	wr_dfe_vga_write_tapsel(8);                         /* Write tap4 */
	wr_dfe_vga_write_val(val);
	wr_dfe_vga_write_en(1);
	return(ERR_CODE_NONE); 
}

static err_code_t _get_rx_dfe4( const phymod_access_t *pa, int8_t *val) {
	ESTM(*val = rd_dfe_4_cmn());
 return (ERR_CODE_NONE);
}

static err_code_t _set_rx_dfe5( const phymod_access_t *pa, int8_t val) {
	if ((val > 15) || (val < -15)) {
		return (_error(ERR_CODE_DFE5_INVALID));  
	}
	
    ESTM(_check_uc_lane_stopped( pa ));                     /* make sure uC is stopped to avoid race conditions */
    
	wr_dfe_vga_write_tapsel(9);                         /* Write tap5 */
	wr_dfe_vga_write_val(val);
	wr_dfe_vga_write_en(1); 
	return(ERR_CODE_NONE); 
}

static err_code_t _get_rx_dfe5( const phymod_access_t *pa, int8_t *val) {
	ESTM(*val = rd_dfe_5_cmn());
 return (ERR_CODE_NONE);
}


static err_code_t _set_tx_post1( const phymod_access_t *pa, uint8_t val) {

  if (val > 63) {
    return (_error(ERR_CODE_TXFIR_POST1_INVALID));  
  }
  wr_txfir_post_override(val); 

  return(ERR_CODE_NONE); 
}


static err_code_t _set_tx_pre( const phymod_access_t *pa, uint8_t val) {

  if (val > 31) {
    return (_error(ERR_CODE_TXFIR_PRE_INVALID));  
  }
  wr_txfir_pre_override(val); 
  return(ERR_CODE_NONE); 
}


static err_code_t _set_tx_main( const phymod_access_t *pa, uint8_t val) {

  if (val > 112) {

    return (_error(ERR_CODE_TXFIR_MAIN_INVALID));  
  }
  wr_txfir_main_override(val); 
  return(ERR_CODE_NONE); 
}

static err_code_t _set_tx_post2( const phymod_access_t *pa, int8_t val) {

  if ((val > 15) || (val < -15)) {
    return (_error(ERR_CODE_TXFIR_POST2_INVALID));  
  }
  /* sign? */
  wr_txfir_post2(val); 

  return(ERR_CODE_NONE); 
}


static err_code_t _get_tx_pre( const phymod_access_t *pa, int8_t *val) {
	ESTM(*val = rd_txfir_pre_after_ovr());
 return (ERR_CODE_NONE);
}

static err_code_t _get_tx_main( const phymod_access_t *pa, int8_t *val) {
	ESTM(*val = rd_txfir_main_after_ovr());
 return (ERR_CODE_NONE);
}

static err_code_t _get_tx_post1( const phymod_access_t *pa, int8_t *val) {
	ESTM(*val = rd_txfir_post_after_ovr());
 return (ERR_CODE_NONE);
}

static err_code_t _get_tx_post2( const phymod_access_t *pa, int8_t *val) {

	ESTM(*val = rd_txfir_post2_adjusted());
 return (ERR_CODE_NONE);
}

static err_code_t _set_tx_post3( const phymod_access_t *pa, int8_t val) {

  if ((val > 7) || (val < -7)) {
    return (_error(ERR_CODE_TXFIR_POST3_INVALID));  
  }
  /* sign? */
  wr_txfir_post3(val); 

  return(ERR_CODE_NONE); 
}

static err_code_t _get_tx_post3( const phymod_access_t *pa, int8_t *val) {

	ESTM(*val = rd_txfir_post3_adjusted());
  return(ERR_CODE_NONE); 
}

static err_code_t _set_tx_amp( const phymod_access_t *pa, int8_t val) {

 if (val > 15) {
    return (_error(ERR_CODE_TX_AMP_CTRL_INVALID));
 }

 wr_ams_tx_amp_ctl(val);
  return(ERR_CODE_NONE); 
}

static err_code_t _get_tx_amp( const phymod_access_t *pa, int8_t *val) {
	ESTM(*val = rd_ams_tx_amp_ctl());
  return(ERR_CODE_NONE); 
}



static err_code_t _eagle_tsc_core_clkgate( const phymod_access_t *pa, uint8_t enable) {

  if (enable) {
      wrc_pmd_tx_clk_vld_frc_val(0x0);
      wrc_pmd_tx_clk_vld_frc(0x1);
      wrc_tx_s_clkgate_frc_on(0x1);
  }
  else {
      wrc_pmd_tx_clk_vld_frc_val(0x0);
      wrc_pmd_tx_clk_vld_frc(0x0);
      wrc_tx_s_clkgate_frc_on(0x0);
  }
  return (ERR_CODE_NONE);
}

static err_code_t _eagle_tsc_lane_clkgate( const phymod_access_t *pa, uint8_t enable) {

  if (enable) {
    /* Use frc/frc_val to force all RX and TX clk_vld signals to 0 */
    wr_pmd_rx_clk_vld_frc_val(0x0);
    wr_pmd_rx_clk_vld_frc(0x1);

      /* Use frc_on to force clkgate */
      wr_ln_rx_s_clkgate_frc_on(0x1);
    
  }
  else {
    wr_pmd_rx_clk_vld_frc_val(0x0);
    wr_pmd_rx_clk_vld_frc(0x0);

      wr_ln_rx_s_clkgate_frc_on(0x0);
  }
  return (ERR_CODE_NONE);
}

SDK_STATIC uint16_t _eye_to_mUI(const phymod_access_t *pa,uint8_t var) 
{   
    /* var is in units of 1/512 th UI, so need to multiply by 1000/512 */
    return ((uint16_t)var)*125/64;
}


SDK_STATIC uint16_t _eye_to_mV(const phymod_access_t *pa, uint8_t var, uint8_t ladder_range) 
{       
    /* find nearest two vertical levels */
    uint16_t vl = _ladder_setting_to_mV(pa,var/8, ladder_range);    
    uint16_t vu = _ladder_setting_to_mV(pa,_min(31,var/8+1), ladder_range);
    return (vl + (vu-vl)*(var&7)/8);
}

SDK_STATIC err_code_t _eagle_tsc_read_lane_state( const phymod_access_t *pa, eagle_tsc_lane_state_st *istate) {  
       
  uint8_t is_micro_stopped;
  eagle_tsc_lane_state_st state = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  uint8_t ladder_range;

  if(!istate) 
	  return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  
  ESTM(is_micro_stopped = rdv_usr_sts_micro_stopped());

  if (!is_micro_stopped)
         EFUN(eagle_tsc_stop_rx_adaptation( pa, 1));
  
  ESTM(state.osr_mode = rd_osr_mode());
  ESTM(state.ucv_config = rdv_config_word());
  ESTM(state.sig_det = rd_signal_detect());
  ESTM(state.rx_lock = rd_pmd_rx_lock());
  ESTM(state.rx_ppm = rd_cdr_integ_reg()/84);
  ESTM(state.clk90 = rd_cnt_d_minus_m1());
  ESTM(state.clkp1 = rd_cnt_d_minus_p1());
  ESTM(state.br_pd_en = rd_br_pd_en());
  /* drop the MSB, the result is actually valid modulo 128 */
  /* Also flip the sign to account for d-m1, versus m1-d */
  state.clk90 = state.clk90<<1;
  state.clk90 = -(state.clk90>>1);
  state.clkp1 = state.clkp1<<1;
  state.clkp1 = -(state.clkp1>>1);
  
  EFUN(_get_rx_pf_main( pa, &state.pf_main));
  ESTM(state.pf_hiz = rd_pf_hiz());
  ESTM(state.pf_low = rd_pf2_lowp_ctrl());
  EFUN(_get_rx_vga( pa, &state.vga));
  ESTM(state.dc_offset = rd_dc_offset());
  ESTM(ladder_range = rd_p1_thresh_sel());
  EFUN(_get_p1_threshold( pa, &state.p1_lvl_ctrl));         
  state.p1_lvl = _ladder_setting_to_mV(pa,state.p1_lvl_ctrl, ladder_range);

  
  state.pf_bst = 0;

  EFUN(_get_rx_dfe1( pa, &state.dfe1));  
  EFUN(_get_rx_dfe2( pa, &state.dfe2));  
  EFUN(_get_rx_dfe3( pa, &state.dfe3));  
  EFUN(_get_rx_dfe4( pa, &state.dfe4));
  EFUN(_get_rx_dfe5( pa, &state.dfe5));

  ESTM(state.dfe1_dcd = rd_dfe_1_e()-rd_dfe_1_o());
  ESTM(state.dfe2_dcd = (rd_dfe_2_se() ? -rd_dfe_2_e(): rd_dfe_2_e()) -(rd_dfe_2_so() ? -rd_dfe_2_o(): rd_dfe_2_o()));
  
  ESTM(state.pe = rd_dfe_offset_adj_p1_even());
  ESTM(state.ze = rd_dfe_offset_adj_data_even());
  ESTM(state.me = rd_dfe_offset_adj_m1_even());
  
  ESTM(state.po = rd_dfe_offset_adj_p1_odd());
  ESTM(state.zo = rd_dfe_offset_adj_data_odd());
  ESTM(state.mo = rd_dfe_offset_adj_m1_odd());

  /* tx_ppm = register/10.84 */
  ESTM(state.tx_ppm = (int8_t)(((int32_t)(rdc_tx_pi_integ2_reg()))*3125/32768));

  EFUN(_get_tx_pre( pa, &state.txfir_pre));
  EFUN(_get_tx_main( pa, &state.txfir_main));
  EFUN(_get_tx_post1( pa, &state.txfir_post1));
  EFUN(_get_tx_post2( pa, &state.txfir_post2)); 
  EFUN(_get_tx_post3( pa, &state.txfir_post3));    
  ESTM(state.heye_left = _eye_to_mUI(pa, rdv_usr_sts_heye_left()));
  ESTM(state.heye_right = _eye_to_mUI(pa, rdv_usr_sts_heye_right()));
  ESTM(state.veye_upper = _eye_to_mV(pa, rdv_usr_sts_veye_upper(), ladder_range));
  ESTM(state.veye_lower = _eye_to_mV(pa, rdv_usr_sts_veye_lower(), ladder_range));
  ESTM(state.link_time = (((uint32_t)rdv_usr_sts_link_time())*8)/10);     /* convert from 80us to 0.1 ms units */

  if (!is_micro_stopped)  
    EFUN(eagle_tsc_stop_rx_adaptation( pa, 0));
  

  *istate = state;
  return (ERR_CODE_NONE);
}

static err_code_t _eagle_tsc_display_lane_state_no_newline( const phymod_access_t *pa ) {     
  uint16_t lane_idx;
  
  eagle_tsc_lane_state_st state = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  const char* e2s_osr_mode_enum[10] = {
    "x1   ",
    "x2   ",
    "x3   ",
    "x3.3 ",
    "x4   ",
    "x5   ",
    "x7.5 ",
    "x8   ",
    "x8.25",
    "x10  "
  };
  char *s;

  EFUN(_eagle_tsc_read_lane_state( pa, &state));
 
  lane_idx = eagle_tsc_get_lane(pa); 
  s = (char *)e2s_osr_mode_enum[state.osr_mode];
  USR_PRINTF(("%2d ", lane_idx));
  USR_PRINTF(("(%2s%s,0x%2x)", (state.br_pd_en) ? "BR" : "OS", s, state.ucv_config & 0xFF));  /* Show lower byte only */
  USR_PRINTF(("   %1d   %1d %4d ", state.sig_det, state.rx_lock, state.rx_ppm));
  USR_PRINTF((" %3d   %3d ", state.clk90, state.clkp1));
  USR_PRINTF(("  %2d,%1d  ", state.pf_main, state.pf_low));
  USR_PRINTF(("   %2d ", state.vga));
  USR_PRINTF(("%3d ", state.dc_offset));
  USR_PRINTF(("%4d ", state.p1_lvl));
  USR_PRINTF(("%3d,%3d,%3d,%3d,%3d,%3d,%3d ", state.dfe1, state.dfe2, state.dfe3, state.dfe4, state.dfe5, state.dfe1_dcd, state.dfe2_dcd));
  USR_PRINTF(("%3d,%3d,%3d,%3d,%3d,%3d  ", state.ze, state.zo, state.pe, state.po, state.me, state.mo));
  USR_PRINTF((" %4d ", state.tx_ppm));
  USR_PRINTF(("   %2d,%2d,%2d,%2d  ", state.txfir_pre, state.txfir_main, state.txfir_post1, state.txfir_post2));
  USR_PRINTF((" %3d,%3d,%3d,%3d ", state.heye_left, state.heye_right, state.veye_upper, state.veye_lower)); 
  USR_PRINTF((" %4d.%01d", state.link_time/10, state.link_time%10));
  return (ERR_CODE_NONE);
}


/* returns 000111 (7 = 8-1), for n = 3  */
#define BFMASK(n) ((1<<((n)))-1)

static void _update_uc_lane_config_st( const phymod_access_t *pa, struct  eagle_tsc_uc_lane_config_st *st) {

  uint16_t in = st->word;

  st->field.lane_cfg_from_pcs = in & BFMASK(1); in >>= 1;
  st->field.an_enabled = in & BFMASK(1); in >>= 1;
  st->field.dfe_on = in & BFMASK(1); in >>= 1;
  st->field.force_brdfe_on = in & BFMASK(1); in >>= 1;
  st->field.media_type = in & BFMASK(2); in >>= 2;
  st->field.unreliable_los = in & BFMASK(1); in >>= 1;
  st->field.scrambling_dis = in & BFMASK(1); in >>= 1;
  st->field.cl72_emulation_en = in & BFMASK(1); in >>= 1;
  st->field.reserved = in & BFMASK(7); in >>= 7;

}

static void _update_usr_ctrl_disable_functions_st( const phymod_access_t *pa, struct eagle_tsc_usr_ctrl_disable_functions_st *st) {

  uint8_t in = st->byte;
  st->field.pf_adaptation = in & BFMASK(1); in >>= 1;
  st->field.dc_adaptation = in & BFMASK(1); in >>= 1;
  st->field.vga_adaptation = in & BFMASK(1); in >>= 1;
  st->field.slicer_offset_tuning = in & BFMASK(1); in >>= 1;
  st->field.clk90_offset_adaptation = in & BFMASK(1); in >>= 1;
  st->field.p1_level_tuning = in & BFMASK(1); in >>= 1;
  st->field.eye_adaptation = in & BFMASK(1); in >>= 1;
  st->field.all_adaptation = in & BFMASK(1); in >>= 1;

}


static void _update_usr_ctrl_disable_dfe_functions_st( const phymod_access_t *pa, struct eagle_tsc_usr_ctrl_disable_dfe_functions_st *st) {
  
  uint8_t in = st->byte;
  
  st->field.dfe_tap1_adaptation = in & BFMASK(1); in >>= 1;
  st->field.dfe_tap2_adaptation = in & BFMASK(1); in >>= 1;
  st->field.dfe_tap3_adaptation = in & BFMASK(1); in >>= 1;
  st->field.dfe_tap4_adaptation = in & BFMASK(1); in >>= 1;
  st->field.dfe_tap5_adaptation = in & BFMASK(1); in >>= 1;
  st->field.dfe_tap1_dcd = in & BFMASK(1); in >>= 1;
  st->field.dfe_tap2_dcd = in & BFMASK(1); in >>= 1;

}

/* word to fields, for display */
static void _update_uc_core_config_st( const phymod_access_t *pa, struct eagle_tsc_uc_core_config_st *st) {

  uint16_t in = st->word;

  st->field.core_cfg_from_pcs = in & BFMASK(1); in >>= 1;
  st->field.vco_rate = in & BFMASK(5); in >>= 5;
  st->field.reserved1 = in & BFMASK(2); in >>= 2;
  st->field.reserved2 = in & BFMASK(8); in >>= 8;

}

/* fields to word, to write into uC RAM */
static void _update_uc_core_config_word( const phymod_access_t *pa, struct eagle_tsc_uc_core_config_st *st) {
  uint16_t in = 0;
  in <<= 8; in |= 0/*st->field.reserved2*/ & BFMASK(8);
  in <<= 2; in |= 0/*st->field.reserved1*/ & BFMASK(2);
  in <<= 5; in |= st->field.vco_rate & BFMASK(5);
  in <<= 1; in |= st->field.core_cfg_from_pcs & BFMASK(1);
  st->word = in;
}

static void _update_uc_lane_config_word( const phymod_access_t *pa, struct eagle_tsc_uc_lane_config_st *st) {

  uint16_t in = 0;

  in <<= 7; in |= 0 /*st->field.reserved*/ & BFMASK(7);
  in <<= 1; in |= st->field.cl72_emulation_en & BFMASK(1);
  in <<= 1; in |= st->field.scrambling_dis & BFMASK(1);
  in <<= 1; in |= st->field.unreliable_los & BFMASK(1);
  in <<= 2; in |= st->field.media_type & BFMASK(2);
  in <<= 1; in |= st->field.force_brdfe_on & BFMASK(1);
  in <<= 1; in |= st->field.dfe_on & BFMASK(1);
  in <<= 1; in |= st->field.an_enabled & BFMASK(1);
  in <<= 1; in |= st->field.lane_cfg_from_pcs & BFMASK(1);

  st->word = in;
}

static void _update_usr_ctrl_disable_functions_byte( const phymod_access_t *pa, struct eagle_tsc_usr_ctrl_disable_functions_st *st) {


  uint8_t in = 0;
  in <<= 1; in |= st->field.all_adaptation & BFMASK(1);
  in <<= 1; in |= st->field.eye_adaptation & BFMASK(1);
  in <<= 1; in |= st->field.p1_level_tuning & BFMASK(1);
  in <<= 1; in |= st->field.clk90_offset_adaptation & BFMASK(1);
  in <<= 1; in |= st->field.slicer_offset_tuning & BFMASK(1);
  in <<= 1; in |= st->field.vga_adaptation & BFMASK(1);
  in <<= 1; in |= st->field.dc_adaptation & BFMASK(1);
  in <<= 1; in |= st->field.pf_adaptation & BFMASK(1);
  st->byte = in;

}


static void _update_usr_ctrl_disable_dfe_functions_byte( const phymod_access_t *pa, struct  eagle_tsc_usr_ctrl_disable_dfe_functions_st *st) {
  
  uint8_t in = 0;
  
  in <<= 1; in |= st->field.dfe_tap2_dcd & BFMASK(1);
  in <<= 1; in |= st->field.dfe_tap1_dcd & BFMASK(1);
  in <<= 1; in |= st->field.dfe_tap5_adaptation & BFMASK(1);
  in <<= 1; in |= st->field.dfe_tap4_adaptation & BFMASK(1);
  in <<= 1; in |= st->field.dfe_tap3_adaptation & BFMASK(1);
  in <<= 1; in |= st->field.dfe_tap2_adaptation & BFMASK(1);
  in <<= 1; in |= st->field.dfe_tap1_adaptation & BFMASK(1);

  st->byte = in;
}

#undef BFMASK

SDK_STATIC void _trnsum_clear_and_enable(const phymod_access_t *pa) {
    /*the trnsum accumulator is cleared on the rising edge of trnsum_en signal */
    /*this function creates a rising edge on the trnsum_en signal */
    wr_trnsum_en(0);
    wr_trnsum_en(1);
    wr_uc_trnsum_en(1);
}

SDK_STATIC err_code_t _check_uc_lane_stopped( const phymod_access_t *pa ) {

  uint8_t is_micro_stopped;
  ESTM(is_micro_stopped = rdv_usr_sts_micro_stopped());
  if (!is_micro_stopped) {
      return(_error(ERR_CODE_UC_NOT_STOPPED));
  } else {
      return(ERR_CODE_NONE);
  }
}

SDK_STATIC err_code_t _calc_patt_gen_mode_sel( const phymod_access_t *pa, uint8_t *mode_sel, uint8_t *zero_pad_len, uint8_t patt_length) {

  if(!mode_sel) {
	  return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }

  /* Select the correct Pattern generator Mode depending on Pattern length */
  if (!(140 % patt_length)) {
    *mode_sel = 6;
    *zero_pad_len = 100;
  }
  else if (!(160 % patt_length)) {
    *mode_sel = 5;
    *zero_pad_len = 80;
  }
  else if (!(180 % patt_length)) {
    *mode_sel = 4;
    *zero_pad_len = 60;
  }
  else if (!(200 % patt_length)) {
    *mode_sel = 3;
    *zero_pad_len = 40;
  }
  else if (!(220 % patt_length)) {
    *mode_sel = 2;
    *zero_pad_len = 20;
  }
  else if (!(240 % patt_length)) {
    *mode_sel = 1;
    *zero_pad_len = 0;
  } else {
    USR_PRINTF(("ERROR: Unsupported Pattern Length\n"));
    return (_error(ERR_CODE_CFG_PATT_INVALID_PATT_LENGTH));
  }
  return(ERR_CODE_NONE);
}
/*****************************************************************/
/*                   SERDES_EVAL specific functions below        */
/*****************************************************************/

