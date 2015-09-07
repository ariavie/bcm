/* $Id: nd_cas.c,v 1.5 Broadcom SDK $
 * Copyright 2011 BATM
 */

#include "nd_debug.h"
#include "nd_cas.h"

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_config_cas_channel_enable */
/*  */
AgResult 
ag_nd_opcode_read_config_cas_channel_enable(
    AgNdDevice *p_device, 
    AgNdMsgConfigCasChannelEnable *p_msg)
{
    AgNdRegPbfCasConfiguration x_cfg;

    /* CAS TEMP: remove if when implemented in Nemo */
    if ( 0 /*AG_ND_ARCH_MASK_NEPTUNE != p_device->n_chip_mask*/)   /*BCM as neptune, cas packets do not bring to cpu*/
    {
        return AG_ND_ERR(p_device, AG_E_ND_UNSUPPORTED);
    }

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);

#ifdef AG_ND_ENABLE_VALIDATION
    if (p_msg->n_channel_id >= p_device->n_pw_max)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif

    ag_nd_reg_read(p_device, AG_REG_PBF_CAS_CONFIGURATION(p_msg->n_channel_id), &(x_cfg.n_reg));
    p_msg->b_enable = x_cfg.x_fields.b_signaling_enable;

    return AG_S_OK;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_config_cas_channel_enable */
/*  */
AgResult 
ag_nd_opcode_write_config_cas_channel_enable(
    AgNdDevice *p_device, 
    AgNdMsgConfigCasChannelEnable *p_msg)
{
    AgNdRegPbfCasConfiguration x_cfg;

    /* CAS TEMP: remove if when implemented in Nemo */
    if (0 /*AG_ND_ARCH_MASK_NEPTUNE != p_device->n_chip_mask*/)
    {
        return AG_ND_ERR(p_device, AG_E_ND_UNSUPPORTED);
    }

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_channel_id=%hu\n", p_msg->n_channel_id);


#ifdef AG_ND_ENABLE_VALIDATION
    if (p_msg->n_channel_id >= p_device->n_pw_max)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif


    ag_nd_reg_read(p_device, AG_REG_PBF_CAS_CONFIGURATION(p_msg->n_channel_id), &(x_cfg.n_reg));


#ifdef AG_ND_ENABLE_VALIDATION
    if (AG_TRUE == x_cfg.x_fields.b_signaling_enable && AG_TRUE == p_msg->b_enable)
        return AG_ND_ERR(p_device, AG_E_ND_ENABLE);
#endif


    x_cfg.x_fields.b_signaling_enable = p_msg->b_enable;
    ag_nd_reg_write(p_device, AG_REG_PBF_CAS_CONFIGURATION(p_msg->n_channel_id), x_cfg.n_reg);

    return AG_S_OK;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_config_cas_channel_ingress */
/*  */
AgResult 
ag_nd_opcode_write_config_cas_channel_ingress(
    AgNdDevice *p_device, 
    AgNdMsgConfigCasChannelIngress *p_msg)
{
    AgNdRegPbfCasConfiguration                      x_cfg;
   #ifndef CES16_BCM_VERSION 
    AgNdRegTransmitCasHeaderAccessControl           x_tx_hdr_ac;
   #endif 
    AgNdRegTransmitCasHeaderConfiguration           x_tx_hdr_cfg;
    AgNdChannelIngressInfo                          x_channel_info;

    AgNdBitwise         x_bw;
    AG_U32              n_hdr_block_size;
    AG_U32              n_hdr_template_size;


    /* CAS TEMP: remove if when implemented in Nemo */
    if (0 /*AG_ND_ARCH_MASK_NEPTUNE != p_device->n_chip_mask*/)
    {
        return AG_ND_ERR(p_device, AG_E_ND_UNSUPPORTED);
    }

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_channel_id=%hu\n", p_msg->n_channel_id);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_cas_sqn=%hu\n", p_msg->n_cas_sqn);

    

#ifdef AG_ND_ENABLE_VALIDATION
    /* */
    /* sanity */
    /*  */
    if (p_msg->n_channel_id >= p_device->n_pw_max)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif

    ag_nd_reg_read(p_device, AG_REG_PBF_CAS_CONFIGURATION(p_msg->n_channel_id), &(x_cfg.n_reg));

#ifdef AG_ND_ENABLE_VALIDATION
    if (AG_TRUE == x_cfg.x_fields.b_signaling_enable)
        return AG_ND_ERR(p_device, AG_E_ND_ENABLE);
#endif

    /* */
    /* build header */
    /* */
    x_bw.p_buf = p_device->a_buf;
    x_bw.p_addr = p_device->a_buf;
    x_bw.n_buf_size = sizeof(p_device->a_buf);
    x_bw.n_offset = 0;

    ag_nd_header_build(
        &(p_msg->x_header),
        &x_bw,
        &(x_channel_info.x_tx_hdr_fmt),
        &(x_channel_info.n_udp_chksum),
        &(x_channel_info.n_udp_len),
        &(x_channel_info.n_ip_chksum),
        &(x_channel_info.n_ip_len));

    x_channel_info.b_dba = AG_FALSE;
    x_channel_info.b_mef_len_support = AG_FALSE;
    x_channel_info.n_cw = 0;
    x_channel_info.n_payload_size = x_cfg.x_fields.n_packet_payload_size * 4; /* timeslots -> bytes */

    /* */
    /* check if the total header size (descriptor and header template) doesn't */
    /* exceeds the currently configured cas header block size */
    /* */
   #ifndef CES16_BCM_VERSION  /*FIXcas not needed at all*/
    ag_nd_reg_read(p_device, AG_REG_TRANSMIT_CAS_HEADER_ACCESS_CONTROL, &(x_tx_hdr_ac.n_reg));
    n_hdr_block_size = 1 << (x_tx_hdr_ac.x_fields.n_header_memory_block_size + 5);
    n_hdr_template_size = x_bw.p_addr - x_bw.p_buf;
    assert(sizeof(AgNdRegTransmitHeaderFormatDescriptor) + n_hdr_template_size <= n_hdr_block_size);
   #else
     n_hdr_block_size = 128;
     n_hdr_template_size = x_bw.p_addr - x_bw.p_buf;
   #endif
    /* */
    /* store descriptor */
    /*  */
    ag_nd_mem_write(
        p_device, 
        p_device->p_mem_cas_header, 
        p_msg->n_channel_id * n_hdr_block_size, 
        &(x_channel_info.x_tx_hdr_fmt.n_reg),
        2);

    /* */
    /* store header template  */
    /* */
    ag_nd_mem_write(
        p_device,
        p_device->p_mem_cas_header,
        p_msg->n_channel_id * n_hdr_block_size + 4, 
        x_bw.p_buf,
        ag_nd_round_up2(n_hdr_template_size, 1) >> 1);


    /* */
    /* update header template with correct ip/udp checksum and length field values */
    /* based on CES CW and payload size values */
    /*  */
    ag_nd_header_update_cw(
        p_device,
        p_device->p_mem_cas_header, 
        p_msg->n_channel_id * n_hdr_block_size,
        AG_ND_CES_CW_M,  
        AG_ND_CES_CW_M,  
        &x_channel_info);


    /* */
    /* configure initial SQN */
    /*  */
    ag_nd_reg_write(p_device,
                    AG_REG_CAS_OUT_SEQUENCE_NUMBER(p_msg->n_channel_id),
                    p_msg->n_cas_sqn);


    x_tx_hdr_cfg.n_reg = 0;
    x_tx_hdr_cfg.x_fields.n_header_memory_size = n_hdr_template_size; 
    x_tx_hdr_cfg.x_fields.n_header_memory_size += sizeof(AgNdRegTransmitHeaderFormatDescriptor);
    ag_nd_reg_write(p_device, AG_REG_TRANSMIT_CAS_HEADER_CONFIGURATION(p_msg->n_channel_id), x_tx_hdr_cfg.n_reg);

    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_config_cas_channel_ingress */
/*  */
AgResult 
ag_nd_opcode_write_command_cas_channel_tx(
    AgNdDevice *p_device, 
    AgNdMsgCommandCasChannelTx *p_msg)
{
    AgNdRegPbfCasBufferStatus x_stat;

    /* CAS TEMP: remove if when implemented in Nemo */
    if (0 /*AG_ND_ARCH_MASK_NEPTUNE != p_device->n_chip_mask*/)
    {
        return AG_ND_ERR(p_device, AG_E_ND_UNSUPPORTED);
    }

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_channel_id=%hu\n", p_msg->n_channel_id);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_packets=%lu\n", p_msg->n_packets);


#ifdef AG_ND_ENABLE_VALIDATION
    if (p_msg->n_channel_id >= p_device->n_pw_max)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if (p_msg->n_packets > 3)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif


    x_stat.n_reg = 0;
    x_stat.x_fields.n_cas_state = p_msg->n_packets;
    ag_nd_reg_write(p_device, AG_REG_PAYLOAD_CAS_BUFFER_STATUS(p_msg->n_channel_id), x_stat.n_reg);

    return AG_S_OK;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_status_cas_data */
/*  */
AgResult 
ag_nd_opcode_read_status_cas_data(
    AgNdDevice *p_device, 
    AgNdMsgStatusCasData *p_msg)
{
  #ifndef CES16_BCM_VERSION
    AG_U32 i, n_ts_idx;
    AG_U16 n_reg;
    AG_U32 n_addr;

    /* CAS TEMP: remove if when implemented in Nemo */
    if (0 /*AG_ND_ARCH_MASK_NEPTUNE != p_device->n_chip_mask*/)
    {
        return AG_ND_ERR(p_device, AG_E_ND_UNSUPPORTED);
    }

    AG_ND_TRACE(p_device, AG_ND_TRACE_PERIODIC, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);


#ifdef AG_ND_ENABLE_VALIDATION
    if (!p_device->p_circuit_is_valid_id(p_device, p_msg->n_circuit_id))
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif


    switch(p_msg->e_path)
    {
    case AG_ND_PATH_INGRESS:
        n_addr = AG_REG_CAS_INGRESS_DATA_BUFFER(p_msg->n_circuit_id, 0);
        break;

    case AG_ND_PATH_EGRESS:
        n_addr = AG_REG_CAS_EGRESS_DATA_BUFFER(p_msg->n_circuit_id, 0);
        break;

    default:
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
    }


    for (i = 0, n_ts_idx = 0; i < AG_ND_SLOT_MAX/4; i++)
    {
        ag_nd_reg_read(p_device, n_addr, &n_reg);

        p_msg->a_abcd[n_ts_idx++] = n_reg & 0xf;
        p_msg->a_abcd[n_ts_idx++] = (n_reg>>4) & 0xf;
        p_msg->a_abcd[n_ts_idx++] = (n_reg>>8) & 0xf;
        p_msg->a_abcd[n_ts_idx++] = (n_reg>>12) & 0xf;

        n_addr += 2;
    }
  #endif
    return AG_S_OK;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_status_cas_change */
/*  */
AgResult 
ag_nd_opcode_read_status_cas_change(
    AgNdDevice *p_device, 
    AgNdMsgStatusCasChange *p_msg)
{
    AG_U16 n_hi, n_lo;

    /* CAS TEMP: remove if when implemented in Nemo */
    if ( 0 /*AG_ND_ARCH_MASK_NEPTUNE != p_device->n_chip_mask*/)
    {
        return AG_ND_ERR(p_device, AG_E_ND_UNSUPPORTED);
    }

    AG_ND_TRACE(p_device, AG_ND_TRACE_PERIODIC, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);


#ifdef AG_ND_ENABLE_VALIDATION
    if (!p_device->p_circuit_is_valid_id(p_device, p_msg->n_circuit_id))
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif

    
    ag_nd_reg_read(p_device, AG_REG_CAS_INGRESS_CHANGE_DETECTION(p_msg->n_circuit_id, 0), &n_lo);
    ag_nd_reg_read(p_device, AG_REG_CAS_INGRESS_CHANGE_DETECTION(p_msg->n_circuit_id, 0), &n_hi);

    p_msg->n_changed = AG_ND_MAKE32LH(n_lo, n_hi);


    return AG_S_OK;
}


