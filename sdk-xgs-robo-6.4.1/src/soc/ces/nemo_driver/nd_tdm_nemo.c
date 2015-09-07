/* $Id: 99070f85293da471296cab1022cddfa15ca0b763 $
 * Copyright 2011 BATM
 */

#include "nd_tdm_nemo.h"
#include "nd_registers.h"
#include "nd_debug.h"


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_config_nemo_cclk */
/* */
AgResult 
ag_nd_opcode_write_config_nemo_cclk(AgNdDevice *p_device, AgNdMsgConfigNemoCClk *p_msg)
{
    AgNdRegT1E1CClkMux1           x_clk_mux_1;
    AgNdRegT1E1CClkMux2           x_clk_mux_2;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "cclk_en=%lu\n", p_msg->b_cclk_enable);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "cclk_sel=%d\n", p_msg->e_cclk_select);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "rclk_proto=%d\n", p_msg->e_ref_clk_proto);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "rclk1_sel=%d\n", p_msg->e_ref_clk_1_select);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "rclk2_sel=%d\n", p_msg->e_ref_clk_2_select);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "rclk1_prt=%lu\n", p_msg->n_ref_clk_1_port);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "rclk2_prt=%lu\n", p_msg->n_ref_clk_2_port);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "rclk1_brg=%lu\n", p_msg->n_ref_clk_1_brg);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "rclk2_brg=%lu\n", p_msg->n_ref_clk_2_brg);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "eclk1_dir=%d\n", p_msg->e_ext_clk_1_dir);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "eclk2_dir=%d\n", p_msg->e_ext_clk_2_dir);
/*/#ifdef AG_ND_PTP_SUPPORT */
	if (p_device->b_ptp_support)
	{
	    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "eclk1_ptp=%d\n", p_msg->e_ref_clk_1_ptp);
	    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "eclk2_ptp=%d\n", p_msg->e_ref_clk_2_ptp);
	}
/*/#endif */

#ifdef AG_ND_ENABLE_VALIDATION
    /* */
    /* sanity */
    /* */
    if (p_msg->e_cclk_select < AG_ND_CCLK_SELECT_REF_CLK_1 ||
        p_msg->e_cclk_select > AG_ND_CCLK_SELECT_EXTERNAL_CLK)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if (p_msg->e_ref_clk_1_select < AG_ND_REF_CLK_SELECT_RCLK ||
        p_msg->e_ref_clk_1_select > AG_ND_REF_CLK_SELECT_PTP)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if (p_msg->e_ref_clk_2_select < AG_ND_REF_CLK_SELECT_RCLK ||
        p_msg->e_ref_clk_2_select > AG_ND_REF_CLK_SELECT_PTP)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if (p_msg->e_ref_clk_proto != AG_ND_TDM_PROTOCOL_E1 &&
        p_msg->e_ref_clk_proto != AG_ND_TDM_PROTOCOL_T1)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if (p_msg->n_ref_clk_1_brg > p_device->n_total_ports ||
        p_msg->n_ref_clk_2_brg > p_device->n_total_ports ||
        p_msg->n_ref_clk_1_port > p_device->n_total_ports ||
        p_msg->n_ref_clk_2_port > p_device->n_total_ports)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if (p_msg->e_ext_clk_1_dir < AG_ND_EXT_CLK_DIR_INPUT ||
        p_msg->e_ext_clk_1_dir > AG_ND_EXT_CLK_DIR_OUTPUT)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if (p_msg->e_ext_clk_2_dir < AG_ND_EXT_CLK_DIR_INPUT ||
        p_msg->e_ext_clk_2_dir > AG_ND_EXT_CLK_DIR_OUTPUT)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

/*/#ifdef AG_ND_PTP_SUPPORT */
	if (p_device->b_ptp_support)
	{
	    if (p_msg->e_ref_clk_1_ptp < AG_ND_PTP_CLK_1 ||
	        p_msg->e_ref_clk_1_ptp > AG_ND_PTP_1PPS_2)
	        return AG_ND_ERR(p_device, AG_E_ND_ARG);

	    if (p_msg->e_ref_clk_2_ptp < AG_ND_PTP_CLK_1 ||
	        p_msg->e_ref_clk_2_ptp > AG_ND_PTP_1PPS_2)
	        return AG_ND_ERR(p_device, AG_E_ND_ARG);
	}
/*/#endif // AG_ND_PTP_SUPPORT */
#endif /* AG_ND_ENABLE_VALIDATION */

    
    x_clk_mux_1.n_reg = 0;
    #ifndef CES16_BCM_VERSION
    x_clk_mux_1.x_fields.n_reference_clk1_brg_select = (field_t) p_msg->n_ref_clk_1_brg;
    x_clk_mux_1.x_fields.n_reference_clk2_brg_select = (field_t) p_msg->n_ref_clk_2_brg;
    #endif
    x_clk_mux_1.x_fields.n_reference_clk1_port_select = (field_t) p_msg->n_ref_clk_1_port;
    x_clk_mux_1.x_fields.n_reference_clk2_port_select = (field_t) p_msg->n_ref_clk_2_port;
    ag_nd_reg_write(p_device, AG_REG_T1_E1_CCLK_MUX_1, x_clk_mux_1.n_reg);

    x_clk_mux_2.x_fields.b_cclk_out_enable = (field_t) p_msg->b_cclk_enable;
    x_clk_mux_2.x_fields.n_reference_clk_t1_e1_select = (field_t) p_msg->e_ref_clk_proto;
    x_clk_mux_2.x_fields.n_cclk_source_select = (field_t) p_msg->e_cclk_select;
    x_clk_mux_2.x_fields.n_reference_clk1_select = (field_t) p_msg->e_ref_clk_1_select;
    x_clk_mux_2.x_fields.n_reference_clk2_select = (field_t) p_msg->e_ref_clk_2_select;
    x_clk_mux_2.x_fields.n_external_clk1_dir = (field_t) p_msg->e_ext_clk_1_dir;
    x_clk_mux_2.x_fields.n_external_clk2_dir = (field_t) p_msg->e_ext_clk_2_dir;

/*/#ifdef AG_ND_PTP_SUPPORT */
	if (p_device->b_ptp_support)
	{
	    x_clk_mux_2.x_fields.n_ptp_clk_out1_select = (field_t) p_msg->e_ref_clk_1_ptp;
	    x_clk_mux_2.x_fields.n_ptp_clk_out2_select = (field_t) p_msg->e_ref_clk_2_ptp;
	}
/*/#endif */

    ag_nd_reg_write(p_device, AG_REG_T1_E1_CCLK_MUX_2, x_clk_mux_2.n_reg);

    return AG_S_OK;
}



/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_config_nemo_cclk */
/* */
AgResult 
ag_nd_opcode_read_config_nemo_cclk(AgNdDevice *p_device, AgNdMsgConfigNemoCClk *p_msg)
{
    AgNdRegT1E1CClkMux1           x_clk_mux_1;
    AgNdRegT1E1CClkMux2           x_clk_mux_2;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);



    ag_nd_reg_read(p_device, AG_REG_T1_E1_CCLK_MUX_1, &(x_clk_mux_1.n_reg));

    p_msg->n_ref_clk_1_brg = x_clk_mux_1.x_fields.n_reference_clk1_brg_select;
    p_msg->n_ref_clk_2_brg = x_clk_mux_1.x_fields.n_reference_clk2_brg_select;
    p_msg->n_ref_clk_1_port = x_clk_mux_1.x_fields.n_reference_clk1_port_select;
    p_msg->n_ref_clk_2_port = x_clk_mux_1.x_fields.n_reference_clk2_port_select;

    ag_nd_reg_read(p_device, AG_REG_T1_E1_CCLK_MUX_2, &(x_clk_mux_2.n_reg));

    p_msg->e_ref_clk_proto = x_clk_mux_2.x_fields.n_reference_clk_t1_e1_select;
    p_msg->b_cclk_enable = x_clk_mux_2.x_fields.b_cclk_out_enable;
    p_msg->e_cclk_select = x_clk_mux_2.x_fields.n_cclk_source_select;
    p_msg->e_ref_clk_1_select = x_clk_mux_2.x_fields.n_reference_clk1_select;
    p_msg->e_ref_clk_2_select = x_clk_mux_2.x_fields.n_reference_clk2_select;
    p_msg->e_ext_clk_1_dir = x_clk_mux_2.x_fields.n_external_clk1_dir;
    p_msg->e_ext_clk_2_dir = x_clk_mux_2.x_fields.n_external_clk2_dir;

/*#ifdef AG_ND_PTP_SUPPORT */
	if (p_device->b_ptp_support)
	{
	    p_msg->e_ref_clk_1_ptp = x_clk_mux_2.x_fields.n_ptp_clk_out1_select;
	    p_msg->e_ref_clk_2_ptp = x_clk_mux_2.x_fields.n_ptp_clk_out2_select;
	}
/*/#endif */

    return AG_S_OK;
}



/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_config_nemo_circuit */
/* */
AgResult 
ag_nd_opcode_write_config_nemo_circuit(AgNdDevice *p_device, AgNdMsgConfigNemoCircuit *p_msg)
{
    AgNdRegT1E1PortConfiguration x_circuit;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "circuit=%hhu\n", p_msg->n_circuit_id);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "structured=%lu\n", p_msg->b_structured);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "tdm=%d\n", p_msg->e_tdm_protocol);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "octet=%lu\n", p_msg->b_octet_aligned);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "prime=%lu\n", p_msg->b_prime_enable);

    /* CAS TEMP: remove if when implemented in Nemo */
    if (1 /*AG_ND_ARCH_MASK_NEPTUNE == p_device->n_chip_mask*/)   /*BCM as neptune*/
    {
        AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "signaling=%lu\n", p_msg->b_signaling_enable);
        AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "D4=%lu\n", p_msg->b_T1_D4_framing);
    }

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "rxclk=%d\n", p_msg->e_clk_rx_select);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "txclk=%d\n", p_msg->e_clk_tx_select);



#ifdef AG_ND_ENABLE_VALIDATION
    if (!p_device->p_circuit_is_valid_id(p_device, p_msg->n_circuit_id))
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
  
    if (p_msg->b_octet_aligned && 
        (p_msg->b_structured || p_msg->e_tdm_protocol != AG_ND_TDM_PROTOCOL_T1))
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    /* CAS TEMP: remove if when implemented in Nemo */
    if (1 /*AG_ND_ARCH_MASK_NEPTUNE == p_device->n_chip_mask*/)   /*BCM as neptune*/
    {
        if (p_msg->b_signaling_enable && !p_msg->b_structured)
            return AG_ND_ERR(p_device, AG_E_ND_ARG);
        
        if (p_msg->e_tdm_protocol != AG_ND_TDM_PROTOCOL_T1 && p_msg->b_T1_D4_framing)
            return AG_ND_ERR(p_device, AG_E_ND_ARG);
    }
#endif

    /* */
    /* configure circuit */
    /*  */
    ag_nd_reg_read(
        p_device, 
        AG_REG_T1_E1_PORT_CONFIGURATION(p_msg->n_circuit_id), 
        &(x_circuit.n_reg)); 
    
    x_circuit.x_fields.b_prime_rtp_timestamp_mode_enable = (field_t) p_msg->b_prime_enable;
    x_circuit.x_fields.n_t1_e1_port_selection = (field_t) p_msg->e_tdm_protocol;
    x_circuit.x_fields.b_structured_service = (field_t) p_msg->b_structured;
   #ifndef CES16_BCM_VERSION
    x_circuit.x_fields.b_octet_aligned_mode_in_satop_t1 = (field_t) p_msg->b_octet_aligned;
   #endif
    /* CAS TEMP: remove if when implemented in Nemo */
    if (1 /*AG_ND_ARCH_MASK_NEPTUNE == p_device->n_chip_mask*/) /*BCM as neptune*/
    {
        x_circuit.x_fields.b_cas_signaling_enable = (field_t) p_msg->b_signaling_enable;
        x_circuit.x_fields.b_t1_d4_framing_mode_enable = (field_t) p_msg->b_T1_D4_framing;
    }

    x_circuit.x_fields.n_receive_clk_and_sync_source_select = (field_t) p_msg->e_clk_rx_select;
    x_circuit.x_fields.n_transmit_clk_source_select = (field_t) p_msg->e_clk_tx_select;

    ag_nd_reg_write(
        p_device, 
        AG_REG_T1_E1_PORT_CONFIGURATION(p_msg->n_circuit_id), 
        x_circuit.n_reg);


    return AG_S_OK;
}



/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_config_nemo_circuit */
/* */
AgResult 
ag_nd_opcode_read_config_nemo_circuit(AgNdDevice *p_device, AgNdMsgConfigNemoCircuit *p_msg)
{
    AgNdRegT1E1PortConfiguration    x_circuit;

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);


#ifdef AG_ND_ENABLE_VALIDATION
    if (!p_device->p_circuit_is_valid_id(p_device, p_msg->n_circuit_id))
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif


    ag_nd_reg_read(p_device, 
                   AG_REG_T1_E1_PORT_CONFIGURATION(p_msg->n_circuit_id), 
                   &x_circuit.n_reg);

    p_msg->b_enable = (AG_BOOL)x_circuit.x_fields.b_port_enable;
    p_msg->b_prime_enable = (AG_BOOL)x_circuit.x_fields.b_prime_rtp_timestamp_mode_enable;

    p_msg->e_tdm_protocol = (AgNdTdmProto)x_circuit.x_fields.n_t1_e1_port_selection;
    p_msg->b_structured = (AG_BOOL)x_circuit.x_fields.b_structured_service;

    /* CAS TEMP: remove if when implemented in Nemo */
    if (1 /*AG_ND_ARCH_MASK_NEPTUNE == p_device->n_chip_mask*/)  /*BCM as neptune*/
    {
        p_msg->b_signaling_enable = (AG_BOOL)x_circuit.x_fields.b_cas_signaling_enable;
        p_msg->b_T1_D4_framing = (AG_BOOL)x_circuit.x_fields.b_t1_d4_framing_mode_enable;
    }

    p_msg->b_octet_aligned = (AG_BOOL)x_circuit.x_fields.b_octet_aligned_mode_in_satop_t1;
    p_msg->e_clk_rx_select = (AgNdRxClkSelect)x_circuit.x_fields.n_receive_clk_and_sync_source_select;
    p_msg->e_clk_tx_select = (AgNdTxClkSelect)x_circuit.x_fields.n_transmit_clk_source_select;


    return AG_S_OK;
}



/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_config_nemo_brg */
/* */
AgResult 
ag_nd_opcode_write_config_nemo_brg(AgNdDevice *p_device, AgNdMsgConfigNemoBrg *p_msg)
{
    AG_ND_TRACE(p_device, AG_ND_TRACE_PERIODIC, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_PERIODIC, AG_ND_TRACE_DEBUG, "circuit=%hhu\n", p_msg->n_circuit_id);
    AG_ND_TRACE(p_device, AG_ND_TRACE_PERIODIC, AG_ND_TRACE_DEBUG, "n_step_size=%lu\n", p_msg->n_step_size);



#ifdef AG_ND_ENABLE_VALIDATION
    if (!p_device->p_circuit_is_valid_id(p_device, p_msg->n_circuit_id))
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif


    ag_nd_reg_write(p_device, 
                    AG_REG_PORT_CLK_BAUD_RATE_GENERATOR1(p_msg->n_circuit_id), 
                    AG_ND_HIGH16(p_msg->n_step_size));

    ag_nd_reg_write(p_device, 
                    AG_REG_PORT_CLK_BAUD_RATE_GENERATOR2(p_msg->n_circuit_id), 
                    AG_ND_LOW16(p_msg->n_step_size));


    return AG_S_OK;
}



/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_config_nemo_brg */
/* */
AgResult 
ag_nd_opcode_read_config_nemo_brg(AgNdDevice *p_device, AgNdMsgConfigNemoBrg *p_msg)
{
    AG_U16 n_hi;
    AG_U16 n_lo;


    AG_ND_TRACE(p_device, AG_ND_TRACE_PERIODIC, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__); 


#ifdef AG_ND_ENABLE_VALIDATION
    if (!p_device->p_circuit_is_valid_id(p_device, p_msg->n_circuit_id))
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif


    ag_nd_reg_read(p_device, AG_REG_PORT_CLK_BAUD_RATE_GENERATOR1(p_msg->n_circuit_id), &n_hi);
    ag_nd_reg_read(p_device, AG_REG_PORT_CLK_BAUD_RATE_GENERATOR2(p_msg->n_circuit_id), &n_lo);
    p_msg->n_step_size = AG_ND_MAKE32LH(n_lo, n_hi);


    return AG_S_OK;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_circuit_nemo_is_valid_id */
/* */
AG_BOOL     
ag_nd_circuit_nemo_is_valid_id(AgNdDevice *p_device, AgNdCircuit n_circuit_id)
{
    return (n_circuit_id < p_device->n_total_ports);
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_circuit_nemo_create_id */
/* */
AgNdCircuit 
ag_nd_circuit_nemo_create_id(AG_U32 n_port)
{
    AG_BOOL b_valid;


    b_valid = (n_port < AG_ND_PORT_MAX);


    if (b_valid)
    {
        return (AgNdCircuit)n_port;
    }
    else
    {
        assert(n_port < AG_ND_PORT_MAX);

        return (AgNdCircuit)-1;
    }   
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_circuit_nemo_get_max_idx */
/* */
AG_U32      
ag_nd_circuit_nemo_get_max_idx(AgNdDevice *p_device)
{
    return p_device->n_total_ports;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_circuit_nemo_get_next_id */
/* */
AgNdCircuit 
ag_nd_circuit_nemo_get_next_id(AgNdDevice *p_device, AgNdCircuit n_circuit_id)
{
    if (n_circuit_id >= p_device->n_total_ports) 
        return (AgNdCircuit)-1;
    else
        return n_circuit_id + 1;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_circuit_nemo_id_to_idx */
/* */
AG_U32      
ag_nd_circuit_nemo_id_to_idx(AgNdCircuit n_circuit_id)
{
    return n_circuit_id;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_circuit_nemo_idx_to_id */
/* */
AgNdCircuit 
ag_nd_circuit_nemo_idx_to_id(AG_U32 n_circuit_idx)
{
    return (AgNdCircuit)n_circuit_idx;
}


