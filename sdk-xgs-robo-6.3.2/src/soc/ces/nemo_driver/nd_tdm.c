/* $Id: nd_tdm.c 1.5 Broadcom SDK $
 * Copyright 2011 BATM
 */

#include "nd_tdm.h"
#include "nd_registers.h"
#include "nd_debug.h"


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_config_circuit_enable */
/* */
AgResult 
ag_nd_opcode_read_config_circuit_enable(AgNdDevice *p_device, AgNdMsgConfigCircuitEnable *p_msg)
{
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);


#ifdef AG_ND_ENABLE_VALIDATION
    if (!p_device->p_circuit_is_valid_id(p_device, p_msg->n_circuit_id))
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif

   #ifndef CES16_BCM_VERSION
    if (AG_ND_ARCH_MASK_NEPTUNE & p_device->n_chip_mask)
    {
        AgNdRegTributaryCircuitConfiguration x_tr_conf;

        ag_nd_reg_read(p_device, AG_REG_TRIBUTARY_CIRCUIT_CONFIGURATION(p_msg->n_circuit_id), &x_tr_conf.n_reg);
        p_msg->b_enable = x_tr_conf.x_fields.b_circuit_enable;
    }
    else
   #endif
    {
        AgNdRegT1E1PortConfiguration x_port;
        
        ag_nd_reg_read(p_device, AG_REG_T1_E1_PORT_CONFIGURATION(p_msg->n_circuit_id), &x_port.n_reg);
        p_msg->b_enable = (AG_BOOL)x_port.x_fields.b_port_enable;
    }

    return AG_S_OK;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_config_circuit_enable */
/* */
AgResult 
ag_nd_opcode_write_config_circuit_enable(AgNdDevice *p_device, AgNdMsgConfigCircuitEnable *p_msg)
{
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "circuit=%hhu\n", p_msg->n_circuit_id);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "enable=%lu\n", p_msg->b_enable);


#ifdef AG_ND_ENABLE_VALIDATION
    if (!p_device->p_circuit_is_valid_id(p_device, p_msg->n_circuit_id))
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif

   #ifndef CES16_BCM_VERSION
    if (AG_ND_ARCH_MASK_NEPTUNE & p_device->n_chip_mask)
    {
        AgNdRegTributaryCircuitConfiguration x_tr_conf;

        ag_nd_reg_read(p_device, AG_REG_TRIBUTARY_CIRCUIT_CONFIGURATION(p_msg->n_circuit_id), &x_tr_conf.n_reg);
        x_tr_conf.x_fields.b_circuit_enable = (field_t) p_msg->b_enable;
		/*/x_tr_conf.x_fields.b_tsx_link_rate_disable = AG_TRUE; */
        ag_nd_reg_write(p_device, AG_REG_TRIBUTARY_CIRCUIT_CONFIGURATION(p_msg->n_circuit_id), x_tr_conf.n_reg);
    }
    else
   #endif
    {
        AgNdRegT1E1PortConfiguration x_port;

        ag_nd_reg_read(p_device, AG_REG_T1_E1_PORT_CONFIGURATION(p_msg->n_circuit_id), &x_port.n_reg);
        x_port.x_fields.b_port_enable = (field_t) p_msg->b_enable;
        ag_nd_reg_write(p_device, AG_REG_T1_E1_PORT_CONFIGURATION(p_msg->n_circuit_id), x_port.n_reg);
    }

    return AG_S_OK;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_config_cas_data_replace */
/* */
AgResult 
ag_nd_opcode_read_config_cas_data_replace(
    AgNdDevice *p_device, 
    AgNdMsgConfigCasDataReplace *p_msg)
{
    AG_U16 n_hi;
    AG_U16 n_lo;

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_circuit_id=%hhu\n", p_msg->n_circuit_id);


#ifdef AG_ND_ENABLE_VALIDATION
    if (!p_device->p_circuit_is_valid_id(p_device, p_msg->n_circuit_id))
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif


    ag_nd_reg_read(
        p_device, 
        AG_REG_CAS_DATA_REPLACEMENT(p_msg->n_circuit_id, 0), 
        &n_lo);

    ag_nd_reg_read(
        p_device, 
        AG_REG_CAS_DATA_REPLACEMENT(p_msg->n_circuit_id, 1), 
        &n_hi);


    p_msg->n_cas_idle_timeslots = AG_ND_MAKE32LH(n_lo, n_hi);

    return AG_S_OK;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_config_cas_data_replace */
/* */
AgResult 
ag_nd_opcode_write_config_cas_data_replace(
    AgNdDevice *p_device, 
    AgNdMsgConfigCasDataReplace *p_msg)
{
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_circuit_id=%hhu\n", p_msg->n_circuit_id);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "idle=%lu\n", p_msg->n_cas_idle_timeslots);


#ifdef AG_ND_ENABLE_VALIDATION
    if (!p_device->p_circuit_is_valid_id(p_device, p_msg->n_circuit_id))
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif


    ag_nd_reg_write(
        p_device, 
        AG_REG_CAS_DATA_REPLACEMENT(p_msg->n_circuit_id, 0), 
        AG_ND_LOW16(p_msg->n_cas_idle_timeslots));

    ag_nd_reg_write(
        p_device, 
        AG_REG_CAS_DATA_REPLACEMENT(p_msg->n_circuit_id, 1), 
        AG_ND_HIGH16(p_msg->n_cas_idle_timeslots));


    return AG_S_OK;
}

