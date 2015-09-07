/*
 * $Id: TkOsAlMsg.c 1.1 Broadcom SDK $
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
 * File:     TkOsAlMsg.c
 * Purpose: 
 *
 */

#include <soc/ea/tk371x/TkTypes.h>
#include <soc/ea/tk371x/TkMsg.h>
#include <soc/ea/tk371x/TkOsAlloc.h>
#include <soc/ea/tk371x/TkOsCm.h>
#include <soc/ea/tk371x/TkOsSync.h>
#include <soc/ea/tk371x/TkDebug.h>

/*
 * Function:
 *      sal_msg_create
 * Purpose:
 *      create an interal message queue.
 * Parameters:
 *      name      - The message name 
 *      count     - Number of the message the message queue have
 * Returns:
 *      NULL or the pointer
 * Notes:
 */
sal_msg_desc_t *
sal_msg_create(char *name, unsigned int count)
{
    sal_msg_desc_t *pmsg;
    char            s[255] = { 0x0 };
    pmsg = (sal_msg_desc_t *)
        sal_alloc(sizeof(sal_msg_desc_t), name);

    if (NULL == pmsg) {
        TkDbgTrace(TkDbgErrorEnable);
        return NULL;
    }

    sal_sprintf(s, "%s.%s", name, "msg");

    pmsg->msg = sal_alloc(sizeof(sal_msg_buffer_t) * count, s);
    if (NULL == pmsg->msg) {
        sal_free(pmsg);
        TkDbgTrace(TkDbgErrorEnable);
        return NULL;
    }

    pmsg->sem = sal_sem_create(name, /*sal_sem_BINARY*/sal_sem_COUNTING, 0);

    pmsg->id = -1;
    pmsg->msg_cnt = 0;
    pmsg->max_cnt = count;

    sal_memset((void *) pmsg->msg, 0x0, sizeof(sal_msg_buffer_t) * count);

    return pmsg;
}

/*
 * Function:
 *      sal_msg_destory
 * Purpose:
 *      Destroy a interal message queue by the message queue pointer
 * Parameters:
 *      msg    - The interal message pointer which will be destory 
 * Returns:
 *      None
 * Notes:
 */
void
sal_msg_destory(sal_msg_desc_t * msg)
{
    if (NULL == msg) {
        TkDbgTrace(TkDbgErrorEnable);
        return;
    }

    sal_free(msg->msg);

    sal_free((void *) msg);

    return;
}

/*
 * Function:
 *      sal_msg_snd
 * Purpose:
 *      Send a information into the message queue
 * Parameters:
 *      msg    - The message queue pointer which the information will be put into.
 *      buff    - The data whic will be put into the message queue
 *      len    - The data len 
 *      ms    - Ms will pend when the send the message, while -1 means return imediatly.
 * Returns:
 *      ERROR or OK
 * Notes:
 */
int
sal_msg_snd(sal_msg_desc_t * msg, char *buff, int len, int ms)
{
    if (NULL == msg || NULL == buff) {
        return -1;
    }

    if (msg->max_cnt == msg->msg_cnt) {
        TkDbgInfoTrace(TkDbgErrorEnable,("Message is full.\n"));
        return -1;
    }

    sal_memcpy(msg->msg[(msg->id +
                         1) % msg->max_cnt].buff, buff, len % SAL_MSG_LEN);

    msg->msg[(msg->id + 1) % msg->max_cnt].len =
        (len < SAL_MSG_LEN) ? len : SAL_MSG_LEN;

    msg->id = (msg->id + msg->max_cnt + 1) % msg->max_cnt;

    msg->msg_cnt++;

    sal_sem_give(msg->sem);

    return 0;
}

/*
 * Function:
 *      sal_msg_rcv
 * Purpose:
 *      Receive a message from a interal message queue
 * Parameters:
 *      msg    - From which message queue 
 *      buff    - The data received will be dropped to 
 *      len   - The buffer length
 *      ms  - Ms of pending. -1 means wait for ever.
 * Returns:
 *      ERROR or the data length of received
 * Notes:
 */
int
sal_msg_rcv(sal_msg_desc_t * msg, char *buff, int len, int ms)
{
    int             valid_len;

    if (NULL == msg || NULL == buff) {
        TkDbgTrace(TkDbgErrorEnable);
        return -1;
    }
    
    if (sal_sem_take(msg->sem, ms)) {
        TkDbgTrace(TkDbgErrorEnable);
        return -1;
    }

    valid_len =
        (msg->msg[msg->id].len < len) ? msg->msg[msg->id].len : len;
    sal_memcpy(buff, msg->msg[msg->id].buff, valid_len);

    msg->msg_cnt--;
    if(msg->msg_cnt == 0){
        msg->id = -1;
    }else{
        msg->id = (msg->id + msg->max_cnt + 1) % msg->max_cnt;
    }
    
    return valid_len;

}

/*
 * Function:
 *      sal_msg_clear
 * Purpose:
 *      Clear message in the interal message queue
 * Parameters:
 *      msg    - Targer message queue
 * Returns:
 *      ERROR or 0
 * Notes:
 */
int
sal_msg_clear(sal_msg_desc_t * msg)
{
    if (NULL == msg) {
        TkDbgTrace(TkDbgErrorEnable);
        return -1;
    }

    while (!sal_sem_take(msg->sem, 1));

    msg->id = -1;
    msg->msg_cnt = 0;
    
    return 0;
}

