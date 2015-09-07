/* $Id: nd_interrupts.c,v 1.6 Broadcom SDK $
 * Copyright 2011 BATM
 */

#include "pub/nd_hw.h"
#include "nd_registers.h"
#include "nd_debug.h"
#include "nd_interrupts.h"
#include "nd_channel_ingress.h"

static void ag_nd_interrupts_pmi(AgNdDevice *p_device);
static void ag_nd_interrupts_pbi(AgNdDevice *p_device);
static void ag_nd_interrupts_psi(AgNdDevice *p_device);
static void ag_nd_interrupts_cwi(AgNdDevice *p_device);

static void ag_nd_interrupts_task(AG_U32, void*);

/*///////////////////////////////////////////////////////////////////////////// */
/* initialize all the device related structures used for interrupt processing */
/*   */
/* creates interrupt event and timer (if working in polling mode) */
/* creates and starts interrupt task */
/*  */
AgResult
ag_nd_interrupts_init(AgNdDevice *p_device)
{
    AgResult n_ret = AG_S_OK;


    assert(p_device);


    p_device->b_intr_task_run = AG_TRUE;
    p_device->b_intr_task_done = AG_FALSE;
    p_device->n_intr_task_entry_count = 0;


    /* */
    /* create interrupts task event */
    /* */
    n_ret |= ag_nd_event_create(&(p_device->x_intr_event), (AG_U8*)"ndIntEv");
    assert(AG_SUCCEEDED(n_ret));


    /* */
    /* set up interrupts processing task */
    /* */
    p_device->p_intr_task_stack = ag_nd_mm_ram_alloc(AG_ND_INTR_TASK_STASK_SIZE);
    if (!p_device->p_intr_task_stack)
        n_ret |= AG_E_ND_ALLOC;

    assert(AG_SUCCEEDED(n_ret));

#ifndef AG_MNT
    n_ret |= ag_nd_task_create(
        &(p_device->x_intr_task),
        (AG_U8*)"ndIntTa",
        ag_nd_interrupts_task,
        (AG_U32)p_device,
        p_device->p_intr_task_stack,
        AG_ND_INTR_TASK_STASK_SIZE,
        p_device->n_intr_task_priority);

    assert(AG_SUCCEEDED(n_ret));
#endif /*AG_MNT */

    return n_ret;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* stops the interrupts task */
/* destroys event and timer */
/*  */
AgResult
ag_nd_interrupts_stop(AgNdDevice *p_device)
{
    AgResult    n_ret = AG_S_OK;


    assert(p_device);


    if (!p_device->b_isr_mode)
    {
        n_ret = ag_nd_timer_cancel(&(p_device->x_intr_timer));
        if (AG_FAILED(n_ret)) {
            assert(0);
            return AG_E_FAIL;
        }
    }

    n_ret = ag_nd_event_set(&(p_device->x_intr_event), AG_ND_INTR_EVENT_QUIT);
    if (AG_FAILED(n_ret)) {
        assert(0);
        return AG_E_FAIL;
    }

    while(!p_device->b_intr_task_done)
        ag_nd_sleep_msec(50);

    n_ret = ag_nd_event_delete(&(p_device->x_intr_event));
    if (AG_FAILED(n_ret)) {
        assert(0);
        return AG_E_FAIL;
    }

    ag_nd_mm_ram_free(p_device->p_intr_task_stack);


    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* should be used by device interrupt timer or ISR in order to wake up  */
/* interrupts processing task */
/*  */
void
ag_nd_interrupts_process(AgNdHandle n_handle)
{
    AgNdDevice  *p_device = (AgNdDevice*)n_handle;
    AgResult    n_ret = AG_S_OK;

    AgNdRegGlobalInterruptStatus x_gints;
    AgNdRegGlobalInterruptEnable x_ginte;


    assert(p_device);


    /* */
    /* ensure that the interrupt belongs to the chip */
    /*  */
    ag_nd_reg_read(p_device, AG_REG_GLOBAL_INTERRUPT_STATUS, &(x_gints.n_reg));
    if (!x_gints.n_reg)
        return;

    /* */
    /* mask interrupts */
    /*  */
    x_ginte.n_reg = 0;
    ag_nd_reg_write(p_device, AG_REG_GLOBAL_INTERRUPT_ENABLE, x_ginte.n_reg);

    /* */
    /* wake up the interrupts processing task */
    /*  */
    n_ret = ag_nd_event_set(&(p_device->x_intr_event), AG_ND_INTR_EVENT_PROCESS);
    if (AG_FAILED(n_ret)) {
        assert(0);
    }
}

/*///////////////////////////////////////////////////////////////////////////// */
/* interrupts processing task */
/*  */
/* the task is event driven: */
/*  */
/*  AG_ND_INTR_EVENT_PROCESS event initiates interrupt registers polling and */
/*      callbacks dispatch */
/*  */
/*  AG_ND_INTR_EVENT_QUIT event terminates the task */
/*  */
static void 
ag_nd_interrupts_task(AG_U32 argc, void *argv)
{
    AgNdRegGlobalInterruptStatus    x_gints;
    AgNdRegGlobalInterruptEnable    x_ginte;
    AgNdRegEventStatus              x_psi_queue_status;
    AgNdRegEventStatus              x_cwi_queue_status;

    AgNdDevice  *p_device;
    AgResult    n_ret = AG_S_OK;
    AG_U32      n_flags;


    assert(argc);
    p_device = (AgNdDevice*)argc;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s started\n", __func__);


    /* */
    /* initilize timer of GINTE according to interrupts working mode */
    /* */
    x_ginte.n_reg = 0;

    if (p_device->b_isr_mode)
    {
        x_ginte.x_fields.b_performance_monitoring   = NULL != p_device->p_cb_pmi;
        x_ginte.x_fields.b_packet_sync              = NULL != p_device->p_cb_psi;
        x_ginte.x_fields.b_ces_control_word         = NULL != p_device->p_cb_cwi;
        x_ginte.x_fields.b_payload_buffer           = NULL != p_device->p_cb_pbi;
    }
    else
    {
        n_ret |= ag_nd_timer_set(
            &(p_device->x_intr_timer),
            (AG_U8*)"ndIntTi",
            (void (*)(AG_U32))ag_nd_interrupts_process,
            (AG_U32)p_device,
            p_device->n_intr_task_wakeup,
            AG_TRUE);
        assert(AG_SUCCEEDED(n_ret));
    }

    ag_nd_reg_write(p_device, AG_REG_GLOBAL_INTERRUPT_ENABLE, x_ginte.n_reg);


    /* */
    /* wait to event: interrupt processing loop */
    /* */
    do
    {
        n_ret = ag_nd_event_wait(
            &(p_device->x_intr_event),
            AG_ND_INTR_EVENT_QUIT | AG_ND_INTR_EVENT_PROCESS,
            AG_ND_EVENT_OR_CLEAR,
            &n_flags,
            0xffffffff);


        p_device->n_intr_task_entry_count++;


        if (n_flags & AG_ND_INTR_EVENT_QUIT)
            break;


        /* */
        /* check if any interrupts pending */
        /*  */
        x_gints.n_reg = 0;
        ag_nd_reg_read(p_device, AG_REG_GLOBAL_INTERRUPT_STATUS, &(x_gints.n_reg));


        /* */
        /* PMI */
        /*  */
        if (x_gints.x_fields.b_performance_monitoring && p_device->p_cb_pmi)
            ag_nd_interrupts_pmi(p_device);


        /* */
        /* PBF */
        /*  */
        if (x_gints.x_fields.b_payload_buffer && p_device->p_cb_pbi)
            ag_nd_interrupts_pbi(p_device);


        /* */
        /* process PSI and CWI queues: the idea is to loop over the queues until  */
        /* the both are empty */
        /*  */
        x_cwi_queue_status.n_reg = 0;
        x_psi_queue_status.n_reg = 0;

        do
        {
            /* */
            /* if CWI is enabled check the queue */
            /* */
            if (p_device->p_cb_cwi)
            {
                ag_nd_reg_read(
                    p_device, 
                    AG_REG_CES_CONTROL_WORD_INTERRUPT_QUEUE_STATUS,
                    &(x_cwi_queue_status.n_reg));
                
                if (x_cwi_queue_status.x_fields.n_queue_status)
                    ag_nd_interrupts_cwi(p_device);
            }

            /* */
            /* if PSI is enabled check the queue */
            /*  */
            if (p_device->p_cb_psi)
            {
                ag_nd_reg_read(p_device, 
                    AG_REG_PACKET_SYNCHRONIZATION_INTERRUPT_QUEUE_STATUS, 
                    &(x_psi_queue_status.n_reg));
                
                if (x_psi_queue_status.x_fields.n_queue_status)
                    ag_nd_interrupts_psi(p_device);
            }
        }
        while (x_cwi_queue_status.x_fields.n_queue_status > 0 ||
               x_psi_queue_status.x_fields.n_queue_status > 0);


        /* */
        /* clear global interrupt bits by writing GINTS back */
        /*  */
        ag_nd_reg_write(p_device, AG_REG_GLOBAL_INTERRUPT_STATUS, x_gints.n_reg);

        /* */
        /* unmask interrupts  */
        /* */
        ag_nd_reg_write(p_device, AG_REG_GLOBAL_INTERRUPT_ENABLE, x_ginte.n_reg);

    }
    while(1);


    p_device->b_intr_task_done = AG_TRUE;
}

/*///////////////////////////////////////////////////////////////////////////// */
/*  */
/* Processes the performance monitoring interrupt */
/*  */
static void 
ag_nd_interrupts_pmi(AgNdDevice *p_device)
{
    p_device->p_cb_pmi();
}

/*///////////////////////////////////////////////////////////////////////////// */
/* */
/* Processes the packet sync status interrupt */
/*  */
static void 
ag_nd_interrupts_psi(AgNdDevice *p_device)
{
    AgNdRegEventQueueEntry          x_queue_entry;
    AgNdRegPacketSyncStatus         x_sync_stat;            

    /* */
    /* read pending events from queue */
    /* */
    do
    {
        /* */
        /* read one entry from PSI queue */
        /* */
        ag_nd_reg_read(
            p_device, 
            AG_REG_PACKET_SYNCHRONIZATION_INTERRUPT_EVENT_QUEUE, 
            &(x_queue_entry.n_reg));

        if (0 == x_queue_entry.x_fields.b_valid_entry)
        {
            /* */
            /* if we get here there is a VLSI bug  */
            /*  */
            assert(x_queue_entry.x_fields.b_valid_entry);
            break;
        }

        /* */
        /* clear per channel PSIS bit */
        /* */
        x_sync_stat.n_reg = 0;
        x_sync_stat.x_fields.n_packet_sync_interrupt_status = 1;

        ag_nd_reg_write(
            p_device,
            AG_REG_PACKET_SYNC_STATUS(x_queue_entry.x_fields.n_pending_event_channel), 
            x_sync_stat.n_reg);

        /* */
        /* read the corresponding channel status */
        /* */
        ag_nd_reg_read(
            p_device, 
            AG_REG_PACKET_SYNC_STATUS(x_queue_entry.x_fields.n_pending_event_channel), 
            &(x_sync_stat.n_reg));

        /* */
        /* update the application with channel id and status */
        /* */
        p_device->p_cb_psi(
            x_queue_entry.x_fields.n_pending_event_channel,
            p_device->n_user_data_psi,
            x_sync_stat.x_fields.n_packet_sync_state);
    }
    while(!x_queue_entry.x_fields.b_last_valid_entry);
}

/*///////////////////////////////////////////////////////////////////////////// */
/* */
/* Processes the control word interrupt */
/*  */
static void 
ag_nd_interrupts_cwi(AgNdDevice *p_device)
{
    AgNdRegEventQueueEntry  x_queue_entry;

    AG_U16  n_cw;       /* current CW of channel  */
    AG_U16  n_status;   /* current CWI status of channel */


    /* */
    /* read the pending events from queue */
    /* */
    do
    {
        /* */
        /* read one entry from CWI queue */
        /* */
        ag_nd_reg_read(
            p_device, 
            AG_REG_CES_CONTROL_WORD_INTERRUPT_EVENT_QUEUE,
            &(x_queue_entry.n_reg));

        if (0 == x_queue_entry.x_fields.b_valid_entry)
        {
            /* */
            /* if we get here there is a VLSI bug  */
            /*  */
            assert(x_queue_entry.x_fields.b_valid_entry);
            break;
        }

        /* */
        /* read the interrupt status of the corresponding channel */
        /* */
        ag_nd_reg_read(
            p_device, 
            AG_REG_CES_CONTROL_WORD_INTERRUPT_STATUS(x_queue_entry.x_fields.n_pending_event_channel),
            &(n_status));

        /* */
        /* clear the per channel CWI status bits */
        /*  */
        ag_nd_reg_write(
            p_device,
            AG_REG_CES_CONTROL_WORD_INTERRUPT_STATUS(x_queue_entry.x_fields.n_pending_event_channel),
            0xffff);

        /* */
        /* read the current channel control word of the corresponding channel */
        /* */
        ag_nd_reg_read(
            p_device, 
            AG_REG_RECEIVE_CES_CONTROL_WORD(x_queue_entry.x_fields.n_pending_event_channel), 
            &(n_cw));

        /* */
        /* update the application with channel id and status */
        /* */
        p_device->p_cb_cwi(
            x_queue_entry.x_fields.n_pending_event_channel,
            p_device->n_user_data_cwi,
            n_cw);

    }
    while(!x_queue_entry.x_fields.b_last_valid_entry);
}

/*///////////////////////////////////////////////////////////////////////////// */
/* */
/* Processes the payload buffer overflow interrupt */
/*  */
static void 
ag_nd_interrupts_pbi(AgNdDevice *p_device)
{
  #ifndef CES16_BCM_VERSION
    AgNdRegPayloadBufferStatus  x_status;
    AgNdChannel                 n_channel_id;


    /* */
    /* poll all the enabled ingress channels for PBF overflow */
    /* */
    for (n_channel_id = 0; n_channel_id < p_device->n_pw_max; n_channel_id++)
    {
        if (ag_nd_channel_ingress_is_enabled(p_device, n_channel_id))
        {
            ag_nd_reg_read(p_device, AG_REG_PAYLOAD_BUFFER_STATUS(n_channel_id), &(x_status.n_reg));

            if (x_status.x_fields.b_buffer_overflow)
                p_device->p_cb_pbi(n_channel_id, p_device->n_user_data_pbi);

            /* */
            /* clear PBF inetrrupt status bit */
            /* */
            ag_nd_reg_write(p_device, AG_REG_PAYLOAD_BUFFER_STATUS(n_channel_id), x_status.n_reg);
        }
    }
  #endif
}

