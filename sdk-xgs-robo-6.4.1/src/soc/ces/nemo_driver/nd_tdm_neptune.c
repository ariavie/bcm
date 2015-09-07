/* $Id: 948ccdfd64cc8992fdf0952c36cfbf50112907e4 $
 * Copyright 2011 BATM
 */

#include "nd_tdm_neptune.h"
#include "nd_registers.h"
#include "nd_debug.h"


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_config_neptune_oc3_hierarchy */
/* */
AgResult 
ag_nd_opcode_write_config_neptune_oc3_hierarchy(
    AgNdDevice *p_device, 
    AgNdMsgConfigNeptuneOc3Hierarchy *p_msg)
{
    AgNdRegTdmBusHierarchyConfiguration x_oc3;
    AG_U32 n_vtg_type;
    AG_S32 n_spe_idx;
    AG_S32 n_vtg_idx;


    /* */
    /* debug trace */
    /* */
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);

    for (n_spe_idx = 0; n_spe_idx < AG_ND_SPE_MAX; n_spe_idx++)
    {
        AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, 
                    "spe=%lu type=%d, vtg types:\n", 
                    n_spe_idx, 
                    p_msg->x_spe[n_spe_idx].n_mode);

        for (n_vtg_idx = 0; n_vtg_idx < AG_ND_VTG_MAX; n_vtg_idx++)
            AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, 
                        "%d\n", 
                        p_msg->x_spe[n_spe_idx].a_vtg_mode[n_vtg_idx]);

    }


    /* */
    /* TDM Bus Hierarchy register */
    /* */
    x_oc3.n_reg = 0;
    x_oc3.x_fields.n_tdm_bus_type = AG_ND_TDM_BUS_TYPE_SBI;
    x_oc3.x_fields.n_vc4_circuit_selection = AG_ND_VC4_SELECT_CHANNELIZED;

    x_oc3.x_fields.n_spe_1_mode = p_msg->x_spe[0].n_mode;
    x_oc3.x_fields.n_spe_2_mode = p_msg->x_spe[1].n_mode;
    x_oc3.x_fields.n_spe_3_mode = p_msg->x_spe[2].n_mode;

    ag_nd_reg_write(p_device, AG_REG_TDM_BUS_HIERARCHY_CONFIGURATION, x_oc3.n_reg);


    /* */
    /* Trubutary Type per VC-2 register */
    /* */
    n_vtg_type = 0;

    for (n_spe_idx = AG_ND_SPE_MAX - 1; n_spe_idx >= 0; n_spe_idx--)
    {
        n_vtg_type <<= 1;

        for (n_vtg_idx = AG_ND_VTG_MAX - 1; n_vtg_idx >= 0; n_vtg_idx--)
        {
            n_vtg_type <<= 1;

            if (AG_ND_TDM_PROTOCOL_E1 == p_msg->x_spe[n_spe_idx].a_vtg_mode[n_vtg_idx])
                n_vtg_type |= 0;
            else
                n_vtg_type |= 1;
        }
    }

    ag_nd_reg_write(p_device, AG_REG_TRIBUTARY_CIRCUIT_TYPE_PER_VC_2_1, AG_ND_HIGH16(n_vtg_type));
    ag_nd_reg_write(p_device, AG_REG_TRIBUTARY_CIRCUIT_TYPE_PER_VC_2_2, AG_ND_LOW16(n_vtg_type));


    return AG_S_OK;
}




/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_config_neptune_oc3_hierarchy */
/* */
AgResult 
ag_nd_opcode_read_config_neptune_oc3_hierarchy(
    AgNdDevice *p_device, 
    AgNdMsgConfigNeptuneOc3Hierarchy *p_msg)
{
    AgNdRegTdmBusHierarchyConfiguration x_oc3;
    AG_U32 n_vtg_type;
    AG_U32 n_spe_idx;
    AG_U32 n_vtg_idx;
    AG_U16 n_vtg_type_lo;
    AG_U16 n_vtg_type_hi;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);



    ag_nd_reg_read(p_device, AG_REG_TDM_BUS_HIERARCHY_CONFIGURATION, &x_oc3.n_reg);

    p_msg->x_spe[0].n_mode = x_oc3.x_fields.n_spe_1_mode;
    p_msg->x_spe[1].n_mode = x_oc3.x_fields.n_spe_2_mode;
    p_msg->x_spe[2].n_mode = x_oc3.x_fields.n_spe_3_mode;


    ag_nd_reg_read(p_device, AG_REG_TRIBUTARY_CIRCUIT_TYPE_PER_VC_2_1, &n_vtg_type_hi);
    ag_nd_reg_read(p_device, AG_REG_TRIBUTARY_CIRCUIT_TYPE_PER_VC_2_2, &n_vtg_type_lo);
    n_vtg_type = AG_ND_MAKE32LH(n_vtg_type_lo, n_vtg_type_hi);


    for (n_spe_idx = 0; n_spe_idx < AG_ND_SPE_MAX; n_spe_idx++)
    {
        for (n_vtg_idx = 0; n_vtg_idx < AG_ND_VTG_MAX; n_vtg_idx++)
        {
            if (n_vtg_type & 1)
                p_msg->x_spe[n_spe_idx].a_vtg_mode[n_vtg_idx] = AG_ND_TDM_PROTOCOL_E1;
            else
                p_msg->x_spe[n_spe_idx].a_vtg_mode[n_vtg_idx] = AG_ND_TDM_PROTOCOL_T1;

            n_vtg_type >>= 1;
        }

        n_vtg_type >>= 1;
    }


    return AG_S_OK;
}




/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_config_neptune_circuit */
/* */
AgResult 
ag_nd_opcode_write_config_neptune_circuit(
    AgNdDevice *p_device, 
    AgNdMsgConfigNeptuneCircuit *p_msg)
{
    AgNdRegTributaryCircuitConfiguration x_circuit;
    

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "circuit=%hhu\n", p_msg->n_circuit_id);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "structured=%lu\n", p_msg->b_structured);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "octet=%lu\n", p_msg->b_octet_aligned);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "signaling=%lu\n", p_msg->b_cas_enable);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_prime=%lu\n", p_msg->b_prime);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "D4=%lu\n", p_msg->b_T1_D4_framing);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "master=%lu\n", p_msg->b_clock_master);


#ifdef AG_ND_ENABLE_VALIDATION
    /* */
    /* sanity */
    /*  */
    if (!p_device->p_circuit_is_valid_id(p_device, p_msg->n_circuit_id))
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if (p_msg->b_octet_aligned && p_msg->b_structured)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if (p_msg->b_cas_enable && !p_msg->b_structured)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif


    /* */
    /* configure circuit */
    /* */
    ag_nd_reg_read(p_device, AG_REG_TRIBUTARY_CIRCUIT_CONFIGURATION(p_msg->n_circuit_id), &x_circuit.n_reg);

    x_circuit.x_fields.b_structured_service = (field_t) p_msg->b_structured;
    x_circuit.x_fields.b_octet_aligned_mode_in_satop_t1 = (field_t) p_msg->b_octet_aligned;
    x_circuit.x_fields.b_clock_master = (field_t) p_msg->b_clock_master;
    x_circuit.x_fields.b_cas_signaling_enable = (field_t) p_msg->b_cas_enable;
    x_circuit.x_fields.b_prime_rtp_timestamp_mode_enable = (field_t) p_msg->b_prime;
    x_circuit.x_fields.b_t1_d4_framing_mode_enable = (field_t) p_msg->b_T1_D4_framing;

    ag_nd_reg_write(p_device, AG_REG_TRIBUTARY_CIRCUIT_CONFIGURATION(p_msg->n_circuit_id), x_circuit.n_reg);


    return AG_S_OK;
}




/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_config_neptune_circuit */
/* */
AgResult 
ag_nd_opcode_read_config_neptune_circuit(
    AgNdDevice *p_device, 
    AgNdMsgConfigNeptuneCircuit *p_msg)
{
    AgNdRegTributaryCircuitConfiguration x_circuit;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);


#ifdef AG_ND_ENABLE_VALIDATION
    /* */
    /* sanity */
    /*  */
    if (!p_device->p_circuit_is_valid_id(p_device, p_msg->n_circuit_id))
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif

    /* */
    /* read */
    /* */
    ag_nd_reg_read(p_device, AG_REG_TRIBUTARY_CIRCUIT_CONFIGURATION(p_msg->n_circuit_id), &x_circuit.n_reg);

    p_msg->b_structured = (AG_BOOL)x_circuit.x_fields.b_structured_service;
    p_msg->b_octet_aligned = (AG_BOOL)x_circuit.x_fields.b_octet_aligned_mode_in_satop_t1;
    p_msg->b_clock_master = (AG_BOOL)x_circuit.x_fields.b_clock_master;
    p_msg->b_enable = (AG_BOOL)x_circuit.x_fields.b_circuit_enable;
    p_msg->b_cas_enable = (AG_BOOL)x_circuit.x_fields.b_cas_signaling_enable;
    p_msg->b_prime = (AG_BOOL)x_circuit.x_fields.b_prime_rtp_timestamp_mode_enable;
    p_msg->b_T1_D4_framing = (AG_BOOL)x_circuit.x_fields.b_t1_d4_framing_mode_enable;

    return AG_S_OK;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_config_neptune_bsg */
/* */
AgResult 
ag_nd_opcode_write_config_neptune_bsg(
    AgNdDevice *p_device, 
    AgNdMsgConfigNeptuneBsg *p_msg)
{
#ifndef CES16_BCM_VERSION
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "circuit=%hhu\n", p_msg->n_circuit_id);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "step=%lu\n", p_msg->n_step_size);


#ifdef AG_ND_ENABLE_VALIDATION
    if (!p_device->p_circuit_is_valid_id(p_device, p_msg->n_circuit_id))
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

	if (p_msg->n_bsg_count >= 256)
	{
        /*/return AG_ND_ERR(p_device, AG_E_ND_ARG); */
	}
#endif


	ag_nd_reg_write(p_device, 
                        AG_REG_BIT_STUFFING_GENERATOR_1(p_msg->n_circuit_id), 
                        AG_ND_HIGH16(p_msg->n_step_size));
    
    ag_nd_reg_write(p_device, 
                        AG_REG_BIT_STUFFING_GENERATOR_2(p_msg->n_circuit_id), 
                        AG_ND_LOW16(p_msg->n_step_size));
#endif
#if 0
	if (p_msg->n_bsg_count >= 0)
	{		
		ag_nd_reg_write(p_device, 
							AG_REG_BSG_ONE_SHOT_COUNTER(p_msg->n_circuit_id), 
							p_msg->n_bsg_count);
	}
#endif

    return AG_S_OK;
}





/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_config_nemo_brg */
/* */
AgResult 
ag_nd_opcode_read_config_neptune_bsg(
    AgNdDevice *p_device, 
    AgNdMsgConfigNeptuneBsg *p_msg)
{
   #ifndef CES16_BCM_VERSION

    AG_U16 n_lo;
    AG_U16 n_hi;
	AG_U16 n_bsg_count;

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);


#ifdef AG_ND_ENABLE_VALIDATION
    if (!p_device->p_circuit_is_valid_id(p_device, p_msg->n_circuit_id))
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif


    ag_nd_reg_read(p_device, AG_REG_BIT_STUFFING_GENERATOR_1(p_msg->n_circuit_id), &n_hi);
    ag_nd_reg_read(p_device, AG_REG_BIT_STUFFING_GENERATOR_2(p_msg->n_circuit_id), &n_lo);

    p_msg->n_step_size = AG_ND_MAKE32LH(n_lo, n_hi);

	ag_nd_reg_read(p_device, 
					AG_REG_BSG_ONE_SHOT_COUNTER(p_msg->n_circuit_id), 
					&n_bsg_count);

	p_msg->n_bsg_count = n_bsg_count;
    #endif
    return AG_S_OK;
}




/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_status_neptune_circuit */
/* */
AgResult 
ag_nd_opcode_read_status_neptune_circuit(
    AgNdDevice *p_device, 
    AgNdMsgStatusNeptuneCircut *p_msg)
{
    AgNdRegJustificationRequestControl x_jst_ctl;
    AG_U16 n_reg;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);


#ifdef AG_ND_ENABLE_VALIDATION
    if (!p_device->p_circuit_is_valid_id(p_device, p_msg->n_circuit_id))
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif


    ag_nd_reg_read(p_device, 
                   AG_REG_JUSTIFICATION_REQUEST_CONTROL(p_msg->n_circuit_id), 
                   &x_jst_ctl.n_reg);

    ag_nd_reg_read(p_device, 
                   AG_REG_RECOVERED_LINK_RATE_PHASE_INTEGRATOR(p_msg->n_circuit_id), 
                   &n_reg);

    p_msg->b_adjustment_in_progress = x_jst_ctl.x_fields.b_explicit_justification_request;
    p_msg->n_phase_shift = n_reg;


    return AG_S_OK;
}



/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_circuit_neptune_is_valid_id */
/* */
AG_BOOL
ag_nd_circuit_neptune_is_valid_id(AgNdDevice *p_device, AgNdCircuit n_circuit_id)
{
    if ((n_circuit_id & 0x60) == 0x60) /* spe == 3 */
        return AG_FALSE;

    if ((n_circuit_id & 0x1c) == 0x1c) /* vtg == 7 */
        return AG_FALSE;

    return AG_TRUE;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_circuit_neptune_create_id */
/* */
AgNdCircuit
ag_nd_circuit_neptune_create_id(AG_U32 n_spe, AG_U32 n_vtg, AG_U32 n_vt)
{
    AG_BOOL b_valid;


    b_valid = (n_spe < AG_ND_SPE_MAX && n_vtg < AG_ND_VTG_MAX && n_vt < AG_ND_VT_MAX);


    if (b_valid)
    {
        return (AgNdCircuit) (( n_spe << 5) | (n_vtg << 2) | n_vt );
    }
    else
    {
        assert(n_spe < AG_ND_SPE_MAX);
        assert(n_vtg < AG_ND_VTG_MAX);
        assert(n_vt < AG_ND_VT_MAX);

        return (AgNdCircuit)-1;
    }   
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_circuit_neptune_get_max_idx */
/* */
AG_U32
ag_nd_circuit_neptune_get_max_idx(AgNdDevice *p_device)
{
    return AG_ND_SPE_MAX * AG_ND_VTG_MAX * AG_ND_VT_MAX;
}

/*///////////////////////////////////////////////////////////////////////////// */
/*  ag_nd_circuit_neptune_get_next_id */
/* */
AgNdCircuit
ag_nd_circuit_neptune_get_next_id(AgNdDevice *p_device, AgNdCircuit n_circuit_id)
{
  /*  (void*)p_device = (void*)0;*/

    n_circuit_id++;

    /* */
    /* if VTG index reached 7, move to next SPE */
    /*  */
    if ((n_circuit_id & 0x1c) == 0x1c)
        n_circuit_id += 0x4;

    /* */
    /* if SPE index reached 4, we done */
    /* */
    if ((n_circuit_id & 0x60) == 0x60)
        return (AgNdCircuit)-1;

    return n_circuit_id;
}



#define AG_ND_CIRCUIT_MAP_SIZE  0xff

static AgNdCircuit  a_idx_to_id_map[AG_ND_CIRCUIT_MAP_SIZE];
static AG_U32       a_id_to_idx_map[AG_ND_CIRCUIT_MAP_SIZE];


/*///////////////////////////////////////////////////////////////////////////// */
/*  ag_nd_circuit_neptune_create_map */
/* */
void
ag_nd_circuit_neptune_create_maps(AgNdDevice *p_device)
{
    AG_U32      i;
    AgNdCircuit n_id;

    for (i = 0; i < AG_ND_CIRCUIT_MAP_SIZE; i++)
    {
        a_idx_to_id_map[i] = (AgNdCircuit)-1;
        a_id_to_idx_map[i] = (AG_U32)-1;
    }

    for (i = 0; i < AG_ND_CIRCUIT_MAP_SIZE; i++)
    {
        a_idx_to_id_map[i] = (AgNdCircuit)-1;
        a_id_to_idx_map[i] = (AG_U32)-1;
    }

    n_id = 0;
    i = 0;

    do
    {
        a_id_to_idx_map[(int) n_id] = i;
        a_idx_to_id_map[i] = n_id;

        i++;
        n_id = ag_nd_circuit_neptune_get_next_id(p_device, n_id);
    }
    while (n_id != (AgNdCircuit)-1);
}

/*///////////////////////////////////////////////////////////////////////////// */
/*  ag_nd_circuit_neptune_id_to_idx */
/* */
AG_U32
ag_nd_circuit_neptune_id_to_idx(AgNdCircuit n_circuit_id)
{
    if (n_circuit_id >= (AG_U8)AG_ND_CIRCUIT_MAP_SIZE)
        return (AG_U32)-1;
    else
        return a_id_to_idx_map[(int)n_circuit_id];
}

/*///////////////////////////////////////////////////////////////////////////// */
/*  ag_nd_circuit_neptune_idx_to_id */
/* */
AgNdCircuit
ag_nd_circuit_neptune_idx_to_id(AG_U32 n_circuit_idx)
{
    if (n_circuit_idx >= AG_ND_CIRCUIT_MAP_SIZE)
        return (AgNdCircuit)-1;
    else
        return a_idx_to_id_map[n_circuit_idx];
}

/*///////////////////////////////////////////////////////////////////////////// */
/*  ag_nd_circuit_neptune_get_spe */
/* */
AG_U32      
ag_nd_circuit_neptune_get_spe(AgNdCircuit n_circuit_id)
{
    return ((AG_U32)n_circuit_id & 0x60) >> 5;
}

/*///////////////////////////////////////////////////////////////////////////// */
/*  ag_nd_circuit_neptune_get_vtg */
/* */
AG_U32      
ag_nd_circuit_neptune_get_vtg(AgNdCircuit n_circuit_id)
{
    return ((AG_U32)n_circuit_id & 0x1c) >> 2;
}

/*///////////////////////////////////////////////////////////////////////////// */
/*  ag_nd_circuit_neptune_get_vt */
/* */
AG_U32      
ag_nd_circuit_neptune_get_vt(AgNdCircuit n_circuit_id)
{
    return (AG_U32)n_circuit_id & 0x3;
}

