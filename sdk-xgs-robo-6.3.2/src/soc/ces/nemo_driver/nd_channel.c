/* $Id: nd_channel.c 1.7 Broadcom SDK $
 * Copyright 2011 BATM
 */

#include "nd_channel.h"
#include "nd_debug.h"
#include "nd_channel_ingress.h"
#include "nd_channel_egress.h"


/*///////////////////////////////////////////////////////////////////////////// */
/* reads channel enable status */
/*  */
AgResult 
ag_nd_opcode_read_config_channel_enable(AgNdDevice *p_device, AgNdMsgConfigChannelEnable *p_msg)
{
    AG_BOOL (*p_read[AG_ND_PATH_MAX])(AgNdDevice *p_device, AG_U16 n_channel_id /*BCM out , AG_U32 n_cookie*/);


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);


    p_read[AG_ND_PATH_EGRESS] = ag_nd_channel_egress_is_enabled;
    p_read[AG_ND_PATH_INGRESS] = ag_nd_channel_ingress_is_enabled;


#ifdef AG_ND_ENABLE_VALIDATION
    if (!p_msg)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
    
    if (p_msg->n_channel_id >= p_device->n_pw_max)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if (p_msg->e_path >= AG_ND_PATH_MAX)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif

 
    p_msg->b_enable = p_read[p_msg->e_path](p_device, p_msg->n_channel_id/* BCMout, p_msg->n_jb_size_milli*/);
            

    return AG_S_OK;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* enables/disables channel */
/*  */
AgResult 
ag_nd_opcode_write_config_channel_enable(AgNdDevice *p_device, AgNdMsgConfigChannelEnable *p_msg)
{
    AgResult (*p_enable[2][AG_ND_PATH_MAX])(AgNdDevice *p_device, AG_U16 n_channel_id, AG_U32 n_cookie);


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_enable=%lu\n", p_msg->b_enable);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "e_path=%d\n", p_msg->e_path);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_channel_id=%hu\n", p_msg->n_channel_id);


    p_enable[AG_TRUE][AG_ND_PATH_EGRESS] = ag_nd_channel_egress_enable;
    p_enable[AG_FALSE][AG_ND_PATH_EGRESS] = ag_nd_channel_egress_disable;
    p_enable[AG_TRUE][AG_ND_PATH_INGRESS] = ag_nd_channel_ingress_enable;
    p_enable[AG_FALSE][AG_ND_PATH_INGRESS] = ag_nd_channel_ingress_disable;

    
#ifdef AG_ND_ENABLE_VALIDATION
    if (!p_msg)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if (p_msg->n_channel_id >= p_device->n_pw_max)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if (p_msg->e_path >= AG_ND_PATH_MAX)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif


    return p_enable[p_msg->b_enable][p_msg->e_path](p_device, p_msg->n_channel_id, p_msg->n_jb_size_milli);
}

