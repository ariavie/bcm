/***********************************************************************************/
/***********************************************************************************/
/*  File Name     :  eagle_tsc_functions.c                                        */
/*  Created On    :  29/04/2013                                                    */
/*  Created By    :  Kiran Divakar                                                 */
/*  Description   :  APIs for Serdes IPs                                           */
/*  Revision      :  $Id: eagle_tsc_functions.c 535 2014-05-23 23:10:44Z kirand $ */
/*                                                                                 */
/*  Copyright: (c) 2014 Broadcom Corporation All Rights Reserved.                 */
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
 *  $Id: 98df9209fd1a1103448c5de9ad2496544b11e2ce $
*/


/** @file eagle_tsc_functions.c
 * Implementation of API functions
 */

#include <phymod/phymod.h>

#ifdef _MSC_VER
/* Enclose all standard headers in a pragma to remove warings for MS compiler */
#pragma warning( push, 0 )
#endif

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#include "eagle_tsc_functions.h"

#include "eagle_tsc_field_access_c.h"
#include "eagle_tsc_internal_c.h"
#include "eagle_tsc_dv_functions_c.h"


  #include "eagle_tsc_pll_config_c.h"




/************************************/
/*  Display Eye Scan                */
/************************************/

/* This is best method for terminal ASCII display */
err_code_t eagle_tsc_display_eye_scan( const phymod_access_t *pa ) {
  int8_t y;
  err_code_t err_code;
  uint16_t status = 0;
  uint32_t stripe[64];
  uint8_t data_thresh;

  /* start horizontal acquisition */
  err_code = eagle_tsc_meas_eye_scan_start( pa, EYE_SCAN_HORIZ);
  if (err_code) {
    eagle_tsc_meas_eye_scan_done( pa );
    return (err_code);
  }
    ESTM(data_thresh = rd_p1_thresh_sel());

  eagle_tsc_display_eye_scan_header( pa, 1);
  /* display stripe */
   for (y = 31;y>=-31;y=y-1) { 
 	err_code = eagle_tsc_read_eye_scan_stripe( pa, &stripe[0], &status);
	if (err_code) {
		eagle_tsc_meas_eye_scan_done( pa );
		return (err_code);
	}
	eagle_tsc_display_eye_scan_stripe( pa, y-data_thresh,&stripe[0]);
	USR_PRINTF(("\n"));  
  }  
  /* stop acquisition */
  err_code = eagle_tsc_meas_eye_scan_done( pa );
  if (err_code) {
	return (err_code);
  }

  eagle_tsc_display_eye_scan_footer( pa, 1);

  return(ERR_CODE_NONE);
}

/* This function is for Falcon and is configured for passive mode */
err_code_t eagle_tsc_meas_lowber_eye( const phymod_access_t *pa, const struct eagle_tsc_eyescan_options_st eyescan_options, uint32_t *buffer) {
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
        EFUN(eagle_tsc_pmd_uc_cmd_with_data( pa, CMD_DIAG_EN,CMD_UC_DIAG_GET_EYE_SAMPLE,((uint16_t)x)<<8 | (uint8_t)y,200));
        /* wait for sample complete */
#if 0
        do {
            eagle_tsc_delay_us(1000);
            ESTM(status=rdv_usr_diag_status());
            USR_PRINTF(("status=%04x\n",status));
        } while((status & 0x8000) == 0); 
#else
        EFUN(eagle_tsc_poll_diag_done( pa, &status, (((uint32_t)timeout)<<7)*10 + 20000));
#endif
        {
            uint8_t osr_mode; ESTM(osr_mode = rd_osr_mode());
            if(osr_mode == OSX1) {
                EFUN(eagle_tsc_prbs_err_count_ll( pa, &errors));
            } else if(osr_mode == OSX2) {
                ESTM(errors = ((uint32_t)rdv_usr_var_msb())<<16 | rdv_usr_var_lsb());
            } else {
                USR_PRINTF(("Error: 2D eye scan is not supported for OSR Mode > 2\n"));
                return(ERR_CODE_BAD_PTR_OR_INVALID_INPUT);
            }
        }
        /* USR_PRINTF(("(%d,%d) = %u\n",x,y,errors & 0x7FFFFFF)); */
        buffer[i] = errors & 0x7FFFFFFF;
        i++;
        USR_PRINTF(("."));
    }
    USR_PRINTF(("\n"));        
  }
  USR_PRINTF(("\n"));
  EFUN(eagle_tsc_meas_eye_scan_done( pa ));
  return(ERR_CODE_NONE);
}


/* Display the LOW BER EyeScan */
err_code_t eagle_tsc_display_lowber_eye( const phymod_access_t *pa, const struct eagle_tsc_eyescan_options_st eyescan_options, uint32_t *buffer) {
    int8_t x,y,i,z;
    int16_t j; /* buffer pointer */
    uint32_t val;
    uint8_t overflow;
    uint32_t limits[13]; /* allows upto 400 sec test time per point (1e-13 BER @ 10G) */
 
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
    
    eagle_tsc_display_eye_scan_header( pa, 1);
	j = 0;
    for (y = eyescan_options.vert_max;y>=eyescan_options.vert_min;y=y-eyescan_options.vstep) {
      ESTM(USR_PRINTF(("%6dmV : ",_ladder_setting_to_mV(pa,y, rd_p1_thresh_sel()))));
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
   eagle_tsc_display_eye_scan_footer( pa, 1);
   return(ERR_CODE_NONE);
}

err_code_t eagle_tsc_meas_eye_scan_start( const phymod_access_t *pa, uint8_t direction) {
  err_code_t err_code;
  uint8_t lock;
  ESTM(lock = rd_pmd_rx_lock());
  if(lock == 0) {
      USR_PRINTF(("Error: No PMD_RX_LOCK on lane requesting 2D eye scan\n"));
      return(ERR_CODE_DIAG_SCAN_NOT_COMPLETE);
  }
  if(direction == EYE_SCAN_VERTICAL) {
	err_code = eagle_tsc_pmd_uc_diag_cmd( pa, CMD_UC_DIAG_START_VSCAN_EYE,200);
  } else {
	  err_code = eagle_tsc_pmd_uc_diag_cmd( pa, CMD_UC_DIAG_START_HSCAN_EYE,200);
  }
  	if (err_code) {
		return (err_code);
	}

  return(ERR_CODE_NONE);
}

err_code_t eagle_tsc_read_eye_scan_stripe( const phymod_access_t *pa, uint32_t *buffer,uint16_t *status){
	int8_t i;
	uint32_t val[2] = {0,0};
    err_code_t err_code;
	uint16_t sts = 0;

	if(!buffer || !status) {
		return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
	}
	*status = 0;
	for(i=0;i<32;i++) {
		err_code = eagle_tsc_poll_diag_eye_data(pa,&val[0], &sts, 200);
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

err_code_t eagle_tsc_display_eye_scan_stripe( const phymod_access_t *pa, int8_t y,uint32_t *buffer) {

  const uint32_t limits[7] = {1835008, 183501, 18350, 1835, 184, 18, 2};
  int8_t x,i;

  	if(!buffer) {
		return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
	}

      ESTM(USR_PRINTF(("%6dmV : ",_ladder_setting_to_mV(pa,y, rd_p1_thresh_sel()))));
 
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

/* Measure LOW BER Eye Scan.   */
err_code_t eagle_tsc_meas_eye_density_data(const phymod_access_t *pa, const struct eagle_tsc_eyescan_options_st eyescan_options, int32_t *buffer,uint16_t *buffer_size) {
  int8_t y,x,z;
  int16_t i;
  int8_t hzcnt;
  /*uint32_t errcnt; */
  /*uint8_t lock_lost; */
  err_code_t err_code;
  
  if(!buffer || !buffer_size) {
	  return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }

  err_code = eagle_tsc_pmd_uc_diag_cmd( pa, CMD_UC_DIAG_DENSITY,2000);
  if (err_code) {
    return (err_code);
  }
  i = 0;hzcnt=0;
  ESTM(USR_PRINTF(("Calculating %d\n",rd_cnt_d_minus_m1())));
  for (y = eyescan_options.vert_max;y>=eyescan_options.vert_min;y=y-eyescan_options.vstep) {   
    _set_p1_threshold( pa, y);
    _move_clkp1_offset( pa, eyescan_options.horz_min-1);   /* Walk back 32 steps */
    _move_clkp1_offset( pa, 1);                              /* Walk forward one (net -31). This sets up the registers for 1 write increments */
	hzcnt = eyescan_options.horz_min;
	for (x=eyescan_options.horz_min;x<=eyescan_options.horz_max;x=x+eyescan_options.hstep) {
		 _trnsum_clear_and_enable(pa);
         err_code = eagle_tsc_poll_dsc_state_equals_uc_tune( pa, 2000);
	     if (err_code) 
           return(err_code);
	     ESTM(buffer[i] = (((int32_t)rd_trnsum_high())<<10) | rd_trnsum_low());
		 USR_PRINTF(("D %d\n",(uint32_t)buffer[i]));
		 i++;
	     for(z=1;z<=eyescan_options.hstep;z++) {
		   wr_rx_pi_manual_strobe(1); /* hstep */
		   hzcnt++;
		 }
		 USR_PRINTF(("."));
    }
	_move_clkp1_offset( pa, -hzcnt);     /* Walk back center */

    USR_PRINTF(("\n"));        
  }
  USR_PRINTF(("\n");  *buffer_size = i);
  err_code = eagle_tsc_meas_eye_scan_done( pa );
  if (err_code) {
	return (err_code);
  }

  return(ERR_CODE_NONE);
}

err_code_t eagle_tsc_display_eye_density_data( const phymod_access_t *pa, const struct eagle_tsc_eyescan_options_st eyescan_options, int32_t *buffer,uint16_t buffer_size) {
	int8_t x,y,i,z;
	int16_t j; /* buffer pointer */
    int32_t maxval = 0, val = 0;
	uint8_t range;

	ESTM(range = rd_p1_thresh_sel());

	if(!buffer) {
		return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
	}
  
        /* Find maxval */
	i=0;
	for(x=eyescan_options.horz_min;x<=eyescan_options.horz_max;x=x+eyescan_options.hstep)
		i++;/* determine row length */
        maxval = 0;j=buffer_size; /* init to last row */
        for (y=eyescan_options.vert_min;y<=eyescan_options.vert_max;y=y+eyescan_options.vstep) {
           for (x=eyescan_options.horz_min;x<=eyescan_options.horz_max;x=x+eyescan_options.hstep) {
             if (y==eyescan_options.vert_max) {
               buffer[i] = 0;   /* zero out top row */
               i--;  /* count number of samples in row */
             } else {
               val = buffer[j] - buffer[j-i];    /* Subtract from above row */
               if (val<0) val = 0;
               if (val>maxval) maxval = val;
               buffer[j] = val;
               j--;
			 }

           }
        }  
	eagle_tsc_display_eye_scan_header( pa, 1);
        for (y = eyescan_options.vert_max-1;y>=eyescan_options.vert_min;y=y-eyescan_options.vstep) {
          USR_PRINTF(("%6dmV : ",(_ladder_setting_to_mV(pa,y, range) + _ladder_setting_to_mV(pa,y+1, range))/2));    
			for(z=-31;z<eyescan_options.horz_min;z++) {
				USR_PRINTF((" ")); /* add leading spaces if any */
			}
            for (x=eyescan_options.horz_min;x<=eyescan_options.horz_max;x=x+eyescan_options.hstep) {
                if (maxval != 0)
                    val = buffer[j]/(maxval/16);             /* Normalize peak to 15 */
                if (val>15) val = 15;
                for(z=1;z<=eyescan_options.hstep;z++) {
                    if(z==1) {
                        if (val) {
                            USR_PRINTF(("%X",(uint32_t)val));
                        }
                        else {
                            if ((x%5)==0 && ((y+3)%5)==0) {
                                USR_PRINTF(("+"));
                            }
                            else if ((x%5)!=0 && ((y+3)%5)==0) {
                                USR_PRINTF(("-"));
                            }
                            else if ((x%5)==0 && ((y+3)%5)!=0) {
                                USR_PRINTF((":"));
                            }
                            else {
                                USR_PRINTF((" "));
                            }
                        }
                    } else {
                        USR_PRINTF((" "));
                    }
                }
                j++;
            }
          USR_PRINTF(("\n"));  
        }  
	eagle_tsc_display_eye_scan_footer( pa, 1);
	return(ERR_CODE_NONE);
}
err_code_t eagle_tsc_display_eye_scan_header( const phymod_access_t *pa, int8_t i) {
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

err_code_t eagle_tsc_display_eye_scan_footer( const phymod_access_t *pa, int8_t i) {
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


/*eye_scan_status_t eagle_tsc_read_eye_scan_status() */
err_code_t eagle_tsc_read_eye_scan_status( const phymod_access_t *pa, uint16_t *status) {

   if(!status) {
		return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
	}

   ESTM(*status=rdv_usr_diag_status());	

	return(ERR_CODE_NONE);
}


err_code_t eagle_tsc_meas_eye_scan_done( const phymod_access_t *pa ) {
  err_code_t err_code;
  err_code = eagle_tsc_pmd_uc_diag_cmd( pa, CMD_UC_DIAG_DISABLE,200);
  if (err_code) return(err_code);
  return(ERR_CODE_NONE);
}


err_code_t eagle_tsc_start_ber_scan_test( const phymod_access_t *pa, uint8_t ber_scan_mode, uint8_t timer_control, uint8_t max_error_control) {
    uint8_t lock;
    ESTM(lock = rd_pmd_rx_lock());
    if(lock == 0) {
        USR_PRINTF(("Error: No PMD_RX_LOCK on lane requesting BER scan\n"));
        return(ERR_CODE_DIAG_SCAN_NOT_COMPLETE);
    }

    wrcv_diag_max_time_control(timer_control);
    wrcv_diag_max_err_control(max_error_control);
    EFUN(eagle_tsc_pmd_uc_cmd( pa, CMD_CAPTURE_BER_START, ber_scan_mode,200));
  
    return(ERR_CODE_NONE);
}

err_code_t eagle_tsc_read_ber_scan_data( const phymod_access_t *pa, uint32_t *errors, uint32_t *timer_values, uint8_t *cnt, uint32_t timeout) {
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
		err_code = eagle_tsc_pmd_uc_cmd( pa,  CMD_READ_DIAG_DATA_WORD, 0, timeout);
		if(err_code) return(err_code);
		ESTM(dataword = rd_uc_dsc_data());           /* LSB contains 2 -4bit nibbles */
		time_byte = (uint8_t)(dataword>>8);    /* MSB is time byte */
		prbs_multi = (uint8_t)dataword & 0x0F; /* split nibbles */
		time_multi = (uint8_t)dataword>>4;
		/* Read 1 bytes of data */
		err_code = eagle_tsc_pmd_uc_cmd( pa,  CMD_READ_DIAG_DATA_BYTE, 0, timeout);
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
err_code_t eagle_tsc_eye_margin_proj(const phymod_access_t *pa, USR_DOUBLE rate, uint8_t ber_scan_mode, uint8_t timer_control, uint8_t max_error_control) {
	return(ERR_CODE_NONE);
}

err_code_t eagle_tsc_display_ber_scan_data(USR_DOUBLE rate, uint8_t ber_scan_mode, uint32_t *total_errs, uint32_t *total_time, uint8_t max_offset) {
 USR_PRINTF(("This functions needs SERDES_API_FLOATING_POINT define to operate \n"));
	return(ERR_CODE_NONE);

}



/*****************************/
/*  Display Lane/Core State  */
/*****************************/

err_code_t eagle_tsc_display_lane_state_hdr( const phymod_access_t *pa ) {
  USR_PRINTF(("LN (CDRxN  ,UC_CFG) "));
  USR_PRINTF(("SD LCK RXPPM "));
  USR_PRINTF(("CLK90 CLKP1 PF(M,L) "));
  USR_PRINTF(("VGA DCO "));
  USR_PRINTF(("P1mV "));
  USR_PRINTF((" DFE(1,2,3,4,5,dcd1,dcd2)   SLICER(ze,zo,pe,po,me,mo) "));
  USR_PRINTF(("TXPPM TXEQ(n1,m,p1,p2) EYE(L,R,U,D) "));
  USR_PRINTF(("LINK_TIME"));
  USR_PRINTF(("\n"));
  return (ERR_CODE_NONE);
}

err_code_t eagle_tsc_display_lane_state_legend( const phymod_access_t *pa ) {
  USR_PRINTF(("\n"));
  USR_PRINTF(("**********************************************************************************************\n")); 
  USR_PRINTF(("****                Legend of Entries in display_lane_state()                             ****\n")); 
  USR_PRINTF(("**********************************************************************************************\n"));
  USR_PRINTF(("LN       : lane index within IP core\n"));
  USR_PRINTF(("(CDRxN,UC_CFG) : CDR type x OSR ratio, micro lane configuration variable\n"));
  USR_PRINTF(("SD       : signal detect\n"));
  USR_PRINTF(("LOCK     : pmd_rx_lock\n"));
  USR_PRINTF(("RXPPM    : Frequency offset of local reference clock with respect to RX data in ppm\n"));
  USR_PRINTF(("CLK90    : Delay of zero crossing slicer, m1, wrt to data in PI codes\n"));
  USR_PRINTF(("CLKP1    : Delay of diagnostic/lms slicer, p1, wrt to data in PI codes\n"));
  USR_PRINTF(("PF(M,L)  : Peaking Filter Main (0..15) and Low Frequency (0..7) settings\n"));
  USR_PRINTF(("VGA      : Variable Gain Amplifier settings (0..42)\n"));
  USR_PRINTF(("DCO      : DC offset DAC control value\n"));
  USR_PRINTF(("P1mV     : Vertical threshold voltage of p1 slicer\n"));
  USR_PRINTF(("DFE taps         : ISI correction taps in units of 2.35mV (for 1 & 2 even values are displayed, dcd = even-odd)\n"));   
  USR_PRINTF(("SLICER(ze,zo,pe,po,me,mo) : Slicer calibration control codes\n"));
  USR_PRINTF(("TXPPM            : Frequency offset of local reference clock with respect to TX data in ppm\n"));  
  USR_PRINTF(("TXEQ(n1,m,p1,p2) : TX equalization FIR tap weights in units of 1Vpp/60 units\n"));
  USR_PRINTF(("EYE(L,R,U,D)     : Eye margin @ 1e-5 as seen by internal diagnostic slicer in mUI and mV\n"));
  USR_PRINTF(("LINK_TIME        : Link time in milliseconds\n"));
  USR_PRINTF(("**********************************************************************************************\n")); 
  return (ERR_CODE_NONE);
}

err_code_t eagle_tsc_display_lane_state( const phymod_access_t *pa ) {
  err_code_t err_code;
  err_code = _eagle_tsc_display_lane_state_no_newline( pa );
  USR_PRINTF(("\n"));
  return (err_code);
}





/**********************************************/
/*  Display Lane/Core Config and Debug Status */
/**********************************************/

err_code_t eagle_tsc_display_core_config( const phymod_access_t *pa ) {

  err_code_t  err_code;
  struct eagle_tsc_uc_core_config_st core_cfg;
  uint16_t vco_rate_mhz;

  core_cfg.field.core_cfg_from_pcs = 0;
  core_cfg.field.vco_rate = 0;

  USR_PRINTF(("\n\n***********************************\n")); 
  USR_PRINTF(("**** SERDES CORE CONFIGURATION ****\n")); 
  USR_PRINTF(("***********************************\n\n")); 

  err_code = eagle_tsc_get_uc_core_config( pa, &core_cfg);
  if (err_code) {
    return (err_code);
  }

    vco_rate_mhz = ((((uint16_t) core_cfg.field.vco_rate) * 250) + 5500);
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


err_code_t eagle_tsc_display_lane_config( const phymod_access_t *pa ) {

  err_code_t  err_code;
  struct eagle_tsc_uc_lane_config_st lane_cfg;
  PHYMOD_MEMSET(&lane_cfg, 0, sizeof(struct eagle_tsc_uc_lane_config_st));


#if 0
	lane_cfg.field.lane_cfg_from_pcs = 0;
	lane_cfg.field.an_enabled = 0;
	lane_cfg.field.dfe_on = 0;
	lane_cfg.field.force_brdfe_on = 0;
	lane_cfg.field.media_type = 0;
	lane_cfg.field.unreliable_los = 0;
	lane_cfg.field.scrambling_dis = 0;
	lane_cfg.field.cl72_emulation_en = 0;
#endif

  USR_PRINTF(("\n\n*************************************\n")); 
  USR_PRINTF(("**** SERDES LANE %d CONFIGURATION ****\n",eagle_tsc_get_lane(pa))); 
  USR_PRINTF(("*************************************\n\n")); 

  err_code = eagle_tsc_get_uc_lane_cfg( pa, &lane_cfg);
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

  ESTM(USR_PRINTF(("CL72 Training Enable  = %d\n", rd_cl72_ieee_training_enable())));
  ESTM(USR_PRINTF(("EEE Mode Enable       = %d\n", rd_eee_mode_en())));
  ESTM(USR_PRINTF(("OSR Mode Force        = %d\n", rd_osr_mode_frc())));
  ESTM(USR_PRINTF(("OSR Mode Force Val    = %d\n", rd_osr_mode_frc_val())));
  ESTM(USR_PRINTF(("TX Polarity Invert    = %d\n", rd_tx_pmd_dp_invert())));
  ESTM(USR_PRINTF(("RX Polarity Invert    = %d\n\n", rd_rx_pmd_dp_invert())));

  ESTM(USR_PRINTF(("TXFIR Post2           = %d\n", rd_txfir_post2())));
  ESTM(USR_PRINTF(("TXFIR Post3           = %d\n", rd_txfir_post3())));
  ESTM(USR_PRINTF(("TXFIR Override Enable = %d\n", rd_txfir_override_en())));
  ESTM(USR_PRINTF(("TXFIR Main Override   = %d\n", rd_txfir_main_override())));
  ESTM(USR_PRINTF(("TXFIR Pre Override    = %d\n", rd_txfir_pre_override())));
  ESTM(USR_PRINTF(("TXFIR Post Override   = %d\n", rd_txfir_post_override())));

  return(ERR_CODE_NONE); 
}


err_code_t eagle_tsc_display_core_state( const phymod_access_t *pa ) {

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



err_code_t eagle_tsc_display_lane_debug_status( const phymod_access_t *pa ) {

  err_code_t  err_code;

  /* startup */
  struct eagle_tsc_usr_ctrl_disable_functions_st ds; 
  struct eagle_tsc_usr_ctrl_disable_dfe_functions_st dsd;

  /* steady state */
  struct eagle_tsc_usr_ctrl_disable_functions_st dss;
  struct eagle_tsc_usr_ctrl_disable_dfe_functions_st dssd;

  PHYMOD_MEMSET(&ds, 0, sizeof(struct eagle_tsc_usr_ctrl_disable_functions_st));
  PHYMOD_MEMSET(&dsd, 0, sizeof(struct eagle_tsc_usr_ctrl_disable_dfe_functions_st));
  PHYMOD_MEMSET(&dss, 0, sizeof(struct eagle_tsc_usr_ctrl_disable_functions_st));
  PHYMOD_MEMSET(&dssd, 0, sizeof(struct eagle_tsc_usr_ctrl_disable_dfe_functions_st));

  USR_PRINTF(("\n\n************************************\n")); 
  USR_PRINTF(("**** SERDES LANE %d DEBUG STATUS ****\n",eagle_tsc_get_lane(pa))); 
  USR_PRINTF(("************************************\n\n")); 

  ESTM(USR_PRINTF(("Restart Count       = %d\n", rdv_usr_sts_restart_counter())));
  ESTM(USR_PRINTF(("Reset Count         = %d\n", rdv_usr_sts_reset_counter())));
  ESTM(USR_PRINTF(("PMD Lock Count      = %d\n\n", rdv_usr_sts_pmd_lock_counter())));
 

  err_code = eagle_tsc_get_usr_ctrl_disable_startup( pa, &ds);
  if (err_code) {
    return (err_code);
  }

  USR_PRINTF(("Disable Startup PF Adaptation           = %d\n", ds.field.pf_adaptation));
  USR_PRINTF(("Disable Startup DC Adaptation           = %d\n", ds.field.dc_adaptation));
  USR_PRINTF(("Disable Startup Slicer Offset Tuning    = %d\n", ds.field.slicer_offset_tuning));
  USR_PRINTF(("Disable Startup Clk90 offset Adaptation = %d\n", ds.field.clk90_offset_adaptation));
  USR_PRINTF(("Disable Startup P1 level Tuning         = %d\n", ds.field.p1_level_tuning));
  USR_PRINTF(("Disable Startup Eye Adaptaion           = %d\n", ds.field.eye_adaptation));
  USR_PRINTF(("Disable Startup All Adaptaion           = %d\n\n", ds.field.all_adaptation));

  err_code = eagle_tsc_get_usr_ctrl_disable_startup_dfe( pa, &dsd);
  if (err_code) {
    return (err_code);
  }

  USR_PRINTF(("Disable Startup DFE Tap1 Adaptation = %d\n",dsd.field.dfe_tap1_adaptation));
  USR_PRINTF(("Disable Startup DFE Tap2 Adaptation = %d\n",dsd.field.dfe_tap2_adaptation));
  USR_PRINTF(("Disable Startup DFE Tap3 Adaptation = %d\n",dsd.field.dfe_tap3_adaptation));
  USR_PRINTF(("Disable Startup DFE Tap4 Adaptation = %d\n",dsd.field.dfe_tap4_adaptation));
  USR_PRINTF(("Disable Startup DFE Tap5 Adaptation = %d\n",dsd.field.dfe_tap5_adaptation));
  USR_PRINTF(("Disable Startup DFE Tap1 DCD        = %d\n",dsd.field.dfe_tap1_dcd));       
  USR_PRINTF(("Disable Startup DFE Tap2 DCD        = %d\n\n",dsd.field.dfe_tap2_dcd));       

  err_code = eagle_tsc_get_usr_ctrl_disable_steady_state( pa, &dss);
  if (err_code) {
    return (err_code);
  }

  USR_PRINTF(("Disable Steady State PF Adaptation           = %d\n", dss.field.pf_adaptation));
  USR_PRINTF(("Disable Steady State DC Adaptation           = %d\n", dss.field.dc_adaptation));
  USR_PRINTF(("Disable Steady State Slicer Offset Tuning    = %d\n", dss.field.slicer_offset_tuning));
  USR_PRINTF(("Disable Steady State Clk90 offset Adaptation = %d\n", dss.field.clk90_offset_adaptation));
  USR_PRINTF(("Disable Steady State P1 level Tuning         = %d\n", dss.field.p1_level_tuning));
  USR_PRINTF(("Disable Steady State Eye Adaptaion           = %d\n", dss.field.eye_adaptation));
  USR_PRINTF(("Disable Steady State All Adaptaion           = %d\n\n", dss.field.all_adaptation));

  err_code = eagle_tsc_get_usr_ctrl_disable_steady_state_dfe( pa, &dssd);
  if (err_code) {
    return (err_code);
  }

  USR_PRINTF(("Disable Steady State DFE Tap1 Adaptation = %d\n",dssd.field.dfe_tap1_adaptation));
  USR_PRINTF(("Disable Steady State DFE Tap2 Adaptation = %d\n",dssd.field.dfe_tap2_adaptation));
  USR_PRINTF(("Disable Steady State DFE Tap3 Adaptation = %d\n",dssd.field.dfe_tap3_adaptation));
  USR_PRINTF(("Disable Steady State DFE Tap4 Adaptation = %d\n",dssd.field.dfe_tap4_adaptation));
  USR_PRINTF(("Disable Steady State DFE Tap5 Adaptation = %d\n",dssd.field.dfe_tap5_adaptation));
  USR_PRINTF(("Disable Steady State DFE Tap1 DCD        = %d\n",dssd.field.dfe_tap1_dcd));       
  USR_PRINTF(("Disable Steady State DFE Tap2 DCD        = %d\n\n",dssd.field.dfe_tap2_dcd));  

  ESTM(USR_PRINTF(("Retune after Reset    = %d\n", rdv_usr_ctrl_retune_after_restart())));               
  ESTM(USR_PRINTF(("Clk90 offset Adjust   = %d\n", rdv_usr_ctrl_clk90_offset_adjust())));                
  ESTM(USR_PRINTF(("Clk90 offset Override = %d\n", rdv_usr_ctrl_clk90_offset_override())));              
  ESTM(USR_PRINTF(("Lane Event Log Level  = %d\n", rdv_usr_ctrl_lane_event_log_level())));

  return(ERR_CODE_NONE); 
}



/*************************/
/*  Stop/Resume uC Lane  */
/*************************/

err_code_t eagle_tsc_stop_uc_lane( const phymod_access_t *pa, uint8_t enable) {

  if (enable) {
    return(eagle_tsc_pmd_uc_control( pa, CMD_UC_CTRL_STOP_GRACEFULLY,2000));
  }
  else {
    return(eagle_tsc_pmd_uc_control( pa, CMD_UC_CTRL_RESUME,2000));
  }
}


err_code_t eagle_tsc_stop_uc_lane_status( const phymod_access_t *pa, uint8_t *uc_lane_stopped) {
  
  if(!uc_lane_stopped) {
	  return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }

  ESTM(*uc_lane_stopped = rdv_usr_sts_micro_stopped());

  return (ERR_CODE_NONE);
}


/*******************************/
/*  Stop/Resume RX Adaptation  */
/*******************************/

err_code_t eagle_tsc_stop_rx_adaptation( const phymod_access_t *pa, uint8_t enable) {

  if (enable) {
    return(eagle_tsc_pmd_uc_control( pa, CMD_UC_CTRL_STOP_GRACEFULLY,2000));
  }
  else {
    return(eagle_tsc_pmd_uc_control( pa, CMD_UC_CTRL_RESUME,2000));
  }
}



/**********************/
/*  uCode CRC Verify  */
/**********************/

err_code_t eagle_tsc_ucode_crc_verify( const phymod_access_t *pa, uint16_t ucode_len,uint16_t expected_crc_value) {
    uint16_t calc_crc;

    EFUN(eagle_tsc_pmd_uc_cmd_with_data( pa, CMD_CALC_CRC,0,ucode_len,2000));

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

err_code_t eagle_tsc_pmd_uc_cmd( const phymod_access_t *pa, enum eagle_tsc_pmd_uc_cmd_enum cmd, uint8_t supp_info, uint32_t timeout_ms) {

  err_code_t err_code;
  uint16_t cmddata;
  uint8_t uc_dsc_error_found;

  err_code = eagle_tsc_poll_uc_dsc_ready_for_cmd_equals_1( pa, timeout_ms); /* Poll for uc_dsc_ready_for_cmd = 1 to indicate eagle_tsc ready for command */
  if (err_code) {
    ESTM(USR_PRINTF(("ERROR : DSC ready for command timed out (before cmd) cmd = %d, supp_info = x%02x err=%d !\n", cmd, supp_info,err_code)));      
    return (err_code);
  }
  /*wr_uc_dsc_supp_info(supp_info);*/                                   /* supplement info field */
  /*wr_uc_dsc_error_found(0x0);    */                                   /* Clear error found field */
  /*wr_uc_dsc_gp_uc_req(cmd);      */                                   /* Set command code */
  /*wr_uc_dsc_ready_for_cmd(0x0);  */                                   /* Issue command, by clearing "ready for command" field */
  cmddata = (((uint16_t)supp_info)<<8) | (uint16_t)cmd;     /* combine writes to single write instead of 4 RMW */

  EFUN(eagle_tsc_pmd_wr_reg(pa,DSC_A_DSC_UC_CTRL, cmddata));         /* This address is same for Eagle, and all merlin */

  EFUN(eagle_tsc_poll_uc_dsc_ready_for_cmd_equals_1( pa, timeout_ms)); /* Poll for uc_dsc_ready_for_cmd = 1 to indicate eagle_tsc ready for command */
  ESTM(uc_dsc_error_found = rd_uc_dsc_error_found());
  if(uc_dsc_error_found) {
    ESTM(USR_PRINTF(("ERROR : DSC ready for command return error ( after cmd) cmd = %d, supp_info = x%02x !\n", cmd, rd_uc_dsc_supp_info())));      
      return(_error(ERR_CODE_UC_CMD_RETURN_ERROR));
  }
  return(ERR_CODE_NONE);
}

err_code_t eagle_tsc_pmd_uc_cmd_with_data( const phymod_access_t *pa, enum eagle_tsc_pmd_uc_cmd_enum cmd, uint8_t supp_info, uint16_t data, uint32_t timeout_ms) {
  uint16_t cmddata;
  err_code_t err_code;
  uint8_t uc_dsc_error_found;

  err_code = eagle_tsc_poll_uc_dsc_ready_for_cmd_equals_1( pa, timeout_ms); /* Poll for uc_dsc_ready_for_cmd = 1 to indicate eagle_tsc ready for command */
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

  EFUN(eagle_tsc_pmd_wr_reg(pa,DSC_A_DSC_UC_CTRL, cmddata));         /* This address is same for Eagle, and all merlin */

  EFUN(eagle_tsc_poll_uc_dsc_ready_for_cmd_equals_1( pa, timeout_ms)); /* Poll for uc_dsc_ready_for_cmd = 1 to indicate eagle_tsc ready for command */
  ESTM(uc_dsc_error_found = rd_uc_dsc_error_found());
  if(uc_dsc_error_found) {
    ESTM(USR_PRINTF(("ERROR : DSC ready for command return error ( after cmd) cmd = %d, supp_info = x%02x !\n", cmd, rd_uc_dsc_supp_info())));      
    return(_error(ERR_CODE_UC_CMD_RETURN_ERROR));
}

  return(ERR_CODE_NONE);
}

err_code_t eagle_tsc_pmd_uc_control( const phymod_access_t *pa, enum eagle_tsc_pmd_uc_ctrl_cmd_enum control, uint32_t timeout_ms) {
  return(eagle_tsc_pmd_uc_cmd( pa, CMD_UC_CTRL, (uint8_t) control, timeout_ms));
}

err_code_t eagle_tsc_pmd_uc_diag_cmd( const phymod_access_t *pa, enum eagle_tsc_pmd_uc_diag_cmd_enum control, uint32_t timeout_ms) {
  return(eagle_tsc_pmd_uc_cmd( pa, CMD_DIAG_EN, (uint8_t) control, timeout_ms));
}



/************************************************************/
/*      Serdes IP RAM access - Lane RAM Variables           */
/*----------------------------------------------------------*/
/*   - through Micro Register Interface for PMD IPs         */
/*   - through Serdes FW DSC Command Interface for Gallardo */
/************************************************************/

/* Micro RAM Lane Byte Read */
uint8_t eagle_tsc_rdbl_uc_var( const phymod_access_t *pa, err_code_t *err_code_p, uint16_t addr) {
	uint8_t rddata;



	if(!err_code_p) {
		return(0);
	}

	EPSTM(rddata = eagle_tsc_rdb_uc_ram( pa, err_code_p, (0x400+addr+(eagle_tsc_get_lane(pa)*0x100)))); /* Use Micro register interface for reading RAM */

	return (rddata);
} 

/* Micro RAM Lane Byte Signed Read */
int8_t eagle_tsc_rdbls_uc_var( const phymod_access_t *pa, err_code_t *err_code_p, uint16_t addr) {
  return ((int8_t) eagle_tsc_rdbl_uc_var( pa, err_code_p, addr));
} 

/* Micro RAM Lane Word Read */
uint16_t eagle_tsc_rdwl_uc_var( const phymod_access_t *pa, err_code_t *err_code_p, uint16_t addr) {
  uint16_t rddata;

  if(!err_code_p) {
	  return(0);
  }

  if (addr%2 != 0) {                                                                /* Validate even address */
	  *err_code_p = ERR_CODE_INVALID_RAM_ADDR;
	  return (0);
  }

  EPSTM(rddata = eagle_tsc_rdw_uc_ram( pa, err_code_p, (0x400+addr+(eagle_tsc_get_lane(pa)*0x100)))); /* Use Micro register interface for reading RAM */

  return (rddata);
}


/* Micro RAM Lane Word Signed Read */
int16_t eagle_tsc_rdwls_uc_var( const phymod_access_t *pa, err_code_t *err_code_p, uint16_t addr) {
  return ((int16_t) eagle_tsc_rdwl_uc_var( pa, err_code_p, addr));
}
  
/* Micro RAM Lane Byte Write */
err_code_t eagle_tsc_wrbl_uc_var( const phymod_access_t *pa, uint16_t addr, uint8_t wr_val) {


    return (eagle_tsc_wrb_uc_ram( pa, (0x400+addr+(eagle_tsc_get_lane(pa)*0x100)), wr_val));    /* Use Micro register interface for writing RAM */
}

/* Micro RAM Lane Byte Signed Write */
err_code_t eagle_tsc_wrbls_uc_var( const phymod_access_t *pa, uint16_t addr, int8_t wr_val) {
  return (eagle_tsc_wrbl_uc_var( pa, addr, wr_val));
}

/* Micro RAM Lane Word Write */
err_code_t eagle_tsc_wrwl_uc_var( const phymod_access_t *pa, uint16_t addr, uint16_t wr_val) {


	if (addr%2 != 0) {                                                                /* Validate even address */
		return (_error(ERR_CODE_INVALID_RAM_ADDR));
	}
    return (eagle_tsc_wrw_uc_ram( pa, (0x400+addr+(eagle_tsc_get_lane(pa)*0x100)), wr_val));    /* Use Micro register interface for writing RAM */
}

/* Micro RAM Lane Word Signed Write */
err_code_t eagle_tsc_wrwls_uc_var( const phymod_access_t *pa, uint16_t addr, int16_t wr_val) {
  return (eagle_tsc_wrwl_uc_var( pa, addr,wr_val));
}


/************************************************************/
/*      Serdes IP RAM access - Core RAM Variables           */
/*----------------------------------------------------------*/
/*   - through Micro Register Interface for PMD IPs         */
/*   - through Serdes FW DSC Command Interface for Gallardo */
/************************************************************/

/* Micro RAM Core Byte Read */
uint8_t eagle_tsc_rdbc_uc_var( const phymod_access_t *pa, err_code_t *err_code_p, uint8_t addr) {

  uint8_t rddata;

  if(!err_code_p) {
	  return(0);
  }

  EPSTM(rddata = eagle_tsc_rdb_uc_ram( pa, err_code_p, (0x50+addr)));                      /* Use Micro register interface for reading RAM */

  return (rddata);
} 

/* Micro RAM Core Byte Signed Read */
int8_t eagle_tsc_rdbcs_uc_var( const phymod_access_t *pa, err_code_t *err_code_p, uint8_t addr) {
  return ((int8_t) eagle_tsc_rdbc_uc_var( pa, err_code_p, addr));
}

/* Micro RAM Core Word Read */
uint16_t eagle_tsc_rdwc_uc_var( const phymod_access_t *pa, err_code_t *err_code_p, uint8_t addr) {

  uint16_t rddata;

  if(!err_code_p) {
	  return(0);
  }
  if (addr%2 != 0) {                                                                /* Validate even address */
	  *err_code_p = ERR_CODE_INVALID_RAM_ADDR;
	  return (0);
  }

  EPSTM(rddata = eagle_tsc_rdw_uc_ram( pa, err_code_p, (0x50+addr)));                  /* Use Micro register interface for reading RAM */

  return (rddata);
}

/* Micro RAM Core Word Signed Read */
int16_t eagle_tsc_rdwcs_uc_var( const phymod_access_t *pa, err_code_t *err_code_p, uint8_t addr) {
  return ((int16_t) eagle_tsc_rdwc_uc_var( pa, err_code_p, addr));
}

/* Micro RAM Core Byte Write  */
err_code_t eagle_tsc_wrbc_uc_var( const phymod_access_t *pa, uint8_t addr, uint8_t wr_val) {


    return (eagle_tsc_wrb_uc_ram( pa, (0x50+addr), wr_val));                                /* Use Micro register interface for writing RAM */
} 


/* Micro RAM Core Byte Signed Write */
err_code_t eagle_tsc_wrbcs_uc_var( const phymod_access_t *pa, uint8_t addr, int8_t wr_val) {
  return (eagle_tsc_wrbc_uc_var( pa, addr, wr_val));
}

/* Micro RAM Core Word Write  */
err_code_t eagle_tsc_wrwc_uc_var( const phymod_access_t *pa, uint8_t addr, uint16_t wr_val) {


	if (addr%2 != 0) {                                                                /* Validate even address */
		return (_error(ERR_CODE_INVALID_RAM_ADDR));
	}
    return (eagle_tsc_wrw_uc_ram( pa, (0x50+addr), wr_val));                                 /* Use Micro register interface for writing RAM */
}

/* Micro RAM Core Word Signed Write */
err_code_t eagle_tsc_wrwcs_uc_var( const phymod_access_t *pa, uint8_t addr, int16_t wr_val) {
  return(eagle_tsc_wrwc_uc_var( pa, addr,wr_val));
}



/*******************************************************************/
/*  APIs to Write Core/Lane Config and User variables into uC RAM  */
/*******************************************************************/

err_code_t eagle_tsc_set_uc_core_config( const phymod_access_t *pa, struct eagle_tsc_uc_core_config_st struct_val) {
  _update_uc_core_config_word( pa, &struct_val);
  return(wrcv_config_word(struct_val.word));
}

err_code_t eagle_tsc_set_usr_ctrl_core_event_log_level( const phymod_access_t *pa, uint8_t core_event_log_level) {
  return(wrcv_usr_ctrl_core_event_log_level(core_event_log_level));
}

err_code_t eagle_tsc_set_uc_lane_cfg( const phymod_access_t *pa, struct eagle_tsc_uc_lane_config_st struct_val) {
  _update_uc_lane_config_word( pa, &struct_val);
  return(wrv_config_word(struct_val.word));
}

err_code_t eagle_tsc_set_usr_ctrl_lane_event_log_level( const phymod_access_t *pa, uint8_t lane_event_log_level) {
  return(wrv_usr_ctrl_lane_event_log_level(lane_event_log_level));
}

err_code_t eagle_tsc_set_usr_ctrl_disable_startup( const phymod_access_t *pa, struct eagle_tsc_usr_ctrl_disable_functions_st set_val) {
  _update_usr_ctrl_disable_functions_byte( pa, &set_val);
  return(wrv_usr_ctrl_disable_startup_functions_byte(set_val.byte));
}

err_code_t eagle_tsc_set_usr_ctrl_disable_startup_dfe( const phymod_access_t *pa, struct eagle_tsc_usr_ctrl_disable_dfe_functions_st set_val) {
  _update_usr_ctrl_disable_dfe_functions_byte( pa, &set_val);
  return(wrv_usr_ctrl_disable_startup_dfe_functions_byte(set_val.byte));
}

err_code_t eagle_tsc_set_usr_ctrl_disable_steady_state( const phymod_access_t *pa, struct eagle_tsc_usr_ctrl_disable_functions_st set_val) {
  _update_usr_ctrl_disable_functions_byte( pa, &set_val);
  return(wrv_usr_ctrl_disable_steady_state_functions_byte(set_val.byte));
}

err_code_t eagle_tsc_set_usr_ctrl_disable_steady_state_dfe( const phymod_access_t *pa, struct eagle_tsc_usr_ctrl_disable_dfe_functions_st set_val) {
  _update_usr_ctrl_disable_dfe_functions_byte( pa, &set_val);
  return(wrv_usr_ctrl_disable_steady_state_dfe_functions_byte(set_val.byte));
}



/******************************************************************/
/*  APIs to Read Core/Lane Config and User variables from uC RAM  */
/******************************************************************/

err_code_t eagle_tsc_get_uc_core_config( const phymod_access_t *pa, struct eagle_tsc_uc_core_config_st *get_val) {

  if(!get_val) {
	 return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }

  ESTM(get_val->word = rdcv_config_word());
  _update_uc_core_config_st( pa, get_val);

  return (ERR_CODE_NONE);
}

err_code_t eagle_tsc_get_usr_ctrl_core_event_log_level( const phymod_access_t *pa, uint8_t *core_event_log_level) {

  if(!core_event_log_level) {
	 return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }

  ESTM(*core_event_log_level = rdcv_usr_ctrl_core_event_log_level());

  return (ERR_CODE_NONE);
}

err_code_t eagle_tsc_get_uc_lane_cfg( const phymod_access_t *pa, struct eagle_tsc_uc_lane_config_st *get_val) { 

  if(!get_val) {
	 return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }

  ESTM(get_val->word = rdv_config_word());
  _update_uc_lane_config_st( pa, get_val);
  return (ERR_CODE_NONE);
}

err_code_t eagle_tsc_get_usr_ctrl_lane_event_log_level( const phymod_access_t *pa, uint8_t *lane_event_log_level) {

  if(!lane_event_log_level) {
	 return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }

  ESTM(*lane_event_log_level = rdv_usr_ctrl_lane_event_log_level());
  return (ERR_CODE_NONE);
}

err_code_t eagle_tsc_get_usr_ctrl_disable_startup( const phymod_access_t *pa, struct eagle_tsc_usr_ctrl_disable_functions_st *get_val) {

  if(!get_val) {
	 return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }

  ESTM(get_val->byte = rdv_usr_ctrl_disable_startup_functions_byte());
  _update_usr_ctrl_disable_functions_st( pa, get_val);
  return (ERR_CODE_NONE);
}

err_code_t eagle_tsc_get_usr_ctrl_disable_startup_dfe( const phymod_access_t *pa, struct eagle_tsc_usr_ctrl_disable_dfe_functions_st *get_val) {

  if(!get_val) {
	 return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }

  ESTM(get_val->byte = rdv_usr_ctrl_disable_startup_dfe_functions_byte());
  _update_usr_ctrl_disable_dfe_functions_st( pa, get_val);
  return (ERR_CODE_NONE);
}

err_code_t eagle_tsc_get_usr_ctrl_disable_steady_state( const phymod_access_t *pa, struct eagle_tsc_usr_ctrl_disable_functions_st *get_val) {

  if(!get_val) {
	 return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }

  ESTM(get_val->byte = rdv_usr_ctrl_disable_steady_state_functions_byte());
  _update_usr_ctrl_disable_functions_st( pa, get_val);
  return (ERR_CODE_NONE);
}

err_code_t eagle_tsc_get_usr_ctrl_disable_steady_state_dfe( const phymod_access_t *pa, struct eagle_tsc_usr_ctrl_disable_dfe_functions_st *get_val) {

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
err_code_t eagle_tsc_poll_diag_done( const phymod_access_t *pa, uint16_t *status, uint32_t timeout_ms) {
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
		 EFUN(eagle_tsc_delay_us(10*timeout_ms));
	 }
 }
 return(_error(ERR_CODE_DIAG_TIMEOUT));
}

/* poll for microcontroller to populate the dsc_data register */
err_code_t eagle_tsc_poll_diag_eye_data(const phymod_access_t *pa,uint32_t *data,uint16_t *status, uint32_t timeout_ms) {
 uint8_t loop;
 uint16_t dscdata;
 if(!data || !status) {
	 return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
  }

 for(loop=0;loop < 100; loop++) {
	 ESTM(*status=rdv_usr_diag_status());
	 if(((*status & 0x00FF) > 2) || ((*status & 0x8000) > 0)) {
		EFUN(eagle_tsc_pmd_uc_cmd( pa,  CMD_READ_DIAG_DATA_WORD, 0, 200));
        	ESTM(dscdata = rd_uc_dsc_data());
		data[0] = _float8_to_int32((float8_t)(dscdata >>8));
		data[1] = _float8_to_int32((float8_t)(dscdata & 0x00FF));
		return(ERR_CODE_NONE);
	 }
	 if(loop>10) {
		 EFUN(eagle_tsc_delay_us(10*timeout_ms));
	 }
 }
 return(_error(ERR_CODE_DIAG_TIMEOUT));
}

#ifndef CUSTOM_REG_POLLING

/* Poll for field "uc_dsc_ready_for_cmd" = 1 [Return Val => Error_code (0 = Polling Pass)] */
err_code_t eagle_tsc_poll_uc_dsc_ready_for_cmd_equals_1( const phymod_access_t *pa, uint32_t timeout_ms) {
  
  uint16_t loop;
  err_code_t  err_code;
  /* read quickly for 4 tries */
  for (loop = 0; loop < 100; loop++) {
    uint16_t rddata;
    EFUN(eagle_tsc_pmd_rdt_reg(pa,DSC_A_DSC_UC_CTRL, &rddata));
    if (rddata & 0x0080) {    /* bit 7 is uc_dsc_ready_for_cmd */
      if (rddata & 0x0040) {  /* bit 6 is uc_dsc_error_found   */
		ESTM(USR_PRINTF(("ERROR : DSC command returned error (after cmd) cmd = x%x, supp_info = x%x !\n", rd_uc_dsc_gp_uc_req(), rd_uc_dsc_supp_info()))); 
		return(_error(ERR_CODE_UC_CMD_RETURN_ERROR));
      } 
      return (ERR_CODE_NONE);
    }     
	if(loop>10) {
		err_code = eagle_tsc_delay_us(10*timeout_ms);
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
err_code_t eagle_tsc_poll_dsc_state_equals_uc_tune( const phymod_access_t *pa, uint32_t timeout_ms) {
  
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
		err_code = eagle_tsc_delay_us(10*timeout_ms);
		if(err_code) return(err_code);
	}
  }
  ESTM(USR_PRINTF(("DSC_STATE = %d\n", rd_dsc_state())));
  return (_error(ERR_CODE_POLLING_TIMEOUT));          /* Error Code for polling timeout */
}    





#endif /* CUSTOM_REG_POLLING */



/****************************************/
/*  Serdes Register/Variable Dump APIs  */
/****************************************/

err_code_t eagle_tsc_reg_dump( const phymod_access_t *pa ) {

  uint16_t addr, rddata;

  USR_PRINTF(("\n\n**********************************\n")); 
  USR_PRINTF(("****  SERDES REGISTER DUMP    ****\n")); 
  USR_PRINTF(("**********************************\n")); 
  USR_PRINTF(("****    ADDR      RD_VALUE    ****\n")); 
  USR_PRINTF(("**********************************\n"));

  for (addr = 0x0; addr < 0xF; addr++) {
    EFUN(eagle_tsc_pmd_rdt_reg(pa,addr,&rddata));
    USR_PRINTF(("       0x%04x      0x%04x\n",addr,rddata));
  }

  for (addr = 0x90; addr < 0x9F; addr++) {
    EFUN(eagle_tsc_pmd_rdt_reg(pa,addr,&rddata));
    USR_PRINTF(("       0x%04x      0x%04x\n",addr,rddata));
  }

  for (addr = 0xD000; addr < 0xD150; addr++) {
    EFUN(eagle_tsc_pmd_rdt_reg(pa,addr,&rddata));
    USR_PRINTF(("       0x%04x      0x%04x\n",addr,rddata));
  }

  for (addr = 0xD200; addr < 0xD230; addr++) {
    EFUN(eagle_tsc_pmd_rdt_reg(pa,addr,&rddata));
    USR_PRINTF(("       0x%04x      0x%04x\n",addr,rddata));
  }

  for (addr = 0xFFD0; addr < 0xFFE0; addr++) {
    EFUN(eagle_tsc_pmd_rdt_reg(pa,addr,&rddata));
    USR_PRINTF(("       0x%04x      0x%04x\n",addr,rddata));
  }
  return (ERR_CODE_NONE);
}


err_code_t eagle_tsc_uc_core_var_dump( const phymod_access_t *pa ) {

  uint8_t     addr, rddata;
  err_code_t  err_code = ERR_CODE_NONE;

  USR_PRINTF(("\n\n******************************************\n")); 
  USR_PRINTF(("**** SERDES UC CORE RAM VARIABLE DUMP ****\n")); 
  USR_PRINTF(("******************************************\n")); 
  USR_PRINTF(("****       ADDR       RD_VALUE        ****\n")); 
  USR_PRINTF(("******************************************\n")); 

  for (addr = 0x0; addr < 0xFF; addr++) {  
    rddata = eagle_tsc_rdbc_uc_var( pa, &err_code, addr);
    if (err_code) {
      return (err_code);
    }
    USR_PRINTF(("           0x%02x         0x%02x\n",addr,rddata));
  }
  return (ERR_CODE_NONE);
}


err_code_t eagle_tsc_uc_lane_var_dump( const phymod_access_t *pa ) {

  uint8_t     addr, rddata;
  err_code_t  err_code = ERR_CODE_NONE;

  USR_PRINTF(("\n\n********************************************\n")); 
  USR_PRINTF(("**** SERDES UC LANE %d RAM VARIABLE DUMP ****\n",eagle_tsc_get_lane(pa))); 
  USR_PRINTF(("********************************************\n")); 
  USR_PRINTF(("*****       ADDR       RD_VALUE        *****\n")); 
  USR_PRINTF(("********************************************\n")); 

    for (addr = 0x0; addr < 0xFF; addr++) {  
      rddata = eagle_tsc_rdbl_uc_var( pa, &err_code, addr);
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

err_code_t eagle_tsc_version( const phymod_access_t *pa, uint32_t *api_version) {

	if(!api_version) {
		return(_error(ERR_CODE_BAD_PTR_OR_INVALID_INPUT));
	}
	*api_version = 0xA10104;
	return (ERR_CODE_NONE);
}


/***************************************/
/*  API Function to Read Event Logger  */
/***************************************/

err_code_t eagle_tsc_read_event_log( const phymod_access_t *pa, uint8_t decode_enable) {
	return (ERR_CODE_NONE);
}


err_code_t eagle_tsc_display_state ( const phymod_access_t *pa ) {

  err_code_t err_code;
  err_code = ERR_CODE_NONE;

  if (!err_code) err_code = eagle_tsc_display_core_state( pa );
  if (!err_code) err_code = eagle_tsc_display_lane_state_hdr( pa );
  if (!err_code) err_code = eagle_tsc_display_lane_state( pa );

  return (err_code);
}

err_code_t eagle_tsc_display_config ( const phymod_access_t *pa ) {

  err_code_t err_code;
  err_code = ERR_CODE_NONE;

  if (!err_code) err_code = eagle_tsc_display_core_config( pa );
  if (!err_code) err_code = eagle_tsc_display_lane_config( pa );

  return (err_code);
}
