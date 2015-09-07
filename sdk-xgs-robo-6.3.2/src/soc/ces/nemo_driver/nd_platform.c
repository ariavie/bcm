/* $Id: nd_platform.c 1.7 Broadcom SDK $
 * Copyright 2011 BATM
 */


#ifndef BCM_CES_SDK
#include <stdarg.h>
#include <stdio.h>
#endif
#include "nd_platform.h"

#ifndef BCM_CES_SDK
#include "utils/uart_generic.h"
#include "drivers/cfgd.h"
#include "infra/sw_logger/pub/sw_logger.h"
#endif /*BCM_CES_SDK*/

#include "pub/nd_api.h"
#include "utils/memutils.h"
#define GEN_ERR_LOG0(str) 

extern void*    ag_memset(void* p_dst, int n_char, size_t n_bytes);

/*///////////////////////////////////////////////////////////////////////////// */
/* sleep in ms */
/* */
void 
ag_nd_sleep_msec(AG_U32 ms)
{
    agos_wake_after(ms);
}

/*///////////////////////////////////////////////////////////////////////////// */
/* */
/* */
AG_U32
ag_nd_time_usec(void)
{
    return ag_get_micro_timestamp();
}

/*///////////////////////////////////////////////////////////////////////////// */
/* */
/* */
void *ag_nd_memset(void *s, int c, size_t n) { return ag_memset(s, c, n); }

void *ag_nd_memcpy(void * s1, const void * s2, size_t n) { return AG_MEMCPY(s1, s2, n); }


/*///////////////////////////////////////////////////////////////////////////// */
/* printf -> AGOS_TRACE */
/* */
#ifndef WIN32

int ag_nd_printf(const char *format, ...)
{
  #ifndef BCM_CES_SDK
    int stat;
    va_list argp;
    char buf[AG_ND_PRINTF_MAX];
    char bufcr[AG_ND_PRINTF_MAX];
    char *p;
    int i;

    
    va_start(argp, format);
    stat = vsnprintf(buf, sizeof(buf), format, argp);
    buf[AG_ND_PRINTF_MAX - 1] = 0;


    for (p = buf, i = 0; *p && (i < sizeof(bufcr)); p++, i++)
    {
        bufcr[i] = *p;
        if ('\n' == *p)
            bufcr[++i] = '\r';
    }


    ag_utl_simple_msgurt_send(bufcr, i, AG_UART_A);
/*    ag_utl_simple_msgurt_send(bufcr, i, AG_UART_B); */


    return stat;
 #else /*BCM_CES_SDK*/
  return 1; /*Count of output chars*/
#endif

}

#endif



/* */
/* timeout value in milliseconds */
/* timeout value of 0 means no wait */
/* timeout value of (AG_U32)-1 means wait forever */
/*  */


/* */
/* mutexes */
/* */
AgResult    
ag_nd_mutex_create(AgNdMutex *p_mutex, AG_CHAR *p_name)
{
    return agos_create_bi_semaphore(&(p_mutex->x_bi_semaphore), p_name);
}

AgResult    
ag_nd_mutex_lock(AgNdMutex *p_mutex, AG_U32 n_timeout)
{
    return agos_get_bi_semaphore(&(p_mutex->x_bi_semaphore), n_timeout);
}

AgResult    
ag_nd_mutex_unlock(AgNdMutex *p_mutex)
{
    return agos_give_bi_semaphore(&(p_mutex->x_bi_semaphore));
}

AgResult    
ag_nd_mutex_destroy(AgNdMutex *p_mutex)
{
    return agos_delete_bi_semaphore(&(p_mutex->x_bi_semaphore));
}


#ifndef BCM_CES_SDK /*queues, not needed for BCM used only with HDLC*/
/* */
/* queues */
/*  */
AgResult    
ag_nd_queue_create(AgNdQueue *p_queue, AG_U8 *p_name, AG_U32 n_queue_size, AG_U32 n_message_size)
{
    return agos_create_queue(&(p_queue->x_queue), p_name, n_queue_size, n_message_size);
}

AgResult    
ag_nd_queue_delete(AgNdQueue *p_queue)
{
    return agos_delete_queue(&(p_queue->x_queue));
}

AgResult    
ag_nd_queue_receive(AgNdQueue *p_queue, void *p_message, AG_U32 n_timeout)
{
    return agos_receive_from_queue(&(p_queue->x_queue), p_message, n_timeout);
}

AgResult    
ag_nd_queue_send(AgNdQueue *p_queue, void *p_message, AG_U32 n_timeout)
{
    return agos_send_to_queue(&(p_queue->x_queue), p_message, n_timeout);
}

AgResult    
ag_nd_queue_send_to_front(AgNdQueue *p_queue, void *p_message, AG_U32 n_timeout)
{
    return agos_send_to_front_of_queue(&(p_queue->x_queue), p_message, n_timeout);
}
#endif


/* */
/* timers */
/*  */
AgResult    
ag_nd_timer_set(
    AgNdTimer   *p_timer, 
    AG_U8       *p_name, 
    void        (*p_callback_function)(AG_U32), 
    AG_U32      n_callpack_parameter,
    AG_U32      n_timer_value,
    AG_BOOL     b_periodic)
{
    return agos_set_timer(
        &(p_timer->x_timer), 
        p_name, 
        p_callback_function, 
        n_callpack_parameter, 
        n_timer_value, 
        b_periodic);
}

AgResult    
ag_nd_timer_cancel(AgNdTimer *p_timer)
{
    return agos_cancel_timer(&(p_timer->x_timer));
}


/* */
/* events */
/*  */
AgResult    
ag_nd_event_create(AgNdEvent *p_event, AG_U8 *p_name)
{
    return agos_create_event_group(&(p_event->x_event_group), p_name);
}

AgResult    
ag_nd_event_set(AgNdEvent *p_event, AG_U32 n_flags)
{
    return agos_set_event(&(p_event->x_event_group), n_flags);
}

AgResult    
ag_nd_event_wait(
    AgNdEvent   *p_event, 
    AG_U32      n_requested_flags,
    AgNdEventOp e_operation,
    AG_U32      *n_retrieved_flags, 
    AG_U32      n_timeout)
{
    return agos_wait_on_event(
        &(p_event->x_event_group), 
        n_requested_flags, 
        (AgosEventWaitOperation)e_operation, 
        n_retrieved_flags, 
        n_timeout);
}

AgResult    
ag_nd_event_delete(AgNdEvent *p_event)
{
    return agos_delete_event_group(&(p_event->x_event_group));
}


/* */
/* tasks */
/*  */

AgResult
ag_nd_task_create(
    AgNdTask    *p_task,
    AG_U8       *p_name,
    void        (*p_task_entry_function)(AG_U32, void*),
    AG_U32      n_param,
    void        *p_stack,
    AG_U32      n_stack_size,
    AG_U32      n_priority)
{
    ag_nd_memset(p_task, 0, sizeof(*p_task));

    return agos_create_task(
        &(p_task->x_task_info),
        p_name,
        p_task_entry_function,
        n_param,
        p_stack,
        n_stack_size,
        n_priority,
        AG_HL_TASK_TIME_SLICE,
        AGOS_TSK_PREEMPT,
        AGOS_TSK_START);
}

/* */
/* SW logger stuff */
/*  */
static AG_U32 n_log_app_id = 0;

void ag_nd_sw_log_app_init()
{
    if (!n_log_app_id)
      #ifndef BCM_CES_SDK
        n_log_app_id = ag_swlog_get_app_id((AG_CHAR*)"NEMO_DRIVER");
      #else
        n_log_app_id = 1; /*For BCM does not have any meanning*/
      #endif
}


void 
ag_nd_sw_log(AG_U16 n_level, char* p_msg, const char *p_file, int n_line)
{
    SwLogLevelType n_sw_level = SW_LOG_LEVEL_NONE;

    switch (n_level)
    {
    case AG_ND_TRACE_NONE:
        return;

    case AG_ND_TRACE_ERROR:
    case AG_ND_TRACE_WARNING:
        n_sw_level = SW_LOG_LEVEL_ERROR;
        break;

    case AG_ND_TRACE_DEBUG:
        n_sw_level = SW_LOG_LEVEL_DEBUG;
        break;

    default:
        return;    
    }

    if (n_sw_level == SW_LOG_LEVEL_NONE) {
        return;
    }
    
    AG_LOG_MESSAGE(n_log_app_id, n_sw_level, "%s/%d %s", p_file, n_line, p_msg, 0, 0, 0);
}

