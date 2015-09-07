/* $Id: 01ed0a81fd6996b3a9825c28020c6ed1632ec60e $
 * Copyright 2011 BATM
 */

#include "pub/nd_api.h"
#include "pub/nd_hw.h"
#include "nd_registers.h"
#include "nd_global.h"
#include "nd_debug.h"


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_config_global */
/* */
AgResult
ag_nd_opcode_read_config_global(AgNdDevice *p_device, AgNdMsgConfigGlobal *p_msg)
{
    AgNdRegPayloadReplacementBytePattern x_rpl;
    AgNdRegPayloadReplacementPolicy      x_prp;
    AgNdRegSyncThreshold                 x_sync;
   #ifndef CES16_BCM_VERSION
    AgNdRegHostPacketLinkConfiguration   x_hpl_cfg;
    AgNdRegPmeStructureAssembly          x_pme_asmb;
   #endif  
    AgNdRegJitterBufferSlipCommand       x_slip_cmd;
    AgNdRegGlobalControl                 x_global;
    AgNdRegRaiConfiguration              x_rai;
    AgNdRegCasIdlePattern                x_cas_idle;
    AgNdRegTransmitCasPacketDelay        x_cas_delay;
    AgNdRegJbfSequenceNumberWindow       x_cas_win;
	
    AG_U32 i;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);

    memset(p_msg,0,sizeof(AgNdMsgConfigGlobal));  /*BCMadd*/

    /* */
    /* payload replacement policy */
    /* */
    ag_nd_reg_read(p_device, AG_REG_PACKET_PAYLOAD_REPLACEMENT_BYTE_PATTERN, &(x_rpl.n_reg));
    p_msg->n_filler_byte = (AG_U8)x_rpl.x_fields.n_filler_byte_pattern;
    p_msg->n_idle_byte = (AG_U8)x_rpl.x_fields.n_idle_byte_pattern;

    ag_nd_reg_read(p_device, AG_REG_PAYLOAD_REPLACEMENT_POLICY, &(x_prp.n_reg));
    p_msg->x_pr_structured.e_missing = x_prp.x_fields.n_missing_structured;
    p_msg->x_pr_unstructured.e_missing = x_prp.x_fields.n_missing_unstructured;
    p_msg->x_pr_structured.e_lops = x_prp.x_fields.n_lops_structured;
    p_msg->x_pr_unstructured.e_lops = x_prp.x_fields.n_lops_unstructured;
    p_msg->x_pr_structured.e_l_bit = x_prp.x_fields.n_l_bit_structured;
    p_msg->x_pr_unstructured.e_l_bit = x_prp.x_fields.n_l_bit_unstructured;
    p_msg->x_pr_structured.e_rai_rdi = x_prp.x_fields.n_rai_structured;
    p_msg->x_pr_unstructured.e_rai_rdi = x_prp.x_fields.n_rai_unstructured;

    /* */
    /* lops/aops */
    /* */
    for (i = 0; i < AG_ND_SYNC_THRESHOLD_TABLE_SIZE; ++i)
    {
        ag_nd_reg_read(p_device, AG_REG_LOSS_OF_PACKET_SYNC_THRESHOLD_TABLE(i), &(x_sync.n_reg));
        p_msg->a_lops_threshold_table[i] = x_sync.x_fields.n_packet_sync_threshold;

        ag_nd_reg_read(p_device, AG_REG_ACQUISITION_OF_PACKET_SYNC_THRESHOLD_TABLE(i), &(x_sync.n_reg));
        p_msg->a_aops_threshold_table[i] = x_sync.x_fields.n_packet_sync_threshold;
    }


    /* */
    /* HPL */
    /* */
   #ifndef CES16_BCM_VERSION
    ag_nd_reg_read(p_device, AG_REG_HOST_PACKET_LINK_CONFIGURATION, &(x_hpl_cfg.n_reg));

    p_msg->b_hpl_rx_enable = x_hpl_cfg.x_fields.b_hpl_rx_enable;
    p_msg->b_hpl_tx_enable = x_hpl_cfg.x_fields.b_hpl_tx_enable;
    p_msg->b_pme_enable = x_hpl_cfg.x_fields.b_pme_export_enable;

    p_msg->x_pme_group[0].n_channels = x_hpl_cfg.x_fields.n_pme_channels_1 * 4;
    p_msg->x_pme_group[1].n_channels = x_hpl_cfg.x_fields.n_pme_channels_2 * 4;
    p_msg->x_pme_group[2].n_channels = x_hpl_cfg.x_fields.n_pme_channels_3 * 4;


    ag_nd_reg_read(p_device, AG_REG_PM_EXPORT_STRUCTURE_ASSEMBLY_REGISTER(0), &(x_pme_asmb.n_reg));
    p_msg->x_pme_group[0].b_auto_export = x_pme_asmb.x_fields.b_auto_trigger_enable;
    p_msg->x_pme_group[0].n_counters = x_pme_asmb.x_fields.n_counter_select;

    ag_nd_reg_read(p_device, AG_REG_PM_EXPORT_STRUCTURE_ASSEMBLY_REGISTER(1), &(x_pme_asmb.n_reg));
    p_msg->x_pme_group[1].b_auto_export = x_pme_asmb.x_fields.b_auto_trigger_enable;
    p_msg->x_pme_group[1].n_counters = x_pme_asmb.x_fields.n_counter_select;

    ag_nd_reg_read(p_device, AG_REG_PM_EXPORT_STRUCTURE_ASSEMBLY_REGISTER(2), &(x_pme_asmb.n_reg));
    p_msg->x_pme_group[2].b_auto_export = x_pme_asmb.x_fields.b_auto_trigger_enable;
    p_msg->x_pme_group[2].n_counters = x_pme_asmb.x_fields.n_counter_select;
  #endif /*CES16_BCM_VERSION*/

    /* */
    /* global trimming enable */
    /* */
    ag_nd_reg_read(p_device, AG_REG_JITTER_BUFFER_SLIP_COMMAND, &(x_slip_cmd.n_reg));
    p_msg->b_trimming = x_slip_cmd.x_fields.b_global_trimming_enable;

    /* */
    /* CW interrupt mask */
    /*  */
    ag_nd_reg_read(p_device, AG_REG_CES_CONTROL_WORD_INTERRUPT_MASK, &(p_msg->n_cw_mask));

    /* */
    /* LAN/WAN DCE mode  */
    /*  */
    if (AG_ND_ARCH_MASK_NEMO & p_device->n_chip_mask) /*BCM do not need*/
    {
        ag_nd_reg_read(p_device, AG_REG_GLOBAL_CONTROL, &(x_global.n_reg));
        p_msg->b_wan_dce_mode_enable = x_global.x_fields.b_wan_dce_mode_enable;
        /*ISTTFIX, if high bit is 1, dce enabled*/
        p_msg->b_lan_dce_mode_enable = x_global.x_fields.b_lan_dce_mode_enable_h;
    }
    else
    {
        p_msg->b_wan_dce_mode_enable = AG_FALSE;
        p_msg->b_lan_dce_mode_enable = AG_FALSE;
    }

    /* */
    /* CAS */
    /*  */
    /* CAS TEMP: remove if when implemented in Nemo */
    if (1 /*AG_ND_ARCH_MASK_NEPTUNE == p_device->n_chip_mask*/)  /*BCM as neptune, cas packets do not bring to cpu*/
    {
        ag_nd_reg_read(p_device, AG_REG_CAS_IDLE_PATTERN, &(x_cas_idle.n_reg));
        p_msg->n_cas_e1_idle_pattern = (AG_U8)x_cas_idle.x_fields.n_e1_idle_pattern;
        p_msg->n_cas_t1_idle_pattern = (AG_U8)x_cas_idle.x_fields.n_t1_idle_pattern;
    
        ag_nd_reg_read(p_device, AG_REG_TRANSMIT_CAS_PACKET_DELAY, &(x_cas_delay.n_reg));
        p_msg->n_cas_change_delay = (AG_U32)x_cas_delay.x_fields.n_change_delay;
        p_msg->n_cas_no_change_delay = (AG_U32)x_cas_delay.x_fields.n_no_change_delay;
	
        ag_nd_reg_read(p_device, AG_REG_JITTER_BUFFER_CAS_SEQUENCE_NUMBER_WINDOW, &(x_cas_win.n_reg));
        p_msg->n_cas_sqn_window = (AG_U32)x_cas_win.x_fields.n_cas_window_limit;
    }

    /* */
    /* RAI policy */
    /*  */
    ag_nd_reg_read(p_device, AG_REG_RAI_CONFIGURATION, &(x_rai.n_reg));
    p_msg->n_rai_detect = 0;

    if (x_rai.x_fields.b_m_bit_detection)
        p_msg->n_rai_detect |= AG_ND_RAI_DETECT_M;

    if (x_rai.x_fields.b_r_bit_detection)
        p_msg->n_rai_detect |= AG_ND_RAI_DETECT_R;


    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_config_global */
/* */

AgResult
ag_nd_opcode_write_config_global(AgNdDevice *p_device, AgNdMsgConfigGlobal *p_msg)
{
    AgNdRegPayloadReplacementBytePattern x_rpl;
    AgNdRegPayloadReplacementPolicy      x_prp;
    AgNdRegSyncThreshold                 x_sync;
   #ifndef CES16_BCM_VERSION
    AgNdRegHostPacketLinkConfiguration   x_hpl_cfg;
    AgNdRegPmeStructureAssembly          x_pme_asmb;
    AgNdRegGlobalControl                 x_global;
   #endif 
    AgNdRegJitterBufferSlipCommand       x_slip_cmd;
    AgNdRegRaiConfiguration              x_rai;
    AgNdRegCasIdlePattern                x_cas_idle;
    AgNdRegTransmitCasPacketDelay        x_cas_delay;
    AgNdRegJbfSequenceNumberWindow       x_cas_win;

    AG_U32 i;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "a_aops_threshold_table:\n");
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%hu\n", p_msg->a_aops_threshold_table[0]);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%hu\n", p_msg->a_aops_threshold_table[1]);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%hu\n", p_msg->a_aops_threshold_table[2]);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%hu\n", p_msg->a_aops_threshold_table[3]);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%hu\n", p_msg->a_aops_threshold_table[4]);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%hu\n", p_msg->a_aops_threshold_table[5]);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%hu\n", p_msg->a_aops_threshold_table[6]);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%hu\n", p_msg->a_aops_threshold_table[7]);

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "a_lops_threshold_table:\n");
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%hu\n", p_msg->a_lops_threshold_table[0]);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%hu\n", p_msg->a_lops_threshold_table[1]);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%hu\n", p_msg->a_lops_threshold_table[2]);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%hu\n", p_msg->a_lops_threshold_table[3]);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%hu\n", p_msg->a_lops_threshold_table[4]);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%hu\n", p_msg->a_lops_threshold_table[5]);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%hu\n", p_msg->a_lops_threshold_table[6]);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%hu\n", p_msg->a_lops_threshold_table[7]);

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "filler byte: %hhu\n", p_msg->n_filler_byte);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "idle byte: %hhu\n", p_msg->n_idle_byte);

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "prp structured:\n"); 
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "e_missing: %d\n", p_msg->x_pr_structured.e_missing); 
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "e_lops: %d\n", p_msg->x_pr_structured.e_lops); 
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "e_l_bit: %d\n", p_msg->x_pr_structured.e_l_bit); 

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "prp unstructured:\n"); 
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "e_missing: %d\n", p_msg->x_pr_unstructured.e_missing); 
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "e_lops: %d\n", p_msg->x_pr_unstructured.e_lops); 
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "e_l_bit: %d\n", p_msg->x_pr_unstructured.e_l_bit); 
    
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_trimming: %lu\n", p_msg->b_trimming); 

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_cw_mask: %hu\n", p_msg->n_cw_mask); 

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_hpl_rx_enable: %lu\n", p_msg->b_hpl_rx_enable); 
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_hpl_tx_enable: %lu\n", p_msg->b_hpl_tx_enable); 

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_pme_enable: %lu\n", p_msg->b_pme_enable); 

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "pme group 0:\n"); 
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_auto_export: %lu\n", p_msg->x_pme_group[0].b_auto_export); 
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_channels: %lu\n", p_msg->x_pme_group[0].n_channels); 
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_counters: %lu\n", p_msg->x_pme_group[0].n_counters); 

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "pme group 1:\n"); 
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_auto_export: %lu\n", p_msg->x_pme_group[1].b_auto_export); 
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_channels: %lu\n", p_msg->x_pme_group[1].n_channels); 
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_counters: %lu\n", p_msg->x_pme_group[1].n_counters); 

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "pme group 2:\n"); 
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_auto_export: %lu\n", p_msg->x_pme_group[2].b_auto_export); 
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_channels: %lu\n", p_msg->x_pme_group[2].n_channels); 
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_counters: %lu\n", p_msg->x_pme_group[2].n_counters); 

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_lan_dce: %lu\n", p_msg->b_lan_dce_mode_enable); 
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_wan_dce: %lu\n", p_msg->b_wan_dce_mode_enable); 

    /* CAS TEMP: remove if when implemented in Nemo */
    if (1 /*AG_ND_ARCH_MASK_NEPTUNE == p_device->n_chip_mask*/)
    {
        AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_cas_e1_idle_pattern: %hhu\n", p_msg->n_cas_e1_idle_pattern); 
        AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_cas_t1_idle_pattern: %hhu\n", p_msg->n_cas_t1_idle_pattern); 
    
        AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_cas_change_delay: %lu\n", p_msg->n_cas_change_delay); 
        AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_cas_no_change_delay: %lu\n", p_msg->n_cas_no_change_delay); 
        AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_cas_sqn_window: %lu\n", p_msg->n_cas_sqn_window); 
    
        AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_rai_detect: %lu\n", p_msg->n_rai_detect); 
    }

#ifdef AG_ND_ENABLE_VALIDATION
    /* */
    /* sanity */
    /* */
    for (i = 0; i < AG_ND_SYNC_THRESHOLD_TABLE_SIZE; i++) 
        if (p_msg->a_aops_threshold_table[i] < AG_ND_SYNC_THRESHOLD_MIN || 
            p_msg->a_aops_threshold_table[i] > AG_ND_SYNC_THRESHOLD_MAX || 
            p_msg->a_lops_threshold_table[i] < AG_ND_SYNC_THRESHOLD_MIN || 
            p_msg->a_lops_threshold_table[i] > AG_ND_SYNC_THRESHOLD_MAX)
            return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if (p_msg->x_pr_structured.e_l_bit < AG_ND_PRP_FIRST || p_msg->x_pr_structured.e_l_bit > AG_ND_PRP_LAST)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if (p_msg->x_pr_structured.e_lops < AG_ND_PRP_FIRST || p_msg->x_pr_structured.e_lops > AG_ND_PRP_LAST)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if (p_msg->x_pr_structured.e_missing < AG_ND_PRP_FIRST || p_msg->x_pr_structured.e_missing > AG_ND_PRP_LAST)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if (p_msg->x_pr_structured.e_rai_rdi < AG_ND_PRP_FIRST || p_msg->x_pr_structured.e_rai_rdi > AG_ND_PRP_LAST)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if (p_msg->x_pr_unstructured.e_l_bit < AG_ND_PRP_FIRST || p_msg->x_pr_unstructured.e_l_bit > AG_ND_PRP_LAST)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if (p_msg->x_pr_unstructured.e_lops < AG_ND_PRP_FIRST || p_msg->x_pr_unstructured.e_lops > AG_ND_PRP_LAST)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if (p_msg->x_pr_unstructured.e_missing < AG_ND_PRP_FIRST || p_msg->x_pr_unstructured.e_missing > AG_ND_PRP_LAST)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if (p_msg->x_pr_unstructured.e_rai_rdi < AG_ND_PRP_FIRST || p_msg->x_pr_unstructured.e_rai_rdi > AG_ND_PRP_LAST)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
    
    #ifndef CES16_BCM_VERSION
    if ((p_msg->x_pme_group[0].n_channels & 0x3) ||
        (p_msg->x_pme_group[1].n_channels & 0x3) ||
        (p_msg->x_pme_group[2].n_channels & 0x3))
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
    #endif   
    /* CAS TEMP: remove if when implemented in Nemo */
    if (1 /*AG_ND_ARCH_MASK_NEPTUNE == p_device->n_chip_mask*/)
    {
        if (p_msg->n_cas_e1_idle_pattern & 0xf0)
            return AG_ND_ERR(p_device, AG_E_ND_ARG);
    
        if (p_msg->n_cas_t1_idle_pattern & 0xf0)
            return AG_ND_ERR(p_device, AG_E_ND_ARG);
    
        if (p_msg->n_cas_change_delay & 0xfffffff8)
            return AG_ND_ERR(p_device, AG_E_ND_ARG);
    
        if (p_msg->n_cas_no_change_delay & 0xfffffff8)
            return AG_ND_ERR(p_device, AG_E_ND_ARG);
    }

    if (p_msg->n_rai_detect & ~(AG_ND_RAI_DETECT_R | AG_ND_RAI_DETECT_M))
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif


    /* */
    /* payload replacement policy */
    /* */
    x_rpl.n_reg = 0;
    x_rpl.x_fields.n_filler_byte_pattern  = p_msg->n_filler_byte;
    x_rpl.x_fields.n_idle_byte_pattern = p_msg->n_idle_byte;
    ag_nd_reg_write(p_device, AG_REG_PACKET_PAYLOAD_REPLACEMENT_BYTE_PATTERN, x_rpl.n_reg);

    x_prp.n_reg = 0;

    x_prp.x_fields.n_missing_structured = (field_t) p_msg->x_pr_structured.e_missing;
    x_prp.x_fields.n_missing_unstructured = (field_t) p_msg->x_pr_unstructured.e_missing;
    x_prp.x_fields.n_lops_structured = (field_t) p_msg->x_pr_structured.e_lops;
    x_prp.x_fields.n_lops_unstructured = (field_t) p_msg->x_pr_unstructured.e_lops;
    x_prp.x_fields.n_l_bit_structured = (field_t) p_msg->x_pr_structured.e_l_bit;
    x_prp.x_fields.n_l_bit_unstructured = (field_t) p_msg->x_pr_unstructured.e_l_bit;
    x_prp.x_fields.n_rai_structured = (field_t) p_msg->x_pr_structured.e_rai_rdi;
    x_prp.x_fields.n_rai_unstructured = (field_t) p_msg->x_pr_unstructured.e_rai_rdi;
    ag_nd_reg_write(p_device, AG_REG_PAYLOAD_REPLACEMENT_POLICY, x_prp.n_reg);

    /* */
    /* lops/aops tables */
    /* */
    for (i = 0; i < AG_ND_SYNC_THRESHOLD_TABLE_SIZE; ++i)
    {
        x_sync.n_reg = 0;
        x_sync.x_fields.n_packet_sync_threshold = p_msg->a_lops_threshold_table[i];
        ag_nd_reg_write(p_device, AG_REG_LOSS_OF_PACKET_SYNC_THRESHOLD_TABLE(i), x_sync.n_reg);

        x_sync.n_reg = 0;
        x_sync.x_fields.n_packet_sync_threshold = p_msg->a_aops_threshold_table[i];
        ag_nd_reg_write(p_device, AG_REG_ACQUISITION_OF_PACKET_SYNC_THRESHOLD_TABLE(i), x_sync.n_reg);
    }


    /* */
    /* HPL */
    /* */
   #ifndef CES16_BCM_VERSION
    x_hpl_cfg.n_reg = 0;

    x_hpl_cfg.x_fields.b_hpl_rx_enable = (field_t) p_msg->b_hpl_rx_enable;
    x_hpl_cfg.x_fields.b_hpl_tx_enable = (field_t) p_msg->b_hpl_tx_enable;
    x_hpl_cfg.x_fields.b_pme_export_enable = (field_t) p_msg->b_pme_enable;

    x_hpl_cfg.x_fields.n_pme_channels_1 = (field_t) (p_msg->x_pme_group[0].n_channels / 4);
    x_hpl_cfg.x_fields.n_pme_channels_2 = (field_t) (p_msg->x_pme_group[1].n_channels / 4);
    x_hpl_cfg.x_fields.n_pme_channels_3 = (field_t) (p_msg->x_pme_group[2].n_channels / 4);

    ag_nd_reg_write(p_device, AG_REG_HOST_PACKET_LINK_CONFIGURATION, x_hpl_cfg.n_reg);
   #endif /*CES16_BCM_VERSION*/

 #ifndef CES16_BCM_VERSION
    x_pme_asmb.n_reg = 0;
    x_pme_asmb.x_fields.b_auto_trigger_enable = (field_t) p_msg->x_pme_group[0].b_auto_export;
    x_pme_asmb.x_fields.n_counter_select = (field_t) p_msg->x_pme_group[0].n_counters;
    ag_nd_reg_write(p_device, AG_REG_PM_EXPORT_STRUCTURE_ASSEMBLY_REGISTER(0), x_pme_asmb.n_reg);

    x_pme_asmb.n_reg = 0;
    x_pme_asmb.x_fields.b_auto_trigger_enable = (field_t) p_msg->x_pme_group[1].b_auto_export;
    x_pme_asmb.x_fields.n_counter_select = (field_t) p_msg->x_pme_group[1].n_counters;
    ag_nd_reg_write(p_device, AG_REG_PM_EXPORT_STRUCTURE_ASSEMBLY_REGISTER(1), x_pme_asmb.n_reg);

    x_pme_asmb.n_reg = 0;
    x_pme_asmb.x_fields.b_auto_trigger_enable = (field_t) p_msg->x_pme_group[2].b_auto_export;
    x_pme_asmb.x_fields.n_counter_select = (field_t) p_msg->x_pme_group[2].n_counters;
    ag_nd_reg_write(p_device, AG_REG_PM_EXPORT_STRUCTURE_ASSEMBLY_REGISTER(2), x_pme_asmb.n_reg);
  #endif
    /* */
    /* global trimming enable */
    /* */
    x_slip_cmd.n_reg = 0;
    x_slip_cmd.x_fields.b_global_trimming_enable = p_msg->b_trimming;
    ag_nd_reg_write(p_device, AG_REG_JITTER_BUFFER_SLIP_COMMAND, x_slip_cmd.n_reg);

    /* */
    /* CW interrupt mask */
    /*  */
    ag_nd_reg_write(p_device, AG_REG_CES_CONTROL_WORD_INTERRUPT_MASK, p_msg->n_cw_mask);

    /* */
    /* LAN/WAN DCE mode  */
    /* */
    #ifndef CES16_BCM_VERSION  /*LAN/WAN Dte/Dce Remove*/
    if (AG_ND_ARCH_MASK_NEMO & p_device->n_chip_mask)
    {
        ag_nd_reg_read(p_device, AG_REG_GLOBAL_CONTROL, &(x_global.n_reg));

       /*
           ISTTFIX: set LAN bits 14 13 in fpga global (fpga version a0)
                                 0  1    for DTE
                                 1  0    for DCE
       */
       if(AG_FALSE == p_msg->b_lan_dce_mode_enable)
       {   /*LAN DTE*/
           x_global.x_fields.b_lan_dce_mode_enable_h = (field_t) 0;
           x_global.x_fields.b_lan_dce_mode_enable =   (field_t) 1;
       }
       else
       {   /*LAN DCE*/
           x_global.x_fields.b_lan_dce_mode_enable_h =  (field_t) 1;
           x_global.x_fields.b_lan_dce_mode_enable =    (field_t) 0;
       }
        x_global.x_fields.b_wan_dce_mode_enable = (field_t) p_msg->b_wan_dce_mode_enable;
        ag_nd_reg_write(p_device, AG_REG_GLOBAL_CONTROL, x_global.n_reg);
    }
   #endif
    /* */
    /* CAS */
    /*  */
    /* CAS TEMP: remove if when implemented in Nemo */
    if (1 /*AG_ND_ARCH_MASK_NEPTUNE == p_device->n_chip_mask*/)
    {
        x_cas_idle.n_reg = 0;
        x_cas_idle.x_fields.n_e1_idle_pattern = (field_t) p_msg->n_cas_e1_idle_pattern;
        x_cas_idle.x_fields.n_t1_idle_pattern = (field_t) p_msg->n_cas_t1_idle_pattern;
        ag_nd_reg_write(p_device, AG_REG_CAS_IDLE_PATTERN, x_cas_idle.n_reg);
    
        x_cas_delay.n_reg = 0;
        x_cas_delay.x_fields.n_change_delay = (field_t) p_msg->n_cas_change_delay;
        x_cas_delay.x_fields.n_no_change_delay = (field_t) p_msg->n_cas_no_change_delay;
        ag_nd_reg_write(p_device, AG_REG_TRANSMIT_CAS_PACKET_DELAY, x_cas_delay.n_reg);
    
        x_cas_win.n_reg = 0;
        x_cas_win.x_fields.n_cas_window_limit = (field_t) p_msg->n_cas_sqn_window;
        ag_nd_reg_write(p_device, AG_REG_JITTER_BUFFER_CAS_SEQUENCE_NUMBER_WINDOW, x_cas_win.n_reg);
    }
    
    /* */
    /* RAI */
    /*  */
    x_rai.n_reg = 0;

    if (p_msg->n_rai_detect & AG_ND_RAI_DETECT_R)
        x_rai.x_fields.b_r_bit_detection = (field_t) AG_TRUE;

    if (p_msg->n_rai_detect & AG_ND_RAI_DETECT_M)
        x_rai.x_fields.b_m_bit_detection = (field_t) AG_TRUE;

    ag_nd_reg_write(p_device, AG_REG_RAI_CONFIGURATION, x_rai.n_reg);

     #ifdef CES16_BCM_VERSION
      ag_nd_reg_write(p_device, AG_REG_PIF_PIFCSR, 0); 
     #endif


    return AG_S_OK;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_chip_info */
/* */
AgResult
ag_nd_opcode_read_chip_info(AgNdDevice *p_device, AgNdMsgChipInfo *p_msg)
{
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);

    p_msg->n_architecture = p_device->n_arch;
    p_msg->n_fpga_code_id = p_device->n_code_id;
    p_msg->n_fpga_revision = p_device->n_revision;
    p_msg->n_total_channels = p_device->n_total_channels;
    p_msg->n_total_ports = p_device->n_total_ports;

    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_register_access */
/* */
AgResult 
ag_nd_opcode_read_register_access(AgNdDevice *p_device, AgNdMsgRegAccess *p_msg)
{
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "addr=0x%lX\n", p_msg->n_addr);

    ag_nd_reg_read(p_device, p_msg->n_addr, &(p_msg->n_value));

    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_register_access */
/* */
AgResult 
ag_nd_opcode_write_register_access(AgNdDevice *p_device, AgNdMsgRegAccess *p_msg)
{
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "addr=%lu\n", p_msg->n_addr);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "value=%hu\n", p_msg->n_value);

    ag_nd_reg_write(p_device, p_msg->n_addr, p_msg->n_value);

    return AG_S_OK;
}

/*ORI*/
AgResult
ag_nd_opcode_write_framer(AgNdDevice *p_device,AgFramerConfig *msg)
{
	AgRegFramerCommonConfigurationLower common_config;
	AgRegFramerPortConfiguration        port_config;
	memset(&common_config,0,sizeof(AgRegFramerCommonConfigurationLower));
	memset(&port_config,0,sizeof(AgRegFramerPortConfiguration));

	if( msg->n_circuit_id < 8 )
	{
    	ag_nd_reg_read(p_device, AG_REG_FRAMER_COMMON_CONFIGURATION_LOWER, &(common_config.n_reg));
	}
	else
	{
		ag_nd_reg_read(p_device, AG_REG_FRAMER_COMMON_CONFIGURATION_UPPER, &(common_config.n_reg));
	}

    switch(msg->n_circuit_id % 8)
	{
		case(0):
			common_config.x_fields.b_port1_enable = (field_t)msg->b_enable;
		break;
		case(1):
			common_config.x_fields.b_port2_enable = (field_t)msg->b_enable;
		break;
		case(2):
			common_config.x_fields.b_port3_enable = (field_t)msg->b_enable;
		break;
		case(3):
			common_config.x_fields.b_port4_enable = (field_t)msg->b_enable;
		break;
		case(4):
			common_config.x_fields.b_port5_enable = (field_t)msg->b_enable;
		break;
		case(5):
			common_config.x_fields.b_port6_enable = (field_t)msg->b_enable;
		break;
		case(6):
			common_config.x_fields.b_port7_enable = (field_t)msg->b_enable;
		break;
		case(7):
			common_config.x_fields.b_port8_enable = (field_t)msg->b_enable;
		break;
		default:
		break;
	}
	if( msg->n_circuit_id < 8 )
	{
    	ag_nd_reg_write(p_device, AG_REG_FRAMER_COMMON_CONFIGURATION_LOWER, (common_config.n_reg));
	}
	else
	{
		ag_nd_reg_write(p_device, AG_REG_FRAMER_COMMON_CONFIGURATION_UPPER, (common_config.n_reg));
	}

    port_config.x_fields.n_master_slave_select = (field_t)msg->b_master;
	port_config.x_fields.b_rec_crc_enable  = (field_t)msg->b_recive_crc;
	port_config.x_fields.b_tran_crc_enable  = (field_t)msg->b_transmit_crc;
	port_config.x_fields.n_t1_sing_format_select = (field_t)msg->n_singling_format;
	port_config.x_fields.n_t1_j1_fram_mode = (field_t)msg->b_esf;
    ag_nd_reg_write(p_device, AG_REG_FRAMER_PORT_CONFIGURATION(msg->n_circuit_id), (port_config.n_reg));
	

    return AG_S_OK;
}
AgResult
ag_nd_opcode_read_framer(AgNdDevice *p_device,AgFramerConfig *msg)
{
	AgRegFramerCommonConfigurationLower common_comfig;
	AgRegFramerPortConfiguration        port_config;
	if( msg->n_circuit_id < 8 )
	{
    	ag_nd_reg_read(p_device, AG_REG_FRAMER_COMMON_CONFIGURATION_LOWER, &(common_comfig.n_reg));
	}
	else
	{
		ag_nd_reg_read(p_device, AG_REG_FRAMER_COMMON_CONFIGURATION_UPPER, &(common_comfig.n_reg));
	}
	ag_nd_reg_read(p_device, AG_REG_FRAMER_PORT_CONFIGURATION(msg->n_circuit_id), &(port_config.n_reg));
	switch(msg->n_circuit_id % 8)
	{
		case(0):
			msg->b_enable= common_comfig.x_fields.b_port1_enable;
		break;
		case(1):
			msg->b_enable= common_comfig.x_fields.b_port2_enable;
		break;
		case(2):
			msg->b_enable= common_comfig.x_fields.b_port3_enable;
		break;
		case(3):
			msg->b_enable= common_comfig.x_fields.b_port4_enable;
		break;
		case(4):
			msg->b_enable= common_comfig.x_fields.b_port5_enable;
		break;
		case(5):
			msg->b_enable= common_comfig.x_fields.b_port6_enable;
		break;
		case(6):
			msg->b_enable= common_comfig.x_fields.b_port7_enable;
		break;
		case(7):
			msg->b_enable= common_comfig.x_fields.b_port8_enable;
		break;
		default:
		break;
	}
	msg->b_master = port_config.x_fields.n_master_slave_select;
	msg->b_esf = port_config.x_fields.n_t1_j1_fram_mode;
	msg->n_singling_format = (AG_U8)port_config.x_fields.n_t1_sing_format_select;
	msg->b_recive_crc = port_config.x_fields.b_rec_crc_enable;
	msg->b_transmit_crc = port_config.x_fields.b_tran_crc_enable;
	return AG_S_OK;
}
AgResult
ag_nd_opcode_read_framer_port_status(AgNdDevice *p_device,AgFramerPortStatus* msg)
{
	AgRegFramerPortStatusRegister1 status_msg;
	ag_nd_reg_read(p_device,AG_REG_FRAMER_PORT_STATUS_1(msg->n_circuit_id), &(status_msg.n_reg));
	msg->b_loopback_deactivation_code = status_msg.x_fields.n_loopback_deactivation_code;
	msg->b_loopback_activation_code = status_msg.x_fields.n_loopback_activation_code;
	msg->b_sa6_change_unlatched_status = status_msg.x_fields.n_sa6_change_unlatched_status;
	msg->b_prm_status = status_msg.x_fields.n_prm_status;
	msg->b_excessive_zeroes_status = status_msg.x_fields.n_cas_ts_16_ais_status;
	msg->b_cas_ts_16_ais_status = status_msg.x_fields.n_cas_ts_16_los_status;
	msg->b_cas_ts_16_los_status = status_msg.x_fields.n_recive_bits_status;
	msg->b_recive_bits_status = status_msg.x_fields.n_recive_bits_status;
	msg->b_signal_multiframe_error_status = status_msg.x_fields.n_signal_multiframe_error_status;
	msg->b_crc_error_status = status_msg.x_fields.n_crc_error_status;
	msg->b_frame_alarm_status = status_msg.x_fields.n_frame_alarm_status;
	msg->b_remote_multiframe_error_status = status_msg.x_fields.n_remote_multiframe_error_status;
	msg->b_remote_alarm_indec_status = status_msg.x_fields.n_remote_alarm_indec_status;
	msg->b_oof_status = status_msg.x_fields.n_oof_status;
	msg->b_ais_status = status_msg.x_fields.n_ais_status;
	msg->b_los_status = status_msg.x_fields.n_los_status;
	return AG_S_OK;
}
AgResult
ag_nd_opcode_read_framer_port_alarm_control(AgNdDevice *p_device,FramerPortAlarmControl* msg)
{
	AgRegFramerPortAlarmControlRegister control_msg;
	ag_nd_reg_read(p_device,AG_REG_FRAMER_PORT_ALARM_CONTROL_REGISTER(msg->n_circuit_id), &(control_msg.n_reg));
	msg->n_t1_oof_config = control_msg.x_fields.n_t1_oof_config;
	msg->b_force_fram_alig_word_error = control_msg.x_fields.n_force_fram_alig_word_error;
	msg->b_force_bip_code = control_msg.x_fields.n_force_bip_code;
	msg->b_force_crc_error = control_msg.x_fields.n_force_crc_error;
    msg->b_rai_insert_signal_bus = control_msg.x_fields.n_rai_insert_signal_bus;
	msg->b_signal_bus_ais_los = control_msg.x_fields.n_signal_bus_ais_los;
	msg->b_signal_bus_ais_oof = control_msg.x_fields.n_signal_bus_ais_oof;
	msg->b_signal_bus_ais_ais = control_msg.x_fields.n_signal_bus_ais_ais;
	msg->b_forward_rai = control_msg.x_fields.n_forward_rai;
	msg->b_forward_ais = control_msg.x_fields.n_forward_ais;
	msg->b_force_rai = control_msg.x_fields.n_force_rai;
	msg->b_force_ais = control_msg.x_fields.n_force_ais;
	return AG_S_OK;
}
AgResult
ag_nd_opcode_write_framer_port_alarm_control(AgNdDevice *p_device,FramerPortAlarmControl* msg)
{
	AgRegFramerPortAlarmControlRegister control_msg;
	ag_nd_reg_read(p_device,AG_REG_FRAMER_PORT_ALARM_CONTROL_REGISTER(msg->n_circuit_id), &(control_msg.n_reg));
	control_msg.x_fields.n_t1_oof_config = (field_t)msg->n_t1_oof_config;
	control_msg.x_fields.n_force_fram_alig_word_error = (field_t)msg->b_force_fram_alig_word_error;
	control_msg.x_fields.n_force_bip_code = (field_t)msg->b_force_bip_code;
	control_msg.x_fields.n_force_crc_error = (field_t)msg->b_force_crc_error;
    control_msg.x_fields.n_rai_insert_signal_bus = (field_t)msg->b_rai_insert_signal_bus;
	control_msg.x_fields.n_signal_bus_ais_los = (field_t)msg->b_signal_bus_ais_los;
	control_msg.x_fields.n_signal_bus_ais_oof = (field_t)msg->b_signal_bus_ais_oof;
	control_msg.x_fields.n_signal_bus_ais_ais = (field_t)msg->b_signal_bus_ais_ais;
	control_msg.x_fields.n_forward_rai = (field_t)msg->b_forward_rai;
	control_msg.x_fields.n_forward_ais = (field_t)msg->b_forward_ais;
	control_msg.x_fields.n_force_rai = (field_t)msg->b_force_rai;
	control_msg.x_fields.n_force_ais = (field_t)msg->b_force_ais;
	ag_nd_reg_write(p_device,AG_REG_FRAMER_PORT_ALARM_CONTROL_REGISTER(msg->n_circuit_id), (control_msg.n_reg));
	return AG_S_OK;
}
AgResult
ag_nd_opcode_read_framer_port_pm(AgNdDevice *p_device,AgFramerPortPm *msg)
{
	AgRegFramerPortCRCErrorCounterShadow crc_msg;
	AgRegFramerPortLineCodeViolationErrorCounterShadow line_code_msg;
	AgRegFramerPortEBitErrorCounterShadow e_bit_msg;
	AgRegFramerPortFrameAlignmentErrorSlipCountersShadow slip_msg;
	AgRegFramerPortSeverelyErroredFrameCOFACounterShadow seve_msg;
	ag_nd_reg_read(p_device,AG_REG_FRAMER_PORT_CRC_ERROR_COUNTER_SHADOW_REGISTER(msg->n_circuit_id), &(crc_msg.n_reg));
	ag_nd_reg_read(p_device, AG_REG_FRAMER_PORT_LINE_CODE_VIOLATION_ERROR_COUNTER_SHADOW_REGISTER(msg->n_circuit_id), &(line_code_msg.n_reg));
	ag_nd_reg_read(p_device, AG_REG_FRAMER_PORT_EBIT_ERROR_COUNTER_SHADOW_REGISTER(msg->n_circuit_id), &(e_bit_msg.n_reg));
	ag_nd_reg_read(p_device, AG_REG_FRAMER_PORT_FRAME_ALIGNMENT_ERROR_SLIP_COUNTERS_SHADOW_REGISTER(msg->n_circuit_id), &(slip_msg.n_reg));
	ag_nd_reg_read(p_device, AG_REG_FRAMER_PORT_SEVERELY_ERRORED_FRAME_COFA_COUNTERS_SHADOW_REGISTER(msg->n_circuit_id), &(seve_msg.n_reg));
	msg->n_crc_error_counter = crc_msg.x_fields.n_crc_error_counter;
	msg->n_line_code_voi_error = line_code_msg.x_fields.n_line_code_voi_error;
	msg->n_e_bit_error_count = e_bit_msg.x_fields.n_e_bit_error_count;
	msg->n_transmit_slip_count = slip_msg.x_fields.n_transmit_slip_count;
	msg->n_recive_slip_count = slip_msg.x_fields.n_recive_slip_count;
	msg->n_frame_alim_error_count = slip_msg.x_fields.n_frame_alim_error_count;
	msg->n_change_framer_alim_count = seve_msg.x_fields.n_change_framer_alim_count;
	msg->n_sev_frame_error_count = seve_msg.x_fields.n_sev_frame_error_count;
	return AG_S_OK;
}
AgResult ag_nd_opcode_read_framer_port_loopback_timeslot_control(AgNdDevice *p_device,AgFramerPortTimeSlotLoopbackControl *msg)
{
	AgRegFramerPortTimeSlotLoopbackControlRegisters loopback_msg;
	ag_nd_reg_read(p_device, AG_REG_FRAMER_PORT_TIME_SLOT_LOOPBACK_CONTROL_REGISTERS1(msg->n_circuit_id), &(loopback_msg.n_reg));
	msg->n_framer_port_timeslot_control = loopback_msg.x_fields.n_time_slot_mask;
	ag_nd_reg_read(p_device, AG_REG_FRAMER_PORT_TIME_SLOT_LOOPBACK_CONTROL_REGISTERS2(msg->n_circuit_id), &(loopback_msg.n_reg));
	msg->n_framer_port_timeslot_control = msg->n_framer_port_timeslot_control | (loopback_msg.x_fields.n_time_slot_mask << 16);
	return AG_S_OK;
}
AgResult ag_nd_opcode_write_framer_port_loopback_timeslot_control(AgNdDevice *p_device,AgFramerPortTimeSlotLoopbackControl *msg)
{
	AG_U16 time_slot;
	AgRegFramerPortTimeSlotLoopbackControlRegisters loopback_msg;
	time_slot = (AG_U16)(msg->n_framer_port_timeslot_control & 0xffff);
	loopback_msg.x_fields.n_time_slot_mask = (field_t)time_slot;
	ag_nd_reg_write(p_device, AG_REG_FRAMER_PORT_TIME_SLOT_LOOPBACK_CONTROL_REGISTERS1(msg->n_circuit_id), loopback_msg.n_reg);
	time_slot = (AG_U16)(msg->n_framer_port_timeslot_control >> 16);
	loopback_msg.x_fields.n_time_slot_mask = (field_t)time_slot;
	ag_nd_reg_write(p_device, AG_REG_FRAMER_PORT_TIME_SLOT_LOOPBACK_CONTROL_REGISTERS2(msg->n_circuit_id), loopback_msg.n_reg);
	return AG_S_OK;
}

AgResult ag_nd_opcode_read_framer_port_loopback_control(AgNdDevice *p_device,AgFramerPortLoopbackControl *msg)
{
	AgRegFramerPortStatusRegister1 status_msg;
	AgFramerPortTimeSlotLoopbackControl mask;
	mask.n_circuit_id = msg->n_circuit_id;
	ag_nd_reg_read(p_device,AG_REG_FRAMER_PORT_STATUS_1(msg->n_circuit_id), &(status_msg.n_reg));
	msg->b_loopback_activation_code = status_msg.x_fields.n_loopback_activation_code;
	msg->b_loopback_deactivation_code = status_msg.x_fields.n_loopback_deactivation_code;
	ag_nd_opcode_read_framer_port_loopback_timeslot_control(p_device,&mask);
	msg->n_mask = mask.n_framer_port_timeslot_control;
	return AG_S_OK;
}

AgResult ag_nd_opcode_write_framer_port_loopback_control(AgNdDevice *p_device,AgFramerPortLoopbackControl *msg)
{
	AgRegFramerPortMaintenanceSlipControl control_msg;
	AgNdRegT1E1PortConfiguration    circuit_msg;
	AgRegFramerPortStatusRegister1 status_msg;
	AgFramerPortTimeSlotLoopbackControl mask;
	ag_nd_reg_read(p_device, AG_REG_FRAMER_PORT_MAINRENANCE_SLIP_CONTROL_REGISTERS(msg->n_circuit_id), &(control_msg.n_reg));
	ag_nd_reg_read(p_device,AG_REG_T1_E1_PORT_CONFIGURATION(msg->n_circuit_id),&circuit_msg.n_reg);
	ag_nd_reg_read(p_device,AG_REG_FRAMER_PORT_STATUS_1(msg->n_circuit_id), &(status_msg.n_reg));
	switch(msg->e_type)
	{
		case(LOOPBACK_REMOTE_SWITCH):
			if(msg->b_loop_enable)
			{
				control_msg.x_fields.b_remote_e1_t1_loopback_enable = AG_TRUE;
			}
			else
			{
				control_msg.x_fields.b_remote_e1_t1_loopback_enable = AG_FALSE;
			}
			ag_nd_reg_write(p_device, AG_REG_FRAMER_PORT_MAINRENANCE_SLIP_CONTROL_REGISTERS(msg->n_circuit_id), control_msg.n_reg);
		break;
		case(LOOPBACK_PAYLOAD_SWITCH):
			if(msg->b_loop_enable)
			{
				control_msg.x_fields.b_time_slot_loopback_enable = AG_TRUE;
			}
			else
			{
				control_msg.x_fields.b_time_slot_loopback_enable = AG_FALSE;
			}
			ag_nd_reg_write(p_device, AG_REG_FRAMER_PORT_MAINRENANCE_SLIP_CONTROL_REGISTERS(msg->n_circuit_id), control_msg.n_reg);
		break;
		case(LOOPBACK_FRAMER_SWITCH):
			if(msg->b_loop_enable)
			{
				control_msg.x_fields.b_local_e1_t1_loopback_enable = AG_TRUE;
			}
			else
			{
				control_msg.x_fields.b_local_e1_t1_loopback_enable = AG_FALSE;
			}
			ag_nd_reg_write(p_device, AG_REG_FRAMER_PORT_MAINRENANCE_SLIP_CONTROL_REGISTERS(msg->n_circuit_id),control_msg.n_reg);
		break;
		case(LOOPBACK_TX_ENABLE):
			if(msg->b_loop_enable)
			{
				circuit_msg.x_fields.b_loopback_enable = AG_TRUE;
			}
			else
			{
				circuit_msg.x_fields.b_loopback_enable = AG_FALSE;
			}
			ag_nd_reg_write(p_device, AG_REG_T1_E1_PORT_CONFIGURATION(msg->n_circuit_id), circuit_msg.n_reg);
		break;
		case(LOOPBACK_TX_DONE):
		break;
		case(LOOPBACK_RX_ACTIVATE):
		break;
		case(LOOPBACK_RX_DONE):
		break;
		default:
		break;
	}
	mask.n_circuit_id = msg->n_circuit_id;
	mask.n_framer_port_timeslot_control = msg->n_mask;
	ag_nd_opcode_write_framer_port_loopback_timeslot_control(p_device,&mask);
	return AG_S_OK;
}

