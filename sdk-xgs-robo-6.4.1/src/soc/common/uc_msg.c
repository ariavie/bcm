/*
 * $Id: uc_msg.c,v 1.55 Broadcom SDK $
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
 * Purpose:     SOC DMA LLC (Link Layer) driver; used for sending
 *              and receiving packets over PCI (and later, the uplink).
 *
 * NOTE:  DV chains are different than hardware chains. The hardware
 * chain bit is used to keep DMA going through multiple packets.
 * This corresponds to the sequence of DCBs (and usually multiple
 * packets) of one DV.  DVs are chained in a linked list in software.
 * The dv->dv_chain member (and dv_chain parameters) implement this
 * linked list.
 */

#include <shared/bsl.h>

#include <stddef.h>             /* for offsetof */

#include <sal/core/boot.h>
#include <sal/core/libc.h>
#include <shared/alloc.h>

#include <soc/drv.h>
#include <soc/dcb.h>
#include <soc/dcbformats.h>     /* only for 5670 crc erratum warning */
#include <soc/pbsmh.h>
#include <soc/higig.h>
#include <soc/debug.h>
#include <soc/cm.h>
#include <soc/error.h>
#include <soc/property.h>

#if defined(BCM_CMICM_SUPPORT) || defined(BCM_IPROC_SUPPORT)
#include <soc/cmicm.h>
#include <soc/shared/mos_intr_common.h>
#include <soc/uc_msg.h>
#include <soc/uc.h>

/* Macro to do 32-bit address calculations for CMICm-based addresses.
   These are NOT local addresses and can only be accessed over the
   PCIe or via CMICm-based DMA operations */

#define CMICM_ADDR(base, field) \
    ((uint32) ((base) + offsetof(mos_msg_area_t, field)))

#define CMICM_DATA_WORD0_BASE(base) \
	((uint32) ((base) + offsetof(mos_msg_area_t,data[0].words[0])))
#define CMICM_DATA_WORD1_BASE(base) \
	((uint32) ((base) + offsetof(mos_msg_area_t,data[0].words[1])))
#define MOS_MSG_DATA_SIZE  sizeof(mos_msg_data_t)

/* Internal routine to determine the base address of the msg area */
/*
 * Function:
 *      _soc_cmic_uc_msg_base
 * Purpose:
 *      Determine the base address of the msg area
 * Parameters:
 *      unit - unit number
 * Returns:
 *      address of the msg area, or NULL
 */
static uint32 _soc_cmic_uc_msg_base(int unit)
{
    if (SOC_IS_HELIX4(unit) || SOC_IS_KATANA2(unit)) {
        return 0x1b07f008;
#ifdef BCM_HURRICANE2_SUPPORT
    } else if (SOC_IS_HURRICANE2(unit)) {
        return 0x000f0000;
#endif
    }

    return CMICM_MSG_AREAS;
}

 /* Static routine to add received message to receive LL
 *Inserts an element at tail of LL */
static void ll_insert_tail(ll_ctrl_t *p_ll_ctrl,  ll_element_t *p_element)
{
    assert(p_element && p_ll_ctrl);

    p_element->p_prev = p_ll_ctrl->p_tail;
    p_element->p_next = NULL; 

    if(p_ll_ctrl->ll_count) {
        /* LL is not empty */
        p_ll_ctrl->p_tail->p_next = p_element;
    } else { 
        /* LL is empty */
        assert(!p_ll_ctrl->p_tail && !p_ll_ctrl->p_head);
        p_ll_ctrl->p_head = p_element;
    }

    p_ll_ctrl->ll_count++;
    p_ll_ctrl->p_tail = p_element;
}

/* Static routine to remove message from receive LL
 *Removes an element from head of LL */
static ll_element_t* ll_remove_head (ll_ctrl_t *p_ll_ctrl)
{
    ll_element_t* p_el;
    
    assert(p_ll_ctrl);

    if(p_ll_ctrl->ll_count) {
        /* list is not empty */
        assert(p_ll_ctrl->p_head && p_ll_ctrl->p_tail);

        /* keep the removed head */
        p_el = p_ll_ctrl->p_head;

       /*update ll_ctrl*/
        p_ll_ctrl->p_head = p_el->p_next;
        p_ll_ctrl->ll_count--;
        if(p_ll_ctrl->ll_count) {
            /* there are still elements in the list */
            assert(p_ll_ctrl->p_head);
            p_ll_ctrl->p_head->p_prev = NULL;
        } else {
            /* p_el was the only list element */
            assert(p_ll_ctrl->p_tail == p_el && !p_ll_ctrl->p_head);
            p_ll_ctrl->p_tail = NULL; 
        }

        /* disconnect the removed element from the list */
        p_el->p_next = NULL;
        p_el->p_prev = NULL;
		
        return p_el;
    }
    return NULL;

}

/* Internal routine to process a status word */
/*
 * Function:
 *      _soc_cmic_uc_msg_process_status
 * Purpose:
 *      Process the uC message status from a uC
 * Parameters:
 *      unit - unit number
 *      uC - uC number
 * Returns:
 *      0 - OK
 *      1 - uC reset
 */
 
int _soc_cmic_uc_msg_process_status(int unit, int uC) {
    soc_control_t       *soc = SOC_CONTROL(unit);
    
    uint32 msg_base = _soc_cmic_uc_msg_base(unit);
    uint32 area_in = MSG_AREA_ARM_TO_HOST(msg_base, uC);
    uint32 area_out = MSG_AREA_HOST_TO_ARM(msg_base, uC);
    
    /* Snapshot the status from the uC memory */
    mos_msg_host_status_t cur_status_in;

    int         out_status = 0;        /* Set when new out status */
    int         i;
    mos_msg_ll_node_t *msg_node = NULL;

    cur_status_in = soc_pci_mcs_read(unit, CMICM_ADDR(area_in, status));

    if (MOS_MSG_STATUS_STATE(cur_status_in) == MOS_MSG_RESET_STATE) {  /* Got a reset */
        return (1);                                  /* Restart init  */
    } 

    sal_mutex_take(soc->uc_msg_control, sal_sem_FOREVER);

    /*  Process the ACKS */
    for (i = MOS_MSG_STATUS_ACK_INDEX(soc->uc_msg_prev_status_in[uC]);
         i != MOS_MSG_STATUS_ACK_INDEX(cur_status_in);
         i = MOS_MSG_INCR(i)) {
        
        /* See if any thread is waiting for an ack */
        if (soc->uc_msg_ack_sems[uC][i] != NULL) {
            *soc->uc_msg_ack_data[uC][i] =
                MOS_MSG_ACK_BIT(cur_status_in, i);
            sal_sem_give(soc->uc_msg_ack_sems[uC][i]);
            soc->uc_msg_ack_sems[uC][i] = NULL;
            soc->uc_msg_ack_data[uC][i] = NULL;
        }

        /* Give back one for each ACK received */
        sal_sem_give(soc->uc_msg_send_queue_sems[uC]);
        
    }

    /* Process the messages */
    for (i = MOS_MSG_STATUS_SENT_INDEX(soc->uc_msg_prev_status_in[uC]);
         i != MOS_MSG_STATUS_SENT_INDEX(cur_status_in); i = MOS_MSG_INCR(i)) {

        /* Read the message data in in 2 32-bit chunks */
        mos_msg_data_t msg;
        msg.words[0] = soc_pci_mcs_read(unit,CMICM_DATA_WORD0_BASE(area_in) +
                            MOS_MSG_DATA_SIZE * i);                  
        msg.words[1] = soc_pci_mcs_read(unit,CMICM_DATA_WORD1_BASE(area_in) +
                            MOS_MSG_DATA_SIZE * i);  

        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "UC%d: msg recv 0x%08x 0x%08x\n"),
                     uC, msg.words[0], msg.words[1]));

	/* swap to host endian */
	msg.words[0] = soc_ntohl(msg.words[0]);
	msg.words[1] = soc_ntohl(msg.words[1]);
        
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "UC%d: msg recv mclass 0x%02x sclass 0x%02x len 0x%04x data 0x%08x\n"),
                     uC, msg.s.mclass, msg.s.subclass,
                     msg.s.len, msg.s.data));
        if (msg.s.mclass > MAX_MOS_MSG_CLASS) {
            MOS_MSG_ACK_BIT_CLEAR(soc->uc_msg_prev_status_out[uC], i);
        } else if ((msg.s.mclass == MOS_MSG_CLASS_SYSTEM) &&
                   (msg.s.subclass == MOS_MSG_SUBCLASS_SYSTEM_PING)) {
            /* Special case - PING just gets an ACK */
            MOS_MSG_ACK_BIT_SET(soc->uc_msg_prev_status_out[uC], i);
        } else {
            /* Enqueue the message in respective LL */
            msg_node = (mos_msg_ll_node_t *)sal_alloc(sizeof(mos_msg_ll_node_t), "UC Msg Node");
            if (msg_node != NULL) {
                MOS_MSG_ACK_BIT_SET(soc->uc_msg_prev_status_out[uC], i);
                msg_node->msg_data.words[0] = msg.words[0];
                msg_node->msg_data.words[1] = msg.words[1];
                ll_insert_tail(&(soc->uc_msg_rcvd_ll[uC][msg.s.mclass]), (ll_element_t *)msg_node);
                
                /* Kick the waiter */
                sal_sem_give(soc->uc_msg_rcv_sems[uC][msg.s.mclass]);

            } else {
                /* Malloc failed, no room for the message - NACK it */
                MOS_MSG_ACK_BIT_CLEAR(soc->uc_msg_prev_status_out[uC], i);
                LOG_ERROR(BSL_LS_SOC_COMMON,
                          (BSL_META_U(unit,
                                      "Could not malloc, Sending NACK, Msg Class - %d\n"), msg.s.mclass));
            }
        }

        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "UC%d: Ack slot %d 0x%08x 0x%08x\n"),
                     uC, i, msg.words[0], msg.words[1]));
        out_status = 1;
    } 

    /* Update the status out with the final index */
    MOS_MSG_STATUS_ACK_INDEX_W(soc->uc_msg_prev_status_out[uC], i);
        
    /* Remember previous status for next time */
    soc->uc_msg_prev_status_in[uC] = cur_status_in;

    if (out_status) {
        /*  We have determined the new status out - write it and notify
            the other uC by writing the word once here and
            interrupting the uC*/
        soc_pci_mcs_write(unit, CMICM_ADDR(area_out, status),
                          soc->uc_msg_prev_status_out[uC]);
        CMIC_CMC_SW_INTR_SET(unit, CMICM_SW_INTR_UC(uC));
    }

    sal_mutex_give(soc->uc_msg_control);

    return (0);
}

/*
 * Function:
 *      soc_cmic_uc_msg_active_wait
 * Purpose:
 *      To wait for the message system to becone active
 * Parameters:
 *      unit - unit number
 *      uC - microcontroller number
 * Returns:
 *      SOC_E_NONE - Handshake complete, msg active
 *      SOC_E_UNAVAIL - system shut down
 */

int
soc_cmic_uc_msg_active_wait(int unit, uint32 uC) {
    soc_control_t       *soc = SOC_CONTROL(unit);

    if ((soc == NULL) || ((soc->swIntrActive & (1 << uC)) == 0) ||
        (soc->uc_msg_active[uC] == NULL)) {
        return (SOC_E_UNAVAIL);
    }

    /* Wait for the semaphore to be available */
    if (!sal_sem_take(soc->uc_msg_active[uC], 10000000)) {
        sal_sem_give(soc->uc_msg_active[uC]);
    }

    return (SOC_E_NONE);
}
    
/*
 * Function:
 *      _soc_cmic_uc_msg_system_thread
 * Purpose:
 *      Thread to system messages
 * Parameters:
 *      unit - unit number
 * Returns:
 *      Nothing
 */

void
_soc_cmic_uc_msg_system_thread(void *unit_vp) {
    int                 unit = PTR_TO_INT(unit_vp);
    soc_control_t       *soc = SOC_CONTROL(unit);
    int                 uC;
    mos_msg_data_t      rcv, send;

    if (soc == NULL) {
        return;
    }
    
    /* Figure out which uC we are processing */
    sal_mutex_take(soc->uc_msg_system_control, sal_sem_FOREVER);
    uC = soc->uc_msg_system_count++;
    sal_mutex_give(soc->uc_msg_system_control);

    while (1) {
        while (soc_cmic_uc_msg_receive(unit, uC, MOS_MSG_CLASS_SYSTEM,
                                       &rcv, sal_sem_FOREVER) != SOC_E_NONE) {
            if (soc_cmic_uc_msg_active_wait(unit, uC) != SOC_E_NONE) {
                /* Down and not coming back up - exit thread */
                LOG_CLI((BSL_META_U(unit,
                                    "System thread exiting\n")));
                return;
            }
        } 

        if (rcv.s.subclass == MOS_MSG_SUBCLASS_SYSTEM_LOG) {
                
        } else if (rcv.s.subclass == MOS_MSG_SUBCLASS_SYSTEM_INFO) {
            send.s.mclass = MOS_MSG_CLASS_SYSTEM;
            send.s.subclass = MOS_MSG_SUBCLASS_SYSTEM_INFO_REPLY;
            send.s.data = ~0;

            if (soc_cmic_uc_msg_send(unit, uC, &send,
                                     soc->uc_msg_send_timeout) == SOC_E_NONE) {
                /* Indicate to waiters that we are up */
                sal_sem_give(soc->uc_msg_active[uC]);
            }
                            
        } else if (rcv.s.subclass == MOS_MSG_SUBCLASS_SYSTEM_STATS) {
            /* TBD - drop for now */
                
        } else if (rcv.s.subclass == MOS_MSG_SUBCLASS_SYSTEM_CONSOLE_IN) {
            /* TBD - drop for now */
            
        } else if (rcv.s.subclass == MOS_MSG_SUBCLASS_SYSTEM_CONSOLE_OUT) {
            LOG_CLI((BSL_META_U(unit,
                                "%c"), rcv.s.data & 0x0ff));
            
        }
    }
 }

/*
 * Function:
 *      _soc_cmic_uc_msg_thread
 * Purpose:
 *      Thread to handle comminucations with UCs (ARMs)
 * Parameters:
 *      unit - unit number
 * Returns:
 *      Nothing
 */

void
_soc_cmic_uc_msg_thread(void *unit_vp) {
    int                 unit = PTR_TO_INT(unit_vp);
    soc_control_t       *soc = SOC_CONTROL(unit);
    int                 i;
    int                 uC = 0;
    int                 uC_intr;

    uint32 msg_base;
    uint32 area_in;
    uint32 area_out;

    mos_msg_host_status_t cur_status_in;
    sal_sem_t tsem;
    mos_msg_ll_node_t *msg_node;
    
    /* Use the message control mutex to pick a UC */
    sal_mutex_take(soc->uc_msg_control, sal_sem_FOREVER);

    /* Our uC is the first one with an uninitialized interrupt semaphore */
    while ((soc->swIntr[CMICM_SW_INTR_UC(uC)] != NULL) ||
           ((soc->swIntrActive & (1 << uC)) == 0)) {
        uC++;
    }

    /* This uC is ready to start */
    uC_intr = CMICM_SW_INTR_UC(uC);
    soc->swIntr[uC_intr] = sal_sem_create("SW interrupt",
                                       sal_sem_BINARY, 0);
    if (soc->swIntr[CMICM_SW_INTR_UC(uC)] == NULL) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "soc_cmic_uc_msg_thread: failed (swIntr) %d\n"), uC));
        sal_mutex_give(soc->uc_msg_control);
        return;
    }

    soc_cmicm_intr0_enable(unit, IRQ_CMCx_SW_INTR(uC_intr));
    
    sal_mutex_give(soc->uc_msg_control);

    /* These are addresses in the uC SRAM and not directly addressible
       from the SDK - they are addesses for PCI operations */
    msg_base = _soc_cmic_uc_msg_base(unit);
    area_in = MSG_AREA_ARM_TO_HOST(msg_base, uC);
    area_out = MSG_AREA_HOST_TO_ARM(msg_base, uC);

    /* While not killed off */
    while (1) {
        /* We must write the IN status area to zero.  This is becuase
        the ECC for the SRAM where it lives might not be initialized
        and the resulting ECC error would cause a PCIe error which
        would cause the SDK to crash.  Zero is safe in this protocol
        since we will not consider that a reset or init state */
        soc_pci_mcs_write(unit, CMICM_ADDR(area_in, status), 0);
        
        soc->uc_msg_prev_status_out[uC] = 0;

        /* Handshake protocol:

               Master (SDK host):
                   (restart or receive RESET state)
                   RESET STATE
                   Wait for INIT state
                   INIT state
                   Wait for READY state
                   READY state

               Slave (uC)
                   (retart or recieve RESET state)
                   RESET state
                   Wait for RESET state
                   INIT state
                   Wait for INIT state
                   READY sate

            */

            
        /* Phase 1 of init handshake (wait for INIT/RESET state) */
        while (1) {
            cur_status_in = soc_pci_mcs_read(unit, CMICM_ADDR(area_in, status));
            if (MOS_MSG_STATUS_STATE(cur_status_in) == MOS_MSG_INIT_STATE) {
                /* Our partner is the subordinate so we go to
                   INIT state when we see him go to INIT state */
                break;
            }

            /* Set the status and write it out.  We do that inside the
               loop in the first phase to avoid a race where the uKernel
               is clearing memory to clear ECC errors and the first write
               is lost.  This is not needed after the first phase since
               we see the uKernel msg area change state. */
            LOG_VERBOSE(BSL_LS_SOC_COMMON,
                        (BSL_META_U(unit,
                                    "UC%d messaging system: reset\n"), uC));
            soc->uc_msg_prev_status_out[uC] = MOS_MSG_RESET_STATE;
            soc_pci_mcs_write(unit, CMICM_ADDR(area_out, status),
                          soc->uc_msg_prev_status_out[uC]);
            CMIC_CMC_SW_INTR_SET(unit, uC_intr);          /* Wake up call */
            
            if (sal_sem_take(soc->swIntr[uC_intr], sal_sem_FOREVER)) {
                goto thread_stop;
            } 
            if ((soc->swIntrActive & (1 << uC)) == 0) { /* If exit signaled */
                goto thread_stop;
            } 
        }
        
        /* Proceed to INIT state */
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "UC%d messaging system: init\n"), uC));
        MOS_MSG_STATUS_STATE_W(soc->uc_msg_prev_status_out[uC], MOS_MSG_INIT_STATE);
        soc_pci_mcs_write(unit, CMICM_ADDR(area_out, status),
                          soc->uc_msg_prev_status_out[uC]);
        CMIC_CMC_SW_INTR_SET(unit, uC_intr);         /* Wake up call */
                                               
        /* Phase 2 of init handshake (wait for READY/INIT state */
        while (1) {
            /* Get the status once */
            cur_status_in = soc_pci_mcs_read(unit, CMICM_ADDR(area_in, status));
            if (MOS_MSG_STATUS_STATE(cur_status_in) == MOS_MSG_READY_STATE) {
                /* Our partner is the subordinate so we go to
                   READY state when we see him go to READY state */
                break;
            } else if (MOS_MSG_STATUS_STATE(cur_status_in) == MOS_MSG_RESET_STATE) {
                /* The subordinate has requested another
                   reset.  process_msg_status will detect this
                   and we will go to the top of the loop */
                break;
            }
            
            if (sal_sem_take(soc->swIntr[uC_intr], sal_sem_FOREVER)) {
                goto thread_stop;
            } 
            if ((soc->swIntrActive & (1 << uC)) == 0) { /* If exit signaled */
                goto thread_stop;
            } 
        }

        /* Go to the ready state */
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "UC%d messaging system: ready\n"), uC));
        MOS_MSG_STATUS_STATE_W(soc->uc_msg_prev_status_out[uC], MOS_MSG_READY_STATE);
        soc_pci_mcs_write(unit, CMICM_ADDR(area_out, status),
                          soc->uc_msg_prev_status_out[uC]);
        soc->uc_msg_prev_status_in[uC] = cur_status_in;
        CMIC_CMC_SW_INTR_SET(unit, uC_intr);         /* Wake up call */


        LOG_CLI((BSL_META_U(unit,
                            "UC%d messaging system: up\n"), uC));
        
        /* Give one count for each slot (all free to start) */
        for (i = 0; i < NUM_MOS_MSG_SLOTS; i++) {
            sal_sem_give(soc->uc_msg_send_queue_sems[uC]);
        }


        /* Start normal processing */
        while (1) {
            if (_soc_cmic_uc_msg_process_status(unit, uC)) {
                break;              /* Received reset */
            }
            if (sal_sem_take(soc->swIntr[uC_intr], sal_sem_FOREVER)) {
                goto thread_stop;                
            } 
            if ((soc->swIntrActive & (1 << uC)) == 0) {
                /* If exit signaled */
                goto thread_stop;
            }
        }

        LOG_CLI((BSL_META_U(unit,
                            "UC messaging back to reset\n")));
        
        /* Connection failed - go back to reset */
        
        if ((soc->swIntrActive & (1 << uC)) == 0) {
            /* If exit signaled */
            goto thread_stop;
        }

        sal_sem_take(soc->uc_msg_active[uC], 0);      /* Get or timeout */

        /* Take all of the slots */
        for (i = 0; i < NUM_MOS_MSG_SLOTS; i++) {
            /* These will timeout immedaitely */
            sal_sem_take(soc->uc_msg_send_queue_sems[uC], 0);
        }
            
        /*Clear receive LL and set Semaphore count to 0*/
        sal_mutex_take(soc->uc_msg_control, sal_sem_FOREVER);
        for (i = 0; i <= MAX_MOS_MSG_CLASS; i++) {
            while( soc->uc_msg_rcvd_ll[uC][i].ll_count ){
                msg_node = (mos_msg_ll_node_t *)ll_remove_head(&(soc->uc_msg_rcvd_ll[uC][i]));
                if (msg_node != NULL){
                    sal_free(msg_node);
                }
                /*Decrement the counting semaphore count to be in sync with LL*/
                sal_sem_take(soc->uc_msg_rcv_sems[uC][i], 10000000);
            };
            
            /* Kick the waiter */
            sal_sem_give(soc->uc_msg_rcv_sems[uC][i]);
        }
        sal_mutex_give(soc->uc_msg_control);
        /*Let the receive thread kick in and exit*/
        sal_thread_yield();
        
        for (i = 0; i < NUM_MOS_MSG_SLOTS; i++) {
            if (soc->uc_msg_ack_sems[uC][i] != NULL) {
                /* Kick the waiter */
                tsem = soc->uc_msg_ack_sems[uC][i];
                
                soc->uc_msg_ack_sems[uC][i] = NULL;
                sal_sem_give(tsem);
            }
        }
        
    } /* end while (1) */

 thread_stop:    

    LOG_CLI((BSL_META_U(unit,
                        "UC msg thread dies %x\n"), uC));

    sal_sem_take(soc->uc_msg_active[uC], 0);
    
    /* NAK all of the current messages */
    for (i = 0; i < NUM_MOS_MSG_SLOTS; i++) {
        if (soc->uc_msg_ack_sems[uC][i] != NULL) {
            tsem = soc->uc_msg_ack_sems[uC][i];
                
            soc->uc_msg_ack_sems[uC][i] = NULL;
            sal_sem_give(tsem);
        }
    } 


    /* Init the rest of our structures */
    sal_sem_destroy(soc->uc_msg_send_queue_sems[uC]);
    soc->uc_msg_send_queue_sems[uC] = NULL;
    
    /*Clear receive LL and set Semaphore count to 0*/
    sal_mutex_take(soc->uc_msg_control, sal_sem_FOREVER);
    for (i = 0; i <= MAX_MOS_MSG_CLASS; i++) {
        while( soc->uc_msg_rcvd_ll[uC][i].ll_count ) {
            msg_node = (mos_msg_ll_node_t *)ll_remove_head(&(soc->uc_msg_rcvd_ll[uC][i]));
            if (msg_node != NULL) {
                sal_free(msg_node);
            }
            /*Decrement the counting semaphore count to be in sync with LL*/
            sal_sem_take(soc->uc_msg_rcv_sems[uC][i], 10000000);
        };

        /* Kick the waiter */
        sal_sem_give(soc->uc_msg_rcv_sems[uC][i]);
    }
    sal_mutex_give(soc->uc_msg_control);
    /*Let the receive thread kick in and exit*/
    sal_thread_yield();
    
    /* Use the message control mutex to shut down */
    sal_mutex_take(soc->uc_msg_control, sal_sem_FOREVER);
    sal_sem_destroy(soc->swIntr[uC_intr]);
    soc->swIntr[uC_intr] = NULL;
    sal_mutex_give(soc->uc_msg_control);
}

/*
 * Function:
 *      soc_cmic_sw_intr
 * Purpose:
 *      Process a S/W generated interrupt from one of the ARMs
 * Parameters:
 *      s - pointer to socket structure.
 *      rupt_num - sw itnerrupt number
 * Returns:
 *      Nothing
 * Notes:
 *      INTERRUPT LEVEL ROUTINE
 */

void
soc_cmic_sw_intr(int unit, uint32 rupt_num)
{
    soc_control_t       *soc = SOC_CONTROL(unit);
    int                 cmc = SOC_PCI_CMC(unit);

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "%s: SW Intr %d\n"), FUNCTION_NAME(), rupt_num));

    /* Clear the interrupt source */
    soc_pci_write(unit, CMIC_CMCx_SW_INTR_CONFIG_OFFSET(cmc), rupt_num);

    /* Re-enable interrupts (including this one) */
    (void)soc_cmicm_intr0_enable(unit, 0);


    if (soc->swIntr[rupt_num]) {
        sal_sem_give(soc->swIntr[rupt_num]);
    }
}


/*
 * Function:
 *      soc_cmic_uc_msg_init
 * Purpose:
 *      Do nothing - threads started elsewhere
 * Parameters:
 *      unit - unit number
 * Returns:
 *      Nothing
 */

void
soc_cmic_uc_msg_init(int unit) {
    COMPILER_REFERENCE(unit);
}

/*
 * Function:
 *      soc_cmic_uc_msg_start
 * Purpose:
 *      Start communication sessions with the UCs
 * Parameters:
 *      unit - unit number
 * Returns:
 *      SOC_E_xxx
 */

int
soc_cmic_uc_msg_start(int unit) {
    soc_control_t       *soc = SOC_CONTROL(unit);
    int                 i;

    if ((soc == NULL) || (soc->uc_msg_control != NULL)) {
        return (SOC_E_INIT);
    }
    
    soc->swIntrActive = 0;
    soc->uc_msg_control = NULL;
    
    /* Get a few properties for efficiency */
    soc->uc_msg_queue_timeout =
        soc_property_get(unit, spn_UC_MSG_QUEUE_TIMEOUT, 200000000);
    soc->uc_msg_ctl_timeout =
        soc_property_get(unit, spn_UC_MSG_CTL_TIMEOUT, 1000000);
    soc->uc_msg_send_timeout =
        soc_property_get(unit, spn_UC_MSG_SEND_TIMEOUT, 10000000);
    soc->uc_msg_send_retry_delay =
        soc_property_get(unit, spn_UC_MSG_SEND_RETRY_DELAY, 100);

    /* Init a few fields so that the threads can sort themselves out */
    soc->uc_msg_control = sal_mutex_create("Msgctrl");
    for (i = 0; i < COUNTOF(soc->swIntr); i++) {
        soc->swIntr[i] = NULL;
    }

    for (i = 0; i < COUNTOF(soc->uc_msg_active); i++) {
        /* Init the active sem and hold it */
        soc->uc_msg_active[i] = sal_sem_create("msgActive",
                                               sal_sem_BINARY, 0);
    }

    /* Init things for the system msg threads */
    soc->uc_msg_system_control = sal_mutex_create("SysMsgCtrl");
    soc->uc_msg_system_count = 0;

    return (SOC_E_NONE);
}


/*
 * Function:
 *      soc_cmic_uc_msg_stop
 * Purpose:
 *      Stop communication sessions with the UCs
 * Parameters:
 *      unit - unit number
 * Returns:
 *      SOC_E_xxx
 */

int
soc_cmic_uc_msg_stop(int unit) {
    soc_control_t       *soc = SOC_CONTROL(unit);
    int                 uC;

    if ((soc == NULL) || (soc->swIntrActive == 0)) {
        return (SOC_E_INIT);      /* Already shut down */
    }
    
    /* Kill all of the msg threads */
    for (uC = 0; uC < COUNTOF(soc->uc_msg_active); uC++) {
        soc_cmic_uc_msg_uc_stop(unit, uC);

        /* Destroy the semaphore.  Threads using it must be quiecsed */
        if (soc->uc_msg_active[uC] != NULL) {
            sal_sem_destroy(soc->uc_msg_active[uC]);
            soc->uc_msg_active[uC] = NULL;
        }
    }

    /* Destroy control mutex */
    if (soc->uc_msg_control != NULL) {
        sal_mutex_destroy(soc->uc_msg_control);
        soc->uc_msg_control = NULL;
    }

    /* Destroy system control mutex */
    if (soc->uc_msg_system_control != NULL) {
        sal_mutex_destroy(soc->uc_msg_system_control);
        soc->uc_msg_system_control = NULL;
    }


    return (SOC_E_NONE);
}

/*
 * Function:
 *      soc_cmic_uc_msg_uc_start
 * Purpose:
 *      Start communicating with one of the uCs
 * Parameters:
 *      unit - unit number
 *      uC - uC number
 * Returns:
 *      SOC_E_xxxx
 */

int
soc_cmic_uc_msg_uc_start(int unit, int uC) {
    char                prop_buff[20];
    soc_control_t       *soc = SOC_CONTROL(unit);
    int                 i;

    sal_sprintf(prop_buff, "uc_msg_ctrl_%i", uC);
    if (soc_property_get(unit, prop_buff, 1) == 0) {
        return (SOC_E_UNAVAIL);
    }

    /* Check for the master control for this uC */
    if ((soc == NULL) || ((soc->swIntrActive & (1 << uC)) != 0)) {
        return (SOC_E_BUSY);
    }
    
    sal_mutex_take(soc->uc_msg_control, sal_sem_FOREVER);
    
    /* Init some data structures */
    soc->uc_msg_send_queue_sems[uC] =
        sal_sem_create("uC msg queue", sal_sem_COUNTING, 0);
    if (soc->uc_msg_send_queue_sems[uC] == NULL) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "soc_cmic_uc_msg_thread: failed (uC msg) %d\n"), uC));
        sal_mutex_give(soc->uc_msg_control);
        return (SOC_E_MEMORY);
    }

    /* Initialize receive LL, Create receive semaphores,*/
    for (i = 0; i <= MAX_MOS_MSG_CLASS; i++) {
        soc->uc_msg_rcvd_ll[uC][i].p_head = NULL;
        soc->uc_msg_rcvd_ll[uC][i].p_tail = NULL;
        soc->uc_msg_rcvd_ll[uC][i].ll_count = 0;
        soc->uc_msg_rcv_sems[uC][i] = sal_sem_create("us_msg_rcv", sal_sem_COUNTING, 0);
    }
    
    /* Clear Ack Semaphores and data*/    
    for (i = 0; i < NUM_MOS_MSG_SLOTS; i++) {
        soc->uc_msg_ack_sems[uC][i] = NULL;
        soc->uc_msg_ack_data[uC][i] = NULL;
    }

    soc->swIntrActive |= 1 << uC;   /* This uC is active */
    sal_thread_create("uC msg", SAL_THREAD_STKSZ,
                      soc_property_get(unit, spn_UC_MSG_THREAD_PRI, 95),
                      _soc_cmic_uc_msg_thread, INT_TO_PTR(unit));
    sal_thread_create("uC system msg", SAL_THREAD_STKSZ,
                      soc_property_get(unit, spn_UC_MSG_THREAD_PRI, 100),
                      _soc_cmic_uc_msg_system_thread, INT_TO_PTR(unit));

    sal_mutex_give(soc->uc_msg_control);

    return (SOC_E_NONE);
}

/*
 * Function:
 *      soc_cmic_uc_msg_uc_stop
 * Purpose:
 *      Stop communicating with one of the uCs
 * Parameters:
 *      unit - unit number
 *      uC - uC number
 * Returns:
 *      SOC_E_xxx
 */

int
soc_cmic_uc_msg_uc_stop(int unit, int uC) {
    soc_control_t       *soc = SOC_CONTROL(unit);
    int i;

    /* Check for the master control for this uC */
    if ((soc == NULL) || ((soc->swIntrActive & (1 << uC)) == 0)) {
        return (SOC_E_INIT);
    }

    sal_mutex_take(soc->uc_msg_control, sal_sem_FOREVER);
    soc->swIntrActive &= ~(1 << uC);   /* This uC is inactive */
    
    if (soc->swIntr[CMICM_SW_INTR_UC(uC)] != NULL) {
        sal_sem_give(soc->swIntr[CMICM_SW_INTR_UC(uC)]); /* Kick msg thread */

    }
    
    while (1) {
        if (soc->swIntr[CMICM_SW_INTR_UC(uC)] == NULL) {
            break;                      /* It is shut down */
        } 
        sal_mutex_give(soc->uc_msg_control);
        sal_thread_yield();
        sal_mutex_take(soc->uc_msg_control, sal_sem_FOREVER);
    }

    /*Destroy receive semaphores*/
    for (i = 0; i <= MAX_MOS_MSG_CLASS; i++) {
        sal_sem_give(soc->uc_msg_rcv_sems[uC][i]);
        sal_thread_yield();
        sal_sem_destroy(soc->uc_msg_rcv_sems[uC][i]);
        soc->uc_msg_rcv_sems[uC][i] = NULL; 
    }

    sal_mutex_give(soc->uc_msg_control);

    return (SOC_E_NONE);

}



/* Function:
 *      soc_cmic_uc_msg_send
 * Purpose:
 *      Send a message to another processor and retry until an ack
 * Parameters:
 *      unit - unit number
 *      uC - uC number
 *      msg - the 64-bit message to send
 *      timeout - time to wait in usecs or zero (no wait)
 * Returns:
 *      SOC_E_NONE = message ACK'd (agent received msg)
 *      SOC_E_xxx - message NAK'd (no agent or buffer space full or timeout)
 *
 */
int soc_cmic_uc_msg_send(int unit, int uC, mos_msg_data_t *msg,
                         sal_usecs_t timeout) {
    soc_control_t       *soc = SOC_CONTROL(unit);
    int32               msg_base;
    int                 rc;
    uint8               ack;
    int index;
    sal_sem_t           ack_sem;
    uint32 area_out;
    mos_msg_data_t	omsg;
    /* Figure out when to give up */
    sal_usecs_t end_time = sal_time_usecs() + timeout;

    if ((soc == NULL) || ((soc->swIntrActive & (1 << uC)) == 0)) {
        return SOC_E_INIT;
    }


    ack_sem = sal_sem_create("uc_msg_send", 1, 0);

    msg_base = _soc_cmic_uc_msg_base(unit);

    while (1) {                 /* Retry loop */

        rc = SOC_E_NONE;

        /* Wait for a semaphore indicating a free slot */
        if (sal_sem_take(soc->uc_msg_send_queue_sems[uC], soc->uc_msg_queue_timeout) == -1) {
            rc = SOC_E_TIMEOUT;
	    break;
        } 
    
        /* Block others for a bit */
        if (sal_mutex_take(soc->uc_msg_control, soc->uc_msg_ctl_timeout) == 0) {
            if (MOS_MSG_STATUS_STATE(soc->uc_msg_prev_status_out[uC]) !=
                MOS_MSG_READY_STATE) {
                sal_mutex_give(soc->uc_msg_control);
                rc = SOC_E_INIT;
		break;
            }
            
            assert(MOS_MSG_INCR(MOS_MSG_STATUS_SENT_INDEX(soc->uc_msg_prev_status_out[uC])) !=
                   MOS_MSG_STATUS_ACK_INDEX(soc->uc_msg_prev_status_in[uC]));

            /* Room for the message - update the local status copy */
            index = MOS_MSG_STATUS_SENT_INDEX(soc->uc_msg_prev_status_out[uC]);
            MOS_MSG_STATUS_SENT_INDEX_W(soc->uc_msg_prev_status_out[uC],
                                        MOS_MSG_INCR(index));
    
            soc->uc_msg_ack_data[uC][index] = &ack;
            soc->uc_msg_ack_sems[uC][index] = ack_sem;

            LOG_VERBOSE(BSL_LS_SOC_COMMON,
                        (BSL_META_U(unit,
                                    "UC%d: msg send mclass 0x%02x sclass 0x%02x len 0x%04x data 0x%08x\n"),
                         uC, msg->s.mclass, msg->s.subclass,
                         msg->s.len, msg->s.data));
	    omsg.words[0] = soc_htonl(msg->words[0]);
	    omsg.words[1] = soc_htonl(msg->words[1]);

            /* Update the outbound area that contains the new message */
	    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                        (BSL_META_U(unit,
                                    "UC%d: send slot %d 0x%08x 0x%08x\n"),
                         uC, index, omsg.words[0], omsg.words[1]));
            area_out = MSG_AREA_HOST_TO_ARM(msg_base, uC);
            soc_pci_mcs_write(unit, CMICM_DATA_WORD0_BASE(area_out) +
                                       MOS_MSG_DATA_SIZE * index, 
                                    omsg.words[0]);
            soc_pci_mcs_write(unit, CMICM_DATA_WORD1_BASE(area_out) +
                                       MOS_MSG_DATA_SIZE *index,
                                    omsg.words[1]);

            soc_pci_mcs_write(unit, CMICM_ADDR(area_out,status),
                              soc->uc_msg_prev_status_out[uC]);
    

            CMIC_CMC_SW_INTR_SET(unit, CMICM_SW_INTR_UC(uC));

            /* Return the global message control mutex */
            sal_mutex_give(soc->uc_msg_control);

            /* Wait for the ACK semaphore to be posted indicating that
               a response has been received */
            if (sal_sem_take(ack_sem, soc->uc_msg_send_timeout)) {
                soc->uc_msg_ack_data[uC][index] = NULL;
                soc->uc_msg_ack_sems[uC][index] = NULL;
                rc = SOC_E_TIMEOUT;
                break;
            }

            if (ack == 1) {
                rc = SOC_E_NONE;
                break;                  /* Message received OK */
            }

            if (end_time < sal_time_usecs()) {
                soc->uc_msg_ack_data[uC][index] = NULL;
                soc->uc_msg_ack_sems[uC][index] = NULL;
                rc = SOC_E_TIMEOUT;
                break;                  /* Timeout */
            }

            sal_usleep(soc->uc_msg_send_retry_delay);
        }
        
    }

    sal_sem_destroy(ack_sem);

    return (rc);
}

/*
 * Function
 *      soc_cmic_uc_msg_receive
 * Purpose:
 *      Wait for a message of a given type from a uC
 * Parameters:
 *      unit - unit number
 *      uC - uC number
 *      mclass - the message class
 *      msg - pointer to buffer for message (64 bits)
 * Returns:
 *      SOC_E_NONE = message ACK'd (agent received msg)
 *      SOC_E_xxx - message NAK'd (no agent or buffer space full or timeout)
 */
int soc_cmic_uc_msg_receive(int unit, int uC, uint8 mclass,
                            mos_msg_data_t *msg, int timeout)
{
    soc_control_t       *soc = SOC_CONTROL(unit);
    int                 rc = SOC_E_NONE;
    mos_msg_ll_node_t   *msg_node;

    if ((soc == NULL) || ((soc->swIntrActive & (1 << uC)) == 0)) {
        return SOC_E_INIT;
    }

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "%s: msg wait mclass %d\n"),
                 FUNCTION_NAME(), mclass));

    /* Wait/Check if msg available to be received */
    if ( !soc->uc_msg_rcv_sems[uC][mclass] || sal_sem_take(soc->uc_msg_rcv_sems[uC][mclass], timeout) ) {
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "%s: semtake  - uc_msg_rcv_sems failed\n"), FUNCTION_NAME()));
        return SOC_E_TIMEOUT;
    }

    /* Block others for a bit */
    if (sal_mutex_take(soc->uc_msg_control, soc->uc_msg_ctl_timeout)) {
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "%s: semtake  - uc_msg_control timed out\n"), FUNCTION_NAME()));
        return SOC_E_TIMEOUT;
    }
    
    /* Remove the message from Receive LL head*/
    msg_node = (mos_msg_ll_node_t *)ll_remove_head(&(soc->uc_msg_rcvd_ll[uC][mclass]));
    if (msg_node != NULL){
        msg->words[0] = msg_node->msg_data.words[0];
        msg->words[1] = msg_node->msg_data.words[1];
        sal_free(msg_node);
    } else {
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "%s: NULL node returned from LL\n"), FUNCTION_NAME()));
        sal_mutex_give(soc->uc_msg_control);
        return SOC_E_INTERNAL;
    }
    
    /* Return the global message control mutex */
    sal_mutex_give(soc->uc_msg_control);

    if ((rc == SOC_E_NONE) && (msg->s.mclass != mclass)) {
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "%s: Reply from wrong mclass\n"), FUNCTION_NAME()));
        rc = SOC_E_INTERNAL;            /* Reply from wrong mclass */
    }

    return (rc);
}

/* Function:
 *      soc_cmic_uc_msg_receive_cancel
 * Purpose:
 *      Cancel wait to receive  an application message
 * Parameters:
 *      unit - unit number
 *      uC - uC number
 *      msg_class - application message class
 * Returns:
 *      SOC_E_NONE = Waiters kicked or none waiting
 *      SOC_E_UNAVAIL = Feature not available
 *
 */
int soc_cmic_uc_msg_receive_cancel(int unit, int uC, int msg_class)
{
    soc_control_t       *soc = SOC_CONTROL(unit);
    mos_msg_ll_node_t   *msg_node;
    
    if (!soc_feature(unit, soc_feature_cmicm)) {
        return (SOC_E_UNAVAIL);
    }

    if ((soc == NULL) || ((soc->swIntrActive & (1 << uC)) == 0) ||
        (soc->uc_msg_active[uC] == NULL)) {
        return (SOC_E_NONE);
    }

   /* Block others for a bit */
    if (sal_mutex_take(soc->uc_msg_control, soc->uc_msg_ctl_timeout)) {
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "%s: semtake  - uc_msg_control timed out\n"), FUNCTION_NAME()));
        return SOC_E_INTERNAL;
    }
    
    /*Clear receive LL and set Semaphore count to 0*/
    while( soc->uc_msg_rcvd_ll[uC][msg_class].ll_count ) {
        msg_node = (mos_msg_ll_node_t *)ll_remove_head(&(soc->uc_msg_rcvd_ll[uC][msg_class]));
        if (msg_node != NULL) {
            sal_free(msg_node);
        }
        /*Decrement the counting semaphore count to be in sync with LL*/
        sal_sem_take(soc->uc_msg_rcv_sems[uC][msg_class], 10000000);
    };
    
    /* Kick the waiter */
    sal_sem_give( soc->uc_msg_rcv_sems[uC][msg_class] );

    sal_mutex_give(soc->uc_msg_control);
    return (SOC_E_NONE);
}

/* Function:
 *      soc_cmic_uc_msg_send_receive
 * Purpose:
 *      Send a message to another processor and wait for a reply
 * Parameters:
 *      unit - unit number
 *      uC - uC number
 *      send - the 64-bit message to send
 *      reply - the 64-bit reply
 *      timeout - time to wait for reply
 * Returns:
 *      SOC_E_NONE = message ACK'd (agent received msg)
 *      SOC_E_xxx - message NAK'd (no agent or buffer space full or timeout)
 *
 */
int soc_cmic_uc_msg_send_receive(int unit, int uC, mos_msg_data_t *send,
                                 mos_msg_data_t *reply, sal_usecs_t timeout) {
    int               rc;

    sal_usecs_t start = sal_time_usecs();
    sal_usecs_t now;
    
    rc = soc_cmic_uc_msg_send(unit, uC, send, timeout);
    now = sal_time_usecs();
    if (timeout <= now - start) {
        rc = SOC_E_TIMEOUT;
        rc = now - start;
    } else if (rc == SOC_E_NONE) {               /* If ack'd */
        timeout -= now - start;
        rc = soc_cmic_uc_msg_receive(unit, uC, send->s.mclass, reply, timeout);
    }

    return (rc);
}

/* Function:
 *      soc_cmic_uc_appl_init
 * Purpose:
 *      Try to initialze an application on a UC
 * Parameters:
 *      unit - unit number
 *      uC - uC number
 *      msg_class - application message class
 *      timeout - time to wait for reply
 *      version_info -  version of the SDK-based application.  The
 *                      contents of this are application dependent.
 *                      Common uses include passing a pointer to an
 *                      area to be DMA'd to the Uc, or passing a
 *                      simple version number. 
 *      min_appl_version - min version for the remote app 
 * Returns:
 *      SOC_E_NONE = message ACK'd (agent received msg)
 *      SOC_E_UNAVAIL = Cannot init this app on this uC
 *      SOC_E_CONFIG = Appl started but app is < min version
 *
 */
int soc_cmic_uc_appl_init(int unit, int uC, int msg_class,
                          sal_usecs_t timeout, uint32 version_info,
                          uint32 min_appl_version)
{
    mos_msg_data_t      send, rcv;
    int                 rc;

    if (!soc_feature(unit, soc_feature_cmicm)) {
        return (SOC_E_UNAVAIL);
    } 

    if (soc_cmic_uc_msg_active_wait(unit, uC) != SOC_E_NONE) {
        /* Down and not coming back up */
        return (SOC_E_UNAVAIL);
    } 

    send.s.mclass = MOS_MSG_CLASS_SYSTEM;
    send.s.subclass = MOS_MSG_SUBCLASS_SYSTEM_APPL_INIT;
    send.s.len = soc_htons(msg_class);
    send.s.data = soc_htonl(version_info);

    rc = soc_cmic_uc_msg_send(unit, uC, &send, timeout);
    if (rc != SOC_E_NONE) {
        return(SOC_E_FAIL);
    }

    rc = soc_cmic_uc_msg_receive(unit, uC, msg_class, &rcv, timeout);

    if (rc != SOC_E_NONE) {
        return (SOC_E_UNAVAIL);
    }
    if (rcv.s.len != 0) {
        return (SOC_E_UNAVAIL);
    }

    if (soc_ntohl(rcv.s.data) < min_appl_version) {
        return (SOC_E_CONFIG);
    } 

    return (SOC_E_NONE);
}

/* Routine to determine the base address of the BroadSync Register cache on KT2 */
uint32 soc_cmic_bs_reg_cache(int unit, int bs_num)
{
    uint32 msg_end = MSG_AREA_END(_soc_cmic_uc_msg_base(unit));
    return msg_end + bs_num * sizeof(mos_msg_bs_reg_cache_t);
}


/* Internal routine to determine the base address of the timestamp data area */
static uint32 soc_cmic_timestamp_base(int unit)
{
    uint32 msg_end = MSG_AREA_END(_soc_cmic_uc_msg_base(unit));
    if (SOC_IS_KATANA2(unit)) {
        /* KT2 has two BS register caches after the msg area, so skip over them */
        return msg_end + 2 * sizeof(mos_msg_bs_reg_cache_t);
    } else {
        return msg_end;
    }
}


static uint64 uc_mem_read_64(int unit, unsigned addr)
{
    uint32 hi = soc_uc_mem_read(unit, addr);
    uint32 lo = soc_uc_mem_read(unit, addr + 4);
    uint64 ret;

    COMPILER_64_SET(ret, hi,lo);
    return ret;
}

/*
 * Function:
 *      soc_cmic_uc_msg_timestamp_get
 * Purpose:
 *      Get most two most recent timestamps from a given source, and corresponding firmware time
 * Parameters:
 *      unit                  - (IN)  Unit number.
 *      event_id              - (IN)  Index of desired timestamp event
 *      ts_data               - (OUT) Timestamp data
 * Returns:
 *      SOC_E_PARAM    - event_id is out of bounds
 *      SOC_E_TIMEOUT  - unable to read consistent timestamp data before timeout
 */
int
soc_cmic_uc_msg_timestamp_get(
    int unit,
    int event_id,
    soc_cmic_uc_ts_data_t *ts_data)
{
    uint64 read_ts[2] = {COMPILER_64_INIT(0,0)};  /* event timestamps as read from shared memory */
    uint64 read_prev_ts[2] = {COMPILER_64_INIT(0,0)};  /* the preceeding ts of the same event */
    uint64 read_ts0[2] = {COMPILER_64_INIT(0,0)};  /* TS0 corresponding to PTP time */
    uint64 read_ts1[2] = {COMPILER_64_INIT(0,0)};  /* TS1 corresponding to PTP time */
    soc_cmic_uc_time_t read_full_time[2] = {{COMPILER_64_INIT(0,0)}};
    uint64 temp;
    int rv = SOC_E_NONE;
    uint32 timestamp_base = soc_cmic_timestamp_base(unit);
    const int max_iter = 100;
    int iter = 0;

    if (event_id >= MOS_MSG_MAX_TS_EVENTS) {
        return SOC_E_PARAM;
    }

    do {
        read_ts[1] = read_ts[0];
        read_prev_ts[1] = read_prev_ts[0];
        read_ts0[1] = read_ts0[0];
        read_ts1[1] = read_ts1[0];
        read_full_time[1] = read_full_time[0];

        /* Get from CMICM / IPROC shared memory */
        read_full_time[0].seconds = uc_mem_read_64(unit, timestamp_base + 4);
        read_full_time[0].nanoseconds = soc_uc_mem_read(unit, timestamp_base + 12);

        read_ts0[0] = uc_mem_read_64(unit, timestamp_base + 16);
        read_ts1[0] = uc_mem_read_64(unit, timestamp_base + 24);

        read_ts[0] = uc_mem_read_64(unit, timestamp_base + 32 + 8 * event_id);
        read_prev_ts[0] = uc_mem_read_64(unit, timestamp_base + 32 + 8 * MOS_MSG_MAX_TS_EVENTS + 8 * event_id);

        /* repeat until everything we care about comes out the same way twice */
        /* to ensure that we didn't race the firmware updating the same data */
    } while ((COMPILER_64_NE(read_full_time[0].seconds,  read_full_time[1].seconds) ||
              read_full_time[0].nanoseconds != read_full_time[1].nanoseconds ||
              COMPILER_64_NE(read_ts[0], read_ts[1]) ||
              COMPILER_64_NE(read_prev_ts[0], read_prev_ts[1]) ||
              COMPILER_64_NE(read_ts1[0], read_ts1[1]) ||
              COMPILER_64_NE(read_ts0[0], read_ts0[1])) &&
             ++iter < max_iter);

    if (iter == max_iter) {
        rv = SOC_E_TIMEOUT;
        /* don't exit- populate the fields with whatever we read, in case it is useful */
    }

    ts_data->hwts = read_ts[0];
    ts_data->prev_hwts = read_prev_ts[0];

    /* ts0/ts1/full timestamps are all for the same time.  So, find offset from event timestamp
       (made with timestamper0) from ts0, to compute event time in ts1 / full domains */

    ts_data->hwts_ts1 = read_ts1[0];  /* = ts1 + (event_ts0 - ts0) => event_ts1 */
    COMPILER_64_ADD_64(ts_data->hwts_ts1, read_ts[0]);
    COMPILER_64_SUB_64(ts_data->hwts_ts1, read_ts0[0]);

    /* Start with the time corresponding to ts0.  Adjusted below by (ts - ts0) */
    ts_data->time.seconds = read_full_time[0].seconds;
    ts_data->time.nanoseconds = read_full_time[0].nanoseconds;

    {
        int64 full_ns;
        uint64 one_billion = COMPILER_64_INIT(0,1000000000);
        uint64 abs_ts_minus_ts0_sec;
        uint64 abs_ts_minus_ts0_nsec;

        COMPILER_64_SET(full_ns, 0, ts_data->time.nanoseconds);
        /* Division / multiplication are unsigned, so use absolute value of difference */
        /* Calculate seconds/nanoseconds of abs(ts - ts0):
         *   abs_ts_minus_ts0_sec = (abs_ts_minus_ts0 / one_billion)
         *   abs_ts_minus_ts0_nsec = abs_ts_minus_ts0 - (one_billion * abs_ts_minus_ts0_sec)
         */

        if (COMPILER_64_LT(read_ts[0], read_ts0[0])) {
            abs_ts_minus_ts0_sec = read_ts0[0];
            COMPILER_64_SUB_64(abs_ts_minus_ts0_sec, read_ts[0]);
        } else {
            abs_ts_minus_ts0_sec = read_ts[0];
            COMPILER_64_SUB_64(abs_ts_minus_ts0_sec, read_ts0[0]);
        }
        abs_ts_minus_ts0_nsec = abs_ts_minus_ts0_sec;

        COMPILER_64_UDIV_64(abs_ts_minus_ts0_sec, one_billion);

        /* abs_ts_minus_ts0_nsec -= (abs_ts_minus_ts0_sec * 1e9) */
        temp = abs_ts_minus_ts0_sec;
        COMPILER_64_UMUL_32(temp, 1000000000);
        COMPILER_64_SUB_64(abs_ts_minus_ts0_nsec, temp);

        if (COMPILER_64_LT(read_ts[0], read_ts0[0])) {
            /* subtract abs_ts_minus_ts0 seconds/nanoseconds from full time */
            COMPILER_64_SUB_64(ts_data->time.seconds, abs_ts_minus_ts0_sec);
            if (COMPILER_64_LT(full_ns, abs_ts_minus_ts0_nsec)) {
                COMPILER_64_ADD_64(full_ns, one_billion);
                COMPILER_64_SUB_32(ts_data->time.seconds, 1);
            }
            COMPILER_64_SUB_64(full_ns, abs_ts_minus_ts0_nsec);
        } else {
            /* add abs_ts_minus_ts0 seconds/nanoseconds to full time */
            COMPILER_64_ADD_64(ts_data->time.seconds, abs_ts_minus_ts0_sec);
            COMPILER_64_ADD_64(full_ns, abs_ts_minus_ts0_nsec);

            if (COMPILER_64_GE(full_ns, one_billion)) {
                COMPILER_64_SUB_64(full_ns, one_billion);
                COMPILER_64_ADD_32(ts_data->time.seconds, 1);
            }
        }

        ts_data->time.nanoseconds = COMPILER_64_LO(full_ns);
    }

    return rv;
}


/*
 * Function:
 *      soc_cmic_uc_msg_timestamp_enable
 * Purpose:
 *      Enable a specified timestamp event
 * Parameters:
 *      unit                  - (IN)  Unit number.
 *      event_id              - (IN)  Index of desired timestamp event
 * Returns:
 *      SOC_E_PARAM    - event_id is out of bounds
 * Note:  CPU-initiated capture is one-shot.  All other events are simply enabled
 */
int
soc_cmic_uc_msg_timestamp_enable(
    int unit,
    int event_id)
{
    uint32 timestamp_base = soc_cmic_timestamp_base(unit);
    uint32 capture_reg;

    capture_reg = soc_uc_mem_read(unit, timestamp_base);

    if (soc_feature(unit, soc_feature_iproc)) {
        /* IPROC */
        if (event_id > 18) {
            return SOC_E_PARAM;
        }
        capture_reg |= (1 << event_id);
    } else {
        /* CMICM */
        if (event_id > 12) {
            return SOC_E_PARAM;
        }
        capture_reg |= (1 << (event_id + 16));
    }
    soc_uc_mem_write(unit, timestamp_base, capture_reg);

    return SOC_E_NONE;
}


/*
 * Function:
 *      soc_cmic_uc_msg_timestamp_disable
 * Purpose:
 *      Disable a specified timestamp event, unless used by firmware
 * Parameters:
 *      unit                  - (IN)  Unit number.
 *      event_id              - (IN)  Index of desired timestamp event
 * Returns:
 *      SOC_E_PARAM    - event_id is out of bounds
 */
int
soc_cmic_uc_msg_timestamp_disable(
    int unit,
    int event_id)
{
    uint32 timestamp_base = soc_cmic_timestamp_base(unit);
    uint32 capture_reg;

    capture_reg = soc_uc_mem_read(unit, timestamp_base);

    if (soc_feature(unit, soc_feature_iproc)) {
        /* IPROC */
        if (event_id > 18) {
            return SOC_E_PARAM;
        }
        capture_reg &= ~(1 << event_id);
    } else {
        /* CMICM */
        if (event_id > 12) {
            return SOC_E_PARAM;
        }
        capture_reg &= ~(1 << (event_id + 16));
    }

    soc_uc_mem_write(unit, timestamp_base, capture_reg);

    return SOC_E_NONE;
}
#endif /* CMICM or IPROC Support */
