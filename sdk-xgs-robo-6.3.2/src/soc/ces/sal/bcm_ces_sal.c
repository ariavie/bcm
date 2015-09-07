/*
 * $Id: bcm_ces_sal.c 1.18 Broadcom SDK $
 * $Copyright: Copyright 2012 Broadcom Corporation.
 * This program is the proprietary software of Broadcom Corporation
 * and/or its licensors, and may only be used, duplicated, modified
 * or distributed pursuant to the terms and conditions of a separate,
 * written license agreement executed between you and Broadcom
 * (an "Authorized License").  Except as set forth in an Authorized
 * License, Broadcom grants no license (express or implied), right
 * to use, or waiver of any kind with respect to the Software, and
 * Broadcom expressly reserves all rights in and to the Software
 * and all intellectual property rights therein.  IF YOU HAVE
 * NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE
 * IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE
 * ALL USE OF THE SOFTWARE.  
 *  
 * Except as expressly set forth in the Authorized License,
 *  
 * 1.     This program, including its structure, sequence and organization,
 * constitutes the valuable trade secrets of Broadcom, and you shall use
 * all reasonable efforts to protect the confidentiality thereof,
 * and to use this information only in connection with your use of
 * Broadcom integrated circuit products.
 *  
 * 2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS
 * PROVIDED "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
 * REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY,
 * OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
 * DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
 * NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
 * ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 * CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
 * OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 * 
 * 3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL
 * BROADCOM OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL,
 * INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER
 * ARISING OUT OF OR IN ANY WAY RELATING TO YOUR USE OF OR INABILITY
 * TO USE THE SOFTWARE EVEN IF BROADCOM HAS BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN EXCESS OF
 * THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF OR USD 1.00,
 * WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING
 * ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.$
 *
 * bcm_ces_sal.c
 *
 */

#include <soc/debug.h>
#include <soc/cm.h>
#include <soc/defs.h>
#include <soc/drv.h>
#include <sal/core/alloc.h>
#include <sal/core/sync.h>
#include <sal/core/libc.h>

#include "utils/List.h"

#include <bcm_ces_sal.h>

#ifdef BCM_CES_SDK 
/*BCM SDK*/
/*Bcm sal*/
/*#define CES_SAL_DEBUG(x) SOC_DEBUG_PRINT(x)*/
#define CES_SAL_DEBUG(x)

 /*Sync*/
AgResult agos_create_bi_semaphore(AgosBiSemaphoreInfo *sem, AG_CHAR *p_name)
{
  sal_sem_t sal_sem;

  CES_SAL_DEBUG((DK_VERBOSE,"%s: %s\n", __func__, p_name));

  if ((sal_sem = sal_sem_create(p_name, sal_sem_BINARY, 1)) == NULL)
    return AG_E_FAIL;

  sem->sal_sem = sal_sem;
  CES_SAL_DEBUG((DK_VERBOSE,"%s: %s %p\n", __func__, p_name, (void*)sal_sem));

  return AG_S_OK;
}

AgResult agos_get_bi_semaphore(AgosBiSemaphoreInfo *sem, AG_U32 n_timeout)
{
  int timeout;

  if (n_timeout == AGOS_WAIT_FOR_EVER)
    timeout = sal_sem_FOREVER;
  else
    timeout = n_timeout * MILLISECOND_USEC;

  CES_SAL_DEBUG((DK_VERBOSE,"%u-%s: %p timeout:%d\n", (uint32)((sal_time_usecs())/1000), __func__, (void*)sem->sal_sem, timeout));

  if (sal_sem_take(sem->sal_sem, timeout) != 0)
    return AG_E_FAIL;

  return AG_S_OK;
}

AgResult agos_give_bi_semaphore(AgosBiSemaphoreInfo *sem)
{
  CES_SAL_DEBUG((DK_VERBOSE,"%u-%s: %p\n", (uint32)((sal_time_usecs())/1000), __func__, (void*)sem->sal_sem));
   if (sal_sem_give(sem->sal_sem) != 0)
    return AG_E_FAIL;

  return AG_S_OK;
}

AgResult agos_delete_bi_semaphore(AgosBiSemaphoreInfo *sem)
{
  CES_SAL_DEBUG((DK_VERBOSE,"%s: %p\n", __func__, (void*)sem->sal_sem));
  sal_sem_destroy(sem->sal_sem);
  return AG_S_OK;
}
 

 /*Time*/
void agos_periodic_timer_callback_shell(AgosTimerInfo *timer, AG_U32 *timer_val, void (*p_callback_function)(AG_U32), void *arg, AG_U8 *name) {
    /*
     * Reschedule timer
     */
    agos_set_timer(timer,
		   name,
		   p_callback_function,
		   (AG_U32)arg,
		   (AG_U32)timer_val,
		   TRUE);
    /*
     * Call func
     */
    p_callback_function((AG_U32)arg);
}

AgResult agos_set_timer(AgosTimerInfo *x_timer,
			AG_U8       *p_name, 
			void        (*p_callback_function)(AG_U32), 
			AG_U32      n_callpack_parameter,
			AG_U32      n_timer_value,
			AG_BOOL     b_periodic)
{
    if (b_periodic)
    {
/*	CES_SAL_DEBUG((DK_VERBOSE,"%s: Periodic:%lumS\n", __func__, n_timer_value));*/
	if (sal_dpc_time(n_timer_value * MILLISECOND_USEC, 
			 (sal_dpc_fn_t)agos_periodic_timer_callback_shell, 
			 (void*)x_timer, 
			 (void*)n_timer_value, 
			 p_callback_function, 
			 (void*)n_callpack_parameter,
			 (void*)p_name) != 0) {
	    return AG_E_FAIL;
	}
    }
    else
	CES_SAL_DEBUG((DK_VERBOSE,"%s - NOT YET IMPLEMENTED\n", __func__));

    return AG_S_OK;
}

AgResult agos_cancel_timer(AgosTimerInfo *x_timer)
{
    sal_dpc_cancel(x_timer);
    return AG_S_OK;
}

void agos_wake_after(AG_U32 after)
{
    sal_usleep(after * MILLISECOND_USEC);
    return;
}

AG_U32 ag_get_micro_timestamp(void)
{
    return (AG_U32)sal_time_usecs();
}

int ag_wait_micro_sec_delay(AG_U32 delay)
{
    sal_usleep(delay);
    return 0;
}


 
 /*Events*/
AgResult agos_create_event_group(AgosEventInfo *x_event_group, AG_U8 *p_name)
{
    CES_SAL_DEBUG((DK_VERBOSE,"%s: %s\n", __func__, p_name));
    sal_strcpy(x_event_group->name, (AG_S8*)p_name);
    x_event_group->events = 0;
    return AG_S_OK;
}

AgResult agos_set_event(AgosEventInfo *x_event_group, AG_U32 n_flags)
{
    CES_SAL_DEBUG((DK_VERBOSE,"%s: flags:0x%08lx\n", __func__, n_flags));
  x_event_group->events |= n_flags;
  return AG_S_OK;
}

AgResult agos_wait_on_event(AgosEventInfo *x_event_group,
			    AG_U32      n_requested_flags,
			    AgosEventWaitOperation e_operation,
			    AG_U32      *n_retrieved_flags, 
			    AG_U32      n_timeout)
{
    AG_U32 timeleft = n_timeout;
    CES_SAL_DEBUG((DK_VERBOSE,"%s: req_flags:0x%08lx\n", __func__, n_requested_flags));
#define AGOS_WAIT_ON_EVENT_POLL_PERIOD 1000

    /*
     * Make sure that the count is aligned with the tick
     */
    timeleft -= (timeleft % AGOS_WAIT_ON_EVENT_POLL_PERIOD);

    while(1)
    {
	if ((((x_event_group->events & n_requested_flags) == n_requested_flags) &&
	     (e_operation == AGOS_EVENTS_AND || e_operation == AGOS_EVENTS_AND_CLEARED)) ||
	    ((x_event_group->events & n_requested_flags) && 
	     (e_operation == AGOS_EVENTS_OR || e_operation == AGOS_EVENTS_OR_CLEARED)))
	{
	  /*	  printk("%s: Event received\n", __func__); */

	    *n_retrieved_flags = x_event_group->events;

	    if (e_operation == AGOS_EVENTS_OR_CLEARED ||
		e_operation == AGOS_EVENTS_AND_CLEARED)
		x_event_group->events &= ~n_requested_flags;

	    return AG_S_OK;
	}

	sal_usleep(MILLISECOND_USEC * AGOS_WAIT_ON_EVENT_POLL_PERIOD);

	if (n_timeout != 0xffffffff)
	{
	    timeleft -= AGOS_WAIT_ON_EVENT_POLL_PERIOD;
	    if (timeleft == 0)
	    {
		*n_retrieved_flags = 0;
		return AG_E_TIMEOUT;
	    }
	}
    }

    return AG_S_OK;
}

AgResult agos_delete_event_group(AgosEventInfo *x_event_group)
{
    return AG_S_OK;
}

AgResult agos_create_task(AgosTaskInfo *x_task_info,
    AG_U8       *p_name,
    void        (*p_task_entry_function)(AG_U32, void*),
    AG_U32      n_param,
    void        *p_stack,
    AG_U32      n_stack_size,
    AG_U32      n_priority,
    AG_U32 slice,
    AG_U32 preempt,
    AG_U32 start)
{
    CES_SAL_DEBUG((DK_VERBOSE,"%s: name:%s priority:%lu\n", __func__, p_name, n_priority));
  if ((x_task_info->thread = sal_thread_create((AG_S8*)p_name, n_stack_size, (n_priority + 100), (void (*)(void*))p_task_entry_function, (void*)n_param)) == SAL_THREAD_ERROR)
    {
      return AG_E_FAIL;
    }

  return AG_S_OK;
}



 
 /*Log*/
 void x_sw_logger_app_info(void){};
 void ag_swlog_print_by_level(void){};






/*
 * Memory
 */
AgResult agos_calloc(int n_size, void *ptr)
{
  *((void**)ptr) = sal_alloc(n_size, "agos_calloc");
  return AG_S_OK;
}


AgResult agos_malloc(int n_size, void *ptr)
{
  *((void**)ptr) = sal_alloc(n_size, "agos_malloc");
  return AG_S_OK;
}

AgResult agos_free(void *ptr)
{
  sal_free(ptr);
  return AG_S_OK;
}

void *ag_memset(void *to, AG_S32 c, size_t size)
{
  char *d = (char *)to;
  size_t count = 0;

  while (count < size)
    {
      *d = (char)c;
      d++;
      count++;
    }

  return to;
}

void *ag_memcpy_asm(void * dest, const void * src, size_t len)
{
  memcpy(dest, src, len);
  return dest;
}

void agos_plat_malloc(void)
{
    /* Currently not used */
    CES_SAL_DEBUG((DK_VERBOSE,"%s - NOT YET IMPLEMENTED\n", __func__));
    return;
}
void agos_plat_free(void)
{
    /* Currently not used */
    CES_SAL_DEBUG((DK_VERBOSE,"%s - NOT YET IMPLEMENTED\n", __func__));
    return;
}

void *memset_word(void *dest, AG_U32 n, size_t size)
{
    AG_U32 *address = (AG_U32*)dest;
    size_t count = 0;

    /*CES_SAL_DEBUG((DK_VERBOSE,"%s: address:%p len:%d data:0x%08lx\n", __func__, address, size, n));*/

    while (count < size)
    {
	*address = n;
	address++;
	count += 4; /* Size is in bytes so increment by four */
    }

    return dest;
}

void memset_qword(void)
{
  CES_SAL_DEBUG((DK_VERBOSE,"%s - NOT YET IMPLEMENTED\n", __func__));
}


#endif /* BCM_CES_SDK */




