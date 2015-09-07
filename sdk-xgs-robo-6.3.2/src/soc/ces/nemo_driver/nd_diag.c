/* $Id: nd_diag.c 1.8 Broadcom SDK $
 * Copyright 2011 BATM
 */

#include "pub/nd_api.h"
#include "pub/nd_hw.h"
#include "nd_registers.h"
#include "nd_diag.h"
#include "nd_debug.h"

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_debug */
/* */
AgResult
ag_nd_opcode_read_diag(AgNdDevice *p_device, AgNdMsgDiag *p_msg)
{
    AgNdRegGlobalControl                     x_glb;
    AgNdRegChannelizerLoopbackChannelSelect  x_channel;
    AgNdRegGlobalClkActivityMonitoring       x_clk;
    AgNdRegT1E1PortConfiguration             x_e1t1;
	AG_U16									 n_reg_tcxo_ocxo;
    AG_U32 i;
	

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);


    ag_nd_memset(p_msg, 0, sizeof(*p_msg));

    ag_nd_reg_read(p_device, AG_REG_GLOBAL_CONTROL, &(x_glb.n_reg));
        
    if (x_glb.x_fields.b_channelizer_loopback)
    {
        p_msg->e_loopback = AG_ND_LOOPBACK_CHANNELIZER;
        ag_nd_reg_read(p_device, AG_REG_CHANNELIZER_LOOPBACK_CHANNEL_SELECT, &(x_channel.n_reg));
        p_msg->n_loopback_channel_select = x_channel.x_fields.n_channel_select;
    }
    else if (x_glb.x_fields.b_dechannelizer_loopback)
        p_msg->e_loopback = AG_ND_LOOPBACK_DECHANNELIZER;
    else if (x_glb.x_fields.b_packet_classifier_loopback)
        p_msg->e_loopback = AG_ND_LOOPBACK_CLASSIFIER;
    else if (x_glb.x_fields.b_packet_interface_loopback)
        p_msg->e_loopback = AG_ND_LOOPBACK_PACKET_INTERFACE;
    else if (x_glb.x_fields.b_packet_local_loopback)
        p_msg->e_loopback = AG_ND_LOOPBACK_PACKET_LOCAL;
    else if (x_glb.x_fields.b_packet_remote_loopback)
        p_msg->e_loopback = AG_ND_LOOPBACK_PACKET_REMOTE;
    else if (x_glb.x_fields.b_tdm_local_loopback)
        p_msg->e_loopback = AG_ND_LOOPBACK_TDM_LOCAL;
    else if (x_glb.x_fields.b_tdm_remote_loopback)
        p_msg->e_loopback = AG_ND_LOOPBACK_TDM_REMOTE;


    /* */
    /* build list of TDM remote loopback ports in the case of Nemo */
    /*  */
    if (1 /*AG_ND_ARCH_MASK_NEMO & p_device->n_chip_mask*/)  /*BCM as nemo*/
    {
        p_msg->n_loopback_port_select = 0;

        for (i = 0; i < p_device->n_total_ports; i++)
        {
            ag_nd_reg_read(p_device, AG_REG_T1_E1_PORT_CONFIGURATION(i), &(x_e1t1.n_reg));

            if (x_e1t1.x_fields.b_loopback_enable)
            {
                p_msg->n_loopback_port_select |= 1<<i;
            }
        }

    }


    ag_nd_reg_read(p_device, AG_REG_TDM_PORT_ACTIVITY_MONITORING, &(p_msg->n_act_port));
    ag_nd_reg_read(p_device, AG_REG_GLOBAL_CLK_ACTIVITY_MONITORING, &(x_clk.n_reg));

    
    p_msg->b_act_ext_clk_sync = (AG_BOOL)x_clk.x_fields.b_external_clk_synchronizer;
    p_msg->b_act_miit_clk = (AG_BOOL)x_clk.x_fields.b_miit_clk;
    p_msg->b_act_miir_clk = (AG_BOOL)x_clk.x_fields.b_miir_clk;
    p_msg->b_act_int_cclk = (AG_BOOL)x_clk.x_fields.b_internal_cclk;
    p_msg->b_act_nomad_brg_clk = (AG_BOOL)x_clk.x_fields.b_nomad_brg_clk;
    p_msg->b_act_1_sec_pulse = (AG_BOOL)x_clk.x_fields.b_one_second_pulse;
    p_msg->b_act_ref_clk1 = (AG_BOOL)x_clk.x_fields.b_reference_clk_input1;
    p_msg->b_act_ref_clk2 = (AG_BOOL)x_clk.x_fields.b_reference_clk_input2;

    ag_nd_reg_read(p_device, 0x2606, &(n_reg_tcxo_ocxo));
    p_msg->e_oscillator = (AgNdOscillatorType)(n_reg_tcxo_ocxo & 0x1);

    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* nd_n_opcode_write_diag */
/* */
AgResult
ag_nd_opcode_write_diag(AgNdDevice *p_device, AgNdMsgDiag *p_msg)
{
    AgNdRegGlobalControl                     x_glb;
    AgNdRegChannelizerLoopbackChannelSelect  x_select;
    AgNdRegT1E1PortConfiguration             x_e1t1;
    AG_U32 i;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "e_loopback=%d\n", p_msg->e_loopback);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_loopback_channel_select=%lu\n", p_msg->n_loopback_channel_select);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_loopback_port_select=%lu\n", p_msg->n_loopback_port_select);



    ag_nd_reg_read(p_device, AG_REG_GLOBAL_CONTROL, &(x_glb.n_reg));


    /* */
    /* First, disable all previously enabled loopbacks and accompanying stuff */
    /*  */
    x_select.n_reg = 0;

    /* */
    /* Nemo: if TDM remote loopback was enabled before, clear loopback enable bit of all ports */
    /* except the port that going to have TDM loopback enabled. */
    /* */
    if (x_glb.x_fields.b_tdm_remote_loopback &&
        (1 /*AG_ND_ARCH_MASK_NEMO & p_device->n_chip_mask*/))   /*BCM need*/
        
        for (i = 0; i < p_device->n_total_ports; i++)
        {
            ag_nd_reg_read(p_device, AG_REG_T1_E1_PORT_CONFIGURATION(i), &(x_e1t1.n_reg));
            x_e1t1.x_fields.b_loopback_enable = AG_FALSE;
            ag_nd_reg_write(p_device, AG_REG_T1_E1_PORT_CONFIGURATION(i), x_e1t1.n_reg);
        }

    x_glb.x_fields.b_channelizer_loopback = AG_FALSE;
    x_glb.x_fields.b_dechannelizer_loopback = AG_FALSE;
    x_glb.x_fields.b_packet_classifier_loopback = AG_FALSE;
    x_glb.x_fields.b_packet_interface_loopback = AG_FALSE;
    x_glb.x_fields.b_packet_local_loopback = AG_FALSE;
    x_glb.x_fields.b_packet_remote_loopback = AG_FALSE;
    x_glb.x_fields.b_tdm_remote_loopback = AG_FALSE;
    x_glb.x_fields.b_tdm_local_loopback = AG_FALSE;

    /* */
    /* Now configure loopback according to the message */
    /*  */
    switch(p_msg->e_loopback)
    {


    case AG_ND_LOOPBACK_CHANNELIZER:

        if (p_msg->n_loopback_channel_select >= p_device->n_pw_max)
            return AG_ND_ERR(p_device, AG_E_ND_ARG);
        
        x_glb.x_fields.b_channelizer_loopback = AG_TRUE;
        x_select.x_fields.n_channel_select = p_msg->n_loopback_channel_select;

        break;



    case AG_ND_LOOPBACK_TDM_REMOTE:       

        x_glb.x_fields.b_tdm_remote_loopback = AG_TRUE;

        if (1 /*AG_ND_ARCH_MASK_NEMO & p_device->n_chip_mask*/)    /*BCM need*/
        {
            AG_U32 n_non_existing_ports_mask;

            n_non_existing_ports_mask = 1<<p_device->n_total_ports;
            n_non_existing_ports_mask = (AG_U32)(-(AG_S32)n_non_existing_ports_mask);

            if (p_msg->n_loopback_port_select & n_non_existing_ports_mask)
                return AG_ND_ERR(p_device, AG_E_ND_ARG);
    
            for (i = 0; i < p_device->n_total_ports; i++)
                if (p_msg->n_loopback_port_select & (1<<i))
                {
                    ag_nd_reg_read(p_device, AG_REG_T1_E1_PORT_CONFIGURATION(i), &(x_e1t1.n_reg));
                    x_e1t1.x_fields.b_loopback_enable = AG_TRUE;
                    ag_nd_reg_write(p_device, AG_REG_T1_E1_PORT_CONFIGURATION(i), x_e1t1.n_reg);
                }
        }

        break;



    case AG_ND_LOOPBACK_TDM_LOCAL:

        x_glb.x_fields.b_tdm_local_loopback = AG_TRUE;
        break;




    case AG_ND_LOOPBACK_DECHANNELIZER:

        x_glb.x_fields.b_dechannelizer_loopback = AG_TRUE;
        break;




    case AG_ND_LOOPBACK_CLASSIFIER:

        x_glb.x_fields.b_packet_classifier_loopback = AG_TRUE;
        break;




    case AG_ND_LOOPBACK_PACKET_INTERFACE:

        x_glb.x_fields.b_packet_interface_loopback = AG_TRUE;
        break;



    case AG_ND_LOOPBACK_PACKET_LOCAL:

        x_glb.x_fields.b_packet_local_loopback = AG_TRUE;
        break;



    case AG_ND_LOOPBACK_PACKET_REMOTE:

        x_glb.x_fields.b_packet_remote_loopback = AG_TRUE;
        break;



    case AG_ND_LOOPBACK_NONE:
        break;



    default:
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
    }


    ag_nd_reg_write(p_device, AG_REG_CHANNELIZER_LOOPBACK_CHANNEL_SELECT, x_select.n_reg);
    ag_nd_reg_write(p_device, AG_REG_GLOBAL_CONTROL, x_glb.n_reg);


    return AG_S_OK;
} 
