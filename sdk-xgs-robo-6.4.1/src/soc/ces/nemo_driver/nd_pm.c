/* $Id: nd_pm.c,v 1.5 Broadcom SDK $
 * Copyright 2011 BATM
 */

#include "pub/nd_api.h"
#include "pub/nd_hw.h"
#include "nd_registers.h"
#include "nd_pm.h"
#include "nd_debug.h"

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_pm_global */
/* */
AgResult 
ag_nd_opcode_read_pm_global(AgNdDevice *p_device, AgNdMsgPmGlobal *p_msg)
{
    AG_U32 i;
    AG_U16 n_reg = 0;

    AG_ND_TRACE(p_device, AG_ND_TRACE_PERIODIC, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    
    #ifndef CES16_BCM_VERSION
    ag_nd_reg_read(p_device, AG_REG_RECEIVED_HOST_PACKET_DROP_COUNT, &n_reg);
    #endif /*CES16_BCM_VERSION*/

    p_msg->n_hpl_hp_dropped = AG_ND_HIGH8(n_reg);
    p_msg->n_hpl_lp_dropped = AG_ND_LOW8(n_reg);

    for (i = 0; i < AG_ND_RPC_CGLB_MAX; i++)
    {
        ag_nd_reg_read(p_device, AG_REG_CLASSIFIER_GLOBAL_PM_COUNTER(i), &n_reg);
        p_msg->a_rpc_global[i] = n_reg;
    }

    ag_nd_reg_read(p_device, AG_REG_CLASSIFIER_DROPPED_PACKET_COUNT, &(p_msg->n_rpc_dropped));
   #ifndef CES16_BCM_VERSION
    ag_nd_reg_read(p_device, AG_REG_HIGH_PRIORITY_HOST_PACKET_COUNT, &(p_msg->n_hpl_hp_forwarded));
    ag_nd_reg_read(p_device, AG_REG_LOW_PRIORITY_HOST_PACKET_COUNT, &(p_msg->n_hpl_lp_forwarded));
   #else
     p_msg->n_hpl_hp_forwarded = 0;
     p_msg->n_hpl_lp_forwarded = 0;
   #endif /*CES16_BCM_VERSION*/


    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_config_channel_pme */
/* */
AgResult 
ag_nd_opcode_read_config_channel_pme(
    AgNdDevice *p_device, 
    AgNdMsgConfigChannelPme *p_msg)
{
   #ifndef CES16_BCM_VERSION
    AgNdRegPmCounterExportChannnelSelection x_pme_sel;
   #endif
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);

#ifdef AG_ND_ENABLE_VALIDATION
    if (p_msg->n_channel_id >= p_device->n_pw_max)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif

   #ifndef CES16_BCM_VERSION
    ag_nd_reg_read(
        p_device, 
        AG_REG_PM_COUNTER_EXPORT_CHANNEL_SELECTION(p_msg->n_channel_id), 
        &(x_pme_sel.n_reg));

    p_msg->a_group[0] = x_pme_sel.x_fields.b_pme_group_1_select;
    p_msg->a_group[1] = x_pme_sel.x_fields.b_pme_group_2_select;
    p_msg->a_group[2] = x_pme_sel.x_fields.b_pme_group_3_select;
   #endif
    return AG_S_OK;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_config_channel_pme */
/* */
AgResult 
ag_nd_opcode_write_config_channel_pme(
    AgNdDevice *p_device, 
    AgNdMsgConfigChannelPme *p_msg)
{
   #ifndef CES16_BCM_VERSION
    AgNdRegPmCounterExportChannnelSelection x_pme_sel;
   #endif
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_channel_id=%hu\n", p_msg->n_channel_id);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "a_group[0]=%lu\n", p_msg->a_group[0]);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "a_group[1]=%lu\n", p_msg->a_group[1]);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "a_group[2]=%lu\n", p_msg->a_group[2]);

#ifdef AG_ND_ENABLE_VALIDATION
    if (p_msg->n_channel_id >= p_device->n_pw_max)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif

   #ifndef CES16_BCM_VERSION

    x_pme_sel.n_reg = 0;
    x_pme_sel.x_fields.b_pme_group_1_select = p_msg->a_group[0];
    x_pme_sel.x_fields.b_pme_group_2_select = p_msg->a_group[1];
    x_pme_sel.x_fields.b_pme_group_3_select = p_msg->a_group[2];

    ag_nd_reg_write(
        p_device, 
        AG_REG_PM_COUNTER_EXPORT_CHANNEL_SELECTION(p_msg->n_channel_id), 
        x_pme_sel.n_reg);
   #endif
    return AG_S_OK;
}
#if 0
   AgResult
   ag_nd_opcode_read_pm(AgNdDevice *p_device, AgNdMsgPm *p_msg)
   {
      AG_U32 i;
   
      AG_U16 n_lo;
      AG_U16 n_hi;
   
      AgNdRegPayloadBufferStatus                  x_pbf_stat;
      AgNdRegJitterBufferPerformanceCounters      x_jbf_pm;
      AgNdRegOutOfOrderPacketCounters             x_jbf_ooo;
      AgNdRegBadLengthPacketCounter               x_jbf_bl;
      AgNdRegJitterBufferChannelStatus            x_jbf_stat;
   
   
      AG_ND_TRACE(p_device, AG_ND_TRACE_PERIODIC, AG_ND_TRACE_DEBUG, 
              "ag_nd_opcode_read_pm\n");
   
   
      if (!p_msg)
          return AG_ND_ERR(p_device, AG_E_ND_ARG);
   
   
      for (i = 0; i < p_device->n_pw_max; i++)
      {
          p_msg->a_enable[AG_ND_PATH_EGRESS][i] = p_msg->a_enable[AG_ND_PATH_INGRESS][i] = AG_FALSE;
      }
   
   
      for (i = 0; i < p_device->n_pw_max; i++)
      {
          
           ingress path
          
   
          if (AG_TRUE /*p_device->a_channel[AG_ND_PATH_INGRESS][i].b_enable*/)
          {
              p_msg->a_enable[AG_ND_PATH_INGRESS][i] = AG_TRUE;
   
              
               strictly speaking PBF status is not a part of PM, so may be it should be read
               by channel config n_opcode and not here
              
              ag_nd_reg_read(p_device, AG_REG_PAYLOAD_BUFFER_STATUS(i), &(x_pbf_stat.n_reg));
              p_msg->a_pbf_utilization[i] = x_pbf_stat.x_fields.n_buffer_maximum_utilization;
   
              ag_nd_reg_read(p_device, AG_REG_PWE_OUT_TRANSMITTED_BYTES_2(i), &(n_lo));
              ag_nd_reg_read(p_device, AG_REG_PWE_OUT_TRANSMITTED_BYTES_1(i), &(n_hi));
              p_msg->a_tpe_pwe_out_byte_counter[i] = AG_ND_MAKE32LH(n_lo, n_hi);
   
              ag_nd_reg_read(p_device, AG_REG_PWE_OUT_TRANSMITTED_PACKETS(i), &(n_lo));
              p_msg->a_tpe_pwe_out_packet_counter[i] = n_lo;
          }
   
   
          
           egress path
          
   
   
          if (AG_TRUE /*p_device->a_channel[AG_ND_PATH_EGRESS][i].b_enable*/)
          {
              p_msg->a_enable[AG_ND_PATH_EGRESS][i] = AG_TRUE;
   
              ag_nd_reg_read(p_device, AG_REG_MINIMUM_JITTER_BUFFER_DEPTH_2(i), &(n_lo));
              ag_nd_reg_read(p_device, AG_REG_MINIMUM_JITTER_BUFFER_DEPTH_1(i), &(n_hi));
              p_msg->a_jbf_depth_min[i] = AG_ND_MAKE32LH(n_lo, n_hi);
   
              ag_nd_reg_read(p_device, AG_REG_MAXIMUM_JITTER_BUFFER_DEPTH_2(i), &(n_lo));
              ag_nd_reg_read(p_device, AG_REG_MAXIMUM_JITTER_BUFFER_DEPTH_1(i), &(n_hi));
              p_msg->a_jbf_depth_max[i] = AG_ND_MAKE32LH(n_lo, n_hi);
   
              ag_nd_reg_read(p_device, AG_REG_JITTER_BUFFER_PERFORMANCE_COUNTERS(i), &(x_jbf_pm.n_reg));
              p_msg->a_jbf_underrun_counter[i] = (AG_U8)x_jbf_pm.x_fields.n_buffer_underrun_counter;
              p_msg->a_jbf_missing_packet_counter[i] = (AG_U8)x_jbf_pm.x_fields.n_missing_packet_counter;
   
              ag_nd_reg_read(p_device, AG_REG_PWE3_IN_BAD_LENGTH_PACKET_COUNT(i), &(x_jbf_bl.n_reg));
              p_msg->a_jbf_bad_length_packet_counter[i] = (AG_U8)x_jbf_bl.x_fields.n_bad_length_packet_counter;
   
              ag_nd_reg_read(p_device, AG_REG_OUT_OF_ORDER_PACKET_COUNTERS(i), &(x_jbf_ooo.n_reg));
              p_msg->a_jbf_dropped_ooo_packet_counter[i] = (AG_U8)x_jbf_ooo.x_fields.n_dropped_out_of_order_packet_count;
              p_msg->a_jbf_reordered_ooo_packet_counter[i] = (AG_U8)x_jbf_ooo.x_fields.n_reordered_out_of_order_packet_count;
   
              ag_nd_reg_read(p_device, AG_REG_PWE_IN_RECEIVED_BYTES_2(i), &(n_lo));
              ag_nd_reg_read(p_device, AG_REG_PWE_IN_RECEIVED_BYTES_1(i), &(n_hi));
              p_msg->a_rpc_pwe_in_byte_counter[i] = AG_ND_MAKE32LH(n_lo, n_hi);
   
              ag_nd_reg_read(p_device, AG_REG_PWE_IN_RECEIVED_PACKETS(i), &(n_lo));
              p_msg->a_rpc_pwe_in_packet_counter[i] = n_lo;
   
               ag_nd_reg_read(p_device, AG_REG_PWE_IN_ERROR_PACKET_COUNT(i), &(n_lo));
               p_msg->a_rpc_pwe_in_error_packet_count[i] = (AG_U8)n_lo;
   
              ag_nd_reg_read(p_device, AG_REG_JITTER_BUFFER_CHANNEL_STATUS(i), &(x_jbf_stat.n_reg));
              p_msg->a_jbf_state[i] = x_jbf_stat.x_fields.n_jitter_buffer_state;
   
              ag_nd_reg_read(p_device, AG_REG_CLASSIFIER_CHANNEL_SPECIFIC_PM_COUNTER_1(i), &(n_lo));
              ag_nd_reg_read(p_device, AG_REG_CLASSIFIER_CHANNEL_SPECIFIC_PM_COUNTER_2(i), &(n_hi));
              p_msg->a_rpc_channel_specific_count[i][0] = AG_ND_HIGH8(n_lo);
              p_msg->a_rpc_channel_specific_count[i][1] = AG_ND_LOW8(n_lo);
              p_msg->a_rpc_channel_specific_count[i][2] = AG_ND_HIGH8(n_hi);
              p_msg->a_rpc_channel_specific_count[i][3] = AG_ND_LOW8(n_hi);
          }
      }
      
      return AG_S_OK;
   }
#endif


/*ORI*/
/*add to get pm missimg status*/ 
AgResult
ag_nd_opcode_read_channel_pme_missing_status(AgNdDevice *p_device,AgNdMsgChannelPmeMessingStatus *p_msg)
{
	AgNdRegJitterBufferMissingPacketCount  missing_msg;
	AgNdRegJitterBufferUnderrunPacketCount underrun_msg;
	AgNdRegJitterBufferRestartCount		   restart_msg;

	/*read from regs*/
	ag_nd_reg_read(
        p_device, 
        AG_REG_JITTER_BUFFER_MISSING_PACKET_COUNT(p_msg->n_channel_id), 
        &(missing_msg.n_reg));
	ag_nd_reg_read(
        p_device, 
        AG_REG_JITTER_BUFFER_UNDERRUN_PACKET_COUNT(p_msg->n_channel_id), 
        &(underrun_msg.n_reg));
	ag_nd_reg_read(
        p_device, 
        AG_REG_JITTER_BUFFER_RESTART_COUNT(p_msg->n_channel_id), 
        &(restart_msg.n_reg));

	p_msg->n_missing_packet_count = missing_msg.x_fields.n_missing_packet_count;
	p_msg->n_underrun_packet_count = underrun_msg.x_fields.n_underrun_packet_count;
	p_msg->n_restart_count = restart_msg.x_fields.n_restart_count;

	return AG_S_OK;
}



