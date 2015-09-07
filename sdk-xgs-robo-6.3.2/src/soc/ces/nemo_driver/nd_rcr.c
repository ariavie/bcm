/* $Id: nd_rcr.c 1.5 Broadcom SDK $
 * Copyright 2011 BATM
 */

#include "pub/nd_api.h"
#include "nd_rcr.h"
#include "nd_registers.h"
#include "nd_debug.h"
#include "nd_util.h"


/*///////////////////////////////////////////////////////////////////////////// */
/* initializes the RCR module */
/*  */
void
ag_nd_rcr_init(AgNdDevice *p_device)
{
    AG_U32 i;

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);

    assert(p_device);

    /* */
    /* initalize RCR<->PSN channel id map */
    /* */
    /* there are 2 timing channels per port (one for primary clock and one for secondary) */
    /*  */
    for (i = 0; i < p_device->p_circuit_get_max_idx(p_device)*2; i++)
        p_device->a_rcr_channel[i].n_psn_channel_id = (AgNdChannel)-1;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_rcr_valid_bit_clear */
/* */
void
ag_nd_rcr_valid_bit_clear(AgNdDevice *p_device, AgNdChannel n_rcr_chid)
{
    ag_nd_reg_write(
        p_device, 
        AG_REG_TPP_CHANNEL_STATUS(n_rcr_chid >> 4), 
        (AG_U16) (1 << (n_rcr_chid & 0xf)));
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_rcr_valid_bit_get */
/* */
AG_BOOL
ag_nd_rcr_valid_bit_get(AgNdDevice *p_device, AgNdChannel n_rcr_chid)
{
    AG_U16  n_reg;

    ag_nd_reg_read(p_device, AG_REG_TPP_CHANNEL_STATUS(n_rcr_chid >> 4), &n_reg);

    return (n_reg >> (n_rcr_chid & 0xf)) & 0x1;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_rcr_config_tpp_timestamp_rate */
/* */
AgResult 
ag_nd_opcode_write_rcr_config_tpp_timestamp_rate(
    AgNdDevice *p_device, 
    AgNdMsgRcrConfigTppTimestampRate *p_msg)
{
    AgNdRegTppTimeStampSelect x_ts_select;
    
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "e_tsr_125=%d\n", p_msg->e_tsr_125);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "e_tsr_77=%d\n", p_msg->e_tsr_77);


#ifdef AG_ND_ENABLE_VALIDATION
    if (!p_device->b_rcr_support)
        return AG_ND_ERR(p_device, AG_E_ND_UNSUPPORTED);

    if (p_msg->e_tsr_77 != AG_ND_RCR_TS_77_RATE_1_MHZ &&
        p_msg->e_tsr_77 != AG_ND_RCR_TS_77_RATE_19_44_MHZ &&
        p_msg->e_tsr_77 != AG_ND_RCR_TS_77_RATE_77_76_MHZ)
    {
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
    }   

    if (p_msg->e_tsr_125 != AG_ND_RCR_TS_125_RATE_1_MHZ &&
        p_msg->e_tsr_125 != AG_ND_RCR_TS_125_RATE_125_MHZ)
    {
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
    }   
#endif

    x_ts_select.x_fields.n_tsr_125 = (field_t)p_msg->e_tsr_125;
    x_ts_select.x_fields.n_tsr_77 =  (field_t)p_msg->e_tsr_77;

    ag_nd_reg_write(p_device, AG_REG_TPP_CONFIG_TIMESTAMP_RATE, x_ts_select.n_reg);

    return AG_S_OK;

}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_rcr_config_tpp_timestamp_rate */
/* */
AgResult 
ag_nd_opcode_read_rcr_config_tpp_timestamp_rate(
    AgNdDevice *p_device, 
    AgNdMsgRcrConfigTppTimestampRate *p_msg)
{
    AgNdRegTppTimeStampSelect x_ts_select;
    
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);


#ifdef AG_ND_ENABLE_VALIDATION
    if (!p_device->b_rcr_support)
        return AG_ND_ERR(p_device, AG_E_ND_UNSUPPORTED);
#endif


    ag_nd_reg_read(p_device, AG_REG_TPP_CONFIG_TIMESTAMP_RATE, &(x_ts_select.n_reg));

    p_msg->e_tsr_125 = x_ts_select.x_fields.n_tsr_125;
    p_msg->e_tsr_77 = x_ts_select.x_fields.n_tsr_77;

    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_rcr_config_channel */
/* */
AgResult 
ag_nd_opcode_write_rcr_config_channel(AgNdDevice *p_device, AgNdMsgRcrConfigChannel *p_msg)
{
    AgNdRegClkSourceSelection       x_source;
    AgNdRegRcrChannelConfiguration  x_conf;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_rcr_channel_id=%hu\n", p_msg->n_rcr_channel_id);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_psn_channel_id=%hu\n", p_msg->n_psn_channel_id);

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_enable=%lu\n", p_msg->b_enable);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_fll_peak_detector_enable=%lu\n", p_msg->b_fll_peak_detector_enable);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_phase_slope_limit_enable=%lu\n", p_msg->b_phase_slope_limit_enable);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_pll_peak_detector_enable=%lu\n", p_msg->b_pll_peak_detector_enable);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_phase_slope_threshold=%hu\n", p_msg->n_phase_slope_threshold);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_sampling_period=%hu\n", p_msg->n_sampling_period);


#ifdef AG_ND_ENABLE_VALIDATION
    if (!p_device->b_rcr_support)
        return AG_ND_ERR(p_device, AG_E_ND_UNSUPPORTED);

    if (p_msg->n_rcr_channel_id >= (p_device->p_circuit_get_max_idx(p_device)*2))
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif


    if (!p_msg->b_enable)
    {
        /* */
        /* disable RCR channel in HW */
        /* */
        x_source.n_reg = 0;
        x_source.x_fields.b_clk_source_enable = AG_FALSE;
        x_source.x_fields.n_target_rcr_channel = 0;

        ag_nd_reg_write(
            p_device, 
            AG_REG_CLK_SOURCE_SELECTION(p_device->a_rcr_channel[p_msg->n_rcr_channel_id].n_psn_channel_id),
            x_source.n_reg);

        /* */
        /* update RCR<->PSN channel id map */
        /* */
        p_device->a_rcr_channel[p_msg->n_rcr_channel_id].n_psn_channel_id = (AgNdChannel)-1;
    }
    else /* configure and enable RCR channel */
    {
        AG_U32 i;

        /* */
        /* check if PSN channel is not assigned already to another RCR channel */
        /* */
        if (p_msg->n_psn_channel_id >= p_device->n_pw_max)
            return AG_ND_ERR(p_device, AG_E_ND_ARG);

        /* */
        /*there are 2 timing channels per port (one for primary clock and one for secondary) */
        /*  */
        for (i = 0; i < p_device->p_circuit_get_max_idx(p_device)*2; i++)
        {
            if (i != p_msg->n_rcr_channel_id &&
				p_device->a_rcr_channel[i].n_psn_channel_id == p_msg->n_psn_channel_id)
            {
                return AG_ND_ERR(p_device, AG_E_ND_ASSIGNED);
            }
        }

        /* */
        /* other sanity checks */
        /*  */
        if (p_msg->e_rtp_ts_sample_period != AG_ND_RCR_TS_SAMPLE_PACKET &&
            p_msg->e_rtp_ts_sample_period != AG_ND_RCR_TS_SAMPLE_PERIOD)
            return AG_ND_ERR(p_device, AG_E_ND_ARG);


        if (p_msg->n_sampling_period < 1 || 
            p_msg->n_sampling_period > 8192 ||
            !ag_nd_is_power_of_2(p_msg->n_sampling_period))
            return AG_ND_ERR(p_device, AG_E_ND_ARG);

        if (p_msg->n_phase_slope_threshold & 0xffff)
            return AG_ND_ERR(p_device, AG_E_ND_ARG);

        /* */
        /* update RCR<->PSN channel id map */
        /* */
        p_device->a_rcr_channel[p_msg->n_rcr_channel_id].n_psn_channel_id = p_msg->n_psn_channel_id;

        /* */
        /* clean the RCR channel's valid bit to be on the safe side */
        /* */
        ag_nd_rcr_valid_bit_clear(p_device, p_msg->n_rcr_channel_id);

        /* */
        /* configure RCR channel */
        /*  */
        x_conf.n_reg = 0;
        x_conf.x_fields.b_fll_peak_detector_enable = (field_t) p_msg->b_fll_peak_detector_enable;
        x_conf.x_fields.b_phase_slope_limit_enable = (field_t) p_msg->b_phase_slope_limit_enable;
        x_conf.x_fields.b_pll_peak_detector_enable = (field_t) p_msg->b_pll_peak_detector_enable;
        x_conf.x_fields.b_timestamp_sampling_period = (field_t) p_msg->e_rtp_ts_sample_period;
        x_conf.x_fields.n_sampling_period_size = (field_t) ag_nd_log2(p_msg->n_sampling_period);

        ag_nd_reg_write(
            p_device, 
            AG_REG_TIMING_CHANNEL_CONFIGURATION(p_msg->n_rcr_channel_id), 
            x_conf.n_reg);

        /* */
        /* configure phase slope threshold */
        /* */
        ag_nd_reg_write(
            p_device,
            AG_REG_PHASE_SLOPE_LIMIT_THRESHOLD(p_msg->n_rcr_channel_id),
            (field_t)p_msg->n_phase_slope_threshold);

        /* */
        /* bind RCR channel to PSN and enable */
        /* */
        x_source.n_reg = 0;
        x_source.x_fields.b_clk_source_enable = AG_TRUE;
        x_source.x_fields.n_target_rcr_channel = p_msg->n_rcr_channel_id;

        ag_nd_reg_write(
            p_device,
            AG_REG_CLK_SOURCE_SELECTION(p_msg->n_psn_channel_id),
            x_source.n_reg);        
    }


    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_rcr_config_channel */
/* */
AgResult 
ag_nd_opcode_read_rcr_config_channel(AgNdDevice *p_device, AgNdMsgRcrConfigChannel *p_msg)
{
    AgNdRegRcrChannelConfiguration  x_conf;
    AG_U16                          n_threshold;

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);

#ifdef AG_ND_ENABLE_VALIDATION
    if (!p_device->b_rcr_support)
        return AG_ND_ERR(p_device, AG_E_ND_UNSUPPORTED);

    /* */
    /*there are 2 timing channels per port (one for primary clock and one for secondary) */
    /*  */
    if (p_msg->n_rcr_channel_id >= p_device->p_circuit_get_max_idx(p_device)*2)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif

    if ((AgNdChannel)-1 == p_device->a_rcr_channel[p_msg->n_rcr_channel_id].n_psn_channel_id)
    {
        p_msg->b_enable = AG_FALSE;
        return AG_S_OK;
    }

    ag_nd_reg_read(
        p_device, 
        AG_REG_TIMING_CHANNEL_CONFIGURATION(p_msg->n_rcr_channel_id), 
        &(x_conf.n_reg));

    ag_nd_reg_read(
        p_device, 
        AG_REG_PHASE_SLOPE_LIMIT_THRESHOLD(p_msg->n_rcr_channel_id), 
        &n_threshold);

    p_msg->n_psn_channel_id = p_device->a_rcr_channel[p_msg->n_rcr_channel_id].n_psn_channel_id;
    p_msg->b_pll_peak_detector_enable = x_conf.x_fields.b_pll_peak_detector_enable;
    p_msg->b_fll_peak_detector_enable = x_conf.x_fields.b_fll_peak_detector_enable;
    p_msg->b_phase_slope_limit_enable = x_conf.x_fields.b_phase_slope_limit_enable;
    p_msg->n_phase_slope_threshold = n_threshold;
    p_msg->n_sampling_period = 1 << x_conf.x_fields.n_sampling_period_size;
    p_msg->b_enable = AG_TRUE;

    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_rcr_timestamp */
/* */
AgResult 
ag_nd_opcode_read_rcr_timestamp(AgNdDevice *p_device, AgNdMsgRcrTs *p_msg)
{
    AG_U16  n_hi, n_lo;

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);


#ifdef AG_ND_ENABLE_VALIDATION
    if (!p_device->b_rcr_support)
        return AG_ND_ERR(p_device, AG_E_ND_UNSUPPORTED);
#endif


    ag_nd_reg_read(p_device, AG_REG_125MHZ_ORIGINATED_TPP_TIMESTAMP_1, &n_hi);
    ag_nd_reg_read(p_device, AG_REG_125MHZ_ORIGINATED_TPP_TIMESTAMP_2, &n_lo);

    p_msg->n_ts_125 = AG_ND_MAKE32LH(n_lo, n_hi);

    if (0 /*AG_ND_ARCH_MASK_NEPTUNE == p_device->n_chip_mask*/)   /*BCM, do not use*/
    {
        ag_nd_reg_read(p_device, AG_REG_77MHZ_ORIGINATED_TPP_TIMESTAMP_1, &n_hi);
        ag_nd_reg_read(p_device, AG_REG_77MHZ_ORIGINATED_TPP_TIMESTAMP_2, &n_lo);

        p_msg->n_ts_77 = AG_ND_MAKE32LH(n_lo, n_hi);
    }
    else
        p_msg->n_ts_77 = 0;

    return AG_S_OK;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_rcr_pm */
/* */
AgResult 
ag_nd_opcode_read_rcr_pm(AgNdDevice *p_device, AgNdMsgRcrPm *p_msg)
{
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);


#ifdef AG_ND_ENABLE_VALIDATION
    if (!p_device->b_rcr_support)
        return AG_ND_ERR(p_device, AG_E_ND_UNSUPPORTED);

    /* */
    /*there are 2 timing channels per port (one for primary clock and one for secondary) */
    /*  */
    if (p_msg->n_rcr_channel_id >= p_device->p_circuit_get_max_idx(p_device)*2)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif


    ag_nd_reg_read(
        p_device, 
        AG_REG_PHASE_SLOPE_LIMIT_PM_COUNTER(p_msg->n_rcr_channel_id), 
        &(p_msg->n_phase_slope_limiter_dropped));

    ag_nd_reg_read(
        p_device, 
        AG_REG_PEAK_DETECTOR_PM_COUNTER(p_msg->n_rcr_channel_id), 
        &(p_msg->n_peak_detector_dropped));

    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_rcr_fll */
/* */
AgResult 
ag_nd_opcode_read_rcr_fll(AgNdDevice *p_device, AgNdMsgRcrFll *p_msg)
{
    AG_U16 n_lo, n_hi;

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);


#ifdef AG_ND_ENABLE_VALIDATION
    if (!p_device->b_rcr_support)
        return AG_ND_ERR(p_device, AG_E_ND_UNSUPPORTED);

    /* */
    /*there are 2 timing channels per port (one for primary clock and one for secondary) */
    /*  */
    if (p_msg->n_rcr_channel_id >= p_device->p_circuit_get_max_idx(p_device)*2)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

#endif

    /* */
    /* check if enabled */
    /* */
    if ((AgNdChannel)-1 == p_device->a_rcr_channel[p_msg->n_rcr_channel_id].n_psn_channel_id)
        return AG_E_ND_ENABLE;


    /* */
    /* check is period results are loaded */
    /*  */
    if (!ag_nd_rcr_valid_bit_get(p_device, p_msg->n_rcr_channel_id))
        return AG_E_ND_IN_PROGRESS;


    /* */
    /* retrieve the results */
    /* */
    ag_nd_reg_read(p_device, AG_REG_ARRIVAL_TIMESTAMP_BASE_1(p_msg->n_rcr_channel_id), &n_hi);
    ag_nd_reg_read(p_device, AG_REG_ARRIVAL_TIMESTAMP_BASE_2(p_msg->n_rcr_channel_id), &n_lo);
    p_msg->n_timestamp_base = AG_ND_MAKE32LH(n_lo, n_hi);

    ag_nd_reg_read(p_device, AG_REG_ARRIVAL_TIMESTAMP_SIGMA_1(p_msg->n_rcr_channel_id), &n_hi);
    ag_nd_reg_read(p_device, AG_REG_ARRIVAL_TIMESTAMP_SIGMA_2(p_msg->n_rcr_channel_id), &n_lo);
    p_msg->n_timestamp_delta_sum = AG_ND_MAKE32LH(n_lo, n_hi);

    ag_nd_reg_read(p_device, AG_REG_SQN_FILLER_SIGMA_1(p_msg->n_rcr_channel_id), &n_hi);
    ag_nd_reg_read(p_device, AG_REG_SQN_FILLER_SIGMA_2(p_msg->n_rcr_channel_id), &n_lo);
    p_msg->n_sqn_filler_num = AG_ND_MAKE32LH(n_lo, n_hi);

    ag_nd_reg_read(p_device, AG_REG_ARRIVAL_TIMESTAMP_COUNT(p_msg->n_rcr_channel_id), &n_lo);
    p_msg->n_timestamp_count = n_lo;

    /* */
    /* clear the valid bit */
    /* */
    ag_nd_rcr_valid_bit_clear(p_device, p_msg->n_rcr_channel_id);


    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_rcr_pll */
/* */
AgResult 
ag_nd_opcode_read_rcr_pll(AgNdDevice *p_device, AgNdMsgRcrPll *p_msg)
{
    AG_U16  n_hi, n_lo;

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);


#ifdef AG_ND_ENABLE_VALIDATION
    if (!p_device->b_rcr_support)
        return AG_ND_ERR(p_device, AG_E_ND_UNSUPPORTED);
#endif

    /* */
    /*there are 2 timing channels per port (one for primary clock and one for secondary) */
    /*  */
    if (p_msg->n_rcr_channel_id >= p_device->p_circuit_get_max_idx(p_device)*2)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    ag_nd_reg_read(p_device, AG_REG_JITTER_BUFFER_DEPTH_PACKET_COUNT(p_msg->n_rcr_channel_id), &(p_msg->n_jbf_depth_count));

    ag_nd_reg_read(p_device, AG_REG_JITTER_BUFFER_DEPTH_ACCUMULATION_1(p_msg->n_rcr_channel_id), &n_hi);
    ag_nd_reg_read(p_device, AG_REG_JITTER_BUFFER_DEPTH_ACCUMULATION_2(p_msg->n_rcr_channel_id), &n_lo);
    p_msg->n_jbf_depth_sum = AG_ND_MAKE32LH(n_lo, n_hi);

    ag_nd_reg_read(p_device, AG_REG_TIMING_INTERFACE_JITTER_BUFFER_MAXIMUM_DEPTH_1(p_msg->n_rcr_channel_id), &n_hi);
    ag_nd_reg_read(p_device, AG_REG_TIMING_INTERFACE_JITTER_BUFFER_MAXIMUM_DEPTH_2(p_msg->n_rcr_channel_id), &n_lo);
    p_msg->n_jbf_depth_max = AG_ND_MAKE32LH(n_lo, n_hi);

    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_rcr_recent_packet */
/* */
AgResult 
ag_nd_opcode_read_rcr_recent_packet(AgNdDevice *p_device, AgNdMsgRcrRecent *p_msg)
{
    AG_U16  n_hi, n_lo;

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);


#ifdef AG_ND_ENABLE_VALIDATION
    if (!p_device->b_rcr_support)
        return AG_ND_ERR(p_device, AG_E_ND_UNSUPPORTED);

    /* */
    /*there are 2 timing channels per port (one for primary clock and one for secondary) */
    /*  */
    if (p_msg->n_rcr_channel_id >= p_device->p_circuit_get_max_idx(p_device)*2)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif

    ag_nd_reg_read(p_device, AG_REG_RECEIVED_PACKET_RTP_TIMESTAMP_1(p_msg->n_rcr_channel_id), &n_hi);
    ag_nd_reg_read(p_device, AG_REG_RECEIVED_PACKET_RTP_TIMESTAMP_2(p_msg->n_rcr_channel_id), &n_lo);
    p_msg->n_timestamp = AG_ND_MAKE32LH(n_lo, n_hi);

    return AG_S_OK;
}

