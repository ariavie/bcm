
/*
 * $Id: ces.c,v 1.34 Broadcom SDK $
 * 
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
 */
#include <sal/core/libc.h>
#include <sal/appl/sal.h>
#include <shared/bsl.h>
#include <appl/diag/system.h>
#include <appl/diag/parse.h>
#include <appl/diag/system.h>
#include <bcm/error.h>
#include <bcm/port.h>
#include <soc/mspi.h>
#ifdef INCLUDE_CES
#include <bcm_int/esw/ces.h>
#include <bcm/stack.h>
#ifdef __KERNEL__
#define atoi _shr_ctoi
#include <linux/kernel.h>
#else
#include <unistd.h>
#endif

#define CLEAN_UP_AND_RETURN(_result) \
    parse_arg_eq_done(&parse_table); \
    return (_result);

#define SERVICE_IS_VALID(s) (s < BCM_CES_CIRCUIT_IDX_MAX)

/*
 * LIU loopback get|set
 */
#define BCM_DIAG_CES_MAX_LIU 16

typedef struct {
    int port;
    char *type;
    int enable;
} bcm_diag_ces_loopback_config_t;

typedef struct {
    uint32 address;
    uint32 value;
    int    liu;
    int    bank;
} bcm_diag_ces_reg_config_t;


typedef struct {
    int service;
    int port;
    int enable;
    int ingress;
    int master;
    int num_timeslots;
    int timeslot[BCM_CES_TDM_MAX];
} _bcm_ces_service_info_t;

/*
 * Globals
 */
static bcm_ces_service_config_t serviceConfig[BCM_CES_CIRCUIT_IDX_MAX];

char bcm_diag_cmd_esw_ces_init_usage[] = 
    "ces init option usage: \n"
    "\tces init - Initialize CES subsystem\n";

char bcm_diag_cmd_esw_ces_show_usage[] = 
    "ces show option usage:\n"
    "\tces show - Show the common CES configuration\n";

char bcm_diag_cmd_esw_ces_test_usage[] = 
    "ces test option usage:\n"
    "\tces test line [Args...] - Run framer PBRT test against LIU ports with loopback.\n"
    "\t                          The loopback may be provided by loopback plugs or by\n"
    "\t                          utilizing the LIU digital loopback path.\n"
    "\t                    time - Number of seconds to test each port for. Default is five seconds.\n"
    "\t                   ports - A comma separated list of ports to test (0 - 15). Defualt is all.\n"
    "\t                loopback - Type of loopback to use. d = Digital, p = External physical. Default is external\n"
    "\tces test system [Args...] - Run framer PRBT test against looped back CES services.\n"
    "\t                            The loopback is provided by setting ge0 into loopback.\n"
    "\t                     time - Number of seconds to test each port. Default is five seconds.\n"
    "\t                    ports - A comma separated list of ports to test (0 - 15). Defualt is all.\n";

char bcm_diag_cmd_esw_ces_diag_usage[] = 
    "ces diag option usage:\n"
    "\tces diag <option> [Args...] - Set or get ces diag information\n"
    "\t    ces diag set [Args...] - Set ces diag loopback\n"
    "\t                  loopback - Can be one of:\n"
    "\t                             0 - None\n"
    "\t                             1 - TDM local\n"
    "\t                             2 - TDM remote (requires that TDM port be specified)\n"
    "\t                             3 - Channelizer (requires service # be specified)\n"
    "\t                             4 - Dechannelizer\n"
    "\t                             5 - Classifier\n"
    "\t                             6 - Packet interface\n"
    "\t                             7 - Packet local\n"
    "\t                             8 - Packet remote\n"
    "\t                   service - CES service number. Required only for channelizer loopback\n"
    "\t                      port - TDM Port number. Required only for TDM remote loopback\n"
    "\t    ces diag get - Display CES diag information\n";

char bcm_diag_cmd_esw_ces_cclk_usage[] = 
    "ces cclk option usage:\n"
    "\tces cclk <option> [Args...] - Manage the common clock configuration\n" 
    "\t    ces cclk set [Args...] - Set common clock configuration\n"
    "\t                  enable - Boolean, Enables clock to framer, 1 = Enable, 0 = Disable\n"
    "\t                  select - Select common clock source, valid values are:\n"
    "\t                           0 - Reference clock 1\n"
    "\t                           1 - Reference clock 2\n"
    "\t                           2 - External clock\n"
    "\t                  c1_sel - Reference clock 1 source, valid values are:\n"
    "\t                           0 - Receive clock (must specify source port - c1_port)\n"
    "\t                           1 - Ref input 1\n"
    "\t                           3 - Internal\n"
    "\t                           7 - System clock select\n"
    "\t                  c2_sel - Reference clock 2 source, valid values are:\n"
    "\t                           0 - Receive clock (must specify source port - c2_port)\n"
    "\t                           2 - Ref input 2 (not used on Katana)\n"
    "\t                           7 - System clock select\n"
    "\t                 cl_port - Receive clock 1 source TDM port\n"
    "\t                 c2_port - Receive clock 2 source TDM port\n"
    "\t    ces cclk get\t- Get common clock configuration\n";

char bcm_diag_cmd_esw_ces_service_usage[] = 
    "ces service option usage:\n"
    "\t    ces service create [Args...] - [port] [slot]\t- Create a CES service\n"
    "\t                            port - TDM port\n"
    "\t                            slot - List of timeslots. If the port us unstructured then just timeslot 0 is required\n"
    "\t                           encap - Encapsulation type, valid values are:\n"
    "\t                                   0 - IP\n"
    "\t                                   1 - MPLS\n"
    "\t                                   2 - ETH\n"
    "\t                                   3 - L2TP\n"
    "\t                       ipversion - IP version, valid values are:\n"
    "\t                                   4 - IPv4\n"
    "\t                                   6 - IPv6\n"
    "\t    ces service show - Display summary configuration for all services\n"
    "\tces service <service#> <option> [args...]\t- Manage CES Service configuration\n"
    "\t    ces service <service#> set channel ingress [Args...] - Setup channel ingress parameters\n"
    "\t                            dba     - Boolean, yes for dba enabled\n"
    "\t                            autor   - Boolean, yes for auto r bit enabled\n"
    "\t                            mef     - Boolean, yes for mef len support enabled\n"
    "\t                            pbfsize - pbf size\n"
    "\t                            red     - Boolean, yes for redundancy enabled\n"
    "\t                            redcon  - Boolean, yes for redundancy config enabled\n"
    "\t    ces service <service#> set channel egress [Args...] - Setup channel egress parameters\n"
    "\t                            service - Service number\n"
    "\t                            pktsync  - Packet sync selector\n"
    "\t                            fpp      - Frames per packect. Defaults to 8 and is used to calculate payload size\n"
    "\t                            jbfrsize - Jitter buffer ring size\n"
    "\t                            jbfwsize - Jitter buffer window size\n"
    "\t                            jbfbop   - Jiffer buffer BOP\n"
    "\t                            dov      - Boolean, yes for drop on valid\n"
    "\t    ces service <service#> set channelizer ingress [Args...] - Setup channelizer ingress parameters\n"
    "\t                            service - Service number\n"
    "\t    ces service <service#> set channelizer engress [Args...] - Setup channelizer engress parameters\n"
    "\t                            service - Service number\n"
    "\t    ces service <service#> set map [Args...] - Configure the ingress and egress channel maps\n"
    "\t                             port - TDM port\n"
    "\t                             slot - TDM timeslot, can be a single integer or a comma separated list\n"
    "\t                           egress - Boolean, yes for egress, no for ingress\n"
    "\t    ces service <service#> set circuit [Args...] - Setup circuit parameters\n"
    "\t                            service - Service number\n"
    "\t    ces service <service#> set header [Args...] - Setup packet header parameters\n"
    "\t                            service - Service number\n"
    "\t                            eth.source - \n"
    "\t                            eth.destination - \n"
    "\t                            vlan.1.priority - \n"
    "\t                            vlan.1.vid - \n"
    "\t                            vlan.2.priority - \n"
    "\t                            vlan.2.vid - \n"
    "\t                            vlan_count - \n"
    "\t                            ipv4.tos - \n"
    "\t                            ipv4.ttl - \n"
    "\t                            ipv4.source - \n"
    "\t                            ipv4.destination - \n"
    "\t                            mpls.1.label - \n"
    "\t                            mpls.1.experimental - \n"
    "\t                            mpls.1.ttl - \n"
    "\t                            mpls.2.label - \n"
    "\t                            mpls.2.experimental - \n"
    "\t                            mpls.2.ttl - \n"
    "\t                            vc_label - \n"
    "\t                            udp.source - \n"
    "\t                            udp.destination - \n"
    "\t                            ip_version - \n"
    "\t                            mpls_count - \n"
    "\t                            l2tpv3_count - \n"
    "\t                            rtp_exists - \n"
    "\t                            rtp_pt - \n"
    "\t                            rtp_ssrc - \n"
    "\t                            udp_chksum - \n"
    "\t                            l2tpv3.udp_mode - \n"
    "\t                            l2tpv3.header - \n"
    "\t                            l2tpv3.session_local_id - \n"
    "\t                            l2tpv3.session_peer_id - \n"
    "\t                            l2tpv3.local_cookie1 - \n"
    "\t                            l2tpv3.local_cookie2 - \n"
    "\t                            l2tpv3.peer_cookie1 - \n"
    "\t                            l2tpv3.peer_cookie2 - \n"
    "\t    ces service <service#> show \t- Display configuration for specified service\n"
    "\t    ces service <service#> enable  [enable]\t- Enable or disable a service\n"
    "\t    ces service <service#> delete \t- Delete the specified service\n"
    "\t    ces service <service#> modify \t- Modify the specified service. Changes made through the \"service set ...\" commands are applied\n"
    "\t    ces service <service#> pm [reset] \t- Display or reset the PM stats for the specified service\n"
    "\t    ces service <service#> status \t - Display the egress status for the specified service\n"
    "\t    ces service <service#> rclock status \t- Display the received clock status for the specified service\n"
    "\t    ces service <service#> rclock get \t- Display the received clock configuration for the specified service\n"
    "\t    ces service <service#> rclock set  - Set received clock configuration\n"
    "\t                                rclock - Rclock instance. Four rclock instances are supported numbered 0 thorugh 3\n"
    "\t                                enable - Boolean, yes to enable, no to disable\n"
    "\t                             outputbrg - Select output BRG. 0 = port, 1 = System BRG1, 2 = System BRG2\n"
    "\t                                  port - TDM port to use if ouputbrg is port (0)\n"
    "\t                                  type - Clock recovery type. 1 = Adaptive, 0 = Differential\n"
    "\t    ces service <service#> cas <option> [Args...] - Manage CAS signalling\n"
    "\t    ces service <service#> cas enable get  - Display CAS enable state\n"
    "\t    ces service <service#> cas enable set  [enable] - Enable or disable CAS on specified service\n"
    "\t    ces service <service#> cas tx  [num_packets] - Specify how many CAS packets should be scheduled for transmission\n";

char bcm_diag_cmd_esw_ces_tdm_usage[] = 
    "ces tdm option usage:\n"
    "\tces tdm <option> [args...] - Manage CES TDM configuration\n"
    "\t    ces tdm show <port> - Show port details or if no port is specified then a summary of all ports\n"
    "\t    ces tdm set [args...] - Set port configuration. Args are:\n"
    "\t                 port     - Port number (39 through 54 for Katana)\n"
    "\t                 struct   - Boolean, yes for structured, no for unstructured\n"
    "\t                 oct      - Boolean, yes for octect aligned, no for unaligned\n"
    "\t                 t1d4     - Boolean, yes for T1/D4 framing\n"
    "\t                 sig      - Boolean, yes for signalling enabled\n"
    "\t                 rxclk    - Receive clock select. Valid values are:\n"
    "\t                            0 - Independent\n"
    "\t                            1 - CES Slave\n"
    "\t                 txclk    - Transmit clock select. Valid values are:\n"
    "\t                            0 - Loopback\n"
    "\t                            1 - Common Clock\n"
    "\t                            2 - Internal BRG\n"
    "\t                 casidle  - CAS idle timeslots\n"
    "\t                 stepsize - Step size\n"
    "\t                    rxcrc - Boolean, yes for receive CRC\n" 
    "\t                    txcrc - Boolean, yes for transmit CRC\n" 
    "\t                   master - Boolean, yes for framer master, no for framer slave\n"
    "\t                   sigfmt - Signaling format, 0 = Clear Channel, 1 = Invalid, 2 = T1 Four state, 3 = T1 Sixteen state\n"
    "\tces tdm framer [option> [args...]\t- Manage the CES TDM framer\n"
    "\t    ces tdm framer loopback [port] [enable] [type] - Configure the loopback configuration of the CES TDM framer port\n"
    "\t                                                     Valid loopback types are:\n"
    "\t                                                     0 - Remote\n"
    "\t                                                     1 - Payload\n"
    "\t                                                     2 - Framer. Local loopback\n"
    "\t    ces tdm framer status [port]\t- Display the framer status\n"
    "\t    ces tdm framer pm [port]\t - Display the framer PM stats\n"
    "\t    ces tdm cas [port] [egress]\t- Get ABCD bits for all timeslots of the circuit\n"
    "\t                            port - TDM port\n"
    "\t                          egress - Boolean, yes for egress, no for ingress\n";

char bcm_diag_cmd_esw_ces_liu_usage[] = 
    "ces liu option usage:\n"
    "\tces liu <option> [args...] - Manage the LIU configuration\n"
    "\t                             For all LIU operations valid port numbers are 0 through 15 and -1 for all ports.\n"
    "\t    ces liu init [t1] - Initialize the LIU device. Set t1=1 for T1 operation or t1=0 for E1 operation.\n"
    "\t                        The default mode of operation is T1.\n"
    "\t    ces liu enable [port] [enable] - Enable or disable the LIU port\n"
    "\t    ces liu status - Display status of all LIU ports\n"
    "\t    ces liu reset - Software reset of LIU. All registers will reset to default values\n"
    "\t    ces liu clock [port] - Select clock NOT YET SUPPORTED\n"
    "\t    ces liu loopback set [port] [type] [enable] - Configure the LIU loopback on a single or all ports\n"
    "\t                                           type - options are:\n"
    "\t                                                  a = analog\n"
    "\t                                                  d = digital\n"
    "\t                                                  r = remote\n"
    "\t                                           enable - options are:\n"
    "\t                                                  0 = disable\n"
    "\t                                                  1 = enable\n"
    "\t                                                - Use just \"port=-1\" to clear all loopbacks\n" 
    "\t    ces liu reg read <address> <bank> - Read a register by address\n"
    "\t                               adress - Hex address\n"
    "\t                                 bank - options are:\n"
    "\t                                        0 = primary (default)\n"
    "\t                                        1 = secondary\n"
    "\t                                        2 = individual\n"
    "\t    ces liu reg write <address> <value> - Write a register by address\n"
    "\t                                address - Hex address\n"
    "\t                                   bank - See valid options for register read\n"
    "\t                                  value - Eight bit value\n";

char bcm_diag_cmd_esw_ces_rpc_usage[] = 
    "ces rpc option usage:\n"
    "\tces rpc <option> [args...] - Manage and display the  RPC PM counters\n\n"
    "\tces rpc set [args...] - Set RPC PM global counter policy. Valid args are gc1 through gc8 representing\n"
    "\t                        global counters one through eight respectively. The value for each counter\n"
    "\t                        policy is a thirty two bit value where the bits represent:\n\n"
    "\t                        Bit  Count                         Bit  Count\n"
    "\t                        ---  ----------------------------  ---  ------------------------\n"
    "\t                         31  UNEXPECTED_END_OF_PACKET       15  FOUND_PTP\n"
    "\t                         30  IP_HEADER_CHECKSUM_ERROR       14  RTP_FLAG_ERROR\n"
    "\t                         29  UNKNOWN_PW_LABEL               13  HOST_DESIGNATED_PACKET\n"
    "\t                         28  CES_CW_ERROR                   12  MPLS_TTL_ZERO\n"
    "\t                         27  SIGNALING_PACKET               11  IP_ADDRESS_ERROR\n"
    "\t                         26  AIS_DBA_PACKET                 10  IP_WRONG_PROTOCOL\n"
    "\t                         25  RAI_R_BIT_IS_1                  9  BAD_IP_HEADER\n"
    "\t                         24  STRICT_CHECK_RESULTS            8  IP_NOT_UNICAST_PACKET\n"
    "\t                         23  CRC_ERROR                       7  UNKNOWN_ETHER_TYPE\n"
    "\t                         22  UDP_CHECKSUM_ERROR              6  ETH_DESTADDR_MISMATCH\n"
    "\t                         21  IP_LEN_ERROR                    5  ETH_NOT_UNICAST_PACKET\n"
    "\t                         20  UDP_LENGTH_ERROR                4  FOUND_UDP\n"
    "\t                         19  UDP_FORWARD_HP_QUEUE            3  FOUND_ECID\n"
    "\t                         18  UDP_FORWARD_LP_QUEUE            2  FOUND_VLAN\n"
    "\t                         17  DISABLED_CES_CHANNEL            1  FOUND_IP\n"
    "\t                         16  RAI_M_BITS_ARE_10               0  FOUND_MPLS\n\n"
    "\t                        The default values for each counter policy that will be used, unless specified on the\n"
    "\t                        command line, will be:\n\n"
    "\t                        Count  Value\n"
    "\t                        -----  ----------\n"
    "\t                            1  0xF0000000\n"
    "\t                            2  0x0F000000\n"
    "\t                            3  0x00F00000\n"
    "\t                            4  0x000F0000\n"
    "\t                            5  0x0000F000\n"
    "\t                            6  0x00000F00\n"
    "\t                            7  0x000000F0\n"
    "\t                            8  0x0000000F\n\n"
    "\tces rpc get - Display the current RPC global PM counts\n"  ;

char bcm_diag_cmd_esw_ces_help_usage[] = 
    "ces help option usage:\n"
    "\tces help <option> - Display command usage information for a specific option\n";

/*
 * Prototypes
 */
#ifndef __KERNEL__
int bcm_esw_port_tdm_init(int unit);
#endif
void bcm_esw_port_tdm_show_all(int unit);
int bcm_diag_ces_create(int unit, int *service, bcm_ces_channel_map_t *map_config, int encapsulation, int ip_version);

char *bcm_diag_esw_ces_encap_string[] = {
    "ENCAPSULATION_IP",
    "ENCAPSULATION_MPLS",
    "ENCAPSULATION_ETH",
    "3-Invalid",
    "4-Invalid",
    "ENCAPSULATION_L2TP"
};

/**
 * Function: 
 *      bcm_esw_show_header()
 * Purpose:
 *      Display header struct
 */
void bcm_esw_show_header(bcm_ces_packet_header_t *header, bcm_ces_packet_header_t *strict) {
    int i;

    cli_out("---- Packet Headers ----------------------------------\n");
    cli_out("Field                     Header                 Strict\n");
    cli_out("------------------------  ---------------------- ---------------------\n");
    cli_out("encapsulation           : %-18s     %-18s\n", 
            bcm_diag_esw_ces_encap_string[header->encapsulation],
            bcm_diag_esw_ces_encap_string[strict->encapsulation]);
    cli_out("a_source                : %02x:%02x:%02x:%02x:%02x:%02x      %02x:%02x:%02x:%02x:%02x:%02x\n",
            0xff & header->eth.source[0],
            0xff & header->eth.source[1],
            0xff & header->eth.source[2],
            0xff & header->eth.source[3],
            0xff & header->eth.source[4],
            0xff & header->eth.source[5],
            0xff & strict->eth.source[0],
            0xff & strict->eth.source[1],
            0xff & strict->eth.source[2],
            0xff & strict->eth.source[3],
            0xff & strict->eth.source[4],
            0xff & strict->eth.source[5]);
    cli_out("a_destination           : %02x:%02x:%02x:%02x:%02x:%02x      %02x:%02x:%02x:%02x:%02x:%02x\n",
            0xff & header->eth.destination[0],
            0xff & header->eth.destination[1],
            0xff & header->eth.destination[2],
            0xff & header->eth.destination[3],
            0xff & header->eth.destination[4],
            0xff & header->eth.destination[5],
            0xff & strict->eth.destination[0],
            0xff & strict->eth.destination[1],
            0xff & strict->eth.destination[2],
            0xff & strict->eth.destination[3],
            0xff & strict->eth.destination[4],
            0xff & strict->eth.destination[5]);

    cli_out("n_vlan_count            : %u                      %u\n",
            header->vlan_count,
            strict->vlan_count);

    if (header->vlan_count > 0 || strict->vlan_count > 0) {
    int n = (header->vlan_count > strict->vlan_count ? header->vlan_count:strict->vlan_count);

    for (i = 0;i < n;i++) {
        cli_out("vlan%d priority          : %d                      %d\n", 
                i,
                header->vlan[i].priority,
                strict->vlan[i].priority);
        cli_out("vlan%d vid               : 0x%02x                   0x%02x\n", 
                i,
                header->vlan[i].vid,
                strict->vlan[i].vid);
    }
    }
 
    cli_out("n_mpls_count            : %u                      %u\n",
            header->mpls_count,
            strict->mpls_count);

    if (header->mpls_count > 0 || strict->mpls_count > 0) {
    int n = (header->mpls_count > strict->mpls_count ? header->mpls_count:strict->mpls_count);

    for (i = 0;i < n;i++) {
        cli_out("mpls%d label              : 0x%08x             0x%08x\n", 
                i,
                header->mpls[i].label,
                strict->mpls[i].label);
        cli_out("mpls%d experimental       : 0x%08x             0x%08x\n", 
                i,
                header->mpls[i].experimental,
                strict->mpls[i].experimental);
        cli_out("mpls%d ttl                : 0x%08x             0x%08x\n", 
                i,
                header->mpls[i].ttl,
                strict->mpls[i].ttl);
    }
    }
 
    cli_out("n_label                 : %08x               %08x\n",
            header->vc_label,
            strict->vc_label);

    cli_out("n_ip_version            : %u                      %u\n",
            header->ip_version,
            strict->ip_version);
    if (header->ip_version == bcmCesIpV4 || strict->ip_version == bcmCesIpV4) {
    cli_out("ipv4.n_source           : %d.%d.%d.%d              %d.%d.%d.%d\n",
            (header->ipv4.source >> 24) & 0xFF,
            (header->ipv4.source >> 16) & 0xFF,
            (header->ipv4.source >> 8) & 0xFF,
            header->ipv4.source & 0xFF,
            (strict->ipv4.source >> 24) & 0xFF,
            (strict->ipv4.source >> 16) & 0xFF,
            (strict->ipv4.source >> 8) & 0xFF,
            strict->ipv4.source & 0xFF);

    cli_out("ipv4.n_destination      : %d.%d.%d.%d              %d.%d.%d.%d\n",
            (header->ipv4.destination >> 24) & 0xFF,
            (header->ipv4.destination >> 16) & 0xFF,
            (header->ipv4.destination >> 8) & 0xFF,
            header->ipv4.destination & 0xFF,
            (strict->ipv4.destination >> 24) & 0xFF,
            (strict->ipv4.destination >> 16) & 0xFF,
            (strict->ipv4.destination >> 8) & 0xFF,
            strict->ipv4.destination & 0xFF);

    cli_out("ipv4.n_tos              : %u                      %u\n",
            header->ipv4.tos,
            strict->ipv4.tos);
    cli_out("ipv4.n_ttl              : %-4u                   %-4u\n",
            header->ipv4.ttl,
            strict->ipv4.ttl);
    }

    if (header->ip_version == bcmCesIpV6 || strict->ip_version == bcmCesIpV6) {
    cli_out("ipv6.traffic_class      : %u                    %u\n",
            header->ipv6.traffic_class,
            strict->ipv6.traffic_class);   /* IPv6 traffic class */
    cli_out("ipv6.flow_label         : %u                    %u\n",
            header->ipv6.flow_label,
            strict->ipv6.flow_label);      /* IPv6 flow label */
    cli_out("ipv6.hop_limit          : %u                    %u\n",
            header->ipv6.hop_limit,
            strict->ipv6.hop_limit);
    cli_out("ipv6.source             : ");
    for (i = 0;i < 16;i += 2) {
        cli_out("%02x%02x", header->ipv6.source[i], header->ipv6.source[i + 1]);      /* IPv6 source address */
        if (i != 14)
        cli_out(":");
        else
        cli_out("    ");
    }
    for (i = 0;i < 16;i += 2) {
        cli_out("%02x%02x", strict->ipv6.source[i], strict->ipv6.source[i + 1]);      /* IPv6 source address */
        if (i != 14)
        cli_out(":");
        else
        cli_out("\n");
    }
    cli_out("ipv6.destination        : ");
    for (i = 0;i < 16;i += 2) {
        cli_out("%02x%02x", header->ipv6.destination[i], header->ipv6.destination[i + 1]);      /* IPv6 destination address */
        if (i != 14)
        cli_out(":");
        else
        cli_out("    ");
    }
    for (i = 0;i < 16;i += 2) {
        cli_out("%02x%02x", strict->ipv6.destination[i], strict->ipv6.destination[i + 1]);      /* IPv6 destination address */
        if (i != 14)
        cli_out(":");
        else
        cli_out("\n");
    }
    }

    cli_out("udp.n_destination       : %08x               %08x\n",
            header->udp.destination,
            strict->udp.destination);
    cli_out("udp.n_source            : %08x               %08x\n",
            header->udp.source,
            strict->udp.source);
    cli_out("udp_chksum              : %d                      %d\n",
            header->udp_chksum,
            strict->udp_chksum);

    if (header->rtp_exists || strict->rtp_exists) {
    cli_out("rtp_exists              : %d                      %d\n",
            header->rtp_exists,
            strict->rtp_exists);
    cli_out("rtp_pt                  : %d                      %d\n",
            header->rtp_pt,
            strict->rtp_pt);
    cli_out("rtp_ssrc                : %d                      %d\n",
            header->rtp_ssrc,
            strict->rtp_ssrc);
    }

    cli_out("l2tpv3_count            : %u                      %u\n",
            header->l2tpv3_count,
            strict->l2tpv3_count);
    if (header->l2tpv3_count > 0 || strict->l2tpv3_count > 0) {
    cli_out("l2tpv3.udp_mode         : %u                      %u\n",
            header->l2tpv3.udp_mode,
            strict->l2tpv3.udp_mode);               /* UDP mode */
    cli_out("l2tpv3.header           : %x                    %x\n",
            header->l2tpv3.header,
            strict->l2tpv3.header);              /* Header */
    cli_out("l2tpv3.session_local_id : %x                    %x\n",
            header->l2tpv3.session_local_id,
            strict->l2tpv3.session_local_id);    /* Session local ID */
    cli_out("l2tpv3.session_peer_id  : %x                    %x\n",
            header->l2tpv3.session_peer_id,
            strict->l2tpv3.session_peer_id);     /* Session peer ID */
    cli_out("l2tpv3.local_cookie1    : %x                    %x\n",
            header->l2tpv3.local_cookie1,
            strict->l2tpv3.local_cookie1);       /* Local cookie 1 */
    cli_out("l2tpv3.local_cookie2    : %x                    %x\n",
            header->l2tpv3.local_cookie2,
            strict->l2tpv3.local_cookie2);       /* Local cookie 2 */
    cli_out("l2tpv3.peer_cookie1     : %x                    %x\n",
            header->l2tpv3.peer_cookie1,
            strict->l2tpv3.peer_cookie1);        /* Peer cookie 1 */
    cli_out("l2tpv3.peer_cookie2     : %x                    %x\n",
            header->l2tpv3.peer_cookie2,
            strict->l2tpv3.peer_cookie2);
    }
    cli_out("------------------------------------------------------\n");
}

void bcm_esw_show_channelizer(int ingress, bcm_ces_channel_map_t *channelizer)
{
    int i;

    cli_out("---- Channelizer %s ----\n",(ingress ? "Ingress":"Egress"));
/*    cli_out("n_channel_id: %d\n",channelizer->n_channel_id);*/
    cli_out("n_first     : %u\n",channelizer->first);
    cli_out("n_size      : %u\n",channelizer->size);
    for (i = channelizer->first;i < (channelizer->first + channelizer->size);i++)
    {
        cli_out("circuit_id/slot: %d/%d\n",
                channelizer->circuit_id[i],
                channelizer->slot[i]);
    }
}

void 
bcm_esw_ces_show_config(bcm_ces_service_config_t *config)
{
    cli_out("Config\n");

    bcm_esw_show_header(&config->header, &config->strict_data);
/*    bcm_esw_port_tdm_show(record->tdmRecord); */

    cli_out("---- Ingress channel ----\n");
    cli_out("b_dba                 : %u\n",config->ingress_channel_config.dba);
    cli_out("n_pbf_size            : %u\n",config->ingress_channel_config.pbf_size);


    cli_out("---- Egress channel ----\n"); 
    cli_out("n_packet_sync_selector: %u\n",config->egress_channel_config.packet_sync_selector);
    cli_out("b_rtp_exists          : %u\n",config->egress_channel_config.rtp_exists);
    cli_out("frames_per_packet     : %u\n", config->egress_channel_config.frames_per_packet);
    cli_out("n_jbf_ring_size       : %u\n",config->egress_channel_config.jbf_ring_size);
    cli_out("n_jbf_win_size        : %u\n",config->egress_channel_config.jbf_win_size);
    cli_out("n_jbf_bop             : %u\n",config->egress_channel_config.jbf_bop);

    bcm_esw_show_channelizer(TRUE, &config->ingress_channel_map);
    bcm_esw_show_channelizer(FALSE, &config->egress_channel_map);
}


/**
 * Function: 
 *      bcm_diag_ces_show()
 * Purpose:
 *      Show common CES configuration
 */
void bcm_diag_ces_show(int unit) {
    bcm_ces_service_global_config_t *config = (bcm_ces_service_global_config_t *)SOC_CONTROL(unit)->ces_ctrl;
    int i;
    int freeCount = 0;
    int usedCount = 0;

    cli_out("Service \n");

    if (config != NULL && BCM_CES_IS_CONFIGURED(config)) {
	cli_out("TDM Protocol             : %s\n", (config->protocol ? "T1":"E1"));
	cli_out("System clock             : %d Hz\n", config->system_clock_rate);
	cli_out("Common ref clock         : %d Hz\n", config->common_ref_clock_rate);
	cli_out("e_one_sec_pulse_direction: %d\n", config->ndInit.e_one_sec_pulse_direction);
	cli_out("e_bit_order              : %d\n", config->ndInit.e_bit_order);
	cli_out("n_packet_max             : %lu\n", config->ndInit.n_packet_max);
	cli_out("n_pw_max                 : %lu\n", config->ndInit.n_pw_max);
	cli_out("n_pbf_max                : %lu\n", config->ndInit.n_pbf_max);
	cli_out("b_isr_mode               : %lu\n", config->ndInit.b_isr_mode);
	cli_out("n_isr_task_priority      : %lu\n", config->ndInit.n_isr_task_priority);
	cli_out("n_isr_task_wakeup        : %lu\n", config->ndInit.n_isr_task_wakeup);
	cli_out("p_cb_pmi                 : %p\n", (void*)config->ndInit.p_cb_pmi);
	cli_out("p_cb_pbi                 : %p\n", (void*)config->ndInit.p_cb_pbi);
	cli_out("p_cb_cwi                 : %p\n", (void*)config->ndInit.p_cb_cwi);
	cli_out("p_cb_psi                 : %p\n", (void*)config->ndInit.p_cb_psi);
	cli_out("p_cb_tpi                 : %p\n", (void*)config->ndInit.p_cb_tpi);
	cli_out("n_user_data_pbi          : %lu\n", config->ndInit.n_user_data_pbi);
	cli_out("n_user_data_cwi          : %lu\n", config->ndInit.n_user_data_cwi);
	cli_out("n_user_data_psi          : %lu\n", config->ndInit.n_user_data_psi);
	cli_out("n_user_data_tpi          : %lu\n", config->ndInit.n_user_data_tpi);

	cli_out("b_no_length_check        : %lu\n", config->ndMac.x_command_config.b_no_length_check);
	cli_out("b_enable_mac_receive     : %lu\n", config->ndMac.x_command_config.b_enable_mac_receive);
	cli_out("b_enable_mac_transmit    : %lu\n", config->ndMac.x_command_config.b_enable_mac_transmit);
	cli_out("b_enable_promiscuous_mode: %lu\n", config->ndMac.x_command_config.b_enable_promiscuous_mode);
	cli_out("b_fwd_pause_frames       : %lu\n", config->ndMac.x_command_config.b_fwd_pause_frames);
	cli_out("b_enable_frame_padding   : %lu\n", config->ndMac.x_command_config.b_enable_frame_padding);
	cli_out("a_mac                    : %02x:%02x:%02x:%02x:%02x:%02x\n", 
                0xff & config->ndMac.a_mac[0],
                0xff & config->ndMac.a_mac[1],
                0xff & config->ndMac.a_mac[2],
                0xff & config->ndMac.a_mac[3],
                0xff & config->ndMac.a_mac[4],
                0xff & config->ndMac.a_mac[5]);
	cli_out("n_rx_section_empty       : %lu\n", config->ndMac.n_rx_section_empty);
	cli_out("n_rx_section_full        : %lu\n", config->ndMac.n_rx_section_full);
	cli_out("n_tx_section_empty       : %lu\n", config->ndMac.n_tx_section_empty);
	cli_out("n_tx_section_full        : %lu\n", config->ndMac.n_tx_section_full);
	cli_out("n_rx_almost_empty        : %lu\n", config->ndMac.n_rx_almost_empty);
	cli_out("n_rx_almost_full         : %lu\n", config->ndMac.n_rx_almost_full);
	cli_out("n_tx_almost_empty        : %lu\n", config->ndMac.n_tx_almost_empty);
	cli_out("n_tx_almost_full         : %lu\n", config->ndMac.n_tx_almost_full);

	cli_out("a_lops_threshold_table[0]: %d\n", config->ndGlobal.a_lops_threshold_table[0]);
	cli_out("a_aops_threshold_table[0]: %d\n", config->ndGlobal.a_aops_threshold_table[0]);
	cli_out("n_cw_mask                : %04x\n", config->ndGlobal.n_cw_mask);

	cli_out("a_dest_mac               : %02x:%02x:%02x:%02x:%02x:%02x\n", 
                0xff & config->ndUcode.a_dest_mac[0],
                0xff & config->ndUcode.a_dest_mac[1],
                0xff & config->ndUcode.a_dest_mac[2],
                0xff & config->ndUcode.a_dest_mac[3],
                0xff & config->ndUcode.a_dest_mac[4],
                0xff & config->ndUcode.a_dest_mac[5]);

	cli_out("b_ecid_direct            : %lu\n", config->ndUcode.b_ecid_direct);
	cli_out("b_mpls_direct            : %lu\n", config->ndUcode.b_mpls_direct);
	cli_out("b_udp_direct             : %lu\n", config->ndUcode.b_udp_direct);
	cli_out("n_dest_ipv4              : %08lx\n", config->ndUcode.n_dest_ipv4);

	cli_out("---- Records ----\n");

	for (i = 0;i < BCM_CES_CIRCUIT_IDX_MAX;i++)
	{
	    if (config->bcm_ces_service_records[i]->config.flags & BCM_CES_FLAG_FREE)
		freeCount++;
	    else
		usedCount++;
	}

	cli_out("Free:%d  Allocated:%d\n", freeCount, usedCount);

	/* bcm_esw_ces_show_records(unit); */
    } else {
	cli_out("Not configured\n");
    }
}


#if 0
void
bcm_esw_ces_show_records(int unit)
{
    int i;
    bcm_ces_service_global_config_t *ces_ctrl = (bcm_ces_service_global_config_t *)SOC_CONTROL(unit)->ces_ctrl;

    for (i = 0;i < BCM_CES_CIRCUIT_IDX_MAX;i++)
    {
	if (ces_ctrl->bcm_ces_service_records[i] != NULL)
	{
	    bcm_esw_ces_show_record(unit, i);
	}
    }
}

#endif


/**
 * Function: 
 *      bcm_diag_esw_ces_cclk_config_get
 * Purpose:
 *      Get the common clock configuration
 */
static char *bcm_ces_clock_source_select_string[] = {
    "Reference 1", "Reference 2", "External" };
static char *bcm_ces_ref_clock_select_string[] = {
    "RCLK from TDM port", "Ref 1 input", "Ref 2 input", "Internal", "Invalid", "Invalid", "Invalid", "System" };

int bcm_diag_esw_ces_cclk_config_get(int unit, bcm_ces_cclk_config_t *config)
{
  cli_out("---- Common Clock Configuration ----\n");
  cli_out("cclk_enable      : %d\n", config->cclk_enable);
  cli_out("cclk_select      : %s\n", bcm_ces_clock_source_select_string[config->cclk_select]);
  cli_out("ref_clk_proto    : %s\n", (config->ref_clk_proto ? "T1":"E1"));
  cli_out("ref_clk_1_select : %s\n", bcm_ces_ref_clock_select_string[config->ref_clk_1_select]);
  cli_out("ref_clk_2_select : %s\n", bcm_ces_ref_clock_select_string[config->ref_clk_2_select]);
  cli_out("ref_clk_1_port   : %d\n", config->ref_clk_1_port);
  cli_out("ref_clk_2_port   : %d\n", config->ref_clk_2_port);
  cli_out("ext_clk_1_dir    : %s\n", (config->ext_clk_1_dir ? "Out":"In"));
  cli_out("ext_clk_2_dir    : %s\n", (config->ext_clk_2_dir ? "Out":"In"));
  cli_out("------------------------------------\n");

  return BCM_E_NONE;
}

/*
 * Function: 
 *      bcm_diag_esw_ces_cclk_config_set
 * Purpose:
 *      Set the common clock configuration
 */
int bcm_diag_esw_ces_cclk_config_set(int unit, bcm_ces_cclk_config_t *config)
{
    int ret = BCM_E_NONE;

    if (config->cclk_select != bcmCesCclkSelectRefClk1 &&
	config->cclk_select != bcmCesCclkSelectRefClk2 &&
	config->cclk_select != bcmCesCclkSelectExternalClk) {
	cli_out("Invalid common clock source selection\n");
	return BCM_E_PARAM;
    }

    if (config->ref_clk_1_select != bcmCesRefClkSelectRclk &&
	config->ref_clk_1_select != bcmCesRefClkSelectRefInput1 &&
	config->ref_clk_1_select != bcmCesRefClkSelectNomadBrg &&
	config->ref_clk_1_select != bcmCesRefClkSelectPtp) {
	cli_out("Invalid clock 1 source selection\n");
	return BCM_E_PARAM;
    }

    if (config->ref_clk_2_select != bcmCesRefClkSelectRclk &&
	config->ref_clk_2_select != bcmCesRefClkSelectRefInput2 &&
	config->ref_clk_2_select != bcmCesRefClkSelectPtp) {
	cli_out("Invalid clock 2 source selection\n");
	return BCM_E_PARAM;
    }

    /*
     * If RCLK is selected then verify c1_port and c2_port
     */
    if (config->ref_clk_2_port == bcmCesRefClkSelectRclk ||
	config->ref_clk_1_port == bcmCesRefClkSelectRclk) {
	if (config->ref_clk_1_port < 39 || config->ref_clk_1_port > 55) {
	    cli_out("c1_port out of range\n");
	    return BCM_E_PARAM;
	}
	if (config->ref_clk_2_port < 39 || config->ref_clk_2_port > 55) {
	    cli_out("c2_port out of range\n");
	    return BCM_E_PARAM;
	}

    }

    ret = bcm_ces_services_cclk_config_set(unit, config);
    return ret;
}





#ifndef __KERNEL__
/**
 * bcm_ces_tdm_init(int unit)
 *
 */
#define BCM_CES_TDM_PORT_NUM 16
void bcm_ces_tdm_init(int unit) 
{
    int ret;
    bcm_pbmp_t pbmp; /* Port bitmap */
    bcm_gport_t tdm_gport[BCM_CES_TDM_PORT_NUM];
    int port = 1;
    bcm_module_t modid;

    sal_memset(tdm_gport, 0, sizeof(bcm_gport_t) * BCM_CES_TDM_PORT_NUM);

    BCM_PBMP_ASSIGN(pbmp, PBMP_CMIC(unit));
    BCM_PBMP_OR(pbmp, PBMP_HG_ALL(unit));
    ret = bcm_stk_my_modid_get(unit, &modid);

    BCM_GPORT_MODPORT_SET(tdm_gport[port], modid,port);

    ret = bcm_esw_port_tdm_init(unit);
    if (ret != BCM_E_NONE)
    {
	cli_out("bcm_esw_port_tdm_init() failed with code:%d\n", ret);
    }

    cli_out("TDM module initialized\n");
}
#endif


void bcm_ces_show_service(int unit);

/**
 * parse_fail()
 *
 */
int parse_fail(parse_table_t *parse_table, char *currentArg)
{
    cli_out("Invalid option: %s\nValid options are:\n", currentArg);
    parse_eq_format(parse_table);
    cli_out("\n");

    return CMD_FAIL;
}


/*********************************************************************************************
 *
 *
 *
 *
 *
 *
 * TDM
 *
 *
 *
 *
 *
 *
 *********************************************************************************************/

/**
 * Function: 
 *      bcm_diag_ces_tdm_cas_abcd()
 * Purpose:
 *      Show ingress or egress CAS bits
 */
void bcm_diag_ces_tdm_cas_abcd(int unit, int port, int egress) {
    uint8 cas_abcd[BCM_CES_SLOT_MAX];
    int i;
    bcm_tdm_port_cas_status_t status = (egress == 0 ? bcmTdmPortCasStatusIngress:bcmTdmPortCasStatusEgress);

    bcm_port_tdm_cas_abcd_get(unit,
			      port, 
			      status,
			      cas_abcd);

    cli_out("\n\nABCD bits for port:%d %s\n\n", port, (egress ? "egress":"ingress"));
    for (i = 0;i < (BCM_CES_SLOT_MAX/2);i++) {
	cli_out("%2d:0x%02x    %2d:0x%02x\n",
                i, cas_abcd[i],
                (i + (BCM_CES_SLOT_MAX/2)), cas_abcd[(i + (BCM_CES_SLOT_MAX/2))]);
    }

    cli_out("\n\n");
}

/**
 * Function: 
 *      bcm_diag_ces_tdm_cas_status()
 * Purpose:
 *      Find timeslots on which a CAS change has occured
 */
void bcm_diag_ces_tdm_cas_change(int unit, int port) {
    uint32 changed;

    bcm_port_tdm_cas_status_get(unit,
				port, 
				&changed);

    cli_out("\n\nPort:%d CAS change timeslots:0x%08x\n\n", port, changed);
}

/**
 * Function: 
 *      bcm_diag_ces_tdm_show_summary()
 * Purpose:
 *      Show TDM port summary
 */
void bcm_diag_ces_tdm_show_summary(int unit) {
    soc_port_t port;
    uint32 count;
    bcm_tdm_port_record_t **tdm_ctrl = (bcm_tdm_port_record_t **)&SOC_CONTROL(unit)->tdm_ctrl;
    bcm_tdm_port_record_t *record;
    int i;
    char slots[BCM_CES_SLOT_MAX + 1];

    /*
     * Find number of TDM ports
     */
    BCM_PBMP_COUNT(PBMP_TDM_ALL(unit), count);
    SOC_CONTROL(unit)->tdm_count = count;


    record = *tdm_ctrl;

    cli_out("\nCES TDM Port Summary\n");
    cli_out("F - Free  C - Configured  M - Modified  E - Enabled\n");
    cli_out("Str - Structured  Oct - Octet Aligned  Fra - Framed\n");
    cli_out("Sig - Signalling  Crx - Recieve Clock  Ctx - Transmit Clock\n");
    cli_out("FrM - Framer master\n\n");
    cli_out("Port F C M E Str Oct Fra Sig Crx Ctx FrM |      |  Timeslots    |       |\n");
    cli_out("---- - - - - --- --- --- --- --- --- --- --------------------------------\n");

    PBMP_TDM_ITER(unit, port) 
    {
	for (i = 0;i < BCM_CES_SLOT_MAX;i++) {
	    if (record->serviceList[i] == NULL)
		slots[i] = '.';
	    else
	    {
		if (record->config.b_structured)
		    slots[i] = '*';
		else
		    slots[i] = '+';
	    }
	}

	slots[BCM_CES_SLOT_MAX] = '\0';

	cli_out("%4d %c %c %c %c %3c %3c %3c %3c %3d %3d %3c %s\n",
                port,
                (BCM_CES_IS_FREE(record) ? 'X':'-'),
                (BCM_CES_IS_CONFIGURED(record) ? 'X':'-'),
                (BCM_CES_IS_MODIFIED(record) ? 'X':'-'),
                (BCM_CES_IS_ENABLED(record) ? 'X':'-'),
                (record->config.b_structured ? 'X':'-'),
                (record->config.b_octet_aligned ? 'X':'-'),
                (record->config.b_T1_D4_framing ? 'X':'-'),
                (record->config.b_signaling_enable ? 'X':'-'),
                record->config.e_clk_rx_select,
                record->config.e_clk_tx_select,
                (record->config.b_master ? 'X':'-'),
                slots);
	/*
	 * Next
	 */
	record++;
    }
}

/**
 * Function: 
 *      bcm_esw_port_tdm_show()
 * Purpose:
 *      Show TDM port 
 */
static char *bcm_ces_tdm_port_rx_clk_string[] = {
    "Independent", "CES slave"};
static char *bcm_ces_tdm_port_tx_clk_string[] = {
  "Loopback", "Invalid", "Common Clock", "Internal BRG"};
static char *bcm_ces_tdm_structured_string[] = {
    "Unstructured", "Structured"};
static char *bcm_ces_tdm_aligned_string[] = {
    "Unaligned", "Aligned"};
static char *bcm_ces_tdm_signaling_string[] = {
    "No Signaling", "Signaling"};
static char *bcm_ces_tdm_D4_string[] = {
    "ESF", "D4"};
void bcm_esw_port_tdm_show(bcm_tdm_port_config_t *config) {
    cli_out("b_structured          : %s\n", bcm_ces_tdm_structured_string[config->b_structured]);
    cli_out("b_octet_aligned       : %s\n", bcm_ces_tdm_aligned_string[config->b_octet_aligned]);
    cli_out("b_signaling_enable    : %s\n", bcm_ces_tdm_signaling_string[config->b_signaling_enable]);
    cli_out("n_signaling_format    : %d\n", config->n_signaling_format);
    cli_out("b_T1_D4_framing       : %s\n", bcm_ces_tdm_D4_string[config->b_T1_D4_framing]);
    cli_out("e_clk_rx_select       : %s\n", bcm_ces_tdm_port_rx_clk_string[config->e_clk_rx_select]);
    cli_out("e_clk_tx_select       : %s\n", bcm_ces_tdm_port_tx_clk_string[config->e_clk_tx_select]);
    cli_out("b_txcrc               : %s\n", (config->b_txcrc ? "True":"False"));
    cli_out("b_rxcrc               : %s\n", (config->b_rxcrc ? "True":"False"));
    cli_out("b_master              : %s\n", (config->b_master ? "True":"False"));
    cli_out("-----------------\n");
}

#ifndef __KERNEL__


/**
 * Function: 
 *      bcm_diag_esw_ces_framer_loopback
 * Purpose:
 *      Enable or disable loopback in the CES framer device
 */
void bcm_diag_esw_ces_framer_loopback(int unit, int port, int enable, int loopback) {
    int ret;

    switch (loopback) {
    case 0:
	loopback = LOOPBACK_REMOTE_SWITCH;
	break;
    case 1:
	loopback = LOOPBACK_PAYLOAD_SWITCH;
	break;
    case 2:
	loopback = LOOPBACK_FRAMER_SWITCH;
	break;

    default:
	cli_out("Unsupported loopback type:%d\n", loopback);
	return;
    }

    ret = bcm_esw_port_tdm_framer_port_loopback_set(unit,
						port,
						enable,
						loopback,
						0xffffffff,
						0,
						0);
    if (ret != BCM_E_NONE) {
	cli_out("Framer loopback %s failed for port:%d loopback%d\n",
                (enable == 0 ? "Disable":"Enable"),
                port, loopback);
    }
}


/**
 * Function: 
 *      bcm_diag_esw_ces_framer_status
 * Purpose:
 *      Retrieve and display CES TDM framer port status
 */
void bcm_diag_esw_ces_framer_status(int unit, int *port) {
    int ret;
    AgFramerPortStatus ndFramerPortStatus;

    ret = bcm_esw_port_tdm_framer_port_status(unit,
					      *port,
					      &ndFramerPortStatus);

    if (ret != BCM_E_NONE) {
	cli_out("Framer status failed for port:%d\n",
                *port);
    } else {
	cli_out("---- Framer Status for Port:%d ----\n", *port);
	cli_out("b_loopback_deactivation_code    : %lu\n", ndFramerPortStatus.b_loopback_deactivation_code);
	cli_out("b_loopback_activation_code      : %lu\n", ndFramerPortStatus.b_loopback_activation_code);
	cli_out("b_sa6_change_unlatched_status   : %lu\n", ndFramerPortStatus.b_sa6_change_unlatched_status);
	cli_out("b_prm_status                    : %lu\n", ndFramerPortStatus.b_prm_status);
	cli_out("b_excessive_zeroes_status       : %lu\n", ndFramerPortStatus.b_excessive_zeroes_status);
	cli_out("b_cas_ts_16_ais_status          : %lu\n", ndFramerPortStatus.b_cas_ts_16_ais_status);
	cli_out("b_cas_ts_16_los_status          : %lu\n", ndFramerPortStatus.b_cas_ts_16_los_status);
	cli_out("b_recive_bits_status            : %lu\n", ndFramerPortStatus.b_recive_bits_status);
	cli_out("b_signal_multiframe_error_status: %lu\n", ndFramerPortStatus.b_signal_multiframe_error_status);
	cli_out("b_crc_error_status              : %lu\n", ndFramerPortStatus.b_crc_error_status);
	cli_out("b_frame_alarm_status            : %lu\n", ndFramerPortStatus.b_frame_alarm_status );
	cli_out("b_remote_multiframe_error_status: %lu\n", ndFramerPortStatus.b_remote_multiframe_error_status );
	cli_out("b_remote_alarm_indec_status     : %lu\n", ndFramerPortStatus.b_remote_alarm_indec_status );
	cli_out("b_oof_status                    : %lu\n", ndFramerPortStatus.b_oof_status);
	cli_out("b_ais_status                    : %lu\n", ndFramerPortStatus.b_ais_status);
	cli_out("b_los_status                    : %lu\n", ndFramerPortStatus.b_los_status);
	cli_out("-----------------------------------\n");

    }
}

/**
 * Function: 
 *      bcm_diag_esw_ces_framer_pm
 * Purpose:
 *      Retrieve and display CES TDM framer port pm counts
 */
void bcm_diag_esw_ces_framer_pm(int unit, int *port) {
    int ret;
    AgFramerPortPm ndFramerPortPm;

    ret = bcm_esw_port_tdm_framer_port_pm(unit,
					      *port,
					      &ndFramerPortPm);

    if (ret != BCM_E_NONE) {
	cli_out("Framer pm failed for port:%d\n",
                *port);
    } else {
	cli_out("---- Framer PM for Port:%d ----\n", *port);
	cli_out("n_crc_error_counter       : %d\n", ndFramerPortPm.n_crc_error_counter);
	cli_out("n_line_code_voi_error     : %d\n", ndFramerPortPm.n_line_code_voi_error);
	cli_out("n_e_bit_error_count       : %d\n", ndFramerPortPm.n_e_bit_error_count);
	cli_out("n_transmit_slip_count     : %d\n", ndFramerPortPm.n_transmit_slip_count);
	cli_out("n_recive_slip_count       : %d\n", ndFramerPortPm.n_recive_slip_count);
	cli_out("n_frame_alim_error_count  : %d\n", ndFramerPortPm.n_frame_alim_error_count);
	cli_out("n_change_framer_alim_count: %d\n", ndFramerPortPm.n_change_framer_alim_count);
	cli_out("n_sev_frame_error_count   : %d\n", ndFramerPortPm.n_sev_frame_error_count);
	cli_out("-------------------------------\n");
    }
}
#endif


/**
 * bcm_ces_tdm_set(int unit)
 *
 */
int bcm_diag_ces_tdm_set(int unit, int *port, bcm_tdm_port_config_t *config) {
    int ret = BCM_E_NONE;

    /*
     * Check values
     */
    if (*port < BCM_CES_TDM_PORT_BASE_KATANA || *port > 54) {
	cli_out("Port is out of range, valid range is 39 through 54\n");
	return BCM_E_PARAM;
    }

    if (config->e_clk_rx_select > 1) {
	cli_out("Receive clock out of range\n");
	return BCM_E_PARAM;
    }
    if (config->e_clk_tx_select > 2) {
	cli_out("Tramsmit clock out of range\n");
	return BCM_E_PARAM;
    }

    switch (config->e_clk_rx_select) {
    case 0:
	config->e_clk_rx_select = bcmCesRxClkSelectIndependent;
	break;
    case 1:
	config->e_clk_rx_select = bcmCesRxClkSelectCesSlave;
	break;
    default:
	cli_out("Invalid receive clock selection\n");
	return BCM_E_PARAM;
	break;
    }

    switch (config->e_clk_tx_select) {
    case 0:
	config->e_clk_tx_select = bcmCesTxClkSelectLoopback;
	break;
    case 1:
	config->e_clk_tx_select = bcmCesTxClkSelectCclk;
	break;
    case 2:
	config->e_clk_tx_select = bcmCesTxClkSelectInternalBrg;
	break;
    default:
	cli_out("Invalid transmit clock selection\n");
	return BCM_E_PARAM;
	break;
    }

    /*
     * Do it to it.
     */
    ret = bcm_port_tdm_config_set(unit, *port, config);

    if (ret != BCM_E_NONE) {
	cli_out("bcm_tdm_port_config_set() failed with code:%d\n", ret);
    }

    return ret;
}

/**
 * bcm_ces_tdm_show(int unit)
 *
 */
void bcm_diag_ces_tdm_show(int unit, int *port) {
    bcm_tdm_port_config_t config;
    uint32 ces_ports[BCM_CES_SLOT_MAX];
    uint32 n_ports;
    int i;

    /*
     * Get config for service
     */
    if (bcm_port_tdm_config_get(unit, *port, &config) != BCM_E_NONE)
	return;

    if (bcm_port_tdm_ces_ports_get(unit, *port, &n_ports, ces_ports) != BCM_E_NONE)
	return;

    /*
     * Display config
     */
    cli_out("----- Port:%d ------\n", *port);
    bcm_esw_port_tdm_show(&config);

    cli_out("Services\n");

    for (i = 0;i < n_ports;i++) {
	cli_out("Service:%d\n", ces_ports[i]); 
    }

    cli_out("--------------------\n");
}

/**
 * cmd_esw_ces_tdm()
 *
 * Process TDM commands
 */
cmd_result_t cmd_esw_ces_tdm(int unit, args_t *args) {
    int ret = CMD_OK;
    char *arg_string_p = NULL;
    parse_table_t parse_table;

    arg_string_p = ARG_GET(args);

    if (arg_string_p == NULL) {
	return CMD_USAGE;
    }

    if (sal_strcasecmp(arg_string_p, "init") == 0)
    {
#ifndef __KERNEL__
	bcm_ces_tdm_init(unit);
#endif
	return CMD_OK;
    }
    else if (sal_strcasecmp(arg_string_p, "set") == 0)
    {
	bcm_tdm_port_config_t get_config;
	bcm_tdm_port_config_t set_config;
	int port;

	/*
	 * Default values
	 */
	port = BCM_CES_TDM_PORT_BASE_KATANA;

	/*
	 * Get the port number
	 */
	parse_table_init(unit, &parse_table);
	parse_table_add(&parse_table, "port", PQ_INT | PQ_DFL,
			0, &port, NULL);

	if (parse_arg_eq(args, &parse_table) < 0) {
	    ret = parse_fail(&parse_table, ARG_CUR(args));
	} else {
	    /*
	     * Get config for service
	     */
	    if (bcm_port_tdm_config_get(unit, port, &get_config) != BCM_E_NONE) {
		ret = CMD_FAIL;
	    } else {

		/*
		 * Defaults
		 */
		memcpy(&set_config, &get_config, sizeof(bcm_tdm_port_config_t));
    
		/*
		 * TDM set
		 */
		parse_table_init(unit, &parse_table);
		parse_table_add(&parse_table, "struct", PQ_BOOL | PQ_DFL,
				0, &set_config.b_structured, NULL);
		parse_table_add(&parse_table, "oct", PQ_BOOL | PQ_DFL,
				0, &set_config.b_octet_aligned, NULL);
		parse_table_add(&parse_table, "t1d4", PQ_BOOL | PQ_DFL,
				0, &set_config.b_T1_D4_framing, NULL);
		parse_table_add(&parse_table, "sig", PQ_BOOL | PQ_DFL,
				0, &set_config.b_signaling_enable, NULL);
		parse_table_add(&parse_table, "rxclk", PQ_INT | PQ_DFL,
				0, &set_config.e_clk_rx_select, NULL);
		parse_table_add(&parse_table, "txclk", PQ_INT | PQ_DFL,
				0, &set_config.e_clk_tx_select, NULL);
		parse_table_add(&parse_table, "casidle", PQ_HEX | PQ_DFL,
				0, &set_config.n_cas_idle_timeslots, NULL);
		parse_table_add(&parse_table, "stepsize", PQ_BOOL | PQ_DFL,
				0, &set_config.n_step_size, NULL);
		parse_table_add(&parse_table, "txcrc", PQ_BOOL | PQ_DFL,
				0, &set_config.b_txcrc, NULL);
		parse_table_add(&parse_table, "rxcrc", PQ_BOOL | PQ_DFL,
				0, &set_config.b_rxcrc, NULL);
		parse_table_add(&parse_table, "master", PQ_BOOL | PQ_DFL,
				0, &set_config.b_master, NULL);
		parse_table_add(&parse_table, "sigfmt", PQ_INT | PQ_DFL,
				0, &set_config.n_signaling_format, NULL);

		if (parse_arg_eq(args, &parse_table) < 0 || ARG_CNT(args) > 0)
		{
		    ret = parse_fail(&parse_table, ARG_CUR(args));
		}
		else
		{
		    bcm_diag_ces_tdm_set(unit, &port, &set_config);
		    ret = CMD_OK;
		}
	    }
	}

	CLEAN_UP_AND_RETURN(ret);
    }
#ifndef __KERNEL__
    else if (sal_strcasecmp(arg_string_p, "framer") == 0)
    {
	/*
	 * TDM framer commands
	 */
	if (ARG_CNT(args) >= 1)
	{
	    arg_string_p = ARG_GET(args);

	    if (sal_strcasecmp(arg_string_p, "loopback") == 0)
	    {
		int enable;
		int port;
		int loopback;

		/*
		 * Defaults
		 */
		port = BCM_CES_TDM_PORT_BASE_KATANA;
		enable = 0;
		loopback = 0;

		/*
		 * Framer loopback
		 */
		parse_table_init(unit, &parse_table);
		parse_table_add(&parse_table, "port", PQ_INT | PQ_DFL,
				0, &port, NULL);
		parse_table_add(&parse_table, "enable", PQ_INT | PQ_DFL,
				0, &enable, NULL);
		parse_table_add(&parse_table, "loopback", PQ_INT | PQ_DFL,
				0, &loopback, NULL);

		if (parse_arg_eq(args, &parse_table) < 0 || ARG_CNT(args) > 0)
		{
		    ret = parse_fail(&parse_table, ARG_CUR(args));
		}
		else
		{
		    bcm_diag_esw_ces_framer_loopback(unit, port, enable, loopback);
		    ret = CMD_OK;
		}

		CLEAN_UP_AND_RETURN(ret);
	    }
	    else if (sal_strcasecmp(arg_string_p, "status") == 0)
	    {
		int port;

		/*
		 * Defaults
		 */
		port = BCM_CES_TDM_PORT_BASE_KATANA;

		/*
		 * Framer status
		 */
		parse_table_init(unit, &parse_table);
		parse_table_add(&parse_table, "port", PQ_INT | PQ_DFL,
				0, &port, NULL);

		if (parse_arg_eq(args, &parse_table) < 0 || ARG_CNT(args) > 0)
		{
		    ret = parse_fail(&parse_table, ARG_CUR(args));
		}
		else
		{
		    bcm_diag_esw_ces_framer_status(unit, &port);
		    ret = CMD_OK;
		}

		CLEAN_UP_AND_RETURN(CMD_FAIL);
	    }
	    else if (sal_strcasecmp(arg_string_p, "pm") == 0)
	    {
		int port;

		/*
		 * Defaults
		 */
		port = BCM_CES_TDM_PORT_BASE_KATANA;

		/*
		 * Framer pm
		 */
		parse_table_init(unit, &parse_table);
		parse_table_add(&parse_table, "port", PQ_INT | PQ_DFL,
				0, &port, NULL);

		if (parse_arg_eq(args, &parse_table) < 0 || ARG_CNT(args) > 0)
		{
		    ret = parse_fail(&parse_table, ARG_CUR(args));
		}
		else
		{
		    bcm_diag_esw_ces_framer_pm(unit, &port);
		    ret = CMD_OK;
		}

		CLEAN_UP_AND_RETURN(ret);
	    }
	    else
	    {
		cli_out("Invalid CES tdm framer subcommand: %s\n", arg_string_p);
		return CMD_FAIL;
	    }
	}
	else
	{
	    cli_out("Insuficient number of arguments. Expecting one of loopback, status or pm.\n");
	    return CMD_FAIL;
	}
    }
#endif
    else if (sal_strcasecmp(arg_string_p, "cas") == 0) {
	if (ARG_CNT(args) > 0) {
	    arg_string_p = ARG_GET(args);
	    if (sal_strcasecmp(arg_string_p, "abcd") == 0) {
		int port;
		int egress;

		/*
		 * Defaults
		 */
		port = BCM_CES_TDM_PORT_BASE_KATANA;
		egress = 1;

		parse_table_init(unit, &parse_table);
		parse_table_add(&parse_table, "port", PQ_INT | PQ_DFL,
				0, &port, NULL);
		parse_table_add(&parse_table, "egress", PQ_BOOL | PQ_DFL,
				0, &egress, NULL);

		if (parse_arg_eq(args, &parse_table) < 0 || ARG_CNT(args) > 0)
		{
		    cli_out("Invalid option: %s\n", ARG_CUR(args));
		    ret = CMD_FAIL;
		} else {
		    bcm_diag_ces_tdm_cas_abcd(unit, port, egress);
		    ret = CMD_OK;
		}

		CLEAN_UP_AND_RETURN(ret);
	    } else if (sal_strcasecmp(arg_string_p, "change") == 0) {
		int port;

		/*
		 * Defaults
		 */
		port = BCM_CES_TDM_PORT_BASE_KATANA;

		parse_table_init(unit, &parse_table);
		parse_table_add(&parse_table, "port", PQ_INT | PQ_DFL,
				0, &port, NULL);

		if (parse_arg_eq(args, &parse_table) < 0 || ARG_CNT(args) > 0)
		{
		    cli_out("Invalid option: %s\n", ARG_CUR(args));
		    ret = CMD_FAIL;
		} else {
		    bcm_diag_ces_tdm_cas_change(unit, port);
		    ret = CMD_OK;
		}

		CLEAN_UP_AND_RETURN(ret);
	    } else {
		cli_out("Expecting either abcd or change\n");
	    }
	} else {
	    cli_out("Expecting either abcd or change\n");
	}
    }
    else if (sal_strcasecmp(arg_string_p, "show") == 0)
    {
	if (ARG_CNT(args) > 0) {
	    int port;

	    /*
	     * Defaults
	     */
	    port = BCM_CES_TDM_PORT_BASE_KATANA;

	    /*
	     * TDM Show
	     */
	    parse_table_init(unit, &parse_table);
	    parse_table_add(&parse_table, "port", PQ_INT | PQ_DFL,
			    0, &port, NULL);

	    if (parse_arg_eq(args, &parse_table) < 0 || ARG_CNT(args) > 0)
	    {
		cli_out("Invalid option: %s\n", ARG_CUR(args));
		ret = CMD_FAIL;
	    } else {
		bcm_diag_ces_tdm_show(unit, &port);
		ret = CMD_OK;
	    }

	    CLEAN_UP_AND_RETURN(ret);
	} else {
	    /*
	     * Summary
	     */
	    bcm_diag_ces_tdm_show_summary(unit);
	}
    }
    else
    {
	cli_out("Invalid CES tdm subcommand: %s, expecing one of init, set, framer or show\n", 
                arg_string_p);
	return CMD_FAIL;
    }

    return ret;
}

#ifndef __KERNEL__
/*********************************************************************************************
 *
 *
 *
 *
 *
 * RPC
 *
 *
 *
 *
 *
 *
 *********************************************************************************************/
static char *bcm_ces_rpc_pm_strings[] = {
    "UNEXPECTED_END_OF_PACKET", /* bit 31 */
    "IP_HEADER_CHECKSUM_ERROR", /* bit 30 */
    "UNKNOWN_PW_LABEL", /* bit 29 */
    "CES_CW_ERROR", /* bit 28 */
    "SIGNALING_PACKET", /* bit 27 */
    "AIS_DBA_PACKET", /* bit 26 */
    "RAI_R_BIT_IS_1", /* bit 25 */
    "STRICT_CHECK_RESULTS", /* bit 24 */
    "CRC_ERROR", /* bit 23 */
    "UDP_CHECKSUM_ERROR", /* bit 22 */
    "IP_LEN_ERROR", /* bit 21 */
    "UDP_LENGTH_ERROR", /* bit 20 */
    "UDP_FORWARD_HP_QUEUE", /* bit 19 */
    "UDP_FORWARD_LP_QUEUE", /* bit 18 */
    "DISABLED_CES_CHANNEL", /* bit 17 */
    "RAI_M_BITS_ARE_10", /* bit 16 */

    /* */
    /* ucode assigned flags */
    /* */
    "FOUND_PTP", /* bit 15 */
    "RTP_FLAG_ERROR", /* bit 14 */
    "HOST_DESIGNATED_PACKET", /* bit 13 */
    "MPLS_TTL_ZERO", /* bit 12 */
    "IP_ADDRESS_ERROR", /* bit 11 */
    "IP_WRONG_PROTOCOL", /* bit 10 */
    "BAD_IP_HEADER", /* bit 9 */
    "IP_NOT_UNICAST_PACKET", /* bit 8 */
    "UNKNOWN_ETHER_TYPE", /* bit 7 */
    "ETH_DESTADDR_MISMATCH", /* bit 6 */
    "ETH_NOT_UNICAST_PACKET", /* bit 5 */
    "FOUND_UDP", /* bit 4 */
    "FOUND_ECID", /* bit 3 */
    "FOUND_VLAN", /* bit 2 */
    "FOUND_IP", /* bit 1 */
    "FOUND_MPLS"  /* bit 0 */
};

/**
 * cmd_esw_ces_rpc_show_flags()
 *
 * Process RPC commands
 */
void cmd_esw_ces_rpc_show_flags(int unit, uint32 flags) {
    int i;
    uint32 bit = 0x80000000;

    if (flags) {
	for (i = 0;i < 32;i++) {
	    if (flags & bit)
		cli_out("\t0x%08x : %s\n", bit, bcm_ces_rpc_pm_strings[i]);
	    bit = bit >> 1;
	}
    } else {
	cli_out("\tPolicy does not enable any counts\n");
    }
}


/**
 * cmd_esw_ces_rpc()
 *
 * Process RPC commands
 */
cmd_result_t cmd_esw_ces_rpc(int unit, args_t *args) {
    int ret = CMD_OK;
    int n_ret;
    char *arg_string_p = NULL;
    AgNdMsgPmGlobal global;
    AgNdMsgConfigRpcPolicy policy;
    int i;
    uint32 *p;
    parse_table_t parse_table;

    arg_string_p = ARG_GET(args);

    if (arg_string_p == NULL) {
	cli_out("%s\n",  bcm_diag_cmd_esw_ces_rpc_usage);
	return CMD_OK;
    }


    if (sal_strcasecmp(arg_string_p, "set") == 0) {
	/*
	 * Set
	 */
	sal_memset(&policy, 0, sizeof(AgNdMsgConfigRpcPolicy));

	policy.x_policy_matrix.n_status_polarity     = 0xFFFFFFFF;
	policy.x_policy_matrix.n_drop_unconditional  = 0xF1F25EC0;
	policy.x_policy_matrix.n_drop_if_not_forward = 0x00000000;
	policy.x_policy_matrix.n_forward_to_ptp      = 0x00000000;
	policy.x_policy_matrix.n_forward_to_host_high = 0x00000000;
	policy.x_policy_matrix.n_forward_to_host_low  = 0x00000000;
	policy.x_policy_matrix.n_channel_counter_1 = 0x11004000;
	policy.x_policy_matrix.n_channel_counter_2 = 0x02000000;
	policy.x_policy_matrix.n_channel_counter_3 = 0x04000000;
	policy.x_policy_matrix.n_channel_counter_4 = 0x00008000;
#if 0
	policy.x_policy_matrix.n_global_counter_1 = 0x00000040;
	policy.x_policy_matrix.n_global_counter_2 = 0x00000020;
	policy.x_policy_matrix.n_global_counter_3 = 0x00000080;
	policy.x_policy_matrix.n_global_counter_4 = 0x00000800;
	policy.x_policy_matrix.n_global_counter_5 = 0x40200200;
	policy.x_policy_matrix.n_global_counter_6 = 0x00000400;
	policy.x_policy_matrix.n_global_counter_7 = 0x00500000;
	policy.x_policy_matrix.n_global_counter_8 = 0x00008000;
#endif
	policy.x_policy_matrix.n_global_counter_1 = 0xF0000000;
	policy.x_policy_matrix.n_global_counter_2 = 0x0F000000;
	policy.x_policy_matrix.n_global_counter_3 = 0x00F00000;
	policy.x_policy_matrix.n_global_counter_4 = 0x000F0000;
	policy.x_policy_matrix.n_global_counter_5 = 0x0000F000;
	policy.x_policy_matrix.n_global_counter_6 = 0x00000F00;
	policy.x_policy_matrix.n_global_counter_7 = 0x000000F0;
	policy.x_policy_matrix.n_global_counter_8 = 0x0000000F;

	parse_table_init(unit, &parse_table);
	parse_table_add(&parse_table, "gc1", PQ_INT | PQ_DFL,
			0, &policy.x_policy_matrix.n_global_counter_1, NULL);
	parse_table_add(&parse_table, "gc2", PQ_INT | PQ_DFL,
			0, &policy.x_policy_matrix.n_global_counter_2, NULL);
	parse_table_add(&parse_table, "gc3", PQ_INT | PQ_DFL,
			0, &policy.x_policy_matrix.n_global_counter_3, NULL);
	parse_table_add(&parse_table, "gc4", PQ_INT | PQ_DFL,
			0, &policy.x_policy_matrix.n_global_counter_4, NULL);
	parse_table_add(&parse_table, "gc5", PQ_INT | PQ_DFL,
			0, &policy.x_policy_matrix.n_global_counter_5, NULL);
	parse_table_add(&parse_table, "gc6", PQ_INT | PQ_DFL,
			0, &policy.x_policy_matrix.n_global_counter_6, NULL);
	parse_table_add(&parse_table, "gc7", PQ_INT | PQ_DFL,
			0, &policy.x_policy_matrix.n_global_counter_7, NULL);
	parse_table_add(&parse_table, "gc8", PQ_INT | PQ_DFL,
			0, &policy.x_policy_matrix.n_global_counter_8, NULL);
	parse_table_add(&parse_table, "du", PQ_INT | PQ_DFL,
			0, &policy.x_policy_matrix.n_drop_unconditional, NULL);

	if (parse_arg_eq(args, &parse_table) < 0 || ARG_CNT(args) > 0) {
	    ret = parse_fail(&parse_table, ARG_CUR(args));
	} else {
	    n_ret = bcm_esw_ces_rpc_pm_set(unit, &policy);
	    if (n_ret != BCM_E_NONE)
		ret = CMD_FAIL;
	}

	CLEAN_UP_AND_RETURN(ret);
    } else if (sal_strcasecmp(arg_string_p, "get") == 0) {
	/*
	 * Get
	 */
	n_ret = bcm_esw_ces_rpc_pm_get(unit, &policy, &global);
	if (n_ret != BCM_E_NONE) {
	    ret = CMD_FAIL;
	} else {
	    p = (uint32*)&policy.x_policy_matrix.n_global_counter_1;

	    cli_out("\n\n");
	    for (i = 0;i < AG_ND_RPC_CGLB_MAX;i++) {
		cli_out("Global counter%d:%d\n", (i + 1), global.a_rpc_global[i]);
		cmd_esw_ces_rpc_show_flags(unit, *p);
		p++;
	    }
	    cli_out("\n\n");
	}

    } else {
	cli_out("Expecting either get or set\n");
	ret = CMD_FAIL;
    }

    return ret;
}
#endif

/*********************************************************************************************
 *
 *
 *
 *
 *
 * MII
 *
 *
 *
 *
 *
 *
 *********************************************************************************************/

/**
 * cmd_esw_ces_mii()
 *
 * Process MII commands
 */
int cmd_esw_ces_mii_get(int unit, bcm_ces_mac_cmd_config_t *config) {
    cli_out("\n\n");
    cli_out("b_rx_error_discard_enable         : %d\n", config->b_rx_error_discard_enable);
    cli_out("b_no_length_check                 : %d\n", config->b_no_length_check);
    cli_out("b_control_frame_enable            : %d\n", config->b_control_frame_enable);
    cli_out("b_node_wake_up_request_indication : %d\n", config->b_node_wake_up_request_indication);
    cli_out("b_put_core_in_sleep_mode          : %d\n", config->b_put_core_in_sleep_mode);         
    cli_out("b_enable_magic_packet_detection   : %d\n", config->b_enable_magic_packet_detection);  
    cli_out("b_software_reset                  : %d\n", config->b_software_reset);                 
    cli_out("b_is_late_collision_condition     : %d\n", config->b_is_late_collision_condition);    
    cli_out("b_is_excessive_collision_condition: %d\n", config->b_is_excessive_collision_condition);
    cli_out("b_enable_half_duplex              : %d\n", config->b_enable_half_duplex);              
    cli_out("b_insert_mac_addr_on_transmit     : %d\n", config->b_insert_mac_addr_on_transmit);     
    cli_out("b_ignore_pause_frame_quanta       : %d\n", config->b_ignore_pause_frame_quanta);       
    cli_out("b_fwd_pause_frames                : %d\n", config->b_fwd_pause_frames);               
    cli_out("b_fwd_crc_field                   : %d\n", config->b_fwd_crc_field);                   
    cli_out("b_enable_frame_padding            : %d\n", config->b_enable_frame_padding);            
    cli_out("b_enable_promiscuous_mode         : %d\n", config->b_enable_promiscuous_mode);         
    cli_out("b_enable_mac_receive              : %d\n", config->b_enable_mac_receive);              
    cli_out("b_enable_mac_transmit             : %d\n", config->b_enable_mac_transmit);   
    cli_out("\n\n");

    return BCM_E_NONE;
}

#ifndef __KERNEL__
/**
 * cmd_esw_ces_mii_pm()
 *
 * Get MII PM counts
 */
int cmd_esw_ces_mii_pm(int unit)
{
  int ret;
  AgNdMsgPmMac stats;

  memset(&stats, 0, sizeof(AgNdMsgPmMac));
  ret = bcm_esw_ces_mac_pm_get(unit, &stats);
  if (ret != BCM_E_NONE) {
	return CMD_FAIL;
  }

  cli_out("TX Frames        : %lu\n", stats.n_frames_transmitted_ok);
  cli_out("Rx Frames        : %lu\n", stats.n_frames_received_ok);
  cli_out("CRC errors       : %lu\n", stats.n_frame_check_sequence_errors);
  cli_out("Alignment errors : %lu\n", stats.n_alignment_errors);
  cli_out("Octets Tx        : %lu\n", stats.n_octets_transmitted_ok);
  cli_out("Octets Rx        : %lu\n", stats.n_octets_received_ok);
  cli_out("Tx pause         : %lu\n", stats.n_tx_pause_mac_ctrl_frames);
  cli_out("Rx pause         : %lu\n", stats.n_rx_pause_mac_ctrl_frames);
  cli_out("If in errors     : %lu\n", stats.n_if_in_errors);
  cli_out("If out errors    : %lu\n", stats.n_if_out_errors);
  cli_out("If in ucast      : %lu\n", stats.n_if_in_ucast_pkts);
  cli_out("If in multi      : %lu\n", stats.n_if_in_multicast_pkts);
  cli_out("If in broadcast  : %lu\n", stats.n_if_in_broadcast_pkts);
  cli_out("If out discards  : %lu\n", stats.n_if_out_disacrds);
  cli_out("If out ucast     : %lu\n", stats.n_if_out_ucast_pkts);
  cli_out("If out multi     : %lu\n", stats.n_if_out_multicast_pkts);
  cli_out("If out broadcast : %lu\n", stats.n_if_out_broadcast_pkts);
  cli_out("Drop events      : %lu\n", stats.n_ether_stats_drop_events);
  cli_out("Ether octets     : %lu\n", stats.n_ether_stats_octets);
  cli_out("Ether pkts       : %lu\n", stats.n_ether_stats_pkts);
  cli_out("Ether oversize   : %lu\n", stats.n_ether_stats_oversize_pkts);
  cli_out("Ether undersize  : %lu\n", stats.n_ether_stats_undersize_pkts);
  cli_out("Ether 64         : %lu\n", stats.n_ether_stats_pkts_64_octets);
  cli_out("Ether 65-127     : %lu\n", stats.n_ether_stats_pkts_65_to_127_octets);
  cli_out("Ether 128-255    : %lu\n", stats.n_ether_stats_pkts_128_to_255_octets);
  cli_out("Ether 256-511    : %lu\n", stats.n_ether_stats_pkts_256_to_511_octets);
  cli_out("Ether 512-1023   : %lu\n", stats.n_ether_stats_pkts_512_to_1023_octets);
  cli_out("Ether 1024-1518  : %lu\n", stats.n_ether_stats_pkts_1024_to_1518_octets);

  return BCM_E_NONE;
}
#endif


/**
 * cmd_esw_ces_mii()
 *
 * Process MII commands
 */
cmd_result_t cmd_esw_ces_mii(int unit, args_t *args) {
    int ret = CMD_OK;
    char *arg_string_p = NULL;
    parse_table_t parse_table;
    bcm_ces_mac_cmd_config_t config;

    arg_string_p = ARG_GET(args);

    if (arg_string_p == NULL) {
	return CMD_USAGE;
    }

    /*
     * Get current settings
     */
    ret = bcm_ces_ethernet_config_get(unit, 0, &config);
    if (ret != BCM_E_NONE) {
	return CMD_FAIL;
    }

    if (sal_strcasecmp(arg_string_p, "pm") == 0) {
#ifndef __KERNEL__
	/*
	 * PM
	 */
	cmd_esw_ces_mii_pm(unit);
#endif
    } else if (sal_strcasecmp(arg_string_p, "get") == 0) {
	/*
	 * Get
	 */
	cmd_esw_ces_mii_get(unit, &config);

    } else if (sal_strcasecmp(arg_string_p, "set") == 0) {
	
	/*
	 * Set
	 */	
	parse_table_init(unit, &parse_table);
	parse_table_add(&parse_table, "rxedis", PQ_BOOL | PQ_DFL,
			0, &config.b_rx_error_discard_enable, NULL);
	parse_table_add(&parse_table, "nolenchk", PQ_BOOL | PQ_DFL,
			0, &config.b_no_length_check, NULL);
	parse_table_add(&parse_table, "encontfr", PQ_BOOL | PQ_DFL,
			0, &config.b_control_frame_enable, NULL);
	parse_table_add(&parse_table, "coresleep", PQ_BOOL | PQ_DFL,
			0, &config.b_put_core_in_sleep_mode, NULL);
	parse_table_add(&parse_table, "enmpktd", PQ_BOOL | PQ_DFL,
			0, &config.b_enable_magic_packet_detection, NULL);
	parse_table_add(&parse_table, "swreset", PQ_BOOL | PQ_DFL,
			0, &config.b_software_reset, NULL);
	parse_table_add(&parse_table, "enhalfd", PQ_BOOL | PQ_DFL,
			0, &config.b_enable_half_duplex, NULL);
	parse_table_add(&parse_table, "imacontx", PQ_BOOL | PQ_DFL,
			0, &config.b_insert_mac_addr_on_transmit, NULL);
	parse_table_add(&parse_table, "igpausefrq", PQ_BOOL | PQ_DFL,
			0, &config.b_ignore_pause_frame_quanta, NULL);
	parse_table_add(&parse_table, "fwdpause", PQ_BOOL | PQ_DFL,
			0, &config.b_fwd_pause_frames, NULL);
	parse_table_add(&parse_table, "fwdcrc", PQ_BOOL | PQ_DFL,
			0, &config.b_fwd_crc_field, NULL);
	parse_table_add(&parse_table, "enfrpad", PQ_BOOL | PQ_DFL,
			0, &config.b_enable_frame_padding, NULL);
	parse_table_add(&parse_table, "enprom", PQ_BOOL | PQ_DFL,
			0, &config.b_enable_promiscuous_mode, NULL);
	parse_table_add(&parse_table, "enmacrx", PQ_BOOL | PQ_DFL,
			0, &config.b_enable_mac_receive, NULL);
	parse_table_add(&parse_table, "enmactx", PQ_BOOL | PQ_DFL,
			0, &config.b_enable_mac_transmit, NULL);

	if (parse_arg_eq(args, &parse_table) < 0 || ARG_CNT(args) > 0) {
	    ret = parse_fail(&parse_table, ARG_CUR(args));
	} else {
	    ret = bcm_ces_ethernet_config_set(unit, 0, &config);
	    if (ret != BCM_E_NONE)
		ret = CMD_FAIL;
	}
	CLEAN_UP_AND_RETURN(ret);
    } else {
	cli_out("Expecting either get or set\n");
	ret = CMD_FAIL;
    }

    return ret;
}

/*********************************************************************************************
 *
 *
 *
 *
 *
 * LIU
 *
 *
 *
 *
 *
 *
 *********************************************************************************************/

/**
 * bcm_diag_esw_liu_bit_swap()
 *
 * Swap bit order on every byte
 */
void bcm_diag_esw_liu_bit_swap(uint8 *msg, int len) {
    int i;
    int bit;
    int j;
    uint8 result;

    for (j = 0;j < len;j++) {
	result = 0;
	bit = 0x80;

	for (i = 0;i < 8;i++) {
	    if (msg[j] & bit)
		result |= (1 << i);
	    bit = (bit >> 1);
	}

	msg[j] =result;
    }
}


/**
 * bcm_diag_ces_liu_bit_get()
 *
 * Get the status of a particular bit in a LIU register pair
 */
int bcm_diag_ces_liu_bit_get(int bit, uint8 high, uint8 low) {
    uint8 data = low;

    if (bit >= (BCM_DIAG_CES_MAX_LIU/2)) {
	data = high;
	bit -= (BCM_DIAG_CES_MAX_LIU/2);
    }

    return ((data & (1 << bit)) ? 1:0);
}

/**
 * bcm_diag_ces_liu_bit_set()
 *
 * Set a particular bit in a LIU register pair
 */
void bcm_diag_ces_liu_bit_set(int bit, int value, uint8 *high, uint8 *low) {
    uint8 *data = low;

    if (bit >= (BCM_DIAG_CES_MAX_LIU/2)) {
	data = high;
	bit -= (BCM_DIAG_CES_MAX_LIU/2);
    }

    if (value)
	*data |= (1 << bit);
    else
	*data &= ~(1 << bit);

    return;
}



#define BCM_LIU_BIT_WR 0
#define BCM_LIU_BIT_RD (1 << 0)

#define BCM_LIU_VERSION 0x79

/*
 * LIU registers
 */
#define BCM_LIU_REG_VERSION   0x00
#define BCM_LIU_REG_SRMS_LOW  0x00 /* Secondary */
#define BCM_LIU_REG_SRMS_HIGH 0x20 /* Secondary */
#define BCM_LIU_REG_MC        0x06 /* Individual */
#define BCM_LIU_REG_TST_LOW   0x10 /* Primary */
#define BCM_LIU_REG_TST_HIGH  0x30 /* Primary */
#define BCM_LIU_REG_TS_LOW    0x11 /* Primary */
#define BCM_LIU_REG_TS_HIGH   0x31 /* Primary */
#define BCM_LIU_REG_OE_LOW    0x12 /* Primary */
#define BCM_LIU_REG_OE_HIGH   0x32 /* Primary */
#define BCM_LIU_REG_RCLKI_LOW  0x13 /* Individual */
#define BCM_LIU_REG_RCLKI_HIGH 0x33 /* Individual */
#define BCM_LIU_REG_ADDP_LOW  0x1F /* LIUs 0-7 */
#define BCM_LIU_REG_ADDP_HIGH 0x3F /* LIUs 8-15 */
#define BCM_LIU_REG_ALBC_LOW  0x01
#define BCM_LIU_REG_ALBC_HIGH 0x21
#define BCM_LIU_REG_RLBC_LOW  0x02
#define BCM_LIU_REG_RLBC_HIGH 0x22
#define BCM_LIU_REG_LOSS_LOW  0x04
#define BCM_LIU_REG_LOSS_HIGH 0x24
#define BCM_LIU_REG_SWR_LOW   0x0A
#define BCM_LIU_REG_SWR_HIGH  0x2A
#define BCM_LIU_REG_DLBC_LOW  0x0C
#define BCM_LIU_REG_DLBC_HIGH 0x2C
#define BCM_LIU_REG_OE_LOW    0x12
#define BCM_LIU_REG_OE_HIGH   0x32
#define BCM_LIU_REG_AIS_LOW   0x13
#define BCM_LIU_REG_AIS_HIGH  0x33

#define BCM_LIU_REG_BANK_PRIMARY    0
#define BCM_LIU_REG_BANK_SECONDARY  1
#define BCM_LIU_REG_BANK_INDIVIDUAL 2
#define BCM_LIU_REG_BANK_BERT       3

uint8 bcm_diag_liu_bank[] = {0x00, 0xAA, 0x01, 0x02};
static int bcm_diag_liu_current_bank = -1;

/**
 * bcm_diag_esw_liu_write()
 *
 * Perform write
 */
int bcm_diag_esw_liu_write(int unit, bcm_diag_ces_reg_config_t *config)
{
    uint8 wmsg[32];
    uint8 rmsg[32];
    int wlen = 0;
    int rlen = 0;

    /*
     * Select LIU
     */
    soc_mspi_config(unit, MSPI_LIU, -1, -1);

    /*
     * Set bank 
     */
    if (bcm_diag_liu_current_bank != config->bank) {
      /*
       * Set low bank select register
       */ 
	wmsg[wlen] = BCM_LIU_BIT_WR | ((BCM_LIU_REG_ADDP_LOW << 1) & 0xFE);
	wlen++;
	wmsg[wlen] =  bcm_diag_liu_bank[config->bank]; 
	wlen++;
	/* 
	 *Set high bank select register
	 */
	wmsg[wlen] = BCM_LIU_BIT_WR | ((BCM_LIU_REG_ADDP_HIGH << 1) & 0xFE);
	wlen++;
	wmsg[wlen] =  bcm_diag_liu_bank[config->bank]; 
	wlen++;
	/*
	 * Save current bank select
	 */

	bcm_diag_liu_current_bank = config->bank;
    }

    /*
     * Write register
     */
    wmsg[wlen] = BCM_LIU_BIT_WR | ((config->address << 1) & 0xFE);
    wlen++;
    wmsg[wlen] = config->value;
    wlen++;

    /*
     * Do it to it
     */
    bcm_diag_esw_liu_bit_swap(wmsg, wlen);
    soc_mspi_write8(unit, wmsg, wlen);

    /*
     * Read back to make sure that it stuck.
     */
    wlen = 0;
    wmsg[wlen] = BCM_LIU_BIT_RD | ((config->address << 1) & 0xFE);
    wlen++;
    bcm_diag_esw_liu_bit_swap(wmsg, wlen);
    rlen = 1;
    soc_mspi_writeread8(unit, wmsg, wlen, rmsg, rlen);
    bcm_diag_esw_liu_bit_swap(rmsg, rlen);

    /*
     * Check
     */
    if (config->value != rmsg[0])
    {
	config->value = rmsg[0];
	return -1;
    }

    config->value = rmsg[0];
    return 0;
}


/**
 * bcm_diag_esw_liu_read()
 *
 * Perform read
 */
int bcm_diag_esw_liu_read(int unit, bcm_diag_ces_reg_config_t *config)
{
    uint8 wmsg[32];
    uint8 rmsg[32];
    int wlen = 0;
    int rlen = 0;

    /*
     * Select LIU
     */
    soc_mspi_config(unit, MSPI_LIU, -1, -1);

    /*
     * Set bank 
     */
    if (bcm_diag_liu_current_bank != config->bank) {
	wmsg[wlen] = BCM_LIU_BIT_WR | ((BCM_LIU_REG_ADDP_LOW << 1) & 0xFE);
	wlen++;
	wmsg[wlen] =  bcm_diag_liu_bank[config->bank]; 
	wlen++;

	wmsg[wlen] = BCM_LIU_BIT_WR | ((BCM_LIU_REG_ADDP_HIGH << 1) & 0xFE);
	wlen++;
	wmsg[wlen] =  bcm_diag_liu_bank[config->bank]; 
	wlen++;

	bcm_diag_liu_current_bank = config->bank;
    }

    /*
     * Read reg
     */
    wmsg[wlen] = BCM_LIU_BIT_RD | ((config->address << 1) & 0xFE);
    wlen++;

    bcm_diag_esw_liu_bit_swap(wmsg, wlen);
    rlen = 1;
    soc_mspi_writeread8(unit, wmsg, wlen, rmsg, rlen);
    bcm_diag_esw_liu_bit_swap(rmsg, rlen);
    config->value = rmsg[0];


    return config->value;
}

/**
 * bcm_diag_esw_liu_reset()
 *
 * Software reset
 */
int bcm_diag_esw_ces_liu_reset(int unit) {
    bcm_diag_ces_reg_config_t config;

    /*
     * Write (any value) to SWR registers
     */
    config.bank = BCM_LIU_REG_BANK_PRIMARY;

    config.address = BCM_LIU_REG_SWR_LOW;
    config.value = 0x00;
    bcm_diag_esw_liu_write(unit, &config);

    config.address = BCM_LIU_REG_SWR_HIGH;
    config.value = 0x00;
    bcm_diag_esw_liu_write(unit, &config);

    return BCM_E_NONE;
}

/**
 * bcm_diag_esw_liu_init()
 *
 * Init LIU for T1 or E1
 */
int bcm_diag_esw_ces_liu_init(int unit, int t1) {
    bcm_diag_ces_reg_config_t config;
    int i;

    /*
     * Init
     */
    bcm_diag_esw_ces_liu_reset(unit);
    sal_sleep(1);

    /*
     * Check that LIU is present and operational
     */
    config.bank    = BCM_LIU_REG_BANK_PRIMARY;
    config.address = BCM_LIU_REG_VERSION;
    config.value   = 0x00;
    bcm_diag_esw_liu_read(unit, &config);

    if (config.value != BCM_LIU_VERSION) {
	return BCM_E_INTERNAL;
    }

    /*
     * Single rail selection
     */
    config.bank    = BCM_LIU_REG_BANK_SECONDARY;
    config.address = BCM_LIU_REG_SRMS_LOW;
    config.value   = 0xFF;
    bcm_diag_esw_liu_write(unit, &config);

    config.bank    = BCM_LIU_REG_BANK_SECONDARY;
    config.address = BCM_LIU_REG_SRMS_HIGH;
    config.value   = 0xFF;
    bcm_diag_esw_liu_write(unit, &config);

    /*
     * Output enable
     */
    config.bank    = BCM_LIU_REG_BANK_PRIMARY;
    config.address = BCM_LIU_REG_OE_LOW;
    config.value   = 0xFF;
    bcm_diag_esw_liu_write(unit, &config);

    config.bank    = BCM_LIU_REG_BANK_PRIMARY;
    config.address = BCM_LIU_REG_OE_HIGH;
    config.value   = 0xFF;
    bcm_diag_esw_liu_write(unit, &config);

    /*
     * Per port template config
     */
    for (i = 0;i < 16;i++) {
	if (i < 8) {
	     config.bank    = BCM_LIU_REG_BANK_PRIMARY;
	     config.address = BCM_LIU_REG_TST_LOW;
	     config.value   = 0x80 | i;
	     bcm_diag_esw_liu_write(unit, &config);

	     config.bank    = BCM_LIU_REG_BANK_PRIMARY;
	     config.address = BCM_LIU_REG_TS_LOW;
	     if (t1)
		 config.value   = 0x87;
	     else
		 config.value   = 0x88;
	     bcm_diag_esw_liu_write(unit, &config);

	} else {
	    config.bank    = BCM_LIU_REG_BANK_PRIMARY;
	     config.address = BCM_LIU_REG_TST_HIGH;
	     config.value   = 0x80 | (i - 8);
	     bcm_diag_esw_liu_write(unit, &config);

	     config.bank    = BCM_LIU_REG_BANK_PRIMARY;
	     config.address = BCM_LIU_REG_TS_HIGH;
	     if (t1)
		 config.value   = 0x87;
	     else
		 config.value   = 0x88;

	     bcm_diag_esw_liu_write(unit, &config);
	}

	
    }

    /*
     * Master clock select
     */
    config.bank    = BCM_LIU_REG_BANK_INDIVIDUAL;
    config.address = BCM_LIU_REG_MC;
    config.value   = 0x00;
    bcm_diag_esw_liu_write(unit, &config);

    /*
     * Receive clock invert
     */
    config.bank    = BCM_LIU_REG_BANK_INDIVIDUAL;
    config.address = BCM_LIU_REG_RCLKI_LOW;
    config.value   = 0xFF;
    bcm_diag_esw_liu_write(unit, &config);

    config.bank    = BCM_LIU_REG_BANK_INDIVIDUAL;
    config.address = BCM_LIU_REG_RCLKI_HIGH;
    config.value   = 0xFF;
    bcm_diag_esw_liu_write(unit, &config);

    return BCM_E_NONE;
}


/**
 * bcm_diag_esw_liu_status()
 *
 * Display LIU status
 */
int bcm_diag_esw_ces_liu_status(int unit) {
    int i;
    bcm_diag_ces_reg_config_t config;
    uint8 loss_low;
    uint8 loss_high;
    uint8 oe_low;
    uint8 oe_high;
    uint8 ais_low;
    uint8 ais_high;
    uint8 version;

    /*
     * Read version
     */
    config.bank = BCM_LIU_REG_BANK_PRIMARY;
    config.address = BCM_LIU_REG_VERSION;
    version = bcm_diag_esw_liu_read(unit, &config);

    /*
     * Read LOSS registers
     */
    config.bank = BCM_LIU_REG_BANK_PRIMARY;

    config.address = BCM_LIU_REG_LOSS_LOW;
    config.value = 0x00;
    loss_low = bcm_diag_esw_liu_read(unit, &config);

    config.address = BCM_LIU_REG_LOSS_HIGH;
    config.value = 0x00;
    loss_high = bcm_diag_esw_liu_read(unit, &config);

    /*
     * Output enable
     */
    config.address = BCM_LIU_REG_OE_LOW;
    config.value = 0x00;
    oe_low = bcm_diag_esw_liu_read(unit, &config);

    config.address = BCM_LIU_REG_OE_HIGH;
    config.value = 0x00;
    oe_high = bcm_diag_esw_liu_read(unit, &config);

    /*
     * AIS status
     */
    config.address = BCM_LIU_REG_AIS_LOW;
    config.value = 0x00;
    ais_low = bcm_diag_esw_liu_read(unit, &config);

    config.address = BCM_LIU_REG_AIS_HIGH;
    config.value = 0x00;
    ais_high = bcm_diag_esw_liu_read(unit, &config);


    /*
     * Display the results
     */
    cli_out("Version:0x%02x\n\n", version);
    cli_out("Port LOSS OE AIS\n");
    cli_out("---- ---- -- ---\n");

    for (i = 0;i < BCM_DIAG_CES_MAX_LIU;i++)
    {
	cli_out("%4d %4c %2c %3c\n",
                i,
                (bcm_diag_ces_liu_bit_get(i, loss_high, loss_low) ? 'X':'-'),
                (bcm_diag_ces_liu_bit_get(i, oe_high, oe_low) ? 'X':'-'), 
                (bcm_diag_ces_liu_bit_get(i, ais_high, ais_low) ? 'X':'-')); 
    }

    return BCM_E_NONE;
}


/**
 * bcm_diag_esw_liu_enable()
 *
 * Enable/Disable LIU ports
 */
int bcm_diag_esw_ces_liu_enable(int unit, bcm_diag_ces_loopback_config_t *loopback_config) {
    bcm_diag_ces_reg_config_t config;
    uint8 oe_low = 0;
    uint8 oe_high = 0;

    if (loopback_config->port == -1) {
	config.bank = 0;
	config.address = BCM_LIU_REG_OE_LOW;
	config.value = (loopback_config->enable ? 0xFF:0x00);
	bcm_diag_esw_liu_write(unit, &config);

	config.bank = 0;
	config.address = BCM_LIU_REG_OE_HIGH;
	config.value = (loopback_config->enable ? 0xFF:0x00);
	bcm_diag_esw_liu_write(unit, &config);
    } else {
	config.bank = 0;
	config.address = (loopback_config->port < 8 ? BCM_LIU_REG_OE_LOW:BCM_LIU_REG_OE_HIGH);
	bcm_diag_ces_liu_bit_set(loopback_config->port, 
				 loopback_config->enable, 
				 &oe_high,
				 &oe_low);
	config.value = (loopback_config->port < 8 ? oe_low:oe_high); 
	bcm_diag_esw_liu_write(unit, &config);
    }

    return BCM_E_NONE;
}



/**
 * bcm_diag_esw_liu_loopback_get()
 *
 * Get current loopback settings
 */
int bcm_diag_esw_ces_liu_loopback_get(int unit, bcm_diag_ces_loopback_config_t *loopback_config) {
    int ret = BCM_E_NONE;
    int i;
    uint8 albc_low = 0;
    uint8 albc_high = 0;
    uint8 rlbc_low = 0;
    uint8 rlbc_high = 0;
    uint8 dlbc_low = 0;
    uint8 dlbc_high = 0;
    bcm_diag_ces_reg_config_t config;


    /*
     * Read the ALBC, RLBC and DLBC registers
     */
    config.bank = BCM_LIU_REG_BANK_PRIMARY;

    config.address = BCM_LIU_REG_ALBC_LOW;
    config.value = 0x00;
    albc_low = bcm_diag_esw_liu_read(unit, &config);

    config.address = BCM_LIU_REG_ALBC_HIGH;
    config.value = 0x00;
    albc_high = bcm_diag_esw_liu_read(unit, &config);

    config.address = BCM_LIU_REG_RLBC_LOW;
    config.value = 0x00;
    rlbc_low = bcm_diag_esw_liu_read(unit, &config);

    config.address = BCM_LIU_REG_RLBC_HIGH;
    config.value = 0x00;
    rlbc_high = bcm_diag_esw_liu_read(unit, &config);

    config.address = BCM_LIU_REG_DLBC_LOW;
    config.value = 0x00;
    dlbc_low = bcm_diag_esw_liu_read(unit, &config);

    config.address = BCM_LIU_REG_DLBC_HIGH;
    config.value = 0x00;
    dlbc_high = bcm_diag_esw_liu_read(unit, &config);

    /*
     * Display the result
     */
    cli_out("     +---- Loopback -----+\n");
    cli_out("Port Analog Remote Digital\n");
    cli_out("---- ------ ------ -------\n");

    for (i = 0;i < BCM_DIAG_CES_MAX_LIU;i++)
    {
	cli_out("%4d %6c %6c %7c\n",
                i,
                (bcm_diag_ces_liu_bit_get(i, albc_high, albc_low) ? 'X':'-'),
                (bcm_diag_ces_liu_bit_get(i, rlbc_high, rlbc_low) ? 'X':'-'), 
                (bcm_diag_ces_liu_bit_get(i, dlbc_high, dlbc_low) ? 'X':'-')); 
    }

    return ret;
}


/**
 * bcm_diag_esw_liu_loopback_set()
 *
 * Process LIU commands
 */
int bcm_diag_esw_ces_liu_loopback_set(int unit, bcm_diag_ces_loopback_config_t *loopback_config) {
    int ret = BCM_E_NONE;
    uint8 lbc_low = 0x00;
    uint8 lbc_high = 0x00;
    bcm_diag_ces_reg_config_t config;

    /*
     * If port is -1 and type is 0 then clear all loopbacks
     */
    if (loopback_config->port == -1 && loopback_config->type == NULL) {
	/*
	 * Write 0x00 to ALBC, RLBC and DLBC register pairs
	 */
	config.bank = BCM_LIU_REG_BANK_PRIMARY;

	config.address = BCM_LIU_REG_ALBC_LOW;
	config.value = 0x00;
	bcm_diag_esw_liu_write(unit, &config);

	config.address = BCM_LIU_REG_ALBC_HIGH;
	config.value = 0x00;
	bcm_diag_esw_liu_write(unit, &config);

	config.address = BCM_LIU_REG_RLBC_LOW;
	config.value = 0x00;
	bcm_diag_esw_liu_write(unit, &config);

	config.address = BCM_LIU_REG_RLBC_HIGH;
	config.value = 0x00;
	bcm_diag_esw_liu_write(unit, &config);

	config.address = BCM_LIU_REG_DLBC_LOW;
	config.value = 0x00;
	bcm_diag_esw_liu_write(unit, &config);

	config.address = BCM_LIU_REG_DLBC_HIGH;
	config.value = 0x00;
	bcm_diag_esw_liu_write(unit, &config);
    }
    else
    {
	config.bank = BCM_LIU_REG_BANK_PRIMARY;
	
	/*
	 * Read appropriate loopback register
	 */
	switch ((int)(*loopback_config->type)) {
	case 'a': /* Analog */
	    config.address = (loopback_config->port < 8 ? BCM_LIU_REG_ALBC_LOW:BCM_LIU_REG_ALBC_HIGH);
	    break;

	case 'd': /* Digital */
	    config.address = (loopback_config->port < 8 ? BCM_LIU_REG_DLBC_LOW:BCM_LIU_REG_DLBC_HIGH);
	    break;

	case 'r': /* Remote */
	    config.address = (loopback_config->port < 8 ? BCM_LIU_REG_RLBC_LOW:BCM_LIU_REG_RLBC_HIGH);
	    break;

	default:
	    cli_out("Invalid loopback type:%s, expecting either a|r|d\n", loopback_config->type);
	    return BCM_E_INTERNAL;
	}

	config.value = bcm_diag_esw_liu_read(unit, &config);

	/*
	 * Set or clear the appropriate bit(s) and write
	 */
	if (loopback_config->port == -1) {
	    lbc_low = 0xFF;
	    lbc_high = 0xFF;
	} else {
	    bcm_diag_ces_liu_bit_set(loopback_config->port, 
				     loopback_config->enable, 
				     &lbc_high,
				     &lbc_low);
	}

	if (loopback_config->port == -1) {
	    if (loopback_config->enable)
		config.value  |= lbc_low;
	    else
		config.value  &= ~lbc_low;

	    bcm_diag_esw_liu_write(unit, &config);
	    config.address += 0x20;

	    if (loopback_config->enable)
		config.value  |= lbc_high;
	    else
		config.value  &= ~lbc_high;

	    bcm_diag_esw_liu_write(unit, &config);

	} else {
	    if (loopback_config->enable)
		config.value  |= (loopback_config->port < 8 ? lbc_low:lbc_high);
	    else
		config.value  &= ~(loopback_config->port < 8 ? lbc_low:lbc_high);

	
	    /*
	     * Write back
	     */
	    bcm_diag_esw_liu_write(unit, &config);
	}
    }

    return ret;
}

/**
 * bcm_diag_esw_liu_reg_check()
 *
 * Validate register address
 */
int bcm_diag_esw_ces_liu_reg_check(int unit, bcm_diag_ces_reg_config_t *config) {

    /*
     * Validate bank
     */
    if (config->bank < 0 || config->bank > 3) {
	cli_out("Inavlid bank value:%d, valid values either 0=Primary, 1=Secondary, 2=Individual, 3=BERT\n", config->bank);
	return 0;
    }

    /*
     * Validate LIU
     */
    if (config->liu < 0 || config->liu > 15) {
	cli_out("Invalid LIU:%d, valid values are between 0 and 15\n", config->liu);
	return 0;
    }

    /*
     * Validate value
     */
    if (config->value > 0xFF) {
	cli_out("Invalid write value:0x%x, valid values are between 0x00 and 0xFF\n", config->value);
	return 0;
    }

    /*
     * Validate address
     */
    if (config->bank == 0) {
	if ((config->address >= 0x16 && config->address <= 0x1E) ||
	    (config->address >= 0x36 && config->address <= 0x3E) ||
	    config->address > 0x3F) {
	    cli_out("Invalid address:0x%x for bank:%d, valid values are between 0x00 and 0x15 or 0x20 and 0x35\n",
                    config->address,
                    config->bank);
	    return 0;
	}
    } else if (config->bank == 1) {
	if ((config->address >= 0x07 && config->address <=0x1E) ||
	    (config->address >= 0x27 && config->address <=0x3E) ||
	    config->address > 0x3F) {
	    cli_out("Invalid address:0x%x for bank:%d, valid values are between 0x00 and 0x07 or 0x20 and 0x27\n",
                    config->address,
                    config->bank);
	    return 0;
	}
    } else if (config->bank == 2) {
	if (config->address > 0x3F) {
	    cli_out("Invalid address:0x%x for bank:%d, valid values are between 0x00 and 0x07 or 0x20 and 0x27\n",
                    config->address,
                    config->bank);
	    return 0;
	}
    }


    return 1;
}


/**
 * bcm_diag_esw_liu_reg_read()
 *
 * Process LIU commands
 */
int bcm_diag_esw_ces_liu_reg_read(int unit, bcm_diag_ces_reg_config_t *config) {
    int ret = BCM_E_NONE;

    /*
     * Validate parameters.
     */
    if (!bcm_diag_esw_ces_liu_reg_check(unit, config))
	return BCM_E_PARAM;

    /*
     * Select LIU
     */
    /*soc_mspi_config(unit, MSPI_LIU, -1, -1);*/

    /*
     * Read
     */
    bcm_diag_esw_liu_read(unit, config);

    /*
     * Display the result
     */
    cli_out("\n\rBank:%d Address:0x%02x Data:0x%02x\n\r",
            config->bank,
            config->address,
            config->value);

    return ret;
}


/**
 * bcm_diag_esw_liu_reg_write()
 *
 * Process LIU commands
 */
int bcm_diag_esw_ces_liu_reg_write(int unit, bcm_diag_ces_reg_config_t *config) {
    int ret = BCM_E_NONE;

    /*
     * Validate parameters.
     */
    if (!bcm_diag_esw_ces_liu_reg_check(unit, config))
	return BCM_E_PARAM;

    /*
     * Select LIU
     */
    /*soc_mspi_config(unit, MSPI_LIU, -1, -1);*/

    if (bcm_diag_esw_liu_write(unit, config) < 0) {
	cli_out("Error: Incorrect value read back from device\n");
    } else {
    /*
     * Display the result
     */
    cli_out("\n\rBank:%d Address:0x%02x Data:0x%02x\n\r",
            config->bank,
            config->address,
            config->value);
    }

    return ret;
}



/**
 * cmd_esw_ces_liu()
 *
 * Process LIU commands
 */
cmd_result_t cmd_esw_ces_liu(int unit, args_t *args) {
    int ret = CMD_OK;
    char *arg_string_p = NULL;
    parse_table_t parse_table;
    bcm_diag_ces_loopback_config_t loopback_config;

    arg_string_p = ARG_GET(args);

    if (arg_string_p == NULL) {
	return CMD_USAGE;
    }

    if (sal_strcasecmp(arg_string_p, "loopback") == 0) {
	/*
	 * LIU loopback configuration
	 */
	if (ARG_CNT(args) >= 1) {

	    arg_string_p = ARG_GET(args);

	    if (sal_strcasecmp(arg_string_p, "get") == 0) {
		ret = bcm_diag_esw_ces_liu_loopback_get(unit, &loopback_config);

		if (BCM_FAILURE(ret)) {
		    cli_out("Command failed.  %s.\n", bcm_errmsg(ret));
		    return CMD_FAIL;
		}
	    } else if (sal_strcasecmp(arg_string_p, "set") == 0) {
		ret = CMD_OK;

		/*
		 * Set default values
		 */
		loopback_config.port = -1;
		loopback_config.type = NULL;
		loopback_config.enable = 0;

		parse_table_init(unit, &parse_table);
		parse_table_add(&parse_table, "port", PQ_INT | PQ_DFL,
				0, &loopback_config.port, NULL);
		parse_table_add(&parse_table, "type", PQ_STRING,
				"a", &loopback_config.type, NULL);
		parse_table_add(&parse_table, "enable", PQ_BOOL | PQ_DFL,
				0, &loopback_config.enable, NULL);

		if (parse_arg_eq(args, &parse_table) < 0 || ARG_CNT(args) > 0) {
		    ret = parse_fail(&parse_table, ARG_CUR(args));
		} else {
		    ret = bcm_diag_esw_ces_liu_loopback_set(unit, &loopback_config);
		    if (BCM_FAILURE(ret)) {
			cli_out("Command failed.  %s.\n", bcm_errmsg(ret));
			ret = CMD_FAIL;
		    }
		}
		CLEAN_UP_AND_RETURN(ret);
	    } else {
		cli_out("Invalid CES LIU loopback subcommand: %s, expecting either get|set\n", arg_string_p);
		return CMD_FAIL;
	    }
	} else {
	    cli_out("Missing loopback subcommand, expecting either get|set\n");
	    return CMD_FAIL;
	}
    } else if (sal_strcasecmp(arg_string_p, "status") == 0) {
        /*
	 * Status
	 */
	ret =  bcm_diag_esw_ces_liu_status(unit);
	if (BCM_FAILURE(ret)) {
	    cli_out("Command failed.  %s.\n", bcm_errmsg(ret));
	    ret = CMD_FAIL;
	}

	return ret;

    } else if (sal_strcasecmp(arg_string_p, "enable") == 0) {
        /*
	 * Enable
	 */
	loopback_config.port = -1;
	loopback_config.enable = 0;

	parse_table_init(unit, &parse_table);
	parse_table_add(&parse_table, "port", PQ_INT | PQ_DFL,
			0, &loopback_config.port, NULL);
	parse_table_add(&parse_table, "enable", PQ_INT | PQ_DFL,
			0, &loopback_config.enable, NULL);

	if (parse_arg_eq(args, &parse_table) < 0 || ARG_CNT(args) > 0) {
	    ret = parse_fail(&parse_table, ARG_CUR(args));
	} else {
	    ret =  bcm_diag_esw_ces_liu_enable(unit, &loopback_config);

	    if (BCM_FAILURE(ret)) {
		cli_out("Command failed.  %s.\n", bcm_errmsg(ret));
		ret = CMD_FAIL;
	    }
	}
	CLEAN_UP_AND_RETURN(ret);

    } else if (sal_strcasecmp(arg_string_p, "reset") == 0) {
	/*
	 * Reset
	 */
	ret =  bcm_diag_esw_ces_liu_reset(unit);
	if (BCM_FAILURE(ret)) {
	    cli_out("Command failed.  %s.\n", bcm_errmsg(ret));
	    ret = CMD_FAIL;
	}

	return ret;
    } else if (sal_strcasecmp(arg_string_p, "init") == 0) {
	/*
	 * Init
	 */	
	int t1 = 1;

	parse_table_init(unit, &parse_table);
	parse_table_add(&parse_table, "t1", PQ_BOOL | PQ_DFL,
			0, &t1, NULL);

	if (parse_arg_eq(args, &parse_table) < 0 || ARG_CNT(args) > 0) {
	    ret = parse_fail(&parse_table, ARG_CUR(args));
	} else {
	    ret =  bcm_diag_esw_ces_liu_init(unit, t1);
	    if (BCM_FAILURE(ret)) {
		cli_out("Command failed.  %s.\n", bcm_errmsg(ret));
		ret = CMD_FAIL;
	    }
	}

	CLEAN_UP_AND_RETURN(ret);

    } else if (sal_strcasecmp(arg_string_p, "reg") == 0) {
	/*
	 * LIU register set|set
	 */
	if (ARG_CNT(args) >= 1) {
	    bcm_diag_ces_reg_config_t reg_config;
	    reg_config.address = 0;
	    reg_config.value   = 0;
	    reg_config.bank    = 0;
	    reg_config.liu     = 0;

	    arg_string_p = ARG_GET(args);

	    if (sal_strcasecmp(arg_string_p, "read") == 0) {

		parse_table_init(unit, &parse_table);
		parse_table_add(&parse_table, "address", PQ_HEX | PQ_DFL,
				0, &reg_config.address, NULL);
		parse_table_add(&parse_table, "bank", PQ_INT | PQ_DFL,
				0, &reg_config.bank, NULL);
		parse_table_add(&parse_table, "liu", PQ_INT | PQ_DFL,
				0, &reg_config.liu, NULL);
	
		if (parse_arg_eq(args, &parse_table) < 0 || ARG_CNT(args) > 0) {
		    ret = parse_fail(&parse_table, ARG_CUR(args));
		} else {
		    ret = bcm_diag_esw_ces_liu_reg_read(unit, &reg_config);

		    if (BCM_FAILURE(ret)) {
			cli_out("Command failed.  %s.\n", bcm_errmsg(ret));
			return CMD_FAIL;
		    }
		}
	    } else if (sal_strcasecmp(arg_string_p, "write") == 0) {
		ret = CMD_OK;

		parse_table_init(unit, &parse_table);
		parse_table_add(&parse_table, "address", PQ_HEX | PQ_DFL,
				0, &reg_config.address, NULL);
		parse_table_add(&parse_table, "bank", PQ_INT | PQ_DFL,
				0, &reg_config.bank, NULL);
		parse_table_add(&parse_table, "liu", PQ_INT | PQ_DFL,
				0, &reg_config.liu, NULL);
		parse_table_add(&parse_table, "value", PQ_HEX | PQ_DFL,
				0, &reg_config.value, NULL);

		if (parse_arg_eq(args, &parse_table) < 0 || ARG_CNT(args) > 0) {
		    ret = parse_fail(&parse_table, ARG_CUR(args));
		} else {
		    ret = bcm_diag_esw_ces_liu_reg_write(unit, &reg_config);
		    if (BCM_FAILURE(ret)) {
			cli_out("Command failed.  %s.\n", bcm_errmsg(ret));
			ret = CMD_FAIL;
		    }
		}
		CLEAN_UP_AND_RETURN(ret);
	    } else {
		cli_out("Invalid CES LIU reg subcommand: %s, expecting either read|write\n", arg_string_p);
		return CMD_FAIL;
	    }
	} else {
	    cli_out("Missing CES LIU reg subcommand, expecting either read|write\n");
	    return CMD_FAIL;
	}

    } else {
	cli_out("Invalid CES LIU subcommand: %s, expecting either reg, loopback, status, init or reset\n", arg_string_p);
	return CMD_FAIL;
    }

    return CMD_OK;
}


/*********************************************************************************************
 *
 *
 *
 *
 *
 * Diag
 *
 *
 *
 *
 *
 *
 *********************************************************************************************/
#ifndef __KERNEL__
char *bcm_diag_ces_diag_loopback_string[] = {
    "None",
    "TDM Remote",
    "TDM Local",
    "Channelizer",
    "Dechannelizer",
    "Classifier",
    "Packet interface",
    "Packet local",
    "packet remote"};

/**
 * bcm_diag_esw_ces_diag_get()
 *
 * Process CES diag commands
 */
int bcm_diag_esw_ces_diag_get(int unit) {
    AgNdMsgDiag diag_config;
    int ret;

    ret = bcm_ces_diag_get(unit, &diag_config);

    if (ret == BCM_E_NONE) {
	cli_out("---- CES Diag ----\n");
	cli_out("               e_loopback: %u - %s\n",diag_config.e_loopback,
                bcm_diag_ces_diag_loopback_string[diag_config.e_loopback]);
	cli_out("n_loopback_channel_select: %lu\n", diag_config.n_loopback_channel_select);
	cli_out("   n_loopback_port_select: %lu\n", diag_config.n_loopback_port_select);
	cli_out("    -- Activity Indicators --\n");
	cli_out("        b_act_1_sec_pulse: %lu\n", diag_config.b_act_1_sec_pulse);
	cli_out("      b_act_nomad_brg_clk: %lu\n", diag_config.b_act_nomad_brg_clk);
	cli_out("           b_act_miit_clk: %lu\n", diag_config.b_act_miit_clk);
	cli_out("           b_act_miir_clk: %lu\n", diag_config.b_act_miir_clk);
	cli_out("           b_act_ref_clk1: %lu\n", diag_config.b_act_ref_clk1);
	cli_out("           b_act_ref_clk2: %lu\n", diag_config.b_act_ref_clk2);
	cli_out("       b_act_ext_clk_sync: %lu\n", diag_config.b_act_ext_clk_sync);
	cli_out("           b_act_int_cclk: %lu\n", diag_config.b_act_int_cclk);
	cli_out("               n_act_port: 0x%08x (Bit per TDM port)\n", diag_config.n_act_port);
	cli_out("    -------------------------\n");
	cli_out("             e_oscillator: %u - %s\n", diag_config.e_oscillator,
                (diag_config.e_oscillator ? "TCXO":"OCXO"));
    }

    return ret;
}

/**
 * cmd_esw_ces_diag()
 *
 * Process CES diag commands
 */
cmd_result_t cmd_esw_ces_diag(int unit, args_t *args) {
    int ret = CMD_OK;
    char *arg_string_p = NULL;
    parse_table_t parse_table;

    arg_string_p = ARG_GET(args);

    if (arg_string_p == NULL) {
	return CMD_USAGE;
    }

    if (sal_strcasecmp(arg_string_p, "get") == 0) {
	/*
	 * Get 
	 */
	ret = bcm_diag_esw_ces_diag_get(unit);
	
	if (BCM_FAILURE(ret)) {
	    cli_out("Command failed.  %s.\n", bcm_errmsg(ret));
	}
    } else if (sal_strcasecmp(arg_string_p, "set") == 0) {

	if (ARG_CNT(args) > 0) {
	    AgNdMsgDiag diag_config;

	    /*
	     * Set default values
	     */
	    diag_config.e_loopback = bcmCesLoopbackNone;
	    diag_config.n_loopback_channel_select = 0;
	    diag_config.n_loopback_port_select = BCM_CES_TDM_PORT_BASE_KATANA;

	    parse_table_init(unit, &parse_table);
	    parse_table_add(&parse_table, "port", PQ_INT | PQ_DFL,
			    0, &diag_config.n_loopback_port_select, NULL);
	    parse_table_add(&parse_table, "service", PQ_INT | PQ_DFL,
			    0, &diag_config.n_loopback_channel_select, NULL);
	    parse_table_add(&parse_table, "loopback", PQ_INT | PQ_DFL,
			    0, &diag_config.e_loopback, NULL);

	    if (parse_arg_eq(args, &parse_table) < 0 || ARG_CNT(args) > 0) {
		ret = parse_fail(&parse_table, ARG_CUR(args));
	    } else {
		ret = bcm_ces_diag_set(unit, &diag_config);
		if (BCM_FAILURE(ret)) {
		    cli_out("Command failed.  %s.\n", bcm_errmsg(ret));
		    ret = CMD_FAIL;
		}
	    }
	    CLEAN_UP_AND_RETURN(ret);
	} else {
	    cli_out("Expecting args\n");
	    return CMD_FAIL;
	}
    } else {
	cli_out("Missing diag subcommand, expecting either get|set\n");
	return CMD_FAIL;
    }

    return CMD_OK;
}


/*********************************************************************************************
 *
 *
 *
 *
 *
 * Test
 *
 *
 *
 *
 *
 *
 *********************************************************************************************/
#define BCM_CES_TEST_LOCK_LOOP_COUNT  10
#define BCM_CES_TEST_LOCK_STABLE_COUNT 3
/**
 * cmd_esw_ces_test_system()
 *
 * Process CES test commands
 */
cmd_result_t bcm_diag_esw_ces_test_system(int unit, int time, uint8 *ports, int num_ports) {
    int i,j;
    bcm_diag_ces_loopback_config_t loopback_config;
    int status;
    int result;
    int port;
    int lowDisable = 0;
    uint32 rval;
    bcm_ces_channel_map_t *map_config;
    int lock_stable;

    map_config = (bcm_ces_channel_map_t *)sal_alloc(sizeof(bcm_ces_channel_map_t), "CES channel map");
    if (map_config == NULL)
        return BCM_E_MEMORY;


    /*
     * Init ces
     */
    cli_out("Initializing CES (This may take a few seconds)\n");
    result = bcm_ces_services_init(unit);

    if (BCM_FAILURE(result)) {
	cli_out("Command failed.  %s.\n", bcm_errmsg(result));
        sal_free(map_config);
	return CMD_FAIL;
    }

    /*
     * Init LIU and put ports into analog loopback
     */
    cli_out("Initializing LIUs\n");
    if (bcm_diag_esw_ces_liu_init(unit, 1)!= BCM_E_NONE) { /* T1 mode */ 
	cli_out("LIU device failed to initialize\n");
        sal_free(map_config);
	return CMD_FAIL;
    }

    cli_out("Setting GE0 line loopback\n");
    READ_COMMAND_CONFIGr(unit, 1, &rval);
    soc_reg_field_set(unit, COMMAND_CONFIGr, &rval, LINE_LOOPBACKf, 1);
    WRITE_COMMAND_CONFIGr(unit, 1, rval);

    /*
     * Setup sixteen CES services
     */
    cli_out("Creating CES services\n");
    memset(map_config, 0, sizeof(bcm_ces_channel_map_t));
    map_config->first = 0;
    map_config->size  = 1;

    for (i = 0;i < num_ports;i++) {
	map_config->circuit_id[0] = BCM_CES_TDM_PORT_BASE_KATANA + ports[i];
	bcm_diag_ces_create(unit, &i, map_config, 2, 4);
	bcm_ces_service_enable_set(unit, i, 1);
    }

    /*
     * Step through BERT tests
     *
     * System
     */
    cli_out("System side tests...\n");

    for (i = 0;i < num_ports;i++) {
	port = 39 + ports[i];

	if (ports[i] > 7 && !lowDisable) {
	    lowDisable = 1;
	    cli_out("Disabling LIU ports 0-7\n");
	    for (j = 0;j < 8;j++) {
		loopback_config.port = j;
		loopback_config.type = NULL;
		loopback_config.enable = 0;
		bcm_diag_esw_ces_liu_enable(unit, &loopback_config);
	    }
	} else if (ports[i] < 8 && lowDisable) {
	    lowDisable = 0;
	    cli_out("Enabling LIU ports 0-7\n");
	    for (j = 0;j < 8;j++) {
		loopback_config.port = j;
		loopback_config.type = NULL;
		loopback_config.enable = 1;
		bcm_diag_esw_ces_liu_enable(unit, &loopback_config);
	    }
	}

	cli_out("LIU Port:%d system side PRBS test", ports[i]);
	bcm_esw_ces_framer_prbs_set(unit, port, 1, 2, 0, 0);

	/*
	 * Wait till pattern lock 
	 */
	j = 0;
	status = 0x0100;
	lock_stable = 0;

	do {
	    sal_sleep(1);
	    j++;
	    bcm_esw_ces_framer_prbs_status(unit, port, &status);
	    if (status) {
		cli_out("-");
		lock_stable = 0;
	    } else {
		cli_out("+");
		lock_stable++;
	    }
	} while (j < BCM_CES_TEST_LOCK_LOOP_COUNT && lock_stable < BCM_CES_TEST_LOCK_STABLE_COUNT);


	if (status == 0) {
	    for (j = 0;j < time;j++) {
		cli_out(".");
		sal_sleep(1);
		bcm_esw_ces_framer_prbs_status(unit, port, &status);
		if (status) {
		    if (status & 0x0100)
			cli_out(" Loss of lock");
		    else
			cli_out(" Errors:%d", (0x00FF & status));
		    break;
		}
	    } 
	} else {
	    if (status & 0x0100)
		cli_out(" No pattern lock");
	    else
		cli_out(" Errors:%d", (0x00FF & status));
	}

	bcm_esw_ces_framer_prbs_set(unit, port, 1, 0, 0, 0);
	cli_out(" - %s\n", (status ? "Failed":"Passed"));
    } 

    /*
     * Result
     */

    cli_out("Cleaning up\n");

    /*
     * Enable LIU ports 0-7
     */
    if (lowDisable) {
	cli_out("Enabling LIU ports 0-7\n");
	for (j = 0;j < 8;j++) {
	    loopback_config.port = j;
	    loopback_config.type = NULL;
	    loopback_config.enable = 1;
	    bcm_diag_esw_ces_liu_enable(unit, &loopback_config);
	}
    }

    /*
     * GE0 loopback
     */
    cli_out("Removing GE0 line loopback\n");
    READ_COMMAND_CONFIGr(unit, 1, &rval);
    soc_reg_field_set(unit, COMMAND_CONFIGr, &rval, LINE_LOOPBACKf, 0);
    WRITE_COMMAND_CONFIGr(unit, 1, rval);

    /*
     * Delete services
     */
    cli_out("Deleting CES services\n");
    result = bcm_ces_services_init(unit);

    /*
     * Done
     */
    cli_out("Done\n");
    sal_free(map_config);
    return CMD_OK;
}

/**
 * cmd_esw_ces_test_line()
 *
 * Process CES test commands
 */
cmd_result_t bcm_diag_esw_ces_test_line(int unit, int time, char *loopback, uint8 *ports, int num_ports) {
    int i,j;
    bcm_diag_ces_loopback_config_t loopback_config;
    int status;
    int result;
    int port;
    int lowDisable = 0;
    bcm_ces_channel_map_t *map_config;
    int lock_stable;

    map_config = (bcm_ces_channel_map_t *)sal_alloc(sizeof(bcm_ces_channel_map_t), "CES channel map");
    if (map_config == NULL)
        return BCM_E_MEMORY;

    /*
     * Init ces
     */
    cli_out("Initializing CES (This may take a few seconds)\n");
    result = bcm_ces_services_init(unit);

    if (BCM_FAILURE(result)) {
	cli_out("Command failed.  %s.\n", bcm_errmsg(result));
        sal_free(map_config);
	return CMD_FAIL;
    }

    /*
     * Init LIU and put ports into analog loopback
     */
    cli_out("Initializing LIUs\n");
    if (bcm_diag_esw_ces_liu_init(unit, 1)!= BCM_E_NONE) { /* T1 mode */ 
	cli_out("LIU device failed to initialize\n");
        sal_free(map_config);
	return CMD_FAIL;
    }

    if (loopback != NULL && *loopback == 'd') {
	cli_out("Setting digital loopback on LIUs\n");
	loopback_config.port = -1;
	loopback_config.type = loopback;
	loopback_config.enable = 1;
	bcm_diag_esw_ces_liu_loopback_set(unit, &loopback_config);
    } else {
	cli_out("/***********************************************/\n");
	cli_out("/*                                             */\n");
	cli_out("/*  Make sure that LIU ports have loopback     */\n");
	cli_out("/*  plugs in place or all tests will fail.     */\n");
	cli_out("/*                                             */\n");
	cli_out("/***********************************************/\n");
    }

    /*
     * Setup sixteen CES services
     */
    cli_out("Creating CES services\n");
    memset(map_config, 0, sizeof(bcm_ces_channel_map_t));
    map_config->first = 0;
    map_config->size  = 1;

    for (i = 0;i < num_ports;i++) {
	map_config->circuit_id[0] = BCM_CES_TDM_PORT_BASE_KATANA + ports[i];
	bcm_diag_ces_create(unit, &i, map_config, 2, 4);
	bcm_ces_service_enable_set(unit, i, 1);
    }

    /*
     * Line
     */
    cli_out("Line side tests...\n");

    for (i = 0;i < num_ports;i++) {
	port = 39 + ports[i];
	if (ports[i] > 7 && !lowDisable) {
	    lowDisable = 1;
	    cli_out("Disabling LIU ports 0-7\n");
	    for (j = 0;j < 8;j++) {
		loopback_config.port = j;
		loopback_config.type = NULL;
		loopback_config.enable = 0;
		bcm_diag_esw_ces_liu_enable(unit, &loopback_config);
	    }
	} else if (ports[i] < 8 && lowDisable) {
	    lowDisable = 0;
	    cli_out("Enabling LIU ports 0-7\n");
	    for (j = 0;j < 8;j++) {
		loopback_config.port = j;
		loopback_config.type = NULL;
		loopback_config.enable = 1;
		bcm_diag_esw_ces_liu_enable(unit, &loopback_config);
	    }
	}

	cli_out("LIU Port:%d line side PRBS test", ports[i]);
	bcm_esw_ces_framer_prbs_set(unit, port, 0, 2, 0, 0);

	/*
	 * Wait till pattern lock 
	 */
	j = 0;
	status = 0x0100;
	lock_stable = 0;

	do {
	    sal_sleep(1);
	    j++;
	    bcm_esw_ces_framer_prbs_status(unit, port, &status);
	    if (status) {
		cli_out("-");
		lock_stable = 0;
	    } else {
		cli_out("+");
		lock_stable++;
	    }
	} while (j < BCM_CES_TEST_LOCK_LOOP_COUNT && lock_stable < BCM_CES_TEST_LOCK_STABLE_COUNT);

	if (status == 0) {
	    for (j = 0;j < time;j++) {
		cli_out(".");
		sal_sleep(1);
		bcm_esw_ces_framer_prbs_status(unit, port, &status);
		if (status) {
		    if (status & 0x0100)
			cli_out(" Loss of lock");
		    else
			cli_out(" Errors:%d", (0x00FF & status));
		    break;
		}
	    }
	} else {
	    if (status & 0x0100)
		cli_out(" No pattern lock");
	    else
		cli_out(" Errors:%d", (0x00FF & status));
	}

	bcm_esw_ces_framer_prbs_set(unit, port, 0, 0, 0, 0);
	cli_out(" - %s\n", (status ? "Failed":"Passed"));
    }

    /*
     * Result
     */

    cli_out("Cleaning up\n");

    /*
     * Remove loopback
     */
    if (loopback != NULL && *loopback == 'd') {
	cli_out("Removing LIU loopback\n");
	loopback_config.port = -1;
	loopback_config.type = NULL;
	loopback_config.enable = 0;
	bcm_diag_esw_ces_liu_loopback_set(unit, &loopback_config);
    }

    /*
     * Enable LIU ports 0-7
     */
    if (lowDisable) {
	cli_out("Enabling LIU ports 0-7\n");
	for (j = 0;j < 8;j++) {
	    loopback_config.port = j;
	    loopback_config.type = NULL;
	    loopback_config.enable = 1;
	    bcm_diag_esw_ces_liu_enable(unit, &loopback_config);
	}
    }

    /*
     * Delete services
     */
    cli_out("Deleting CES services\n");
    result = bcm_ces_services_init(unit);

    /*
     * Done
     */
    cli_out("Done\n");
    sal_free(map_config);
    return CMD_OK;
}


/**
 * cmd_esw_ces_test()
 *
 * Process CES test commands
 */
cmd_result_t cmd_esw_ces_test(int unit, args_t *args) {
    int ret = CMD_OK;
    char *arg_string_p = NULL;
    uint8 ports[16];
    int num_ports;
    char *loopback;
    int time;
    char *port_string;
    parse_table_t parse_table;
    uint32 rval;
    char *tokstr;

    arg_string_p = ARG_GET(args);

    if (arg_string_p == NULL) {
	cli_out("%s\n", bcm_diag_cmd_esw_ces_test_usage);
	return CMD_FAIL;
    }

    /*
     * Make sure that board is set up correctly
     */
    WRITE_CMIC_GP_DATA_OUTr(unit, 0x4);
    WRITE_CMIC_GP_OUT_ENr(unit, 0x4);
    READ_CMIC_OVERRIDE_STRAPr(unit, &rval);
    soc_reg_field_set(unit, CMIC_OVERRIDE_STRAPr, &rval,
                      ENABLE_OVERRIDE_SPI_MASTER_SLAVE_MODEf, 1);
    soc_reg_field_set(unit, CMIC_OVERRIDE_STRAPr, &rval,
                      SPI_MASTER_SLAVE_MODEf, 1);
    WRITE_CMIC_OVERRIDE_STRAPr(unit, rval);


    if (sal_strcasecmp(arg_string_p, "line") == 0) {
	/*
	 * line
	 */
	num_ports = 0;
	time = 5;
	loopback = NULL;
	port_string = NULL;

	parse_table_init(unit, &parse_table);
	parse_table_add(&parse_table, "ports", PQ_STRING,
			"0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15", &port_string, NULL);
	parse_table_add(&parse_table, "time", PQ_INT | PQ_DFL,
			0, &time, NULL);
	parse_table_add(&parse_table, "loopback", PQ_STRING,
			"p", &loopback, NULL);

	if (parse_arg_eq(args, &parse_table) < 0 || ARG_CNT(args) > 0) {
	    ret = parse_fail(&parse_table, ARG_CUR(args));
	} else {
	    /*
	     * Get port list
	     */
	    
	    if (sal_strlen(port_string) > 0) {
		char *token = sal_strtok_r(port_string, ",", &tokstr);
		while (token != NULL) {
		    ports[num_ports] = atoi(token);
		    num_ports++;
		    token = sal_strtok_r(NULL, ",", &tokstr);
		} 
	    } else {
		ports[0] = 0;
		num_ports = 0;
	    }

	    if (loopback != NULL && *loopback != 'd' && *loopback != 'p') {
		cli_out("Invalid loopback type, valid values are d or p\n");
		ret = CMD_FAIL;
	    } else {
		ret = bcm_diag_esw_ces_test_line(unit, time, loopback, ports, num_ports);
		if (BCM_FAILURE(ret)) {
		    cli_out("Command failed.  %s.\n", bcm_errmsg(ret));
		    ret = CMD_FAIL;
		}
	    }
	}
	CLEAN_UP_AND_RETURN(ret);
    } else if (sal_strcasecmp(arg_string_p, "system") == 0) {
	/*
	 * system
	 */
	num_ports = 0;
	time = 5;
	port_string = NULL;

	parse_table_init(unit, &parse_table);
	parse_table_add(&parse_table, "ports", PQ_STRING,
			"0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15", &port_string, NULL);
	parse_table_add(&parse_table, "time", PQ_INT | PQ_DFL,
			0, &time, NULL);

	if (parse_arg_eq(args, &parse_table) < 0 || ARG_CNT(args) > 0) {
	    ret = parse_fail(&parse_table, ARG_CUR(args));
	} else {
	    /*
	     * Get port list
	     */
	    
	    if (sal_strlen(port_string) > 0) {
		char *token = sal_strtok_r(port_string, ",", &tokstr);
		while (token != NULL) {
		    ports[num_ports] = atoi(token);
		    num_ports++;
		    token = sal_strtok_r(NULL, ",", &tokstr);
		} 
	    } else {
		ports[0] = 0;
		num_ports = 0;
	    }

            ret = bcm_diag_esw_ces_test_system(unit, time, ports, num_ports);
            if (BCM_FAILURE(ret)) {
                cli_out("Command failed.  %s.\n", bcm_errmsg(ret));
                ret = CMD_FAIL;
            }
	}
	CLEAN_UP_AND_RETURN(ret);
    } else {
	cli_out("Missing test subcommand, expecting line | system\n\n");

	return CMD_FAIL;
    }

    return CMD_OK;
}
#endif

/*********************************************************************************************
 *
 *
 *
 *
 *
 * Service
 *
 *
 *
 *
 *
 *
 *********************************************************************************************/

/*Table*/
const char* ces_rcr_AgRcrStatesMsg [] =
{
    "Free running"    ,  /*   */
    "Acquisition"      ,  /*   RCR_STATE_ACQUISITION,       */
    "Normal"          ,  /*   RCR_STATE_NORMAL,           */
    "Holdover"        ,  /*   RCR_STATE_HOLDOVER,         */
    "Holdover (qualification period)" ,/* RCR_STATE_QUALIFICATION_PERIOD1 */
    "Fast acquisiton"    /*   RCR_STATE_FAST_ACQUISITION */,
    "Unknown"    /*   RCR_STATE_FAST_ACQUISITION */,
};


/**
 * Function: 
 *      bcm_diag_esw_ces_rclock_status_get
 * Purpose:
 *      Get the received clock recovery configuration
 */
int bcm_diag_esw_ces_rclock_status_get(int unit, int *service) {
    int ret;
    bcm_ces_crm_status_msg_t status;

    ret = _bcm_esw_ces_rclock_status_get(unit, *service, &status);

    if (ret == BCM_E_NONE) {
	cli_out("\n---- Received Clock Configuration %d ----\n", status.rclock);
	cli_out("state         : %s\n",  ces_rcr_AgRcrStatesMsg[status.clock_state]);
	cli_out("active (sec)  : %d\n",  status.seconds_active);
	cli_out("locked (sec)  : %d\n",  status.seconds_locked);
	cli_out("frequency     : %d.%d\n\n",  
                status.calculated_frequency_w,
                status.calculated_frequency_f);
	cli_out("------------------------------------------\n");
    }

    return ret;
}

/**
 * Function: 
 *      bcm_diag_esw_ces_rclock_config_get
 * Purpose:
 *      Get the received clock recovery configuration
 */
char *bcm_diag_esw_ces_output_brg_string[] = {
    "Port", "System1", "System2"};
char *bcm_diag_esw_ces_recovery_type_string[] = {
    "Differential", "Adaptive"};
int bcm_diag_esw_ces_rclock_config_get(int unit, int *service) {
    int ret;
    bcm_ces_rclock_config_t config;

    ret = bcm_ces_service_rclock_config_get(unit, *service, &config);

    if (ret == BCM_E_NONE) {
	cli_out("---- Received Clock Configuration %d ----\n", config.rclock);
	cli_out("enable        : %d\n",  config.enable);
	cli_out("output_brg    : %s\n",  bcm_diag_esw_ces_output_brg_string[config.output_brg]);
	cli_out("port          : %d\n",  config.port);
	cli_out("recovery_type : %s\n",  bcm_diag_esw_ces_recovery_type_string[config.recovery_type]);
	cli_out("------------------------------------------\n");
    }

    return ret;
}

/**
 * Function: 
 *      bcm_diag_esw_ces_rclock_config_set
 * Purpose:
 *      Set the received clock recovery configuration
 */
int bcm_diag_esw_ces_rclock_config_set(int unit, int *service, bcm_ces_rclock_config_t *config) {
    int ret = BCM_E_NONE;

    ret = bcm_ces_service_rclock_config_set(unit, *service, config);

    return ret;
}

/**
 * Function: 
 *      bcm_diag_esw_ces_egress_status
 * Purpose:
 *      
 */
static char *bcm_ces_egress_status_sync_state_string[] = {
    "LOPS", "AOPS"};
static char *bcm_ces_egress_status_jbf_state_string[] = {
    "Idle", "Fill", "Normal", "Flush"};

void bcm_diag_esw_ces_egress_status(int unit, int service) {
    int ret;
    bcm_ces_service_egress_status_t status;

    /*
     * Egress egress status
     */
    ret = bcm_ces_egress_status_get(unit, service, &status);

    if (ret == BCM_E_NONE) {
	cli_out("---- Egress Status ----\n");
	cli_out("sync_state : %s\n", bcm_ces_egress_status_sync_state_string[status.sync_state]);
	cli_out("jbf_state  : %s\n", bcm_ces_egress_status_jbf_state_string[status.jbf_state]);
	cli_out("ces_cw     : 0x%08x\n", status.ces_cw);
	cli_out("trimming   : %s\n", (status.trimming ? "On":"Off"));
	cli_out("----------------------\n");
    }
}


/**
 * Function: 
 *      bcm_diag_esw_ces_service_show_summary()
 * Purpose:
 *      Displays a summary of all CES services
 */
void bcm_diag_esw_ces_service_show_summary(int unit) {

    bcm_ces_service_config_t *config1;
    bcm_ces_service_config_t *config2;
    bcm_ces_service_config_t *config3;
    bcm_ces_service_config_t *config4;

    int i;

    config1 = (bcm_ces_service_config_t*)sal_alloc(sizeof(bcm_ces_service_config_t), "CES config");
    if (config1 == NULL)
        return;

    config2 = (bcm_ces_service_config_t*)sal_alloc(sizeof(bcm_ces_service_config_t), "CES config");
    if (config2 == NULL) {
        sal_free(config1);
        return;
    }

    config3 = (bcm_ces_service_config_t*)sal_alloc(sizeof(bcm_ces_service_config_t), "CES config");
    if (config3 == NULL) {
        sal_free(config2);
        sal_free(config1);
        return;
    }

    config4 = (bcm_ces_service_config_t*)sal_alloc(sizeof(bcm_ces_service_config_t), "CES config");
    if (config4 == NULL) {
        sal_free(config3);
        sal_free(config2);
        sal_free(config1);
        return;
    }

    cli_out("\nCES Service Summary\n");
    cli_out("F - Free  C - Configured  M - Modified  E - Enabled\n\n");
    cli_out("Srv F C M E | Srv F C M E | Srv F C M E | Srv F C M E\n");
    cli_out("--- - - - - + --- - - - - + --- - - - - + --- - - - -\n");

    for (i = 0;i < (BCM_CES_CIRCUIT_IDX_MAX/4);i++) {
	bcm_ces_service_config_get(unit, i, config1);
	bcm_ces_service_config_get(unit, (i + (1 * (BCM_CES_CIRCUIT_IDX_MAX/4))), config2);
	bcm_ces_service_config_get(unit, (i + (2 * (BCM_CES_CIRCUIT_IDX_MAX/4))), config3);
	bcm_ces_service_config_get(unit, (i + (3 * (BCM_CES_CIRCUIT_IDX_MAX/4))), config4);

	cli_out("%3d %c %c %c %c | %3d %c %c %c %c | %3d %c %c %c %c | %3d %c %c %c %c\n", 
                i,
                (BCM_CES_IS_FREE(config1) ? 'X':'-'),
                (BCM_CES_IS_CONFIGURED(config1) ? 'X':'-'),
                (BCM_CES_IS_MODIFIED(config1) ? 'X':'-'),
                (BCM_CES_IS_ENABLED(config1) ? 'X':'-'),
                (i + (1 * (BCM_CES_CIRCUIT_IDX_MAX/4))),
                (BCM_CES_IS_FREE(config2) ? 'X':'-'),
                (BCM_CES_IS_CONFIGURED(config2) ? 'X':'-'),
                (BCM_CES_IS_MODIFIED(config2) ? 'X':'-'),
                (BCM_CES_IS_ENABLED(config2) ? 'X':'-'),
                (i + (2 * (BCM_CES_CIRCUIT_IDX_MAX/4))),
                (BCM_CES_IS_FREE(config3) ? 'X':'-'),
                (BCM_CES_IS_CONFIGURED(config3) ? 'X':'-'),
                (BCM_CES_IS_MODIFIED(config3) ? 'X':'-'),
                (BCM_CES_IS_ENABLED(config3) ? 'X':'-'),
                (i + (3 * (BCM_CES_CIRCUIT_IDX_MAX/4))),
                (BCM_CES_IS_FREE(config4) ? 'X':'-'),
                (BCM_CES_IS_CONFIGURED(config4) ? 'X':'-'),
                (BCM_CES_IS_MODIFIED(config4) ? 'X':'-'),
                (BCM_CES_IS_ENABLED(config4) ? 'X':'-'));
    }

    sal_free(config4);
    sal_free(config3);
    sal_free(config2);
    sal_free(config1);
}

/**
 * Function: 
 *      bcm_diag_esw_ces_service_show
 * Purpose:
 *      
 */
void bcm_diag_esw_ces_service_show(int unit, int *service) {
    bcm_ces_service_config_t *config;

    config = (bcm_ces_service_config_t*)sal_alloc(sizeof(bcm_ces_service_config_t), "CES config");
    if (config == NULL)
        return;

    /*
     * Get config for service
     */
    if (bcm_ces_service_config_get(unit, *service, config) != BCM_E_NONE)
	return;

    cli_out("---- Service:%d ----\n", *service);

    /*
     * Display config
     */
    bcm_esw_ces_show_config(config);
    cli_out("--------------------\n");
    sal_free(config);
}

/**
 * bcm_diag_ces_service_encap_ip_version()
 *
 * Create ETH encapsulation. 
 */
int bcm_diag_ces_service_encap_ip_version(int unit, 
				   bcm_ces_packet_header_t *pkt_h) {
    if (pkt_h->ip_version == bcmCesIpV4) {
	pkt_h->ip_version       = bcmCesIpV4;
	pkt_h->ipv4.tos         = 1;
	pkt_h->ipv4.ttl         = 100;
	pkt_h->ipv4.source      = 0x0a0a0a0a;
	pkt_h->ipv4.destination = 0x0a0a0a0a;
    } else {
	; 
    }

    return BCM_E_NONE;
}

/**
 * bcm_diag_ces_service_copy_to_strict()
 *
 * Create ETH encapsulation. 
 */
void bcm_diag_ces_service_copy_to_strict(int unit, 
					 bcm_ces_encapsulation_t encapsulation,
					 bcm_ces_packet_header_t *pkt_h,
					 bcm_ces_packet_header_t *stk_h)
{
    int i;

    /*
     * Copy header to strict header 
     */
    memset(stk_h, 0, sizeof(bcm_ces_packet_header_t));
    stk_h->encapsulation = encapsulation;

    /*
     * Set source MAC
     */
    if (encapsulation == bcmCesEncapsulationEth) {
	memcpy(stk_h->eth.source, pkt_h->eth.destination, sizeof(pkt_h->eth.source));
    }

    /*
     * VLAN
     */
    if (encapsulation == bcmCesEncapsulationEth ||
	encapsulation == bcmCesEncapsulationMpls ||
	encapsulation == bcmCesEncapsulationIp) {
	stk_h->vlan_count = pkt_h->vlan_count;

	if (pkt_h->vlan_count > 0) {
	    for (i = 0;i < pkt_h->vlan_count;i++) {
		stk_h->vlan[i].vid = pkt_h->vlan[i].vid;
	    }
	}
    }


    /*
     * IPV4 source address
     */
    if ((encapsulation == bcmCesEncapsulationIp || encapsulation == bcmCesEncapsulationL2tp) && 
	pkt_h->ip_version == bcmCesIpV4) {
	stk_h->ip_version = bcmCesIpV4;
	stk_h->ipv4.source = pkt_h->ipv4.destination;
    }

    /*
     * IPV6 source address
     */
    if ((encapsulation == bcmCesEncapsulationIp || encapsulation == bcmCesEncapsulationL2tp) && 
	pkt_h->ip_version == bcmCesIpV6) {
	stk_h->ip_version = bcmCesIpV6;
	memcpy(stk_h->ipv6.source,pkt_h->ipv6.destination, sizeof(ip6_addr_t)); 
    }

    if (encapsulation == bcmCesEncapsulationL2tp &&
	(pkt_h->l2tpv3_count > 0) ) {

      stk_h->vc_label                = pkt_h->vc_label;
      stk_h->l2tpv3_count            = pkt_h->l2tpv3_count;
      stk_h->l2tpv3.udp_mode         = pkt_h->l2tpv3.udp_mode;
      stk_h->l2tpv3.header           = pkt_h->l2tpv3.header;
      stk_h->l2tpv3.session_local_id = pkt_h->l2tpv3.session_local_id;
      stk_h->l2tpv3.session_peer_id  = pkt_h->l2tpv3.session_peer_id;
      stk_h->l2tpv3.local_cookie1    = pkt_h->l2tpv3.local_cookie1;
      stk_h->l2tpv3.local_cookie2    = pkt_h->l2tpv3.local_cookie2;
      stk_h->l2tpv3.peer_cookie1     = pkt_h->l2tpv3.peer_cookie1;
      stk_h->l2tpv3.peer_cookie2     = pkt_h->l2tpv3.peer_cookie2;
    }
}

/**
 * bcm_diag_ces_service_encap_ETH()
 *
 * Create ETH encapsulation. 
 */
int bcm_diag_ces_service_encap_ETH(int unit, 
				   bcm_ces_encapsulation_t encap,
				   uint32 label,
				   bcm_ces_packet_header_t *pkt_h,
				   bcm_ces_packet_header_t *stk_h)
{
    pkt_h->encapsulation =  bcmCesEncapsulationEth;

    pkt_h->mpls_count = 0;
    pkt_h->l2tpv3_count = 0;

    pkt_h->vc_label = label;

    /*
     * Copy header to strict header 
     */
    bcm_diag_ces_service_copy_to_strict(unit,  encap, pkt_h, stk_h);


    return BCM_E_NONE;
}

/**
 * bcm_diag_ces_service_encap_MPLS()
 *
 * Create MPLS encapsulation
 */
int bcm_diag_ces_service_encap_MPLS(int unit, 
				    bcm_ces_encapsulation_t encap,
				    uint32 label,
				    bcm_ces_packet_header_t *pkt_h,
				    bcm_ces_packet_header_t *stk_h)
{
    pkt_h->encapsulation =  bcmCesEncapsulationMpls;

    /*
     * Must have:
     *
     * MPLS tunnel label(s)
     */
    pkt_h->vc_label = label;
    pkt_h->mpls_count = 1;
    pkt_h->mpls[0].label        = label;
    pkt_h->mpls[0].experimental = 0x12345678;
    pkt_h->mpls[0].ttl          = 100;

    /*
     * Can have:
     *
     *  vlan.
     */
    pkt_h->l2tpv3_count = 0;

    /*
     * Copy header to strict header 
     */
    bcm_diag_ces_service_copy_to_strict(unit,  encap, pkt_h, stk_h);

    return BCM_E_NONE;
}

/**
 * bcm_diag_ces_service_encap_IP()
 *
 * Create IP encapsulation.
 */
int bcm_diag_ces_service_encap_IP(int unit, 
				  bcm_ces_encapsulation_t encap,
				  uint32 label,
				  bcm_ces_packet_header_t *pkt_h,
				  bcm_ces_packet_header_t *stk_h)
{
    pkt_h->encapsulation =  bcmCesEncapsulationIp;

    /*
     * Can't have:
     *
     * MPLS
     */
    pkt_h->mpls_count = 0;

    /*
     * Must have:
     *
     * IP config
     */
    pkt_h->udp.destination = label;
    pkt_h->udp.source      = label;
    pkt_h->vc_label        = label;
    pkt_h->udp_chksum = 1;

    if (pkt_h->ip_version == bcmCesIpV6) {
	uint8 ipv6Address[16] = {0x20, 0x01, 0x0d, 0xb8, 0x85, 0xa3, 0x00, 0x00, 
                                 0x00, 0x00, 0x8a, 0x2e, 0x03, 0x70, 0x73, 0x34};
	sal_memcpy(pkt_h->ipv6.source, ipv6Address, sizeof(ip6_addr_t));
	sal_memcpy(pkt_h->ipv6.destination, ipv6Address, sizeof(ip6_addr_t));
	pkt_h->ipv6.hop_limit = 255;
    }

    /*
     * Copy header to strict header 
     */
    bcm_diag_ces_service_copy_to_strict(unit,  encap, pkt_h, stk_h);

    return BCM_E_NONE;
}

/**
 * bcm_diag_ces_service_encap_L2TP()
 *
 * Create L2TP encapsulation.
 */
int bcm_diag_ces_service_encap_L2TP(int unit, 
				    bcm_ces_encapsulation_t encap,
				    uint32 label,
				    bcm_ces_packet_header_t *pkt_h,
				    bcm_ces_packet_header_t *stk_h)
{
    pkt_h->encapsulation =  bcmCesEncapsulationL2tp;

    pkt_h->mpls_count = 0;

    if (pkt_h->ip_version == bcmCesIpV6) {
	uint8 ipv6Address[16] = {0x20, 0x01, 0x0d, 0xb8, 0x85, 0xa3, 0x00, 0x00, 
                                 0x00, 0x00, 0x8a, 0x2e, 0x03, 0x70, 0x73, 0x34};
	sal_memcpy(pkt_h->ipv6.source, ipv6Address, sizeof(ip6_addr_t));
	sal_memcpy(pkt_h->ipv6.destination, ipv6Address, sizeof(ip6_addr_t));
	pkt_h->ipv6.hop_limit = 255;
    }

    /*
     * Must have:
     *
     * L2TP config
     */
    pkt_h->vc_label                = label;
    pkt_h->l2tpv3_count            = 3;
    pkt_h->l2tpv3.udp_mode         = 1;           /* Using UDP */
    pkt_h->l2tpv3.header           = 0x30000;  /* Header */
    pkt_h->l2tpv3.session_local_id = label;    /* Session local ID */
    pkt_h->l2tpv3.session_peer_id  = label;     /* Session peer ID */
    pkt_h->l2tpv3.local_cookie1    = 0x5555;       /* Local cookie 1 */
    pkt_h->l2tpv3.local_cookie2    = 0x7777;       /* Local cookie 2 */
    pkt_h->l2tpv3.peer_cookie1     = 0x5555;        /* Peer cookie 1 */
    pkt_h->l2tpv3.peer_cookie2     = 0x7777;        /* Peer cookie 2 */

    pkt_h->udp.destination = 1701;
    pkt_h->udp.source      = 1701;
    pkt_h->udp_chksum = 1;

    /*
     * Copy header to strict header 
     */
    bcm_diag_ces_service_copy_to_strict(unit,  bcmCesEncapsulationL2tp, pkt_h, stk_h);

    return BCM_E_NONE;
}


/**
 * bcm_diag_ces_service_set_header()
 *
 * Modify CES service
 */
int bcm_diag_ces_service_set_header(int unit, bcm_ces_service_config_t *config, int service, int encapsulation) {
    uint32 label = 0xAA00 + (service * 2);

    /*
     * Encapsulation
     */
    switch (encapsulation)
    {
    case 0:
	config->encapsulation = bcmCesEncapsulationIp;
	bcm_diag_ces_service_encap_IP(unit, encapsulation, label, &config->header, &config->strict_data);
	break;
    case 1:
	config->encapsulation = bcmCesEncapsulationMpls;
	bcm_diag_ces_service_encap_MPLS(unit, encapsulation, label, &config->header, &config->strict_data);
	break;
    case 2:
	config->encapsulation = bcmCesEncapsulationEth;
	bcm_diag_ces_service_encap_ETH(unit, encapsulation, label, &config->header, &config->strict_data);
	break;
    case 3:
	config->encapsulation = bcmCesEncapsulationL2tp;
	bcm_diag_ces_service_encap_L2TP(unit, bcmCesEncapsulationL2tp , label, &config->header, &config->strict_data);
	break;
    default:
	cli_out("Unsupported encapsulation:%d, valid values are 0 thorugh 3\n", encapsulation);
	return CMD_FAIL;
	break;
    }

    return CMD_OK;
}



/**
 * bcm_diag_ces_service_modify()
 *
 * Modify CES service
 */
void bcm_diag_ces_service_modify(int unit, int *service) {
    bcm_ces_service_t ces_service = (bcm_ces_service_t)*service;

    if (SERVICE_IS_VALID(*service)) {

	/*
	 * Apply changes
	 */
	if (bcm_ces_service_create(unit, BCM_CES_WITH_ID, &serviceConfig[*service], &ces_service)!= BCM_E_NONE)
	{
	    cli_out("CES service:%d modify failed\n", *service);
	}
	else
	    cli_out("CES service:%d modified\n", *service);
    } else 
	cli_out("Invalid service:%d\n", *service);
}

/**
 * bcm_diag_ces_create()
 *
 * Create CES service
 */
int bcm_diag_ces_create(int unit, int *service, bcm_ces_channel_map_t *map_config, int encapsulation, int ip_version) {
    int i;
    uint32 flags = 0;
    bcm_ces_service_config_t *def_config;
    bcm_ces_service_config_t *config;
    bcm_ces_service_t ces_service = 0;
    int ret;

    config = (bcm_ces_service_config_t*)sal_alloc(sizeof(bcm_ces_service_config_t), "CES config");
    if (config == NULL)
        return CMD_FAIL;

    def_config = (bcm_ces_service_config_t*)sal_alloc(sizeof(bcm_ces_service_config_t), "CES config");
    if (def_config == NULL) {
        sal_free(config);
        return CMD_FAIL;
    }

    if (service != NULL)
	ces_service = *service;

    if (ip_version != 4 && ip_version != 6) {
	cli_out("Invalid IP version:%d. Valid values are 4 and 6\n", ip_version);
        sal_free(config);
        sal_free(def_config);
	return CMD_FAIL;
    }

    /*
     * Get current config
     */
    if (bcm_ces_service_config_get(unit, ces_service, def_config) != BCM_E_NONE) {
	cli_out("Failed to get default config\n");
        sal_free(config);
        sal_free(def_config);
	return CMD_FAIL;
    }

    /*
     * Make copy so that we don't stomp on existing config
     */
    memcpy(config, def_config, sizeof(bcm_ces_service_config_t));

    /*
     * Ingress channel map
     */
    config->ingress_channel_map.size     = map_config->size;
    config->ingress_channel_map.first    = 0;

    for (i = 0;i < map_config->size;i++)
    {
	config->ingress_channel_map.circuit_id[i] = map_config->circuit_id[0];
	config->ingress_channel_map.slot[i] = map_config->slot[i];
    }


    /*
     * Egress channel map
     */
    config->egress_channel_map.size     = map_config->size;
    config->egress_channel_map.first    = 0;

    for (i = 0;i < map_config->size;i++)
    {
	config->egress_channel_map.circuit_id[i] = map_config->circuit_id[0];
	config->egress_channel_map.slot[i] = map_config->slot[i];
    }

    /*
     * IP Version
     */
    config->header.ip_version = ip_version;

    if ((ret = bcm_diag_ces_service_set_header(unit, config, ces_service, encapsulation)) != CMD_OK) {
        sal_free(config);
        sal_free(def_config);
	return ret;
    }

    /*
     * Use specified service number?
     */
    if (service != NULL) {
	ces_service = *service;
	flags = BCM_CES_WITH_ID;
    }

    
    if (bcm_ces_service_create(unit, flags, config, &ces_service)!= BCM_E_NONE) {
	cli_out("CES service create failed\n");
    } else {
	memcpy(&serviceConfig[ces_service], config, sizeof(bcm_ces_service_config_t));
    }
 
    sal_free(config);
    sal_free(def_config);
    return ces_service;
}

/**
 * Function:
 *      bcm_ces_service_ingress_channel_config_set()
 * Purpose:
 *      
 *      
 */
void bcm_ces_service_ingress_channel_config_set(
    int unit, 
    int service, 
    bcm_ces_ingress_channel_config_t *config)
{
    if (!SERVICE_IS_VALID(service))  {
	cli_out("Invalid service:%d\n", service);
        return;
    }

    /*
     * Modify record.
     */
    memcpy(&serviceConfig[service].ingress_channel_config, 
	   config, 
	   sizeof(bcm_ces_ingress_channel_config_t));
}

/**
 * Function:
 *      bcm_ces_service_egress_channel_config_set()
 * Purpose:
 *      
 *      
 */
void bcm_ces_service_egress_channel_config_set(
    int unit, 
    int service, 
    bcm_ces_egress_channel_config_t *config)
{
    if (!SERVICE_IS_VALID(service)) {
	cli_out("Invalid service:%d\n", service);
        return;
    }

    /*
     * Modify record.
     */
    memcpy(&serviceConfig[service].egress_channel_config, 
	   config, 
	   sizeof(bcm_ces_egress_channel_config_t));
}

/**
 * Function:
 *      bcm_ces_service_channel_map_config_set()
 * Purpose:
 *      
 */
void bcm_ces_service_channel_map_config_set(
    int unit, 
    int service, 
    int egress,
    bcm_ces_channel_map_t *config)
{
    if (!SERVICE_IS_VALID(service)) {
	cli_out("Invalid service:%d\n", service);
        return;
    }

    /*
     * Modify record.
     */
    if (egress)
	memcpy(&serviceConfig[service].egress_channel_map,
	       config, 
	       sizeof(bcm_ces_channel_map_t));
    else
	memcpy(&serviceConfig[service].ingress_channel_map,
	       config, 
	       sizeof(bcm_ces_channel_map_t));
}

cmd_result_t cmd_esw_ces_service_cas(int unit, int service, args_t *args) {
    int ret = CMD_OK;
    char *arg_string_p = NULL;
    parse_table_t parse_table;
    int enable;
    bcm_ces_cas_packet_control_t packet_control;

    arg_string_p = ARG_GET(args);

    if (arg_string_p == NULL) {
	return CMD_USAGE;
    }

    if (sal_strcasecmp(arg_string_p, "enable") == 0) {

	if (ARG_CNT(args) > 0) {
	    arg_string_p = ARG_GET(args);

	    if (sal_strcasecmp(arg_string_p, "get") == 0) {
		    bcm_ces_service_ingress_cas_enable_get(unit, service, &enable);
		    ret = CMD_OK;
		    cli_out("CAS is %s for CES service %d\n", 
                            (enable ? "Enabled":"Disabled"),
                            service);
	    } else if (sal_strcasecmp(arg_string_p, "set") == 0) {
		/*
		 * Defaults
		 */
		enable = 1;
		
		/*
		 * CAS Enable
		 */
		parse_table_init(unit, &parse_table);
		parse_table_add(&parse_table, "enable", PQ_BOOL | PQ_DFL,
				0, &enable, NULL);

		if (parse_arg_eq(args, &parse_table) < 0 || ARG_CNT(args) > 0) {
		    ret = parse_fail(&parse_table, ARG_CUR(args));
		} else {
		    bcm_ces_service_ingress_cas_enable_set(unit, service, enable);
		    ret = CMD_OK;
		}

		CLEAN_UP_AND_RETURN(ret);
	    } else {
		cli_out("Expected either get or set\n");
		ret = CMD_FAIL;
	    }
	}
    } else if (sal_strcasecmp(arg_string_p, "tx") == 0) {

	/*
	 * Defaults
	 */
	service = 0;
	packet_control.packet_count = 1;
	packet_control.flags = bcmCesCasPacketSchedule;

	/*
	 * CAS Enable
	 */
	parse_table_init(unit, &parse_table);
	parse_table_add(&parse_table, "service", PQ_INT | PQ_DFL,
			0, &service, NULL);
	parse_table_add(&parse_table, "pkts", PQ_INT | PQ_DFL,
			0, &packet_control.packet_count, NULL);

	if (parse_arg_eq(args, &parse_table) < 0 || ARG_CNT(args) > 0) {
	    ret = parse_fail(&parse_table, ARG_CUR(args));
	} else {
	    bcm_ces_service_cas_packet_enable(unit, service, &packet_control);
	    ret = CMD_OK;
	}
	CLEAN_UP_AND_RETURN(ret);
    } else {
	cli_out("Expected either enable or tx\n");
	ret = CMD_FAIL;
    }

    return ret;
}

cmd_result_t cmd_esw_ces_service_create(int unit, args_t *args) {
    int ret = CMD_OK;
    parse_table_t parse_table;
    int encapsulation;
    int ip_version;
    char *slots;
    bcm_ces_channel_map_t *map_config;
    int port = BCM_CES_TDM_PORT_BASE_KATANA;
    int service = -1;
    int *service_ptr = NULL;
    char *tokstr;

    map_config = (bcm_ces_channel_map_t *)sal_alloc(sizeof(bcm_ces_channel_map_t), "CES channel map");
    if (map_config == NULL)
        return BCM_E_MEMORY;

    /*
     * Defaults
     */
    encapsulation = bcmCesEncapsulationIp;
    ip_version = 4;
    memset(map_config, 0, sizeof(bcm_ces_channel_map_t));
    map_config->first = 0;
    map_config->size  = 0;

    /*
     * Service Create 
     */
    parse_table_init(unit, &parse_table);
    parse_table_add(&parse_table, "service", PQ_INT | PQ_DFL,
		    0, &service, NULL);

    parse_table_add(&parse_table, "port", PQ_INT | PQ_DFL,
		    0, &port, NULL);

    parse_table_add(&parse_table, "slots", PQ_STRING,
		    "0", &slots, NULL);

    parse_table_add(&parse_table, "encap", PQ_INT | PQ_DFL,
		    0, &encapsulation, NULL);

    parse_table_add(&parse_table, "ip_version", PQ_INT | PQ_DFL,
		    0, &ip_version, NULL);

    if (parse_arg_eq(args, &parse_table) < 0 || ARG_CNT(args) > 0)
    {
	ret = parse_fail(&parse_table, ARG_CUR(args));
    }
    else
    {
	map_config->circuit_id[0] = port;

	/*
	 * Parse slots list
	 */
	if (sal_strlen(slots) > 0) {
	    char *token = sal_strtok_r(slots, ",", &tokstr);
	    while (token != NULL) {
		map_config->slot[map_config->size] = atoi(token);
		map_config->circuit_id[map_config->size] = map_config->circuit_id[0];
		map_config->size++;
		token = sal_strtok_r(NULL, ",", &tokstr);
	    }
	} else {
	    map_config->slot[0] = 0;
	    map_config->size = 1;
	}

	/*
	 * Was service specified?
	 */
	if (service != -1)
	    service_ptr = &service;

	bcm_diag_ces_create(unit, service_ptr, map_config, encapsulation, ip_version);
	ret = CMD_OK;
    }

    sal_free(map_config);
    CLEAN_UP_AND_RETURN(ret);
}


#ifndef __KERNEL__
/**
 * Function: 
 *      cmd_esw_ces_service_pm
 * Purpose:
 *      
 */
cmd_result_t cmd_esw_ces_service_pm(int unit, int service, args_t *args) {
    int ret = CMD_OK;
    parse_table_t parse_table;
    int i;
    int reset;

    /*	    
     * Defaults
     */
    reset = 0;

    parse_table_init(unit, &parse_table);
    parse_table_add(&parse_table, "reset", PQ_INT | PQ_DFL,
		    0, &reset, NULL);

    if (parse_arg_eq(args, &parse_table) < 0 || ARG_CNT(args) > 0) {
	ret = parse_fail(&parse_table, ARG_CUR(args));
    } else {
	bcm_ces_service_pm_stats_t pm_counts;

	bcm_ces_service_pm_get(unit, service, &pm_counts);

	cli_out("bad_length_packets            : %d\n", pm_counts.jbf_bad_length_packets);
	cli_out("missordered_dropped_packets   : %d\n", pm_counts.jbf_dropped_ooo_packets);
	cli_out("reordered_packets             : %d\n", pm_counts.jbf_reordered_ooo_packets);
	cli_out("minimum_jitter_buffer_depth   : %d\n", pm_counts.jbf_depth_min);
	cli_out("maximum_jitter_buffer_depth   : %d\n", pm_counts.jbf_depth_max);
	cli_out("jitter_buffer_missing_packets : %d\n", pm_counts.jbf_missing_packets);
	cli_out("jitter_buffer_underrun_packets: %d\n", pm_counts.jbf_underruns);
/*	cli_out("jitter_buffer_restart         : %d\n", pm_counts.jitter_buffer_restart); */
	cli_out("transmitted_bytes             : %d\n", pm_counts.transmitted_bytes);
	cli_out("transmitted_packets           : %d\n", pm_counts.transmitted_packets);
	cli_out("received_bytes                : %d\n", pm_counts.received_bytes);
	cli_out("received_packets              : %d\n", pm_counts.received_packets);
	for (i = 0;i < BCM_CES_RPC_CCNT_MAX;i++) {
	    cli_out("Count%d                        : %d\n", i, pm_counts.rpc_channel_specific[i]);
	}

	if (reset == 1) 
	    bcm_ces_service_pm_clear(unit, service);

	ret = CMD_OK;
    }

    CLEAN_UP_AND_RETURN(ret);
}
#endif

cmd_result_t cmd_esw_ces_service_header(int unit, int service, args_t *args) {
    int ret = CMD_OK;
    parse_table_t parse_table;
    bcm_ces_service_config_t *config = &serviceConfig[service];

    if (!SERVICE_IS_VALID(service)) 
    {
	cli_out("Invalid service:%d\n", service);
	return CMD_FAIL;
    }

    parse_table_init(unit, &parse_table);

    parse_table_add(&parse_table, "encap", PQ_INT | PQ_DFL,
		    0, &config->encapsulation, NULL);
    parse_table_add(&parse_table, "eth.source", PQ_MAC | PQ_DFL,
		    0, &config->header.eth.source, NULL);
    parse_table_add(&parse_table, "eth.destination", PQ_MAC | PQ_DFL,
		    0, &config->header.eth.destination, NULL);
    parse_table_add(&parse_table, "vlan.1.priority", PQ_INT | PQ_DFL,
		    0, &config->header.vlan[0].priority, NULL);
    parse_table_add(&parse_table, "vlan.1.vid", PQ_INT | PQ_DFL,
		    0, &config->header.vlan[0].vid, NULL);
    parse_table_add(&parse_table, "vlan.2.priority", PQ_INT | PQ_DFL,
		    0, &config->header.vlan[1].priority, NULL);
    parse_table_add(&parse_table, "vlan.2.vid", PQ_INT | PQ_DFL,
		    0, &config->header.vlan[1].vid, NULL);
    parse_table_add(&parse_table, "vlan_count", PQ_INT | PQ_DFL,
		    0, &config->header.vlan_count, NULL);
    parse_table_add(&parse_table, "ipv4.tos", PQ_INT | PQ_DFL,
		    0, &config->header.ipv4.tos, NULL);
    parse_table_add(&parse_table, "ipv4.ttl", PQ_INT | PQ_DFL,
		    0, &config->header.ipv4.ttl, NULL);
    parse_table_add(&parse_table, "ipv4.source", PQ_IP | PQ_DFL,
		    0, &config->header.ipv4.source, NULL);
    parse_table_add(&parse_table, "ipv4.destination", PQ_IP | PQ_DFL,
		    0, &config->header.ipv4.destination, NULL);
    parse_table_add(&parse_table, "mpls.1.label", PQ_INT | PQ_DFL,
		    0, &config->header.mpls[0].label, NULL);
    parse_table_add(&parse_table, "mpls.1.experimental", PQ_INT | PQ_DFL,
		    0, &config->header.mpls[0].experimental, NULL);
    parse_table_add(&parse_table, "mpls.1.ttl", PQ_INT | PQ_DFL,
		    0, &config->header.mpls[0].ttl, NULL);
    parse_table_add(&parse_table, "mpls.2.label", PQ_INT | PQ_DFL,
		    0, &config->header.mpls[1].label, NULL);
    parse_table_add(&parse_table, "mpls.2.experimental", PQ_INT | PQ_DFL,
		    0, &config->header.mpls[1].experimental, NULL);
    parse_table_add(&parse_table, "mpls.2.ttl", PQ_INT | PQ_DFL,
		    0, &config->header.mpls[1].ttl, NULL);
    parse_table_add(&parse_table, "vc_label", PQ_INT | PQ_DFL,
		    0, &config->header.vc_label, NULL);
    parse_table_add(&parse_table, "udp.source", PQ_INT | PQ_DFL,
		    0, &config->header.udp.source, NULL);
    parse_table_add(&parse_table, "udp.destination", PQ_INT | PQ_DFL,
		    0, &config->header.udp.destination, NULL);
    parse_table_add(&parse_table, "ip_version", PQ_INT | PQ_DFL,
		    0, &config->header.ip_version, NULL);
    parse_table_add(&parse_table, "mpls_count", PQ_INT | PQ_DFL,
		    0, &config->header.mpls_count, NULL);
    parse_table_add(&parse_table, "l2tpv3_count", PQ_INT | PQ_DFL,
		    0, &config->header.l2tpv3_count, NULL);
    parse_table_add(&parse_table, "rtp_exists", PQ_BOOL | PQ_DFL,
		    0, &config->header.rtp_exists, NULL);
    parse_table_add(&parse_table, "rtp_pt", PQ_INT | PQ_DFL,
		    0, &config->header.rtp_pt, NULL);
    parse_table_add(&parse_table, "rtp_ssrc", PQ_INT | PQ_DFL,
		    0, &config->header.rtp_ssrc, NULL);
    parse_table_add(&parse_table, "udp_chksum", PQ_INT | PQ_DFL,
		    0, &config->header.udp_chksum, NULL);
    parse_table_add(&parse_table, "l2tpv3.udp_mode", PQ_INT | PQ_DFL,
		    0, &config->header.l2tpv3.udp_mode, NULL);
    parse_table_add(&parse_table, "l2tpv3.header", PQ_INT | PQ_DFL,
		    0, &config->header.l2tpv3.header, NULL);
    parse_table_add(&parse_table, "l2tpv3.session_local_id", PQ_INT | PQ_DFL,
		    0, &config->header.l2tpv3.session_local_id, NULL);
    parse_table_add(&parse_table, "l2tpv3.session_peer_id", PQ_INT | PQ_DFL,
		    0, &config->header.l2tpv3.session_peer_id, NULL);
    parse_table_add(&parse_table, "l2tpv3.local_cookie1", PQ_INT | PQ_DFL,
		    0, &config->header.l2tpv3.local_cookie1, NULL);
    parse_table_add(&parse_table, "l2tpv3.local_cookie2", PQ_INT | PQ_DFL,
		    0, &config->header.l2tpv3.local_cookie2, NULL);
    parse_table_add(&parse_table, "l2tpv3.peer_cookie1", PQ_INT | PQ_DFL,
		    0, &config->header.l2tpv3.peer_cookie1, NULL);
    parse_table_add(&parse_table, "l2tpv3.peer_cookie2", PQ_INT | PQ_DFL,
		    0, &config->header.l2tpv3.peer_cookie2, NULL);

    if (parse_arg_eq(args, &parse_table) < 0 || ARG_CNT(args) > 0) {
	ret = parse_fail(&parse_table, ARG_CUR(args));
    } else {
	ret = CMD_OK;
    }

    bcm_diag_ces_service_copy_to_strict(unit, config->encapsulation, &config->header, &config->strict_data);

    CLEAN_UP_AND_RETURN(ret);
}


cmd_result_t cmd_esw_ces_service(int unit, args_t *args) {
    int result = 0;
    int ret = CMD_OK;
    char *arg_string_p = NULL;
    parse_table_t parse_table;
    int service = 0;
    bcm_ces_service_config_t *config;
    char *tail;
    char *tokstr;

    arg_string_p = ARG_GET(args);

    if (arg_string_p == NULL) {
	cli_out("%s\n", bcm_diag_cmd_esw_ces_service_usage);
	return CMD_FAIL;
    }

    /*
     * Find service number
     */
    service = strtol(arg_string_p, &tail, 10);
    if (service == 0 && tail == arg_string_p) {
	/*
	 * No service number
	 */
	if (sal_strcasecmp(arg_string_p, "create") == 0) {

	    ret = cmd_esw_ces_service_create(unit, args);

	} else if (sal_strcasecmp(arg_string_p, "show") == 0) {
            /*
	     * Show summary of all services
	     */
	    bcm_diag_esw_ces_service_show_summary(unit);
	} else {
	    cli_out("Invalid CES service subcommand: %s, expecting one of create or show\n", arg_string_p);
	    return CMD_FAIL;
	}
    } else {

	/*
	 * Got service number
	 */
	if (service < 0 || service >= BCM_CES_CIRCUIT_IDX_MAX) {
	    cli_out("Invalid service number, valid values are between 0 and 63\n");
	    return CMD_FAIL;
	}

        config = &serviceConfig[service];

	arg_string_p = ARG_GET(args);

	if (arg_string_p == NULL) {
	    cli_out("%s\n", bcm_diag_cmd_esw_ces_service_usage);
	    return CMD_FAIL;
	}

	/*
	 * Service commands
	 */
	if (sal_strcasecmp(arg_string_p, "set") == 0) {

	    if (ARG_CNT(args) > 0) {
		arg_string_p = ARG_GET(args);

		if (sal_strcasecmp(arg_string_p, "channel") == 0) {

		    if (ARG_CNT(args) > 0) {
			arg_string_p = ARG_GET(args);

			if (sal_strcasecmp(arg_string_p, "ingress") == 0) {
			    bcm_ces_ingress_channel_config_t ingress_config;

			    memcpy(&ingress_config, &config->ingress_channel_config, sizeof(bcm_ces_ingress_channel_config_t));

			    /*
			     * Set ingress channel config
			     */
			    parse_table_init(unit, &parse_table);
			    parse_table_add(&parse_table, "dba", PQ_BOOL | PQ_DFL,
					    0, &ingress_config.dba, NULL);
			    parse_table_add(&parse_table, "autor", PQ_BOOL | PQ_DFL,
					    0, &ingress_config.auto_r_bit, NULL);
			    parse_table_add(&parse_table, "mef", PQ_BOOL | PQ_DFL,
					    0, &ingress_config.mef_len_support, NULL);
			    parse_table_add(&parse_table, "pbfsize", PQ_INT | PQ_DFL,
					    0, &ingress_config.pbf_size, NULL);

			    if (parse_arg_eq(args, &parse_table) < 0 || ARG_CNT(args) > 0)
			    {
				ret = parse_fail(&parse_table, ARG_CUR(args));
			    }
			    else
			    {
				bcm_ces_service_ingress_channel_config_set(unit, service, &ingress_config);
				ret = CMD_OK;
			    }

			    CLEAN_UP_AND_RETURN(ret);

			} else if (sal_strcasecmp(arg_string_p, "egress") == 0) {
			    bcm_ces_egress_channel_config_t egress_config;

			    memcpy(&egress_config, 
				   &config->egress_channel_config, 
				   sizeof(bcm_ces_egress_channel_config_t));

			    /*
			     * Set ingress channel config
			     */
			    parse_table_init(unit, &parse_table);
			    parse_table_add(&parse_table, "pktsync", PQ_INT | PQ_DFL,
					    0, &egress_config.packet_sync_selector, NULL);
			    parse_table_add(&parse_table, "fpp", PQ_INT | PQ_DFL,
					    0, &egress_config.frames_per_packet, NULL);
			    parse_table_add(&parse_table, "jbfrsize", PQ_INT | PQ_DFL,
					    0, &egress_config.jbf_ring_size, NULL);
			    parse_table_add(&parse_table, "jbfwsize", PQ_INT | PQ_DFL,
					    0, &egress_config.jbf_win_size, NULL);
			    parse_table_add(&parse_table, "jbfbop", PQ_INT | PQ_DFL,
					    0, &egress_config.jbf_bop, NULL);


			    if (parse_arg_eq(args, &parse_table) < 0 || ARG_CNT(args) > 0)
			    {
				ret = parse_fail(&parse_table, ARG_CUR(args));
			    }
			    else
			    {
				bcm_ces_service_egress_channel_config_set(unit, service, &egress_config);
				ret = CMD_OK;
			    }

			    CLEAN_UP_AND_RETURN(ret);

			} else {
			    cli_out("Unknown service set channel option:%s\n", arg_string_p);
			    ret = CMD_FAIL;
			}
		    } else {
			cli_out("Expected either ingress or egress\n");
			ret = CMD_FAIL;
		    }
		} else if (sal_strcasecmp(arg_string_p, "map") == 0) {

		    if (ARG_CNT(args) > 0) {
			bcm_ces_channel_map_t *map_config;
			int egress = 1;
			char *slots;
			int port = BCM_CES_TDM_PORT_BASE_KATANA;

                        map_config = (bcm_ces_channel_map_t *)sal_alloc(sizeof(bcm_ces_channel_map_t), "CES channel map");
                        if (map_config == NULL)
                            return BCM_E_MEMORY;
			memset(map_config, 0, sizeof(bcm_ces_channel_map_t));
			map_config->first = 0;
			map_config->size  = 0;

			parse_table_init(unit, &parse_table);
			parse_table_add(&parse_table, "egress", PQ_BOOL | PQ_DFL,
					0, &egress, NULL);
			parse_table_add(&parse_table, "port", PQ_INT | PQ_DFL,
					0, &port, NULL);
			parse_table_add(&parse_table, "slots", PQ_STRING,
					"0", &slots, NULL);

			if (parse_arg_eq(args, &parse_table) < 0 || ARG_CNT(args) > 0)
			{
			    ret = parse_fail(&parse_table, ARG_CUR(args));
			}
			else
			{
			    map_config->circuit_id[0] = port;

			    /*
			     * Parse slots list
			     */
			    if (sal_strlen(slots) > 0) {
				char *token = sal_strtok_r(slots, ",", &tokstr);
				while (token != NULL) {
				    map_config->slot[map_config->size] = atoi(token);
				    map_config->circuit_id[map_config->size] = map_config->circuit_id[0];
				    map_config->size++;
				    token = sal_strtok_r(NULL, ",", &tokstr);
				}
			    } else {
				map_config->slot[0] = 0;
				map_config->size = 1;
			    }

			    bcm_ces_service_channel_map_config_set(unit, service, egress, map_config);
			    ret = CMD_OK;
			}

                        sal_free(map_config);
			CLEAN_UP_AND_RETURN(ret);
		    }
		} else if (sal_strcasecmp(arg_string_p, "header") == 0) {

		    if (ARG_CNT(args) > 0) {
			ret = cmd_esw_ces_service_header(unit, service, args);
		    }
		} else {
		    cli_out("Expected either channel, map or header\n");
		    ret = CMD_FAIL;
		}
	    } else {
		cli_out("Unknown service set option:%s\n", arg_string_p);
		ret = CMD_FAIL;
	    }
	} else if (sal_strcasecmp(arg_string_p, "show") == 0) {

	    bcm_diag_esw_ces_service_show(unit, &service);
	    ret = CMD_OK;

	} else if (sal_strcasecmp(arg_string_p, "enable") == 0) {
	    int enable;

	    /*
	     * Defaults
	     */
	    enable = 1;

	    /*
	     * Service Enable
	     */
	    parse_table_init(unit, &parse_table);
	    parse_table_add(&parse_table, "enable", PQ_INT | PQ_DFL,
			    0, &enable, NULL);

	    if (parse_arg_eq(args, &parse_table) < 0 || ARG_CNT(args) > 0) {
		ret = parse_fail(&parse_table, ARG_CUR(args));
	    } else {
		bcm_ces_service_enable_set(unit, service, enable);
		ret = CMD_OK;
	    }
	    CLEAN_UP_AND_RETURN(ret);
	} else if (sal_strcasecmp(arg_string_p, "cas") == 0) {
	    /*
	     * CAS
	     */
	    ret =  cmd_esw_ces_service_cas(unit, service, args);

	} else if (sal_strcasecmp(arg_string_p, "modify") == 0) {

	    bcm_diag_ces_service_modify(unit, &service);
	    ret = CMD_OK;
	} else if (sal_strcasecmp(arg_string_p, "delete") == 0) {
	    bcm_ces_service_destroy(unit, service);
	    ret = CMD_OK;
#ifndef __KERNEL__
	} else if (sal_strcasecmp(arg_string_p, "pm") == 0) {
	    ret = cmd_esw_ces_service_pm(unit,  service, args);
	    return ret;
#endif
	} else if (sal_strcasecmp(arg_string_p, "status") == 0) {
	    bcm_diag_esw_ces_egress_status(unit, service);
	    ret = CMD_OK;
	} else if ( sal_strcasecmp(arg_string_p, "rclock") == 0 ) {
	    if (ARG_CNT(args) >= 1) {
		/*
		 * Received clock configuration
		 */
		arg_string_p = ARG_GET(args);

		if (sal_strcasecmp(arg_string_p, "status") == 0) {
		    result = bcm_diag_esw_ces_rclock_status_get(unit, &service);
		    if (BCM_FAILURE(result))
		    {
			cli_out("Command failed.  %s.\n", bcm_errmsg(result));
			ret= CMD_FAIL;
		    }
		}
		else if (sal_strcasecmp(arg_string_p, "get") == 0) {
		    result = bcm_diag_esw_ces_rclock_config_get(unit, &service);
		    if (BCM_FAILURE(result))
		    {
			cli_out("Command failed.  %s.\n", bcm_errmsg(result));
			ret= CMD_FAIL;
		    }
		}
		else if (sal_strcasecmp(arg_string_p, "set") == 0)
		{
		    /*
		     * Service rclock set
		     */
		    bcm_ces_rclock_config_t config;
		    int port;
		    int outputbrg;
		    int rclock;
		    int type;

		    /*
		     * Note that the bcm_ces_rclock_config_t struct is packed
		     * and most of the fields are uint8 so use local ints
		     * for the parsing and then cast them into the struct after.
		     *
		     * Defaults
		     */
		    outputbrg = 0;
		    rclock = 0;
		    port = 39;
		    type = 0;
		    sal_memset(&config, 0, sizeof(bcm_ces_rclock_config_t));

		    parse_table_init(unit, &parse_table);
		    parse_table_add(&parse_table, "enable", PQ_BOOL | PQ_DFL,
				    0, &config.enable, NULL);
		    parse_table_add(&parse_table, "outputbrg", PQ_INT | PQ_DFL,
				    0, &outputbrg, NULL);
		    parse_table_add(&parse_table, "rclock", PQ_INT | PQ_DFL,
				    0, &rclock, NULL);
		    parse_table_add(&parse_table, "port", PQ_INT | PQ_DFL,
				    0, &port, NULL);
		    parse_table_add(&parse_table, "type", PQ_INT | PQ_DFL,
				    0, &type, NULL);

		    if (parse_arg_eq(args, &parse_table) < 0 || ARG_CNT(args) > 0)
		    {
			ret = parse_fail(&parse_table, ARG_CUR(args));
		    }
		    else
		    {
			config.output_brg = (uint8)outputbrg;
			config.rclock = (uint8)rclock;
			config.port = (uint8)port;
			config.recovery_type = (uint8)type;

			result = bcm_diag_esw_ces_rclock_config_set(unit, &service, &config);
			if (BCM_FAILURE(result))
			{
			    cli_out("Command failed.  %s.\n", bcm_errmsg(result));
			    ret = CMD_FAIL;
			}
		    }

		    CLEAN_UP_AND_RETURN(ret);
		}
		else
		{
		    cli_out("Invalid CES service subcommand: %s\n", arg_string_p);
		    return CMD_FAIL;
		}
	    }
	    else
	    {
		cli_out("Insuficient number of arguments. Expecting one of status|get|set.\n");
		return CMD_FAIL;
	    }
	}
	else
	{
	    cli_out("Invalid CES service subcommand: %s, expecting one of create|delete|show|rclock \n", arg_string_p);
	    return CMD_FAIL;
	}
    }

    return ret;
}



/**
 * execute ces_cmd.
 * @ param unit chip unit
 * @ a argument list 
 * @return stat.
 */
cmd_result_t cmd_esw_ces(int unit, args_t *args) {
    char *arg_string_p = NULL;
    parse_table_t parse_table;
    int result = 0;
    int ret = CMD_OK;

    arg_string_p = ARG_GET(args);

    if (arg_string_p == NULL)
    {
	return CMD_USAGE;
    }

    if (!sh_check_attached(ARG_CMD(args), unit))
    {
	return CMD_FAIL;
    }

    if (sal_strcasecmp(arg_string_p, "init") == 0)
    {
      extern int bcm_esw_ces_init(int unit);

	result = bcm_ces_services_init(unit);

	if (BCM_FAILURE(result))
	{
	    cli_out("Command failed.  %s.\n", bcm_errmsg(result));

	    return CMD_FAIL;
	}

	cli_out("CES module initialized.\n");
    } 
    else if ( sal_strcasecmp(arg_string_p, "off") == 0 ) 
    {
	/*
	 * Detach CES 
	 */
	bcm_ces_detach(unit); 
    }
    else if ( sal_strcasecmp(arg_string_p, "show") == 0 ) 
    {
	/*
	 * Show common CES config
	 */
	bcm_diag_ces_show(unit); 
    }
    else if ( sal_strcasecmp(arg_string_p, "liu") == 0 ) {
	/*
	 * LIU configuration
	 */
	return cmd_esw_ces_liu(unit, args);
#ifndef __KERNEL__
    } else if ( sal_strcasecmp(arg_string_p, "diag") == 0 ) {
	/*
	 * Diag configuration
	 */
	return cmd_esw_ces_diag(unit, args);

    } else if ( sal_strcasecmp(arg_string_p, "rpc") == 0 ) {
	/*
	 * RPC PM 
	 */
	return cmd_esw_ces_rpc(unit, args);
#endif
    } else if ( sal_strcasecmp(arg_string_p, "mii") == 0 ) {
	/*
	 * MII configuration
	 */
	return cmd_esw_ces_mii(unit, args);
    } else if ( sal_strcasecmp(arg_string_p, "cclk") == 0 ) 
    {
	/*
	 * Common clock configuration
	 */
	if (ARG_CNT(args) >= 1)
	{
	    bcm_ces_cclk_config_t config;
	    arg_string_p = ARG_GET(args);
	    ret = bcm_ces_services_cclk_config_get(unit, &config);

	    if (sal_strcasecmp(arg_string_p, "get") == 0)
	    {
		result = bcm_diag_esw_ces_cclk_config_get(unit, &config);
		if (BCM_FAILURE(result))
		{
		    cli_out("Command failed.  %s.\n", bcm_errmsg(result));
		    return CMD_FAIL;
		}
	    }
	    else if (sal_strcasecmp(arg_string_p, "set") == 0)
	    {
		ret = CMD_OK;

		/*
		 * Set default values
		 */
		parse_table_init(unit, &parse_table);
		parse_table_add(&parse_table, "enable", PQ_BOOL | PQ_DFL,
				0, &config.cclk_enable, NULL);
		parse_table_add(&parse_table, "select", PQ_INT | PQ_DFL,
				0, &config.cclk_select, NULL);
		parse_table_add(&parse_table, "c1_sel", PQ_INT | PQ_DFL,
				0, &config.ref_clk_1_select, NULL);
		parse_table_add(&parse_table, "c2_sel", PQ_INT | PQ_DFL,
				0, &config.ref_clk_2_select, NULL);
		parse_table_add(&parse_table, "c1_port", PQ_INT | PQ_DFL,
				0, &config.ref_clk_1_port, NULL);
		parse_table_add(&parse_table, "c2_port", PQ_INT | PQ_DFL,
				0, &config.ref_clk_2_port, NULL);

		if (parse_arg_eq(args, &parse_table) < 0 || ARG_CNT(args) > 0)
		{
		    ret = parse_fail(&parse_table, ARG_CUR(args));
		}
		else
		{
		    ret = bcm_diag_esw_ces_cclk_config_set(unit, &config);
		    if (BCM_FAILURE(ret))
		    {
			cli_out("Command failed.  %s.\n", bcm_errmsg(result));
			ret = CMD_FAIL;
		    }
		}
		CLEAN_UP_AND_RETURN(ret);
	    }
	    else
	    {
		cli_out("Invalid CES service subcommand: %s\n", arg_string_p);
		return CMD_FAIL;
	    }
	}
	else
	{
	    cli_out("Insuficient number of arguments. Expecting at least get|set.\n");
	    return CMD_FAIL;
	}
    }
    else if ( sal_strcasecmp(arg_string_p, "service") == 0 ) 
    {
	cmd_esw_ces_service(unit, args);
    }
    else if ( sal_strcasecmp(arg_string_p, "tdm") == 0 ) 
    {
        /*
	 * TDM configuration
	 */
	return cmd_esw_ces_tdm(unit, args);
#ifndef __KERNEL__
    } else if ( sal_strcasecmp(arg_string_p, "test") == 0 ) 
    {
        /*
	 * Test setup
	 */
	return cmd_esw_ces_test(unit, args);
#endif
    } else if ( sal_strcasecmp(arg_string_p, "help") == 0 ) {
	/*
	 * Help
	 */
	if (ARG_CNT(args) >= 1) {
	    arg_string_p = ARG_GET(args);

	    if (arg_string_p == NULL)
		return CMD_USAGE;

	    if ( sal_strcasecmp(arg_string_p, "init") == 0 ) 
		cli_out("%s\n", bcm_diag_cmd_esw_ces_init_usage);
	    else if ( sal_strcasecmp(arg_string_p, "show") == 0 ) 
		cli_out("%s\n", bcm_diag_cmd_esw_ces_show_usage);
	    else if ( sal_strcasecmp(arg_string_p, "diag") == 0 ) 
		cli_out("%s\n", bcm_diag_cmd_esw_ces_diag_usage);
	    else if ( sal_strcasecmp(arg_string_p, "cclk") == 0 ) 
		cli_out("%s\n", bcm_diag_cmd_esw_ces_cclk_usage);
	    else if ( sal_strcasecmp(arg_string_p, "service") == 0 ) 
		cli_out("%s\n", bcm_diag_cmd_esw_ces_service_usage);
	    else if ( sal_strcasecmp(arg_string_p, "tdm") == 0 ) 
		cli_out("%s\n", bcm_diag_cmd_esw_ces_tdm_usage);
	    else if ( sal_strcasecmp(arg_string_p, "liu") == 0 ) 
		cli_out("%s\n", bcm_diag_cmd_esw_ces_liu_usage);
	    else if ( sal_strcasecmp(arg_string_p, "rpc") == 0 ) 
		cli_out("%s\n", bcm_diag_cmd_esw_ces_rpc_usage);
	    else if ( sal_strcasecmp(arg_string_p, "test") == 0 ) 
		cli_out("%s\n", bcm_diag_cmd_esw_ces_test_usage);
	    else if ( sal_strcasecmp(arg_string_p, "help") == 0 ) 
		cli_out("%s\n", bcm_diag_cmd_esw_ces_help_usage);
	    else
		return CMD_USAGE;

	} else {
	    return CMD_USAGE;
	}

    } else {
	cli_out("Invalid CES subcommand: %s\n", arg_string_p);
	return CMD_FAIL;
    }

    return CMD_OK;
}


#else
    int  __no_complilation_complaints_about_ces__1;
#endif /* INCLUDE_CES */
