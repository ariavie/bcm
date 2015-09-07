/* $Id: b98a4046e18a661b65c9ea71a0cba4c837e8b577 $
 * Copyright 2011 BATM
 */


#include "pub/nd_api.h"
#include "nd_nemo_diff.h"
#include "nd_registers.h"
#include "nd_debug.h"
#include "nd_util.h"


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_config_nemo_dcr_configurations */
/* */
AgResult 
ag_nd_opcode_write_config_nemo_dcr_configurations(
    AgNdDevice *p_device, 
    AgNdMsgConfigNemoDcrConfigurations *p_msg)
{    
    AgNdRegRtpTimestampGeneratorConfiguration x_rtp_config;
    AG_U16 n_data;

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_ts_generation_enable=%ld\n",p_msg->b_ts_generation_enable);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_fast_phase_enable=%ld\n", p_msg->b_fast_phase_enable);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_err_correction_enable=%ld\n", p_msg->b_err_correction_enable);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_prime_ts_sample_period=%u\n", p_msg->n_prime_ts_sample_period);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_rtp_tsx_ts_step_size=%lu\n", p_msg->n_rtp_tsx_ts_step_size);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_rtp_tsx_fast_phase_step_size=%u\n", p_msg->n_rtp_tsx_fast_phase_step_size);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_rtp_tsx_error_corrrection_period=%u\n", p_msg->n_rtp_tsx_error_corrrection_period);


#ifdef AG_ND_ENABLE_VALIDATION
    if (!p_device->b_rcr_support)
        return AG_ND_ERR(p_device, AG_E_ND_UNSUPPORTED);

    if (p_msg->n_prime_ts_sample_period > 0xFFF)/*2^12-1) */
    {
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
    }   

    if (p_msg->n_rtp_tsx_ts_step_size > 0x1FFFFFFF)/*2^29-1) */
    {
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
    }   

    if (p_msg->n_rtp_tsx_fast_phase_step_size > 0x3FF)/*2^10-1) */
    {
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
    }   
#endif


    memset(&x_rtp_config,0,sizeof(x_rtp_config));
    x_rtp_config.x_fields.b_timestamp_generation_enable = (field_t)p_msg->b_ts_generation_enable;
    x_rtp_config.x_fields.b_fast_phase_enable = (field_t)p_msg->b_fast_phase_enable;
    x_rtp_config.x_fields.b_error_correction_enable = (field_t)p_msg->b_err_correction_enable;
    x_rtp_config.x_fields.n_prime_timestamp_step_size = (field_t)p_msg->n_prime_ts_sample_period;
    ag_nd_reg_write(p_device, AG_REG_RTP_TIMESTAMP_GENERATOR_CONFIGURATION, x_rtp_config.n_reg);

    n_data = (AG_U16)((p_msg->n_rtp_tsx_ts_step_size >> 16) & 0x1FFF);
    ag_nd_reg_write(p_device,AG_REG_RTP_TIMESTAMP_STEP_SIZE_1,n_data);
    n_data = (AG_U16)(p_msg->n_rtp_tsx_ts_step_size & 0xFFFF);
    ag_nd_reg_write(p_device,AG_REG_RTP_TIMESTAMP_STEP_SIZE_2,n_data);

    n_data = (AG_U16)((p_msg->n_rtp_tsx_fast_phase_step_size) & 0x3FF);
    ag_nd_reg_write(p_device,AG_REG_RTP_TIMESTAMP_FAST_PHASE_CONFIGURATION,n_data);

    n_data = (AG_U16)(p_msg->n_rtp_tsx_error_corrrection_period);
    ag_nd_reg_write(p_device,AG_REG_RTP_TIMESTAMP_ERROR_CORRECTION_PERIOD,n_data);

    return AG_S_OK;

}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_rcr_config_tpp_timestamp_rate */
/* */
AgResult 
ag_nd_opcode_read_config_nemo_dcr_configurations(
    AgNdDevice *p_device, 
    AgNdMsgConfigNemoDcrConfigurations *p_msg)
{
    AgNdRegRtpTimestampGeneratorConfiguration x_rtp_config;
    AG_U16  n_hi, n_lo;

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);


#ifdef AG_ND_ENABLE_VALIDATION
    if (!p_device->b_rcr_support)
        return AG_ND_ERR(p_device, AG_E_ND_UNSUPPORTED);
#endif


    ag_nd_reg_read(p_device, AG_REG_RTP_TIMESTAMP_GENERATOR_CONFIGURATION, &(x_rtp_config.n_reg));
    p_msg->b_ts_generation_enable = x_rtp_config.x_fields.b_timestamp_generation_enable;
    p_msg->b_fast_phase_enable = x_rtp_config.x_fields.b_fast_phase_enable;
    p_msg->b_err_correction_enable = x_rtp_config.x_fields.b_error_correction_enable;
    p_msg->n_prime_ts_sample_period = x_rtp_config.x_fields.n_prime_timestamp_step_size;

    ag_nd_reg_read(p_device, AG_REG_RTP_TIMESTAMP_STEP_SIZE_1, &n_hi);
    ag_nd_reg_read(p_device, AG_REG_RTP_TIMESTAMP_STEP_SIZE_2, &n_lo);
    p_msg->n_rtp_tsx_ts_step_size = AG_ND_MAKE32LH(n_lo, n_hi);

    ag_nd_reg_read(p_device,AG_REG_RTP_TIMESTAMP_FAST_PHASE_CONFIGURATION,&(p_msg->n_rtp_tsx_fast_phase_step_size));

    ag_nd_reg_read(p_device,AG_REG_RTP_TIMESTAMP_ERROR_CORRECTION_PERIOD,&(p_msg->n_rtp_tsx_error_corrrection_period));

    return AG_S_OK;
}



/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_config_nemo_dcr_configurations */
/* */
AgResult 
ag_nd_opcode_write_config_nemo_dcr_clk_source(
    AgNdDevice *p_device, 
    AgNdMsgConfigNemoDcrClkSource *p_msg)
{    
    AgNdRegDcrReferenceClockSourceSelection x_rtp_config;

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "e_clk_source=%d\n",p_msg->e_clk_source);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_tdm_port=%d\n", p_msg->n_tdm_port);


#ifdef AG_ND_ENABLE_VALIDATION
    if (!p_device->b_rcr_support)
        return AG_ND_ERR(p_device, AG_E_ND_UNSUPPORTED);

    if (p_msg->e_clk_source != AG_ND_DCR_TDM_PORT_RX_CLK)
    {
		if (p_msg->n_tdm_port != 0)
    	    return AG_ND_ERR(p_device, AG_E_ND_ARG);
    }
	else if ((p_msg->n_tdm_port != 0xf) &&
		     (!p_device->p_circuit_is_valid_id(p_device, p_msg->n_tdm_port)))
	{
		return AG_ND_ERR(p_device, AG_E_ND_ARG);
	}
#endif


    memset(&x_rtp_config,0,sizeof(x_rtp_config));
    x_rtp_config.x_fields.n_port_select = (field_t)p_msg->n_tdm_port;
    x_rtp_config.x_fields.n_ref_clock = (field_t)p_msg->e_clk_source;
    ag_nd_reg_write(p_device, AG_REG_DCR_COMMON_REF_CLK_SOURCE_SELECTION, x_rtp_config.n_reg);

    return AG_S_OK;

}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_config_nemo_dcr_clk_source */
/* */
AgResult 
ag_nd_opcode_read_config_nemo_dcr_clk_source(
    AgNdDevice *p_device, 
    AgNdMsgConfigNemoDcrClkSource *p_msg)
{
    AgNdRegDcrReferenceClockSourceSelection x_rtp_config;


#ifdef AG_ND_ENABLE_VALIDATION
    if (!p_device->b_rcr_support)
        return AG_ND_ERR(p_device, AG_E_ND_UNSUPPORTED);
#endif


    ag_nd_reg_read(p_device, AG_REG_DCR_COMMON_REF_CLK_SOURCE_SELECTION, &(x_rtp_config.n_reg));
    p_msg->n_tdm_port = x_rtp_config.x_fields.n_port_select;
    p_msg->e_clk_source = x_rtp_config.x_fields.n_ref_clock;

    return AG_S_OK;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_nemo_dcr_local_ts */
/* */
AgResult 
ag_nd_opcode_read_nemo_dcr_local_ts(
    AgNdDevice *p_device, 
    AgNdMsgNemoDcrLocalTs *p_msg)
{
    AG_U16  n_hi, n_lo;
    AG_U16  n_fs_data;


#ifdef AG_ND_ENABLE_VALIDATION
    if (!p_device->b_rcr_support)
        return AG_ND_ERR(p_device, AG_E_ND_UNSUPPORTED);

    if (!p_device->p_circuit_is_valid_id(p_device, p_msg->n_circuit_id))
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif


    /*must read AG_REG_RECOVERED_CLK_TIMESTAMP_SAMPLING_PERIOD_1 first to latch fast phase value */
    ag_nd_reg_read(p_device, AG_REG_RECOVERED_CLK_LOCAL_TIMESTAMP_1(p_msg->n_circuit_id), &n_hi);
    ag_nd_reg_read(p_device, AG_REG_RECOVERED_CLK_LOCAL_TIMESTAMP_2(p_msg->n_circuit_id), &n_lo);
    ag_nd_reg_read(p_device, AG_REG_RECOVERED_CLK_FAST_PHASE_TIMESTAMP(p_msg->n_circuit_id), &n_fs_data);

    p_msg->n_timestamp = AG_ND_MAKE32LH(n_lo, n_hi);
    p_msg->n_fast_phase_timestamp = (AG_U8)(n_fs_data & 0xFF);

    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_nemo_dcr_local_prime_ts */
/* */
AgResult 
ag_nd_opcode_read_nemo_dcr_local_prime_ts(
    AgNdDevice *p_device, 
    AgNdMsgNemoDcrLocalPrimeTs *p_msg)
{
    AG_U16  n_hi, n_lo;


#ifdef AG_ND_ENABLE_VALIDATION
    if (!p_device->b_rcr_support)
        return AG_ND_ERR(p_device, AG_E_ND_UNSUPPORTED);

    if (!p_device->p_circuit_is_valid_id(p_device, p_msg->n_circuit_id))
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif


    ag_nd_reg_read(p_device, AG_REG_RECOVERED_CLK_PRIME_LOCAL_TIMESTAMP_1(p_msg->n_circuit_id), &n_hi);
    ag_nd_reg_read(p_device, AG_REG_RECOVERED_CLK_PRIME_LOCAL_TIMESTAMP_2(p_msg->n_circuit_id), &n_lo);

    p_msg->n_prime_timestamp = AG_ND_MAKE32LH(n_lo, n_hi);

    return AG_S_OK;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_config_nemo_dcr_local_sample_period */
/* */
AgResult 
ag_nd_opcode_read_config_nemo_dcr_local_sample_period(
    AgNdDevice *p_device, 
    AgNdMsgConfigNemoDcrSamplePeriod *p_msg)
{
    AG_U16  n_data;
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);


#ifdef AG_ND_ENABLE_VALIDATION
    if (!p_device->b_rcr_support)
        return AG_ND_ERR(p_device, AG_E_ND_UNSUPPORTED);

    if (!p_device->p_circuit_is_valid_id(p_device, p_msg->n_circuit_id))
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

#endif

    ag_nd_reg_read(p_device,AG_REG_RECOVERED_CLK_TIMESTAMP_SAMPLING_PERIOD_1(p_msg->n_circuit_id),&n_data);
    p_msg->n_sample_period = (n_data & 0x7);
	p_msg->n_sample_period >>= 16;

    ag_nd_reg_read(p_device,AG_REG_RECOVERED_CLK_TIMESTAMP_SAMPLING_PERIOD_2(p_msg->n_circuit_id),&n_data);
	p_msg->n_sample_period |= n_data;

    return AG_S_OK;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_config_nemo_dcr_local_sample_period */
/* */
AgResult 
ag_nd_opcode_write_config_nemo_dcr_local_sample_period(
    AgNdDevice *p_device, 
    AgNdMsgConfigNemoDcrSamplePeriod *p_msg)
{
    AG_U16  n_data;
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);


#ifdef AG_ND_ENABLE_VALIDATION
    if (!p_device->b_rcr_support)
        return AG_ND_ERR(p_device, AG_E_ND_UNSUPPORTED);

    if (!p_device->p_circuit_is_valid_id(p_device, p_msg->n_circuit_id))
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if (p_msg->n_sample_period > 0x7FFFF)/*2^19-1) */
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif


    n_data = (AG_U16)((p_msg->n_sample_period >> 16) & 0x7);
    ag_nd_reg_write(p_device,AG_REG_RECOVERED_CLK_TIMESTAMP_SAMPLING_PERIOD_1(p_msg->n_circuit_id),n_data);
    n_data = (AG_U16)(p_msg->n_sample_period & 0xFFFF);
    ag_nd_reg_write(p_device,AG_REG_RECOVERED_CLK_TIMESTAMP_SAMPLING_PERIOD_2(p_msg->n_circuit_id),n_data);

    return AG_S_OK;
}




/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_nemo_dcr_system_local_ts */
/* */
AgResult 
ag_nd_opcode_read_nemo_dcr_system_local_ts(
    AgNdDevice *p_device, 
    AgNdMsgNemoDcrLocalTs *p_msg)
{
    AG_U16  n_hi, n_lo;
    AG_U16  n_fs_data;


#ifdef AG_ND_ENABLE_VALIDATION
    if (!p_device->b_rcr_support)
        return AG_ND_ERR(p_device, AG_E_ND_UNSUPPORTED);

    if (p_msg->n_circuit_id >= AG_ND_SYSTEM_IDX_MAX)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif


    /*must read AG_REG_RECOVERED_CLK_TIMESTAMP_SAMPLING_PERIOD_1 first to latch fast phase value */
	/*ORI*/
	/*FIX READ CORRECT REGS */
    
    ag_nd_reg_read(p_device, AG_REG_RECOVERED_CLK_SYSTEM_LOCAL_TIMESTAMP_1(p_msg->n_circuit_id), &n_hi);
    ag_nd_reg_read(p_device, AG_REG_RECOVERED_CLK_SYSTEM_LOCAL_TIMESTAMP_2(p_msg->n_circuit_id), &n_lo);
    ag_nd_reg_read(p_device, AG_REG_RECOVERED_CLK_SYSTEM_FAST_PHASE_TIMESTAMP(p_msg->n_circuit_id), &n_fs_data);

    p_msg->n_timestamp = AG_ND_MAKE32LH(n_lo, n_hi);
    p_msg->n_fast_phase_timestamp = (AG_U8)(n_fs_data & 0xFF);

    return AG_S_OK;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_nemo_dcr_local_prime_ts */
/* */
AgResult 
ag_nd_opcode_read_nemo_dcr_system_local_prime_ts(
    AgNdDevice *p_device, 
    AgNdMsgNemoDcrLocalPrimeTs *p_msg)
{
    AG_U16  n_hi, n_lo;


#ifdef AG_ND_ENABLE_VALIDATION
    if (!p_device->b_rcr_support)
        return AG_ND_ERR(p_device, AG_E_ND_UNSUPPORTED);

    if (p_msg->n_circuit_id >= AG_ND_SYSTEM_IDX_MAX)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif


    ag_nd_reg_read(p_device, AG_REG_RECOVERED_CLK_PRIME_LOCAL_TIMESTAMP_1(p_msg->n_circuit_id), &n_hi);
    ag_nd_reg_read(p_device, AG_REG_RECOVERED_CLK_PRIME_LOCAL_TIMESTAMP_2(p_msg->n_circuit_id), &n_lo);

    p_msg->n_prime_timestamp = AG_ND_MAKE32LH(n_lo, n_hi);

    return AG_S_OK;
}





/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_config_nemo_dcr_system_local_sample_period */
/* */
AgResult 
ag_nd_opcode_read_config_nemo_dcr_system_local_sample_period(
    AgNdDevice *p_device, 
    AgNdMsgConfigNemoDcrSamplePeriod *p_msg)
{
    AG_U16  n_data;
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);


#ifdef AG_ND_ENABLE_VALIDATION
    if (!p_device->b_rcr_support)
        return AG_ND_ERR(p_device, AG_E_ND_UNSUPPORTED);

    if (p_msg->n_circuit_id >= AG_ND_SYSTEM_IDX_MAX)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

#endif

  	/*ORI*/
	/*need to tso */
    ag_nd_reg_read(p_device,AG_REG_RECOVERED_CLK_SYSTEM_TIMESTAMP_SAMPLING_PERIOD_1(p_msg->n_circuit_id),&n_data);
    p_msg->n_sample_period = (n_data & 0x7);
	p_msg->n_sample_period >>= 16;

    ag_nd_reg_read(p_device,AG_REG_RECOVERED_CLK_SYSTEM_TIMESTAMP_SAMPLING_PERIOD_2(p_msg->n_circuit_id),&n_data);
	p_msg->n_sample_period |= n_data;
    return AG_S_OK;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_config_nemo_dcr_local_sample_period */
/* */
AgResult 
ag_nd_opcode_write_config_nemo_dcr_system_local_sample_period(
    AgNdDevice *p_device, 
    AgNdMsgConfigNemoDcrSamplePeriod *p_msg)
{
    AG_U16  n_data;
    AG_U16  n_cur_val;
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);


#ifdef AG_ND_ENABLE_VALIDATION
    if (!p_device->b_rcr_support)
        return AG_ND_ERR(p_device, AG_E_ND_UNSUPPORTED);

    if (p_msg->n_circuit_id >= AG_ND_SYSTEM_IDX_MAX)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if (p_msg->n_sample_period > 0x7FFFF)/*2^19-1) */
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif
	/*ORI*/
	/*need to tso */
    n_data = (AG_U16)((p_msg->n_sample_period >> 16) & 0x7);
    ag_nd_reg_read(p_device,
    			AG_REG_RECOVERED_CLK_SYSTEM_TIMESTAMP_SAMPLING_PERIOD_1(p_msg->n_circuit_id),
    			&n_cur_val);
	n_cur_val &= 0xFFF8; /*clear bits 0-2  */

	n_cur_val |= n_data;
    ag_nd_reg_write(p_device,AG_REG_RECOVERED_CLK_SYSTEM_TIMESTAMP_SAMPLING_PERIOD_1(p_msg->n_circuit_id),n_cur_val);

    n_data = (AG_U16)(p_msg->n_sample_period & 0xFFFF);
    ag_nd_reg_write(p_device,AG_REG_RECOVERED_CLK_SYSTEM_TIMESTAMP_SAMPLING_PERIOD_2(p_msg->n_circuit_id),n_data);
    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_config_nemo_dcr_system_tso_mode */
/* */
AgResult 
ag_nd_opcode_read_config_nemo_dcr_system_tso_mode(
    AgNdDevice *p_device, 
    AgNdMsgConfigNemoDcrSystemTsoMode *p_msg)
{
    AgNdRegDcrSystemTsoMode x_tso_mode;
	AgNdRegDcrSystemTsoRefClockConfiguration x_tso_ref_clock;
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);


#ifdef AG_ND_ENABLE_VALIDATION
    if (!p_device->b_rcr_support)
        return AG_ND_ERR(p_device, AG_E_ND_UNSUPPORTED);

    if (p_msg->n_circuit_id >= AG_ND_SYSTEM_IDX_MAX)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

#endif

   /*ORI*/
	/*need to tso */
    ag_nd_reg_read(p_device,
    			AG_REG_RECOVERED_CLK_SYSTEM_TIMESTAMP_SAMPLING_PERIOD_1(p_msg->n_circuit_id),
    			&(x_tso_mode.n_reg));
	p_msg->e_tso_mode = x_tso_mode.x_fields.n_mode;

    ag_nd_reg_read(p_device,
				AG_REG_DCR_SYSTEM_TSO_CLK_SOURCE_SELECTION(p_msg->n_circuit_id),
				&(x_tso_ref_clock.n_reg));

	p_msg->e_clock_select = x_tso_ref_clock.x_fields.n_ref_select;

    return AG_S_OK;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_config_nemo_dcr_local_sample_period */
/* */
AgResult 
ag_nd_opcode_write_config_nemo_dcr_system_tso_mode(
			AgNdDevice	*p_device, 
			AgNdMsgConfigNemoDcrSystemTsoMode *p_msg)
{
    AgNdRegDcrSystemTsoMode x_tso_mode;
	AgNdRegDcrSystemTsoRefClockConfiguration x_tso_ref_clock;
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);


#ifdef AG_ND_ENABLE_VALIDATION
    if (!p_device->b_rcr_support)
        return AG_ND_ERR(p_device, AG_E_ND_UNSUPPORTED);

    if (p_msg->n_circuit_id >= AG_ND_SYSTEM_IDX_MAX)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif


    memset(&x_tso_mode,0,sizeof(x_tso_mode));
    memset(&x_tso_ref_clock,0,sizeof(x_tso_ref_clock));
   /*ORI*/
	/*need to tso */
    ag_nd_reg_read(p_device,
    			AG_REG_RECOVERED_CLK_SYSTEM_TIMESTAMP_SAMPLING_PERIOD_1(p_msg->n_circuit_id),
    			&(x_tso_mode.n_reg));
   
	x_tso_mode.x_fields.n_mode = (field_t)p_msg->e_tso_mode;
    ag_nd_reg_write(p_device,AG_REG_RECOVERED_CLK_SYSTEM_TIMESTAMP_SAMPLING_PERIOD_1(p_msg->n_circuit_id),x_tso_mode.n_reg);
	x_tso_ref_clock.x_fields.n_ref_select = (field_t)p_msg->e_clock_select;
    ag_nd_reg_write(p_device,AG_REG_DCR_SYSTEM_TSO_CLK_SOURCE_SELECTION(p_msg->n_circuit_id),x_tso_ref_clock.n_reg);



    return AG_S_OK;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_config_nemo_dcr_configurations */
/* */
AgResult 
ag_nd_opcode_write_config_nemo_dcr_system_clk_source(
    AgNdDevice *p_device, 
    AgNdMsgConfigNemoDcrSystemClkInputSource *p_msg)
{    
    AgNdRegDcrReferenceSystemClockSourceSelection x_rtp_config;

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "e_clk_source=%d\n",p_msg->e_clk_source);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_prime_sample_period=%d\n", p_msg->n_prime_sample_period);


#ifdef AG_ND_ENABLE_VALIDATION
    if (!p_device->b_rcr_support)
        return AG_ND_ERR(p_device, AG_E_ND_UNSUPPORTED);

    if (p_msg->n_circuit_id >= AG_ND_SYSTEM_IDX_MAX)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif


    memset(&x_rtp_config,0,sizeof(x_rtp_config));
    x_rtp_config.x_fields.n_prime_timestamp_step_size = (field_t)p_msg->n_prime_sample_period;
    x_rtp_config.x_fields.n_clock_source_select = (field_t)p_msg->e_clk_source;
    ag_nd_reg_write(p_device, AG_REG_DCR_SYSTEM_REF_CLK_SOURCE_SELECTION(p_msg->n_circuit_id), x_rtp_config.n_reg);

    return AG_S_OK;

}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_config_nemo_dcr_clk_source */
/* */
AgResult 
ag_nd_opcode_read_config_nemo_dcr_system_clk_source(
    AgNdDevice *p_device, 
    AgNdMsgConfigNemoDcrSystemClkInputSource  *p_msg)
{
    AgNdRegDcrReferenceSystemClockSourceSelection x_rtp_config;


#ifdef AG_ND_ENABLE_VALIDATION
    if (!p_device->b_rcr_support)
        return AG_ND_ERR(p_device, AG_E_ND_UNSUPPORTED);

	if (p_msg->n_circuit_id >= AG_ND_SYSTEM_IDX_MAX)
		return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif


    ag_nd_reg_read(p_device, AG_REG_DCR_SYSTEM_REF_CLK_SOURCE_SELECTION(p_msg->n_circuit_id), &(x_rtp_config.n_reg));
    p_msg->n_prime_sample_period = x_rtp_config.x_fields.n_prime_timestamp_step_size;
    p_msg->e_clk_source = x_rtp_config.x_fields.n_clock_source_select;

    return AG_S_OK;
}

