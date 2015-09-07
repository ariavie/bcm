/* $Id: nd_init.c 1.6 Broadcom SDK $
 * Copyright 2011 BATM
 */

#include "pub/nd_hw.h"
#include "nd_registers.h"
#include "nd_debug.h"
#include "nd_interrupts.h"
#include "nd_util.h"
#include "nd_rpc.h"
#include "nd_rcr.h"
#ifndef CES16_BCM_VERSION
static AgResult ag_nd_init_mm_nemo(AgNdMsgConfigInit *p_msg, AgNdDevice *p_device);
static AgResult ag_nd_init_mm_neptune(AgNdMsgConfigInit *p_msg, AgNdDevice *p_device);
#else
static AgResult ag_nd_init_mm_bcm_ces(AgNdMsgConfigInit *p_msg, AgNdDevice *p_device);
#endif
static AgResult ag_nd_init_device(AgNdDevice *p_device);


extern AG_U16 ag_nd_tx_mirror[8][8];
AgResult 
ag_nd_opcode_write_config_init(AgNdDevice *p_device, AgNdMsgConfigInit *p_msg)
{
    AgNdRegGlobalControl                 x_glb;
   #ifndef CES16_BCM_VERSION
	AgNdRegMemoryConfiguration			 x_mem_conf;
   #endif 
    AgResult                             n_ret = AG_S_OK;



    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "e_bit_order: %d\n", p_msg->e_bit_order); 
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "e_one_sec_pulse_direction: %d\n", p_msg->e_one_sec_pulse_direction); 

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_pbf_max: %lu\n", p_msg->n_pbf_max); 
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_pw_max: %lu\n", p_msg->n_pw_max); 
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_packet_max: %lu\n", p_msg->n_packet_max); 

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_isr_mode: %lu\n", p_msg->b_isr_mode); 
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_isr_task_priority: %lu\n", p_msg->n_isr_task_priority); 
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_isr_task_wakeup: %lu\n", p_msg->n_isr_task_wakeup); 

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "p_cb_cwi: %p\n", p_msg->p_cb_cwi); 
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "p_cb_pbi: %p\n", p_msg->p_cb_pbi); 
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "p_cb_pmi: %p\n", p_msg->p_cb_pmi); 
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "p_cb_psi: %p\n", p_msg->p_cb_psi); 
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "p_cb_tpi: %p\n", p_msg->p_cb_tpi); 

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_user_data_cwi: 0x%08lX\n", p_msg->n_user_data_cwi); 
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_user_data_tpi: 0x%08lX\n", p_msg->n_user_data_tpi); 
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_user_data_pbi: 0x%08lX\n", p_msg->n_user_data_pbi); 
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_user_data_psi: 0x%08lX\n", p_msg->n_user_data_psi); 

    memset(ag_nd_tx_mirror,0,sizeof(ag_nd_tx_mirror));

#ifdef AG_ND_ENABLE_VALIDATION
    if (p_msg->e_bit_order != AG_ND_TDM_PAYLOAD_BIT_ORDERING_STANDARD &&
        p_msg->e_bit_order != AG_ND_TDM_PAYLOAD_BIT_ORDERING_REDUX)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if (p_msg->e_one_sec_pulse_direction != AG_ND_ONE_SECOND_PULSE_DIRECTION_IN &&
        p_msg->e_one_sec_pulse_direction != AG_ND_ONE_SECOND_PULSE_DIRECTION_OUT) 
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
  
    if (p_msg->n_pbf_max < 64 ||
        p_msg->n_pbf_max > 8192 ||
        !ag_nd_is_power_of_2(p_msg->n_pbf_max))
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if (AG_ND_STATE_INIT == p_device->e_state)
        return AG_ND_ERR(p_device, AG_E_ND_STATE);

    if (p_msg->n_pw_max < 1 ||
        p_msg->n_pw_max > p_device->n_total_channels)
        return AG_ND_ERR(p_device, AG_E_ND_UNSUPPORTED);

    if (!p_msg->n_packet_max)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif

    p_device->b_dynamic_memory = p_msg->b_dynamic_memory;

    p_device->n_pw_max = p_msg->n_pw_max;
    p_device->n_vba_size = p_msg->n_packet_max;

    p_device->b_rcr_support = p_msg->b_rcr_support;
    p_device->b_ptp_support = p_msg->b_ptp_support;

    /*  */
    /* reset registers */
    /* */
    ag_nd_regs_reset(p_device);

    /* */
    /* initialize shadow device structures */
    /*  */
    ag_nd_init_device(p_device);

    /* */
    /* bit order, 1sec pulse, PM counters clear on read */
    /* */
    ag_nd_reg_read(p_device, AG_REG_GLOBAL_CONTROL, &(x_glb.n_reg));
    x_glb.x_fields.n_bit_order = p_msg->e_bit_order;
    x_glb.x_fields.n_one_second_pulse_direction = p_msg->e_one_sec_pulse_direction;
    x_glb.x_fields.b_rclr_on_host_access = AG_TRUE;
    ag_nd_reg_write(p_device, AG_REG_GLOBAL_CONTROL, x_glb.n_reg);

    #ifndef CES16_BCM_VERSION
	if (p_device->b_dynamic_memory)
	{
		AG_U32 n_bank_0_size = p_device->n_ext_mem_bank0_size/(1024*1024);
		AG_U32 n_bank_1_size = p_device->n_ext_mem_bank1_size/(1024*1024);
		x_mem_conf.n_reg = 0;
		if (n_bank_0_size)
			x_mem_conf.x_fields.n_bank_0_size = 4 + (ag_nd_log2(n_bank_0_size));

		if (n_bank_1_size)
			x_mem_conf.x_fields.n_bank_1_size = 4 + (ag_nd_log2(n_bank_1_size));
	
		x_mem_conf.x_fields.n_max_pw = 2 + (ag_nd_log2(p_device->n_total_channels) - 3);
		ag_nd_reg_write(p_device, AG_REG_MEMORY_CONFIGURATION_ADDRESS, x_mem_conf.n_reg);
	}
    #else
    #endif /*CES16_BCM_VERSION*/


    /* */
    /* JBF configuration restrictions */
    /*  */
    p_device->n_ring_max = ag_nd_flp2(p_device->n_vba_size / p_device->n_total_channels);

    /* */
    /* initialize the memory management stuff */
    /* */
    ag_nd_memset(p_device->a_mem_unit, 0, sizeof(p_device->a_mem_unit));
    p_device->n_mem_unit_count = 0;

    p_device->p_mem_ucode = &p_device->a_mem_unit[p_device->n_mem_unit_count++];
    p_device->p_mem_data_header = &p_device->a_mem_unit[p_device->n_mem_unit_count++];
    p_device->p_mem_lstree = &p_device->a_mem_unit[p_device->n_mem_unit_count++];
    p_device->p_mem_data_strict = &p_device->a_mem_unit[p_device->n_mem_unit_count++];
 
    /* CAS TEMP: remove if when implemented in Nemo */
    #ifndef CES16_BCM_VERSION
    if (AG_ND_ARCH_MASK_NEPTUNE == p_device->n_chip_mask) /*BCMad neptune*/
    {
    #endif
        p_device->p_mem_cas_header = &p_device->a_mem_unit[p_device->n_mem_unit_count++];
    #ifndef CES16_BCM_VERSION
    }
   #endif

/*/#ifdef AG_ND_PTP_SUPPORT */
	if (p_device->b_ptp_support)
	{
	    p_device->p_mem_ptp_header = &p_device->a_mem_unit[p_device->n_mem_unit_count++];
	    p_device->p_mem_ptp_strict = &p_device->a_mem_unit[p_device->n_mem_unit_count++];
	}
/*/#endif */

    strncpy(p_device->p_mem_ucode->a_name, "ucode", sizeof(p_device->p_mem_ucode->a_name));
    strncpy(p_device->p_mem_data_header->a_name, "data headers", sizeof(p_device->p_mem_data_header->a_name));
    strncpy(p_device->p_mem_lstree->a_name, "labels", sizeof(p_device->p_mem_lstree->a_name));
    strncpy(p_device->p_mem_data_strict->a_name, "data strict", sizeof(p_device->p_mem_data_strict->a_name));

    /* CAS TEMP: remove if when implemented in Nemo */
    #ifndef CES16_BCM_VERSION
    if (AG_ND_ARCH_MASK_NEPTUNE == p_device->n_chip_mask)  /*BCM as  neptune*/
    {
    #endif
        strncpy(p_device->p_mem_cas_header->a_name, "cas headers", sizeof(p_device->p_mem_cas_header->a_name));
     #ifndef CES16_BCM_VERSION
    }
    #endif

/*/#ifdef AG_ND_PTP_SUPPORT */
	if (p_device->b_ptp_support)
	{
	    strncpy(p_device->p_mem_ptp_header->a_name, "ptp headers", sizeof(p_device->p_mem_ptp_header->a_name));
	    strncpy(p_device->p_mem_ptp_strict->a_name, "ptp strict", sizeof(p_device->p_mem_ptp_strict->a_name));
	}
	/*/#endif */
    #ifndef CES16_BCM_VERSION
    if (AG_ND_ARCH_MASK_NEMO == p_device->n_chip_mask)
        n_ret = ag_nd_init_mm_nemo(p_msg, p_device);
    else
        n_ret = ag_nd_init_mm_neptune(p_msg, p_device);
   #else
    n_ret = ag_nd_init_mm_bcm_ces(p_msg, p_device);
   #endif
    if (AG_FAILED(n_ret))
        return AG_ND_ERR(p_device, n_ret);

    #ifndef CES16_BCM_VERSION
    /* */
    /* initialize VBA allocator */
    /*  */
	if (!p_device->b_dynamic_memory)
	{
	    ag_nd_allocator_init(
	        &p_device->x_allocator_vba, 
	        0, 
	        p_device->n_vba_size,
	        p_device->n_ring_max);
	}
    #endif	
    /* */
    /* initialize RPC module */
    /* */
    ag_nd_rpc_init(p_device);

    /* */
    /* initialize RCR module */
    /*  */
    if (p_device->b_rcr_support)
        ag_nd_rcr_init(p_device);

    /* */
    /* initialize interrupts */
    /* */
    p_device->p_cb_pmi = p_msg->p_cb_pmi;

    p_device->p_cb_pbi = p_msg->p_cb_pbi;
    p_device->n_user_data_pbi = p_msg->n_user_data_pbi;

    p_device->p_cb_cwi = p_msg->p_cb_cwi;
    p_device->n_user_data_cwi = p_msg->n_user_data_cwi;

    p_device->p_cb_psi = p_msg->p_cb_psi;
    p_device->n_user_data_psi = p_msg->n_user_data_psi;

    p_device->p_cb_tpi = p_msg->p_cb_tpi;
    p_device->n_user_data_tpi = p_msg->n_user_data_tpi;

    p_device->n_intr_task_priority = p_msg->n_isr_task_priority;
    p_device->n_intr_task_wakeup = p_msg->n_isr_task_wakeup;

    p_device->b_isr_mode = p_msg->b_isr_mode;

    ag_nd_interrupts_init(p_device);

    p_device->e_state = AG_ND_STATE_INIT;

    return n_ret;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_init_device */
/*  */
static AgResult
ag_nd_init_device(AgNdDevice *p_device)
{
    AgNdPath    e_path;

    /* */
    /* allocate timeslots info table */
    /* */
    p_device->a_ts_info_table = 
        ag_nd_mm_ram_alloc(
            p_device->p_circuit_get_max_idx(p_device) *
            AG_ND_SLOT_MAX * 
            AG_ND_PATH_MAX * 
            sizeof(AgNdTsInfo));

    /* */
    /* initialize both ingress channelizer and egress dechannelizer stuctures */
    /* */
    for (e_path = AG_ND_PATH_FIRST; e_path <= AG_ND_PATH_LAST; e_path++)
    {
        AG_U32 n_channel_id;
        AG_U32 n_circuit_idx;
        AG_U32 n_ts_idx;

        /* */
        /* initialize all channels */
        /* */
        for (n_channel_id = 0; 
              n_channel_id < p_device->n_total_channels; 
              n_channel_id++)
        {
            /* */
            /* channel disabled */
            /* */
            p_device->a_channel[n_channel_id].a_enable[e_path] = AG_FALSE;

            /* */
            /* no valid FTIP (default hardware state) */
            /* */
            AG_ND_TS_INVALIDATE(p_device->a_channel[n_channel_id].a_channelizer[e_path].x_first);

            /* */
            /* list of enabled timeslots is empty */
            /* */
            ag_nd_list_init(&p_device->a_channel[n_channel_id].a_channelizer[e_path].a_ts_list);
        }

        /* */
        /* list of disabled timeslots is empty */
        /* */
        ag_nd_list_init(&p_device->a_ts_list[e_path]);

        /* */
        /* iterate over all the circuits and all the timeslots */
        /* */
        for (n_circuit_idx = 0; 
              n_circuit_idx < p_device->p_circuit_get_max_idx(p_device); 
              n_circuit_idx++)
        {
            for (n_ts_idx = 0; n_ts_idx < AG_ND_SLOT_MAX; n_ts_idx++)
            {
                AgNdTsInfo *p_tsinfo;

                p_tsinfo = 
                    ag_nd_get_ts_info(
                        p_device, 
                        p_device->p_circuit_idx_to_id(n_circuit_idx),
                        n_ts_idx,
                        e_path);

                /* */
                /* initialize timeslot info according to default hardware state: */
                /* channel id 0 and disabled */
                /* */
                p_tsinfo->n_channel_id = 0;
                p_tsinfo->x_id.n_circuit_id = p_device->p_circuit_idx_to_id(n_circuit_idx);
                p_tsinfo->x_id.n_slot_idx = (AgNdSlot)n_ts_idx;
                p_tsinfo->b_enable = AG_FALSE;

                /* */
                /* add each timeslot to the list of disabled timeslots   */
                /* */
                ag_nd_list_add_tail(&p_device->a_ts_list[e_path], &p_tsinfo->x_list);
            }
        }
    }

    return AG_S_OK;
}


/*///////////////////////////////////////////////////////////////////////////// */
  #ifndef CES16_BCM_VERSION
/* ag_nd_init_mm_nemo */
/* */
static AgResult 
ag_nd_init_mm_nemo(AgNdMsgConfigInit *p_msg, AgNdDevice *p_device)
{
    AgNdRegTransmitHeaderAccessControl      x_tx_data_hdr_ac;
    AgNdRegTransmitCasHeaderAccessControl   x_tx_cas_hdr_ac;

/*/#ifdef AG_ND_PTP_SUPPORT */
    AgNdRegPtpTrasmitAccessControl          x_tx_ptp_hdr_ac;
/*/#endif */

    AG_U32 n_bank_0_pos;


    n_bank_0_pos = 0;

    /* */
    /* ucode: FPGA internal memory, offset 0 */
    /* */
    p_device->p_mem_ucode->b_internal = AG_TRUE;
    p_device->p_mem_ucode->n_bank = 0;
    p_device->p_mem_ucode->n_size = 0x400;
    p_device->p_mem_ucode->n_start = 0;

    /* */
    /* label search tree: fixed size of 64KB, resides in the external memory  */
    /* (need 64KB alignment) */
    /* */
    n_bank_0_pos = ag_nd_round_up2(n_bank_0_pos, AG_ND_ALIGNMENT_LSTREE);

    p_device->p_mem_lstree->n_size = 0x10000;
    if (n_bank_0_pos + p_device->p_mem_lstree->n_size > p_device->n_ext_mem_bank0_size)
        return AG_ND_ERR(0, AG_E_ND_ALLOC);

    p_device->p_mem_lstree->n_start = n_bank_0_pos;
    p_device->p_mem_lstree->b_internal = AG_FALSE;
    p_device->p_mem_lstree->n_bank = 0;

    n_bank_0_pos += p_device->p_mem_lstree->n_size;

   #ifndef CES16_BCM_VERSION
    ag_nd_reg_write(
        p_device, 
        AG_REG_LABEL_SEARCH_TABLE_BASE_ADDRESS, 
        (AG_U16) (p_device->p_mem_lstree->n_start >> 16));
   #else
   #endif
    /* */
    /* strict buffers: up to 8KB, external memory,  */
    /* right after the label search tree (need 64KB alignment too) */
    /* */
    n_bank_0_pos = ag_nd_round_up2(n_bank_0_pos, AG_ND_ALIGNMENT_STRICT);

    p_device->p_mem_data_strict->n_size = p_device->n_pw_max * 0x20;
    if (n_bank_0_pos + p_device->p_mem_data_strict->n_size > p_device->n_ext_mem_bank0_size)
        return AG_ND_ERR(0, AG_E_ND_ALLOC);

    p_device->p_mem_data_strict->n_start = n_bank_0_pos;
    p_device->p_mem_data_strict->b_internal = AG_FALSE;
    p_device->p_mem_data_strict->n_bank = 0;

    n_bank_0_pos += p_device->p_mem_data_strict->n_size;

   #ifndef CES16_BCM_VERSION
    ag_nd_reg_write(
        p_device, 
        AG_REG_STRICT_CHECK_TABLE_BASE_ADDRESS, 
        (AG_U16) (p_device->p_mem_data_strict->n_start >> 16));
   #else
   #endif
    /* */
    /* data headers: external memory, resides after lstree and strict,  */
    /* size is a function of max headers size (assumed 128) and total number of channels */
    /* (need 1KB alignment) */
    /* */
    n_bank_0_pos = ag_nd_round_up2(n_bank_0_pos, AG_ND_ALIGNMENT_HEADERS);

    p_device->p_mem_data_header->n_size = p_device->n_pw_max * 128 * 2;
    if (n_bank_0_pos + p_device->p_mem_data_header->n_size > p_device->n_ext_mem_bank0_size)
        return AG_ND_ERR(0, AG_E_ND_ALLOC);

    p_device->p_mem_data_header->n_start = n_bank_0_pos;
    p_device->p_mem_data_header->b_internal = AG_FALSE;
    p_device->p_mem_data_header->n_bank = 0;

    n_bank_0_pos += p_device->p_mem_data_header->n_size;

   #ifndef CES16_BCM_VERSION
    x_tx_data_hdr_ac.n_reg = 0;
    x_tx_data_hdr_ac.x_fields.n_header_memory_block_size = AG_ND_PACKET_HEADER_128B;
    x_tx_data_hdr_ac.x_fields.n_header_memory_base_address = (field_t)((AG_U32)p_device->p_mem_data_header->n_start >> 10);
    ag_nd_reg_write(p_device, AG_REG_TRANSMIT_HEADER_ACCESS_CONTROL, x_tx_data_hdr_ac.n_reg);
   #else
   #endif
    /* */
    /* save header memory block size in DRAM for faster access */
    /*  */
    p_device->n_data_header_memory_block_size = 1 << (x_tx_data_hdr_ac.x_fields.n_header_memory_block_size + 5);


    /* CAS TEMP: remove if when implemented in Nemo */
    if (1 /*AG_ND_ARCH_MASK_NEPTUNE == p_device->n_chip_mask*/)      /*BCM as neptune*/
    {
        /* */
        /* cas headers: external memory  */
        /* size is a function of max headers size (assumed 128) and total number of channels */
        /* (need 1KB alignment) */
        /* */
        n_bank_0_pos = ag_nd_round_up2(n_bank_0_pos, AG_ND_ALIGNMENT_HEADERS);
    
        p_device->p_mem_cas_header->n_size = p_device->n_pw_max * 128;
        if (n_bank_0_pos + p_device->p_mem_cas_header->n_size > p_device->n_ext_mem_bank0_size)
            return AG_ND_ERR(0, AG_E_ND_ALLOC);
    
        p_device->p_mem_cas_header->n_start = n_bank_0_pos;
        p_device->p_mem_cas_header->b_internal = AG_FALSE;
        p_device->p_mem_cas_header->n_bank = 0;
    
        n_bank_0_pos += p_device->p_mem_cas_header->n_size;
    
      #ifndef CES16_BCM_VERSION
        x_tx_cas_hdr_ac.n_reg = 0;
        x_tx_cas_hdr_ac.x_fields.n_header_memory_block_size = AG_ND_PACKET_HEADER_128B;
        x_tx_cas_hdr_ac.x_fields.n_header_memory_base_address = (field_t)((AG_U32)p_device->p_mem_cas_header->n_start >> AG_ND_ALIGNMENT_HEADERS);
        ag_nd_reg_write(p_device, AG_REG_TRANSMIT_HEADER_ACCESS_CONTROL, x_tx_cas_hdr_ac.n_reg);
      #else
      #endif
    }


/*/#ifdef AG_ND_PTP_SUPPORT */
	if (p_device->b_ptp_support)
	{
	    /* */
	    /* ptp headers: external memory  */
	    /* size is a function of max headers size (assumed 128) and total number of PTP channels */
	    /* (need 1KB alignment) */
	    /* */
	    n_bank_0_pos = ag_nd_round_up2(n_bank_0_pos, AG_ND_ALIGNMENT_HEADERS);

	    p_device->p_mem_ptp_header->n_size = 2 * p_device->n_pw_max * 128;
	    if (n_bank_0_pos + p_device->p_mem_ptp_header->n_size > p_device->n_ext_mem_bank0_size)
	        return AG_ND_ERR(0, AG_E_ND_ALLOC);

	    p_device->p_mem_ptp_header->n_start = n_bank_0_pos;
	    p_device->p_mem_ptp_header->b_internal = AG_FALSE;
	    p_device->p_mem_ptp_header->n_bank = 0;

	    n_bank_0_pos += p_device->p_mem_ptp_header->n_size;

       #ifndef CES16_BCM_VERSION
	    x_tx_ptp_hdr_ac.n_reg = 0;
	    x_tx_ptp_hdr_ac.x_fields.n_header_memory_block_size = AG_ND_PACKET_HEADER_128B;
	    x_tx_ptp_hdr_ac.x_fields.n_header_memory_base_address = (field_t)((AG_U32)p_device->p_mem_ptp_header->n_start >> AG_ND_ALIGNMENT_HEADERS);
	    ag_nd_reg_write(p_device, AG_REG_PTP_TRANSMIT_HEADER_ACCESS_CONTROL, x_tx_ptp_hdr_ac.n_reg);
       #else
       #endif
	    /* */
	    /* ptp strict buffers: up to 8KB, external memory,  */
	    /* need 64KB alignment  */
	    /* */
	    n_bank_0_pos = ag_nd_round_up2(n_bank_0_pos, AG_ND_ALIGNMENT_STRICT);

	    p_device->p_mem_ptp_strict->n_size = p_device->n_pw_max * 0x20;
	    if (n_bank_0_pos + p_device->p_mem_ptp_strict->n_size > p_device->n_ext_mem_bank0_size)
	        return AG_ND_ERR(0, AG_E_ND_ALLOC);

	    p_device->p_mem_ptp_strict->n_start = n_bank_0_pos;
	    p_device->p_mem_ptp_strict->b_internal = AG_FALSE;
	    p_device->p_mem_ptp_strict->n_bank = 0;

	    n_bank_0_pos += p_device->p_mem_ptp_strict->n_size;

       #ifndef CES16_BCM_VERSION
	    ag_nd_reg_write(
	        p_device, 
	        AG_REG_PTP_CHANNELS_STRICT_CHECK_TABLE_BASE_ADDRESS, 
	        (AG_U16) (p_device->p_mem_ptp_strict->n_start >> 16));
       #else
       #endif
	}
/*/#endif AG_ND_PTP_SUPPORT */

    /* */
    /* ensure that there left enough memory for payload buffers  */
    /* assuming 8KB alignment  */
    /*  */
    n_bank_0_pos = ag_nd_round_up2(n_bank_0_pos, 13);
    if (n_bank_0_pos + p_msg->n_pbf_max * p_device->n_pw_max > p_device->n_ext_mem_bank0_size)
        return AG_ND_ERR(0, AG_E_ND_ALLOC);

    /* */
    /* payload buffers and jitter buffers share the same memory  */
    /* */
    p_device->p_mem_pbf = &p_device->a_mem_unit[p_device->n_mem_unit_count++];
    strncpy(p_device->p_mem_pbf->a_name, "jbf/pbf", sizeof(p_device->p_mem_pbf->a_name));
    p_device->p_mem_jbf = p_device->p_mem_pbf;

    p_device->p_mem_pbf->n_start = n_bank_0_pos; 
    p_device->p_mem_pbf->b_internal = AG_FALSE;
    p_device->p_mem_pbf->n_bank = 0;
    p_device->p_mem_pbf->n_size = p_device->n_ext_mem_bank0_size - p_device->p_mem_pbf->n_start;


    /* */
    /* initialize payload buffer allocator */
    /*  */
    ag_nd_allocator_init(
        &p_device->x_allocator_pbf, 
        n_bank_0_pos, 
        p_device->n_pw_max * p_msg->n_pbf_max,
        p_msg->n_pbf_max);


    /* */
    /* ensure that there left enough memory for jitter buffers  */
    /* */
    n_bank_0_pos += p_device->n_pw_max * p_msg->n_pbf_max;
    n_bank_0_pos = ag_nd_round_up2(n_bank_0_pos, AG_ND_ALIGNMENT_JBF);

    if (n_bank_0_pos >= p_device->n_ext_mem_bank0_size)
        return AG_ND_ERR(0, AG_E_ND_ALLOC);


    /* */
    /* determine the available jitter buffer size */
    /* */
    p_device->n_jbf_size = (p_device->n_ext_mem_bank0_size - n_bank_0_pos) / p_device->n_pw_max;
    p_device->n_jbf_size = ag_nd_round_down2(p_device->n_jbf_size, AG_ND_ALIGNMENT_JBF);

    if (!p_device->n_jbf_size)
        return AG_ND_ERR(0, AG_E_ND_ALLOC);


    /* */
    /* initialize jitter buffer allocator */
    /* */
    ag_nd_allocator_init(
        &p_device->x_allocator_jbf, 
        n_bank_0_pos, 
        p_device->n_pw_max * p_device->n_jbf_size,
        p_device->n_jbf_size);


    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_init_mm_neptune */
/* */
static AgResult 
ag_nd_init_mm_neptune(AgNdMsgConfigInit *p_msg, AgNdDevice *p_device)
{
    AgNdRegTransmitHeaderAccessControl      x_tx_data_hdr_ac;
    AgNdRegTransmitCasHeaderAccessControl   x_tx_cas_hdr_ac;

    AG_U32  n_bank_0_pos;


    /* */
    /* ucode: internal memory, 1KB, bank 0, offset 0 */
    /* */
    p_device->p_mem_ucode->b_internal = AG_TRUE;
    p_device->p_mem_ucode->n_bank = 0;
    p_device->p_mem_ucode->n_size = 0x400;
    p_device->p_mem_ucode->n_start = 0;


    /* */
    /* label search tree: internal memory, bank 1, offset 0  */
    /* */
    p_device->p_mem_lstree->b_internal = AG_TRUE;
    p_device->p_mem_lstree->n_bank = 1;
    p_device->p_mem_lstree->n_size = 0x10000;
    p_device->p_mem_lstree->n_start = 0;


    /* */
    /* strict buffers: internal memory, bank 2, offset 0 */
    /* */
    p_device->p_mem_data_strict->b_internal = AG_TRUE;
    p_device->p_mem_data_strict->n_bank = 2;
    p_device->p_mem_data_strict->n_size = p_device->n_pw_max * 0x20;
    p_device->p_mem_data_strict->n_start = 0;


    n_bank_0_pos = 0;

    /* */
    /* data packet headers: external memory, bank 0, offset 0  */
    /* size is a function of max headers size (assumed 128) and total number of channels */
    /* (need 1KB alignment) */
    /* */
    p_device->p_mem_data_header->n_size = p_device->n_pw_max * 128 * 2;
    n_bank_0_pos = ag_nd_round_up2(n_bank_0_pos, AG_ND_ALIGNMENT_HEADERS);

    if (n_bank_0_pos + p_device->p_mem_data_header->n_size > p_device->n_ext_mem_bank0_size)
        return AG_ND_ERR(0, AG_E_ND_ALLOC);

    p_device->p_mem_data_header->n_start = n_bank_0_pos;
    p_device->p_mem_data_header->b_internal = AG_FALSE;
    p_device->p_mem_data_header->n_bank = 0;

    n_bank_0_pos += p_device->p_mem_data_header->n_size;
   
   #ifndef CES16_BCM_VERSION
    x_tx_data_hdr_ac.n_reg = 0;
    x_tx_data_hdr_ac.x_fields.n_header_memory_block_size = AG_ND_PACKET_HEADER_128B;
    x_tx_data_hdr_ac.x_fields.n_header_memory_base_address = 0;
    ag_nd_reg_write(p_device, AG_REG_TRANSMIT_HEADER_ACCESS_CONTROL, x_tx_data_hdr_ac.n_reg);
   #else
   #endif
    /* */
    /* save header memory block size in DRAM for faster access */
    /*  */
    p_device->n_data_header_memory_block_size = 1 << (x_tx_data_hdr_ac.x_fields.n_header_memory_block_size + 5);

    /* */
    /* CAS packet headers: external memory, bank 0 */
    /* size is a function of max headers size (assumed 128) and total number of channels */
    /* (need 1KB alignment) */
    /* */
    p_device->p_mem_cas_header->n_size = 128 * p_device->n_pw_max;
    n_bank_0_pos = ag_nd_round_up2(n_bank_0_pos, AG_ND_ALIGNMENT_HEADERS);

    if (n_bank_0_pos + p_device->p_mem_cas_header->n_size > p_device->n_ext_mem_bank0_size)
        return AG_ND_ERR(0, AG_E_ND_ALLOC);

    p_device->p_mem_cas_header->n_start = n_bank_0_pos;
    p_device->p_mem_cas_header->b_internal = AG_FALSE;
    p_device->p_mem_cas_header->n_bank = 0;

    n_bank_0_pos += p_device->p_mem_cas_header->n_size;

   #ifndef CES16_BCM_VERSION
    x_tx_cas_hdr_ac.n_reg = 0;
    x_tx_cas_hdr_ac.x_fields.n_header_memory_block_size = AG_ND_PACKET_HEADER_128B;
    x_tx_cas_hdr_ac.x_fields.n_header_memory_base_address = (field_t)((AG_U32)p_device->p_mem_cas_header->n_start >> AG_ND_ALIGNMENT_HEADERS);
    ag_nd_reg_write(p_device, AG_REG_TRANSMIT_CAS_HEADER_ACCESS_CONTROL, x_tx_cas_hdr_ac.n_reg);
   #else
   #endif
    /* */
    /* ensure that there left enough memory for payload buffers  */
    /* assuming 8KB alignment (according to max PBF size) */
    /*  */
    n_bank_0_pos = ag_nd_round_up2(n_bank_0_pos, 13);
    if (n_bank_0_pos + p_msg->n_pbf_max * p_device->n_pw_max > p_device->n_ext_mem_bank0_size)
        return AG_ND_ERR(0, AG_E_ND_ALLOC);

    /* */
    /* payload buffers and headers share bank 0  */
    /* */
    p_device->p_mem_pbf = &p_device->a_mem_unit[p_device->n_mem_unit_count++];
    strncpy(p_device->p_mem_pbf->a_name, "pbf", sizeof(p_device->p_mem_pbf->a_name));

    p_device->p_mem_pbf->b_internal = AG_FALSE;
    p_device->p_mem_pbf->n_bank = 0;
    p_device->p_mem_pbf->n_start = n_bank_0_pos;
    p_device->p_mem_pbf->n_size = p_device->n_ext_mem_bank0_size - n_bank_0_pos;


    /* */
    /* jitter buffers occupy whole bank 1 */
    /* */
    p_device->p_mem_jbf = &p_device->a_mem_unit[p_device->n_mem_unit_count++];
    strncpy(p_device->p_mem_jbf->a_name, "jbf", sizeof(p_device->p_mem_jbf->a_name));

    p_device->p_mem_jbf->b_internal = AG_FALSE;
    p_device->p_mem_jbf->n_bank = 1;
    p_device->p_mem_jbf->n_start = 0;
    p_device->p_mem_jbf->n_size = p_device->n_ext_mem_bank1_size;

    p_device->n_jbf_size = p_device->n_ext_mem_bank1_size / p_device->n_pw_max;
    p_device->n_jbf_size = ag_nd_round_down2(p_device->n_jbf_size, AG_ND_ALIGNMENT_JBF);

    /* */
    /* initialize the JBF/PBF allocators  */
    /*  */
    ag_nd_allocator_init(
        &p_device->x_allocator_jbf, 
        p_device->p_mem_jbf->n_start, 
        p_device->p_mem_jbf->n_size,
        p_device->n_jbf_size);

    ag_nd_allocator_init(
        &p_device->x_allocator_pbf, 
        p_device->p_mem_pbf->n_start, 
        p_device->p_mem_pbf->n_size,
        p_msg->n_pbf_max);

    return AG_S_OK;
}
#else
/* ag_nd_init_mm_bcm_ces */
/* */
static AgResult 
ag_nd_init_mm_bcm_ces(AgNdMsgConfigInit *p_msg, AgNdDevice *p_device)
{
    AgNdRegTransmitHeaderAccessControl      x_tx_data_hdr_ac;

    AG_U32  n_bank_0_pos;


    /* */
    /* ucode: internal memory, 1KB, bank 0, offset 0 */
    /* */
    p_device->p_mem_ucode->b_internal = AG_TRUE;
    p_device->p_mem_ucode->n_bank = 0;
    p_device->p_mem_ucode->n_size = 0x400;
    p_device->p_mem_ucode->n_start = 0;


    /* */
    /* label search tree: internal memory, bank 1, offset 0  */
    /* */
    p_device->p_mem_lstree->b_internal = AG_TRUE;
    p_device->p_mem_lstree->n_bank = 1;
    p_device->p_mem_lstree->n_size = 0x2000; /*8k*/
    p_device->p_mem_lstree->n_start = 0;


    /* */
    /* strict buffers: internal memory, bank 2, offset 0 */
    /* */
    p_device->p_mem_data_strict->b_internal = AG_TRUE;
    p_device->p_mem_data_strict->n_bank = 2;
    p_device->p_mem_data_strict->n_size = p_device->n_pw_max * 0x20;
    p_device->p_mem_data_strict->n_start = 0;


    n_bank_0_pos = 0;

    /* */
    /* data packet headers: internal, bank 0, offset 0  */
    /* size is a function of max headers size (assumed 128) and total number of channels */
    /* (need 1KB alignment) */
    /* */
    p_device->p_mem_data_header->n_size = p_device->n_pw_max * 128 * 2;

    p_device->p_mem_data_header->n_start = 0x10000;
    p_device->p_mem_data_header->b_internal = AG_TRUE;
    p_device->p_mem_data_header->n_bank = 4;

    n_bank_0_pos += p_device->p_mem_data_header->n_size;
   
    x_tx_data_hdr_ac.n_reg = 0;
    x_tx_data_hdr_ac.x_fields.n_header_memory_block_size = AG_ND_PACKET_HEADER_128B;
    x_tx_data_hdr_ac.x_fields.n_header_memory_base_address = 0;
    /* */
    /* save header memory block size in DRAM for faster access */
    /*  */
    p_device->n_data_header_memory_block_size = 1 << (x_tx_data_hdr_ac.x_fields.n_header_memory_block_size + 5);

    /* */
    /* CAS packet headers: external memory, bank 0 */
    /* size is a function of max headers size (assumed 128) and total number of channels */
    /* (need 1KB alignment) */
    /* */
    p_device->p_mem_cas_header->n_size = 128 * p_device->n_pw_max;

    p_device->p_mem_cas_header->n_start = 0x14000;
    p_device->p_mem_cas_header->b_internal = AG_TRUE;
    p_device->p_mem_cas_header->n_bank = 4;


    /* */
    /* ensure that there left enough memory for payload buffers  */
    /* assuming 8KB alignment (according to max PBF size) */
    /*  */
    /* */
    /* payload buffers and headers share bank 0  */
    /* */
    p_device->p_mem_pbf = &p_device->a_mem_unit[p_device->n_mem_unit_count++];
    strncpy(p_device->p_mem_pbf->a_name, "pbf", sizeof(p_device->p_mem_pbf->a_name));

    p_device->p_mem_pbf->b_internal = AG_TRUE;
    p_device->p_mem_pbf->n_bank = 4;
    p_device->p_mem_pbf->n_start = 0;
    p_device->p_mem_pbf->n_size = 10000;  /*96K*/

    /*ag_nd_debug_mu_print(p_device); BCM*/
    
    #ifndef CES16_BCM_VERSION
    /* */
    /* jitter buffers occupy whole bank 1 */
    /* */
    p_device->p_mem_jbf = &p_device->a_mem_unit[p_device->n_mem_unit_count++];
    strncpy(p_device->p_mem_jbf->a_name, "jbf", sizeof(p_device->p_mem_jbf->a_name));

    p_device->p_mem_jbf->b_internal = AG_FALSE;
    p_device->p_mem_jbf->n_bank = 1;
    p_device->p_mem_jbf->n_start = 0;
    p_device->p_mem_jbf->n_size = p_device->n_ext_mem_bank1_size;

    p_device->n_jbf_size = p_device->n_ext_mem_bank1_size / p_device->n_pw_max;
    p_device->n_jbf_size = ag_nd_round_down2(p_device->n_jbf_size, AG_ND_ALIGNMENT_JBF);
    /* */
    /* initialize the JBF/PBF allocators  */
    /*  */
    ag_nd_allocator_init(
        &p_device->x_allocator_jbf, 
        p_device->p_mem_jbf->n_start, 
        p_device->p_mem_jbf->n_size,
        p_device->n_jbf_size);

    ag_nd_allocator_init(
        &p_device->x_allocator_pbf, 
        p_device->p_mem_pbf->n_start, 
        p_device->p_mem_pbf->n_size,
        p_msg->n_pbf_max);
    #endif
    
   /* ag_nd_debug_mu_print(p_device); BCMout */

    return AG_S_OK;
}

#endif



