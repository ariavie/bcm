/* $Id: nd_channel_egress.c,v 1.8 Broadcom SDK $
 * Copyright 2011 BATM
 */

#include "nd_channel_egress.h"
#include "nd_registers.h"
#include "nd_debug.h"
#include "nd_protocols.h"
#include "nd_rpc.h"
#include "nd_util.h"

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_config_jitter_buffer_params */
/*  */
AgResult
ag_nd_opcode_read_config_jitter_buffer_params(
    AgNdDevice *p_device, 
    AgNdMsgConfigJitterBufferParams *p_msg)
{
    AgNdRegJitterBufferChannelConfiguration1         x_jbf_cfg1;

    ag_nd_reg_read(p_device, 
                   AG_REG_JITTER_BUFFER_CHANNEL_CONFIGURATION_1(p_msg->n_channel_id), 
                   &x_jbf_cfg1.n_reg);
    /* */
    /* retrieve JBF settings */
	/* */
   #ifndef CES16_BCM_VERSION
	if (p_device->b_dynamic_memory)
	{
	    p_msg->n_jbf_ring_size = 1 << (3 + p_device->a_channel[p_msg->n_channel_id].n_jbf_ring_size);
	}
	else
	{
	    p_msg->n_jbf_ring_size = 1 << (3 + x_jbf_cfg1.x_fields.n_jitter_buffer_ring_size);
	}
   #else
       p_msg->n_jbf_ring_size = 128; /*Field remove from register*/
   #endif
    p_msg->n_jbf_win_size = 1 << (3 + x_jbf_cfg1.x_fields.n_window_size_limit);

    p_msg->n_jbf_bop = x_jbf_cfg1.x_fields.n_jitter_buffer_break_out_point;

	return AG_S_OK;

}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_config_jitter_buffer_params */
/*  */
AgResult
ag_nd_opcode_write_config_jitter_buffer_params(
    AgNdDevice *p_device, 
    AgNdMsgConfigJitterBufferParams *p_msg)
{
    AgNdRegJitterBufferChannelConfiguration1         x_jbf_cfg1;

#ifdef AG_ND_ENABLE_VALIDATION
		/* */
		/* sanity */
		/*  */
		if (p_msg->n_channel_id >= p_device->n_pw_max)
			return AG_ND_ERR(p_device, AG_E_ND_ARG);
	   
       #ifndef CES16_BCM_VERSION	
		if (p_msg->n_jbf_ring_size < 8 ||
			p_msg->n_jbf_ring_size > 1024 ||
			!ag_nd_is_power_of_2(p_msg->n_jbf_ring_size))
			return AG_ND_ERR(p_device, AG_E_ND_ARG);
	
		if (AG_ND_JBF_NO_LIMIT == p_msg->n_jbf_win_size )
		{
			if (p_msg->n_jbf_bop > p_msg->n_jbf_ring_size)
				return AG_ND_ERR(p_device, AG_E_ND_ARG);
		}
		else
		{
			if (p_msg->n_jbf_win_size < 8 ||
				p_msg->n_jbf_win_size > 512 ||
				!ag_nd_is_power_of_2(p_msg->n_jbf_win_size))
				return AG_ND_ERR(p_device, AG_E_ND_ARG);
			if (p_msg->n_jbf_bop + p_msg->n_jbf_win_size > p_msg->n_jbf_ring_size)			
				return AG_ND_ERR(p_device, AG_E_ND_ARG);
		}
	  #endif
	
		if (0 == p_msg->n_jbf_bop)
			return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif
	
	
	/* */
	/* set the registers to zero to be on the safe size */
	/*  */
	x_jbf_cfg1.n_reg = 0;

	/* */
	/* JBF parameters */
	/* */
	p_device->a_channel[p_msg->n_channel_id].n_jbf_ring_size = (field_t) (ag_nd_log2(p_msg->n_jbf_ring_size) - 3);

	x_jbf_cfg1.x_fields.n_jitter_buffer_ring_size = (field_t) (ag_nd_log2(p_msg->n_jbf_ring_size) - 3);
	x_jbf_cfg1.x_fields.n_jitter_buffer_break_out_point = (field_t) p_msg->n_jbf_bop;

	if (AG_ND_JBF_NO_LIMIT == p_msg->n_jbf_win_size)
		x_jbf_cfg1.x_fields.n_window_size_limit = 7;
	else
		x_jbf_cfg1.x_fields.n_window_size_limit = (field_t) (ag_nd_log2(p_msg->n_jbf_win_size) - 3);

	/* configure hw */
	/* */
	ag_nd_reg_write(p_device, 
					AG_REG_JITTER_BUFFER_CHANNEL_CONFIGURATION_1(p_msg->n_channel_id), 
					x_jbf_cfg1.n_reg);

	return AG_S_OK;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_config_channel_egress */
/*  */
AgResult
ag_nd_opcode_read_config_channel_egress(
    AgNdDevice *p_device, 
    AgNdMsgConfigChannelEgress *p_msg)
{
    AgNdRegJitterBufferChannelConfiguration1         x_jbf_cfg1;
    AgNdRegJitterBufferChannelConfiguration2         x_jbf_cfg2;
    AgNdRegReceiveChannelConfiguration               x_rcp_cfg;
    AgNdRegChannelControl                            x_che_ctl;
    AgNdRegJitterBufferQueueBaseAddress              x_jbf_addr;
    AgNdRegJitterBufferValidBitArrayConfiguration    x_jbf_vba;

    x_jbf_addr.n_reg = 0;

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);


#ifdef AG_ND_ENABLE_VALIDATION
    /* */
    /* sanity */
    /* */
    if (p_msg->n_channel_id >= p_device->n_pw_max)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif


    /* */
    /* read data from hw */
    /* */
    ag_nd_reg_read(p_device, 
                   AG_REG_EGRESS_CHANNEL_CONTROL(p_msg->n_channel_id), 
                   &x_che_ctl.n_reg);

    ag_nd_reg_read(p_device, 
                   AG_REG_JITTER_BUFFER_CHANNEL_CONFIGURATION_1(p_msg->n_channel_id), 
                   &x_jbf_cfg1.n_reg);

    ag_nd_reg_read(p_device, 
                   AG_REG_JITTER_BUFFER_CHANNEL_CONFIGURATION_2(p_msg->n_channel_id), 
                   &x_jbf_cfg2.n_reg);

    ag_nd_reg_read(p_device, 
                   AG_REG_RECEIVE_CHANNEL_CONFIGURATION(p_msg->n_channel_id), 
                   &x_rcp_cfg.n_reg);

   #ifndef CES16_BCM_VERSION
    ag_nd_reg_read(p_device, 
                   AG_REG_JITTER_BUFFER_QUEUE_BASE_ADDRESS(p_msg->n_channel_id), 
                   &x_jbf_addr.n_reg);
	
    if (!p_device->b_dynamic_memory)
	{
	    ag_nd_reg_read(p_device, 
                   AG_REG_JITTER_BUFFER_VALID_BIT_ARRAY_CONFIGURATION(p_msg->n_channel_id), 
                   &x_jbf_vba.n_reg);
	}
	else
   #endif

	{
		x_jbf_vba.n_reg = 0;
	}

    /* */
    /* check that channel enable is consistent between hw and shadow structures */
    /* */
    assert(x_che_ctl.x_fields.b_channel_enable == x_rcp_cfg.x_fields.b_channel_enable);
    assert(x_che_ctl.x_fields.b_channel_enable == p_device->a_channel[p_msg->n_channel_id].a_enable[AG_ND_PATH_EGRESS]);


    /* */
    /* retrieve JBF settings */
    /* */
	if  (p_device->b_dynamic_memory)
	{
    	p_msg->n_jbf_ring_size = 1 << (3 + p_device->a_channel[p_msg->n_channel_id].n_jbf_ring_size);
	}
	else
	{
    	p_msg->n_jbf_ring_size = 1 << (3 + x_jbf_cfg1.x_fields.n_jitter_buffer_ring_size);
	}

    p_msg->n_jbf_win_size = 1 << (3 + x_jbf_cfg1.x_fields.n_window_size_limit);

    p_msg->n_jbf_bop = x_jbf_cfg1.x_fields.n_jitter_buffer_break_out_point;

    p_msg->n_jbf_addr = 
        ((AG_U32)x_jbf_addr.x_fields.n_base_address << 10) +
        p_device->n_base +
        0x800000;

    p_msg->n_packet_sync_selector = x_jbf_cfg2.x_fields.n_packet_sync_thresholds_pointer;

    p_msg->n_payload_size = x_jbf_cfg2.x_fields.n_packet_payload_size;

    p_msg->n_vba_addr = ((AG_U32)x_jbf_vba.x_fields.n_valid_bit_array_base_address) << 3;


    /* */
    /* retrieve channel status */
    /* */
    p_msg->b_enable = x_rcp_cfg.x_fields.b_channel_enable;


    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_config_channel_egress */
/* */
AgResult
ag_nd_opcode_write_config_channel_egress(
    AgNdDevice *p_device, 
    AgNdMsgConfigChannelEgress *p_msg)
{
    AgNdRegJitterBufferChannelConfiguration1         x_jbf_cfg1;
    AgNdRegJitterBufferChannelConfiguration2         x_jbf_cfg2;
    AgNdRegReceiveChannelConfiguration               x_rcp_cfg;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_channel_id=%hu\n", p_msg->n_channel_id);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_rtp_exists=%lu\n", p_msg->b_rtp_exists);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_jbf_bop=%lu\n", p_msg->n_jbf_bop);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_jbf_ring_size=%lu\n", p_msg->n_jbf_ring_size);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_jbf_win_size=%lu\n", p_msg->n_jbf_win_size);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "e_encapsulation=%d\n", p_msg->x_strict_data.e_encapsulation);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_packet_sync_selector=%lu\n", p_msg->n_packet_sync_selector);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_payload_size=%lu\n", p_msg->n_payload_size);


#ifdef AG_ND_ENABLE_VALIDATION
    /* */
    /* sanity */
    /*  */
    if (p_msg->n_channel_id >= p_device->n_pw_max)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if (ag_nd_channel_egress_is_enabled(p_device, p_msg->n_channel_id))
        return AG_ND_ERR(p_device, AG_E_ND_ENABLE);

    if (p_msg->n_jbf_ring_size < 8 ||
        p_msg->n_jbf_ring_size > 1024 ||
        !ag_nd_is_power_of_2(p_msg->n_jbf_ring_size))
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if (AG_ND_JBF_NO_LIMIT == p_msg->n_jbf_win_size )
    {
        if (p_msg->n_jbf_bop > p_msg->n_jbf_ring_size)
            return AG_ND_ERR(p_device, AG_E_ND_ARG);
    }
    else
    {
        if (p_msg->n_jbf_win_size < 8 ||
            p_msg->n_jbf_win_size > 512 ||
            !ag_nd_is_power_of_2(p_msg->n_jbf_win_size))
            return AG_ND_ERR(p_device, AG_E_ND_ARG);

        if (p_msg->n_jbf_bop + p_msg->n_jbf_win_size > p_msg->n_jbf_ring_size)          
            return AG_ND_ERR(p_device, AG_E_ND_ARG);
    }


    if (0 == p_msg->n_jbf_bop)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif


    /* */
    /* set the registers to zero to be on the safe size */
    /*  */
    x_jbf_cfg1.n_reg = 0;
    x_jbf_cfg2.n_reg = 0;
    x_rcp_cfg.n_reg = 0;


    /* */
    /* JBF parameters */
    /* */
	p_device->a_channel[p_msg->n_channel_id].n_jbf_ring_size = (field_t) (ag_nd_log2(p_msg->n_jbf_ring_size) - 3);

   	x_jbf_cfg1.x_fields.n_jitter_buffer_ring_size = (field_t) (ag_nd_log2(p_msg->n_jbf_ring_size) - 3);
    x_jbf_cfg1.x_fields.n_jitter_buffer_break_out_point = (field_t) p_msg->n_jbf_bop;

    if (AG_ND_JBF_NO_LIMIT == p_msg->n_jbf_win_size)
        x_jbf_cfg1.x_fields.n_window_size_limit = 7;
    else
        x_jbf_cfg1.x_fields.n_window_size_limit = (field_t) (ag_nd_log2(p_msg->n_jbf_win_size) - 3);


    /* */
    /*  payload size */
    /* */
    if (p_msg->n_payload_size > 1500)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    x_jbf_cfg2.x_fields.n_packet_payload_size = (field_t) p_msg->n_payload_size;


    /* */
    /* packet sync thresholds */
    /* */
    if (p_msg->n_packet_sync_selector >= AG_ND_SYNC_THRESHOLD_TABLE_SIZE)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    x_jbf_cfg2.x_fields.n_packet_sync_thresholds_pointer = (field_t) p_msg->n_packet_sync_selector;


    /* */
    /* interrupts */
    /* */
    x_jbf_cfg2.x_fields.b_packet_sync_interrupt_enable = (field_t) (NULL != p_device->p_cb_psi);
    x_rcp_cfg.x_fields.b_control_word_interrupt_enable = (field_t) (NULL != p_device->p_cb_cwi);


	x_jbf_cfg2.x_fields.b_drop_on_valid = (field_t) ((p_msg->b_drop_on_valid) ?  1 : 0);


    /* */
    /* RPC settings */
    /* */
    if (p_msg->b_rtp_exists && AG_ND_ENCAPSULATION_IP != p_msg->x_strict_data.e_encapsulation)
        x_rcp_cfg.x_fields.b_rtp_follows_ces = (field_t) AG_TRUE;
    else
        x_rcp_cfg.x_fields.b_rtp_follows_ces = (field_t) AG_FALSE;


    /* */
    /* configure hw */
    /* */
    ag_nd_reg_write(p_device, 
                    AG_REG_JITTER_BUFFER_CHANNEL_CONFIGURATION_1(p_msg->n_channel_id), 
                    x_jbf_cfg1.n_reg);

    ag_nd_reg_write(p_device, 
                    AG_REG_JITTER_BUFFER_CHANNEL_CONFIGURATION_2(p_msg->n_channel_id), 
                    x_jbf_cfg2.n_reg);

	/*ORI*/
	/*update reg AG_REG_RECEIVE_CHANNEL_CONFIGURATION for l2tpv3 */
	x_rcp_cfg.x_fields.n_l2tpv3_cookie_num = (field_t) 0x0;
	
	if(p_msg->x_strict_data.n_l2tpv3_count > 0)
	{
		/*case we are in l2tpv3 need to update rtp flag(0b00)->ces before rtp */
		if(p_msg->b_rtp_exists)
		{
			x_rcp_cfg.x_fields.b_rtp_follows_ces = (field_t) AG_TRUE;
		}
		
		if(p_msg->x_strict_data.n_l2tpv3_count == 2)
		{
			/*case only one cookie(32) */
			x_rcp_cfg.x_fields.n_l2tpv3_cookie_num = (field_t) 0x1;
		}
		if(p_msg->x_strict_data.n_l2tpv3_count == 3)
		{
			/*case only two cookie(32) */
			x_rcp_cfg.x_fields.n_l2tpv3_cookie_num = (field_t) 0x2;
		}
	}
	
    ag_nd_reg_write(p_device, 
                    AG_REG_RECEIVE_CHANNEL_CONFIGURATION(p_msg->n_channel_id), 
                    x_rcp_cfg.n_reg);

    /* */
    /* configure strict buffer data */
    /* */
    ag_nd_strict_build(p_device, &(p_msg->x_strict_data), p_msg->n_channel_id);


    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_status_egress */
/*  */
AgResult 
ag_nd_opcode_read_status_egress(
    AgNdDevice *p_device, 
    AgNdMsgStatusEgress *p_msg)
{
    AgNdRegPacketSyncStatus                 x_sync_stat;
    AgNdRegJitterBufferChannelStatus        x_jbf_stat;
    AgNdRegJitterBufferSlipCommand          x_slip_cmd;
    AgNdRegJitterBufferSlipConfiguration    x_slip_cfg;
    AG_U16                                  n_cw;

    x_slip_cfg.n_reg = 0;

    AG_ND_TRACE(p_device, AG_ND_TRACE_PERIODIC, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);


#ifdef AG_ND_ENABLE_VALIDATION
    /* */
    /* sanity */
    /* */
    if (p_msg->n_channel_id >= p_device->n_pw_max)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

#endif

    ag_nd_reg_read(p_device, 
                   AG_REG_PACKET_SYNC_STATUS(p_msg->n_channel_id), 
                   &x_sync_stat.n_reg);

    ag_nd_reg_read(p_device, 
                   AG_REG_JITTER_BUFFER_CHANNEL_STATUS(p_msg->n_channel_id), 
                   &x_jbf_stat.n_reg);

    ag_nd_reg_read(p_device, 
                   AG_REG_RECEIVE_CES_CONTROL_WORD(p_msg->n_channel_id), 
                   &n_cw);

    /* */
    
    /* */
    ag_nd_reg_read(p_device, 
                   AG_REG_JITTER_BUFFER_SLIP_COMMAND, 
                   &x_slip_cmd.n_reg);

   #ifndef CES16_BCM_VERSION
    ag_nd_reg_read(p_device, 
                   AG_REG_JITTER_BUFFER_SLIP_CONFIGURATION, 
                   &x_slip_cfg.n_reg);
   #endif /*CES16_BCM_VERSION*/


    if (x_slip_cfg.x_fields.n_slip_channel_select == p_msg->n_channel_id &&
        (x_slip_cmd.x_fields.n_byte_slip_command_status ||
         x_slip_cmd.x_fields.n_packet_slip_command_status))

        p_msg->b_slip = AG_TRUE;
    else
        p_msg->b_slip = AG_FALSE;


    p_msg->b_trimming = x_jbf_stat.x_fields.b_trimming_mode;
    p_msg->e_jbf_state = x_jbf_stat.x_fields.n_jitter_buffer_state;
    p_msg->e_sync_state = x_sync_stat.x_fields.n_packet_sync_state;
    p_msg->n_ces_cw = n_cw;


    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* get egress channel enable status */
/*  */
AG_BOOL
ag_nd_channel_egress_is_enabled(AgNdDevice *p_device, AgNdChannel n_channel_id)
{
    AgNdRegReceiveChannelConfiguration   x_rcp_cfg;
    AgNdRegChannelControl                x_che_ctl;

    ag_nd_reg_read(p_device, AG_REG_RECEIVE_CHANNEL_CONFIGURATION(n_channel_id), &(x_rcp_cfg.n_reg));
    ag_nd_reg_read(p_device, AG_REG_EGRESS_CHANNEL_CONTROL(n_channel_id), &(x_che_ctl.n_reg));

    assert(x_che_ctl.x_fields.b_channel_enable == x_rcp_cfg.x_fields.b_channel_enable);
    assert(x_che_ctl.x_fields.b_channel_enable == p_device->a_channel[n_channel_id].a_enable[AG_ND_PATH_EGRESS]);

    return (AG_BOOL)x_che_ctl.x_fields.b_channel_enable;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* enables egress channel */
/*  */
AgResult
ag_nd_channel_egress_enable(AgNdDevice *p_device, AgNdChannel n_channel_id, AG_U32 n_cookie)
{
    AgNdRegChannelControl                           x_che_ctl;
    AgNdRegReceiveChannelConfiguration              x_rcp_cfg;
   #ifndef CES16_BCM_VERSION
    AgNdRegJitterBufferQueueBaseAddress             x_jbf_addr;
    AgNdRegJitterBufferValidBitArrayConfiguration   x_jbf_vba;
    AG_U32 n_jbf_addr;
    AG_U32 n_vba_addr;
    AG_U32 n_jbf_size;
    AG_U32 n_vba_size;
   #endif 
    AgNdRegJitterBufferChannelConfiguration1        x_jbf_cfg1;
    AgNdRegJitterBufferChannelConfiguration2        x_jbf_cfg2;

    AgResult n_ret = AG_S_OK;

#ifdef AG_ND_ENABLE_VALIDATION
    /* */
    /* check if FTIP is configured */
    /* */
    if (!AG_ND_TS_ISVALID(p_device->a_channel[n_channel_id].a_channelizer[AG_ND_PATH_EGRESS].x_first))
        return AG_ND_ERR(p_device, AG_E_ND_FTIP);

    /* */
    /* check if channel is already enabled */
    /* */
    if (ag_nd_channel_egress_is_enabled(p_device, n_channel_id))
        return AG_ND_ERR(p_device, AG_E_ND_ENABLE);
#endif


    /* */
    /* read the JBF configuration, compute the JBF and VBA size and allocate addresses */
    /*  */
    ag_nd_reg_read(p_device, 
                   AG_REG_JITTER_BUFFER_CHANNEL_CONFIGURATION_1(n_channel_id), 
                   &(x_jbf_cfg1.n_reg));

    ag_nd_reg_read(p_device, 
                   AG_REG_JITTER_BUFFER_CHANNEL_CONFIGURATION_2(n_channel_id), 
                   &(x_jbf_cfg2.n_reg));

    #ifndef CES16_BCM_VERSION
	if (p_device->b_dynamic_memory)
	{
    	n_vba_size = 1 << (p_device->a_channel[n_channel_id].n_jbf_ring_size + 3);  
	}
	else
	{
    	n_vba_size = 1 << (x_jbf_cfg1.x_fields.n_jitter_buffer_ring_size + 3);  
	}
    n_jbf_size = n_vba_size * x_jbf_cfg2.x_fields.n_packet_payload_size;                
    n_ret = ag_nd_allocator_alloc(&(p_device->x_allocator_jbf), n_jbf_size, 0x400, &n_jbf_addr);
    if (AG_FAILED(n_ret))
    {
        return AG_ND_ERR(p_device, AG_E_ND_ALLOC);
    }

	if (!p_device->b_dynamic_memory)
	{
	    n_ret = ag_nd_allocator_alloc(&(p_device->x_allocator_vba), n_vba_size, 8, &n_vba_addr);
	    if (AG_FAILED(n_ret))
	    {
	        ag_nd_allocator_free(&(p_device->x_allocator_jbf), n_jbf_addr);
	        return AG_ND_ERR(p_device, AG_E_ND_ALLOC);
	    }

    /* */
    /* write JBF and VBA bases to HW */
    /* */
	    ag_nd_reg_read(p_device,
	                   AG_REG_JITTER_BUFFER_VALID_BIT_ARRAY_CONFIGURATION(n_channel_id),
	                   &(x_jbf_vba.n_reg));

	    x_jbf_vba.x_fields.n_valid_bit_array_base_address = (field_t) (n_vba_addr >> 3);

	    ag_nd_reg_write(p_device, 
	                    AG_REG_JITTER_BUFFER_VALID_BIT_ARRAY_CONFIGURATION(n_channel_id),
	                    x_jbf_vba.n_reg);
	}

    x_jbf_addr.n_reg = 0;
    x_jbf_addr.x_fields.n_base_address = (field_t) (n_jbf_addr >> 10);
	x_jbf_addr.x_fields.b_drop_on_valid = (field_t) 1;
    
      ag_nd_reg_write(p_device, 
                    AG_REG_JITTER_BUFFER_QUEUE_BASE_ADDRESS(n_channel_id),
                    x_jbf_addr.n_reg);
  #endif
    /* */
    /* enable CHE: it is important to enable CHE before JBF reset */
    /* */
    ag_nd_reg_read(p_device, AG_REG_EGRESS_CHANNEL_CONTROL(n_channel_id), &(x_che_ctl.n_reg));
    x_che_ctl.x_fields.b_channel_enable = AG_TRUE;
    ag_nd_reg_write(p_device, AG_REG_EGRESS_CHANNEL_CONTROL(n_channel_id), x_che_ctl.n_reg);

    /* */
    /* reset JBF */
    /*  */
    ag_nd_channel_egress_jbf_reset(p_device, n_channel_id);

    /* */
    /* enable RPC */
    /*  */
    ag_nd_reg_read(p_device, AG_REG_RECEIVE_CHANNEL_CONFIGURATION(n_channel_id), &(x_rcp_cfg.n_reg));
    x_rcp_cfg.x_fields.b_channel_enable = AG_TRUE;
    ag_nd_reg_write(p_device, AG_REG_RECEIVE_CHANNEL_CONFIGURATION(n_channel_id), x_rcp_cfg.n_reg);

 
    /* */
    /* update shadow structures */
    /* */
    p_device->a_channel[n_channel_id].a_enable[AG_ND_PATH_EGRESS] = AG_TRUE;


    return n_ret;

	AG_UNUSED_PARAM(n_cookie);
}

/*///////////////////////////////////////////////////////////////////////////// */
/* restarts JBF */
/*  */

void ag_nd_channel_egress_jbf_reset(AgNdDevice *p_device, AgNdChannel n_channel_id)
{
    AgNdRegJitterBufferChannelStatus    x_jbf_stat;
    AgNdRegPacketSyncStatus             x_jbf_sync;

    AG_U32  n_usec_start;
    AG_U32  n_usec_current;

    /* */
    /* reset JBF */
    /*  */
    x_jbf_stat.n_reg = 0;
    x_jbf_stat.x_fields.n_jitter_buffer_state = AG_ND_JITTER_BUFFER_STATE_FLUSH; 
    x_jbf_stat.x_fields.b_trimming_mode = AG_TRUE;
    ag_nd_reg_write(p_device, AG_REG_JITTER_BUFFER_CHANNEL_STATUS(n_channel_id), x_jbf_stat.n_reg);

    /* */
    /* need to wait for JBF read machine enter LOPS state */
    /* it will take 250 usec in a worst case */
    /*  */
    n_usec_start = ag_nd_time_usec();

    do
    {
        n_usec_current = ag_nd_time_usec();
        ag_nd_reg_read(p_device, AG_REG_PACKET_SYNC_STATUS(n_channel_id), &(x_jbf_sync.n_reg));
        
    } while (AG_ND_PACKET_SYNC_LOPS != x_jbf_sync.x_fields.n_packet_sync_state &&
             n_usec_current - n_usec_start < 500);

    AG_ND_TRACE(p_device, 
                AG_ND_TRACE_API, 
                AG_ND_TRACE_DEBUG, 
                "chid %hu: waited for %lu usec for JBF to reset\n", 
                n_channel_id,
                n_usec_current - n_usec_start);


    /* */
    /* ensure JBF is indeed reset */
    /*  */
    ag_nd_reg_read(p_device, AG_REG_JITTER_BUFFER_CHANNEL_STATUS(n_channel_id), &(x_jbf_stat.n_reg));

    assert(AG_FALSE == x_jbf_stat.x_fields.b_trimming_mode);
    assert(AG_ND_JITTER_BUFFER_STATE_IDLE == x_jbf_stat.x_fields.n_jitter_buffer_state);
    assert(AG_ND_PACKET_SYNC_LOPS == x_jbf_sync.x_fields.n_packet_sync_state);  
}

/*///////////////////////////////////////////////////////////////////////////// */
/* disables egress channel */
/*  */
AgResult
ag_nd_channel_egress_disable(AgNdDevice *p_device, AgNdChannel n_channel_id, AG_U32 n_jb_size_milli)
{
    AgNdRegChannelControl                           x_che_ctl;
    AgNdRegReceiveChannelConfiguration              x_rcp_cfg;
  
   #ifndef CES16_BCM_VERSION
    AgNdRegJitterBufferQueueBaseAddress             x_jbf_addr;
    AgNdRegJitterBufferValidBitArrayConfiguration   x_jbf_vba;

    AG_U32 n_jbf_addr;
    AG_U32 n_vba_addr;

    AgResult n_ret;
   #endif

#ifdef AG_ND_ENABLE_VALIDATION
    /* */
    /* check if channel is already disabled */
    /* */
    if (!ag_nd_channel_egress_is_enabled(p_device, n_channel_id))
        return AG_ND_ERR(p_device, AG_E_ND_ENABLE);
#endif

    /* */
    /* disable RPC */
    /* */
    ag_nd_reg_read(p_device, AG_REG_RECEIVE_CHANNEL_CONFIGURATION(n_channel_id), &(x_rcp_cfg.n_reg));
    x_rcp_cfg.x_fields.b_channel_enable = AG_FALSE;
    ag_nd_reg_write(p_device, AG_REG_RECEIVE_CHANNEL_CONFIGURATION(n_channel_id), x_rcp_cfg.n_reg);


	ag_wait_micro_sec_delay(n_jb_size_milli * 1000 * ND_JB_RESET_DELAY_FACTOR);
    
    /* */
    /* reset JBF: it is important to keep CHE enabled in order to JBF reset to succeed */
    /* */
    ag_nd_channel_egress_jbf_reset(p_device, n_channel_id);

    /* */
    /* and then disable CHE */
    /* */
    ag_nd_reg_read(p_device, AG_REG_EGRESS_CHANNEL_CONTROL(n_channel_id), &(x_che_ctl.n_reg));
    assert(x_che_ctl.x_fields.b_channel_enable == p_device->a_channel[n_channel_id].a_enable[AG_ND_PATH_EGRESS]);
    x_che_ctl.x_fields.b_channel_enable = AG_FALSE;
    ag_nd_reg_write(p_device, AG_REG_EGRESS_CHANNEL_CONTROL(n_channel_id), x_che_ctl.n_reg);

    /* */
    /* deallocate JBF and VBA */
    /* */
  #ifndef CES16_BCM_VERSION
    ag_nd_reg_read(p_device, AG_REG_JITTER_BUFFER_QUEUE_BASE_ADDRESS(n_channel_id), &(x_jbf_addr.n_reg));
    n_jbf_addr = (AG_U32)x_jbf_addr.x_fields.n_base_address << 10;
    n_ret = ag_nd_allocator_free(&(p_device->x_allocator_jbf), n_jbf_addr);
    assert(AG_SUCCEEDED(n_ret));
	if (!p_device->b_dynamic_memory)
	{
		ag_nd_reg_read(p_device, AG_REG_JITTER_BUFFER_VALID_BIT_ARRAY_CONFIGURATION(n_channel_id), &(x_jbf_vba.n_reg));
		n_vba_addr = (AG_U32)x_jbf_vba.x_fields.n_valid_bit_array_base_address << 3;
	    n_ret = ag_nd_allocator_free(&(p_device->x_allocator_vba), n_vba_addr);
		x_jbf_vba.x_fields.n_valid_bit_array_base_address = 0;
		ag_nd_reg_write(p_device, AG_REG_JITTER_BUFFER_VALID_BIT_ARRAY_CONFIGURATION(n_channel_id), x_jbf_vba.n_reg);
	}
    assert(AG_SUCCEEDED(n_ret));


    /* */
    /* set JBF and VBA addresses to 0 */
    /* */
    x_jbf_addr.x_fields.n_base_address = 0;
    ag_nd_reg_write(p_device, AG_REG_JITTER_BUFFER_QUEUE_BASE_ADDRESS(n_channel_id), x_jbf_addr.n_reg);
  #endif

    /* */
    /* update shadow structures */
    /* */
    p_device->a_channel[n_channel_id].a_enable[AG_ND_PATH_EGRESS] = (AG_BOOL)x_che_ctl.x_fields.b_channel_enable;
	p_device->a_channel[n_channel_id].n_jbf_ring_size = 0;


    return AG_S_OK;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_command_jbf_slip */
/* */
AgResult
ag_nd_opcode_write_command_jbf_slip(
    AgNdDevice *p_device,
    AgNdMsgCommandJbfSlip *p_msg)
{
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_channel_id=%hu\n", p_msg->n_channel_id);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_size=%lu\n", p_msg->n_size);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "e_dir=%d\n", p_msg->e_dir);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_unit=%d\n", p_msg->e_unit);


#ifdef AG_ND_ENABLE_VALIDATION
    /* */
    /* sanity */
    /* */
    if (p_msg->n_channel_id >= p_device->n_pw_max)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif


    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_command_stop_trimming */
/* */
AgResult
ag_nd_opcode_write_command_stop_trimming(
    AgNdDevice *p_device,
    AgNdMsgCommandStopTrimming *p_msg)
{
    AgNdRegJitterBufferChannelStatus    x_stat;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_channel_id=%hu\n", p_msg->n_channel_id);


#ifdef AG_ND_ENABLE_VALIDATION
    /* */
    /* sanity */
    /* */
    if (p_msg->n_channel_id >= p_device->n_pw_max)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif


    /* */
    /* stop trimming mode by clearing the trimming bit  */
    /* */
    x_stat.n_reg = 0;
    x_stat.x_fields.b_trimming_mode = AG_TRUE;

    ag_nd_reg_write(
        p_device, 
        AG_REG_JITTER_BUFFER_CHANNEL_STATUS(p_msg->n_channel_id), 
        x_stat.n_reg);


    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_command_jbf_restart */
/* */
AgResult
ag_nd_opcode_write_command_jbf_restart(
    AgNdDevice *p_device,
    AgNdMsgCommandStopTrimming *p_msg)
{
    AgNdRegJitterBufferChannelStatus    x_stat;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_channel_id=%hu\n", p_msg->n_channel_id);


#ifdef AG_ND_ENABLE_VALIDATION
    /* */
    /* sanity */
    /* */
    if (p_msg->n_channel_id >= p_device->n_pw_max)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif


    /* */
    /* restart JBF  */
    /* */
    x_stat.n_reg = 0;
    x_stat.x_fields.n_jitter_buffer_state = AG_ND_JITTER_BUFFER_STATE_FLUSH;

    ag_nd_reg_write(
        p_device, 
        AG_REG_JITTER_BUFFER_CHANNEL_STATUS(p_msg->n_channel_id), 
        x_stat.n_reg);

    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_config_compute_ring_size */
/* */
AgResult 
ag_nd_opcode_read_config_compute_ring_size(
    AgNdDevice *p_device, 
    AgNdMsgConfigComputeRingSize *p_msg)
{
   #ifndef CES16_BCM_VERSION
    AG_U32  n_segment_size;
    AG_U32  n_ring;
   #endif

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_payload_size=%lu\n", p_msg->n_payload_size);
  #ifndef CES16_BCM_VERSION
    n_segment_size = ag_nd_round_up2(p_msg->n_payload_size, AG_ND_ALIGNMENT_JBF_SEGMENT);
    n_ring = p_device->n_jbf_size / n_segment_size;
    n_ring = ag_nd_flp2(n_ring);

    if (p_device->n_ring_max < n_ring)
        p_msg->n_ring_size = p_device->n_ring_max;
    else
        p_msg->n_ring_size = n_ring;

  #else
        p_msg->n_ring_size = 128;
  #endif  
    return AG_S_OK;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* PM */
/* */
AgResult 
ag_nd_opcode_read_pm_egress(
    AgNdDevice *p_device, 
    AgNdMsgPmEgress *p_msg)
{
    AG_U16 n_hi, n_lo;


    AG_ND_TRACE(p_device, AG_ND_TRACE_PERIODIC, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);


#ifdef AG_ND_ENABLE_VALIDATION
    /* */
    /* check if channel id is valid */
    /* */
    if (p_msg->n_channel_id >= p_device->n_pw_max)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif


	/*ORI*/
	/*ori switch regs order read first hi and then low*/
    ag_nd_reg_read(p_device, AG_REG_MINIMUM_JITTER_BUFFER_DEPTH_1(p_msg->n_channel_id), &(n_hi));
	ag_nd_reg_read(p_device, AG_REG_MINIMUM_JITTER_BUFFER_DEPTH_2(p_msg->n_channel_id), &n_lo);

    p_msg->n_jbf_depth_min = AG_ND_MAKE32LH(n_lo, n_hi);
    
    ag_nd_reg_read(p_device, AG_REG_MAXIMUM_JITTER_BUFFER_DEPTH_1(p_msg->n_channel_id), &(n_hi));
	ag_nd_reg_read(p_device, AG_REG_MAXIMUM_JITTER_BUFFER_DEPTH_2(p_msg->n_channel_id), &n_lo);
    p_msg->n_jbf_depth_max = AG_ND_MAKE32LH(n_lo, n_hi);
    
    ag_nd_reg_read(p_device, AG_REG_PWE3_IN_BAD_LENGTH_PACKET_COUNT(p_msg->n_channel_id), &n_lo);
    p_msg->n_jbf_bad_length_packets = n_lo;
    
    ag_nd_reg_read(p_device, AG_REG_MISORDERED_DROPPED_PACKET_COUNT(p_msg->n_channel_id), &n_lo);
    p_msg->n_jbf_dropped_ooo_packets = n_lo;

    ag_nd_reg_read(p_device, AG_REG_REORDERED_GOOD_PACKET_COUNT(p_msg->n_channel_id), &n_lo);
    p_msg->n_jbf_reordered_ooo_packets = n_lo;
    
    ag_nd_reg_read(p_device, AG_REG_PWE_IN_RECEIVED_BYTES_1(p_msg->n_channel_id), &(n_hi));
	ag_nd_reg_read(p_device, AG_REG_PWE_IN_RECEIVED_BYTES_2(p_msg->n_channel_id), &n_lo);
    p_msg->n_rpc_pwe_in_bytes = AG_ND_MAKE32LH(n_lo, n_hi);
    
    ag_nd_reg_read(p_device, AG_REG_PWE_IN_RECEIVED_PACKETS(p_msg->n_channel_id), &n_lo);
    p_msg->n_rpc_pwe_in_packets = n_lo;
    
    ag_nd_reg_read(p_device, AG_REG_CLASSIFIER_CHANNEL_SPECIFIC_PM_COUNTER(p_msg->n_channel_id, 0), &n_lo);
    p_msg->a_rpc_channel_specific[0] = n_lo;

    ag_nd_reg_read(p_device, AG_REG_CLASSIFIER_CHANNEL_SPECIFIC_PM_COUNTER(p_msg->n_channel_id, 1), &n_lo);
    p_msg->a_rpc_channel_specific[1] = n_lo;

    ag_nd_reg_read(p_device, AG_REG_CLASSIFIER_CHANNEL_SPECIFIC_PM_COUNTER(p_msg->n_channel_id, 2), &n_lo);
    p_msg->a_rpc_channel_specific[2] = n_lo;

    ag_nd_reg_read(p_device, AG_REG_CLASSIFIER_CHANNEL_SPECIFIC_PM_COUNTER(p_msg->n_channel_id, 3), &n_lo);
    p_msg->a_rpc_channel_specific[3] = n_lo;

	/*ORI*/
	/*need fix to neptun for brodcom*/
    /* CAS TEMP: remove if when implemented in Nemo */
    if (1 /*AG_ND_ARCH_MASK_NEPTUNE == p_device->n_chip_mask*/)  /*BCM as neptune, cas packets do not bring to cpu*/
    {
        ag_nd_reg_read(p_device, AG_REG_CAS_MISORDERED_DROPPED_PACKET_COUNT(p_msg->n_channel_id), &n_lo);
        p_msg->n_cas_dropped = n_lo;
    }

    return AG_S_OK;
}



AgResult ag_nd_opcode_write_config_vlan_egress(AgNdDevice *p_device, AgNdVlanHeader* p_msg)
{
	assert(p_device);
	assert(p_msg);

	ag_nd_strict_update_vlan_id(p_device,
								p_msg->n_vlan_count,
							    p_msg->a_vlan,
							    p_msg->n_channel_id,
							    p_msg->b_ptp,
							    NULL); /* address of VLAN ID in strict, will be calculated in the function */

	return AG_S_OK;
}

