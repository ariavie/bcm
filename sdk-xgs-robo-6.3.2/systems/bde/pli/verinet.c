/*
 * $Id: verinet.c 1.24 Broadcom SDK $
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
 * Routines for accessing the network and sending RPC commands to/from
 * the PLI test-server.
 */

#include <unistd.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <stdlib.h>
#include <netinet/in.h>
#ifndef VXWORKS
#include <string.h>
#include <netdb.h>
#endif
#include <arpa/inet.h>
#include <errno.h>
#include <ctype.h>

#ifdef VXWORKS
#include <vxWorks.h>
#include <sockLib.h>
#endif

#include <sal/core/sync.h>
#include <sal/appl/io.h>
#include <soc/debug.h>	

#include "verinet.h"

#include <sim/pcid/pcid.h>
#define _BCM_RPC_ERROR_COUNT_MAX (10)
/*
 * One verinet structure per devNo (unit)
 */
verinet_t verinet[N_VERINET_DEVICES];

/*
 *  Write "n" bytes to a descriptor.
 */
int writen(int fd, void *ptr, int nbytes)
{
    int nleft, nwritten;
    nleft = nbytes;
    while (nleft > 0){
	nwritten = write(fd, ptr, nleft);
	if (nwritten <= 0)
	    return(nwritten);               
	nleft -= nwritten;
	ptr = (char *)ptr + nwritten;
    }
    return(nbytes - nleft);
}


/*
 *  Read "n" bytes from a descriptor.
 */
int readn(int fd, void* ptr, int nbytes)
{
    int nleft, nread;
    nleft = nbytes;
    while (nleft > 0) {
	nread = read(fd, ptr, nleft);
	if (nread < 0)
	    return(nread);          	/* error, return < 0 */
	else if (nread == 0)
	    break;                  	/* EOF */
	nleft -= nread;
	ptr = (char *)ptr + nread;
    }
    return(nbytes - nleft);         	/* return >= 0 */
}

/*
 * Get the port number to which a socket is bound.
 */

int getsockport(int sock_fd)
{
    struct sockaddr_in name;
    socklen_t name_len;

    name_len = sizeof (name);

    if (getsockname(sock_fd, (struct sockaddr *)&name, &name_len) < 0) {
	perror("getsockname");
	return 0;
    }

    return ntohs(name.sin_port);
}

/*
 * reads an RPC command.
 * Parameters:
 *  sockfd -socket file descriptor.
 * Returns:
 *	-1 on error (nothing returned in rpc_cmd_t *)
 *	 0 if no command is found (nothing returned in rpc_cmd_t *)
 *	 1 if command is found (returned in rpc_cmd_t *)
 */

int get_command(int sockfd, struct timeval* tv, rpc_cmd_t* command)
{
    int i;
    fd_set read_vect;
    uint32 swap;

    debugk(DK_VERINET+DK_VERBOSE, "get_command: sockfd=%d\n", sockfd);

    assert(sockfd > 2);		/* Verinet initialized? */

    /* Setup bitmap for read notification ... */
    FD_ZERO(&read_vect);
    FD_SET(sockfd, &read_vect);

    while (1) {
        if(select(sockfd+1,&read_vect,(fd_set*)0x0,(fd_set*)0x0,tv)<0){
            if (errno == EINTR) {
                /* 
                 * Interrupted by a signal such as a GPROF alarm so
                 * restart the call
                 */
                continue;
            }
            perror("get_command: select error");
            goto fail;
        } else {
            break;
        }
    }
    /*
     * Data ready to be read.
     */
    if( FD_ISSET(sockfd, &read_vect)){

	/*
	 * Read RPC opcode, argcount, status
	 */
#define SOCKET_GET_U32(sockfd, swap, where, what) \
    do { \
	if(readn(sockfd, &(swap), sizeof(uint32)) < sizeof(uint32)) { \
	    printk("get_command: could not read RPC %s\n", what); \
	    goto fail; \
	} \
	(where) = ntohl(swap);  /* Convert to host order */ \
    } while (0)

        SOCKET_GET_U32(sockfd, swap, command->opcode, "opcode");
        SOCKET_GET_U32(sockfd, swap, command->argcount, "arg count");

        if (RPC_OP_USE_STRING(command->opcode)) {
            if (command->argcount > (sizeof(uint32) * MAX_RPC_ARGS)) {
                printk("RPC string too long (get): %d\n", command->argcount);
                goto fail;
            }
            SOCKET_GET_U32(sockfd, swap, command->args[0], "pkt address");

            if (readn(sockfd, &command->args[1], command->argcount) <
                command->argcount) {
                printk("get_command: could not read all of string. %d.\n",
                       command->argcount);
                goto fail;
            }
        } else {
            if (command->argcount > MAX_RPC_ARGS) {
                printk("get_command: bogus argcount (%d) from rpc socket\n",
                       command->argcount);
                goto fail;
            }
            /* Read each argument into command struct ... */
            for(i = 0; i < command->argcount; i++){
                SOCKET_GET_U32(sockfd, swap, command->args[i], "argument");
            }
        }

        SOCKET_GET_U32(sockfd, swap, command->status, "status");

#undef SOCKET_GET_U32

	debugk(DK_VERINET,
	       "get_command: opcode=0x%x nargs=%d status=%d\n",
	       command->opcode, command->argcount, command->status);

	return 1;
    }

    return 0;	/* Time expire with no command */

 fail:
    return -1;
}


/*
 * Non-blocking version of actual call made by PLI interface.
 * Note that the timeout is 0 which guarantees that the Verilog
 * model does not block.
 *
 * Returns:
 *	-1 on error (nothing returned in rpc_cmd_t *)
 *	 0 if no command is found (nothing returned in rpc_cmd_t *)
 *	 1 if command is found (returned in rpc_cmd_t *)
 */
int poll_command(int sockfd, rpc_cmd_t* command)
{
    struct timeval tv = {0,0};
    return get_command(sockfd,(struct timeval*)&tv,command);
}

/*
 * Blocking version of actual call made by PLI interface.
 *
 * Returns:
 *	-1 on error (nothing returned in rpc_cmd_t *)
 *	 0 on success
 */
int wait_command(int sockfd, rpc_cmd_t* command)
{
    return get_command(sockfd,(struct timeval*)0,command);
}


/*
 * Writes an RPC command to the server.
 * Parameters:
 * 1) sockfd-socket file descriptor.
 * 2) command -rpc_cmd_t filled in for the necessary command.
 *
 * Return values:
 *  RPC_OK on success
 *  RPC_FAIL on error
 */
int write_command(int sockfd, rpc_cmd_t* command)
{
    int i;
    uint8 write_buf[sizeof(uint32) * (MAX_RPC_ARGS + 4)];
    int write_cnt;
    uint32 swap;
    static int error_count = 0;

    debugk(DK_VERINET,
	   "write_command: sockfd=%d opcode=0x%x nargs=%d status=%d\n",
	   sockfd, command->opcode, command->argcount, command->status);

    assert(sockfd > 2);	/* Verinet initialized? */

    /*
     * Each value is converted to network byte order before writing.
     * On some hosts (e.g. SPARC,MIPS, COLDFire which run in big-endian
     * mode, these calls are NOPS, but we include them here for
     * correctness and porting to other platforms).
     *
     * Data is saved in a buffer and written using one write to make
     * performance over the network about a ZILLION times faster.
     */

    write_cnt = 0;

    /* RPC opcode */
#define STORE_U32(val, buf, swap, idx) \
    do { \
        swap = htonl(val); \
        sal_memcpy(&(buf[idx]), &(swap), sizeof(uint32)); \
        idx += sizeof(uint32); \
    } while(0)

    /* Store opcode and argcount */
    STORE_U32(command->opcode, write_buf, swap, write_cnt);
    STORE_U32(command->argcount, write_buf, swap, write_cnt);

    /* Each argument from command struct, unless as string */

    if (RPC_OP_USE_STRING(command->opcode)) {
        /* Arg 0 is address */
        STORE_U32(command->args[0], write_buf, swap, write_cnt);
        /* Here, arg count is a number of bytes */
        if ((command->argcount + write_cnt) > (sizeof(uint32) * MAX_RPC_ARGS)) {
            printk("RPC string too long (send0: %d\n", command->argcount);
            goto fail;
        }
        sal_memcpy(&write_buf[write_cnt], (uint8 *) &command->args[1],
               command->argcount);
        write_cnt += command->argcount;
    } else {
        if (command->argcount > MAX_RPC_ARGS) {
            printk("RPC: too many arguments. %d\n", command->argcount);
            goto fail;
        }
        for(i = 0; i < command->argcount; i++) {
            STORE_U32(command->args[i], write_buf, swap, write_cnt);
        }
    }

    /* Store status */
    STORE_U32(command->status, write_buf, swap, write_cnt);

#undef STORE_U32

    /* Perform write */
    if (writen(sockfd, write_buf, write_cnt) < 0) {
	printk("write_command: could not write RPC request.\n");
	goto fail;
    }

    goto done;

    /* Failure */
 fail:
    error_count ++;
    if (error_count > _BCM_RPC_ERROR_COUNT_MAX) {
	    printk("write_command: Too many write failures.\n");
        assert (0);
    } 
    return RPC_FAIL;

    /* Success */
 done:
    return RPC_OK;
}

/*
 * Creates an RPC command.
 * Generic routine used by all RPC command stubs below.
 * NOTE: this should be used the same way by all stubs since the
 * ordering of arguments is significant.
 */

void make_rpc(rpc_cmd_t* command,
	      uint32 opcode,
	      uint32 status,
	      uint32 argcount,
	      ...)
{
    int i;
    va_list ap;
    command->opcode = opcode;
    command->argcount = argcount;
    command->status = status;
    va_start(ap, argcount);
    for( i = 0; i < argcount; i++)
	command->args[i] = va_arg(ap, uint32);
    va_end(ap);
}


/*
 * Creates an RPC command with data as a string.
 */

void make_rpc_string(rpc_cmd_t* command,
                     uint32 opcode,
                     uint32 status,
                     uint32 addr,
                     uint8 *data,
                     uint32 len)
{
    command->opcode = opcode;
    command->argcount = len;
    command->status = status;
    command->args[0] = addr;
    if (data) {
        sal_memcpy((uint8 *)&command->args[1], data, len);
    }
}

/*
 * Makes an RPC structure to be sent to the server for the client (request);
 * This is for the setmem(addr,data) call.
 */
void make_rpc_setmem_req(rpc_cmd_t* cmd, uint32 addr, uint32 data)
{
    make_rpc(cmd,
	     RPC_DMAC_WRITE_REQ,
	     RPC_OUTSTANDING,
	     2,
	     addr,
	     data);
}
/*
 * Makes an RPC structure to be sent back to the client in response.
 * This is for the response to the client for setmem(addr,data).
 * Here we send the updated value of the data @ addr.
 */
void make_rpc_setmem_resp(rpc_cmd_t* cmd, uint32 data)
{
    make_rpc(cmd,
	     RPC_DMAC_WRITE_RESP,
	     RPC_OK,
	     1,
	     data);
}


/*
 * Makes an RPC structure to be sent to the server for the client (request);
 * this is for the getmem(addr) call.
 */
void make_rpc_getmem_req(rpc_cmd_t* cmd, uint32 addr)
{
    make_rpc(cmd,
	     RPC_DMAC_READ_REQ,
	     RPC_OUTSTANDING,
	     1,
	     addr);
}

/*
 * Makes an RPC response to send back to the client. This is for
 * a response to getmem(addr).
 */
void make_rpc_getmem_resp(rpc_cmd_t* cmd, uint32 data)
{
    make_rpc(cmd,
	     RPC_DMAC_READ_RESP,
	     RPC_OK,
	     1,
	     data);
}



/*
 * Makes an RPC structure to be sent to the server for the client (request);
 * This is for the pli_setreg() call.
 */
void make_rpc_setreg_req(rpc_cmd_t* cmd,
			 uint32 regtype, uint32 regnum, uint32 regval)
{
    make_rpc(cmd,
	     RPC_SETREG_REQ,
	     RPC_OUTSTANDING,
	     3,
	     regnum,
	     regval,
	     regtype);
}

/*
 * Makes an RPC structure to be sent to the client from the server
 * (response); this is for the response to the pli_setreg() call.
 */

void make_rpc_setreg_resp(rpc_cmd_t* cmd,
			  uint32 regval)
{
    make_rpc(cmd,
	     RPC_SETREG_RESP,
	     RPC_OK,
	     1,
	     regval);
}


/*
 * Makes an RPC structure to be sent to the server for the client (request);
 */
void make_rpc_ioctl_req(rpc_cmd_t* cmd, uint32 op, uint32 p1, uint32 p2)
{
}


/*
 * Makes an RPC structure to be sent to the server for the client (request);
 * this is for the pli_getreg() call.
 */
void make_rpc_getreg_req(rpc_cmd_t* cmd,
			 uint32 regtype, uint32 regnum)
{
    make_rpc(cmd,
	     RPC_GETREG_REQ,
	     RPC_OUTSTANDING,
	     2,
	     regnum,
	     regtype);
}


/*
 * Makes an RPC structure to be sent to the client from the server
 * (response); this is for the response to the pli_getreg() call.
 * NOTE: buf must be 32 byte in size.
 */
void make_rpc_getreg_resp(rpc_cmd_t* cmd,
			  uint32 status,
			  uint32 regval,
			  char* buf32byte)
{
    uint32	w[8];

    /* Kludge: shove a 32 byte buffer into some integers.*/

    memcpy((char *) w, buf32byte, 32);	/* Get aligned on word boundary */

    make_rpc(cmd,
	     RPC_GETREG_RESP,
	     RPC_OK,
	     10,
	     status,
	     regval,
	     w[0], w[1], w[2], w[3], w[4], w[5], w[6], w[7]);
}



/*
 * Makes an RPC structure to be sent to the server for the client (request);
 * this is for the pli_interrupt() call.
 */
void make_rpc_intr_req(rpc_cmd_t* cmd,
		       uint32 cause)
{
    make_rpc(cmd,
	     RPC_SEND_INTR,
	     RPC_OUTSTANDING,
	     1,
	     cause);
}

/*
 * Makes an RPC structure to be sent to the server for the client (request);
 * this is for the response to the pli_interrupt() call.
 */
void make_rpc_intr_resp(rpc_cmd_t* cmd,
			uint32 cause)
{
    make_rpc(cmd,
	     RPC_SEND_INTR,
	     RPC_OK,
	     1,
	     cause);
}


/*
 * Makes an RPC disconnect request structure.
 */
void make_rpc_disconnect_req(rpc_cmd_t* cmd)
{
    make_rpc(cmd,
	     RPC_DISCONNECT,
	     RPC_OUTSTANDING,
	     0,0,0);
}

/*
 * Makes an RPC registration request. This call will register the
 * client to receive interrupts on one port, and dma requests (r/w)
 * on another.
 */

void make_rpc_register_req(rpc_cmd_t* cmd,
			   int intPort,
			   int dmacPort,
			   uint32 netAddr)
{
    make_rpc(cmd,
	     RPC_REGISTER_CLIENT,
	     RPC_OUTSTANDING,
	     3,
	     intPort,
	     dmacPort,
	     netAddr);
}

/*
 * RPC registration response: send RPC_OK on success.
 */
void make_rpc_register_resp(rpc_cmd_t* cmd,
			    uint32 status)
{
    make_rpc(cmd,
	     RPC_REGISTER_CLIENT,
	     RPC_OK,
	     1,
	     status);
}


int pli_client_count(void)
{
    char *s;

    /* Returns number of PLI clients expected to be running. */
    /* This should be set in the environment variable SOC_TARGET_COUNT. */

    s = getenv("SOC_TARGET_COUNT");

    return (s == NULL) ? 1 : atoi(s);
}

/*
 * Generic network routines for client and server.
 */

/*
 * Connect a client to the server.
 * This performs the following PLI client-side initialization.
 *
 * 1) Initialize a lock for PLI setreg/getreg accesses
 * 2) Input connection host/port information from user.
 * 3) Setup DMA controller task to listen (locally) on dmaPort.
 * 4) Setup Interrupt Controller task to listen (locally) on intPort.
 * 5) Register an interrupt callback routine (passed in).
 * 6) Setup a local socket file descriptor to communicate with
 *    pliHost on the given pliPort.
 * 7) Register the client with the server on pliHost.
 *    The server may immediately start sending DMA and INTR requests.
 * Return:
 *    0 on success, a negative value on error.
 */
int pli_client_attach(int devNo)
{
    verinet_t *v = &verinet[devNo];
    struct sockaddr_in cli_addr;
    socklen_t cli_addr_size;
    struct sockaddr_in srv_addr;
#ifndef VXWORKS
    struct hostent *hostentPtr = NULL;
#endif
    rpc_cmd_t cmd;
    char tmp[80], *s;
    int rv = -1;

    memset(v, 0, sizeof (*v));

    v->devNo = devNo;

    v->sockfd = -1;
    v->skipTestInt = 1;

    v->pli_mutex = sal_mutex_create("PLI socket mutex");

    /* Setup target host */
    s = getenv("SOC_TARGET_SERVER");
    if (!s) {
	sprintf(tmp, "SOC_TARGET_SERVER%d", devNo);
	s = getenv(tmp);
    }
    if(!s) {
	sprintf(tmp,
		"Enter unit %d PLI server host"
#ifdef VXWORKS
		" address"
#else
		"name [localhost]"
#endif
		": ",
		devNo);
	sal_readline(tmp, v->pliHost, sizeof (v->pliHost), 0);
        if (sal_strlen(v->pliHost) == 0) {
            sal_strcpy(v->pliHost, "localhost");
        }
    } else {
	if(strlen(s) >= sizeof(v->pliHost))
           strncpy(v->pliHost, s, sizeof(v->pliHost) - 1);
        else
           strncpy(v->pliHost, s, strlen(s));
    }    
    /* Setup target port */
    s = getenv("SOC_TARGET_PORT");
    if (!s) {
	sprintf(tmp, "SOC_TARGET_PORT%d", devNo);
	s = getenv(tmp);
    }
    if(!s){
	sprintf(tmp, "Enter unit %d remote port: ", devNo);
	sal_readline(tmp, tmp, sizeof (tmp), "2400");
	v->pliPort = atoi(tmp);
    }
    else
	v->pliPort = atoi(s);

    /*
     * Connect to host running Vera (ncsim)
     */
    memset((void *)&srv_addr, 0, sizeof(srv_addr));

#ifdef VXWORKS
    if (!isdigit((unsigned) v->pliHost[0])) {
	printk("pli_client_attach: can't resolve host names in vxworks\n");
    }

    srv_addr.sin_addr.s_addr = inet_addr(v->pliHost);
#else
    hostentPtr = gethostbyname(v->pliHost);
    if(hostentPtr == NULL) {
	printk("pli_client_attach: hostname lookup failed "
	       "for host [%s].\n", v->pliHost);
	perror("gethostbyname");
	goto error;
    }
    memcpy(&srv_addr.sin_addr,hostentPtr->h_addr,4);
#endif

    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(v->pliPort);

    if ((v->sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	perror("server: can't open stream socket");
	goto error;
    }

#ifdef VERINET_CUSTOM_SOCKET_HOOK 
       VERINET_CUSTOM_SOCKET_HOOK 
#endif 

    if (connect(v->sockfd,
		(struct sockaddr*)&srv_addr, sizeof(srv_addr)) < 0) {
	printk("pli_client_attach: could not connect unit %d to [%s.%d] "
	       "-service unavailable.\n",
	       devNo, v->pliHost, v->pliPort);
	perror("connect");
	goto error;
    }

    /*
     * Get local host name to send back to server
     */

    cli_addr_size = sizeof (cli_addr);
    getsockname(v->sockfd, (struct sockaddr *)&cli_addr, &cli_addr_size);
    cli_addr.sin_addr.s_addr = htonl(cli_addr.sin_addr.s_addr);

    /*
     * Start INTC and DMAC tasks and register callback for ISR.
     */

    if (!v->jobsStarted) {
	if (dmac_init(v) < 0) {
	    goto error;
	}

	if (intr_init(v) < 0) {
	    goto error;
	}

	v->jobsStarted = 1;
    }

    /*
     * Send REGISTER request to PLI server.
     */
    make_rpc_register_req(&cmd,
			  v->intPort, v->dmaPort,
			  cli_addr.sin_addr.s_addr);

    if (write_command(v->sockfd, &cmd) != RPC_OK) {
	printk("pli_client_attach: could not send REGISTER request\n");
	goto error;
    }

    if (wait_command(v->sockfd, &cmd) < 0)
	goto error;

    if (cmd.opcode != RPC_REGISTER_CLIENT ||
	cmd.status != RPC_OK) {
	printk("pli_client_attach: PLI registration failed\n");
	goto error;
    }

    return 0;

 error:
    if (v->sockfd >= 0) {
	close(v->sockfd);
	v->sockfd = -1;
    }

    return rv;
}

int pli_register_isr(int devNo, pli_isr_t isr, void *isr_data)
{
    intr_set_vector(&verinet[devNo], isr, isr_data);
    return 0;
}

int pli_client_detach(int devNo)
{
    verinet_t *v = &verinet[devNo];

    if (v->sockfd >= 0) {
	disconnect(v->sockfd);
	close(v->sockfd);
	v->sockfd = -1;
    }

    return 0;
}

/*
 * Disconnect a client from the server.
 */
void disconnect(int sockfd)
{
    rpc_cmd_t command;
    make_rpc_disconnect_req(&command);
    printk("Disconnect: 0x%x\n", sockfd);
    write_command(sockfd, &command);
}


/*
 * Send an interrupt to a client. "cause" indicates the reason.
 */
uint32 send_interrupt(int sockfd, uint32 cause)
{
    rpc_cmd_t command;
    make_rpc_intr_req(&command,cause);
    if (write_command(sockfd, &command) != RPC_OK) {
        return -1;
    }
    if (wait_command(sockfd, &command) < 0)
	return -1;
    return command.args[0];
}




/*
 * Register a client with the server. This will add the client to
 * the servers queue of service requests. We pass the network address
 * and two ports which are used for DMA r/w requests and interrupt
 * requests.
 */
netconnection_t* register_client(int sockfd,
				 uint32 intPort,
				 uint32 dmaPort,
				 uint32 netAddr)
{
    int dmaSockFD = -1; /* DMA socket file descriptor */
    int intSockFD = -1; /* Interrupt file descriptor */
    netconnection_t *nc;
    struct sockaddr_in cli_addr; /* Client net address */
    
    memset(&cli_addr , 0 , sizeof(struct sockaddr_in)); 

    if((nc = (netconnection_t*)malloc(sizeof(netconnection_t))) == NULL)
	return (netconnection_t*)0x0;
    
    nc->intsock = -1;
    nc->dmasock = -1;
    nc->piosock = -1;

#if 0
    printk("register_client:s=%d irqPort=%d dmaPort=%d netAddr=%d.%d.%d.%d\n",
	   sockfd, intPort, dmaPort,
	   (netAddr & 0xff000000) >> 24,
	   (netAddr & 0x00ff0000) >> 16,
	   (netAddr & 0x0000ff00) >> 8,
	   (netAddr & 0x000000ff));
#endif

    /* Keep local copy of netaddress in native byte order */
    memcpy(&nc->inetaddr,&netAddr,4);
    /* Setup netaddr for client */
    netAddr = htonl(netAddr);
    memcpy(&cli_addr.sin_addr,&netAddr,4);

    cli_addr.sin_family = AF_INET;
    cli_addr.sin_port = htons(intPort);

    /* Setup interrupt socket */
    if((intSockFD = socket(AF_INET, SOCK_STREAM, 0)) < 0){
	perror("server: can't open stream socket");
	free(nc);
	return (netconnection_t*)0x0;
    }
    /* Connect back to client */
    if(connect(intSockFD, (struct sockaddr*)&cli_addr, sizeof(cli_addr))< 0){
	printk("Error: INTC connect failed to [0x%x.%d] -service unavailable.\n",
	       netAddr, intPort);
	perror("connect");
	close(intSockFD);
	free(nc);
	return (netconnection_t*)0x0;
    }

    /* Setup DMA socket */
    cli_addr.sin_port = htons(dmaPort);

    if((dmaSockFD = socket(AF_INET, SOCK_STREAM, 0)) < 0){
	perror("server: can't open stream socket");
	close(intSockFD);
	free(nc);
	return (netconnection_t*)0x0;
    }
  
    /* Connect back to client */
    if(connect(dmaSockFD, (struct sockaddr*)&cli_addr, sizeof(cli_addr))< 0){
	printk("Error: DMAC connect failed to [0x%x.%d] -service unavailable.\n",
	       netAddr, dmaPort);
	perror("connect");
	close(intSockFD);
	close(dmaSockFD);
	free(nc);
	return (netconnection_t*)0x0;
    }

    /* Fill in fields */
    nc->intsock = intSockFD ;
    nc->dmasock = dmaSockFD ;
    nc->piosock = sockfd;

    return nc;
}

int unregister_client(netconnection_t *nc)
{
    close(nc->intsock);
    close(nc->dmasock);
    close(nc->piosock);
    free(nc);
    return 0;
}

/*
 * Server PLI-level call to write to DMA memory.
 * Load the given memory address with some data on the client.
 */
int dma_writemem(int sockfd, uint32 addr, uint32 data)
{
    rpc_cmd_t cmd;
    make_rpc_setmem_req(&cmd,addr,data);
    if (write_command(sockfd, &cmd) != RPC_OK) {
        return RPC_FAIL;
    }
    if (wait_command(sockfd,&cmd) < 0) {
        return RPC_FAIL;
    }
    return RPC_OK;
}


/*
 * Server PLI-level client call to read DMA memory.
 * Returns the data at a given memory address on client.
 */
int dma_readmem(int sockfd, uint32 addr, uint32 *value)
{
    rpc_cmd_t cmd;
    make_rpc_getmem_req(&cmd,addr);
    if (write_command(sockfd, &cmd) != RPC_OK) {
        return RPC_FAIL;
    }
    if (wait_command(sockfd,&cmd) < 0) {
        return RPC_FAIL;
    }
    *value = cmd.args[0];
    return RPC_OK;
}

/*
 * Server PLI-level call to write to DMA memory as a string.
 */
int dma_write_bytes(int sockfd, uint32 addr, uint8 *data, int len)
{
    rpc_cmd_t cmd;

    make_rpc_string(&cmd, RPC_DMAC_WRITE_BYTES_REQ, RPC_OUTSTANDING,
                    addr, data, len);
    if (write_command(sockfd, &cmd) != RPC_OK) {
        return -1;
    }
    if (wait_command(sockfd, &cmd) < 0) {
        return -1;
    }

    return cmd.argcount;
}


/*
 * Server PLI-level client call to read DMA memory as a string.
 * Data is copied up to the max number of bytes to *data.
 */
int dma_read_bytes(int sockfd, uint32 addr, uint8 *data, int len)
{
    rpc_cmd_t cmd;
    
    make_rpc_string(&cmd, RPC_DMAC_READ_BYTES_REQ, RPC_OUTSTANDING,
                    addr, NULL, len);
    if (write_command(sockfd, &cmd) != RPC_OK) {
        return -1;
    }
    if (wait_command(sockfd, &cmd) < 0) {
        return -1;
    }

    memcpy(data, &cmd.args[1], cmd.argcount);

    return cmd.argcount;
}


/*
 * Client Call: get a register on the simulator.
 */
uint32 pli_getreg(int devNo, uint32 regtype, uint32 regnum)
{
    int i ;
    verinet_t *v = &verinet[devNo];
    uint32 status = 0x0;
    uint32 regval = 0x0;
    char buf[32], *ptr;
    rpc_cmd_t command;

    debugk(DK_VERINET,
	   "pli_getreg: regtype=0x%x regnum=0x%x\n", regtype, regnum);

    make_rpc_getreg_req(&command, regtype,regnum);

    sal_mutex_take(v->pli_mutex, sal_mutex_FOREVER);

    if(write_command(v->sockfd, &command) != RPC_OK) {
	printk("Error: pli_getreg write_command failed\n");
	sal_mutex_give(v->pli_mutex);
	return -1;
    }

    if (wait_command(v->sockfd, &command) < 0) {
	sal_mutex_give(v->pli_mutex);
	return -1;
    }

    sal_mutex_give(v->pli_mutex);

    status = command.args[0];
    regval = command.args[1];
    ptr = (char*)&command.args[2];
    for(i = 0; i < 32; i++){
	buf[i] = *ptr++;
    }

    debugk(DK_VERINET, "pli_getreg: regval=0x%x diagbuf=%s\n", regval, buf);

    if(status == 0) /* Read succeeded */
	return regval;
    else
	return -1; /* RPC failed */
}

/*
 * Client call: set a register on the simulator.
 */
uint32 pli_setreg(int devNo, uint32 regtype, uint32 regnum, uint32 regval)
{
    rpc_cmd_t command;
    verinet_t *v = &verinet[devNo];

    make_rpc_setreg_req(&command,regtype,regnum,regval);

    debugk(DK_VERINET,
	   "pli_setreg: regtype=0x%x regnum=0x%x regval=0x%x\n",
	   regtype, regnum, regval);

    sal_mutex_take(v->pli_mutex, sal_mutex_FOREVER);

    if(write_command(v->sockfd, &command) != RPC_OK) {
	printk("Error: pli_setreg write_command failed\n");
	sal_mutex_give(v->pli_mutex);
	return -1;
    }

    if (wait_command(v->sockfd,&command) < 0) {
	sal_mutex_give(v->pli_mutex);
	return -1;
    }

    sal_mutex_give(v->pli_mutex);

    return command.args[0];
}

/*
 * Client call: send packet to be transmitted into the switch
 */
uint32 pli_send_packet(int devNo, uint32 src_port, uint32 count,
		       int len, unsigned char *buf)
{
    verinet_t *v = &verinet[devNo];
    rpc_cmd_t command;

    make_rpc(&command,RPC_SEND_PACKET,RPC_OK,3,src_port,count,len);

    if(write_command(v->sockfd, &command) != RPC_OK) {
	printk("Error: pli_send_packet cmd failed\n");
	return -1;
    }
    if (writen(v->sockfd,buf,len) != len) {
	printk("Error: pli_send_packet data failed\n");
	return -1;
    }
    return 0;
}

/*
 * Client call: enable port transmission
 */
uint32 pli_tx_enable(int devNo, uint32 portmap)
{
    verinet_t *v = &verinet[devNo];
    rpc_cmd_t command;

    make_rpc(&command,RPC_TX_ENABLE,RPC_OK,1,portmap);

    if(write_command(v->sockfd, &command) != RPC_OK) {
	printk("Error: pli_tx_enable cmd failed\n");
	return -1;
    }
    return 0;
}

/*
 * Client call: tell testbench to shutdown
 */
uint32 pli_shutdown(int devNo)
{
    verinet_t *v = &verinet[devNo];
    rpc_cmd_t command;

    make_rpc(&command,RPC_SHUTDOWN,RPC_OK,0);

    if(write_command(v->sockfd, &command) != RPC_OK) {
	printk("Error: pli_shutdown cmd failed\n");
	return -1;
    }
    return 0;
}


/*
 * Send signal to simulation to activate
 */
uint32 pli_sim_start(int devNo)
{
    verinet_t *v = &verinet[devNo];
    rpc_cmd_t command;

    make_rpc(&command, RPC_IOCTL_REQ, RPC_OK, 3, BCM_SIM_ACTIVATE, 0, 0);

    if(write_command(v->sockfd, &command) != RPC_OK) {
	printk("Error: pli_sim_start cmd failed\n");
	return -1;
    }
    return 0;
}



int get_pkt_data(int sockfd, struct timeval* tv, uint32* data)
{
    int i, len;
    fd_set read_vect;
    uint32 swap;

    debugk(DK_VERINET+DK_VERBOSE, "get_pkt_data: sockfd=%d\n", sockfd);

    assert(sockfd > 2);		/* Verinet initialized? */

    /* Setup bitmap for read notification ... */
    FD_ZERO(&read_vect);
    FD_SET(sockfd, &read_vect);

    while (1) {
        if(select(sockfd+1,&read_vect,(fd_set*)0x0,(fd_set*)0x0,tv)<0){
            if (errno == EINTR) {
                /* 
                 * Interrupted by a signal such as a GPROF alarm so
                 * restart the call
                 */
                continue;
            }
            perror("get_command: select error");
            goto fail;
        } else {
            break;
        }
    }
    /*
     * Data ready to be read.
     */
    if (FD_ISSET(sockfd, &read_vect)) {
	/*
	 * Read RPC opcode.
	 */
	if(readn(sockfd, &swap, sizeof(uint32)) < sizeof(uint32)){
	    goto fail;
	}
	/* Convert precious info to host byte order */
        i=0;
	len = ntohl(swap);
        data[i]=len;
        while (len>0){ 
            if (readn(sockfd, &swap, sizeof(uint32)) < sizeof(uint32)){
                printk("get_pkt_data: could not read DATA.\n");
                goto fail;
            }
            i++;
            data[i]=ntohl(swap);
            len-=4;
        }

	return 1;
    }

 fail:
    return -1;
}

/*
 * Perform an SCHANNEL operation. 
 */
uint32 pli_schan(int devNo, uint32* words, int dw_write, int dw_read)
{
    int i ;
    verinet_t *v = &verinet[devNo];
    uint32 status = 0x0;
    rpc_cmd_t command;

    make_rpc(&command, RPC_SCHAN_REQ, RPC_OUTSTANDING, 0); 

    command.argcount = dw_write+2;
    command.args[0] = dw_write; 
    command.args[1] = dw_read; 

    for(i = 0; i < dw_write; i++) {
        command.args[i+2] = words[i]; 
    }   

    sal_mutex_take(v->pli_mutex, sal_mutex_FOREVER);

    if(write_command(v->sockfd, &command) != RPC_OK) {
	printk("Error: pli_schan write_command failed\n");
	sal_mutex_give(v->pli_mutex);
	return -1;
    }
    
    if (wait_command(v->sockfd, &command) < 0) {
	sal_mutex_give(v->pli_mutex);
	return -1;
    }

    sal_mutex_give(v->pli_mutex);

    /* Schannel operation response is the first argument */
    status = command.args[0];
    
    for(i = 0; i < dw_read; i++){
	words[i] = command.args[i+1]; 
    }
    
    if(status == 0) /* Read succeeded */
	return 0; 
    else
	return -1; /* RPC failed */
}
