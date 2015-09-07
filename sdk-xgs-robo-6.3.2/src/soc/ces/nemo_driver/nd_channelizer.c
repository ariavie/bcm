/* $Id: nd_channelizer.c 1.8 Broadcom SDK $
 * Copyright 2011 BATM
 */

#include "nd_registers.h"
#include "nd_channelizer.h"
#include "nd_debug.h"
#include "nd_channel_egress.h"
#include "nd_channel_ingress.h"

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_config_timeslot */
/* */
AgResult 
ag_nd_opcode_read_config_timeslot(AgNdDevice *p_device, AgNdMsgConfigTimeslot *p_msg)
{
    AgNdRegCircuitToChannelMap  x_map;
    AG_U32                      a_addr[AG_ND_PATH_MAX];
    AgNdTsInfo                  *p_tsinfo;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);


#ifdef AG_ND_ENABLE_VALIDATION
    /* */
    /* sanity */
    /*  */
    if (!p_msg)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if (!p_device->p_circuit_is_valid_id(p_device, p_msg->x_id.n_circuit_id) || 
        p_msg->x_id.n_slot_idx >= AG_ND_SLOT_MAX ||
        p_msg->e_path >= AG_ND_PATH_MAX)

        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif


    a_addr[AG_ND_PATH_EGRESS] =  AG_REG_EGRESS_CIRCUIT_TO_CHANNEL_MAP(p_msg->x_id.n_circuit_id, p_msg->x_id.n_slot_idx);
    a_addr[AG_ND_PATH_INGRESS] =  AG_REG_INGRESS_CIRCUIT_TO_CHANNEL_MAP(p_msg->x_id.n_circuit_id, p_msg->x_id.n_slot_idx);


    ag_nd_reg_read(p_device, a_addr[p_msg->e_path], &(x_map.n_reg));
    
    p_msg->n_channel_id = x_map.x_fields.n_target_channel_id;
    p_msg->b_first = (AG_BOOL)x_map.x_fields.b_first_timeslot_in_packet;
    p_msg->b_enable = (AG_BOOL)x_map.x_fields.b_timeslot_enable;

    /* */
    /* check consistency with shadow structures (channel id) */
    /* */
    p_tsinfo = ag_nd_get_ts_info(
        p_device, 
        p_msg->x_id.n_circuit_id, 
        p_msg->x_id.n_slot_idx, 
        p_msg->e_path);

    assert(p_msg->n_channel_id == p_tsinfo->n_channel_id);
    /* return error if asserts are disabled */
    if (p_msg->n_channel_id != p_tsinfo->n_channel_id) {
        return AG_E_FAIL;
    }

    /* */
    /* check consistency with shadow structures (FTIP) */
    /* */
    assert((p_msg->b_first && AG_ND_TS_ISEQUAL(p_device->a_channel[p_msg->n_channel_id].a_channelizer[p_msg->e_path].x_first, p_msg->x_id)) ||
           (!p_msg->b_first && !AG_ND_TS_ISEQUAL(p_device->a_channel[p_msg->n_channel_id].a_channelizer[p_msg->e_path].x_first, p_msg->x_id)));


    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_config_timeslot */
/* */
AgResult 
ag_nd_opcode_write_config_timeslot(AgNdDevice *p_device, AgNdMsgConfigTimeslot *p_msg)
{
    AgNdRegCircuitToChannelMap   x_map;
    AG_U32                       a_addr[AG_ND_PATH_MAX];
    AgNdTsInfo                   *p_tsinfo;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_enabled=%lu\n", p_msg->b_enable);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_first=%lu\n", p_msg->b_first);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "e_path=%d\n", p_msg->e_path);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_channel_id=%hu\n", p_msg->n_channel_id);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_circuit_id=%hhu\n", p_msg->x_id.n_circuit_id);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_slot_idx=%hhu\n", p_msg->x_id.n_slot_idx);


    a_addr[AG_ND_PATH_EGRESS] =  AG_REG_EGRESS_CIRCUIT_TO_CHANNEL_MAP(p_msg->x_id.n_circuit_id, p_msg->x_id.n_slot_idx);
    a_addr[AG_ND_PATH_INGRESS] =  AG_REG_INGRESS_CIRCUIT_TO_CHANNEL_MAP(p_msg->x_id.n_circuit_id, p_msg->x_id.n_slot_idx);


#ifdef AG_ND_ENABLE_VALIDATION
    /* */
    /* parameters sanity check */
    /* */
    
    if (!p_msg)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if (p_msg->n_channel_id >= p_device->n_pw_max ||
        !p_device->p_circuit_is_valid_id(p_device, p_msg->x_id.n_circuit_id) ||
        p_msg->x_id.n_slot_idx > AG_ND_SLOT_MAX ||
        p_msg->e_path >= AG_ND_PATH_MAX)

        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    
    /* */
    /* parameters check for FTIP violations */
    /* */
    if ((AG_ND_PATH_EGRESS == p_msg->e_path && ag_nd_channel_egress_is_enabled(p_device, p_msg->n_channel_id)) ||
        (AG_ND_PATH_INGRESS == p_msg->e_path && ag_nd_channel_ingress_is_enabled(p_device, p_msg->n_channel_id)))
    {
        /* */
        /* FTIP can'n be changed for enabled channel */
        /* */
        if (p_msg->b_first && 
            AG_ND_TS_ISEQUAL(p_device->a_channel[p_msg->n_channel_id].a_channelizer[p_msg->e_path].x_first, p_msg->x_id))
            return AG_ND_ERR(p_device, AG_E_ND_FTIP);

        if (!p_msg->b_first && 
            AG_ND_TS_ISEQUAL(p_device->a_channel[p_msg->n_channel_id].a_channelizer[p_msg->e_path].x_first, p_msg->x_id))
            return AG_ND_ERR(p_device, AG_E_ND_FTIP);
    }
    else
    {
        /* */
        /* avoid situation when two ts are marked as FTIP for disabled channel. */
        /* FTIP indication should be cleared first */
        /* */
        if (p_msg->b_first &&
            !AG_ND_TS_ISEQUAL(p_device->a_channel[p_msg->n_channel_id].a_channelizer[p_msg->e_path].x_first, p_msg->x_id) &&
            AG_ND_TS_ISVALID(p_device->a_channel[p_msg->n_channel_id].a_channelizer[p_msg->e_path].x_first))

            return AG_ND_ERR(p_device, AG_E_ND_FTIP);
    }
#endif

    /* */
    /* update hardware */
    /* */
    x_map.n_reg = 0;
    x_map.x_fields.b_first_timeslot_in_packet = (field_t) p_msg->b_first;
    x_map.x_fields.n_target_channel_id = (field_t) p_msg->n_channel_id;
    x_map.x_fields.b_timeslot_enable = (field_t) p_msg->b_enable;

    ag_nd_reg_write(p_device, a_addr[p_msg->e_path], x_map.n_reg);
    

    /* */
    /* update "shadow" structures */
    /* */
    p_tsinfo = ag_nd_get_ts_info(
        p_device, 
        p_msg->x_id.n_circuit_id, 
        p_msg->x_id.n_slot_idx, 
        p_msg->e_path);


    if (p_msg->n_channel_id != p_tsinfo->n_channel_id ||
        p_msg->n_channel_id != p_tsinfo->b_enable)
    {
        /* */
        /* update ts info table entry for this ts */
        /* */
        p_tsinfo->n_channel_id = p_msg->n_channel_id;
        p_tsinfo->b_enable = p_msg->b_enable;
        
        /* */
        /* move ts to appropriate list:  */
        /* to the channel's timeslots list if the timeslot is being enabled or to */
        /* per channelizer disabled timeslots list otherwise */
        /* */
        ag_nd_list_del(&p_tsinfo->x_list);

        if (p_msg->b_enable)
            ag_nd_list_add_tail(&p_device->a_channel[p_msg->n_channel_id].a_channelizer[p_msg->e_path].a_ts_list,
                                &p_tsinfo->x_list);
        else
            ag_nd_list_add_tail(&p_device->a_ts_list[p_msg->e_path], 
                                &p_tsinfo->x_list);
    }

    /* */
    /* update channel's FTIP indication */
    /* */
    if (p_msg->b_first)
        p_device->a_channel[p_msg->n_channel_id].a_channelizer[p_msg->e_path].x_first = p_msg->x_id;
    else
        if (AG_ND_TS_ISEQUAL(p_device->a_channel[p_msg->n_channel_id].a_channelizer[p_msg->e_path].x_first, p_msg->x_id))
            AG_ND_TS_INVALIDATE(p_device->a_channel[p_msg->n_channel_id].a_channelizer[p_msg->e_path].x_first);
            

    return AG_S_OK;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_config_channelizer */
/* */
AgResult 
ag_nd_opcode_read_config_channelizer(AgNdDevice *p_device, AgNdMsgConfigChannelizer *p_msg)
{
    AgNdList                    *p_entry;
    AgNdTsInfo                  *p_tsinfo;
    AgNdChannelizer             *p_chinfo;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);


#ifdef AG_ND_ENABLE_VALIDATION
    /* */
    /* sanity */
    /* */
    if (p_msg->n_channel_id >= p_device->n_pw_max ||
        p_msg->e_path >= AG_ND_PATH_MAX)

        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif


    /* */
    /* iterate over the channels's list of enabled timeslots */
    /* */
    p_msg->x_map.n_size = 0;
    p_chinfo = &p_device->a_channel[p_msg->n_channel_id].a_channelizer[p_msg->e_path];

    AG_ND_LIST_FOREACH(p_entry, &p_chinfo->a_ts_list)
    {
        p_tsinfo = AG_ND_LIST_ENTRY(p_entry, AgNdTsInfo, x_list);
        p_msg->x_map.a_ts[p_msg->x_map.n_size] = p_tsinfo->x_id;

        if (AG_ND_TS_ISEQUAL(p_tsinfo->x_id, p_chinfo->x_first))
            p_msg->x_map.n_first = p_msg->x_map.n_size;

        p_msg->x_map.n_size++;
    }

    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_config_channelizer */
/* */
AgResult 
ag_nd_opcode_write_config_channelizer(AgNdDevice *p_device, AgNdMsgConfigChannelizer *p_msg)
{
    AgNdList                    *p_entry;
    AgNdTsInfo                  *p_tsinfo;
    AgNdChannelizer             *p_chinfo;
    AG_U32                      i;
    AG_U32                      n_addr;
    AgNdRegCircuitToChannelMap  x_map;
    AgNdList                    *p_aux;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "p_msg->e_path=%d\n", p_msg->e_path);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "p_msg->n_channel_id=%hu\n", p_msg->n_channel_id);


#ifdef AG_ND_ENABLE_VALIDATION
    /* */
    /* sanity */
    /* */
    if (p_msg->e_path >= AG_ND_PATH_MAX ||
        p_msg->n_channel_id >= p_device->n_pw_max  ||
        p_msg->x_map.n_size > p_device->p_circuit_get_max_idx(p_device) * AG_ND_SLOT_MAX ||
        p_msg->x_map.n_first >= p_msg->x_map.n_size)

        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if ((AG_ND_PATH_EGRESS == p_msg->e_path && ag_nd_channel_egress_is_enabled(p_device, p_msg->n_channel_id)) ||
        (AG_ND_PATH_INGRESS == p_msg->e_path && ag_nd_channel_ingress_is_enabled(p_device, p_msg->n_channel_id)))

        return AG_ND_ERR(p_device, AG_E_ND_ENABLE);
#endif


    /* */
    /* for all currently enabled timeslots of this channel: disable */
    /* */
    p_chinfo = &p_device->a_channel[p_msg->n_channel_id].a_channelizer[p_msg->e_path];

    AG_ND_LIST_FOREACH_SAFE(p_entry, p_aux, &p_chinfo->a_ts_list)
    {
        p_tsinfo = AG_ND_LIST_ENTRY(p_entry, AgNdTsInfo, x_list);

        /* */
        /* timeslot register address */
        /* */
        if (AG_ND_PATH_EGRESS == p_msg->e_path)
            n_addr = AG_REG_EGRESS_CIRCUIT_TO_CHANNEL_MAP(
                p_tsinfo->x_id.n_circuit_id, 
                p_tsinfo->x_id.n_slot_idx);
        else
            n_addr = AG_REG_INGRESS_CIRCUIT_TO_CHANNEL_MAP(
                p_tsinfo->x_id.n_circuit_id, 
                p_tsinfo->x_id.n_slot_idx);

        /* */
        /* disable timeslot and clear FTIP indication */
        /* */
        x_map.n_reg = 0;
        x_map.x_fields.b_first_timeslot_in_packet = AG_FALSE;
        x_map.x_fields.b_timeslot_enable = AG_FALSE;
        x_map.x_fields.n_target_channel_id = 0;

        ag_nd_reg_write(p_device, n_addr, x_map.n_reg);

        /* */
        /* update shadow structure */
        /* */
        p_tsinfo->b_enable = AG_FALSE;

        /* */
        /* move the timeslot from the list of enabled timeslots to the list of disabled */
        /* */
        ag_nd_list_del(&p_tsinfo->x_list);
        ag_nd_list_add_tail(&p_device->a_ts_list[p_msg->e_path], &p_tsinfo->x_list);
    }


    /* */
    /* enable the new list of timeslots */
    /* */
    for (i = 0; i < p_msg->x_map.n_size; i++)
    {
        p_tsinfo = ag_nd_get_ts_info(
            p_device, 
            p_msg->x_map.a_ts[i].n_circuit_id,
            p_msg->x_map.a_ts[i].n_slot_idx,
            p_msg->e_path);

        /* */
        /* timeslot register address */
        /* */
        if (AG_ND_PATH_EGRESS == p_msg->e_path)
            n_addr = AG_REG_EGRESS_CIRCUIT_TO_CHANNEL_MAP(
                p_tsinfo->x_id.n_circuit_id, 
                p_tsinfo->x_id.n_slot_idx);
        else
            n_addr = AG_REG_INGRESS_CIRCUIT_TO_CHANNEL_MAP(
                p_tsinfo->x_id.n_circuit_id, 
                p_tsinfo->x_id.n_slot_idx);

        /* */
        /* enable the timeslot and set FTIP if required */
        /* */
        x_map.n_reg = 0;
        x_map.x_fields.b_first_timeslot_in_packet = (i == p_msg->x_map.n_first);
        x_map.x_fields.b_timeslot_enable = AG_TRUE;
        x_map.x_fields.n_target_channel_id = p_msg->n_channel_id;

        ag_nd_reg_write(p_device, n_addr, x_map.n_reg);

        /* */
        /* update shadow structure */
        /* */
        p_tsinfo->b_enable = AG_TRUE;
        p_tsinfo->n_channel_id = p_msg->n_channel_id;

        /* */
        /* delete the timeslot from its current list */
        /* (it can be assigned to another channel and even to be its FTIP -  */
        /* there is no requirement to check) */
        /* */
        ag_nd_list_del(&p_tsinfo->x_list);

        /* */
        /* move the timeslot to the list of enabled */
        /* */
        ag_nd_list_add_tail(&p_chinfo->a_ts_list, &p_tsinfo->x_list);
    }


    /* */
    /* update shadow FTIP */
    /* */
    p_chinfo->x_first = p_msg->x_map.a_ts[p_msg->x_map.n_first];


    /* CAS TEMP: remove if when implemented in Nemo */
    if (1 /*AG_ND_ARCH_MASK_NEPTUNE == p_device->n_chip_mask*/)  /*BCM as neptune, cas packets do not bring to cpu*/
    {
        /* */
        /* configure CAS */
        /*  */
        if (AG_ND_PATH_EGRESS == p_msg->e_path)
        {
            AgNdRegJbfCasConfiguration x_jbf_cas;
    
            x_jbf_cas.n_reg = 0;
            x_jbf_cas.x_fields.n_packet_payload_size = (field_t) p_msg->x_map.n_size;
    
            ag_nd_reg_write(
                p_device, 
                AG_REG_JITTER_BUFFER_CAS_CONFIGURATION(p_msg->n_channel_id), 
                x_jbf_cas.n_reg);
        }
        else
        {
            AgNdRegPbfCasConfiguration x_pbf_cas;
    
            x_pbf_cas.n_reg = 0;
            x_pbf_cas.x_fields.n_packet_payload_size = (field_t) p_msg->x_map.n_size;
    
            ag_nd_reg_write(
                p_device, 
                AG_REG_PBF_CAS_CONFIGURATION(p_msg->n_channel_id), 
                x_pbf_cas.n_reg);
        }
    }

    return AG_S_OK;
}

