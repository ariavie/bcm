/***********************************************************************************/
/***********************************************************************************/
/*  File Name     :  falcon_tsc_functions.c                                        */
/*  Created On    :  29/04/2013                                                    */
/*  Created By    :  Kiran Divakar                                                 */
/*  Description   :  APIs for Serdes IPs                                           */
/*  Revision      :  $Id: falcon_tsc_functions.c 535 2014-05-23 23:10:44Z kirand $ */
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
 */

/** @file falcon_tsc_functions.c
 * Implementation of API functions
 */

#ifdef _MSC_VER
/* Enclose all standard headers in a pragma to remove warings for MS compiler */
#pragma warning( push, 0 )
#endif

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#include <phymod/phymod.h>
#include "falcon_tsc_functions.h"

#include "falcon_tsc_field_access_c.h"
#include "falcon_tsc_internal_c.h"
/* #include "falcon_tsc_dv_functions.c" */



#include "falcon_tsc_pll_config_c.h"
#define UINT32_MAX (4294967295UL)



/************************************/
/*  Display Eye Scan                */
/************************************/

/* This is best method for terminal ASCII display */
err_code_t falcon_tsc_display_eye_scan( const phymod_access_t *pa ) {
  int8_t y;
  err_code_t err_code;
  uint16_t status = 0;
  uint32_t stripe[64];
  uint8_t data_thresh;

  /* start horizontal acquisition */
  err_code = falcon_tsc_meas_eye_scan_start( pa, EYE_SCAN_HORIZ);
  if (err_code) {
    falcon_tsc_meas_eye_scan_done( pa );
    return (err_code);
  }
  ESTM(data_thresh = rd_rx_data_thresh_sel());

  falcon_tsc_display_eye_scan_header( pa, 1);
  /* display stripe */
  for (y = 124;y>=-124;y=y-4) { 
 	err_code = falcon_tsc_read_eye_scan_stripe( pa, &stripe[0], &status);
	if (err_code) {
		falcon_tsc_meas_eye_scan_done( pa );
		return (err_code);
	}
	falcon_tsc_display_eye_scan_stripe( pa, y-data_thresh,&stripe[0]);
	USR_PRINTF(("\n"));  
  }  
  /* stop acquisition */
  err_code = falcon_tsc_meas_eye_scan_done( pa );
  if (err_code) {
	return (err_code);
  }

  falcon_tsc_display_eye_scan_footer( pa, 1);
  return(ERR_CODE_NONE);
}

/* This function is for Falcon and is configured for passive mode */
err_code_t falcon_tsc_meas_lowber_eye( const phymod_access_t *pa, const struct falcon_tsc_eyescan_options_st eyescan_options, uint32_t *buffer) {
  int8_t y,x;
  int16_t i;
  uint16_t status;
  uint32_t errors = 0;
  uint16_t timeout;


  if(!buffer) {
	  return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }
  i = 0;

  timeout = eyescan_options.timeout_in_milliseconds;
  wrcv_diag_max_time_control((uint8_t)timeout);
 
  wrv_usr_diag_mode(eyescan_options.mode);

  USR_PRINTF(("Calculating\n"));
  for (y = eyescan_options.vert_max;y>=eyescan_options.vert_min;y=y-eyescan_options.vstep) {   
	for (x=eyescan_options.horz_min;x<=eyescan_options.horz_max;x=x+eyescan_options.hstep) {
	  /* acquire sample */
        EFUN(falcon_tsc_pmd_uc_cmd_with_data( pa, CMD_DIAG_EN,CMD_UC_DIAG_GET_EYE_SAMPLE,((uint16_t)x)<<8 | (uint8_t)y,200));
        /* wait for sample complete */
#if 0
        do {
            falcon_tsc_delay_us(1000);
            ESTM(status=rdv_usr_diag_status());
            USR_PRINTF(("status=%04x\n",status));
        } while((status & 0x8000) == 0); 
#else
        EFUN(falcon_tsc_poll_diag_done( pa, &status, (((uint32_t)timeout)<<7)*10 + 20000));
#endif
        EFUN(falcon_tsc_prbs_err_count_ll( pa, &errors));
        /* USR_PRINTF(("(%d,%d) = %u\n",x,y,errors & 0x7FFFFFF)); */
        buffer[i] = errors & 0x7FFFFFFF;
        i++;
        USR_PRINTF(("."));
    }
    USR_PRINTF(("\n"));        
  }
  USR_PRINTF(("\n"));
  EFUN(falcon_tsc_meas_eye_scan_done( pa ));
  return(ERR_CODE_NONE);
}


/* Display the LOW BER EyeScan */
err_code_t falcon_tsc_display_lowber_eye( const phymod_access_t *pa, const struct falcon_tsc_eyescan_options_st eyescan_options, uint32_t *buffer) {
    int8_t x,y,i,z;
    int16_t j; /* buffer pointer */
    uint32_t val;
    uint8_t overflow;
    uint32_t limits[13]; /* allows upto 400 sec test time per point (1e-13 BER @ 10G) */
    int16_t mV;
 
    if(!buffer) {
        return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
     }
     
    /* Calculate initial total bitcount BER 1e-1 */
    limits[0] = _mult_with_overflow_check(eyescan_options.linerate_in_khz/10, eyescan_options.timeout_in_milliseconds, &overflow);
    if(overflow > 0) {
        limits[0] = UINT32_MAX;
        USR_PRINTF(("Very long timout_in_milliseconds results in saturation of Err counter can cause in accurate results\n"));
    }

    for(i=1;i<13;i++) {            /* calculate thresholds */
        limits[i] = limits[i-1]/10;
    }
    
    falcon_tsc_display_eye_scan_header( pa, 1);
	j = 0;
    for (y = eyescan_options.vert_max;y>=eyescan_options.vert_min;y=y-eyescan_options.vstep) {
        mV = _ladder_setting_to_mV(y, 0);
        ESTM(USR_PRINTF(("%6dmV : ", mV)));
	  for(z=-31;z<eyescan_options.horz_min;z++) {
              USR_PRINTF((" ")); /* add leading spaces if any */
	  }
      for (x=eyescan_options.horz_min;x<=eyescan_options.horz_max;x=x+eyescan_options.hstep) {
/*        val = float8_to_int32(buffer[j); */
		val = buffer[j];

        for (i=0;i<13;i++) {
          if ((val != 0) & ((val>=limits[i]) | (limits[i] == 0))) {
		    for(z=1;z<=eyescan_options.hstep;z++) {
				if(z==1) {
				  if(i<=8) {
					 USR_PRINTF(("%c", '1'+i));
				  } else {
					  USR_PRINTF(("%c", 'A'+i-9));
				  }
                                }
				else {
			          USR_PRINTF((" "));
                                }
		    }
		    break;
		  }
	    }
        if (i==13) {
	      for(z=1;z<=eyescan_options.hstep;z++) {
			  if(z==1) {
				if ((x%5)==0 && (y%5)==0) {
				   USR_PRINTF(("+"));
                                }
				else if ((x%5)!=0 && (y%5)==0) {
				   USR_PRINTF(("-"));
                                }
				else if ((x%5)==0 && (y%5)!=0) {
				   USR_PRINTF((":"));
                                }
				else {
				   USR_PRINTF((" "));
                                }
                          }
			  else {
				USR_PRINTF((" "));
                          }
              }
        }
        j++;
     }
     USR_PRINTF(("\n"));  
   }  
   falcon_tsc_display_eye_scan_footer( pa, 1);
   return(ERR_CODE_NONE);
}

err_code_t falcon_tsc_meas_eye_scan_start( const phymod_access_t *pa, uint8_t direction) {
  err_code_t err_code;
  uint8_t lock;
  ESTM(lock = rd_pmd_rx_lock());
  if(lock == 0) {
      USR_PRINTF(("Error: No PMD_RX_LOCK on lane requesting 2D eye scan\n"));
      return(ERR_CODE_DIAG_SCAN_NOT_COMPLETE);
  }
  if(direction == EYE_SCAN_VERTICAL) {
	err_code = falcon_tsc_pmd_uc_diag_cmd( pa, CMD_UC_DIAG_START_VSCAN_EYE,200);
  } else {
	  err_code = falcon_tsc_pmd_uc_diag_cmd( pa, CMD_UC_DIAG_START_HSCAN_EYE,200);
  }
  	if (err_code) {
		return (err_code);
	}

  return(ERR_CODE_NONE);
}

err_code_t falcon_tsc_read_eye_scan_stripe( const phymod_access_t *pa, uint32_t *buffer,uint16_t *status){
	int8_t i;
	uint32_t val[2] = {0,0};
    err_code_t err_code;
	uint16_t sts = 0;

	if(!buffer || !status) {
		return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
	}
	*status = 0;
	for(i=0;i<32;i++) {
		err_code = falcon_tsc_poll_diag_eye_data(pa,&val[0], &sts, 200);
 		*status |= sts & 0xF000;
		if (err_code) {
			return (err_code);
		}
		buffer[i*2]     = val[0];
		buffer[(i*2)+1] = val[1];
	}
	*status |= sts & 0x00FF;
    return(ERR_CODE_NONE);
}

err_code_t falcon_tsc_display_eye_scan_stripe( const phymod_access_t *pa, int8_t y,uint32_t *buffer) {
  const uint32_t limits[7] = {1835008, 183501, 18350, 1835, 184, 18, 2};
  int8_t x,i;
  int16_t mV;

  	if(!buffer) {
		return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
	}

    mV = _ladder_setting_to_mV(y, 0);
    ESTM(USR_PRINTF(("%6dmV : ", mV)));

 
	for (x=-31;x<32;x++) {
	  for (i=0;i<7;i++) 
		if (buffer[x+31]>=limits[i]) {
			USR_PRINTF(("%c", '0'+i+1));
			break;
		}
		if (i==7) {
			if ((x%5)==0 && (y%5)==0) {
				USR_PRINTF(("+"));
                        }
			else if ((x%5)!=0 && (y%5)==0) {
				USR_PRINTF(("-"));
                        }
			else if ((x%5)==0 && (y%5)!=0) {
				USR_PRINTF((":"));
                        }
			else {
				USR_PRINTF((" "));
                        }
		}
	}
	return(ERR_CODE_NONE);
}

err_code_t falcon_tsc_display_eye_scan_header( const phymod_access_t *pa, int8_t i) {
int8_t x;
	USR_PRINTF(("\n"));
	for(x=1;x<=i;x++) {
		USR_PRINTF(("  UI/64  : -30  -25  -20  -15  -10  -5    0    5    10   15   20   25   30"));
	}
	USR_PRINTF(("\n"));
	for(x=1;x<=i;x++) {
		USR_PRINTF(("         : -|----|----|----|----|----|----|----|----|----|----|----|----|-"));
	}
	USR_PRINTF(("\n"));
	return(ERR_CODE_NONE);
}

err_code_t falcon_tsc_display_eye_scan_footer( const phymod_access_t *pa, int8_t i) {
int8_t x;
	for(x=1;x<=i;x++) {
		USR_PRINTF(("         : -|----|----|----|----|----|----|----|----|----|----|----|----|-"));
	}
	USR_PRINTF(("\n"));
	for(x=1;x<=i;x++) {
		USR_PRINTF(("  UI/64  : -30  -25  -20  -15  -10  -5    0    5    10   15   20   25   30"));
	}
	USR_PRINTF(("\n"));
	return(ERR_CODE_NONE);
}


/*eye_scan_status_t falcon_tsc_read_eye_scan_status() */
err_code_t falcon_tsc_read_eye_scan_status( const phymod_access_t *pa, uint16_t *status) {
   if(!status) {
		return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
	}

   ESTM(*status=rdv_usr_diag_status());	
	return(ERR_CODE_NONE);
}


err_code_t falcon_tsc_meas_eye_scan_done( const phymod_access_t *pa ) {
  err_code_t err_code;
  err_code = falcon_tsc_pmd_uc_diag_cmd( pa, CMD_UC_DIAG_DISABLE,200);
  if (err_code) return(err_code);
  return(ERR_CODE_NONE);
}


err_code_t falcon_tsc_start_ber_scan_test( const phymod_access_t *pa, uint8_t ber_scan_mode, uint8_t timer_control, uint8_t max_error_control) {
    uint8_t lock;
    ESTM(lock = rd_pmd_rx_lock());
    if(lock == 0) {
        USR_PRINTF(("Error: No PMD_RX_LOCK on lane requesting BER scan\n"));
        return(ERR_CODE_DIAG_SCAN_NOT_COMPLETE);
    }

    wrcv_diag_max_time_control(timer_control);
    wrcv_diag_max_err_control(max_error_control);
    EFUN(falcon_tsc_pmd_uc_cmd( pa, CMD_CAPTURE_BER_START, ber_scan_mode,200));
    return(ERR_CODE_NONE);
}

err_code_t falcon_tsc_read_ber_scan_data( const phymod_access_t *pa, uint32_t *errors, uint32_t *timer_values, uint8_t *cnt, uint32_t timeout) {
    err_code_t err_code;
    uint8_t i,prbs_byte,prbs_multi,time_byte,time_multi;
    uint16_t sts,dataword;

    if(!errors || !timer_values || !cnt) {
        return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }
    /* init data arrays */
    for(i=0;i< DIAG_MAX_SAMPLES;i++) {
        errors[i]=0;
        timer_values[i]=0;
    }
	/* Check for completion read ln.diag_status byte?*/
 ESTM(sts = rdv_usr_diag_status());
	if((sts & 0x8000) == 0) {
		return(_error(ERR_CODE_DATA_NOTAVAIL));
	}
	*cnt = (sts & 0x00FF)/3;
	for(i=0;i < *cnt;i++) {
		/* Read 2 bytes of data */
		err_code = falcon_tsc_pmd_uc_cmd( pa,  CMD_READ_DIAG_DATA_WORD, 0, timeout);
		if(err_code) return(err_code);
		ESTM(dataword = rd_uc_dsc_data());           /* LSB contains 2 -4bit nibbles */
		time_byte = (uint8_t)(dataword>>8);    /* MSB is time byte */
		prbs_multi = (uint8_t)dataword & 0x0F; /* split nibbles */
		time_multi = (uint8_t)dataword>>4;
		/* Read 1 bytes of data */
		err_code = falcon_tsc_pmd_uc_cmd( pa,  CMD_READ_DIAG_DATA_BYTE, 0, timeout);
		if(err_code) return(err_code);
		ESTM(prbs_byte = (uint8_t)rd_uc_dsc_data());
		errors[i] = _float12_to_uint32( pa, prbs_byte,prbs_multi); /* convert 12bits to uint32 */
		timer_values[i] = (_float12_to_uint32( pa, time_byte,time_multi)<<3);
		/*USR_PRINTF(("Err=%d (%02x<<%d); Time=%d (%02x<<%d)\n",errors[i],prbs_byte,prbs_multi,timer_values[i],time_byte,time_multi<<3));*/
		/*if(timer_values[i] == 0 && errors[i] == 0) break;*/
	}
  return(ERR_CODE_NONE);
}


/* This is good example function to do BER extrapolation */
err_code_t falcon_tsc_eye_margin_proj( const phymod_access_t *pa, USR_DOUBLE rate, uint8_t ber_scan_mode, uint8_t timer_control, uint8_t max_error_control) {

	return(ERR_CODE_NONE);
}

err_code_t falcon_tsc_display_ber_scan_data(USR_DOUBLE rate, uint8_t ber_scan_mode, uint32_t *total_errs, uint32_t *total_time, uint8_t max_offset) {
 USR_PRINTF(("This functions needs SERDES_API_FLOATING_POINT define to operate \n"));
	return(ERR_CODE_NONE);

}



/*****************************/
/*  Display Lane/Core State  */
/*****************************/

err_code_t falcon_tsc_display_lane_state_hdr( const phymod_access_t *pa ) {
  USR_PRINTF(("LN (CDRxN  ,UC_CFG) "));
  USR_PRINTF(("SD LCK RXPPM "));
  USR_PRINTF(("PF,PF2 "));
  USR_PRINTF(("VGA DCO "));
  USR_PRINTF((" DFE(1,2,3,4,5,6)        "));
  USR_PRINTF(("TXPPM TXEQ(n1,m,p1,p2,p3) EYE(L,R,U,D)  "));
  USR_PRINTF(("LINK_TIME"));
  USR_PRINTF(("\n"));
  return (ERR_CODE_NONE);
}

err_code_t falcon_tsc_display_lane_state_legend( const phymod_access_t *pa ) {
  USR_PRINTF(("\n"));
  USR_PRINTF(("**********************************************************************************************\n")); 
  USR_PRINTF(("****                Legend of Entries in display_lane_state()                             ****\n")); 
  USR_PRINTF(("**********************************************************************************************\n"));
  USR_PRINTF(("LN       : lane index within IP core\n"));
  USR_PRINTF(("(CDRxN,UC_CFG) : CDR type x OSR ratio, micro lane configuration variable\n"));
  USR_PRINTF(("SD       : signal detect\n"));
  USR_PRINTF(("LOCK     : pmd_rx_lock\n"));
  USR_PRINTF(("RXPPM    : Frequency offset of local reference clock with respect to RX data in ppm\n"));
  USR_PRINTF(("PF  PF2  : Peaking Filter Main (0..15) and PF2 (0..7)\n"));
  USR_PRINTF(("VGA      : Variable Gain Amplifier settings (0..42)\n"));
  USR_PRINTF(("DCO      : DC offset DAC control value\n"));
  USR_PRINTF(("DFE taps         : ISI correction taps in units of 2.35mV (for 1 & 2 even values are displayed, dcd = even-odd)\n"));   
  USR_PRINTF(("TXPPM            : Frequency offset of local reference clock with respect to TX data in ppm\n"));  
  USR_PRINTF(("TXEQ(n1,m,p1,p2) : TX equalization FIR tap weights in units of 1Vpp/60 units\n"));
  USR_PRINTF(("EYE(L,R,U,D)     : Eye margin @ 1e-5 as seen by internal diagnostic slicer in mUI and mV\n"));
  USR_PRINTF(("LINK_TIME        : Link time in milliseconds\n"));
  USR_PRINTF(("**********************************************************************************************\n")); 
  return (ERR_CODE_NONE);
}

err_code_t falcon_tsc_display_lane_state( const phymod_access_t *pa ) {
  err_code_t err_code;
  err_code = _falcon_tsc_display_lane_state_no_newline( pa );
  USR_PRINTF(("\n"));
  return (err_code);
}





/**********************************************/
/*  Display Lane/Core Config and Debug Status */
/**********************************************/

err_code_t falcon_tsc_display_core_config( const phymod_access_t *pa ) {

  err_code_t  err_code;
  struct falcon_tsc_uc_core_config_st core_cfg;
  uint16_t vco_rate_mhz;

  PHYMOD_MEMSET(&core_cfg, 0, sizeof(struct falcon_tsc_uc_core_config_st));


  USR_PRINTF(("\n\n***********************************\n")); 
  USR_PRINTF(("**** SERDES CORE CONFIGURATION ****\n")); 
  USR_PRINTF(("***********************************\n\n")); 

  err_code = falcon_tsc_get_uc_core_config( pa, &core_cfg);
  if (err_code) {
    return (err_code);
  }

    vco_rate_mhz = (((((uint16_t) core_cfg.field.vco_rate) * 125)/2) + 14000);
  USR_PRINTF(("uC Config VCO Rate   = %d (%d.%dGHz)\n",core_cfg.field.vco_rate,vco_rate_mhz/1000,vco_rate_mhz % 1000));
  USR_PRINTF(("Core Config from PCS = %d\n\n", core_cfg.field.core_cfg_from_pcs));

  ESTM(USR_PRINTF(("Lane Addr 0          = %d\n", rdc_lane_addr_0())));
  ESTM(USR_PRINTF(("Lane Addr 1          = %d\n", rdc_lane_addr_1())));
  ESTM(USR_PRINTF(("Lane Addr 2          = %d\n", rdc_lane_addr_2())));
  ESTM(USR_PRINTF(("Lane Addr 3          = %d\n", rdc_lane_addr_3())));


  ESTM(USR_PRINTF(("TX Lane Map 0        = %d\n", rdc_tx_lane_map_0())));
  ESTM(USR_PRINTF(("TX Lane Map 1        = %d\n", rdc_tx_lane_map_1())));
  ESTM(USR_PRINTF(("TX Lane Map 2        = %d\n", rdc_tx_lane_map_2())));
  ESTM(USR_PRINTF(("TX Lane Map 3        = %d\n\n", rdc_tx_lane_map_3())));

  return(ERR_CODE_NONE); 
}


err_code_t falcon_tsc_display_lane_config( const phymod_access_t *pa ) {

  err_code_t  err_code;
  struct falcon_tsc_uc_lane_config_st lane_cfg;
  PHYMOD_MEMSET(&lane_cfg, 0, sizeof(struct falcon_tsc_uc_lane_config_st));

  USR_PRINTF(("\n\n*************************************\n")); 
  USR_PRINTF(("**** SERDES LANE %d CONFIGURATION ****\n",falcon_tsc_get_lane(pa))); 
  USR_PRINTF(("*************************************\n\n")); 

  err_code = falcon_tsc_get_uc_lane_cfg( pa, &lane_cfg);
  if (err_code) {
    return (err_code);
  }

  USR_PRINTF(("Auto-Neg Enabled      = %d\n", lane_cfg.field.an_enabled));
  USR_PRINTF(("DFE on                = %d\n", lane_cfg.field.dfe_on));
  USR_PRINTF(("Brdfe_on              = %d\n", lane_cfg.field.force_brdfe_on));
  USR_PRINTF(("Media Type            = %d\n", lane_cfg.field.media_type));
  USR_PRINTF(("Unreliable LOS        = %d\n", lane_cfg.field.unreliable_los));
  USR_PRINTF(("Scrambling Disable    = %d\n", lane_cfg.field.scrambling_dis));
  USR_PRINTF(("CL72 Emulation Enable = %d\n", lane_cfg.field.cl72_emulation_en));
  USR_PRINTF(("Lane Config from PCS  = %d\n\n", lane_cfg.field.lane_cfg_from_pcs));

  ESTM(USR_PRINTF(("CL93/72 Training Enable  = %d\n", rd_cl93n72_ieee_training_enable())));
  ESTM(USR_PRINTF(("EEE Mode Enable       = %d\n", rd_eee_mode_en())));
  ESTM(USR_PRINTF(("OSR Mode Force        = %d\n", rd_osr_mode_frc())));
  ESTM(USR_PRINTF(("OSR Mode Force Val    = %d\n", rd_osr_mode_frc_val())));
  ESTM(USR_PRINTF(("TX Polarity Invert    = %d\n", rd_tx_pmd_dp_invert())));
  ESTM(USR_PRINTF(("RX Polarity Invert    = %d\n\n", rd_rx_pmd_dp_invert())));

  ESTM(USR_PRINTF(("TXFIR Post2           = %d\n", rd_txfir_post2())));
  ESTM(USR_PRINTF(("TXFIR Post3           = %d\n", rd_txfir_post3())));
  ESTM(USR_PRINTF(("TXFIR Main            = %d\n", rd_cl93n72_txfir_main())));
  ESTM(USR_PRINTF(("TXFIR Pre             = %d\n", rd_cl93n72_txfir_pre())));
  ESTM(USR_PRINTF(("TXFIR Post            = %d\n", rd_cl93n72_txfir_post())));

  return(ERR_CODE_NONE); 
}


err_code_t falcon_tsc_display_core_state( const phymod_access_t *pa ) {

  uint8_t     temp_idx;
  int8_t      temp_val;

  USR_PRINTF(("\n\n***********************************\n")); 
  USR_PRINTF(("**** SERDES CORE DISPLAY STATE ****\n")); 
  USR_PRINTF(("***********************************\n\n")); 

  ESTM(USR_PRINTF(("Average Die TMON_reg13bit = %d\n", rdcv_avg_tmon_reg13bit())));
  ESTM(USR_PRINTF(("Temperature Force Val     = %d\n", rdcv_temp_frc_val())));

  ESTM(temp_idx = rdcv_temp_idx());
  if (temp_idx == 0) {
    USR_PRINTF(("Temperature Index         = 0  [T_MIN < -36C; T_CENTRE = -36C; T_MAX = -32C]\n"));
  }
  else if (temp_idx < 20) {
    temp_val = temp_idx * 8;
    temp_val = temp_val - 40;
    USR_PRINTF(("Temperature Index         = %d  [%dC to %dC]\n",temp_idx,temp_val,temp_val+8));
  }
  else if (temp_idx == 20) {
    USR_PRINTF(("Temperature Index         = 20  [T_MIN = 120C; T_CENTRE = 124C; T_MAX > 124C]\n"));
  }
  else {
    return (_error(ERR_CODE_INVALID_TEMP_IDX));
  }
  ESTM(USR_PRINTF(("Core Event Log Level      = %d\n\n", rdcv_usr_ctrl_core_event_log_level())));

  ESTM(USR_PRINTF(("Core DP Reset State       = %d\n\n", rdc_core_dp_reset_state())));

  ESTM(USR_PRINTF(("Common Ucode Version       = 0x%x\n", rdcv_common_ucode_version())));
  ESTM(USR_PRINTF(("Common Ucode Minor Version = 0x%x\n", rdcv_common_ucode_minor_version())));
  ESTM(USR_PRINTF(("AFE Hardware Version       = 0x%x\n\n", rdcv_afe_hardware_version())));

  return(ERR_CODE_NONE); 
}



err_code_t falcon_tsc_display_lane_debug_status( const phymod_access_t *pa ) {

  err_code_t  err_code;

  /* startup */
  struct falcon_tsc_usr_ctrl_disable_functions_st ds; 
  struct falcon_tsc_usr_ctrl_disable_dfe_functions_st dsd; 
  /* steady state */
  struct falcon_tsc_usr_ctrl_disable_functions_st dss;
  struct falcon_tsc_usr_ctrl_disable_dfe_functions_st dssd; 

  PHYMOD_MEMSET(&ds, 0, sizeof(struct falcon_tsc_usr_ctrl_disable_functions_st));
  PHYMOD_MEMSET(&dsd, 0, sizeof(struct falcon_tsc_usr_ctrl_disable_dfe_functions_st));
  PHYMOD_MEMSET(&dss, 0, sizeof(struct falcon_tsc_usr_ctrl_disable_functions_st));
  PHYMOD_MEMSET(&dssd, 0, sizeof(struct falcon_tsc_usr_ctrl_disable_dfe_functions_st));


  USR_PRINTF(("\n\n************************************\n")); 
  USR_PRINTF(("**** SERDES LANE %d DEBUG STATUS ****\n",falcon_tsc_get_lane(pa))); 
  USR_PRINTF(("************************************\n\n")); 

  ESTM(USR_PRINTF(("Restart Count       = %d\n", rdv_usr_sts_restart_counter())));
  ESTM(USR_PRINTF(("Reset Count         = %d\n", rdv_usr_sts_reset_counter())));
  ESTM(USR_PRINTF(("PMD Lock Count      = %d\n\n", rdv_usr_sts_pmd_lock_counter())));
 

  err_code = falcon_tsc_get_usr_ctrl_disable_startup( pa, &ds);
  if (err_code) {
    return (err_code);
  }

  USR_PRINTF(("Disable Startup PF Adaptation           = %d\n", ds.field.pf_adaptation));
  USR_PRINTF(("Disable Startup PF2 Adaptation          = %d\n", ds.field.pf2_adaptation));
  USR_PRINTF(("Disable Startup DC Adaptation           = %d\n", ds.field.dc_adaptation));
  USR_PRINTF(("Disable Startup VGA Adaptation          = %d\n", ds.field.vga_adaptation));
  USR_PRINTF(("Disable Startup Slicer vOffset Tuning   = %d\n", ds.field.slicer_voffset_tuning));
  USR_PRINTF(("Disable Startup Slicer hOffset Tuning   = %d\n", ds.field.slicer_hoffset_tuning));
  USR_PRINTF(("Disable Startup Phase offset Adaptation = %d\n", ds.field.phase_offset_adaptation));
  USR_PRINTF(("Disable Startup Eye Adaptaion           = %d\n", ds.field.eye_adaptation));
  USR_PRINTF(("Disable Startup All Adaptaion           = %d\n\n", ds.field.all_adaptation));

  err_code = falcon_tsc_get_usr_ctrl_disable_startup_dfe( pa, &dsd);
  if (err_code) {
    return (err_code);
  }

  USR_PRINTF(("Disable Startup DFE Tap1 Adaptation    = %d\n",dsd.field.dfe_tap1_adaptation));
  USR_PRINTF(("Disable Startup DFE FX Taps Adaptation = %d\n",dsd.field.dfe_fx_taps_adaptation));
  USR_PRINTF(("Disable Startup DFE FL Taps Adaptation = %d\n",dsd.field.dfe_fl_taps_adaptation));
  USR_PRINTF(("Disable Startup DFE Tap DCD            = %d\n",dsd.field.dfe_dcd_adaptation));       

  err_code = falcon_tsc_get_usr_ctrl_disable_steady_state( pa, &dss);
  if (err_code) {
    return (err_code);
  }

  USR_PRINTF(("Disable Steady State PF Adaptation           = %d\n", dss.field.pf_adaptation));
  USR_PRINTF(("Disable Steady State PF2 Adaptation          = %d\n", dss.field.pf2_adaptation));
  USR_PRINTF(("Disable Steady State DC Adaptation           = %d\n", dss.field.dc_adaptation));
  USR_PRINTF(("Disable Steady State VGA Adaptation          = %d\n", dss.field.vga_adaptation));
  USR_PRINTF(("Disable Steady State Slicer vOffset Tuning   = %d\n", dss.field.slicer_voffset_tuning));
  USR_PRINTF(("Disable Steady State Slicer hOffset Tuning   = %d\n", dss.field.slicer_hoffset_tuning));
  USR_PRINTF(("Disable Steady State Phase offset Adaptation = %d\n", dss.field.phase_offset_adaptation));
  USR_PRINTF(("Disable Steady State Eye Adaptaion           = %d\n", dss.field.eye_adaptation));
  USR_PRINTF(("Disable Steady State All Adaptaion           = %d\n\n", dss.field.all_adaptation));

  err_code = falcon_tsc_get_usr_ctrl_disable_steady_state_dfe( pa, &dssd);
  if (err_code) {
    return (err_code);
  }

  USR_PRINTF(("Disable Steady State DFE Tap1 Adaptation    = %d\n",dssd.field.dfe_tap1_adaptation));
  USR_PRINTF(("Disable Steady State DFE FX Taps Adaptation = %d\n",dssd.field.dfe_fx_taps_adaptation));
  USR_PRINTF(("Disable Steady State DFE FL Taps Adaptation = %d\n",dssd.field.dfe_fl_taps_adaptation));
  USR_PRINTF(("Disable Steady State DFE Tap DCD            = %d\n",dssd.field.dfe_dcd_adaptation));       

  ESTM(USR_PRINTF(("Retune after Reset    = %d\n", rdv_usr_ctrl_retune_after_restart())));               
  ESTM(USR_PRINTF(("Clk90 offset Adjust   = %d\n", rdv_usr_ctrl_clk90_offset_adjust())));                
  ESTM(USR_PRINTF(("Clk90 offset Override = %d\n", rdv_usr_ctrl_clk90_offset_override())));              
  ESTM(USR_PRINTF(("Lane Event Log Level  = %d\n", rdv_usr_ctrl_lane_event_log_level())));

  return(ERR_CODE_NONE); 
}



/*************************/
/*  Stop/Resume uC Lane  */
/*************************/

err_code_t falcon_tsc_stop_uc_lane( const phymod_access_t *pa, uint8_t enable) {

  if (enable) {
    return(falcon_tsc_pmd_uc_control( pa, CMD_UC_CTRL_STOP_GRACEFULLY,2000));
  }
  else {
    return(falcon_tsc_pmd_uc_control( pa, CMD_UC_CTRL_RESUME,2000));
  }
}


err_code_t falcon_tsc_stop_uc_lane_status( const phymod_access_t *pa, uint8_t *uc_lane_stopped) {
  
  if(!uc_lane_stopped) {
	  return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }

  ESTM(*uc_lane_stopped = rdv_usr_sts_micro_stopped());

  return (ERR_CODE_NONE);
}


/*******************************/
/*  Stop/Resume RX Adaptation  */
/*******************************/

err_code_t falcon_tsc_stop_rx_adaptation( const phymod_access_t *pa, uint8_t enable) {

  if (enable) {
    return(falcon_tsc_pmd_uc_control( pa, CMD_UC_CTRL_STOP_GRACEFULLY,2000));
  }
  else {
    return(falcon_tsc_pmd_uc_control( pa, CMD_UC_CTRL_RESUME,2000));
  }
}



/**********************/
/*  uCode CRC Verify  */
/**********************/

err_code_t falcon_tsc_ucode_crc_verify( const phymod_access_t *pa, uint16_t ucode_len,uint16_t expected_crc_value) {
    uint16_t calc_crc;

    EFUN(falcon_tsc_pmd_uc_cmd_with_data( pa, CMD_CALC_CRC,0,ucode_len,2000));

    ESTM(calc_crc = rd_uc_dsc_data());
    if(calc_crc != expected_crc_value) {
        USR_PRINTF(("UC CRC did not match expected=%04x : calculated=%04x\n",expected_crc_value, calc_crc));
        return(_error(ERR_CODE_UC_CRC_NOT_MATCH));
    }

    return(ERR_CODE_NONE);
}


/******************************************************/
/*  Commands through Serdes FW DSC Command Interface  */
/******************************************************/

err_code_t falcon_tsc_pmd_uc_cmd( const phymod_access_t *pa, enum falcon_tsc_pmd_uc_cmd_enum cmd, uint8_t supp_info, uint32_t timeout_ms) {

  err_code_t err_code;
  uint16_t cmddata;
  uint8_t uc_dsc_error_found;

  err_code = falcon_tsc_poll_uc_dsc_ready_for_cmd_equals_1( pa, timeout_ms); /* Poll for uc_dsc_ready_for_cmd = 1 to indicate falcon_tsc ready for command */
  if (err_code) {
    ESTM(USR_PRINTF(("ERROR : DSC ready for command timed out (before cmd) cmd = %d, supp_info = x%02x err=%d !\n", cmd, supp_info,err_code)));      
    return (err_code);
  }
  /*wr_uc_dsc_supp_info(supp_info);*/                                   /* supplement info field */
  /*wr_uc_dsc_error_found(0x0);    */                                   /* Clear error found field */
  /*wr_uc_dsc_gp_uc_req(cmd);      */                                   /* Set command code */
  /*wr_uc_dsc_ready_for_cmd(0x0);  */                                   /* Issue command, by clearing "ready for command" field */
  cmddata = (((uint16_t)supp_info)<<8) | (uint16_t)cmd;     /* combine writes to single write instead of 4 RMW */

  EFUN(falcon_tsc_pmd_wr_reg(pa,DSC_A_DSC_UC_CTRL, cmddata));         /* This address is same for Eagle, and all merlin */

  EFUN(falcon_tsc_poll_uc_dsc_ready_for_cmd_equals_1( pa, timeout_ms)); /* Poll for uc_dsc_ready_for_cmd = 1 to indicate falcon_tsc ready for command */
  ESTM(uc_dsc_error_found = rd_uc_dsc_error_found());
  if(uc_dsc_error_found) {
    ESTM(USR_PRINTF(("ERROR : DSC ready for command return error ( after cmd) cmd = %d, supp_info = x%02x !\n", cmd, rd_uc_dsc_supp_info())));      
      return(_error(ERR_CODE_UC_CMD_RETURN_ERROR));
  }
  return(ERR_CODE_NONE);
}

err_code_t falcon_tsc_pmd_uc_cmd_with_data( const phymod_access_t *pa, enum falcon_tsc_pmd_uc_cmd_enum cmd, uint8_t supp_info, uint16_t data, uint32_t timeout_ms) {
  uint16_t cmddata;
  err_code_t err_code;
  uint8_t uc_dsc_error_found;

  err_code = falcon_tsc_poll_uc_dsc_ready_for_cmd_equals_1( pa, timeout_ms); /* Poll for uc_dsc_ready_for_cmd = 1 to indicate falcon_tsc ready for command */
  if (err_code) {
	 USR_PRINTF(("ERROR : DSC ready for command timed out (before cmd) cmd = %d, supp_info = x%02x, data = x%04x err=%d !\n", cmd, supp_info, data,err_code));      
    return (err_code);
  }

  EFUN(wr_uc_dsc_data(data));                                       /* Write value written to uc_dsc_data field */
  /*wr_uc_dsc_supp_info(supp_info);  */                               /* supplement info field */
  /*wr_uc_dsc_error_found(0x0);      */                               /* Clear error found field */
  /*wr_uc_dsc_gp_uc_req(cmd);        */                               /* Set command code */
  /*wr_uc_dsc_ready_for_cmd(0x0);    */                               /* Issue command, by clearing "ready for command" field */
  cmddata = (((uint16_t)supp_info)<<8) | (uint16_t)cmd;   /* combine writes to single write instead of 4 RMW */

  EFUN(falcon_tsc_pmd_wr_reg(pa,DSC_A_DSC_UC_CTRL, cmddata));         /* This address is same for Eagle, and all merlin */

  EFUN(falcon_tsc_poll_uc_dsc_ready_for_cmd_equals_1( pa, timeout_ms)); /* Poll for uc_dsc_ready_for_cmd = 1 to indicate falcon_tsc ready for command */
  ESTM(uc_dsc_error_found = rd_uc_dsc_error_found());
  if(uc_dsc_error_found) {
    ESTM(USR_PRINTF(("ERROR : DSC ready for command return error ( after cmd) cmd = %d, supp_info = x%02x !\n", cmd, rd_uc_dsc_supp_info())));      
    return(_error(ERR_CODE_UC_CMD_RETURN_ERROR));
}

  return(ERR_CODE_NONE);
}

err_code_t falcon_tsc_pmd_uc_control( const phymod_access_t *pa, enum falcon_tsc_pmd_uc_ctrl_cmd_enum control, uint32_t timeout_ms) {
  return(falcon_tsc_pmd_uc_cmd( pa, CMD_UC_CTRL, (uint8_t) control, timeout_ms));
}

err_code_t falcon_tsc_pmd_uc_diag_cmd( const phymod_access_t *pa, enum falcon_tsc_pmd_uc_diag_cmd_enum control, uint32_t timeout_ms) {
  return(falcon_tsc_pmd_uc_cmd( pa, CMD_DIAG_EN, (uint8_t) control, timeout_ms));
}



/************************************************************/
/*      Serdes IP RAM access - Lane RAM Variables           */
/*----------------------------------------------------------*/
/*   - through Micro Register Interface for PMD IPs         */
/*   - through Serdes FW DSC Command Interface for Gallardo */
/************************************************************/

/* Micro RAM Lane Byte Read */
uint8_t falcon_tsc_rdbl_uc_var( const phymod_access_t *pa, err_code_t *err_code_p, uint16_t addr) {
	uint8_t rddata;



	if(!err_code_p) {
		return(0);
	}

	EPSTM(rddata = falcon_tsc_rdb_uc_ram( pa, err_code_p, (0x400+addr+(falcon_tsc_get_lane(pa)*0x100)))); /* Use Micro register interface for reading RAM */

	return (rddata);
} 

/* Micro RAM Lane Byte Signed Read */
int8_t falcon_tsc_rdbls_uc_var( const phymod_access_t *pa, err_code_t *err_code_p, uint16_t addr) {
  return ((int8_t) falcon_tsc_rdbl_uc_var( pa, err_code_p, addr));
} 

/* Micro RAM Lane Word Read */
uint16_t falcon_tsc_rdwl_uc_var( const phymod_access_t *pa, err_code_t *err_code_p, uint16_t addr) {
  uint16_t rddata;

  if(!err_code_p) {
	  return(0);
  }

  if (addr%2 != 0) {                                                                /* Validate even address */
	  *err_code_p = ERR_CODE_INVALID_RAM_ADDR;
	  return (0);
  }

  EPSTM(rddata = falcon_tsc_rdw_uc_ram( pa, err_code_p, (0x400+addr+(falcon_tsc_get_lane(pa)*0x100)))); /* Use Micro register interface for reading RAM */

  return (rddata);
}


/* Micro RAM Lane Word Signed Read */
int16_t falcon_tsc_rdwls_uc_var( const phymod_access_t *pa, err_code_t *err_code_p, uint16_t addr) {
  return ((int16_t) falcon_tsc_rdwl_uc_var( pa, err_code_p, addr));
}
  
/* Micro RAM Lane Byte Write */
err_code_t falcon_tsc_wrbl_uc_var( const phymod_access_t *pa, uint16_t addr, uint8_t wr_val) {


    return (falcon_tsc_wrb_uc_ram( pa, (0x400+addr+(falcon_tsc_get_lane(pa)*0x100)), wr_val));    /* Use Micro register interface for writing RAM */
}

/* Micro RAM Lane Byte Signed Write */
err_code_t falcon_tsc_wrbls_uc_var( const phymod_access_t *pa, uint16_t addr, int8_t wr_val) {
  return (falcon_tsc_wrbl_uc_var( pa, addr, wr_val));
}

/* Micro RAM Lane Word Write */
err_code_t falcon_tsc_wrwl_uc_var( const phymod_access_t *pa, uint16_t addr, uint16_t wr_val) {


	if (addr%2 != 0) {                                                                /* Validate even address */
		return (_error(ERR_CODE_INVALID_RAM_ADDR));
	}
    return (falcon_tsc_wrw_uc_ram( pa, (0x400+addr+(falcon_tsc_get_lane(pa)*0x100)), wr_val));    /* Use Micro register interface for writing RAM */
}

/* Micro RAM Lane Word Signed Write */
err_code_t falcon_tsc_wrwls_uc_var( const phymod_access_t *pa, uint16_t addr, int16_t wr_val) {
  return (falcon_tsc_wrwl_uc_var( pa, addr,wr_val));
}


/************************************************************/
/*      Serdes IP RAM access - Core RAM Variables           */
/*----------------------------------------------------------*/
/*   - through Micro Register Interface for PMD IPs         */
/*   - through Serdes FW DSC Command Interface for Gallardo */
/************************************************************/

/* Micro RAM Core Byte Read */
uint8_t falcon_tsc_rdbc_uc_var( const phymod_access_t *pa, err_code_t *err_code_p, uint8_t addr) {

  uint8_t rddata;

  if(!err_code_p) {
	  return(0);
  }

  EPSTM(rddata = falcon_tsc_rdb_uc_ram( pa, err_code_p, (0x50+addr)));                      /* Use Micro register interface for reading RAM */

  return (rddata);
} 

/* Micro RAM Core Byte Signed Read */
int8_t falcon_tsc_rdbcs_uc_var( const phymod_access_t *pa, err_code_t *err_code_p, uint8_t addr) {
  return ((int8_t) falcon_tsc_rdbc_uc_var( pa, err_code_p, addr));
}

/* Micro RAM Core Word Read */
uint16_t falcon_tsc_rdwc_uc_var( const phymod_access_t *pa, err_code_t *err_code_p, uint8_t addr) {

  uint16_t rddata;

  if(!err_code_p) {
	  return(0);
  }
  if (addr%2 != 0) {                                                                /* Validate even address */
	  *err_code_p = ERR_CODE_INVALID_RAM_ADDR;
	  return (0);
  }

  EPSTM(rddata = falcon_tsc_rdw_uc_ram( pa, err_code_p, (0x50+addr)));                  /* Use Micro register interface for reading RAM */

  return (rddata);
}

/* Micro RAM Core Word Signed Read */
int16_t falcon_tsc_rdwcs_uc_var( const phymod_access_t *pa, err_code_t *err_code_p, uint8_t addr) {
  return ((int16_t) falcon_tsc_rdwc_uc_var( pa, err_code_p, addr));
}

/* Micro RAM Core Byte Write  */
err_code_t falcon_tsc_wrbc_uc_var( const phymod_access_t *pa, uint8_t addr, uint8_t wr_val) {


    return (falcon_tsc_wrb_uc_ram( pa, (0x50+addr), wr_val));                                /* Use Micro register interface for writing RAM */
} 


/* Micro RAM Core Byte Signed Write */
err_code_t falcon_tsc_wrbcs_uc_var( const phymod_access_t *pa, uint8_t addr, int8_t wr_val) {
  return (falcon_tsc_wrbc_uc_var( pa, addr, wr_val));
}

/* Micro RAM Core Word Write  */
err_code_t falcon_tsc_wrwc_uc_var( const phymod_access_t *pa, uint8_t addr, uint16_t wr_val) {


	if (addr%2 != 0) {                                                                /* Validate even address */
		return (_error(ERR_CODE_INVALID_RAM_ADDR));
	}
    return (falcon_tsc_wrw_uc_ram( pa, (0x50+addr), wr_val));                                 /* Use Micro register interface for writing RAM */
}

/* Micro RAM Core Word Signed Write */
err_code_t falcon_tsc_wrwcs_uc_var( const phymod_access_t *pa, uint8_t addr, int16_t wr_val) {
  return(falcon_tsc_wrwc_uc_var( pa, addr,wr_val));
}



/*******************************************************************/
/*  APIs to Write Core/Lane Config and User variables into uC RAM  */
/*******************************************************************/

err_code_t falcon_tsc_set_uc_core_config( const phymod_access_t *pa, struct falcon_tsc_uc_core_config_st struct_val) {
  _update_uc_core_config_word( pa, &struct_val);
  return(wrcv_config_word(struct_val.word));
}

err_code_t falcon_tsc_set_usr_ctrl_core_event_log_level( const phymod_access_t *pa, uint8_t core_event_log_level) {
  return(wrcv_usr_ctrl_core_event_log_level(core_event_log_level));
}

err_code_t falcon_tsc_set_uc_lane_cfg( const phymod_access_t *pa, struct falcon_tsc_uc_lane_config_st struct_val) {
  _update_uc_lane_config_word( pa, &struct_val);
  return(wrv_config_word(struct_val.word));
}

err_code_t falcon_tsc_set_usr_ctrl_lane_event_log_level( const phymod_access_t *pa, uint8_t lane_event_log_level) {
  return(wrv_usr_ctrl_lane_event_log_level(lane_event_log_level));
}

err_code_t falcon_tsc_set_usr_ctrl_disable_startup( const phymod_access_t *pa, struct falcon_tsc_usr_ctrl_disable_functions_st set_val) {
  _update_usr_ctrl_disable_functions_byte( pa, &set_val);
  return(wrv_usr_ctrl_disable_startup_functions_word(set_val.word));
}

err_code_t falcon_tsc_set_usr_ctrl_disable_startup_dfe( const phymod_access_t *pa, struct falcon_tsc_usr_ctrl_disable_dfe_functions_st set_val) {
  _update_usr_ctrl_disable_dfe_functions_byte( pa, &set_val);
  return(wrv_usr_ctrl_disable_startup_dfe_functions_byte(set_val.byte));
}

err_code_t falcon_tsc_set_usr_ctrl_disable_steady_state( const phymod_access_t *pa, struct falcon_tsc_usr_ctrl_disable_functions_st set_val) {
  _update_usr_ctrl_disable_functions_byte( pa, &set_val);
  return(wrv_usr_ctrl_disable_steady_state_functions_word(set_val.word));
}

err_code_t falcon_tsc_set_usr_ctrl_disable_steady_state_dfe( const phymod_access_t *pa, struct falcon_tsc_usr_ctrl_disable_dfe_functions_st set_val) {
  _update_usr_ctrl_disable_dfe_functions_byte( pa, &set_val);
  return(wrv_usr_ctrl_disable_steady_state_dfe_functions_byte(set_val.byte));
}



/******************************************************************/
/*  APIs to Read Core/Lane Config and User variables from uC RAM  */
/******************************************************************/

err_code_t falcon_tsc_get_uc_core_config( const phymod_access_t *pa, struct falcon_tsc_uc_core_config_st *get_val) {

  if(!get_val) {
	 return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }

  ESTM(get_val->word = rdcv_config_word());
  _update_uc_core_config_st( pa, get_val);

  return (ERR_CODE_NONE);
}

err_code_t falcon_tsc_get_usr_ctrl_core_event_log_level( const phymod_access_t *pa, uint8_t *core_event_log_level) {

  if(!core_event_log_level) {
	 return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }

  ESTM(*core_event_log_level = rdcv_usr_ctrl_core_event_log_level());

  return (ERR_CODE_NONE);
}

err_code_t falcon_tsc_get_uc_lane_cfg( const phymod_access_t *pa, struct falcon_tsc_uc_lane_config_st *get_val) { 

  if(!get_val) {
	 return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }

  ESTM(get_val->word = rdv_config_word());
  _update_uc_lane_config_st( pa, get_val);
  return (ERR_CODE_NONE);
}

err_code_t falcon_tsc_get_usr_ctrl_lane_event_log_level( const phymod_access_t *pa, uint8_t *lane_event_log_level) {

  if(!lane_event_log_level) {
	 return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }

  ESTM(*lane_event_log_level = rdv_usr_ctrl_lane_event_log_level());
  return (ERR_CODE_NONE);
}

err_code_t falcon_tsc_get_usr_ctrl_disable_startup( const phymod_access_t *pa, struct falcon_tsc_usr_ctrl_disable_functions_st *get_val) {

  if(!get_val) {
	 return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }

  ESTM(get_val->word = rdv_usr_ctrl_disable_startup_functions_word());
  _update_usr_ctrl_disable_functions_st( pa, get_val);
  return (ERR_CODE_NONE);
}

err_code_t falcon_tsc_get_usr_ctrl_disable_startup_dfe( const phymod_access_t *pa, struct falcon_tsc_usr_ctrl_disable_dfe_functions_st *get_val) {

  if(!get_val) {
	 return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }

  ESTM(get_val->byte = rdv_usr_ctrl_disable_startup_dfe_functions_byte());
  _update_usr_ctrl_disable_dfe_functions_st( pa, get_val);
  return (ERR_CODE_NONE);
}

err_code_t falcon_tsc_get_usr_ctrl_disable_steady_state( const phymod_access_t *pa, struct falcon_tsc_usr_ctrl_disable_functions_st *get_val) {

  if(!get_val) {
	 return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }

  ESTM(get_val->word = rdv_usr_ctrl_disable_steady_state_functions_word());
  _update_usr_ctrl_disable_functions_st( pa, get_val);
  return (ERR_CODE_NONE);
}

err_code_t falcon_tsc_get_usr_ctrl_disable_steady_state_dfe( const phymod_access_t *pa, struct falcon_tsc_usr_ctrl_disable_dfe_functions_st *get_val) {

  if(!get_val) {
	 return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }

  ESTM(get_val->byte = rdv_usr_ctrl_disable_steady_state_dfe_functions_byte());
  _update_usr_ctrl_disable_dfe_functions_st( pa, get_val);
  return (ERR_CODE_NONE);
}



/******************************************/
/*  Serdes Register field Poll functions  */
/******************************************/

/* poll for microcontroller to populate the dsc_data register */
err_code_t falcon_tsc_poll_diag_done( const phymod_access_t *pa, uint16_t *status, uint32_t timeout_ms) {
 uint8_t loop;

 if(!status) {
	 return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }

 for(loop=0;loop < 100; loop++) {
	 ESTM(*status=rdv_usr_diag_status());

	 if((*status & 0x8000) > 0) {
		return(ERR_CODE_NONE);
	 }
	 if(loop>10) {
		 EFUN(falcon_tsc_delay_us(10*timeout_ms));
	 }
 }
 return(_error(ERR_CODE_DIAG_TIMEOUT));
}

/* poll for microcontroller to populate the dsc_data register */
err_code_t falcon_tsc_poll_diag_eye_data(const phymod_access_t *pa,uint32_t *data,uint16_t *status, uint32_t timeout_ms) {
 uint8_t loop;
 uint16_t dscdata;
 if(!data || !status) {
	 return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }

 for(loop=0;loop < 100; loop++) {
	 ESTM(*status=rdv_usr_diag_status());
	 if(((*status & 0x00FF) > 2) || ((*status & 0x8000) > 0)) {
		EFUN(falcon_tsc_pmd_uc_cmd( pa,  CMD_READ_DIAG_DATA_WORD, 0, 200));
        ESTM(dscdata = rd_uc_dsc_data());
		data[0] = _float8_to_int32((float8_t)(dscdata >>8));
		data[1] = _float8_to_int32((float8_t)(dscdata & 0x00FF));
		return(ERR_CODE_NONE);
	 }
	 if(loop>10) {
		 EFUN(falcon_tsc_delay_us(10*timeout_ms));
	 }
 }
 return(_error(ERR_CODE_DIAG_TIMEOUT));
}

#ifndef CUSTOM_REG_POLLING

/* Poll for field "uc_dsc_ready_for_cmd" = 1 [Return Val => Error_code (0 = Polling Pass)] */
err_code_t falcon_tsc_poll_uc_dsc_ready_for_cmd_equals_1( const phymod_access_t *pa, uint32_t timeout_ms) {
  
  uint16_t loop;
  err_code_t  err_code;
  /* read quickly for 4 tries */
  for (loop = 0; loop < 100; loop++) {
    uint16_t rddata;
    EFUN(falcon_tsc_pmd_rdt_reg(pa,DSC_A_DSC_UC_CTRL, &rddata));
    if (rddata & 0x0080) {    /* bit 7 is uc_dsc_ready_for_cmd */
      if (rddata & 0x0040) {  /* bit 6 is uc_dsc_error_found   */
		ESTM(USR_PRINTF(("ERROR : DSC command returned error (after cmd) cmd = x%x, supp_info = x%x !\n", rd_uc_dsc_gp_uc_req(), rd_uc_dsc_supp_info()))); 
		return(_error(ERR_CODE_UC_CMD_RETURN_ERROR));
      } 
      return (ERR_CODE_NONE);
    }     
	if(loop>10) {
		err_code = falcon_tsc_delay_us(10*timeout_ms);
		if(err_code) return(err_code);
	}
  }
  USR_PRINTF(("ERROR : DSC ready for command is not working, applying workaround and getting debug info !\n"));
  ESTM(DISP(rd_uc_dsc_supp_info()));
  ESTM(DISP(rd_uc_dsc_gp_uc_req()));
  ESTM(DISP(rd_dsc_state()));
  /* artifically terminate the command to re-enable the command interface */
  wr_uc_dsc_ready_for_cmd(0x1);        
  return (_error(ERR_CODE_POLLING_TIMEOUT));          /* Error Code for polling timeout */
}    

/* Poll for field "dsc_state" = DSC_STATE_UC_TUNE [Return Val => Error_code (0 = Polling Pass)] */
err_code_t falcon_tsc_poll_dsc_state_equals_uc_tune( const phymod_access_t *pa, uint32_t timeout_ms) {
  
  uint16_t loop;
  err_code_t  err_code;
  /* poll 10 times to avoid longer delays later */
  for (loop = 0; loop < 100; loop++) {
    uint16_t dsc_state;
    ESTM(dsc_state = rd_dsc_state());
    if (dsc_state == DSC_STATE_UC_TUNE) {
      return (ERR_CODE_NONE);
    }    
	if(loop>10) {
		err_code = falcon_tsc_delay_us(10*timeout_ms);
		if(err_code) return(err_code);
	}
  }
  ESTM(USR_PRINTF(("DSC_STATE = %d\n", rd_dsc_state())));
  return (_error(ERR_CODE_POLLING_TIMEOUT));          /* Error Code for polling timeout */
}    




/* Poll for field "micro_ra_initdone" = 1 [Return Val => Error_code (0 = Polling Pass)] */
err_code_t falcon_tsc_poll_micro_ra_initdone( const phymod_access_t *pa, uint32_t timeout_ms) {
  
  uint16_t loop;
  err_code_t  err_code;
  uint8_t result;
  for (loop = 0; loop <= 100; loop++) {
    ESTM(result = rdc_micro_ra_initdone());
    if (result) {
      return (ERR_CODE_NONE);
    }    
    err_code = falcon_tsc_delay_us(10*timeout_ms);
    if (err_code) {
      return (err_code);
    }
  }
  return (_error(ERR_CODE_POLLING_TIMEOUT));          /* Error Code for polling timeout */
}

#endif /* CUSTOM_REG_POLLING */



/****************************************/
/*  Serdes Register/Variable Dump APIs  */
/****************************************/

err_code_t falcon_tsc_reg_dump( const phymod_access_t *pa ) {

  uint16_t addr, rddata;

  USR_PRINTF(("\n\n**********************************\n")); 
  USR_PRINTF(("****  SERDES REGISTER DUMP    ****\n")); 
  USR_PRINTF(("**********************************\n")); 
  USR_PRINTF(("****    ADDR      RD_VALUE    ****\n")); 
  USR_PRINTF(("**********************************\n"));

  for (addr = 0x0; addr < 0xF; addr++) {
    EFUN(falcon_tsc_pmd_rdt_reg(pa,addr,&rddata));
    USR_PRINTF(("       0x%04x      0x%04x\n",addr,rddata));
  }

  for (addr = 0x90; addr < 0x9F; addr++) {
    EFUN(falcon_tsc_pmd_rdt_reg(pa,addr,&rddata));
    USR_PRINTF(("       0x%04x      0x%04x\n",addr,rddata));
  }

  for (addr = 0xD000; addr < 0xD150; addr++) {
    EFUN(falcon_tsc_pmd_rdt_reg(pa,addr,&rddata));
    USR_PRINTF(("       0x%04x      0x%04x\n",addr,rddata));
  }

  for (addr = 0xD200; addr < 0xD230; addr++) {
    EFUN(falcon_tsc_pmd_rdt_reg(pa,addr,&rddata));
    USR_PRINTF(("       0x%04x      0x%04x\n",addr,rddata));
  }

  for (addr = 0xFFD0; addr < 0xFFE0; addr++) {
    EFUN(falcon_tsc_pmd_rdt_reg(pa,addr,&rddata));
    USR_PRINTF(("       0x%04x      0x%04x\n",addr,rddata));
  }
  return (ERR_CODE_NONE);
}


err_code_t falcon_tsc_uc_core_var_dump( const phymod_access_t *pa ) {

  uint8_t     addr, rddata;
  err_code_t  err_code = ERR_CODE_NONE;

  USR_PRINTF(("\n\n******************************************\n")); 
  USR_PRINTF(("**** SERDES UC CORE RAM VARIABLE DUMP ****\n")); 
  USR_PRINTF(("******************************************\n")); 
  USR_PRINTF(("****       ADDR       RD_VALUE        ****\n")); 
  USR_PRINTF(("******************************************\n")); 

  for (addr = 0x0; addr < 0xFF; addr++) {  
    rddata = falcon_tsc_rdbc_uc_var( pa, &err_code, addr);
    if (err_code) {
      return (err_code);
    }
    USR_PRINTF(("           0x%02x         0x%02x\n",addr,rddata));
  }
  return (ERR_CODE_NONE);
}


err_code_t falcon_tsc_uc_lane_var_dump( const phymod_access_t *pa ) {

  uint8_t     addr, rddata;
  err_code_t  err_code = ERR_CODE_NONE;

  USR_PRINTF(("\n\n********************************************\n")); 
  USR_PRINTF(("**** SERDES UC LANE %d RAM VARIABLE DUMP ****\n",falcon_tsc_get_lane(pa))); 
  USR_PRINTF(("********************************************\n")); 
  USR_PRINTF(("*****       ADDR       RD_VALUE        *****\n")); 
  USR_PRINTF(("********************************************\n")); 

    for (addr = 0x0; addr < 0xFF; addr++) {  
      rddata = falcon_tsc_rdbl_uc_var( pa, &err_code, addr);
      if (err_code) {
        return (err_code);
      }
      USR_PRINTF(("            0x%02x         0x%02x\n",addr,rddata));
    }
  return (ERR_CODE_NONE);
}



/************************/
/*  Serdes API Version  */
/************************/

err_code_t falcon_tsc_version( const phymod_access_t *pa, uint32_t *api_version) {

	if(!api_version) {
		return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
	}
	*api_version = 0xA10104;
	return (ERR_CODE_NONE);
}


/***************************************/
/*  API Function to Read Event Logger  */
/***************************************/

err_code_t falcon_tsc_read_event_log( const phymod_access_t *pa, uint8_t decode_enable) {
	return (ERR_CODE_NONE);
}


err_code_t falcon_tsc_display_state ( const phymod_access_t *pa ) {

  err_code_t err_code;
  err_code = ERR_CODE_NONE;

  if (!err_code) err_code = falcon_tsc_display_core_state( pa );
  if (!err_code) err_code = falcon_tsc_display_lane_state_hdr( pa );
  if (!err_code) err_code = falcon_tsc_display_lane_state( pa );

  return (err_code);
}

err_code_t falcon_tsc_display_config ( const phymod_access_t *pa ) {

  err_code_t err_code;
  err_code = ERR_CODE_NONE;

  if (!err_code) err_code = falcon_tsc_display_core_config( pa );
  if (!err_code) err_code = falcon_tsc_display_lane_config( pa );

  return (err_code);
}


/* Cofigure shared TX Pattern (Return Val = 0:PASS, 1-6:FAIL (reports 6 possible error scenarios if failed)) */
/* err_code_t falcon_tsc_config_shared_tx_pattern( const phymod_access_t *pa, uint8_t patt_length, const char pattern[]) { */
err_code_t falcon_tsc_config_shared_tx_pattern( const phymod_access_t *pa, uint8_t patt_length, const char pattern[]) {
   
  char       patt_final[241] = ""; 
  char       patt_mod[241]   = "", bin[5] = "";
  uint8_t    str_len, i, k, j = 0;
  uint8_t    offset_len, actual_patt_len = 0, hex = 0;
  uint8_t    zero_pad_len;
  uint16_t   patt_gen_wr_val = 0;
  uint8_t mode_sel; 

  err_code_t err_code;

  EFUN(_calc_patt_gen_mode_sel( pa, &mode_sel,&zero_pad_len,patt_length)); 

  /* Generating the appropriate write value to patt_gen_seq registers */
  str_len = (int8_t)USR_STRLEN(pattern);
 
  if ((str_len > 2) && ((USR_STRNCMP(pattern, "0x", 2)) == 0)) {
    /* Hexadecimal Pattern */
    for (i=2; i < str_len; i++) {
      err_code = _compute_bin( pa, pattern[i],bin);
      if (err_code) {
        return (err_code);
      }
      /* coverity[secure_coding] */
      USR_STRCAT(patt_mod,bin);  
      if (pattern[i] != '_') {
        actual_patt_len = actual_patt_len + 4;
        if (actual_patt_len > 240) {
          USR_PRINTF(("ERROR: Pattern bigger than max pattern length\n"));
          return (_error(ERR_CODE_CFG_PATT_PATTERN_BIGGER_THAN_MAXLEN));
        }
      }
    }
 
    offset_len = (actual_patt_len - patt_length);
    if ((offset_len > 3)  || (actual_patt_len < patt_length)) {
      USR_PRINTF(("ERROR: Pattern length provided does not match the hexadecimal pattern provided\n"));
      return (_error(ERR_CODE_CFG_PATT_LEN_MISMATCH));
    }  
    else if (offset_len) {
      for (i=0; i < offset_len; i++) {
        if (patt_mod[i] != '0') {
          USR_PRINTF(("ERROR: Pattern length provided does not match the hexadecimal pattern provided\n"));
          return (_error(ERR_CODE_CFG_PATT_LEN_MISMATCH));
        }
      }
      for (i=offset_len; i <= actual_patt_len; i++) {
        patt_mod[i - offset_len] = patt_mod[i];
      }
    }    
  }

  else {
    /* Binary Pattern */
    for (i=0; i < str_len; i++) {
      if ((pattern[i] == '0') || (pattern[i] == '1')) {
        bin[0] = pattern[i];
        bin[1] = '\0';
        USR_STRCAT(patt_mod,bin);   
        actual_patt_len++;
        if (actual_patt_len > 240) {
          USR_PRINTF(("ERROR: Pattern bigger than max pattern length\n"));
          return (_error(ERR_CODE_CFG_PATT_PATTERN_BIGGER_THAN_MAXLEN));
        }
      }
      else if (pattern[i] != '_') {
        USR_PRINTF(("ERROR: Invalid input Pattern\n"));
        return (_error(ERR_CODE_CFG_PATT_INVALID_PATTERN));
      } 
    } 
    
    if (actual_patt_len != patt_length) {
      USR_PRINTF(("ERROR: Pattern length provided does not match the binary pattern provided\n"));
      return (_error(ERR_CODE_CFG_PATT_LEN_MISMATCH));
    } 
  }

  /* Zero padding upper bits and concatinating patt_mod to form patt_final */
  for (i=0; i < zero_pad_len; i++) {
    USR_STRCAT(patt_final,"0");
    j++; 
  }
  for (i=zero_pad_len; i + patt_length < 241; i = i + patt_length) {  
    USR_STRCAT(patt_final,patt_mod);
    j++; 
  }
  
  /* USR_PRINTF(("\nFinal Pattern = %s\n\n",patt_final)); */

  for (i=0; i < 15; i++) {

    for (j=0; j < 4; j++) {
      k = i*16 + j*4;
      bin[0] = patt_final[k];
      bin[1] = patt_final[k+1];
      bin[2] = patt_final[k+2];
      bin[3] = patt_final[k+3];
      bin[4] = '\0';
      err_code = _compute_hex( pa, bin, &hex);
      if (err_code) {
        return (err_code);
      }
      patt_gen_wr_val = ((patt_gen_wr_val << 4) | hex); 
    }
    /* USR_PRINTF(("patt_gen_wr_val[%d] = 0x%x\n",(14-i),patt_gen_wr_val)); */

    /* Writing to apprpriate patt_gen_seq Registers */
    switch (i) {
      case 0:  wrc_patt_gen_seq_14(patt_gen_wr_val);
               break;
      case 1:  wrc_patt_gen_seq_13(patt_gen_wr_val);
               break;
      case 2:  wrc_patt_gen_seq_12(patt_gen_wr_val);
               break;
      case 3:  wrc_patt_gen_seq_11(patt_gen_wr_val);
               break;
      case 4:  wrc_patt_gen_seq_10(patt_gen_wr_val);
               break;
      case 5:  wrc_patt_gen_seq_9(patt_gen_wr_val);
               break;
      case 6:  wrc_patt_gen_seq_8(patt_gen_wr_val);
               break;
      case 7:  wrc_patt_gen_seq_7(patt_gen_wr_val);
               break;
      case 8:  wrc_patt_gen_seq_6(patt_gen_wr_val);
               break;
      case 9:  wrc_patt_gen_seq_5(patt_gen_wr_val);
               break;
      case 10: wrc_patt_gen_seq_4(patt_gen_wr_val);
               break;
      case 11: wrc_patt_gen_seq_3(patt_gen_wr_val);
               break;
      case 12: wrc_patt_gen_seq_2(patt_gen_wr_val);
               break;
      case 13: wrc_patt_gen_seq_1(patt_gen_wr_val);
               break;
      case 14: wrc_patt_gen_seq_0(patt_gen_wr_val);
               break;
      default: USR_PRINTF(("ERROR: Invalid write to patt_gen_seq register\n"));
               return (_error(ERR_CODE_CFG_PATT_INVALID_SEQ_WRITE));
               break;
    }
  }

  /* Pattern Generator Mode Select */
  /* wr_patt_gen_mode_sel(mode_sel); */
  /* USR_PRINTF(("Pattern gen Mode = %d\n",mode)); */
 
  /* Enable Fixed pattern Generation */
  /* wr_patt_gen_en(0x1);  */
  return(ERR_CODE_NONE); 
}


/* Enable/Disable Shared TX pattern generator */
err_code_t falcon_tsc_tx_shared_patt_gen_en( const phymod_access_t *pa, uint8_t enable, uint8_t patt_length) {
  uint8_t mode_sel,zero_pad_len;

  EFUN(_calc_patt_gen_mode_sel( pa, &mode_sel,&zero_pad_len,patt_length));

  if (enable) {
      if ((mode_sel < 1) || (mode_sel > 6)) {
        return (_error(ERR_CODE_PATT_GEN_INVALID_MODE_SEL));
      }
      mode_sel = (12 - mode_sel);                     
      wr_patt_gen_start_pos(mode_sel);                /* Start position for pattern */
      wr_patt_gen_stop_pos(0x0);                      /* Stop position for pattern */
    wr_patt_gen_en(0x1);                              /* Enable Fixed pattern Generation  */
  }
  else {
    wr_patt_gen_en(0x0);                              /* Disable Fixed pattern Generation  */
  }
  return(ERR_CODE_NONE); 
}

/**************************************/
/*  PMD_RX_LOCK and CL72/CL93 Status  */
/**************************************/

err_code_t falcon_tsc_pmd_lock_status( const phymod_access_t *pa, uint8_t *pmd_rx_lock) {
  if(!pmd_rx_lock) {
	  return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }

  ESTM(*pmd_rx_lock = rd_pmd_rx_lock());
  return (ERR_CODE_NONE);
}


err_code_t falcon_tsc_display_cl93n72_status( const phymod_access_t *pa ) {
 
  uint8_t rddata;
  USR_PRINTF(("\n\n***************************\n"));
  USR_PRINTF(("** LANE %d CL93n72 Status **\n",falcon_tsc_get_lane(pa)));
  USR_PRINTF(("***************************\n"));
  ESTM(rddata = rd_cl93n72_training_fsm_signal_detect());
  USR_PRINTF(("cl93n72_signal_detect         = %d   (1 = CL93n72 training FSM in SEND_DATA state;  0 = CL93n72 in training state)\n", rddata));

  ESTM(rddata = rd_cl93n72_ieee_training_failure());
  USR_PRINTF(("cl93n72_ieee_training_failure = %d   (1 = Training failure detected;                0 = Training failure not detected)\n", rddata));

  ESTM(rddata = rd_cl93n72_ieee_training_status());
  USR_PRINTF(("cl93n72_ieee_training_status  = %d   (1 = Start-up protocol in progress;            0 = Start-up protocol complete)\n", rddata));

  ESTM(rddata = rd_cl93n72_ieee_receiver_status());
  USR_PRINTF(("cl93n72_ieee_receiver_status  = %d   (1 = Receiver trained and ready to receive;    0 = Receiver training)\n\n", rddata));

  return(ERR_CODE_NONE); 
}




/**********************************/
/*  Serdes TX disable/RX Restart  */
/**********************************/

err_code_t falcon_tsc_tx_disable( const phymod_access_t *pa, uint8_t enable) {

  if (enable) {
    wr_sdk_tx_disable(0x1);
  }
  else {
    wr_sdk_tx_disable(0x0);
  }
  return(ERR_CODE_NONE); 
}


err_code_t falcon_tsc_rx_restart( const phymod_access_t *pa, uint8_t enable) { 

  wr_rx_restart_pmd_hold(enable);
  return(ERR_CODE_NONE); 
}


/******************************************************/
/*  Single function to set/get all RX AFE parameters  */
/******************************************************/

err_code_t falcon_tsc_write_rx_afe( const phymod_access_t *pa, enum falcon_tsc_rx_afe_settings_enum param, int8_t val) {
  /* Assumes the micro is not actively tuning */

  switch(param) {
    case RX_AFE_PF: 
    	return(_set_rx_pf_main( pa, val));	 
    
    case RX_AFE_PF2:
    	return(_set_rx_pf2( pa, val));
    
    case RX_AFE_VGA:
      return(_set_rx_vga( pa, val)); 
    
    case RX_AFE_DFE1:
    	return(_set_rx_dfe1( pa, val));
    
    case RX_AFE_DFE2:
    	return(_set_rx_dfe2( pa, val));
    
    case RX_AFE_DFE3:
    	return(_set_rx_dfe3( pa, val)); 
    
    case RX_AFE_DFE4:
    	return(_set_rx_dfe4( pa, val)); 
    
    case RX_AFE_DFE5:
    	return(_set_rx_dfe5( pa, val)); 
   
    case RX_AFE_DFE6:           
	return (_set_rx_dfe6( pa, val));  

    case RX_AFE_DFE7:          
	return (_set_rx_dfe7( pa, val));  

    case RX_AFE_DFE8:          
	return (_set_rx_dfe8( pa, val));  
    
    case RX_AFE_DFE9:          
	return (_set_rx_dfe9( pa, val));  
    
    case RX_AFE_DFE10:         
	return (_set_rx_dfe10( pa, val));  
    
    case RX_AFE_DFE11:     
        return (_set_rx_dfe11( pa, val));  

    case RX_AFE_DFE12: 
        return (_set_rx_dfe12( pa, val));  
    
    case RX_AFE_DFE13: 
        return (_set_rx_dfe13( pa, val));  
    
    case RX_AFE_DFE14: 
        return (_set_rx_dfe14( pa, val));  
    default:
	return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }
}


err_code_t falcon_tsc_read_rx_afe( const phymod_access_t *pa, enum falcon_tsc_rx_afe_settings_enum param, int8_t *val) {
  /* Assumes the micro is not actively tuning */
  if(!val) {
  	return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }
  
  switch(param) {
    case RX_AFE_PF: 
    	EFUN(_get_rx_pf_main( pa, val)); 
    	break;
    case RX_AFE_PF2:
    	EFUN(_get_rx_pf2( pa, val)); 
    	break;
    case RX_AFE_VGA:
    	EFUN(_get_rx_vga( pa, val)); 
      break;
    
    case RX_AFE_DFE1:
    	EFUN(_get_rx_dfe1( pa, val));
    	break;
    case RX_AFE_DFE2:
    	EFUN(_get_rx_dfe2( pa, val));
    	break;
    case RX_AFE_DFE3:
    	EFUN(_get_rx_dfe3( pa, val));
    	break;
    case RX_AFE_DFE4:
    	EFUN(_get_rx_dfe4( pa, val));
    	break;
    case RX_AFE_DFE5:
    	EFUN(_get_rx_dfe5( pa, val));
    	break;
    case RX_AFE_DFE6:
    	EFUN(_get_rx_dfe6( pa, val));
    	break;
    case RX_AFE_DFE7:
    	EFUN(_get_rx_dfe7( pa, val));
    	break;
    case RX_AFE_DFE8:
    	EFUN(_get_rx_dfe8( pa, val));
    	break;
    case RX_AFE_DFE9:
    	EFUN(_get_rx_dfe9( pa, val));
    	break;
    case RX_AFE_DFE10:
    	EFUN(_get_rx_dfe10( pa, val));
    	break;
    case RX_AFE_DFE11:
    	EFUN(_get_rx_dfe11( pa, val));
    	break;
    case RX_AFE_DFE12:
    	EFUN(_get_rx_dfe12( pa, val));
    	break;
    case RX_AFE_DFE13:
    	EFUN(_get_rx_dfe13( pa, val));
    	break;
    case RX_AFE_DFE14:
    	EFUN(_get_rx_dfe14( pa, val));
    	break;
    default:
    	return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }
  return(ERR_CODE_NONE);
}


err_code_t falcon_tsc_validate_txfir_cfg( const phymod_access_t *pa, int8_t pre, int8_t main, int8_t post1, int8_t post2, int8_t post3) {

  err_code_t err_code = ERR_CODE_NONE;

  if ((pre > 31) || (pre < 0)) 
    err_code = err_code | ERR_CODE_TXFIR_PRE_INVALID;  

  if ((main > 112) || (main < 0)) 
    err_code = err_code | ERR_CODE_TXFIR_MAIN_INVALID;  

  if ((post1 > 63) || (post1 < 0)) 
    err_code = err_code | ERR_CODE_TXFIR_POST1_INVALID;  

  if ((post2 > 15) || (post2 < -15)) 
    err_code = err_code | ERR_CODE_TXFIR_POST2_INVALID;  

  if ((post3 > 7) || (post3 < -7)) 
    err_code = err_code | ERR_CODE_TXFIR_POST3_INVALID;  
  
  if ((int16_t)(main + 48) < (int16_t)(pre + post1 + post2 + post3 + 1))
    err_code = err_code | ERR_CODE_TXFIR_V2_LIMIT;
  
  if ((int16_t)(pre + main + post1 + _abs(post2) + _abs(post3)) > 112)
    err_code = err_code | ERR_CODE_TXFIR_SUM_LIMIT;

  return (_error(err_code));  
}


err_code_t falcon_tsc_write_tx_afe( const phymod_access_t *pa, enum falcon_tsc_tx_afe_settings_enum param, int8_t val) {

  switch(param) {
    case TX_AFE_PRE:
    	return(_set_tx_pre( pa, val));
    case TX_AFE_MAIN:
    	return(_set_tx_main( pa, val));
    case TX_AFE_POST1:
    	return(_set_tx_post1( pa, val));
    case TX_AFE_POST2:
    	return(_set_tx_post2( pa, val));
    case TX_AFE_POST3:
    	return(_set_tx_post3( pa, val));
    case TX_AFE_AMP:
    	return(_set_tx_amp( pa, val));
    default:
    	return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }
}

err_code_t falcon_tsc_read_tx_afe( const phymod_access_t *pa, enum falcon_tsc_tx_afe_settings_enum param, int8_t *val) {
  if(!val) {
  	return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }
  
  switch(param) {
    case TX_AFE_PRE:
    	EFUN(_get_tx_pre( pa, val));
    	break;
    case TX_AFE_MAIN:
    	ESTM(_get_tx_main( pa, val));
    	break;
    case TX_AFE_POST1:
    	EFUN(_get_tx_post1( pa, val));
    	break;
    case TX_AFE_POST2:
    	EFUN(_get_tx_post2( pa, val));
    	break;
    case TX_AFE_POST3:
    	EFUN(_get_tx_post3( pa, val));
    	break;
    case TX_AFE_AMP:
    	EFUN(_get_tx_amp( pa, val));
    	break;
    default:
    	return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }
  return(ERR_CODE_NONE);
}

err_code_t falcon_tsc_apply_txfir_cfg( const phymod_access_t *pa, int8_t pre, int8_t main, int8_t post1, int8_t post2, int8_t post3) {
  
  err_code_t failcode = falcon_tsc_validate_txfir_cfg( pa, pre, main, post1, post2, post3);

  if (!failcode) {
    failcode |= _set_tx_pre( pa, pre);
    failcode |= _set_tx_main( pa, main);    
    failcode |= _set_tx_post1( pa, post1);
    failcode |= _set_tx_post2( pa, post2);
    failcode |= _set_tx_post3( pa, post3);
  }

  return (_error(failcode));  
}





/**************************************/
/*  PRBS Generator/Checker Functions  */
/**************************************/

/* Configure PRBS Generator */
err_code_t falcon_tsc_config_tx_prbs( const phymod_access_t *pa, enum falcon_tsc_prbs_polynomial_enum prbs_poly_mode, uint8_t prbs_inv) {

  wr_prbs_gen_mode_sel((uint8_t)prbs_poly_mode);        /* PRBS Generator mode sel */
  wr_prbs_gen_inv(prbs_inv);                            /* PRBS Invert Enable/Disable */
  /* To enable PRBS Generator */
  /* wr_prbs_gen_en(0x1); */
  return (ERR_CODE_NONE);
}

err_code_t falcon_tsc_get_tx_prbs_config( const phymod_access_t *pa, enum falcon_tsc_prbs_polynomial_enum *prbs_poly_mode, uint8_t *prbs_inv) {
    uint8_t val;

  ESTM(val = rd_prbs_gen_mode_sel());                   /* PRBS Generator mode sel */
  *prbs_poly_mode = (enum falcon_tsc_prbs_polynomial_enum)val;
  ESTM(val = rd_prbs_gen_inv());                        /* PRBS Invert Enable/Disable */
  *prbs_inv = val;

  return (ERR_CODE_NONE);
}

/* PRBS Generator Enable/Disable */
err_code_t falcon_tsc_tx_prbs_en( const phymod_access_t *pa, uint8_t enable) {

  if (enable) {
    wr_prbs_gen_en(0x1);                                /* Enable PRBS Generator */
  } 
  else { 
    wr_prbs_gen_en(0x0);                                /* Disable PRBS Generator */
  } 
  return (ERR_CODE_NONE);
}

/* Get PRBS Generator Enable/Disable */
err_code_t falcon_tsc_get_tx_prbs_en( const phymod_access_t *pa, uint8_t *enable) {

  ESTM(*enable = rd_prbs_gen_en());                                
 
  return (ERR_CODE_NONE);
}

/* PRBS 1-bit error injection */
err_code_t falcon_tsc_tx_prbs_err_inject( const phymod_access_t *pa, uint8_t enable) {
  /* PRBS Error Insert.
     0 to 1 transition on this signal will insert single bit error in the MSB bit of the data bus. 
     Reset value is 0x0.
  */
  if(enable)
    wr_prbs_gen_err_ins(0x1);
  wr_prbs_gen_err_ins(0);
  return (ERR_CODE_NONE);
}

/* Configure PRBS Checker */
err_code_t falcon_tsc_config_rx_prbs( const phymod_access_t *pa, enum falcon_tsc_prbs_polynomial_enum prbs_poly_mode, enum falcon_tsc_prbs_checker_mode_enum prbs_checker_mode, uint8_t prbs_inv) {

  wr_prbs_chk_mode_sel((uint8_t)prbs_poly_mode);       /* PRBS Checker Polynomial mode sel  */
  wr_prbs_chk_mode((uint8_t)prbs_checker_mode);        /* PRBS Checker mode sel (PRBS LOCK state machine select) */
  wr_prbs_chk_inv(prbs_inv);                           /* PRBS Invert Enable/Disable */
  /* To enable PRBS Checker */
  /* wr_prbs_chk_en(0x1); */
  return (ERR_CODE_NONE);
}

/* get PRBS Checker */
err_code_t falcon_tsc_get_rx_prbs_config( const phymod_access_t *pa, enum falcon_tsc_prbs_polynomial_enum *prbs_poly_mode, enum falcon_tsc_prbs_checker_mode_enum *prbs_checker_mode, uint8_t *prbs_inv) {
  uint8_t val;

  ESTM(val = rd_prbs_chk_mode_sel());                 /* PRBS Checker Polynomial mode sel  */
  *prbs_poly_mode = (enum falcon_tsc_prbs_polynomial_enum)val;
  ESTM(val = rd_prbs_chk_mode());                     /* PRBS Checker mode sel (PRBS LOCK state machine select) */
  *prbs_checker_mode = (enum falcon_tsc_prbs_checker_mode_enum)val;
  ESTM(val = rd_prbs_chk_inv());                      /* PRBS Invert Enable/Disable */
  *prbs_inv = val;
  return (ERR_CODE_NONE);
}

/* PRBS Checker Enable/Disable */
err_code_t falcon_tsc_rx_prbs_en( const phymod_access_t *pa, uint8_t enable) {

  if (enable) {
    wr_prbs_chk_en(0x1);                                /* Enable PRBS Checker */
  } 
  else { 
    wr_prbs_chk_en(0x0);                                /* Disable PRBS Checker */
  } 
  return (ERR_CODE_NONE);
}

err_code_t falcon_tsc_get_rx_prbs_en( const phymod_access_t *pa, uint8_t *enable) {

  ESTM(*enable = rd_prbs_chk_en());                                
 
  return (ERR_CODE_NONE);
}


/* PRBS Checker Lock State */
err_code_t falcon_tsc_prbs_chk_lock_state( const phymod_access_t *pa, uint8_t *chk_lock) {
  if(!chk_lock) {
	  return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }

  ESTM(*chk_lock = rd_prbs_chk_lock());                  /* PRBS Checker Lock Indication 1 = Locked, 0 = Out of Lock */
  return (ERR_CODE_NONE);
}

/* PRBS Error Count and Lock Lost (bit 31 in lock lost) */
err_code_t falcon_tsc_prbs_err_count_ll( const phymod_access_t *pa, uint32_t *prbs_err_cnt) {
  uint16_t rddata;
  if(!prbs_err_cnt) {
	  return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }

  EFUN(falcon_tsc_pmd_rdt_reg(pa,TLB_RX_PRBS_CHK_ERR_CNT_MSB_STATUS, &rddata));
  *prbs_err_cnt = ((uint32_t) rddata)<<16;
  ESTM(*prbs_err_cnt = (*prbs_err_cnt | rd_prbs_chk_err_cnt_lsb()));
  return (ERR_CODE_NONE);
}
   
/* PRBS Error Count State  */
err_code_t falcon_tsc_prbs_err_count_state( const phymod_access_t *pa, uint32_t *prbs_err_cnt, uint8_t *lock_lost) {

  if(!prbs_err_cnt || !lock_lost) {
	  return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }

  EFUN(falcon_tsc_prbs_err_count_ll( pa, prbs_err_cnt));
  *lock_lost    = (*prbs_err_cnt >> 31);
  *prbs_err_cnt = (*prbs_err_cnt & 0x7FFFFFFF);
  return (ERR_CODE_NONE);
}


/**********************************************/
/*  Loopback and Ultra-Low Latency Functions  */
/**********************************************/

/* Enable/Diasble Digital Loopback */
err_code_t falcon_tsc_dig_lpbk( const phymod_access_t *pa, uint8_t enable) {
    wr_rx_pi_manual_mode(!enable);
  wr_dig_lpbk_en(enable);                               /* 0 = diabled, 1 = enabled */
  return (ERR_CODE_NONE);
}

/* Locks TX_PI to Loop timing */
err_code_t falcon_tsc_loop_timing( const phymod_access_t *pa, uint8_t enable) {
  
  err_code_t err_code;
    uint8_t    osr_mode;

  if (enable) {

   

      wr_tx_pi_loop_timing_src_sel(0x1);                /* RX phase_sum_val_logic enable */
      ESTM(osr_mode = rd_osr_mode());
      if ((osr_mode == OSX16P5) || (osr_mode == OSX20P625)) {
        wr_rx_pi_manual_mode(0x1);
      }
      wrc_tx_pi_en(0x1);                                /* TX_PI enable: 0 = diabled, 1 = enabled */
      wrc_tx_pi_jitter_filter_en(0x1);                  /* Jitter filter enable to lock freq: 0 = diabled, 1 = enabled */

    err_code = falcon_tsc_delay_us(25);                     /* Wait for tclk to lock to CDR */
    if (err_code) {
      return(err_code);
    }
  }
  else {



      wrc_tx_pi_jitter_filter_en(0x0);                  /* Jitter filter enable to lock freq: 0 = diabled, 1 = enabled */
      wrc_tx_pi_en(0x0);                                /* TX_PI enable: 0 = diabled, 1 = enabled */
      wr_tx_pi_loop_timing_src_sel(0x0);                /* RX phase_sum_val_logic enable */

  }
  return (ERR_CODE_NONE);
}  


/* Setup Remote Loopback mode  */
err_code_t falcon_tsc_rmt_lpbk( const phymod_access_t *pa, uint8_t enable) {

  err_code_t err_code;

  if (enable) {
    err_code = falcon_tsc_loop_timing( pa, enable);
    if (err_code) {
      return(err_code);
    }
      wrc_tx_pi_ext_ctrl_en(0x1);                       /* PD path enable: 0 = diabled, 1 = enabled */
    wr_rmt_lpbk_en(0x1);                                /* Remote Loopback Enable: 0 = diabled, 1 = enable  */
    err_code = falcon_tsc_delay_us(50);                     /* Wait for rclk and tclk phase lock before expecing good data from rmt loopback */
    return(err_code);                                   /* Might require longer wait time for smaller values of tx_pi_ext_phase_bwsel_integ */
  }
  else {
    wr_rmt_lpbk_en(0x0);                                /* Remote Loopback Enable: 0 = diabled, 1 = enable  */
      wrc_tx_pi_ext_ctrl_en(0x0);                       /* PD path enable: 0 = diabled, 1 = enabled */
    err_code = falcon_tsc_loop_timing( pa, enable);
    return(err_code);
  }
}
    




/**********************************/
/*  TX_PI Jitter Generation APIs  */
/**********************************/

/* TX_PI Frequency Override (Fixed Frequency Mode) */
err_code_t falcon_tsc_tx_pi_freq_override( const phymod_access_t *pa, uint8_t enable, int16_t freq_override_val) {                  

   if (enable) {
     wrc_tx_pi_en(0x1);                                 /* TX_PI enable :            0 = disabled, 1 = enabled */
     wrc_tx_pi_freq_override_en(0x1);                   /* Fixed freq mode enable:   0 = disabled, 1 = enabled */
     wrc_tx_pi_freq_override_val(freq_override_val);    /* Fixed Freq Override Value (+/-8192) */
   } 
   else {
     wrc_tx_pi_freq_override_val(0);                    /* Fixed Freq Override Value to 0 */
     wrc_tx_pi_freq_override_en(0x0);                   /* Fixed freq mode enable:   0 = disabled, 1 = enabled */
     wrc_tx_pi_en(0x0);                                 /* TX_PI enable :            0 = disabled, 1 = enabled */
   }

  return (ERR_CODE_NONE);
}


/* TX_PI Sinusoidal or Spread-Spectrum (SSC) Jitter Generation  */
err_code_t falcon_tsc_tx_pi_jitt_gen( const phymod_access_t *pa, uint8_t enable, int16_t freq_override_val, enum falcon_tsc_tx_pi_freq_jit_gen_enum jit_type, uint8_t tx_pi_jit_freq_idx, uint8_t tx_pi_jit_amp) {

  err_code_t err_code;

  err_code = falcon_tsc_tx_pi_freq_override( pa, enable, freq_override_val);
  if (err_code) {
    return (err_code);
  }

    if (enable) {
      wrc_tx_pi_jit_freq_idx(tx_pi_jit_freq_idx);                             
      wrc_tx_pi_jit_amp(tx_pi_jit_amp);       
                      
      if (jit_type == TX_PI_SSC_HIGH_FREQ) { 
        wrc_tx_pi_jit_ssc_freq_mode(0x1);               /* SSC_FREQ_MODE:             0 = 6G SSC mode, 1 = 10G SSC mode */
        wrc_tx_pi_ssc_gen_en(0x1);                      /* SSC jitter enable:         0 = disabled,    1 = enabled */
      } 
      else if (jit_type == TX_PI_SSC_LOW_FREQ) {
        wrc_tx_pi_jit_ssc_freq_mode(0x0);               /* SSC_FREQ_MODE:             0 = 6G SSC mode, 1 = 10G SSC mode */
        wrc_tx_pi_ssc_gen_en(0x1);                      /* SSC jitter enable:         0 = disabled,    1 = enabled */
      }
      else if (jit_type == TX_PI_SJ) {
        wrc_tx_pi_sj_gen_en(0x1);                       /* Sinusoidal jitter enable:  0 = disabled,    1 = enabled */
      }
    } 
    else {
      wrc_tx_pi_ssc_gen_en(0x0);                        /* SSC jitter enable:         0 = disabled,    1 = enabled */
      wrc_tx_pi_sj_gen_en(0x0);                         /* Sinusoidal jitter enable:  0 = disabled,    1 = enabled */
    }
  return (ERR_CODE_NONE);
}


/***************************/
/*  Configure Serdes IDDQ  */
/***************************/

err_code_t falcon_tsc_core_config_for_iddq( const phymod_access_t *pa ) {

  /* Use frc/frc_val to force TX clk_vld signals to 0 */
  wrc_pmd_tx_clk_vld_frc_val(0x0);
  wrc_pmd_tx_clk_vld_frc(0x1);

  /* Switch all the lane clocks to comclk by writing to RX/TX comclk_sel registers */
  wrc_tx_s_comclk_sel(0x1);
  return (ERR_CODE_NONE);
}
 

err_code_t falcon_tsc_lane_config_for_iddq( const phymod_access_t *pa ) {

  /* Use frc/frc_val to force all RX and TX clk_vld signals to 0 */
  wr_pmd_rx_clk_vld_frc_val(0x0);
  wr_pmd_rx_clk_vld_frc(0x1);

 
  /* Use frc/frc_val to force all pmd_rx_lock signals to 0 */
  wr_rx_dsc_lock_frc_val(0x0);
  wr_rx_dsc_lock_frc(0x1);

  /* Switch all the lane clocks to comclk by writing to RX/TX comclk_sel registers */
  wr_ln_rx_s_comclk_sel(0x1);

  /* Assert all the AFE pwrdn/reset pins using frc/frc_val to make sure AFE is in lowest possible power mode */
  wr_afe_tx_pwrdn_frc_val(0x1);
  wr_afe_tx_pwrdn_frc(0x1);
  wr_afe_rx_pwrdn_frc_val(0x1);
  wr_afe_rx_pwrdn_frc(0x1);
  wr_afe_tx_reset_frc_val(0x1);
  wr_afe_tx_reset_frc(0x1);
  wr_afe_rx_reset_frc_val(0x1);
  wr_afe_rx_reset_frc(0x1);

  /* Set pmd_iddq pin to enable IDDQ */
  return (ERR_CODE_NONE);
} 


/*********************************/
/*  uc_active and uc_reset APIs  */
/*********************************/

/* Set the uC active mode */
err_code_t falcon_tsc_uc_active_enable( const phymod_access_t *pa, uint8_t enable) {
  wrc_uc_active(enable);
  return (ERR_CODE_NONE);
}



/* Enable or Disable the uC reset */
err_code_t falcon_tsc_uc_reset( const phymod_access_t *pa, uint8_t enable) {
  if (enable) { 
    /* Assert micro reset and reset all micro registers (all non-status registers written to default value) */
    wrc_micro_core_clk_en(0x0);                           /* Disable clock to M0 core */
    wrc_micro_master_clk_en(0x0);                         /* Disable clock to microcontroller subsystem */
    EFUN(falcon_tsc_pmd_wr_reg(pa,0xD200, 0x0000));
    EFUN(falcon_tsc_pmd_wr_reg(pa,0xD201, 0x0000));
    EFUN(falcon_tsc_pmd_wr_reg(pa,0xD202, 0x0000));
    EFUN(falcon_tsc_pmd_wr_reg(pa,0xD204, 0x0000));
    EFUN(falcon_tsc_pmd_wr_reg(pa,0xD205, 0x0000));
    EFUN(falcon_tsc_pmd_wr_reg(pa,0xD206, 0x0000));
    EFUN(falcon_tsc_pmd_wr_reg(pa,0xD207, 0x0000));
    EFUN(falcon_tsc_pmd_wr_reg(pa,0xD208, 0x0000));
    EFUN(falcon_tsc_pmd_wr_reg(pa,0xD209, 0x0000));
    EFUN(falcon_tsc_pmd_wr_reg(pa,0xD20A, 0x0000));
    EFUN(falcon_tsc_pmd_wr_reg(pa,0xD20B, 0x0000));
    EFUN(falcon_tsc_pmd_wr_reg(pa,0xD20C, 0x0000));
    EFUN(falcon_tsc_pmd_wr_reg(pa,0xD20D, 0x0000));
    EFUN(falcon_tsc_pmd_wr_reg(pa,0xD20E, 0x0000));
    EFUN(falcon_tsc_pmd_wr_reg(pa,0xD211, 0x0000));
    EFUN(falcon_tsc_pmd_wr_reg(pa,0xD212, 0x0000));
    EFUN(falcon_tsc_pmd_wr_reg(pa,0xD213, 0x0000));
    EFUN(falcon_tsc_pmd_wr_reg(pa,0xD214, 0x0000));
    EFUN(falcon_tsc_pmd_wr_reg(pa,0xD215, 0x0000));
    EFUN(falcon_tsc_pmd_wr_reg(pa,0xD216, 0x0007));
    EFUN(falcon_tsc_pmd_wr_reg(pa,0xD217, 0x0000));
    EFUN(falcon_tsc_pmd_wr_reg(pa,0xD218, 0x0000));
    EFUN(falcon_tsc_pmd_wr_reg(pa,0xD219, 0x0000));
    EFUN(falcon_tsc_pmd_wr_reg(pa,0xD21A, 0x0000));
    EFUN(falcon_tsc_pmd_wr_reg(pa,0xD21B, 0x0000));
    EFUN(falcon_tsc_pmd_wr_reg(pa,0xD220, 0x0000));
    EFUN(falcon_tsc_pmd_wr_reg(pa,0xD221, 0x0000));
    EFUN(falcon_tsc_pmd_wr_reg(pa,0xD224, 0x0000));
    EFUN(falcon_tsc_pmd_wr_reg(pa,0xD225, 0x0401));
  }
  else {
    /* De-assert micro reset - Start executing code */
    wrc_micro_core_rstb(0x1);                           
  }
 return (ERR_CODE_NONE);
}


/****************************************************/
/*  Serdes Powerdown, ClockGate and Deep_Powerdown  */
/****************************************************/

err_code_t falcon_tsc_core_pwrdn( const phymod_access_t *pa, enum core_pwrdn_mode_enum mode) {
  err_code_t err_code;
  switch(mode) {
    case PWR_ON:
            err_code = _falcon_tsc_core_clkgate( pa, 0);
            if (err_code) return (err_code);
            wrc_afe_s_pll_pwrdn(0x0);
            wrc_core_dp_s_rstb(0x1);
            break;
    case PWRDN:
            wrc_afe_s_pll_pwrdn(0x1);
            wrc_core_dp_s_rstb(0x0);
            break;
    case PWRDN_DEEP:
            wrc_afe_s_pll_pwrdn(0x1);
            wrc_core_dp_s_rstb(0x0);
            err_code = _falcon_tsc_core_clkgate( pa, 1);
            if (err_code) return (err_code);
            break;
    default : return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }
  return (ERR_CODE_NONE);
}

err_code_t falcon_tsc_lane_pwrdn( const phymod_access_t *pa, enum core_pwrdn_mode_enum mode) {
  err_code_t err_code;
  switch(mode) {
    case PWR_ON:
            wr_ln_tx_s_pwrdn(0x0);
            wr_ln_rx_s_pwrdn(0x0);
            err_code = _falcon_tsc_lane_clkgate( pa, 0);
            if (err_code) return (err_code);
            break;
    case PWRDN:
            wr_ln_tx_s_pwrdn(0x1);
            wr_ln_rx_s_pwrdn(0x1);
            break;
    case PWRDN_DEEP:
            wr_ln_tx_s_pwrdn(0x1);
            wr_ln_rx_s_pwrdn(0x1);
            err_code = _falcon_tsc_lane_clkgate( pa, 1);
            if (err_code) return (err_code);
            break;
    case PWRDN_TX:
            wr_ln_tx_s_pwrdn(0x1);
            break;
    case PWRDN_RX:
      wr_ln_rx_s_pwrdn(0x1);
            break;
    default : return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }
  return (ERR_CODE_NONE);
}


/*******************************/
/*  Isolate Serdes Input Pins  */
/*******************************/

err_code_t falcon_tsc_isolate_ctrl_pins( const phymod_access_t *pa, uint8_t enable) {

  if (enable) {
    wr_pmd_ln_tx_h_pwrdn_pkill(0x1);
    wr_pmd_ln_rx_h_pwrdn_pkill(0x1);
    wr_pmd_ln_dp_h_rstb_pkill(0x1);
    wr_pmd_ln_h_rstb_pkill(0x1);
    wrc_pmd_core_dp_h_rstb_pkill(0x1);
  }
  else {
    wr_pmd_ln_tx_h_pwrdn_pkill(0x0);
    wr_pmd_ln_rx_h_pwrdn_pkill(0x0);
    wr_pmd_ln_dp_h_rstb_pkill(0x0);
    wr_pmd_ln_h_rstb_pkill(0x0);
    wrc_pmd_core_dp_h_rstb_pkill(0x0);
  }
  return (ERR_CODE_NONE);
}




/***********************************************/
/*  Microcode Load into Program RAM Functions  */
/***********************************************/


  /* uCode Load through Register (MDIO) Interface [Return Val = Error_Code (0 = PASS)] */
  err_code_t falcon_tsc_ucode_mdio_load( const phymod_access_t *pa, uint8_t *ucode_image, uint16_t ucode_len) {

    uint16_t   ucode_len_padded, count = 0;
    uint16_t   wrdata_msw, wrdata_lsw; 
    uint8_t    wrdata_lsb; 
    err_code_t err_code;
	if(!ucode_image) {
		return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
	}
     
    if (ucode_len > UCODE_MAX_SIZE) {                     /* uCode size should be less than UCODE_MAX_SIZE  */
      return (_error(ERR_CODE_INVALID_UCODE_LEN));
    }  

    wrc_micro_master_clk_en(0x1);                         /* Enable clock to microcontroller subsystem */
    wrc_micro_master_rstb(0x1);                           /* De-assert reset to microcontroller sybsystem */
    wrc_micro_master_rstb(0x0);                           /* Assert reset to microcontroller sybsystem - Toggling reset*/
    wrc_micro_master_rstb(0x1);                           /* De-assert reset to microcontroller sybsystem */

    wrc_micro_ra_init(0x1);                               /* Set initialization command to initialize code RAM */
    err_code = falcon_tsc_poll_micro_ra_initdone( pa, 250);        /* Poll for micro_ra_initdone = 1 to indicate initialization done */
    if (err_code) {
      return (err_code);
    }

    wrc_micro_ra_init(0x0);                               /* Clear initialization command */

    ucode_len_padded = ((ucode_len + 3) & 0xFFFC);        /* Aligning ucode size to 4-byte boundary */

    /* Code to Load microcode */
    wrc_micro_autoinc_wraddr_en(0x1);                     /* To auto increment RAM write address */
    wrc_micro_ra_wrdatasize(0x2);                         /* Select 32bit transfers */
    wrc_micro_ra_wraddr_msw(0x0);                         /* Upper 16bits of start address of Program RAM where the ucode is to be loaded */
    wrc_micro_ra_wraddr_lsw(0x0);                         /* Lower 16bits of start address of Program RAM where the ucode is to be loaded */

    do {                                                  /* ucode_image loaded 32bits at a time */
      wrdata_lsb = (count < ucode_len) ? ucode_image[count] : 0x0; /* wrdata_lsb read from ucode_image; zero padded to 8byte boundary */
      count++;
      wrdata_lsw = (count < ucode_len) ? ucode_image[count] : 0x0; /* wrdata_msb read from ucode_image; zero padded to 8byte boundary */
      count++;
      wrdata_lsw = ((wrdata_lsw << 8) | wrdata_lsb);      /* 16bit wrdata_lsw formed from 8bit msb and lsb values read from ucode_image */

      wrdata_lsb = (count < ucode_len) ? ucode_image[count] : 0x0; /* wrdata_lsb read from ucode_image; zero padded to 8byte boundary */
      count++;
      wrdata_msw = (count < ucode_len) ? ucode_image[count] : 0x0; /* wrdata_msb read from ucode_image; zero padded to 8byte boundary */
      count++;
      wrdata_msw = ((wrdata_msw << 8) | wrdata_lsb);      /* 16bit wrdata_msw formed from 8bit msb and lsb values read from ucode_image */

      wrc_micro_ra_wrdata_msw(wrdata_msw);                /* Program RAM upper 16bits write data */ 
      wrc_micro_ra_wrdata_lsw(wrdata_lsw);                /* Program RAM lower 16bits write data */
    } while (count < ucode_len_padded);                   /* Loop repeated till entire image loaded (upto the 8byte boundary) */

    wrc_micro_core_clk_en(0x1);                           /* Enable clock to M0 core */
    /* De-assert reset to micro to start executing microcode */
    /* wrc_micro_core_rstb(0x1); */
    return (ERR_CODE_NONE);                               /* NO Errors while loading microcode (uCode Load PASS) */
  } 


  /* Read-back uCode from Program RAM and verify against ucode_image [Return Val = Error_Code (0 = PASS)]  */
  err_code_t falcon_tsc_ucode_load_verify( const phymod_access_t *pa, uint8_t *ucode_image, uint16_t ucode_len) {
   
    uint16_t ucode_len_padded, count = 0;
    uint16_t rddata_msw, rddata_lsw; 
    uint16_t data_msw, data_lsw; 
    uint8_t  rddata_lsb; 

    if(!ucode_image) {
      return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
    }
                
    ucode_len_padded = ((ucode_len + 3) & 0xFFFC);        /* Aligning ucode size to 4-byte boundary */
  
    if (ucode_len_padded > UCODE_MAX_SIZE) {              /* uCode size should be less than UCODE_MAX_SIZE */
      return (_error(ERR_CODE_INVALID_UCODE_LEN));
    }  
  
    wrc_micro_autoinc_rdaddr_en(0x1);                     /* To auto increment RAM read address */
    wrc_micro_ra_rddatasize(0x2);                         /* Select 32bit transfers */
    wrc_micro_ra_rdaddr_msw(0x0);                         /* Upper 16bits of start address of Program RAM from where to read ucode */
    wrc_micro_ra_rdaddr_lsw(0x0);                         /* Lower 16bits of start address of Program RAM from where to read ucode */

    do {                                                  /* ucode_image read 32bits at a time */
 
      rddata_lsb = (count < ucode_len) ? ucode_image[count] : 0x0; /* rddata_lsb read from ucode_image; zero padded to 8byte boundary */
      count++;
      rddata_lsw = (count < ucode_len) ? ucode_image[count] : 0x0; /* rddata_msb read from ucode_image; zero padded to 8byte boundary */
      count++;
      rddata_lsw = ((rddata_lsw << 8) | rddata_lsb);      /* 16bit rddata_lsw formed from 8bit msb and lsb values read from ucode_image */

      rddata_lsb = (count < ucode_len) ? ucode_image[count] : 0x0; /* rddata_lsb read from ucode_image; zero padded to 8byte boundary */
      count++;
      rddata_msw = (count < ucode_len) ? ucode_image[count] : 0x0; /* rddata_msb read from ucode_image; zero padded to 8byte boundary */
      count++;
      rddata_msw = ((rddata_msw << 8) | rddata_lsb);      /* 16bit rddata_msw formed from 8bit msb and lsb values read from ucode_image */

      /* Compare Program RAM ucode to ucode_image (Read to micro_ra_rddata_lsw reg auto-increments the ram_address) */

      ESTM(data_msw = rdc_micro_ra_rddata_msw());
      ESTM(data_lsw = rdc_micro_ra_rddata_lsw());
      if ((data_msw != rddata_msw) || (data_lsw != rddata_lsw)) {
        USR_PRINTF(("Ucode_Load_Verify_FAIL: Addr = 0x%x: Read_data = 0x%x, 0x%x :  Expected_data = 0x%x, 0x%x\n",(count-4),data_msw,data_lsw,rddata_msw,rddata_lsw));
	return (_error(ERR_CODE_UCODE_VERIFY_FAIL));      /* Verify uCode FAIL */
      }
	 
    } while (count < ucode_len_padded);                   /* Loop repeated till entire image loaded (upto the 8byte boundary) */

    return (ERR_CODE_NONE);                               /* Verify uCode PASS */
  }




/*************************************************/
/*  RAM access through Micro Register Interface  */
/*************************************************/
 
 
  /* Micro RAM Word Write */
  err_code_t falcon_tsc_wrw_uc_ram( const phymod_access_t *pa, uint16_t addr, uint16_t wr_val) {        
    
    wrc_micro_ra_wrdatasize(0x1);                       /* Select 16bit write datasize */    
    wrc_micro_ra_wraddr_msw(0x0);                       /* Upper 16bits of RAM address to be written to */
    wrc_micro_ra_wraddr_lsw(addr);                      /* Lower 16bits of RAM address to be written to */
    wrc_micro_ra_wrdata_lsw(wr_val);                    /* uC RAM lower 16bits write data */ 
    return (ERR_CODE_NONE);                               
  }

  /* Micro RAM Byte Write */
  err_code_t falcon_tsc_wrb_uc_ram( const phymod_access_t *pa, uint16_t addr, uint8_t wr_val) {

    wrc_micro_ra_wrdatasize(0x0);                       /* Select 8bit write datasize */    
    wrc_micro_ra_wraddr_msw(0x0);                       /* Upper 16bits of RAM address to be written to */
    wrc_micro_ra_wraddr_lsw(addr);                      /* Lower 16bits of RAM address to be written to */
    wrc_micro_ra_wrdata_lsw(wr_val);                    /* uC RAM lower 16bits write data */ 
    return (ERR_CODE_NONE);                               
  }

  /* Micro RAM Word Read */
  uint16_t falcon_tsc_rdw_uc_ram( const phymod_access_t *pa, err_code_t *err_code_p, uint16_t addr) {      
   uint16_t rddata;
   if(!err_code_p) {
     return(0);
   }
   
    wrc_micro_ra_rddatasize(0x1);                       /* Select 16bit read datasize */
    wrc_micro_ra_rdaddr_msw(0x0);                       /* Upper 16bits of RAM address to be read */
    wrc_micro_ra_rdaddr_lsw(addr);                      /* Lower 16bits of RAM address to be read */
    EPSTM(rddata = rdc_micro_ra_rddata_lsw());                /* 16bit read data */
    *err_code_p = ERR_CODE_NONE;
    return (rddata);
  }
  
  /* Micro RAM Byte Read */
  uint8_t falcon_tsc_rdb_uc_ram( const phymod_access_t *pa, err_code_t *err_code_p, uint16_t addr) {      
    uint8_t rddata;
    if(!err_code_p) {
      return(0);
     }
  
    wrc_micro_ra_rddatasize(0x1);                       /* Select 16bit read datasize */
    wrc_micro_ra_rdaddr_msw(0x0);                       /* Upper 16bits of RAM address to be read */
    wrc_micro_ra_rdaddr_lsw(addr);                      /* Lower 16bits of RAM address to be read */
    EPSTM(rddata = (uint8_t) rdc_micro_ra_rddata_lsw());      /* 16bit read data */
    *err_code_p = ERR_CODE_NONE;
    return (rddata);
  }


  /* Micro RAM Word Signed Write */
  err_code_t falcon_tsc_wrws_uc_ram( const phymod_access_t *pa, uint16_t addr, int16_t wr_val) { 
    return (falcon_tsc_wrw_uc_ram( pa, addr, wr_val));
  }

  /* Micro RAM Byte Signed Write */
  err_code_t falcon_tsc_wrbs_uc_ram( const phymod_access_t *pa, uint16_t addr, int8_t wr_val) {
    return (falcon_tsc_wrb_uc_ram( pa, addr, wr_val));  
  }

  /* Micro RAM Word Signed Read */
  int16_t falcon_tsc_rdws_uc_ram( const phymod_access_t *pa, err_code_t *err_code_p, uint16_t addr) {      
    return ((int16_t)falcon_tsc_rdw_uc_ram( pa, err_code_p, addr));
  }

  /* Micro RAM Byte Signed Read */
  int8_t falcon_tsc_rdbs_uc_ram( const phymod_access_t *pa, err_code_t *err_code_p, uint16_t addr) {      
    return ((int8_t)falcon_tsc_rdb_uc_ram( pa, err_code_p, addr));
  }
  




