/* $Id: nd_channel_ingress.c,v 1.7 Broadcom SDK $
 * Copyright 2011 BATM
 */

#include "nd_channel_ingress.h"
#include "nd_registers.h"
#include "nd_debug.h"
#include "nd_util.h"
#include "nd_protocols.h"



/*BCM. define next debug, to check correct writing of ingress haeder*/
/*#define DEBUG_CHECK_INGRESS */
AgResult 
ag_nd_opcode_read_config_channel_ingress(
    AgNdDevice *p_device, 
    AgNdMsgConfigChannelIngress *p_msg)
{
    AgNdRegPayloadBufferChannelConfiguration        x_pbf_cfg;
    AgNdRegTransmitHeaderConfiguration              x_tx_hdr_cfg;
    AgNdRegChannelControl                           x_chi_ctl;
    AG_U16                                          n_pbf_base = 0;
    AG_U16                                          n_sqn;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);


#ifdef AG_ND_ENABLE_VALIDATION
    /* */
    /* sanity */
    /*  */
    if (p_msg->n_channel_id >= p_device->n_pw_max)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif

   ag_nd_set_kiss_debug(AG_TRUE);  
    /* */
    /* read configurations */
    /* */
    ag_nd_reg_read(p_device,
                   AG_REG_PAYLOAD_BUFFER_CHANNEL_CONFIGURATION(p_msg->n_channel_id),
                   &x_pbf_cfg.n_reg);

    ag_nd_reg_read(p_device,
                   AG_REG_TRANSMIT_HEADER_CONFIGURATION(p_msg->n_channel_id),
                   &x_tx_hdr_cfg.n_reg);
   #ifndef CES16_BCM_VERSION

    ag_nd_reg_read(p_device,
                   AG_REG_PAYLOAD_BUFFER_QUEUE_BASE_ADDRESS(p_msg->n_channel_id), 
                   &n_pbf_base);
   #endif /*CES16_BCM_VERSION*/

    ag_nd_reg_read(p_device, 
                   AG_REG_INGRESS_CHANNEL_CONTROL(p_msg->n_channel_id), 
                   &(x_chi_ctl.n_reg));

    ag_nd_reg_read(p_device, 
                   AG_REG_PW_OUT_SEQUENCE_NUMBER(p_msg->n_channel_id), 
                   &n_sqn);

    ag_nd_set_kiss_debug(AG_FALSE);  

    p_msg->n_payload_size = x_pbf_cfg.x_fields.n_packet_payload_size;
    p_msg->n_pbf_size = 1 << (x_pbf_cfg.x_fields.n_payload_buffer_queue_size + 6);
    p_msg->n_pbf_addr = ((AG_U32)n_pbf_base << 6) + p_device->n_base + 0x800000;

    p_msg->n_ces_sqn = n_sqn;

    p_msg->b_enable = x_chi_ctl.x_fields.b_channel_enable;
    assert(x_chi_ctl.x_fields.b_channel_enable == p_device->a_channel[p_msg->n_channel_id].a_enable[AG_ND_PATH_INGRESS]);

    p_msg->b_dba = p_device->a_channel_ingress[p_msg->n_channel_id].b_dba;
    p_msg->b_auto_r_bit = x_tx_hdr_cfg.x_fields.b_auto_r_bit_insertion;
    p_msg->b_mef_len_support = p_device->a_channel_ingress[p_msg->n_channel_id].b_mef_len_support;

    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_config_channel_ingress */
/* */
AgResult 
ag_nd_opcode_write_config_channel_ingress(
    AgNdDevice *p_device, 
    AgNdMsgConfigChannelIngress *p_msg)
{
    AgNdRegPayloadBufferChannelConfiguration        x_pbf_cfg;
    AgNdRegTransmitHeaderConfiguration              x_tx_hdr_cfg;
    /*AgNdRegTransmitHeaderAccessControl              x_tx_hdr_ac; BCMnot used*/

    AgNdBitwise                                     x_bw;
    AG_U32                                          n_hdr_template_size;

    AG_U32                                          n_header_template_idx;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_channel_id=%hu\n", p_msg->n_channel_id);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_dba=%lu\n", p_msg->b_dba);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_auto_r_bit=%lu\n", p_msg->b_auto_r_bit);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_payload_size=%lu\n", p_msg->n_payload_size);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_pbf_size=%lu\n", p_msg->n_pbf_size);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_ces_sqn=%lu\n", p_msg->n_ces_sqn);


#ifdef AG_ND_ENABLE_VALIDATION
    /* */
    /* sanity */
    /*  */
    if (p_msg->n_channel_id >= p_device->n_pw_max)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if (ag_nd_channel_ingress_is_enabled(p_device, p_msg->n_channel_id))
        return AG_ND_ERR(p_device, AG_E_ND_ENABLE);

    if (p_msg->n_payload_size > 1500)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if (p_msg->n_pbf_size < p_msg->n_payload_size)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if (p_msg->n_pbf_size < 64 ||
        p_msg->n_pbf_size > 8192 ||
        !ag_nd_is_power_of_2(p_msg->n_pbf_size))
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if ((p_msg->b_mef_len_support == AG_TRUE)
        && (p_msg->x_header.e_encapsulation != AG_ND_ENCAPSULATION_ETH))
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif

   ag_nd_set_kiss_debug(AG_TRUE); 

    /* */
    /* configure PBF */
    /* */
    if (!p_msg->b_redundancy_config)
    {
	    x_pbf_cfg.n_reg = 0;
	    x_pbf_cfg.x_fields.b_interrupt_enable = (field_t) (NULL != p_device->p_cb_pbi);
	    x_pbf_cfg.x_fields.b_packet_redundancy_enable = (field_t) ((p_msg->b_redundancy_enabled) ? AG_TRUE : AG_FALSE);
	    x_pbf_cfg.x_fields.n_packet_payload_size = (field_t) p_msg->n_payload_size;
      #ifndef CES16_BCM_VERSION /*Buf queue size removed*/
	    x_pbf_cfg.x_fields.n_payload_buffer_queue_size = (field_t) (ag_nd_log2(p_msg->n_pbf_size) - 6);
      #endif
	    ag_nd_reg_write(p_device, 
	                    AG_REG_PAYLOAD_BUFFER_CHANNEL_CONFIGURATION(p_msg->n_channel_id), 
	                    x_pbf_cfg.n_reg);
    }

    /* */
    /* store payload size */
    /*  */
    p_device->a_channel_ingress[p_msg->n_channel_id].n_payload_size = p_msg->n_payload_size;

    /* */
    /* store DBA mode */
    /* */
    p_device->a_channel_ingress[p_msg->n_channel_id].b_dba = p_msg->b_dba;


    /* */
    /* set auto R bit insertion mode  */
    /*  */
    x_tx_hdr_cfg.n_reg = 0;
    x_tx_hdr_cfg.x_fields.b_auto_r_bit_insertion = p_msg->b_auto_r_bit;
    ag_nd_reg_write(p_device,
                    AG_REG_TRANSMIT_HEADER_CONFIGURATION(p_msg->n_channel_id),
                    x_tx_hdr_cfg.n_reg);
    

    /* */
    /* store MEF support mode */
    /* */
    p_device->a_channel_ingress[p_msg->n_channel_id].b_mef_len_support = p_msg->b_mef_len_support;
	

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
        &(p_device->a_channel_ingress[p_msg->n_channel_id].x_tx_hdr_fmt),
        &(p_device->a_channel_ingress[p_msg->n_channel_id].n_udp_chksum),
        &(p_device->a_channel_ingress[p_msg->n_channel_id].n_udp_len),
        &(p_device->a_channel_ingress[p_msg->n_channel_id].n_ip_chksum),
        &(p_device->a_channel_ingress[p_msg->n_channel_id].n_ip_len));
    
    #ifdef DEBUG_CHECK_INGRESS
    if( x_bw.p_buf[4]==0 && x_bw.p_buf[5]==0 && x_bw.p_buf[10]==0 &&  x_bw.p_buf[11]==0)
    {
    
        fprintf(stderr,"\n\r  build %02x-%02x-%02x-%02x",x_bw.p_buf[4] ,x_bw.p_buf[5],x_bw.p_buf[10] ,x_bw.p_buf[11]);
        return AG_E_FAIL;
    }
   #endif
   
    #if 0 /*def DEBUG_CHECK_INGRESS*/
    if(  x_bw.p_buf[10]==0x99 &&  x_bw.p_buf[11]==0x99)
    {
       int loop;
       for (loop=0;loop<52;loop+=4)
       {
           fprintf(stderr,"\n\r%02x  %02x-%02x-%02x-%02x",loop,
                     x_bw.p_buf[loop],x_bw.p_buf[loop+1],x_bw.p_buf[loop+2],x_bw.p_buf[loop+3]);
       }
    }
    #endif


    /* */
    /* store current CW value */
    /*  */
    p_device->a_channel_ingress[p_msg->n_channel_id].n_cw = 0;

    /* */
    /* check if the total header size (descriptor and header template) doesn't */
    /* exceeds the currently configured header block size */
    /* */
    n_hdr_template_size = x_bw.p_addr - x_bw.p_buf;
    assert(sizeof(AgNdRegTransmitHeaderFormatDescriptor) + n_hdr_template_size <= p_device->n_data_header_memory_block_size);

    /* */
    /* loop over primary and alternate header templates of the channel */
    /* */
    for (n_header_template_idx = 0; n_header_template_idx < 2; n_header_template_idx++)
    {
        /* */
        /* store descriptor */
        /*  */
       #ifdef DEBUG_CHECK_INGRESS
        if( x_bw.p_buf[4]==0 && x_bw.p_buf[5]==0 && x_bw.p_buf[10]==0 && x_bw.p_buf[11]==0)
        {
        
             fprintf(stderr,"\n\r  11111 %02x-%02x-%02x-%02x",x_bw.p_buf[4] ,x_bw.p_buf[5],x_bw.p_buf[10] ,x_bw.p_buf[11]);
             return AG_E_FAIL | 1;
        }
      #endif
        ag_nd_mem_write(
            p_device, 
            p_device->p_mem_data_header, 
            (p_msg->n_channel_id * 2 + n_header_template_idx) * p_device->n_data_header_memory_block_size, 
            &(p_device->a_channel_ingress[p_msg->n_channel_id].x_tx_hdr_fmt.n_reg),
            2);

        /* */
        /* store header template  */
        /* */
        #ifdef DEBUG_CHECK_INGRESS

        if( x_bw.p_buf[4]==0 && x_bw.p_buf[5]==0 && x_bw.p_buf[10]==0 &&  x_bw.p_buf[11]==0)
        {
        
        fprintf(stderr,"\n\r  22222 %02x-%02x-%02x-%02x",x_bw.p_buf[4] ,x_bw.p_buf[5],x_bw.p_buf[10] ,x_bw.p_buf[11]);
          return AG_E_FAIL | 2;
        }
       #endif
        ag_nd_mem_write(
            p_device,
            p_device->p_mem_data_header,
            (p_msg->n_channel_id * 2 + n_header_template_idx) * p_device->n_data_header_memory_block_size + 4, 
            x_bw.p_buf,
            ag_nd_round_up2(n_hdr_template_size, 1) >> 1);

        /* */
        /* update header template with correct ip/udp checksum and length field values */
        /* based on CES CW and payload size values */
        /*  */
        #ifdef DEBUG_CHECK_INGRESS
        if( x_bw.p_buf[4]==0 && x_bw.p_buf[5]==0 && x_bw.p_buf[10]==0 &&  x_bw.p_buf[11]==0)
        {
        
            fprintf(stderr,"\n\r  33333 %02x-%02x-%02x-%02x",x_bw.p_buf[4] ,x_bw.p_buf[5],x_bw.p_buf[10] ,x_bw.p_buf[11]);
            return AG_E_FAIL | 3;
         }
        #endif
        ag_nd_header_update_cw(
            p_device,
            p_device->p_mem_data_header, 
            (p_msg->n_channel_id * 2 + n_header_template_idx) * p_device->n_data_header_memory_block_size,
            0,  
            0,  
            &(p_device->a_channel_ingress[p_msg->n_channel_id]));

        #ifdef DEBUG_CHECK_INGRESS
        if( x_bw.p_buf[4]==0 && x_bw.p_buf[5]==0 && x_bw.p_buf[10]==0 &&  x_bw.p_buf[11]==0)
         {

          fprintf(stderr,"\n\r  4444 %02x-%02x-%02x-%02x",x_bw.p_buf[4] ,x_bw.p_buf[5],x_bw.p_buf[10] ,x_bw.p_buf[11]);
          return AG_E_FAIL | 4;
         }
       #endif

    }
        

    /* */
    /* update HW with header configuration, select primary header */
    /* */
    x_tx_hdr_cfg.n_reg = 0;
    x_tx_hdr_cfg.x_fields.b_auto_r_bit_insertion = p_msg->b_auto_r_bit;
    x_tx_hdr_cfg.x_fields.b_dba_packet = AG_FALSE;
    x_tx_hdr_cfg.x_fields.b_header_select_alternate = AG_FALSE;
    x_tx_hdr_cfg.x_fields.n_header_memory_size = n_hdr_template_size;
    x_tx_hdr_cfg.x_fields.n_header_memory_size += sizeof(AgNdRegTransmitHeaderFormatDescriptor);
    ag_nd_reg_write(p_device, AG_REG_TRANSMIT_HEADER_CONFIGURATION(p_msg->n_channel_id), x_tx_hdr_cfg.n_reg);
   ag_nd_set_kiss_debug(AG_FALSE);

    return AG_S_OK;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* get ingress channel enable status */
/*  */
AG_BOOL
ag_nd_channel_ingress_is_enabled(AgNdDevice *p_device, AgNdChannel n_channel_id)
{
    AgNdRegChannelControl x_chi_ctl;


    ag_nd_reg_read(p_device, AG_REG_INGRESS_CHANNEL_CONTROL(n_channel_id), &(x_chi_ctl.n_reg));
    assert(x_chi_ctl.x_fields.b_channel_enable == p_device->a_channel[n_channel_id].a_enable[AG_ND_PATH_INGRESS]);


    return (AG_BOOL)x_chi_ctl.x_fields.b_channel_enable;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* enables ingress channel */
/*  */
AgResult
ag_nd_channel_ingress_enable(AgNdDevice *p_device, AgNdChannel n_channel_id, AG_U32 n_cookie)
{
    AgNdRegChannelControl                       x_chi_ctl;
   #ifndef CES16_BCM_VERSION /*Not needed for neptune*/
    AgNdRegPayloadBufferChannelConfiguration    x_pbf_cfg;
    AG_U32                                      n_pbf_addr;
    AG_U32                                      n_pbf_size;
    AgResult                                    n_ret;
   #endif

#ifdef AG_ND_ENABLE_VALIDATION
    /* */
    /* check if channel is already enabled */
    /* */
    if (ag_nd_channel_ingress_is_enabled(p_device, n_channel_id))
        return AG_ND_ERR(p_device, AG_E_ND_ENABLE);

    /* */
    /* check if FTIP is configured */
    /* */
    if (!AG_ND_TS_ISVALID(p_device->a_channel[n_channel_id].a_channelizer[AG_ND_PATH_INGRESS].x_first))
        return AG_ND_ERR(p_device, AG_E_ND_FTIP);
#endif


   #ifndef CES16_BCM_VERSION /*Not needed for neptune*/
    /* */
    /* allocate PBF */
    /* */
    ag_nd_reg_read(p_device, AG_REG_PAYLOAD_BUFFER_CHANNEL_CONFIGURATION(n_channel_id), &(x_pbf_cfg.n_reg));
    n_pbf_size = 1 << ((AG_U32)x_pbf_cfg.x_fields.n_payload_buffer_queue_size + 6);

    n_ret = ag_nd_allocator_alloc(&(p_device->x_allocator_pbf), n_pbf_size, 64, &n_pbf_addr);
    if (AG_FAILED(n_ret))
        return AG_ND_ERR(p_device, AG_E_ND_ALLOC);

    /* */
    /* configure PBF address */
    /* */
    ag_nd_reg_write(p_device, AG_REG_PAYLOAD_BUFFER_QUEUE_BASE_ADDRESS(n_channel_id), (AG_U16)(n_pbf_addr >> 6));
   #endif /*CES16_BCM_VERSION*/


    /* */
    /* enable CHI */
    /* */
    ag_nd_reg_read(p_device, AG_REG_INGRESS_CHANNEL_CONTROL(n_channel_id), &(x_chi_ctl.n_reg));
    assert(x_chi_ctl.x_fields.b_channel_enable == p_device->a_channel[n_channel_id].a_enable[AG_ND_PATH_INGRESS]);
    x_chi_ctl.x_fields.b_channel_enable = AG_TRUE;
    ag_nd_reg_write(p_device, AG_REG_INGRESS_CHANNEL_CONTROL(n_channel_id), x_chi_ctl.n_reg);

    /* */
    /* update shadow structures */
    /* */
    p_device->a_channel[n_channel_id].a_enable[AG_ND_PATH_INGRESS] = (AG_BOOL)x_chi_ctl.x_fields.b_channel_enable;


    return AG_S_OK;

	AG_UNUSED_PARAM(n_cookie);
}


/*///////////////////////////////////////////////////////////////////////////// */
/* disables ingress channel */
/*  */
AgResult
ag_nd_channel_ingress_disable(AgNdDevice *p_device, AgNdChannel n_channel_id, AG_U32 n_cookie)
{
    AgNdRegChannelControl x_chi_ctl;
   #ifndef CES16_BCM_VERSION  /*Not needed for neptune*/
    AG_U16                n_pbf_base;
    AgResult              n_ret;
   #endif

#ifdef AG_ND_ENABLE_VALIDATION
    /* */
    /* check if channel is already disabled */
    /* */
    if (!ag_nd_channel_ingress_is_enabled(p_device, n_channel_id))
        return AG_ND_ERR(p_device, AG_E_ND_ENABLE);
#endif


    /* */
    /* disable CHI */
    /* */
    ag_nd_reg_read(p_device, AG_REG_INGRESS_CHANNEL_CONTROL(n_channel_id), &(x_chi_ctl.n_reg));
    assert(x_chi_ctl.x_fields.b_channel_enable == p_device->a_channel[n_channel_id].a_enable[AG_ND_PATH_INGRESS]);
    x_chi_ctl.x_fields.b_channel_enable = AG_FALSE;
    ag_nd_reg_write(p_device, AG_REG_INGRESS_CHANNEL_CONTROL(n_channel_id), x_chi_ctl.n_reg);


  #ifndef CES16_BCM_VERSION  /*Not needed for neptune*/
    /* */
    /* deallocate PBF */
    /* */
    ag_nd_reg_read(p_device, AG_REG_PAYLOAD_BUFFER_QUEUE_BASE_ADDRESS(n_channel_id), &n_pbf_base);
    n_ret = ag_nd_allocator_free(&(p_device->x_allocator_pbf), (AG_U32)n_pbf_base << 6);
    assert(AG_SUCCEEDED(n_ret));

    /* */
    /* set PBF address to zero */
    /* */
    ag_nd_reg_write(p_device, AG_REG_PAYLOAD_BUFFER_QUEUE_BASE_ADDRESS(n_channel_id), 0);
  #endif

    /* */
    /* update shadow structures */
    /* */
    p_device->a_channel[n_channel_id].a_enable[AG_ND_PATH_INGRESS] = (AG_BOOL)x_chi_ctl.x_fields.b_channel_enable;


    return AG_S_OK;

	AG_UNUSED_PARAM(n_cookie);
}

/*///////////////////////////////////////////////////////////////////////////// */
/* dynamic CWS CW change */
/*  */
AgResult
ag_nd_opcode_write_config_control_word(AgNdDevice *p_device, AgNdMsgConfigCw *p_msg)
{
    AG_U16  n_cw;
    AgNdRegTransmitHeaderConfiguration  x_tx_hdr_cfg;
  #ifndef CES16_BCM_VERSION  /*Not needed for neptune*/
    AgNdRegTransmitHeaderAccessControl  x_tx_hdr_ac;
  #endif
    AG_U32  n_hdr_ptr;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_channel_id=%hu\n", p_msg->n_channel_id);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_bit_selector=%hu\n", p_msg->n_bit_selector);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_l_bit=%hhu\n", p_msg->n_l_bit);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_r_bit=%hhu\n", p_msg->n_r_bit);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_m_bits=%hhu\n", p_msg->n_m_bits);
    

#ifdef AG_ND_ENABLE_VALIDATION
    /* */
    /* check if channel id is valid */
    /* */
    if (p_msg->n_channel_id >= p_device->n_pw_max)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    /* */
    /* make sure bit selector contains only permitted bits */
    /* */
    if (p_msg->n_bit_selector & ~(AG_ND_CES_CW_L | AG_ND_CES_CW_R | AG_ND_CES_CW_M))
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    /* */
    /* can't set only one of M bits */
    /* */
    if (p_msg->n_bit_selector & AG_ND_CES_CW_M)
        if (AG_ND_CES_CW_M != (p_msg->n_bit_selector & AG_ND_CES_CW_M))
            return AG_ND_ERR(p_device, AG_E_ND_ARG);

    /* */
    /* check if channel is already enabled */
    /* */
    if (!p_msg->b_redundancy_config)
    {
    	if (!ag_nd_channel_ingress_is_enabled(p_device, p_msg->n_channel_id))
	        return AG_ND_ERR(p_device, AG_E_ND_ENABLE);
    }
#endif

    /* */
    /* can't change R bit if automatic insertion mode is enabled for this channel */
    /*  */
    ag_nd_reg_read(p_device, 
                   AG_REG_TRANSMIT_HEADER_CONFIGURATION(p_msg->n_channel_id), 
                   &(x_tx_hdr_cfg.n_reg));

#ifdef AG_ND_ENABLE_VALIDATION
    if (x_tx_hdr_cfg.x_fields.b_auto_r_bit_insertion && 
        (p_msg->n_bit_selector & AG_ND_CES_CW_R))
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif


    n_cw = 0;


    if (p_msg->n_bit_selector & AG_ND_CES_CW_L)
    {
#ifdef AG_ND_ENABLE_VALIDATION
        if ((p_msg->n_l_bit << AG_ND_CES_CW_L_START_BIT) & ~AG_ND_CES_CW_L)
            return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif

        n_cw |= p_msg->n_l_bit << AG_ND_CES_CW_L_START_BIT;
    }


    if (p_msg->n_bit_selector & AG_ND_CES_CW_R)
    {
#ifdef AG_ND_ENABLE_VALIDATION
        if ((p_msg->n_r_bit << AG_ND_CES_CW_R_START_BIT) & ~AG_ND_CES_CW_R)
            return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif

        n_cw |= p_msg->n_r_bit << AG_ND_CES_CW_R_START_BIT;
    }


    if (p_msg->n_bit_selector & AG_ND_CES_CW_M)
    {
#ifdef AG_ND_ENABLE_VALIDATION
        if ((p_msg->n_m_bits << AG_ND_CES_CW_M_START_BIT) & ~AG_ND_CES_CW_M)
            return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif

        n_cw |= p_msg->n_m_bits << AG_ND_CES_CW_M_START_BIT;
    }


    /* */
    /* set pointer to inactive header  */
    /* */
    n_hdr_ptr = p_msg->n_channel_id * 2 + (x_tx_hdr_cfg.x_fields.b_header_select_alternate ? 0 : 1);
    n_hdr_ptr *= p_device->n_data_header_memory_block_size;
    ag_nd_set_kiss_debug(AG_TRUE); 

    /* */
    /* update CW in inactive header  */
    /* */
    ag_nd_header_update_cw(
        p_device,
        p_device->p_mem_data_header,
        n_hdr_ptr,
        p_msg->n_bit_selector,
        n_cw,
        &(p_device->a_channel_ingress[p_msg->n_channel_id]));


    /* */
    /* enable DBA if L bit set and DBA enabled */
    /*  */
    if ((n_cw & AG_ND_CES_CW_L) && 
        (p_device->a_channel_ingress[p_msg->n_channel_id].b_dba))

        x_tx_hdr_cfg.x_fields.b_dba_packet = AG_TRUE;
    else
        x_tx_hdr_cfg.x_fields.b_dba_packet = AG_FALSE;


    /* */
    /* switch headers */
    /* */
    x_tx_hdr_cfg.x_fields.b_header_select_alternate = !x_tx_hdr_cfg.x_fields.b_header_select_alternate;
    ag_nd_reg_write(p_device, AG_REG_TRANSMIT_HEADER_CONFIGURATION(p_msg->n_channel_id), x_tx_hdr_cfg.n_reg);

    ag_nd_set_kiss_debug(AG_FALSE); 

    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* dynamic destination MAC change */
/*  */
AgResult 
ag_nd_opcode_write_config_dest_mac(
    AgNdDevice              *p_device, 
    AgNdMsgConfigDestMac    *p_msg)
{
    AgNdRegTransmitHeaderConfiguration          x_tx_hdr_cfg;
    AgNdRegTransmitHeaderAccessControl          x_tx_hdr_ac;

    AG_U32  n_hdr_ptr;
    AG_U32  n_hdr_template_ptr;
    AG_U32  n_blk_size;

    x_tx_hdr_ac.n_reg = 0;

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_channel_id=%hu\n", p_msg->n_channel_id);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "a_dest_mac[0]=%hhu\n", p_msg->a_dest_mac[0]);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "a_dest_mac[1]=%hhu\n", p_msg->a_dest_mac[1]);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "a_dest_mac[2]=%hhu\n", p_msg->a_dest_mac[2]);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "a_dest_mac[3]=%hhu\n", p_msg->a_dest_mac[3]);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "a_dest_mac[4]=%hhu\n", p_msg->a_dest_mac[4]);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "a_dest_mac[5]=%hhu\n", p_msg->a_dest_mac[5]);


#ifdef AG_ND_ENABLE_VALIDATION
    /* */
    /* check if channel id is valid */
    /* */
    if (p_msg->n_channel_id >= p_device->n_pw_max)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    /* */
    /* check if channel is already enabled */
    /* */
    if (!p_msg->b_is_redundancy)
    {
    	if (!ag_nd_channel_ingress_is_enabled(p_device, p_msg->n_channel_id))
        	return AG_ND_ERR(p_device, AG_E_ND_ENABLE);
    }
#endif

   ag_nd_set_kiss_debug(AG_TRUE);

    /* */
    /* read headers configuration */
    /*  */
    ag_nd_reg_read(p_device, AG_REG_TRANSMIT_HEADER_CONFIGURATION(p_msg->n_channel_id), &(x_tx_hdr_cfg.n_reg));
   #ifndef CES16_BCM_VERSION
    ag_nd_reg_read(p_device, AG_REG_TRANSMIT_HEADER_ACCESS_CONTROL, &(x_tx_hdr_ac.n_reg));
   #endif /*CES16_BCM_VERSION*/
 
    n_blk_size = 1 << (x_tx_hdr_ac.x_fields.n_header_memory_block_size + 5);

    /* */
    /* set pointer to inactive header  */
    /* */
    n_hdr_ptr = p_msg->n_channel_id * 2 + (x_tx_hdr_cfg.x_fields.b_header_select_alternate ? 0 : 1);
    n_hdr_ptr *= n_blk_size;
    n_hdr_template_ptr = n_hdr_ptr + 4; 

    /* */
    /* store the new destination MAC  */
    /* */
    ag_nd_mem_write(p_device, p_device->p_mem_data_header, n_hdr_template_ptr, p_msg->a_dest_mac, 3);

    /* */
    /* switch headers */
    /* */
    x_tx_hdr_cfg.x_fields.b_header_select_alternate = !x_tx_hdr_cfg.x_fields.b_header_select_alternate;
    ag_nd_reg_write(p_device, AG_REG_TRANSMIT_HEADER_CONFIGURATION(p_msg->n_channel_id), x_tx_hdr_cfg.n_reg);

    /* */
    /* set pointer to inactive header  */
    /* */
    n_hdr_ptr = p_msg->n_channel_id * 2 + (x_tx_hdr_cfg.x_fields.b_header_select_alternate ? 0 : 1);
    n_hdr_ptr *= n_blk_size;
    n_hdr_template_ptr = n_hdr_ptr + 4; 

    /* */
    /* store the new destination MAC  */
    /* */
    ag_nd_mem_write(p_device, p_device->p_mem_data_header, n_hdr_template_ptr, p_msg->a_dest_mac, 3);

   ag_nd_set_kiss_debug(AG_FALSE); 

    return AG_S_OK;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* Set PW out sequence number */
/*  */
AgResult
ag_nd_opcode_write_config_sequence_number(
	AgNdDevice *p_device, 
	AgNdMsgConfigSeqNumber *p_msg)
{
	/* */
    /* configure initial SQN */
    /*  */
    ag_nd_reg_write(p_device,
                    AG_REG_PW_OUT_SEQUENCE_NUMBER(p_msg->n_channel_id),
                    p_msg->n_ces_sqn);
    return AG_S_OK;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* PM */
/*  */
AgResult 
ag_nd_opcode_read_pm_ingress(
    AgNdDevice *p_device, 
    AgNdMsgPmIngress *p_msg)
{
   #ifndef CES16_BCM_VERSION
    AgNdRegPayloadBufferStatus x_pbf_stat;
   #endif 
    AG_U16 n_lo;
    AG_U16 n_hi;


    AG_ND_TRACE(p_device, AG_ND_TRACE_PERIODIC, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);

#ifdef AG_ND_ENABLE_VALIDATION
    /* */
    /* check if channel id is valid */
    /* */
    if (p_msg->n_channel_id >= p_device->n_pw_max)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif

   #ifndef CES16_BCM_VERSION
    ag_nd_reg_read(p_device, AG_REG_PAYLOAD_BUFFER_STATUS(p_msg->n_channel_id), &(x_pbf_stat.n_reg));
    p_msg->n_pbf_utilization = x_pbf_stat.x_fields.n_buffer_maximum_utilization;
   #endif
    /*ORI*/
   /*changed read of regs*/
    ag_nd_reg_read(p_device, AG_REG_PWE_OUT_TRANSMITTED_BYTES_1(p_msg->n_channel_id), &(n_hi));
	ag_nd_reg_read(p_device, AG_REG_PWE_OUT_TRANSMITTED_BYTES_2(p_msg->n_channel_id), &(n_lo));
    p_msg->n_tpe_pwe_out_byte_counter = AG_ND_MAKE32LH(n_lo, n_hi);
    
    ag_nd_reg_read(p_device, AG_REG_PWE_OUT_TRANSMITTED_PACKETS(p_msg->n_channel_id), &(n_lo));
    p_msg->n_tpe_pwe_out_packet_counter = n_lo;

    /* CAS TEMP: remove if when implemented in Nemo */
    if (1 /*AG_ND_ARCH_MASK_NEPTUNE == p_device->n_chip_mask*/)  /*BCM as neptune, cas packets do not bring to cpu*/
    {
        ag_nd_reg_read(p_device, AG_REG_CAS_OUT_TRANSMITTED_PACKETS(p_msg->n_channel_id), &(n_lo));
        p_msg->n_cas_tx = n_lo;
    }

    return AG_S_OK;
}

