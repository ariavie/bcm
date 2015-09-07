/* $Id: nd_rpc.c 1.6 Broadcom SDK $
 * Copyright 2011 BATM
 */

#include "pub/nd_hw.h"
#include "nd_registers.h"
#include "nd_rpc.h"
#include "nd_debug.h"
#include "nd_ucode.h"


/*BCM: temporary pacth to initalize classifyer memory*/
/*The following number were copyed from CES application */

AgResult bcm_ag_clsb_cfg(void);


/*///////////////////////////////////////////////////////////////////////////// */
/* initiazes RPC: ucode */
/*   */
void
ag_nd_rpc_init(AgNdDevice *p_device)
{
    AgResult n_ret;
    AgNdRegGlobalControl x_glb_ctl;
    /* fprintf(stderr, "\n\rag_nd_rpc_init IN"); BCMdebug*/
    /*BCMdebugag_nd_debug_mu_print(p_device); */

    /* */
    /* load ucode */
    /* */
    ag_nd_reg_read(p_device, AG_REG_GLOBAL_CONTROL, &(x_glb_ctl.n_reg));
    x_glb_ctl.x_fields.b_ucode_memory_locked = (field_t) AG_FALSE;
    ag_nd_reg_write(p_device, AG_REG_GLOBAL_CONTROL, x_glb_ctl.n_reg);

    ag_nd_mem_write(p_device, p_device->p_mem_ucode, 0, nNdUcodeInstructions, nNdUcodeSize * 4);

    x_glb_ctl.x_fields.b_ucode_memory_locked = (field_t) AG_TRUE;
    ag_nd_reg_write(p_device, AG_REG_GLOBAL_CONTROL, x_glb_ctl.n_reg);


   #if 1
    /*Bcm initalize classifier memory, instead of initalizing it by CES application*/
     bcm_ag_clsb_cfg();   /*Will initalize memory only if not initialized*/
   #endif
    /* */
    /* create miniprogram for lable search tree */
    /* */
    ag_nd_mem_unit_set(p_device, p_device->p_mem_lstree);

    n_ret = ag_clsb_create_hw_mem(
        (void*)(p_device),
        (void*)(p_device->p_mem_lstree->n_start + p_device->n_base + 0x800000),
        p_device->p_mem_lstree->n_size,
        AG_CLS_BIT_MODE_8,
        &(p_device->n_lstree_handle));
    
    if (AG_FAILED(n_ret)) {
        assert(0);
        return;
    }

    n_ret = ag_clsb_create_std_prg(
        p_device->n_lstree_handle,
        0xFF,
        AG_CLSB_OPC8_CLASSIFY,
        &(p_device->n_lstree_mpid),
        AG_CLSB_STATIC_PRG);

    if (AG_FAILED(n_ret)) {
        assert(0);
    }

}


/*///////////////////////////////////////////////////////////////////////////// */
/* writes one status evaluation policy register */
/* */
void
ag_nd_rpc_sevpm_reg_write(
    AgNdDevice      *p_device,
    AG_U32          n_address,   
    AG_U32          n_value)    
{
    ag_nd_reg_write(p_device, n_address, AG_ND_HIGH16(n_value));
    ag_nd_reg_write(p_device, n_address + 2, AG_ND_LOW16(n_value));
}

/*///////////////////////////////////////////////////////////////////////////// */
/* reads one status evaluation policy register */
/* */
void
ag_nd_rpc_sevpm_reg_read(
    AgNdDevice      *p_device,
    AG_U32          n_address,   
    AG_U32          *p_value)    
{
    AG_U16  n_hi;
    AG_U16  n_lo;

    ag_nd_reg_read(p_device, n_address, &n_hi);
    ag_nd_reg_read(p_device, n_address + 2, &n_lo);

    *p_value = AG_ND_MAKE32LH(n_lo, n_hi);
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_config_rpc_policy */
/* */
AgResult 
ag_nd_opcode_read_config_rpc_policy(AgNdDevice *p_device, AgNdMsgConfigRpcPolicy *p_msg)
{
    
    AgNdRegPrioterizedCounterSelectionMask  x_cnt_mask;
    AgNdRegGlobalCounterPolicySelection     x_cnt_policy;

    /* */
    /* load policy matrix */
    /* */
    ag_nd_rpc_sevpm_reg_read(p_device, AG_REG_POLICY_STATUS_POLARITY(0),        &(p_msg->x_policy_matrix.n_status_polarity));
    ag_nd_rpc_sevpm_reg_read(p_device, AG_REG_POLICY_DROP_UNCONDITIONAL(0),     &(p_msg->x_policy_matrix.n_drop_unconditional));

#ifndef CES16_BCM_VERSION
/*/#ifdef AG_ND_PTP_SUPPORT */
	if (p_device->b_ptp_support)
	    ag_nd_rpc_sevpm_reg_read(p_device, AG_REG_POLICY_PTP(0),                    &(p_msg->x_policy_matrix.n_forward_to_ptp));
/*/#endif */
#endif
#if 1
    ag_nd_rpc_sevpm_reg_read(p_device, AG_REG_POLICY_DROP_IF_NOT_FORWARD(0),    &(p_msg->x_policy_matrix.n_drop_if_not_forward));
    ag_nd_rpc_sevpm_reg_read(p_device, AG_REG_POLICY_FORWARD_HIGH(0),           &(p_msg->x_policy_matrix.n_forward_to_host_high));
    ag_nd_rpc_sevpm_reg_read(p_device, AG_REG_POLICY_FORWARD_LOW(0),            &(p_msg->x_policy_matrix.n_forward_to_host_low));

    ag_nd_rpc_sevpm_reg_read(p_device, AG_REG_POLICY_PER_CHANNEL_COUNTERS(0),   &(p_msg->x_policy_matrix.n_channel_counter_1));
    ag_nd_rpc_sevpm_reg_read(p_device, AG_REG_POLICY_PER_CHANNEL_COUNTERS(2),   &(p_msg->x_policy_matrix.n_channel_counter_2));
    ag_nd_rpc_sevpm_reg_read(p_device, AG_REG_POLICY_PER_CHANNEL_COUNTERS(4),   &(p_msg->x_policy_matrix.n_channel_counter_3));
    ag_nd_rpc_sevpm_reg_read(p_device, AG_REG_POLICY_PER_CHANNEL_COUNTERS(6),   &(p_msg->x_policy_matrix.n_channel_counter_4));

    ag_nd_rpc_sevpm_reg_read(p_device, AG_REG_POLICY_GLOBAL_COUNTERS(0),        &(p_msg->x_policy_matrix.n_global_counter_1));
    ag_nd_rpc_sevpm_reg_read(p_device, AG_REG_POLICY_GLOBAL_COUNTERS(2),        &(p_msg->x_policy_matrix.n_global_counter_2));
    ag_nd_rpc_sevpm_reg_read(p_device, AG_REG_POLICY_GLOBAL_COUNTERS(4),        &(p_msg->x_policy_matrix.n_global_counter_3));
    ag_nd_rpc_sevpm_reg_read(p_device, AG_REG_POLICY_GLOBAL_COUNTERS(6),        &(p_msg->x_policy_matrix.n_global_counter_4));
    ag_nd_rpc_sevpm_reg_read(p_device, AG_REG_POLICY_GLOBAL_COUNTERS(8),        &(p_msg->x_policy_matrix.n_global_counter_5));
    ag_nd_rpc_sevpm_reg_read(p_device, AG_REG_POLICY_GLOBAL_COUNTERS(10),       &(p_msg->x_policy_matrix.n_global_counter_6));
    ag_nd_rpc_sevpm_reg_read(p_device, AG_REG_POLICY_GLOBAL_COUNTERS(12),       &(p_msg->x_policy_matrix.n_global_counter_7));
    ag_nd_rpc_sevpm_reg_read(p_device, AG_REG_POLICY_GLOBAL_COUNTERS(14),       &(p_msg->x_policy_matrix.n_global_counter_8));


    /* */
    /* counters prioterization */
    /* */
    ag_nd_reg_read(p_device, AG_REG_PRIORITIZED_COUNTER_SELECTION_MASK, &(x_cnt_mask.n_reg));

    p_msg->a_gc_prioritized[0] = x_cnt_mask.x_fields.b_global_1_priority_enable;
    p_msg->a_gc_prioritized[1] = x_cnt_mask.x_fields.b_global_2_priority_enable;
    p_msg->a_gc_prioritized[2] = x_cnt_mask.x_fields.b_global_3_priority_enable;
    p_msg->a_gc_prioritized[3] = x_cnt_mask.x_fields.b_global_4_priority_enable;
    p_msg->a_gc_prioritized[4] = x_cnt_mask.x_fields.b_global_5_priority_enable;
    p_msg->a_gc_prioritized[5] = x_cnt_mask.x_fields.b_global_6_priority_enable;
    p_msg->a_gc_prioritized[6] = x_cnt_mask.x_fields.b_global_7_priority_enable;
    p_msg->a_gc_prioritized[7] = x_cnt_mask.x_fields.b_global_8_priority_enable;

    p_msg->a_cc_prioritized[0] = x_cnt_mask.x_fields.b_per_channel_1_priority_enable;
    p_msg->a_cc_prioritized[1] = x_cnt_mask.x_fields.b_per_channel_2_priority_enable;
    p_msg->a_cc_prioritized[2] = x_cnt_mask.x_fields.b_per_channel_3_priority_enable;
    p_msg->a_cc_prioritized[3] = x_cnt_mask.x_fields.b_per_channel_4_priority_enable;

    /* */
    /* global counters incrementing policy */
    /*  */
    ag_nd_reg_read(p_device, AG_REG_GLOBAL_COUNTER_POLICY_SELECTION, &(x_cnt_policy.n_reg));
    p_msg->a_gc_cnt_policy[0] = x_cnt_policy.x_fields.n_global_1_policy_selection;
    p_msg->a_gc_cnt_policy[1] = x_cnt_policy.x_fields.n_global_2_policy_selection;
    p_msg->a_gc_cnt_policy[2] = x_cnt_policy.x_fields.n_global_3_policy_selection;
    p_msg->a_gc_cnt_policy[3] = x_cnt_policy.x_fields.n_global_4_policy_selection;
    p_msg->a_gc_cnt_policy[4] = x_cnt_policy.x_fields.n_global_5_policy_selection;
    p_msg->a_gc_cnt_policy[5] = x_cnt_policy.x_fields.n_global_6_policy_selection;
    p_msg->a_gc_cnt_policy[6] = x_cnt_policy.x_fields.n_global_7_policy_selection;
    p_msg->a_gc_cnt_policy[7] = x_cnt_policy.x_fields.n_global_8_policy_selection;
#endif /*CES16_BCM_VERSION*/
    return AG_S_OK;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_config_rpc_policy */
/* */
AgResult 
ag_nd_opcode_write_config_rpc_policy(AgNdDevice *p_device, AgNdMsgConfigRpcPolicy *p_msg)
{
    AgNdRegPrioterizedCounterSelectionMask  x_cnt_mask;
    AgNdRegGlobalCounterPolicySelection     x_cnt_policy;

    /* */
    /* load policy matrix */
    /* */
    ag_nd_rpc_sevpm_reg_write(p_device, AG_REG_POLICY_STATUS_POLARITY(0),        p_msg->x_policy_matrix.n_status_polarity);
    ag_nd_rpc_sevpm_reg_write(p_device, AG_REG_POLICY_DROP_UNCONDITIONAL(0),     p_msg->x_policy_matrix.n_drop_unconditional);

#ifndef CES16_BCM_VERSION
/*/#ifdef AG_ND_PTP_SUPPORT */
	if (p_device->b_ptp_support)
	    ag_nd_rpc_sevpm_reg_write(p_device, AG_REG_POLICY_PTP(0),                    p_msg->x_policy_matrix.n_forward_to_ptp);
/*/#endif */
    #endif
#if 1 /*ndef CES16_BCM_VERSION*/
    ag_nd_rpc_sevpm_reg_write(p_device, AG_REG_POLICY_DROP_IF_NOT_FORWARD(0),    p_msg->x_policy_matrix.n_drop_if_not_forward);
    ag_nd_rpc_sevpm_reg_write(p_device, AG_REG_POLICY_FORWARD_HIGH(0),           p_msg->x_policy_matrix.n_forward_to_host_high);
    ag_nd_rpc_sevpm_reg_write(p_device, AG_REG_POLICY_FORWARD_LOW(0),            p_msg->x_policy_matrix.n_forward_to_host_low);

    ag_nd_rpc_sevpm_reg_write(p_device, AG_REG_POLICY_PER_CHANNEL_COUNTERS(0),   p_msg->x_policy_matrix.n_channel_counter_1);
    ag_nd_rpc_sevpm_reg_write(p_device, AG_REG_POLICY_PER_CHANNEL_COUNTERS(2),   p_msg->x_policy_matrix.n_channel_counter_2);
    ag_nd_rpc_sevpm_reg_write(p_device, AG_REG_POLICY_PER_CHANNEL_COUNTERS(4),   p_msg->x_policy_matrix.n_channel_counter_3);
    ag_nd_rpc_sevpm_reg_write(p_device, AG_REG_POLICY_PER_CHANNEL_COUNTERS(6),   p_msg->x_policy_matrix.n_channel_counter_4);

    ag_nd_rpc_sevpm_reg_write(p_device, AG_REG_POLICY_GLOBAL_COUNTERS(0),        p_msg->x_policy_matrix.n_global_counter_1);
    ag_nd_rpc_sevpm_reg_write(p_device, AG_REG_POLICY_GLOBAL_COUNTERS(2),        p_msg->x_policy_matrix.n_global_counter_2);
    ag_nd_rpc_sevpm_reg_write(p_device, AG_REG_POLICY_GLOBAL_COUNTERS(4),        p_msg->x_policy_matrix.n_global_counter_3);
    ag_nd_rpc_sevpm_reg_write(p_device, AG_REG_POLICY_GLOBAL_COUNTERS(6),        p_msg->x_policy_matrix.n_global_counter_4);
    ag_nd_rpc_sevpm_reg_write(p_device, AG_REG_POLICY_GLOBAL_COUNTERS(8),        p_msg->x_policy_matrix.n_global_counter_5);
    ag_nd_rpc_sevpm_reg_write(p_device, AG_REG_POLICY_GLOBAL_COUNTERS(10),       p_msg->x_policy_matrix.n_global_counter_6);
    ag_nd_rpc_sevpm_reg_write(p_device, AG_REG_POLICY_GLOBAL_COUNTERS(12),       p_msg->x_policy_matrix.n_global_counter_7);
    ag_nd_rpc_sevpm_reg_write(p_device, AG_REG_POLICY_GLOBAL_COUNTERS(14),       p_msg->x_policy_matrix.n_global_counter_8);

    /* */
    /* counters prioterization */
    /* */
    x_cnt_mask.n_reg = 0;

    x_cnt_mask.x_fields.b_global_1_priority_enable = (field_t) p_msg->a_gc_prioritized[0];
    x_cnt_mask.x_fields.b_global_2_priority_enable = (field_t) p_msg->a_gc_prioritized[1];
    x_cnt_mask.x_fields.b_global_3_priority_enable = (field_t) p_msg->a_gc_prioritized[2];
    x_cnt_mask.x_fields.b_global_4_priority_enable = (field_t) p_msg->a_gc_prioritized[3];
    x_cnt_mask.x_fields.b_global_5_priority_enable = (field_t) p_msg->a_gc_prioritized[4];
    x_cnt_mask.x_fields.b_global_6_priority_enable = (field_t) p_msg->a_gc_prioritized[5];
    x_cnt_mask.x_fields.b_global_7_priority_enable = (field_t) p_msg->a_gc_prioritized[6];
    x_cnt_mask.x_fields.b_global_8_priority_enable = (field_t) p_msg->a_gc_prioritized[7];

    x_cnt_mask.x_fields.b_per_channel_1_priority_enable = (field_t) p_msg->a_cc_prioritized[0];
    x_cnt_mask.x_fields.b_per_channel_2_priority_enable = (field_t) p_msg->a_cc_prioritized[1];
    x_cnt_mask.x_fields.b_per_channel_3_priority_enable = (field_t) p_msg->a_cc_prioritized[2];
    x_cnt_mask.x_fields.b_per_channel_4_priority_enable = (field_t) p_msg->a_cc_prioritized[3];

    ag_nd_reg_write(p_device, AG_REG_PRIORITIZED_COUNTER_SELECTION_MASK, x_cnt_mask.n_reg);

    /* */
    /* global counters incrementing policy */
    /*  */
    x_cnt_policy.n_reg = 0;
     
    x_cnt_policy.x_fields.n_global_1_policy_selection = p_msg->a_gc_cnt_policy[0];
    x_cnt_policy.x_fields.n_global_2_policy_selection = p_msg->a_gc_cnt_policy[1];
    x_cnt_policy.x_fields.n_global_3_policy_selection = p_msg->a_gc_cnt_policy[2];
    x_cnt_policy.x_fields.n_global_4_policy_selection = p_msg->a_gc_cnt_policy[3];
    x_cnt_policy.x_fields.n_global_5_policy_selection = p_msg->a_gc_cnt_policy[4];
    x_cnt_policy.x_fields.n_global_6_policy_selection = p_msg->a_gc_cnt_policy[5];
    x_cnt_policy.x_fields.n_global_7_policy_selection = p_msg->a_gc_cnt_policy[6];
    x_cnt_policy.x_fields.n_global_8_policy_selection = p_msg->a_gc_cnt_policy[7];

    ag_nd_reg_write(p_device, AG_REG_GLOBAL_COUNTER_POLICY_SELECTION, x_cnt_policy.n_reg);
#else
    ag_nd_reg_write(p_device, 0xb3a , 0x0887); 
    ag_nd_reg_write(p_device, 0xb3c , 0xffff);    /*Polarity*/
    ag_nd_reg_write(p_device, 0xb3e , 0xffff);    /*Polarity*/
    ag_nd_reg_write(p_device, 0xb40 , 0xf0fe);   /*Hardware mask*/
    ag_nd_reg_write(p_device, 0xb42 , 0x7fe0);   /*ucode mask*/

#endif /*CES16_BCM_VERSION*/

    return AG_S_OK;
}


/* */
/* classification path builder functions array */
/* indexed by AgNdEncapsulation */
/* */
static AgNdPathBuilder a_path_builder[AG_ND_ENCAPSULATION_MAX] = 
{
    ag_nd_rpc_build_udp_path,   /* AG_ND_ENCAPSULATION_IP */
    ag_nd_rpc_build_mpls_path,  /* AG_ND_ENCAPSULATION_MPLS */
    ag_nd_rpc_build_ecid_path,  /* AG_ND_ENCAPSULATION_ETH */
    ag_nd_rpc_build_ptp_path,   /* AG_ND_ENCAPSULATION_PTP_IP */
    ag_nd_rpc_build_ptp_path,    /* AG_ND_ENCAPSULATION_PTP_EHT (same func as for PTP_IP) */
    ag_nd_rpc_build_l2tp_path  /*AG_ND_ENCAPSULATION_L2TP  ORI*/
};



/*///////////////////////////////////////////////////////////////////////////// */
/* builds the classification path for ECID case */
/* */
void
ag_nd_rpc_build_ecid_path(AgNdLsTreePath *p_path, AgNdClsData *p_cls_data)
{
    AG_U32 n_label = p_cls_data->n_label;


    assert(0 == (n_label & 0xfff00000)); /* ensure that label is 20 bits */


    n_label |= AG_ND_RPC_LSTREE_HIGH_NIBBLE_ECID << 20; /* pad with msb ECID nibble */


    p_path->a_data[0] = (AG_U8) (n_label >> 16);  
    p_path->a_opcode[0]  = AG_CLSB_OPC8_CLASSIFY; 

    p_path->a_data[1] = (AG_U8) (n_label >> 8);  
    p_path->a_opcode[1]  = AG_CLSB_OPC8_CLASSIFY;

    p_path->a_data[2] = (AG_U8) (n_label);     
    p_path->a_opcode[2]  = AG_CLSB_OPC8_TAG;

    p_path->n_size = 3;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* builds the classification path for MPLS case */
/* */
void
ag_nd_rpc_build_mpls_path(AgNdLsTreePath *p_path, AgNdClsData *p_cls_data)
{
    AG_U32 n_label = p_cls_data->n_label;


    assert(0 == (n_label & 0xfff00000)); /* ensure that label is 20 bits */


    n_label |= AG_ND_RPC_LSTREE_HIGH_NIBBLE_MPLS << 20; /* pad with msb MPLS nibble */


    p_path->a_data[0] = (AG_U8) (n_label >> 16);  
    p_path->a_opcode[0]  = AG_CLSB_OPC8_CLASSIFY; 

    p_path->a_data[1] = (AG_U8) (n_label >> 8);  
    p_path->a_opcode[1]  = AG_CLSB_OPC8_CLASSIFY;

    p_path->a_data[2] = (AG_U8) (n_label);     
    p_path->a_opcode[2]  = AG_CLSB_OPC8_TAG;

    p_path->n_size = 3;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* builds the classification path for L2TP label case */
/* */

/*ORI*/
/*CAHNGED L2TP */
/*opcode of  classification must be msb if size is odd the a_opcode prev to last must be AG_CLSB_OPC8_HIGH_NIBBLE */
void
ag_nd_rpc_build_l2tp_path(AgNdLsTreePath *p_path, AgNdClsData *p_cls_data)
{
    AG_U32 n_label = p_cls_data->n_label;

	p_path->a_data[0] = (AG_U8) (AG_ND_RPC_LSTREE_HIGH_NIBBLE_L2TP<<4)|(n_label >> 28);  
    p_path->a_opcode[0]  = AG_CLSB_OPC8_CLASSIFY; 

    p_path->a_data[1] = (AG_U8) ((n_label<<4) >> 24);  
    p_path->a_opcode[1]  = AG_CLSB_OPC8_CLASSIFY; 

    p_path->a_data[2] = (AG_U8) ((n_label<<12) >> 24);  
    p_path->a_opcode[2]  = AG_CLSB_OPC8_CLASSIFY;

    p_path->a_data[3] = (AG_U8) ((n_label<<20)>> 24);    
    p_path->a_opcode[3]  = AG_CLSB_OPC8_HIGH_NIBBLE;

    p_path->a_data[4] = (AG_U8) ((n_label<<28)>>24);     
    p_path->a_opcode[4]  = AG_CLSB_OPC8_TAG;

    p_path->n_size = 5;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* builds the classification path for UDP case */
/* */
void
ag_nd_rpc_build_udp_path(AgNdLsTreePath *p_path, AgNdClsData *p_cls_data)
{
    AG_U32 n_label = p_cls_data->n_label;


    assert(0 == (n_label & 0xffff0000)); /* ensure that label is 16 bit */


    n_label <<= 4;                                      /* shift left one nibble */
    n_label |= AG_ND_RPC_LSTREE_HIGH_NIBBLE_UDP << 20;  /* pad with msb UDP nibble */


    p_path->a_data[0] = (AG_U8) (n_label >> 16);  
    p_path->a_opcode[0]  = AG_CLSB_OPC8_CLASSIFY; 

    p_path->a_data[1] = (AG_U8) (n_label >> 8);  
    p_path->a_opcode[1]  = AG_CLSB_OPC8_HIGH_NIBBLE; 

    p_path->a_data[2] = (AG_U8) (n_label);
    p_path->a_opcode[2]  = AG_CLSB_OPC8_TAG;

    p_path->n_size = 3;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* builds the classification path for PTP label case */
/* */
void
ag_nd_rpc_build_ptp_path(AgNdLsTreePath *p_path, AgNdClsData *p_cls_data)
{
    AG_U32  i;
    AG_U8   *p_ptp_port = p_cls_data->p_ptp_port;


    for (i = 1; i <= 9; i++)
    {
        p_path->a_data[i] = (p_ptp_port[i-1] << 4) | (p_ptp_port[i] >> 4);
        p_path->a_opcode[i] = AG_CLSB_OPC8_CLASSIFY;
    }

    p_path->a_data[0] = (AG_ND_RPC_LSTREE_HIGH_NIBBLE_PTP << 4) | (p_ptp_port[0] >> 4);
    p_path->a_opcode[0] = AG_CLSB_OPC8_CLASSIFY;

    p_path->a_opcode[9] = AG_CLSB_OPC8_HIGH_NIBBLE;

    p_path->a_data[10] = p_ptp_port[9] << 4;
    p_path->a_opcode[10] = AG_CLSB_OPC8_TAG;

    p_path->n_size = 11;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* queries whether the classification path exist in the label search tree */
/* */
AgResult
ag_nd_rpc_query_label(
    AgNdDevice          *p_device, 
    AgNdEncapsulation   e_encapsulation,    /* encapsulation type */
    AgNdClsData         *p_cls_data,        /* multiplexing label */
    AgNdChannel         *n_channel_id)      /* the channel id of the classfication path (if found) */
{
    AgNdLsTreePath  x_path;
    AgResult        n_ret = AG_S_OK;
    AG_U32          n_found_channel_id;
    AG_U32          n_pos;


    assert(e_encapsulation >= AG_ND_ENCAPSULATION_FIRST);
    assert(e_encapsulation <= AG_ND_ENCAPSULATION_LAST);


    a_path_builder[e_encapsulation](&x_path, p_cls_data);

    ag_nd_mem_unit_set(p_device, p_device->p_mem_lstree);

    n_ret = ag_clsb_query_path_8(
        p_device->n_lstree_mpid, 
        x_path.a_data, 
        x_path.n_size, 
        CLSB_QUERY_PATH_MATCH, 
        &n_found_channel_id, 
        &n_pos);

    assert(n_found_channel_id >= AG_ND_RPC_LSTREE_TAG_BASE);

    *n_channel_id = (AG_U16)(n_found_channel_id - AG_ND_RPC_LSTREE_TAG_BASE);


    return n_ret;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* adds label classification path  */
/* */
AgResult 
ag_nd_rpc_label_to_chid_map(
    AgNdDevice          *p_device, 
    AgNdEncapsulation   e_encapsulation,    /* encapsulation type */
    AgNdClsData         *p_cls_data,        /* multiplexing label */
    AgNdChannel         n_channel_id)       /* channel associated with the label */
{
    AgNdLsTreePath  x_path;
    AG_U32          n_tag;
    AgResult        n_ret = AG_S_OK;


    assert(e_encapsulation >= AG_ND_ENCAPSULATION_FIRST);
    assert(e_encapsulation <= AG_ND_ENCAPSULATION_LAST);


    a_path_builder[e_encapsulation](&x_path, p_cls_data);

    n_tag = AG_ND_RPC_LSTREE_TAG_BASE + (AG_U32)n_channel_id;

    ag_nd_mem_unit_set(p_device, p_device->p_mem_lstree);

    n_ret = ag_clsb_add_path_8(
        p_device->n_lstree_mpid, 
        x_path.a_data, 
        x_path.a_opcode, 
        x_path.n_size, 
        &n_tag, 
        1);


    return n_ret;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* deletes classification path  */
/* */
AgResult 
ag_nd_rpc_label_to_chid_unmap(
    AgNdDevice          *p_device, 
    AgNdEncapsulation   e_encapsulation,    /* encapsulation type */
    AgNdClsData         *p_cls_data)        /* multiplexing label */
{
    AgNdLsTreePath  x_path;
    AgResult        n_ret = AG_S_OK;
    AG_U32          n_erase_pos;
    AG_U16          n_ref_count;


    assert(e_encapsulation >= AG_ND_ENCAPSULATION_FIRST);
    assert(e_encapsulation <= AG_ND_ENCAPSULATION_LAST);


    a_path_builder[e_encapsulation](&x_path, p_cls_data);

    ag_nd_mem_unit_set(p_device, p_device->p_mem_lstree);

    n_ret = ag_clsb_delete_path_8(
        p_device->n_lstree_mpid, 
        x_path.a_data, 
        x_path.a_opcode, 
        x_path.n_size, 
        &n_erase_pos, 
        &n_ref_count);


    return n_ret;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_config_rpc_ucode */
/* */
AgResult 
ag_nd_opcode_read_config_rpc_ucode(AgNdDevice *p_device, AgNdMsgConfigRpcUcode *p_msg)
{
    AgNdRegUcodeInstruction x_instr;
    AG_U32 i;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);


    /* */
    /* read the destination MAC address */
    /* */
    for (i = 0; i < 3; i++)
    {
        ag_nd_mem_read(p_device, p_device->p_mem_ucode, 8 * nNdUcodeDestMacInstr[i], x_instr.n_reg, 4);

        p_msg->a_dest_mac[i * 2] = (AG_U8)(x_instr.x_fields.n_value >> 8);
        p_msg->a_dest_mac[i * 2 + 1] = (AG_U8)(x_instr.x_fields.n_value & 0xff);
    }

    /* */
    /* read the destination IPv4 address */
    /* */
    for (i = 0; i < 2; i++)
    {
        ag_nd_mem_read(p_device, p_device->p_mem_ucode, 8 * nNdUcodeDestIpv4Instr[i], x_instr.n_reg, 4);

        p_msg->n_dest_ipv4 <<= 16;
        p_msg->n_dest_ipv4 |= (AG_U32)x_instr.x_fields.n_value;
    }

    /* */
    /* read the destination IPv6 address */
    /* */
    for (i = 0; i < 8; i++)
    {
        ag_nd_mem_read(p_device, p_device->p_mem_ucode, 8 * nNdUcodeDestIpv6Instr[i], x_instr.n_reg, 4);

        p_msg->a_dest_ipv6[i * 2] = (AG_U8)(x_instr.x_fields.n_value >> 8);
        p_msg->a_dest_ipv6[i * 2 + 1] = (AG_U8)(x_instr.x_fields.n_value & 0xff);
    }

    /* */
    /* read UDP classification mode */
    /* */
    ag_nd_mem_read(p_device, p_device->p_mem_ucode, 8 * nNdUcodeUdpCmdInstr[0], x_instr.n_reg, 4);
    p_msg->b_udp_direct = x_instr.x_fields.n_opcode == AG_ND_UCODE_OPCODE_UID;

    /* */
    /* read MPLS classification mode */
    /* */
    ag_nd_mem_read(p_device, p_device->p_mem_ucode, 8 * nNdUcodeMplsCmdInstr[0], x_instr.n_reg, 4);
    p_msg->b_udp_direct = x_instr.x_fields.n_opcode == AG_ND_UCODE_OPCODE_CID;

    /* */
    /* read ECID classification mode */
    /* */
    ag_nd_mem_read(p_device, p_device->p_mem_ucode, 8 * nNdUcodeEcidCmdInstr[0], x_instr.n_reg, 4);
    p_msg->b_udp_direct = x_instr.x_fields.n_opcode == AG_ND_UCODE_OPCODE_CID;

	/*ORI*/
	/*OPEN L2TP */
    /*#if 0 def CES16_BCM_VERSION TODl2TP*/
    /* */
    /* read L2Tp classification mode */
    /* */
    ag_nd_mem_read(p_device, p_device->p_mem_ucode, 8 * nNdUcodeL2TpCmdInstr[0], x_instr.n_reg, 4);
    p_msg->b_l2tp_direct = x_instr.x_fields.n_opcode == AG_ND_UCODE_OPCODE_CID;
	/* */
    /* read VLAN bitmask for strict check */
    /* */
    ag_nd_mem_read(p_device, p_device->p_mem_ucode, 8 * nNdUcodeVlanInstr[0], x_instr.n_reg, 4);
	p_msg->n_vlan_mask = x_instr.x_fields.n_mask;


    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_config_rpc_ucode */
/* */
AgResult 
ag_nd_opcode_write_config_rpc_ucode(AgNdDevice *p_device, AgNdMsgConfigRpcUcode *p_msg)
{
    AgNdRegUcodeInstruction x_instr;
    AgNdRegGlobalControl    x_glb_ctl;
    AG_U32  i;
    AG_CHAR a_str[80];


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);

    sprintf(a_str, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
            p_msg->a_dest_mac[0],
            p_msg->a_dest_mac[1],
            p_msg->a_dest_mac[2],
            p_msg->a_dest_mac[3],
            p_msg->a_dest_mac[4],
            p_msg->a_dest_mac[5]);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "a_dest_mac=%s\n", a_str);

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "a_dest_ipv4=0x%08lx\n", p_msg->n_dest_ipv4);

    sprintf(a_str, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
            p_msg->a_dest_ipv6[0],
            p_msg->a_dest_ipv6[1],
            p_msg->a_dest_ipv6[2],
            p_msg->a_dest_ipv6[3],
            p_msg->a_dest_ipv6[4],
            p_msg->a_dest_ipv6[5],
            p_msg->a_dest_ipv6[6],
            p_msg->a_dest_ipv6[7],
            p_msg->a_dest_ipv6[8],
            p_msg->a_dest_ipv6[9],
            p_msg->a_dest_ipv6[10],
            p_msg->a_dest_ipv6[11],
            p_msg->a_dest_ipv6[12],
            p_msg->a_dest_ipv6[13],
            p_msg->a_dest_ipv6[14],
            p_msg->a_dest_ipv6[15]);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "a_dest_ipv6=%s\n", a_str);

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_udp_direct=%lu\n", p_msg->b_udp_direct);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_mpls_direct=%lu\n", p_msg->b_mpls_direct);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_ecid_direct=%lu\n", p_msg->b_ecid_direct);


    /* */
    /* unlock ucode memory lock */
    /*  */
    ag_nd_reg_read(p_device, AG_REG_GLOBAL_CONTROL, &(x_glb_ctl.n_reg));
    x_glb_ctl.x_fields.b_ucode_memory_locked = (field_t) AG_FALSE;
    ag_nd_reg_write(p_device, AG_REG_GLOBAL_CONTROL, x_glb_ctl.n_reg);


    /* */
    /* change destination MAC address value: */
    /* bits 32:47, 16:31 and 0:15 at first, second and third iteration respectively */
    /* */
    for (i = 0; i < 3; i++)
    {
        ag_nd_mem_read(p_device, p_device->p_mem_ucode, 8 * nNdUcodeDestMacInstr[i], x_instr.n_reg, 4);

        x_instr.x_fields.n_value = 
            (AG_U16)p_msg->a_dest_mac[i * 2] << 8 | (AG_U16)p_msg->a_dest_mac[i * 2 + 1];

        ag_nd_mem_write(p_device, p_device->p_mem_ucode, 8 * nNdUcodeDestMacInstr[i], x_instr.n_reg, 4);
    }


    /* */
    /* change destination IPv4 address value */
    /* bits 16:31 and 0:15 at first and second iteration respectively */
    /*  */
    for (i = 0; i < 2; i++)
    {
        ag_nd_mem_read(p_device, p_device->p_mem_ucode, 8 * nNdUcodeDestIpv4Instr[i], x_instr.n_reg, 4);

        x_instr.x_fields.n_value = 
            (AG_U16)(p_msg->n_dest_ipv4 >> (16 * (1 - i)));

        ag_nd_mem_write(p_device, p_device->p_mem_ucode, 8 * nNdUcodeDestIpv4Instr[i], x_instr.n_reg, 4);
    }


    /* */
    /* change destination IP address value */
    /* bits 112:127, 96:11 ... 0:15 at first, second ... and 8 iteration respectively */
    for (i = 0; i < 8; i++)
    {
        ag_nd_mem_read(p_device, p_device->p_mem_ucode, 8 * nNdUcodeDestIpv6Instr[i], x_instr.n_reg, 4);

        x_instr.x_fields.n_value = 
            (AG_U16)p_msg->a_dest_ipv6[i * 2] << 8 | (AG_U16)p_msg->a_dest_ipv6[i * 2 + 1];

        ag_nd_mem_write(p_device, p_device->p_mem_ucode, 8 * nNdUcodeDestIpv6Instr[i], x_instr.n_reg, 4);
    }


    /* */
    /* change the UDP label classification mode */
    /* */
    ag_nd_mem_read(p_device, p_device->p_mem_ucode, 8 * nNdUcodeUdpCmdInstr[0], x_instr.n_reg, 4);
    x_instr.x_fields.n_opcode = p_msg->b_udp_direct ? AG_ND_UCODE_OPCODE_UID : AG_ND_UCODE_OPCODE_LBU;
    ag_nd_mem_write(p_device, p_device->p_mem_ucode, 8 * nNdUcodeUdpCmdInstr[0], x_instr.n_reg, 4);


    /* */
    /* change the MPLS label classification mode */
    /* */
    ag_nd_mem_read(p_device, p_device->p_mem_ucode, 8 * nNdUcodeMplsCmdInstr[0], x_instr.n_reg, 4);
    x_instr.x_fields.n_opcode = p_msg->b_udp_direct ? AG_ND_UCODE_OPCODE_CID : AG_ND_UCODE_OPCODE_LBM;
    ag_nd_mem_write(p_device, p_device->p_mem_ucode, 8 * nNdUcodeMplsCmdInstr[0], x_instr.n_reg, 4);


    /* */
    /* change the ECID label classification mode */
    /* */
    ag_nd_mem_read(p_device, p_device->p_mem_ucode, 8 * nNdUcodeEcidCmdInstr[0], x_instr.n_reg, 4);
    x_instr.x_fields.n_opcode = p_msg->b_udp_direct ? AG_ND_UCODE_OPCODE_CID : AG_ND_UCODE_OPCODE_LBE;
    ag_nd_mem_write(p_device, p_device->p_mem_ucode, 8 * nNdUcodeEcidCmdInstr[0], x_instr.n_reg, 4);

	/*ORI*/
	/*OPEN L2TP */
    /* */
    /* change the L2TP label classification mode */
    /* */
    ag_nd_mem_read(p_device, p_device->p_mem_ucode, 8 * nNdUcodeL2TpCmdInstr[0], x_instr.n_reg, 4);
    x_instr.x_fields.n_opcode = p_msg->b_l2tp_direct ? AG_ND_UCODE_OPCODE_CID : AG_ND_UCODE_OPCODE_LBL;
    ag_nd_mem_write(p_device, p_device->p_mem_ucode, 8 * nNdUcodeL2TpCmdInstr[0], x_instr.n_reg, 4);


	/* */
    /* change VLAN bitmask for strict check */
    /* */
    ag_nd_mem_read(p_device, p_device->p_mem_ucode, 8 * nNdUcodeVlanInstr[0], x_instr.n_reg, 4);
	x_instr.x_fields.n_mask = p_msg->n_vlan_mask;
    ag_nd_mem_write(p_device, p_device->p_mem_ucode, 8 * nNdUcodeVlanInstr[0], x_instr.n_reg, 4);


    /* */
    /* lock ucode memory lock back */
    /*  */
    x_glb_ctl.x_fields.b_ucode_memory_locked = (field_t) AG_TRUE;
    ag_nd_reg_write(p_device, AG_REG_GLOBAL_CONTROL, x_glb_ctl.n_reg);


    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_config_rpc_port_fwd */
/* */
AgResult 
ag_nd_opcode_write_config_rpc_port_fwd(AgNdDevice *p_device, AgNdMsgConfigRpcPortFwd *p_msg)
{
    AgResult    n_ret = AG_S_OK;
    AgNdChannel n_channel_id;
    AG_BOOL     b_label_exists = AG_FALSE;
    AgNdClsData x_cls_data;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "e_action=%d\n", p_msg->e_action);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_port=%hu\n", p_msg->n_port);


    /* */
    /* check if the port is present in classification tables */
    /* */
    x_cls_data.n_label = (AG_U32)p_msg->n_port;

    n_ret = ag_nd_rpc_query_label(
        p_device, 
        AG_ND_ENCAPSULATION_IP, 
        &x_cls_data, 
        &n_channel_id);

    if (AG_SUCCEEDED(n_ret))
    {
        b_label_exists = AG_TRUE;

        /* */
        /* if used by CES fail the request */
        /* */
        if (n_channel_id < p_device->n_total_channels)
            return AG_ND_ERR(p_device, AG_E_ND_CES_PORT);
    }


    switch(p_msg->e_action)
    {
    case AG_ND_RPC_FWD_HP:
    case AG_ND_RPC_FWD_LP:

        /* */
        /*  create/update the mapping */
        /* */

        if (AG_ND_RPC_FWD_HP == p_msg->e_action)
            n_channel_id = AG_ND_RPC_UDP_FWD_HPQ_CHID;
        else
            n_channel_id = AG_ND_RPC_UDP_FWD_LPQ_CHID;

        n_ret = ag_nd_rpc_label_to_chid_map(
            p_device, 
            AG_ND_ENCAPSULATION_IP, 
            &x_cls_data,
            n_channel_id);
        break;


    case AG_ND_RPC_FWD_NONE:

        /* */
        /* if not present in classification tables - nothing to do */
        /* otherwise delete the mapping */
        /* */
        if (!b_label_exists)
            break;

        n_ret = ag_nd_rpc_label_to_chid_unmap(
            p_device, 
            AG_ND_ENCAPSULATION_IP, 
            &x_cls_data);
        break;
    }


    return n_ret;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_config_rpc_map_label2chid */
/* */
AgResult 
ag_nd_opcode_write_config_rpc_map_label2chid(
    AgNdDevice                      *p_device, 
    AgNdMsgConfigRpcMapLabelToChid  *p_msg)
{
    AgResult    n_ret = AG_S_OK;
    AG_U32      i;
    AgNdClsData x_cls_data;

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_channel_id=%hu\n", p_msg->n_channel_id);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_enable=%lu\n", p_msg->b_enable);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "e_encapsulation=%hu\n", p_msg->e_encapsulation);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_label=%lu\n", p_msg->n_label);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "a_ptp_port:\n");

    for (i = 0; i < sizeof(p_msg->a_ptp_port); i++)
        AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "0x%02hhx\n", p_msg->a_ptp_port[i]);

#ifdef AG_ND_ENABLE_VALIDATION
    /* */
    /* sanity */
    /* */
    if (p_msg->n_channel_id >= p_device->n_pw_max)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if (p_msg->e_encapsulation < AG_ND_ENCAPSULATION_FIRST ||
        p_msg->e_encapsulation > AG_ND_ENCAPSULATION_LAST)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif


    x_cls_data.n_label = p_msg->n_label;
    x_cls_data.p_ptp_port = p_msg->a_ptp_port;


    if (p_msg->b_enable)
    {
        /* */
        /* check if the label is already present in classification tables */
        /* */
        AgNdChannel n_channel_id;
        
        n_ret = ag_nd_rpc_query_label(
            p_device, 
            p_msg->e_encapsulation, 
            &x_cls_data, 
            &n_channel_id);

        if (AG_SUCCEEDED(n_ret))
            return AG_ND_ERR(p_device, AG_E_ND_ASSIGNED);

        /* */
        /* Add classification path to label search tree: */
        /*   */
        n_ret = ag_nd_rpc_label_to_chid_map(
            p_device, 
            p_msg->e_encapsulation,
            &x_cls_data,
            p_msg->n_channel_id);   

        if (AG_FAILED(n_ret))
        {
            AG_ND_TRACE(0, AG_ND_TRACE_ALL, AG_ND_TRACE_ERROR, "enable rpc_lable failed %hu %d %lu %lx\n",
                    p_msg->n_channel_id,
                    p_msg->e_encapsulation,
                    p_msg->n_label,
                    n_ret);
        }
    }
    else
    {
        /* */
        /* delete classification path from labels search tree */
        /* */
        n_ret = ag_nd_rpc_label_to_chid_unmap(
            p_device, 
            p_msg->e_encapsulation,
            &x_cls_data);

        if (AG_FAILED(n_ret))
        {
            AG_ND_TRACE(0, AG_ND_TRACE_ALL, AG_ND_TRACE_ERROR, "disable rpc_lable failed %hu %d %lu %lx\n",
                    p_msg->n_channel_id,
                    p_msg->e_encapsulation,
                    p_msg->n_label,
                    n_ret);
        }
    }


    return n_ret;
}


