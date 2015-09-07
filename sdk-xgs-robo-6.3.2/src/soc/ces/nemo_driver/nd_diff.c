/* $Id: nd_diff.c 1.5 Broadcom SDK $
 * Copyright 2011 BATM
 */

#include "pub/nd_api.h"
#include "pub/nd_hw.h"
#include "nd_registers.h"
#include "nd_diff.h"
#include "nd_debug.h"		/*for AG_ND_TRACE */
/*#include "nd_bit.h" */

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_diff_config */
/* */
AgResult
ag_nd_opcode_read_config_neptune_dcr_configurations(AgNdDevice *p_device, AgNdMsgConfigNeptuneDcrConfigurations *p_msg)
{
	AgNdRegDiffRTPTimestampGeneratorConfiguration x_RTSGCFG;
	AgNdRegDiffRTPTimestampFrameStepSize x_RTSFSZ;
	AG_U16 n_RTSBSZ;
	AG_U16 n_rtp_rate;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);

    /* */
    /* RTP Timestamp Generator Configuration */
    /* */
    ag_nd_reg_read(p_device, AG_REG_RTP_TIMESTAMP_GENERATOR_CONFIGURATION_OC3, &x_RTSGCFG.n_reg);
	p_msg->b_TSE_enable = x_RTSGCFG.x_fields.b_timestamp_generation_enable;
	p_msg->b_bit_justification_toggle = x_RTSGCFG.x_fields.b_bit_justification_toggle_enable;
	p_msg->n_prime_timestamp_period = x_RTSGCFG.x_fields.n_prime_timestamp_sampling_period;

	/* */
    /* RTP Timestamp Frame Step Size */
    /* */
    ag_nd_reg_read(p_device, AG_REG_RTP_TIMESTAMP_FRAME_STEP_SIZE, &x_RTSFSZ.n_reg);
	p_msg->n_frame_step_size = x_RTSFSZ.x_fields.n_frame_step_size;

	/* */
    /* RTP Timestamp Bit Step Size */
    /* */
    ag_nd_reg_read(p_device, AG_REG_RTP_TIMESTAMP_BIT_STEP_SIZE, &n_RTSBSZ);
	p_msg->n_bit_step_size = n_RTSBSZ;

	ag_nd_reg_read(p_device, AG_REG_RTP_TIMESTAMP_RATE, &n_rtp_rate);
	p_msg->n_timestamp_rate = n_rtp_rate;

	
    return AG_S_OK;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_diff_config */
/* */
AgResult
ag_nd_opcode_write_config_neptune_dcr_configurations(AgNdDevice *p_device, AgNdMsgConfigNeptuneDcrConfigurations *p_msg)
{
	AgNdRegDiffRTPTimestampGeneratorConfiguration x_RTSGCFG;
	AgNdRegDiffRTPTimestampFrameStepSize x_RTSFSZ;
	AG_U16 n_RTSBSZ;
	AG_U16 n_rtp_rate;

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
	
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_TSE_enable=%lu\n", p_msg->b_TSE_enable);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_prime_timestamp_period=%lu\n", p_msg->n_prime_timestamp_period);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_bit_justification_toggle=%lu\n", p_msg->b_bit_justification_toggle);

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_frame_step_size=%lu\n", p_msg->n_frame_step_size);

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_bit_step_size=%lu\n", p_msg->n_bit_step_size);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_timestamp_rate=%lu\n", p_msg->n_timestamp_rate);
	
 	/* */
	/* RTP Timestamp Generator Configuration */
	/* */
	x_RTSGCFG.n_reg = 0x0000;		/*Clear bit field */
	x_RTSGCFG.x_fields.b_timestamp_generation_enable = p_msg->b_TSE_enable;
	x_RTSGCFG.x_fields.b_bit_justification_toggle_enable = p_msg->b_bit_justification_toggle;
	x_RTSGCFG.x_fields.n_prime_timestamp_sampling_period = p_msg->n_prime_timestamp_period;
	ag_nd_reg_write(p_device, AG_REG_RTP_TIMESTAMP_GENERATOR_CONFIGURATION_OC3, x_RTSGCFG.n_reg);
	
	/* */
	/* RTP Timestamp Frame Step Size */
	/* */
	x_RTSFSZ.n_reg = 0x0000;		/*Clear bit field */
	x_RTSFSZ.x_fields.n_frame_step_size = p_msg->n_frame_step_size;
	ag_nd_reg_write(p_device, AG_REG_RTP_TIMESTAMP_FRAME_STEP_SIZE, x_RTSFSZ.n_reg);
	
	/* */
	/* RTP Timestamp Bit Step Size */
	/* */
	n_RTSBSZ = (AG_U16)p_msg->n_bit_step_size;
	ag_nd_reg_write(p_device, AG_REG_RTP_TIMESTAMP_BIT_STEP_SIZE, n_RTSBSZ);

	/* */
	/* RTP Timestamp rate */
	/* */
	n_rtp_rate = (AG_U16)p_msg->n_timestamp_rate;
	ag_nd_reg_write(p_device, AG_REG_RTP_TIMESTAMP_RATE, n_rtp_rate);

	return AG_S_OK;
}



/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_bsg_config */
/* */
AgResult
ag_nd_opcode_read_bsg_config(AgNdDevice *p_device, AgNdMsgBsgConfig *p_msg)
{
	AgNdRegJustificationRequestControl x_JSTRQC;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);

#ifdef AG_ND_ENABLE_VALIDATION
	if (!p_device->p_circuit_is_valid_id(p_device, p_msg->n_circuit_id))
		return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif

    /* */
    /* Justification Request Control (BSG) */
    /* */
    ag_nd_reg_read(p_device, AG_REG_JUSTIFICATION_REQUEST_CONTROL(p_msg->n_circuit_id), &x_JSTRQC.n_reg);
	p_msg->b_justification_request = x_JSTRQC.x_fields.b_explicit_justification_request;
	p_msg->b_justification_polarity = x_JSTRQC.x_fields.n_explicit_justification_polarity;
	p_msg->b_justification_type = x_JSTRQC.x_fields.n_explicit_justification_type;
	p_msg->b_BSG_enable = x_JSTRQC.x_fields.b_bsg_enable;
	p_msg->b_request_polarity = x_JSTRQC.x_fields.n_bsg_request_polarity;
	p_msg->b_request_type = x_JSTRQC.x_fields.n_bsg_request_type;
	p_msg->b_bsg_count_enable = x_JSTRQC.x_fields.b_bsg_count_enable;

    return AG_S_OK;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_bsg_config */
/* */
AgResult
ag_nd_opcode_write_bsg_config(AgNdDevice *p_device, AgNdMsgBsgConfig *p_msg)
{
    AgNdRegJustificationRequestControl x_JSTRQC;
   
   #ifndef CES16_BCM_VERSION
    /*These variables are not used*/
	AgNdRegDiffRTPTimestampGeneratorConfiguration x_RTSGCFG;
	AgNdRegDiffRTPTimestampFrameStepSize x_RTSFSZ;
	AG_U16 n_RTSBSZ;
   #endif
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
	
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_explicit_justification_request=%lu\n", p_msg->b_justification_request);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_explicit_justification_polarity=%lu\n", p_msg->b_justification_polarity);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_explicit_justification_type=%lu\n", p_msg->b_justification_type);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_BSG_enable=%lu\n", p_msg->b_BSG_enable);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_BSG_request_polarity=%lu\n", p_msg->b_request_polarity);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_BSG_request_typet=%lu\n", p_msg->b_request_type);

#ifdef AG_ND_ENABLE_VALIDATION
	if (!p_device->p_circuit_is_valid_id(p_device, p_msg->n_circuit_id))
		return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif

	/* */
	/* Justification Request Control (BSG) */
	/* */
	x_JSTRQC.n_reg = 0x0000;		/*Clear bit field */
    x_JSTRQC.x_fields.b_explicit_justification_request = p_msg->b_justification_request;
	x_JSTRQC.x_fields.n_explicit_justification_polarity = p_msg->b_justification_polarity;
	x_JSTRQC.x_fields.n_explicit_justification_type = p_msg->b_justification_type;
	x_JSTRQC.x_fields.b_bsg_enable = p_msg->b_BSG_enable;
	x_JSTRQC.x_fields.n_bsg_request_polarity = p_msg->b_request_polarity;
	x_JSTRQC.x_fields.n_bsg_request_type = p_msg->b_request_type;
	x_JSTRQC.x_fields.b_bsg_count_enable = p_msg->b_bsg_count_enable;
	ag_nd_reg_write(p_device, AG_REG_JUSTIFICATION_REQUEST_CONTROL(p_msg->n_circuit_id), x_JSTRQC.n_reg);

    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_diff_poll */
/* */
AgResult
ag_nd_opcode_read_diff_tso_poll(AgNdDevice *p_device, AgNdMsgDiffTsoPoll *p_msg)
{
    /*BCMout not used AG_U16 n_data = 0;*/
	AgNdRegDiffRecoveredLinkRatePhaseIntegration x_RCVDPHI;

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);

#ifdef AG_ND_ENABLE_VALIDATION
	if (!p_device->p_circuit_is_valid_id(p_device, p_msg->n_circuit_id))
		return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif
	
    /* */
    /* Recovered Link-rate Phase Integration */
    /* */
    ag_nd_reg_read(p_device, AG_REG_RECOVERED_LINK_RATE_PHASE_INTEGRATOR(p_msg->n_circuit_id), &x_RCVDPHI.n_reg);
	p_msg->n_output_phase_shift = x_RCVDPHI.x_fields.n_output_phase_shift;

    return AG_S_OK;
}

