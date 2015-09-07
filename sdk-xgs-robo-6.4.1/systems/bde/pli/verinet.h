/*
 * $Id: verinet.h,v 1.9 Broadcom SDK $
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
 * File: 	verinet.h
 * Purpose: 	SAL PLI simulation definitions (Unix PLI sim only)
 */

#ifndef _SAL_VERINET_H
#define _SAL_VERINET_H

#include <stdarg.h>
#ifdef VXWORKS
#include <sys/times.h>
#else
#include <sys/time.h>
#endif

#include <sal/types.h>
#include <sal/core/thread.h>
#include <sal/core/sync.h>

#if !defined(_SOCKLEN_T) && !defined(__socklen_t_defined)
typedef int socklen_t;
#endif

#define N_VERINET_DEVICES	16

/*
 * Register types for PLI transactions
 */

#define PCI_CONFIG		0xfeed1001
#define PCI_MEMORY		0xfeed1002
#define I2C_CONFIG		0xfeed1003
#define PLI_CONFIG		0xfeed1004
#define JTAG_CONFIG		0xfeed1005
#define DMA_MEMORY		0xfeed1006
#define SOC_INTERNAL		0xfeed1007
#define PCI_SLAVE_MEMORY	0xfeed1008
#define PHY_EXERNAL		0xfeed1009

typedef void (*pli_isr_t)(void *isr_data);

/*
 * Returned to PLI server code when a client connection is established.
 */

typedef struct netconnection_t {
    int intsock;
    int dmasock;
    int piosock;
    uint32 inetaddr; 
} netconnection_t;

/*
 * PLI RPC command type: Format of RPC command messages.
 * All RPC messages from the SAL/testclient to the server (veriserv)
 * are of the following format.
 */

#define MAX_RPC_ARGS   512 	/* DWORD ARG COUNT */

typedef struct rpc_cmd_t {
    uint32  opcode;
    /* RPC OPCODES : Requests */
#define   RPC_GETREG_REQ        0x101 
#define   RPC_SETREG_REQ        0x102
#define   RPC_IOCTL_REQ         0x103

    /* For PCI Interface */
#define   RPC_PCI_CONFIG_GET    0x104
#define   RPC_PCI_CONFIG_SET    0x105
#define   RPC_PCI_MEMORY_GET    0x106
#define   RPC_PCI_MEMORY_SET    0x107
#define   RPC_DMAC_WRITE_REQ    0x108
#define   RPC_DMAC_READ_REQ     0x109
#define   RPC_SEND_INTR         0x110
#define   RPC_PCI_CONDITION     0x111
#define   RPC_REGISTER_CLIENT   0x112

    /* For I2C Interface */
#define   RPC_I2C_CONDITION     0x113
#define   RPC_I2C_BYTE_READ     0x114
#define   RPC_I2C_BYTE_WRITE    0x115
#define   RPC_I2C_ASSERT_ACK    0x116
#define   RPC_I2C_CHECK_ACK     0x117
#define   RPC_I2C_ASSERT_START  0x118
#define   RPC_I2C_CHECK_START   0x119
#define   RPC_I2C_ASSERT_STOP   0x120
#define   RPC_I2C_CHECK_STOP    0x121

    /* For simulation data transfer */
#define   RPC_PACKET_DATA_RAW   0x122
    /* First arg is data length in bytes; Data is packed into args */
    /* Currently supports up to 2 K */
#define   RPC_PACKET_DESC       0x123
    /* Data is coded (described) in args */
#define   RPC_SIM_WAIT          0x124
    /* Simulation should wait the specified number of usecs in arg 0. */

    /* Call test routine with packet data */
#define   RPC_TEST_PKT          0x125
#define   RPC_DMAC_WRITE_BYTES_REQ   0x126
#define   RPC_DMAC_READ_BYTES_REQ    0x127
#define   RPC_SCHAN_REQ         0x128

#define   RPC_SHUTDOWN		0x3fd
#define   RPC_TX_ENABLE		0x3fe
#define   RPC_SEND_PACKET	0x3ff
#define   RPC_DISCONNECT	0x400
#define   RPC_TERMINATE		0x500

    /* RPC OPCODES: Responses */
#define   RPC_GETREG_RESP       0x201 
#define   RPC_SETREG_RESP       0x202
#define   RPC_IOCTL_RESP        0x203
#define   RPC_DMAC_READ_RESP    0x204
#define   RPC_DMAC_WRITE_RESP   0x205
#define   RPC_PACKET_RESP       0x206
#define   RPC_SIM_WAIT_RESP     0x207
#define   RPC_DMAC_WRITE_BYTES_RESP   0x208
#define   RPC_DMAC_READ_BYTES_RESP    0x209
#define   RPC_SCHAN_RESP        0x210

    uint32  argcount;
    uint32  args[MAX_RPC_ARGS];

    uint32  status;
    /* RPC STATUS CODES */
#define   RPC_OK		0x300
#define   RPC_OUTSTANDING	0x200
#define   RPC_FAIL		0x100
} rpc_cmd_t;

/* Indicate opcodes that should use rpc string packing */
#define RPC_OP_USE_STRING(op) \
   (((op) == RPC_DMAC_WRITE_BYTES_REQ)   || \
    ((op) == RPC_DMAC_READ_BYTES_REQ)    || \
    ((op) == RPC_DMAC_WRITE_BYTES_RESP)  || \
    ((op) == RPC_DMAC_READ_BYTES_RESP))

/* Connection information maintained by PLI client */

typedef struct verinet_t {
    int 		devNo;

    char 		pliHost[128];
    int 		pliPort;
    int 		intPort;
    int 		dmaPort;
    int 		skipTestInt;	/* skip test intr sent by plicode */
    int 		sockfd;		/* fd for socket conn. to simulator */
    int			jobsStarted;

    /*
     * pli_mutex is used by client routines so that multiple threads
     * may share the pli_setreg/pli_getreg calls.
     */
    sal_mutex_t 	pli_mutex;

    /*
     * DMAC
     */

    int 		dmacInited;
    sal_thread_t 	dmacListener;
    sal_thread_t 	dmacHandler;
    sal_sem_t 		dmacListening;
    volatile int 	dmacWorkerExit;
    int 		dmacFd;

    /*
     * INTR
     */

    int 		intrInited;
    sal_thread_t 	intrThread;
    sal_thread_t 	intrDispatchThread;
    sal_sem_t	 	intrDispatchSema;
    sal_sem_t	 	intrListening;
    int 		intrSkipTest;
    int 		intrFinished;
    sal_thread_t 	intrHandlerThread;
    volatile int 	intrWorkerExit;
    int			intrFd;

    pli_isr_t		ISR;
    void		*ISR_data;
} verinet_t;

extern verinet_t verinet[];

/*
 * Network utility routines.
 */

extern int writen(int fd, void* ptr, int nbytes);
extern int readn(int fd, void* ptr, int nbytes);
extern int get_command(int sockfd, struct timeval* tv, rpc_cmd_t* command);
extern int wait_command(int sockfd, rpc_cmd_t* cmd);
extern int poll_command(int sockfd, rpc_cmd_t* cmd);
extern int write_command(int sockfd, rpc_cmd_t* command);
extern int get_pkt_data(int sockfd, struct timeval* tv, uint32* data);
extern void make_rpc(rpc_cmd_t* r, uint32 opcode, uint32 status,
		     uint32 argcount, ...);
extern int getsockport(int sock_fd);

/*
 * RPC: requests.
 */

extern void make_rpc_setreg_req(rpc_cmd_t* c,
			 uint32 type, uint32 regnum, uint32 regval);
extern void make_rpc_getreg_req(rpc_cmd_t* c,uint32 type, uint32 regnum);
extern void make_rpc_disconnect_req(rpc_cmd_t* c);
extern void make_rpc_setmem_req(rpc_cmd_t* c, uint32 addr, uint32 data);
extern void make_rpc_getmem_req(rpc_cmd_t* c, uint32 addr);
extern void make_rpc_intr_req(rpc_cmd_t* c, uint32 cause);
extern void make_rpc_register_req(rpc_cmd_t* c,
			   int intPort, int dmacPort, uint32 netAddr);

/*
 * RPC: responses.
 */

extern void make_rpc_getreg_resp(rpc_cmd_t* c,
			  uint32 status, uint32 regval, char* buf32b);
extern void make_rpc_setreg_resp(rpc_cmd_t* c, uint32 regval);
extern void make_rpc_getmem_resp(rpc_cmd_t* c, uint32 data);
extern void make_rpc_setmem_resp(rpc_cmd_t* c, uint32 data);
extern void make_rpc_intr_resp(rpc_cmd_t* c, uint32 cause);
extern void make_rpc_register_resp(rpc_cmd_t* c, uint32 status);


/*
 * Server calls.
 */

extern void disconnect(int sockfd);
extern uint32 send_interrupt(int sockfd, uint32 cause);
extern netconnection_t* register_client(int s, uint32 i,uint32 d,
                                        uint32 netaddr);
extern int unregister_client(netconnection_t *);
extern int dma_writemem(int sockfd, uint32 addr, uint32 data);
extern int dma_readmem(int sockfd,uint32 addr, uint32 *value);
int dma_write_bytes(int sockfd, uint32 addr, uint8 *data, int len);
int dma_read_bytes(int sockfd, uint32 addr, uint8 *data, int len);

/*
 * Client calls.
 */

extern int pli_client_count(void);
extern int pli_client_attach(int devNo);
extern int pli_register_isr(int devNo, pli_isr_t isr, void *isr_data);
extern uint32 pli_getreg(int devNo, uint32 regtype, uint32 regnum);
extern uint32 pli_setreg(int devNo, uint32 regtype, uint32 regnum,
                         uint32 regval);
extern int pli_client_detach(int devNo);

/*
 * Support for genering fw packets with ncsim
 */

extern uint32 pli_send_packet(int devNo, uint32,uint32,int,unsigned char *);
extern uint32 pli_tx_enable(int devNo, uint32);
extern uint32 pli_shutdown(int devNo);

extern uint32 pli_sim_start(int devNo);

/*
 * DMAC: DMA Controller (emulator)
 * See: sal/unix/dmac.c
 */

extern int dmac_init(verinet_t *v);

/*
 * INTC: Interrupt Controller
 * See: sal/unix/intr.c
 */

extern int intr_init(verinet_t *v);
extern void intr_set_vector(verinet_t *v, pli_isr_t isr, void *isr_data);
extern void intr_handler(void *v_void);		/* verinet_t * */
extern int intr_int_context(void);
  
#endif	/* !_SAL_VERINET_H */
