/* $Id: 1b4fdf010ad13301de8fee6e8bfaec52df5940bd $
 * Copyright 2011 BATM
 */

#include "nd_debug.h"
#include "nd_ptp.h"

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_config_ptp_tsg */
/*  */
AgResult 
ag_nd_opcode_read_config_ptp_tsg(
    AgNdDevice *p_device, 
    AgNdMsgConfigPtpTsg *p_msg)
{
  #ifndef CES16_BCM_VERSION
    AgNdRegPtpTimestampGeneratorsConfiguration  x_tsg_cfg;
    AG_U16 n_lo, n_hi;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);


    ag_nd_reg_read(p_device, AG_REG_PTP_TIMESTAMP_GENERATORS_CONFIGURATION, &(x_tsg_cfg.n_reg));
    p_msg->b_error_correction_enable = x_tsg_cfg.x_fields.b_error_correction_enable;
    p_msg->b_fast_phase_enable = x_tsg_cfg.x_fields.b_fast_phase_enable;
    p_msg->n_enable = x_tsg_cfg.x_fields.b_timestamp_generator_enable;

    ag_nd_reg_read(p_device, AG_REG_PTP_TIMESTAMP_ERROR_CORRECTION_PERIOD, &n_hi);
    p_msg->n_error_correction_period = n_hi;

    ag_nd_reg_read(p_device, AG_REG_PTP_TIMESTAMP_FAST_PHASE_CONFIGURATION, &n_hi);
    p_msg->n_fast_phase_step_size = n_hi;

    ag_nd_reg_read(p_device, AG_REG_PTP_TIMESTAMP_STEP_SIZE_INTEGER, &n_hi);
    p_msg->n_step_size_integer = n_hi;

    ag_nd_reg_read(p_device, AG_REG_PTP_TIMESTAMP_STEP_SIZE_FRACTION(0), &n_hi);
    ag_nd_reg_read(p_device, AG_REG_PTP_TIMESTAMP_STEP_SIZE_FRACTION(1), &n_lo);
    p_msg->n_step_size_fraction = AG_ND_MAKE16LH(n_lo, n_hi);

  #endif
    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_config_ptp_tsg */
/*  */
AgResult 
ag_nd_opcode_write_config_ptp_tsg(
    AgNdDevice *p_device, 
    AgNdMsgConfigPtpTsg *p_msg)
{
  #ifndef CES16_BCM_VERSION  
    AgNdRegPtpTimestampGeneratorsConfiguration  x_tsg_cfg;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_error_correction_enable=%lu\n", p_msg->b_error_correction_enable);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_error_correction_period=%lu\n", p_msg->n_error_correction_period);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_fast_phase_enable=%lu\n", p_msg->b_fast_phase_enable);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_fast_phase_step_size=%lu\n", p_msg->n_fast_phase_step_size);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_step_size_integer=%lu\n", p_msg->n_step_size_integer);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_step_size_fraction=%lu\n", p_msg->n_step_size_fraction);


    ag_nd_reg_read(
        p_device, 
        AG_REG_PTP_TIMESTAMP_GENERATORS_CONFIGURATION, 
        &(x_tsg_cfg.n_reg));

/*
    if (AG_TRUE == x_tsg_cfg.x_fields.b_timestamp_generator_enable)
        return AG_ND_ERR(p_device, AG_E_ND_ENABLE);
*/
    if (0xffff0000 & p_msg->n_error_correction_period)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if (0xfffff000 & p_msg->n_fast_phase_step_size)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if (0xfffffC00 & p_msg->n_step_size_integer)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if (0xff000000 & p_msg->n_step_size_fraction)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);


    x_tsg_cfg.x_fields.b_error_correction_enable = (field_t) p_msg->b_error_correction_enable;
    x_tsg_cfg.x_fields.b_fast_phase_enable = (field_t) p_msg->b_fast_phase_enable;
    ag_nd_reg_write(
        p_device, 
        AG_REG_PTP_TIMESTAMP_GENERATORS_CONFIGURATION, 
        x_tsg_cfg.n_reg);

    ag_nd_reg_write(
        p_device, 
        AG_REG_PTP_TIMESTAMP_ERROR_CORRECTION_PERIOD, 
        (AG_U16) p_msg->n_error_correction_period);

    ag_nd_reg_write(
        p_device, 
        AG_REG_PTP_TIMESTAMP_FAST_PHASE_CONFIGURATION, 
        (AG_U16) p_msg->n_fast_phase_step_size);

    ag_nd_reg_write(
        p_device, 
        AG_REG_PTP_TIMESTAMP_STEP_SIZE_INTEGER, 
        (AG_U16) p_msg->n_step_size_integer);

    ag_nd_reg_write(
        p_device, 
        AG_REG_PTP_TIMESTAMP_STEP_SIZE_FRACTION(0), 
        AG_ND_HIGH16(p_msg->n_step_size_fraction));

    ag_nd_reg_write(
        p_device, 
        AG_REG_PTP_TIMESTAMP_STEP_SIZE_FRACTION(1), 
        AG_ND_LOW16(p_msg->n_step_size_fraction));

  #endif
    return AG_S_OK;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_config_ptp_stateless */
/*  */
AgResult 
ag_nd_opcode_read_config_ptp_stateless(
    AgNdDevice *p_device, 
    AgNdMsgConfigPtpStateless *p_msg)
{
  #ifndef CES16_BCM_VERSION
    AgNdRegPtpFilter x_filter;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);


    ag_nd_reg_read(p_device, AG_REG_PTP_FILTER, &(x_filter.n_reg));

    p_msg->b_delay_request_enable = x_filter.x_fields.b_rx_delay_request_enable;
    p_msg->b_delay_response_enable = x_filter.x_fields.b_rx_delay_response_enable;
    p_msg->b_pdelay_request_enable = x_filter.x_fields.b_rx_pdelay_request_enable;
    p_msg->b_pdelay_response_enable = x_filter.x_fields.b_rx_pdelay_response_enable;
    p_msg->b_sync_enable = x_filter.x_fields.b_rx_sync_enable;
    p_msg->n_ptp_idx = x_filter.x_fields.n_clk_select;
  #endif
    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_config_ptp_stateless */
/*  */
AgResult 
ag_nd_opcode_write_config_ptp_stateless(
    AgNdDevice *p_device, 
    AgNdMsgConfigPtpStateless *p_msg)
{
  #ifndef CES16_BCM_VERSION

    AgNdRegPtpFilter x_filter;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_delay_request_enable=%lu\n", p_msg->b_delay_request_enable);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_delay_response_enable=%lu\n", p_msg->b_delay_response_enable);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_pdelay_request_enable=%lu\n", p_msg->b_pdelay_request_enable);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_pdelay_response_enable=%lu\n", p_msg->b_pdelay_response_enable);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_sync_enable=%lu\n", p_msg->b_sync_enable);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_sync_enable=%lu\n", p_msg->b_sync_enable);


    if (p_msg->n_ptp_idx >= AG_ND_PTP_IDX_MAX)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);    


    x_filter.n_reg = 0;
    x_filter.x_fields.b_rx_delay_request_enable = (field_t) p_msg->b_delay_request_enable;
    x_filter.x_fields.b_rx_delay_response_enable = (field_t) p_msg->b_delay_response_enable;
    x_filter.x_fields.b_rx_pdelay_request_enable = (field_t) p_msg->b_pdelay_request_enable;
    x_filter.x_fields.b_rx_pdelay_response_enable = (field_t) p_msg->b_pdelay_response_enable;
    x_filter.x_fields.b_rx_sync_enable = (field_t) p_msg->b_sync_enable;
    x_filter.x_fields.n_clk_select = (field_t) p_msg->n_ptp_idx;

    ag_nd_reg_write(p_device, AG_REG_PTP_FILTER, x_filter.n_reg);
 #endif

    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_config_ptp_enable */
/*  */
AgResult 
ag_nd_opcode_read_config_ptp_tsg_enable(
    AgNdDevice *p_device, 
    AgNdMsgConfigPtpTsgEnable *p_msg)
{
  #ifndef CES16_BCM_VERSION
    AgNdRegPtpTimestampGeneratorsConfiguration  x_tsg_cfg;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);

    ag_nd_reg_read(
        p_device, 
        AG_REG_PTP_TIMESTAMP_GENERATORS_CONFIGURATION, 
        &(x_tsg_cfg.n_reg));

    p_msg->b_enable = x_tsg_cfg.x_fields.b_timestamp_generator_enable;
   #endif

    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_config_ptp_tsg_enable */
/*  */
AgResult 
ag_nd_opcode_write_config_ptp_tsg_enable(
    AgNdDevice *p_device, 
    AgNdMsgConfigPtpTsgEnable *p_msg)
{
  #ifndef CES16_BCM_VERSION

    AgNdRegPtpTimestampGeneratorsConfiguration  x_tsg_cfg;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_enable=%lu\n", p_msg->b_enable);

    ag_nd_reg_read(
        p_device, 
        AG_REG_PTP_TIMESTAMP_GENERATORS_CONFIGURATION, 
        &(x_tsg_cfg.n_reg));

    x_tsg_cfg.x_fields.b_timestamp_generator_enable = (field_t) p_msg->b_enable;

    ag_nd_reg_write(
        p_device, 
        AG_REG_PTP_TIMESTAMP_GENERATORS_CONFIGURATION, 
        x_tsg_cfg.n_reg);
    #endif

    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_config_ptp_clk_source */
/*  */
AgResult 
ag_nd_opcode_read_config_ptp_clk_source(
    AgNdDevice *p_device, 
    AgNdMsgConfigPtpClkSrc *p_msg)
{
   #ifndef CES16_BCM_VERSION

    AgNdRegPtpTimestampClkSourceSelection x_clk_sel;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_ptp_idx=%hhu\n", p_msg->n_ptp_idx);


#ifdef AG_ND_ENABLE_VALIDATION
    if (p_msg->n_ptp_idx >= AG_ND_PTP_IDX_MAX)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);    
#endif


    ag_nd_reg_read(
        p_device, 
        AG_REG_PTP_TIMESTAMP_CLOCK_SOURCE_SELECTION(p_msg->n_ptp_idx), 
        &(x_clk_sel.n_reg));

    p_msg->n_port = x_clk_sel.x_fields.n_port_select;
    p_msg->e_clk_src = x_clk_sel.x_fields.n_reference_clock_source_select;
 
  #endif
    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_config_ptp_clk_source */
/*  */
AgResult 
ag_nd_opcode_write_config_ptp_clk_source(
    AgNdDevice *p_device, 
    AgNdMsgConfigPtpClkSrc *p_msg)
{
  #ifndef CES16_BCM_VERSION

    AgNdRegPtpTimestampClkSourceSelection x_clk_sel;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_ptp_idx=%hhu\n", p_msg->n_ptp_idx);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "e_clk_src=%d\n", p_msg->e_clk_src);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_port=%hhu\n", p_msg->n_port);


#ifdef AG_ND_ENABLE_VALIDATION
    if (p_msg->n_ptp_idx >= AG_ND_PTP_IDX_MAX)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);    

    if (p_msg->n_port >= p_device->n_total_ports)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);    

    if (p_msg->e_clk_src < AG_ND_PTP_CLK_EXT_1 || p_msg->e_clk_src > AG_ND_PTP_CLK_BRG_2)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);    
#endif


    x_clk_sel.n_reg = 0;
    x_clk_sel.x_fields.n_port_select = (field_t) p_msg->n_port;
    x_clk_sel.x_fields.n_reference_clock_source_select = (field_t) p_msg->e_clk_src;

    ag_nd_reg_write(
        p_device, 
        AG_REG_PTP_TIMESTAMP_CLOCK_SOURCE_SELECTION(p_msg->n_ptp_idx), 
        x_clk_sel.n_reg);

#endif
    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_config_ptp_brg */
/*  */
AgResult 
ag_nd_opcode_read_config_ptp_brg(
    AgNdDevice *p_device, 
    AgNdMsgConfigPtpBrg *p_msg)
{

    AG_U16 n_hi;
    AG_U16 n_lo;

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_ptp_idx=%hhu\n", p_msg->n_ptp_idx);


#ifdef AG_ND_ENABLE_VALIDATION
    if (p_msg->n_ptp_idx >= AG_ND_PTP_IDX_MAX)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);    
#endif


    ag_nd_reg_read(p_device, AG_REG_PTP_BAUD_RATE_GENERATOR(p_msg->n_ptp_idx, 0), &(n_hi));
    ag_nd_reg_read(p_device, AG_REG_PTP_BAUD_RATE_GENERATOR(p_msg->n_ptp_idx, 1), &(n_lo));

    p_msg->n_step_size = AG_ND_MAKE16LH(n_lo, n_hi);

    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_config_ptp_brg */
/*  */
AgResult 
ag_nd_opcode_write_config_ptp_brg(
    AgNdDevice *p_device, 
    AgNdMsgConfigPtpBrg *p_msg)
{

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_ptp_idx=%hhu\n", p_msg->n_ptp_idx);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_step_size=%lu\n", p_msg->n_step_size);


#ifdef AG_ND_ENABLE_VALIDATION
    if (p_msg->n_ptp_idx >= AG_ND_PTP_IDX_MAX)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);    
#endif


    ag_nd_reg_write(
        p_device, 
        AG_REG_PTP_BAUD_RATE_GENERATOR(p_msg->n_ptp_idx, 0), 
        AG_ND_HIGH16(p_msg->n_step_size));

    ag_nd_reg_write(
        p_device, 
        AG_REG_PTP_BAUD_RATE_GENERATOR(p_msg->n_ptp_idx, 1), 
        AG_ND_LOW16(p_msg->n_step_size));

    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_config_ptp_channel_ingress */
/*  */
AgResult 
ag_nd_opcode_write_config_ptp_channel_ingress(
    AgNdDevice *p_device, 
    AgNdMsgConfigPtpCHannelIngress *p_msg)
{
 #ifndef CES16_BCM_VERSION

    AgNdRegPtpConfiguration                 x_ptp_cfg;
	AgNdRegPtpOutPacketConfiguration		x_ptp_out_packet_cfg;
    AgNdRegPtpConfiguration                 x_orig_ptp_cfg;
    AgNdRegPtpTrasmitAccessControl          x_tx_hdr_ac;
    AgNdRegTransmitHeaderFormatDescriptor   x_tx_hdr_fmt;
    AgNdRegPbfPtpConfiguration              x_ptp_pbf;
    AG_U32  i;

    AG_BOOL     b_short_msg_template;
    AgNdBitwise x_bw;

    AG_U32      n_hdr_block_size;
    AG_U32      n_hdr_template_size;
    AG_U32      n_hdr_offset;

    AG_U32      n_ip_chksum;
    AG_U16      n_ip_len;
    AG_U32      n_udp_chksum;
    AG_U16      n_udp_len;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_step_size=%hu\n", p_msg->n_channel_id);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_ptp_idx=%hhu\n", p_msg->n_ptp_idx);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_tx_sync_rate=%d\n", p_msg->n_rx_response_rate);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_tx_sync_rate=%d\n", p_msg->n_tx_sync_rate);


#ifdef AG_ND_ENABLE_VALIDATION
    /* */
    /* sanity checks */
    /*  */

    if (p_msg->n_channel_id >= p_device->n_pw_max)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if (p_msg->n_ptp_idx >= AG_ND_PTP_IDX_MAX)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);    

    if (p_msg->n_tx_sync_rate < -7 ||
        p_msg->n_tx_sync_rate > 0)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if (p_msg->n_rx_response_rate < -7 ||
        p_msg->n_rx_response_rate > 0)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if ((AG_ND_ENCAPSULATION_PTP_IP  != p_msg->x_header.e_encapsulation) &&
        (AG_ND_ENCAPSULATION_PTP_EHT != p_msg->x_header.e_encapsulation) )
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif

#if 0
    /* */
    /* fail if channel is enabled */
    /*  */
    ag_nd_reg_read(p_device, AG_REG_PTP_CONFIGURATION(p_msg->n_channel_id), &(x_ptp_cfg.n_reg));

    if (x_ptp_cfg.x_fields.b_auto_tx_delay_request_enable ||
        x_ptp_cfg.x_fields.b_auto_tx_delay_response_enable ||
        x_ptp_cfg.x_fields.b_auto_tx_pdelay_request_enable ||
        x_ptp_cfg.x_fields.b_auto_tx_pdelay_response_enable ||
        x_ptp_cfg.x_fields.b_auto_tx_sync_enable)
        return AG_ND_ERR(p_device, AG_E_ND_ENABLE);
#endif

    ag_nd_reg_read(p_device, AG_REG_PTP_CONFIGURATION(p_msg->n_channel_id), &(x_orig_ptp_cfg.n_reg));

    /* */
    /* first disable all tx on the channel */
    /* and configure tx rate and clock */
    /*  */
    x_ptp_cfg.n_reg = 0;
    x_ptp_cfg.x_fields.n_tx_sync_rate = (field_t) (-p_msg->n_tx_sync_rate);
    x_ptp_cfg.x_fields.n_rx_response_rate = (field_t) (-p_msg->n_rx_response_rate);
    x_ptp_cfg.x_fields.n_clk_select = (field_t) (p_msg->n_ptp_idx);
    x_ptp_cfg.x_fields.b_count_in_delay_req = x_orig_ptp_cfg.x_fields.b_count_in_delay_req;
    ag_nd_reg_write(p_device, AG_REG_PTP_CONFIGURATION(p_msg->n_channel_id), x_ptp_cfg.n_reg);

	x_ptp_out_packet_cfg.n_reg = 0;
    x_ptp_out_packet_cfg.x_fields.b_count_out_delay_req = (field_t) (p_msg->b_count_out_delay_req);
    ag_nd_reg_write(p_device, AG_REG_PTP_OUTGOING_COUNTERS_CONFIGURATION(p_msg->n_channel_id), x_ptp_out_packet_cfg.n_reg);
     
    /* */
    /* read PTP header block size */
    /* */
   #ifndef CES16_BCM_VERSION
    ag_nd_reg_read(p_device, AG_REG_PTP_TRANSMIT_HEADER_ACCESS_CONTROL, &(x_tx_hdr_ac.n_reg));
   #endif 
    n_hdr_block_size = 1 << (x_tx_hdr_ac.x_fields.n_header_memory_block_size + 5);

    /* */
    /* build header templates: fisrt the shorter one, then the longer */
    /*  */

    x_ptp_pbf.n_reg = 0;
    
    for ( i = 0, b_short_msg_template = AG_TRUE; 
          i < 2; 
          i++, b_short_msg_template = AG_FALSE)
    {
        /* */
        /* build header template */
        /*  */
        x_bw.p_buf = p_device->a_buf;
        x_bw.p_addr = p_device->a_buf;
        x_bw.n_buf_size = sizeof(p_device->a_buf);
        x_bw.n_offset = 0;

        p_msg->x_header.b_udp_chksum = AG_TRUE;
        p_msg->x_header.x_ptp.b_dst_port = !b_short_msg_template;

        ag_nd_header_build(
            &(p_msg->x_header),
            &x_bw,
            &x_tx_hdr_fmt,
            &n_udp_chksum,
            &n_udp_len,
            &n_ip_chksum,
            &n_ip_len);

        /* */
        /* check if the total header size (descriptor and header template) doesn't */
        /* exceeds the currently configured header block size */
        /* */
        n_hdr_template_size = x_bw.p_addr - x_bw.p_buf;
        assert(sizeof(AgNdRegTransmitHeaderFormatDescriptor) + n_hdr_template_size <= n_hdr_block_size);


        /* */
        /* calculate header offset from memory unit start */
        /*  */
        n_hdr_offset = p_msg->n_channel_id * 2;

        if (b_short_msg_template)
        {
            x_ptp_pbf.x_fields.n_header_0_memory_size = n_hdr_template_size + sizeof(AgNdRegTransmitHeaderFormatDescriptor);
        }
        else
        {
            x_ptp_pbf.x_fields.n_header_1_memory_size = n_hdr_template_size + sizeof(AgNdRegTransmitHeaderFormatDescriptor);
            n_hdr_offset++;
        }

        n_hdr_offset *= n_hdr_block_size;

        /* */
        /* store descriptor */
        /*  */
        ag_nd_mem_write(
            p_device, 
            p_device->p_mem_ptp_header, 
            n_hdr_offset, 
            &(x_tx_hdr_fmt.n_reg),
            2);

        /* */
        /* store header template  */
        /* */
        ag_nd_mem_write(
            p_device,
            p_device->p_mem_ptp_header,
            n_hdr_offset + 4, 
            x_bw.p_buf,
            ag_nd_round_up2(n_hdr_template_size, 1) >> 1);

        if (p_msg->x_header.e_encapsulation != AG_ND_ENCAPSULATION_PTP_EHT) {
            /* */
            /* update header template with correct ip/udp checksum and length field values */
            /* must be called after the descriptor is written */
            /*   */
            ag_nd_header_update_ptp(
                p_device,
                p_device->p_mem_ptp_header,
                n_hdr_offset,
                n_udp_chksum, 
                n_udp_len,    
                n_ip_chksum,  
                n_ip_len);
        } /* if */

    } /* for */

    /* */
    /* store header size */
    /*  */
    ag_nd_reg_write(p_device, AG_REG_PBF_PTP_CONFIGURATION_REGISTER(p_msg->n_channel_id), x_ptp_pbf.n_reg);

	/* now restore tx flags */
    x_ptp_cfg.x_fields.b_auto_tx_delay_request_enable = x_orig_ptp_cfg.x_fields.b_auto_tx_delay_request_enable;
    x_ptp_cfg.x_fields.b_auto_tx_delay_response_enable = x_orig_ptp_cfg.x_fields.b_auto_tx_delay_response_enable;
    x_ptp_cfg.x_fields.b_auto_tx_pdelay_request_enable = x_orig_ptp_cfg.x_fields.b_auto_tx_pdelay_request_enable;
    x_ptp_cfg.x_fields.b_auto_tx_pdelay_response_enable = x_orig_ptp_cfg.x_fields.b_auto_tx_pdelay_response_enable;
    x_ptp_cfg.x_fields.b_auto_tx_sync_enable = x_orig_ptp_cfg.x_fields.b_auto_tx_sync_enable;
    x_ptp_cfg.x_fields.b_random_tx_enable = x_orig_ptp_cfg.x_fields.b_random_tx_enable;
	ag_nd_reg_write(p_device, AG_REG_PTP_CONFIGURATION(p_msg->n_channel_id), x_ptp_cfg.n_reg);
#endif /*CES16_BCM_VERSION*/

    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_config_ptp_channel_egress */
/*  */

int ptp_strict = 1;
AgResult 
ag_nd_opcode_write_config_ptp_channel_egress(
    AgNdDevice *p_device, 
    AgNdMsgConfigPtpCHannelEgress *p_msg)
{
  #ifndef CES16_BCM_VERSION

    AgNdRegReceivePtpChannelConfiguration x_rx_cfg;
    AgNdRegPtpConfiguration                 x_orig_ptp_cfg;

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_channel_id=%hu\n", p_msg->n_channel_id);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_ptp_idx=%hhu\n", p_msg->n_ptp_idx);


#ifdef AG_ND_ENABLE_VALIDATION
    /* */
    /* sanity checks */
    /*  */

    if (p_msg->n_channel_id >= p_device->n_pw_max)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if (p_msg->n_ptp_idx >= AG_ND_PTP_IDX_MAX)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);    
#endif


    ag_nd_reg_read(
        p_device, 
        AG_REG_RECEIVE_PTP_CHANNEL_CONFIGURATION(p_msg->n_channel_id), 
        &(x_rx_cfg.n_reg));

    /* */
    /* configure  */
    /*  */
    x_rx_cfg.x_fields.n_clk_select = (field_t) p_msg->n_ptp_idx;
    ag_nd_reg_write(
        p_device, 
        AG_REG_RECEIVE_PTP_CHANNEL_CONFIGURATION(p_msg->n_channel_id), 
        x_rx_cfg.n_reg);


    ag_nd_reg_read(p_device, AG_REG_PTP_CONFIGURATION(p_msg->n_channel_id), &(x_orig_ptp_cfg.n_reg));
    x_orig_ptp_cfg.x_fields.b_count_in_delay_req = (field_t) (p_msg->b_count_in_delay_req);
    ag_nd_reg_write(p_device, AG_REG_PTP_CONFIGURATION(p_msg->n_channel_id), x_orig_ptp_cfg.n_reg);

    /* */
    /* configure strict buffer data */
    /* */
    if (ptp_strict)
	    ag_nd_strict_build(p_device, &(p_msg->x_strict_data), p_msg->n_channel_id);

#endif
    return AG_S_OK;
}    


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_config_ptp_channel_enable_ingress */
/*  */
AgResult 
ag_nd_opcode_read_config_ptp_channel_enable_ingress(
    AgNdDevice *p_device, 
    AgNdMsgConfigPtpChannelEnableIngress *p_msg)
{
 #ifndef CES16_BCM_VERSION

    AgNdRegPtpConfiguration x_cfg;

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);

#ifdef AG_ND_ENABLE_VALIDATION
    if (p_msg->n_channel_id >= p_device->n_pw_max)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif


    ag_nd_reg_read(p_device, AG_REG_PTP_CONFIGURATION(p_msg->n_channel_id), &(x_cfg.n_reg));

    p_msg->b_delay_request_enable   = x_cfg.x_fields.b_auto_tx_delay_request_enable;
    p_msg->b_delay_response_enable  = x_cfg.x_fields.b_auto_tx_delay_response_enable;
    p_msg->b_pdelay_request_enable  = x_cfg.x_fields.b_auto_tx_pdelay_request_enable;
    p_msg->b_pdelay_response_enable = x_cfg.x_fields.b_auto_tx_pdelay_response_enable;
    p_msg->b_sync_enable            = x_cfg.x_fields.b_auto_tx_sync_enable;
    p_msg->b_random_tx_enable       = x_cfg.x_fields.b_random_tx_enable;
#endif
    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_config_ptp_channel_enable_ingress */
/*  */
AgResult 
ag_nd_opcode_write_config_ptp_channel_enable_ingress(
    AgNdDevice *p_device, 
    AgNdMsgConfigPtpChannelEnableIngress *p_msg)
{
 #ifndef CES16_BCM_VERSION

    AgNdRegPtpConfiguration x_cfg;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_channel_id=%hu\n", p_msg->n_channel_id);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_delay_request_enable=%lu\n", p_msg->b_delay_request_enable);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_delay_response_enable=%lu\n", p_msg->b_delay_response_enable);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_pdelay_request_enable=%lu\n", p_msg->b_pdelay_request_enable);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_pdelay_response_enable=%lu\n", p_msg->b_pdelay_response_enable);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_sync_enable=%lu\n", p_msg->b_sync_enable);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_random_tx_enable=%lu\n", p_msg->b_random_tx_enable);


#ifdef AG_ND_ENABLE_VALIDATION
    if (p_msg->n_channel_id >= p_device->n_pw_max)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif


    ag_nd_reg_read(p_device, AG_REG_PTP_CONFIGURATION(p_msg->n_channel_id), &(x_cfg.n_reg));

    x_cfg.x_fields.b_auto_tx_delay_request_enable = (field_t) p_msg->b_delay_request_enable;
    x_cfg.x_fields.b_auto_tx_delay_response_enable = (field_t) p_msg->b_delay_response_enable;
    x_cfg.x_fields.b_auto_tx_pdelay_request_enable = (field_t) p_msg->b_pdelay_request_enable;
    x_cfg.x_fields.b_auto_tx_pdelay_response_enable = (field_t) p_msg->b_pdelay_response_enable;
    x_cfg.x_fields.b_auto_tx_sync_enable = (field_t) p_msg->b_sync_enable;
    x_cfg.x_fields.b_random_tx_enable = (field_t) p_msg->b_random_tx_enable;

    ag_nd_reg_write(p_device, AG_REG_PTP_CONFIGURATION(p_msg->n_channel_id), x_cfg.n_reg);

 #endif
    return AG_S_OK;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_config_ptp_channel_enable_egress */
/*  */
AgResult 
ag_nd_opcode_read_config_ptp_channel_enable_egress(
    AgNdDevice *p_device, 
    AgNdMsgConfigPtpChannelEnableEgress *p_msg)
{
 #ifndef CES16_BCM_VERSION

    AgNdRegReceivePtpChannelConfiguration x_rx_cfg;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);


#ifdef AG_ND_ENABLE_VALIDATION
    if (p_msg->n_channel_id >= p_device->n_pw_max)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif


    ag_nd_reg_read(
        p_device, 
        AG_REG_RECEIVE_PTP_CHANNEL_CONFIGURATION(p_msg->n_channel_id), 
        &(x_rx_cfg.n_reg));


    p_msg->b_enable = x_rx_cfg.x_fields.b_channel_enable;
 
 #endif
    return AG_S_OK;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_config_ptp_channel_enable_egress */
/*  */
AgResult 
ag_nd_opcode_write_config_ptp_channel_enable_egress(
    AgNdDevice *p_device, 
    AgNdMsgConfigPtpChannelEnableEgress *p_msg)
{
  #ifndef CES16_BCM_VERSION

    AgNdRegReceivePtpChannelConfiguration x_rx_cfg;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_channel_id=%hu\n", p_msg->n_channel_id);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_enable=%lu\n", p_msg->b_enable);


#ifdef AG_ND_ENABLE_VALIDATION
    if (p_msg->n_channel_id >= p_device->n_pw_max)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif


    ag_nd_reg_read(
        p_device, 
        AG_REG_RECEIVE_PTP_CHANNEL_CONFIGURATION(p_msg->n_channel_id), 
        &(x_rx_cfg.n_reg));

    x_rx_cfg.x_fields.b_channel_enable = p_msg->b_enable;

    ag_nd_reg_write(
        p_device, 
        AG_REG_RECEIVE_PTP_CHANNEL_CONFIGURATION(p_msg->n_channel_id), 
        x_rx_cfg.n_reg);

 #endif
    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_command_ptp_correct_timestamp */
/*  */
AgResult 
ag_nd_opcode_write_command_ptp_correct_timestamp(
    AgNdDevice *p_device, 
    AgNdMsgCommandPtpCorrectTs *p_msg)
{
 #ifndef CES16_BCM_VERSION

    AG_U32 i;
    AG_U16 n_reg;

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_ptp_idx=%hhu\n", p_msg->n_ptp_idx);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "a_correction_value:\n");
    for (i = 0; i < sizeof(p_msg->a_correction_value); i++)
        AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "0x%02hhx\n", p_msg->a_correction_value[i]);


#ifdef AG_ND_ENABLE_VALIDATION
    if (p_msg->n_ptp_idx >= AG_ND_PTP_IDX_MAX)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);    
#endif


    for (i = 0; i < sizeof(p_msg->a_correction_value)/2; i++)
    {
        n_reg = AG_ND_MAKE16LH(p_msg->a_correction_value[i*2+1], p_msg->a_correction_value[i*2]);

        ag_nd_reg_write(
            p_device, 
            AG_REG_PTP_TIMESTAMP_CORRECTION_VALUE(p_msg->n_ptp_idx, i),
            n_reg);
    }

 #endif
    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_command_write_ptp_adjustment */
/*  */
AgResult 
ag_nd_opcode_write_command_ptp_adjustment(
    AgNdDevice *p_device, 
    AgNdMsgCommandPtpAdjustment *p_msg)
{
  #ifndef CES16_BCM_VERSION

    AgNdRegPtpBaudRateGeneratorCorrection x_brg_correct;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_ptp_idx=%hhu\n", p_msg->n_ptp_idx);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "e_polarity=%d\n", p_msg->e_polarity);


#ifdef AG_ND_ENABLE_VALIDATION
    if (p_msg->n_ptp_idx >= AG_ND_PTP_IDX_MAX)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);    
#endif


    x_brg_correct.n_reg = 0;

    switch(p_msg->e_polarity)
    {
    case AG_ND_ADJ_POLARITY_NEG:
        x_brg_correct.x_fields.n_negative_adjustment = 1;
        break;

    case AG_ND_ADJ_POLARITY_POS:
        x_brg_correct.x_fields.n_positive_adjustment = 1;

    default:
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
    }

    ag_nd_reg_write(
        p_device, 
        AG_REG_PTP_BAUD_RATE_GENERATOR_CORRECTION(p_msg->n_ptp_idx), 
        x_brg_correct.n_reg);

 #endif
    return AG_S_OK;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_ptp_counters */
/*  */
AgResult 
ag_nd_opcode_read_ptp_counters(
    AgNdDevice *p_device, 
    AgNdMsgPtpCounters *p_msg)
{
#ifndef CES16_BCM_VERSION

	AG_U16 n_out_packets_1_2;
	AG_U16 n_in_packets_1_2;
	AG_U16 n_in_packets_3;
	

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_channel_id=%hhu\n", p_msg->n_channel_id);


#ifdef AG_ND_ENABLE_VALIDATION
    if (p_msg->n_channel_id >= p_device->n_pw_max)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif


    ag_nd_reg_read(
        p_device, 
        AG_REG_PTP_OUT_PACKETS_1_2(p_msg->n_channel_id), 
        &n_out_packets_1_2);

    ag_nd_reg_read(
        p_device, 
        AG_REG_PTP_IN_PACKETS_1_2(p_msg->n_channel_id), 
        &n_in_packets_1_2);

    ag_nd_reg_read(
        p_device, 
        AG_REG_PTP_IN_PACKETS_3(p_msg->n_channel_id), 
        &n_in_packets_3);


	p_msg->n_out_sync_plus_followup = n_out_packets_1_2 & 0xFF;
	p_msg->n_out_delay_req_or_resp = (n_out_packets_1_2 >> 8) & 0xFF;

	p_msg->n_in_sync = n_in_packets_1_2 & 0xFF;
	p_msg->n_in_followup = (n_in_packets_1_2 >> 8) & 0xFF;
	p_msg->n_in_delay_req_or_resp = n_in_packets_3 & 0xFF;
	
#endif
    return AG_S_OK;
}



