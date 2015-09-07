/*
 * $Id: gport.h 1.70.2.1 Broadcom SDK $
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
 * This file defines gport (generic port) parameters. The gport is useful
 * for specifying the following types of identifiers:
 *   LOCAL      :  port on the local unit
 *   MODPORT    :  {module ID, port} pair
 *   TRUNK      :  trunk ID
 *   PROXY      :  {module ID, port} pair employed in proxy operations
 *   BLACK_HOLE :  identifier indicating drop destination
 *   LOCAL_CPU  :  identifier indicating local CPU destination
 *   MPLS_PORT  :  L2 MPLS virtual-port (VPLS/VPWS)
 *   SUBPORT_GROUP : Subport group
 *   SUBPORT_PORT  : Subport virtual-port
 *   UCAST_QUEUE_GROUP : Group of unicast COSQs
 *   MCAST_QUEUE_GROUP : Group of multicast COSQs
 *   UCAST_SUBSCRIBER_QUEUE_GROUP : Group of 64K subscriber unicast COSQs
 *   MCAST_SUBSCRIBER_QUEUE_GROUP : Group of 64K subscriber multicast COSQs
 *   MCAST :  distribution set identifier
 *   SCHEDULER  : COSQ scheduler
 *   DEVPORT    : {device ID, port} pair (ports on devices without a module ID)
 *   SPECIAL    :  application special value (invalid in BCM APIs)
 *   MIRROR     :  Mirror (modport & encapsulation) for mirrored traffic.
 *   MIM_PORT   :  MIM virtual-port
 *   VLAN_PORT  :  VLAN virtual-port
 *   WLAN_PORT  :  WLAN virtual-port
 *   TRILL_PORT :  TRILL virtual-port
 *   NIV_PORT   :  NIV virtual-port
 *   EXTENDER_PORT :  Extender virtual-port
 *   MAC_PORT   :  MAC virtual-port
 *   TUNNEL     :  Tunnel ID
 *   MULTIPATH  :  Multipath shaper
 *
 * This header requires that the uint32 type be defined before inclusion.
 * Using <sal/types.h> is the simplest (and recommended) way of doing this.
 *
 * Its contents are not used directly by applications; it is used only
 * by header files of parent APIs which need to define gport parameters.
 *
 * The following macros are made available.  All have names starting
 * with _SHR_, which have been elided from this list:
 *
 * Constants or Expressions suitable for assignment:
 * GPORT_NONE                   gport initializer
 * GPORT_INVALID                invalid gport identifier
 * GPORT_BLACK_HOLE             black-hole gport idenitifier
 * GPORT_LOCAL_CPU              local CPU gport identifier
 *
 * Predicates: (return 0 or 1, suitable for using in if statements)
 * GPORT_IS_SET                 is the gport set?
 * GPORT_IS_LOCAL               is the gport a local port type?
 * GPORT_IS_MODPORT             is the gport a modid/port type?
 * GPORT_IS_TRUNK               is the gport a trunk type?
 * GPORT_IS_BLACK_HOLE          is the gport a black-hole type?
 * GPORT_IS_LOCAL_CPU           is the gport a local CPU type?
 * GPORT_IS_MPLS_PORT           is the gport a MPLS port type?
 * GPORT_IS_SUBPORT_GROUP       is the gport a subport group type?
 * GPORT_IS_SUBPORT_PORT        is the gport a subport port type?
 * GPORT_IS_UCAST_QUEUE_GROUP   is the gport a unicast group type?
 * GPORT_IS_UCAST_SUBSCRIBER_QUEUE_GROUP   is the gport a subscriber unicast group type?
 * GPORT_IS_MCAST               is the gport a multicast set type?
 * GPORT_IS_MCAST_QUEUE_GROUP   is the gport a multicast queue group type?
 * GPORT_IS_MCAST_SUBSCRIBER_QUEUE_GROUP   is the gport a multicast subscriber queue group type?
 * GPORT_IS_SCHEDULER           is the gport a scheduler type?
 * GPORT_IS_DEVPORT             is the gport a devid/port type?
 * GPORT_IS_SPECIAL             is the gport a special type?
 * GPORT_IS_MIRROR              is the gport a mirror destination type?
 * GPORT_IS_MIM_PORT            is the gport a MIM port type?
 * GPORT_IS_VLAN_PORT           is the gport a VLAN port type?
 * GPORT_IS_WLAN_PORT           is the gport a WLAN port type?
 * GPORT_IS_TRILL_PORT          is the gport a TRILL port type?
 * GPORT_IS_NIV_PORT            is the gport a NIV port type?
 * GPORT_IS_EXTENDER_PORT       is the gport an Extender port type?
 * EXTENDER_GPORT_IS_PORT       is the extender gport a port subtype?
 * EXTENDER_GPORT_IS_FORWARD    is the extender gport a port forward?
 * EXTENDER_GPORT_IS_ENCAP      is the extender gport a port encap?
 * GPORT_IS_MAC_PORT            is the gport a MAC port type?
 * GPORT_IS_TUNNEL              is the gport a tunnel type?
 * GPORT_IS_CHILD               is the gport a child port type?
 * GPORT_IS_EGRESS_GROUP        is the gport a egress group type?
 * GPORT_IS_EGRESS_CHILD        is the gport a egress child type?
 * GPORT_IS_EGRESS_MODPORT      is the gport a egress modport type?
 * COSQ_GPORT_IS_MULTIPATH      is the gport a multipath type?
 *
 * Statements: (cannot be used as a predicate)
 * GPORT_LOCAL_SET              set a gport local port type and value
 * GPORT_LOCAL_GET              get a port value from a local gport
 * GPORT_LOCAL_CPU_SET          set a gport local CPU port type and value
 * GPORT_LOCAL_CPU_GET          get a CPU port value from a local gport
 * GPORT_MODPORT_SET            set a gport modid/port type and values
 * GPORT_MODPORT_MODID_GET      get a modid value from a modport gport
 * GPORT_MODPORT_PORT_GET       get a port value from a modport gport
 * GPORT_TRUNK_SET              set a gport trunk type and value
 * GPORT_TRUNK_GET              get a trunk_id value from a trunk gport
 * GPORT_MPLS_PORT_ID_SET       set a MPLS ID type and value
 * GPORT_MPLS_PORT_ID_GET       get a MPLS ID from a MPLS gport
 * GPORT_SUBPORT_GROUP_SET      set a subport group type and value
 * GPORT_SUBPORT_GROUP_GET      get a subport group ID from a gport
 * GPORT_SUBPORT_PORT_SET       set a subport port type and value
 * GPORT_SUBPORT_PORT_GET       get a subport port ID from a gport
 * GPORT_SCHEDULER_SET          set a scheduler type and value
 * GPORT_SCHEDULER_GET          get a scheduler ID from a gport
 * GPORT_DEVPORT_SET            set a gport devid/port type and values
 * GPORT_DEVPORT_DEVID_GET      get a devid value from a devport gport
 * GPORT_DEVPORT_PORT_GET       get a port value from a devport gport
 * GPORT_UCAST_QUEUE_GROUP_SET  set a queue group type and value
 * GPORT_UCAST_QUEUE_GROUP_GET  get a queue group ID from a gport
 * GPORT_MCAST_SET              set a mcast dist set and value
 * GPORT_MCAST_GET              get an mcast dist set from a gport
 * GPORT_MCAST_DS_ID_SET        set an mcast DS ID and value
 * GPORT_MCAST_DS_ID_GET        get an mcast DS ID from a gport
 * GPORT_MCAST_QUEUE_GROUP_SET  set an mcast queue group type and value
 * GPORT_MCAST_QUEUE_GROUP_GET  get an mcast queue group ID from a gport
 * GPORT_MCAST_QUEUE_GROUP_SYSQID_SET set an mcast queue group system queue ID
 * GPORT_MCAST_QUEUE_GROUP_SYSPORTID_GET get an mcast queue group system port ID
 * GPORT_MCAST_QUEUE_GROUP_QID_GET get an mcast queue group queue ID
 * GPORT_UCAST_QUEUE_GROUP_SYSQID_SET get a unicast queue group system queue ID
 * GPORT_UCAST_QUEUE_GROUP_SYSPORTID_GET get a unicast queue group system port ID
 * GPORT_UCAST_QUEUE_GROUP_QID_GET get a unicast queue group queue ID
 * GPORT_UCAST_SUBSCRIBER_QUEUE_GROUP_QID_SET set a ucast subscriber queue group queue id
 * GPORT_UCAST_SUBSCRIBER_QUEUE_GROUP_QID_GET get a ucast subscriber queue group queue id
 * GPORT_MCAST_SUBSCRIBER_QUEUE_GROUP_QID_SET set an mcast subscriber queue group queue id
 * GPORT_MCAST_SUBSCRIBER_QUEUE_GROUP_QID_GET get an mcast subscriber queue group queue id
 * GPORT_SPECIAL_SET            set a gport special type and value
 * GPORT_SPECIAL_GET            get a value from a special gport
 * GPORT_MIRROR_SET             set a gport mirror type and value
 * GPORT_MIRROR_GET             get a value from a mirror gport
 * GPORT_MIM_PORT_ID_SET        set a MIM ID type and value
 * GPORT_MIM_PORT_ID_GET        get a MIM ID from a Mim gport
 * GPORT_VLAN_PORT_ID_SET       set a VLAN port ID type and value
 * GPORT_VLAN_PORT_ID_GET       get a VLAN port ID from a VLAN gport
 * GPORT_WLAN_PORT_ID_SET       set a WLAN ID type and value
 * GPORT_WLAN_PORT_ID_GET       get a WLAN ID from a WLAN gport
 * GPORT_TRILL_PORT_ID_SET      set a TRILL ID type and value
 * GPORT_TRILL_PORT_ID_GET      get a TRILL ID from a Trill gport
 * GPORT_NIV_PORT_ID_SET        set a NIV ID type and value 
 * GPORT_NIV_PORT_ID_GET        get a NIV ID from a NIV gport
 * EXTENDER_GPORT_PORT_SET      set an Extender type, Port subtype and ID value
 * EXTENDER_GPORT_FORWARD_SET   set an Extender type, Forward subtype and ID value
 * EXTENDER_GPORT_ENCAP_SET     set an Extender type, Encap subtype and ID value
 * GPORT_EXTENDER_PORT_ID_SET   same as EXTENDER_GPORT_PORT_SET for backward compatability
 * GPORT_EXTENDER_PORT_ID_GET   get an Extender ID from an Extender gport
 * GPORT_MAC_PORT_ID_SET        set a MAC ID type and value 
 * GPORT_MAC_PORT_ID_GET        get a MAC ID from an MAC gport
 * GPORT_TUNNEL_ID_SET          set a gport tunnel type and value
 * GPORT_TUNNEL_ID_GET          get a tunnel_id value from a tunnel gport
 * GPORT_CHILD_SET              set a child gport
 * GPORT_CHILD_MODID_GET        get a child gport modid
 * GPORT_CHILD_PORT_GET         get a child gport port
 * GPORT_EGRESS_GROUP_SET       set an egress group
 * GPORT_EGRESS_GROUP_GET       get an egress group
 * GPORT_EGRESS_GROUP_MODID_GET get an egress group modid
 * GPORT_EGRESS_CHILD_SET       set an egress child gport
 * GPORT_EGRESS_CHILD_MODID_GET get an egress child gport modid
 * GPORT_EGRESS_CHILD_PORT_GET  get an egress child gport port
 * GPORT_EGRESS_MODPORT_SET     set an egress modport
 * GPORT_EGRESS_MODPORT_MODID_GET get an egress modport modid
 * GPORT_EGRESS_MODPORT_PORT_GET  get an egress modport port
 * GPORT_MULTIPATH_SET          set an multipath port id
 * GPORT_MULTIPATH_GET          get an multipath port id
 */

#ifndef _SHR_GPORT_H
#define _SHR_GPORT_H
/*
 * Typedef:
 *    _shr_gport_t
 * Purpose:
 *    GPORT (generic port) type for shared definitions
 */
typedef uint32 _shr_gport_t;

/*
 * Defines:
 *     _SHR_GPORT_*
 * Purpose:
 *     GPORT (Generic port) definitions. GPORT can be used to specify
 *     a {module, port} pair, trunk, and other port types.
 */

#define _SHR_GPORT_NONE             (0)
#define _SHR_GPORT_INVALID         (-1)

#define _SHR_GPORT_TYPE_LOCAL               1  /* Port on local unit     */
#define _SHR_GPORT_TYPE_MODPORT             2  /* Module ID and port     */
#define _SHR_GPORT_TYPE_TRUNK               3  /* Trunk ID               */
#define _SHR_GPORT_TYPE_BLACK_HOLE          4  /* Black hole destination */
#define _SHR_GPORT_TYPE_LOCAL_CPU           5  /* CPU destination        */
#define _SHR_GPORT_TYPE_MPLS_PORT           6  /* MPLS port ID           */
#define _SHR_GPORT_TYPE_SUBPORT_GROUP       7  /* Subport group ID       */
#define _SHR_GPORT_TYPE_SUBPORT_PORT        8  /* Subport port ID        */
#define _SHR_GPORT_TYPE_UCAST_QUEUE_GROUP   9  /* Queue Group            */
#define _SHR_GPORT_TYPE_DEVPORT            10  /* Device ID and port     */
#define _SHR_GPORT_TYPE_MCAST              11  /* Multicast Set          */
#define _SHR_GPORT_TYPE_MCAST_QUEUE_GROUP  12  /* Multicast Queue Group  */
#define _SHR_GPORT_TYPE_SCHEDULER          13  /* Scheduler              */
#define _SHR_GPORT_TYPE_SPECIAL            14  /* Special value          */
#define _SHR_GPORT_TYPE_MIRROR             15  /* Mirror destination.    */
#define _SHR_GPORT_TYPE_MIM_PORT           16  /* MIM port ID            */
#define _SHR_GPORT_TYPE_VLAN_PORT          17  /* VLAN port              */
#define _SHR_GPORT_TYPE_WLAN_PORT          18  /* WLAN port              */
#define _SHR_GPORT_TYPE_TUNNEL             19  /* Tunnel ID              */
#define _SHR_GPORT_TYPE_CHILD              20  /* Child port             */
#define _SHR_GPORT_TYPE_EGRESS_GROUP       21  /* Egress group port      */
#define _SHR_GPORT_TYPE_EGRESS_CHILD       22  /* Egress child port      */
#define _SHR_GPORT_TYPE_EGRESS_MODPORT     23  /* Egress Mod and port    */
#define _SHR_GPORT_TYPE_UCAST_SUBSCRIBER_QUEUE_GROUP  24  /* Local Queue Group */
#define _SHR_GPORT_TYPE_MCAST_SUBSCRIBER_QUEUE_GROUP  25  /* Local Multicast Queue Group  */
#define _SHR_GPORT_TYPE_TRILL_PORT         26  /* TRILL port */  
#define _SHR_GPORT_TYPE_SYSTEM_PORT        27  /* DPP System-Port        */ 
#define _SHR_GPORT_TYPE_NIV_PORT           28  /* NIV port */  
#define _SHR_GPORT_TYPE_CONGESTION         29  /* congestion port, e.g. out of band HCFC port */
#define _SHR_GPORT_TYPE_COSQ               30  /* cosq gport */ 
#define _SHR_GPORT_TYPE_L2GRE_PORT         31  /* L2GRE gport */ 
#define _SHR_GPORT_TYPE_VXLAN_PORT         32  /* VXLAN gport */ 
#define _SHR_GPORT_TYPE_EPON_LINK          33  /* EPON link gport */
#define _SHR_GPORT_TYPE_PHY                34  /* Phy gport multiple types */
#define _SHR_GPORT_TYPE_EXTENDER_PORT      35  /* Extender port */  
#define _SHR_GPORT_TYPE_MAC_PORT           36  /* MAC port */  
#define _SHR_GPORT_TYPE_PROXY              37  /* Proxy (module ID, port) source or destination */  
#define _SHR_GPORT_TYPE_FORWARD_PORT       38  /* Gport for fec destinations */
#define _SHR_GPORT_TYPE_MAX _SHR_GPORT_TYPE_FORWARD_PORT /* Used for sanity */
                                                /* checks only.    */
/* COSQ GPORT subtypes*/
#define _SHR_COSQ_GPORT_TYPE_VOQ_CONNECTOR       1
#define _SHR_COSQ_GPORT_TYPE_UCAST_EGRESS_QUEUE  2
#define _SHR_COSQ_GPORT_TYPE_MCAST_EGRESS_QUEUE  3
#define _SHR_COSQ_GPORT_TYPE_VSQ                 4
#define _SHR_COSQ_GPORT_TYPE_E2E_PORT            5
#define _SHR_COSQ_GPORT_TYPE_ISQ                 6
#define _SHR_COSQ_GPORT_TYPE_MULTIPATH           7
#define _SHR_COSQ_GPORT_TYPE_SCHED_CIR           8
#define _SHR_COSQ_GPORT_TYPE_SCHED_PIR           9
#define _SHR_COSQ_GPORT_TYPE_COMPOSITE_SF2       10
#define _SHR_COSQ_GPORT_TYPE_PORT_TC             11
#define _SHR_COSQ_GPORT_TYPE_PORT_TCG            12
#define _SHR_COSQ_GPORT_TYPE_E2E_PORT_TC         13
#define _SHR_COSQ_GPORT_TYPE_E2E_PORT_TCG        14
#define _SHR_COSQ_GPORT_TYPE_SYSTEM_RED          15

#define _SHR_COSQ_GPORT_RESERVED_VOQ_CONNECTOR 1
#define _SHR_COSQ_GPORT_RESERVED_SCHEDULER     0

/* Extender GPORT subtypes*/
#define _SHR_EXTENDER_GPORT_TYPE_PORT               (0)
#define _SHR_EXTENDER_GPORT_TYPE_FORWARD            (1)
#define _SHR_EXTENDER_GPORT_TYPE_ENCAP              (2)
 

#define _SHR_GPORT_TYPE_SHIFT                           26
#define _SHR_GPORT_TYPE_MASK                            0x3f
#define _SHR_GPORT_MODID_SHIFT                          11
#define _SHR_GPORT_MODID_MASK                           0x7fff
#define _SHR_GPORT_PORT_SHIFT                           0
#define _SHR_GPORT_PORT_MASK                            0x7ff
#define _SHR_GPORT_TRUNK_SHIFT                          0
#define _SHR_GPORT_TRUNK_MASK                           0x3ffffff
#define _SHR_GPORT_MPLS_PORT_SHIFT                      0
#define _SHR_GPORT_MPLS_PORT_MASK                       0xffffff
#define _SHR_GPORT_SUBPORT_GROUP_SHIFT                  0
#define _SHR_GPORT_SUBPORT_GROUP_MASK                   0xffffff
#define _SHR_GPORT_SUBPORT_PORT_SHIFT                   0
#define _SHR_GPORT_SUBPORT_PORT_MASK                    0xffffff
#define _SHR_GPORT_UCAST_QUEUE_GROUP_SHIFT              0
#define _SHR_GPORT_UCAST_QUEUE_GROUP_MASK               0x3ffffff
#define _SHR_GPORT_UCAST_QUEUE_GROUP_QID_SHIFT          0
#define _SHR_GPORT_UCAST_QUEUE_GROUP_QID_MASK           0x3fff
#define _SHR_GPORT_UCAST_QUEUE_GROUP_SYSPORTID_SHIFT    14
#define _SHR_GPORT_UCAST_QUEUE_GROUP_SYSPORTID_MASK     0xfff
#define _SHR_GPORT_UNICAST_QUEUE_GROUP_QID_SHIFT        0
#define _SHR_GPORT_UNICAST_QUEUE_GROUP_QID_MASK         0x1ffff
#define _SHR_GPORT_UCAST_SUBSCRIBER_QUEUE_GROUP_QID_SHIFT 0
#define _SHR_GPORT_UCAST_SUBSCRIBER_QUEUE_GROUP_QID_MASK  0xffff
#define _SHR_GPORT_MCAST_SHIFT                          0
#define _SHR_GPORT_MCAST_MASK                           0x3ffffff
#define _SHR_GPORT_MCAST_DS_ID_SHIFT                    0
#define _SHR_GPORT_MCAST_DS_ID_MASK                     0x3ffffff
#define _SHR_GPORT_MCAST_QUEUE_GROUP_SHIFT              0
#define _SHR_GPORT_MCAST_QUEUE_GROUP_MASK               0x3ffffff
#define _SHR_GPORT_MCAST_QUEUE_GROUP_QID_SHIFT          0
#define _SHR_GPORT_MCAST_QUEUE_GROUP_QID_MASK           0x3fff
#define _SHR_GPORT_MCAST_QUEUE_GROUP_SYSPORTID_SHIFT    14
#define _SHR_GPORT_MCAST_QUEUE_GROUP_SYSPORTID_MASK     0xfff
#define _SHR_GPORT_MCAST_SUBSCRIBER_QUEUE_GROUP_QID_SHIFT 0
#define _SHR_GPORT_MCAST_SUBSCRIBER_QUEUE_GROUP_QID_MASK  0x3fff
#define _SHR_GPORT_SCHEDULER_SHIFT                      0
#define _SHR_GPORT_SCHEDULER_MASK                       0x3ffffff
#define _SHR_GPORT_SCHEDULER_LEVEL_SHIFT                20
#define _SHR_GPORT_SCHEDULER_LEVEL_MASK                 0x1f
#define _SHR_GPORT_SCHEDULER_NODE_SHIFT                 0
#define _SHR_GPORT_SCHEDULER_NODE_MASK                  0xfffff
#define _SHR_GPORT_DEVID_SHIFT                          _SHR_GPORT_MODID_SHIFT
#define _SHR_GPORT_DEVID_MASK                           _SHR_GPORT_MODID_MASK
#define _SHR_GPORT_SPECIAL_SHIFT                        0
#define _SHR_GPORT_SPECIAL_MASK                         0xffff
#define _SHR_GPORT_MIRROR_SHIFT                         0
#define _SHR_GPORT_MIRROR_MASK                          0xffff
#define _SHR_GPORT_MIM_PORT_SHIFT                       0
#define _SHR_GPORT_MIM_PORT_MASK                        0xffffff
#define _SHR_GPORT_VLAN_PORT_SHIFT                      0
#define _SHR_GPORT_VLAN_PORT_MASK                       0xffffff
#define _SHR_GPORT_WLAN_PORT_SHIFT                      0
#define _SHR_GPORT_WLAN_PORT_MASK                       0xffffff
#define _SHR_GPORT_TRILL_PORT_SHIFT                     0
#define _SHR_GPORT_TRILL_PORT_MASK                      0xffffff
#define _SHR_GPORT_NIV_PORT_SHIFT                       0
#define _SHR_GPORT_NIV_PORT_MASK                        0xffffff
#define _SHR_GPORT_L2GRE_PORT_SHIFT                   0
#define _SHR_GPORT_L2GRE_PORT_MASK                    0xffffff
#define _SHR_GPORT_VXLAN_PORT_SHIFT                   0
#define _SHR_GPORT_VXLAN_PORT_MASK                    0xffffff
#define _SHR_GPORT_MAC_PORT_SHIFT                       0
#define _SHR_GPORT_MAC_PORT_MASK                        0xffffff
#define _SHR_GPORT_TUNNEL_SHIFT                         0
#define _SHR_GPORT_TUNNEL_MASK                          0x3ffffff
#define _SHR_GPORT_CHILD_SHIFT                          0
#define _SHR_GPORT_CHILD_MASK                           0x3ffffff
#define _SHR_GPORT_CHILD_MODID_SHIFT                    10
#define _SHR_GPORT_CHILD_MODID_MASK                     0xffff
#define _SHR_GPORT_CHILD_PORT_SHIFT                     0
#define _SHR_GPORT_CHILD_PORT_MASK                      0x3ff
#define _SHR_GPORT_EGRESS_GROUP_MODID_SHIFT             10
#define _SHR_GPORT_EGRESS_GROUP_MODID_MASK              0xffff
#define _SHR_GPORT_EGRESS_GROUP_SHIFT                   0
#define _SHR_GPORT_EGRESS_GROUP_MASK                    0x3ff
#define _SHR_GPORT_EGRESS_CHILD_SHIFT                   0
#define _SHR_GPORT_EGRESS_CHILD_MASK                    0x3ffffff
#define _SHR_GPORT_EGRESS_CHILD_PORT_SHIFT              0
#define _SHR_GPORT_EGRESS_CHILD_PORT_MASK               0x3ff
#define _SHR_GPORT_EGRESS_CHILD_MODID_SHIFT             10
#define _SHR_GPORT_EGRESS_CHILD_MODID_MASK              0xffff
#define _SHR_GPORT_EGRESS_MODPORT_SHIFT                 0
#define _SHR_GPORT_EGRESS_MODPORT_MASK                  0x3ffffff
#define _SHR_GPORT_EGRESS_MODPORT_MODID_SHIFT           10
#define _SHR_GPORT_EGRESS_MODPORT_MODID_MASK            0xffff
#define _SHR_GPORT_EGRESS_MODPORT_PORT_SHIFT            0
#define _SHR_GPORT_EGRESS_MODPORT_PORT_MASK             0x3ff
#define _SHR_GPORT_CONGESTION_SHIFT                     0
#define _SHR_GPORT_CONGESTION_MASK                      0xff
#define _SHR_GPORT_SYSTEM_PORT_SHIFT                    0
#define _SHR_GPORT_SYSTEM_PORT_MASK                     0xffffff
#define _SHR_GPORT_EPON_LINK_SHIFT                      0
#define _SHR_GPORT_EPON_LINK_MASK                       0xffff
#define _SHR_GPORT_EPON_LINK_NUM_SHIFT                  0
#define _SHR_GPORT_EPON_LINK_NUM_MASK                   0xffff
#define _SHR_GPORT_FORWARD_PORT_SHIFT                   0
#define _SHR_GPORT_FORWARD_PORT_MASK                    0xffffff

#define _SHR_ENCAP_ID_SHIFT                             0
#define _SHR_ENCAP_ID_MASK                              0xfffffff
#define _SHR_ENCAP_REMOTE_SHIFT                         28
#define _SHR_ENCAP_REMOTE_MASK                          0x1



#define _SHR_GPORT_ISQ_ROOT                             (_SHR_GPORT_SCHEDULER_MASK - 1)
#define _SHR_GPORT_FMQ_ROOT                             (_SHR_GPORT_SCHEDULER_MASK - 2)
#define _SHR_GPORT_FMQ_GUARANTEED                       (_SHR_GPORT_SCHEDULER_MASK - 3)
#define _SHR_GPORT_FMQ_BESTEFFORT_AGR                   (_SHR_GPORT_SCHEDULER_MASK - 4)
#define _SHR_GPORT_FMQ_BESTEFFORT0                      (_SHR_GPORT_SCHEDULER_MASK - 5)
#define _SHR_GPORT_FMQ_BESTEFFORT1                      (_SHR_GPORT_SCHEDULER_MASK - 6)
#define _SHR_GPORT_FMQ_BESTEFFORT2                      (_SHR_GPORT_SCHEDULER_MASK - 7)
#define _SHR_GPORT_FABRIC_MESH_LOCAL                    (_SHR_GPORT_SCHEDULER_MASK - 8)
#define _SHR_GPORT_FABRIC_MESH_DEV1                     (_SHR_GPORT_SCHEDULER_MASK - 9)
#define _SHR_GPORT_FABRIC_MESH_DEV2                     (_SHR_GPORT_SCHEDULER_MASK - 10)
#define _SHR_GPORT_FABRIC_MESH_DEV3                     (_SHR_GPORT_SCHEDULER_MASK - 11)
#define _SHR_GPORT_FABRIC_MESH_DEV4                     (_SHR_GPORT_SCHEDULER_MASK - 12)
#define _SHR_GPORT_FABRIC_MESH_DEV5                     (_SHR_GPORT_SCHEDULER_MASK - 13)
#define _SHR_GPORT_FABRIC_MESH_DEV6                     (_SHR_GPORT_SCHEDULER_MASK - 14)
#define _SHR_GPORT_FABRIC_MESH_DEV7                     (_SHR_GPORT_SCHEDULER_MASK - 15)
#define _SHR_GPORT_FABRIC_CLOS_UNICAST_LOCAL            (_SHR_GPORT_SCHEDULER_MASK - 16)
#define _SHR_GPORT_FABRIC_CLOS_UNICAST_LOCAL_LOW        (_SHR_GPORT_SCHEDULER_MASK - 17)
#define _SHR_GPORT_FABRIC_CLOS_UNICAST_LOCAL_HIGH       (_SHR_GPORT_SCHEDULER_MASK - 18)
#define _SHR_GPORT_FABRIC_CLOS_FABRIC                   (_SHR_GPORT_SCHEDULER_MASK - 19)
#define _SHR_GPORT_FABRIC_CLOS_FABRIC_HIGH              (_SHR_GPORT_SCHEDULER_MASK - 20)
#define _SHR_GPORT_FABRIC_CLOS_FABRIC_LOW               (_SHR_GPORT_SCHEDULER_MASK - 21)
#define _SHR_GPORT_FABRIC_CLOS_UNICAST_FABRIC_HIGH      (_SHR_GPORT_SCHEDULER_MASK - 22)
#define _SHR_GPORT_FABRIC_CLOS_UNICAST_FABRIC_LOW       (_SHR_GPORT_SCHEDULER_MASK - 23)
#define _SHR_GPORT_FABRIC_CLOS_FMQ_GUARANTEED           (_SHR_GPORT_SCHEDULER_MASK - 24)
#define _SHR_GPORT_FABRIC_CLOS_FMQ_BESTEFFORT           (_SHR_GPORT_SCHEDULER_MASK - 25)
#define _SHR_GPORT_FMQ_CLASS1                           (_SHR_GPORT_SCHEDULER_MASK - 26)
#define _SHR_GPORT_FMQ_CLASS2                           (_SHR_GPORT_SCHEDULER_MASK - 27)
#define _SHR_GPORT_FMQ_CLASS3                           (_SHR_GPORT_SCHEDULER_MASK - 28)
#define _SHR_GPORT_FMQ_CLASS4                           (_SHR_GPORT_SCHEDULER_MASK - 29)

/* COSQ GPORT subtype shift & mask definitions */
#define _SHR_COSQ_GPORT_TYPE_SHIFT                      21
#define _SHR_COSQ_GPORT_TYPE_MASK                       0x1F
#define _SHR_COSQ_GPORT_UCAST_EGRESS_QUEUE_PORT_MASK    0x7FF
#define _SHR_COSQ_GPORT_UCAST_EGRESS_QUEUE_PORT_SHIFT   0
#define _SHR_COSQ_GPORT_MCAST_EGRESS_QUEUE_PORT_MASK    0x7FF
#define _SHR_COSQ_GPORT_MCAST_EGRESS_QUEUE_PORT_SHIFT   0
#define _SHR_COSQ_GPORT_VSQ_TYPE_MASK                   0x7F
#define _SHR_COSQ_GPORT_VSQ_TYPE_SHIFT                  12
#define _SHR_COSQ_GPORT_VSQ_TYPE_INFO_MASK              0xFFF
#define _SHR_COSQ_GPORT_VSQ_TYPE_INFO_SHIFT             0
#define _SHR_VSQ_TYPE_INFO_CATEGORY_MASK                0x3
#define _SHR_VSQ_TYPE_INFO_CATEGORY_SHIFT               5
#define _SHR_VSQ_TYPE_INFO_TC_MASK                      0x7
#define _SHR_VSQ_TYPE_INFO_TC_SHIFT                     0
#define _SHR_VSQ_TYPE_INFO_CONNECTION_MASK             0xF
#define _SHR_VSQ_TYPE_INFO_CONNECTION_SHIFT             0
#define _SHR_VSQ_TYPE_INFO_SRC_PORT_MASK                0xFF
#define _SHR_VSQ_TYPE_INFO_SRC_PORT_SHIFT               4
#define _SHR_COSQ_GPORT_E2E_PORT_MASK                   0x7FF
#define _SHR_COSQ_GPORT_E2E_PORT_SHIFT                  0
#define _SHR_COSQ_GPORT_ISQ_QID_SHIFT                   0
#define _SHR_COSQ_GPORT_ISQ_QID_MASK                    0x1FFFF
#define _SHR_COSQ_GPORT_MULTIPATH_SHIFT                 0
#define _SHR_COSQ_GPORT_MULTIPATH_MASK                  0x7fffff
#define _SHR_COSQ_GPORT_SCHED_CIR_SHIFT                 0
#define _SHR_COSQ_GPORT_SCHED_CIR_MASK                  0xFFFFF
#define _SHR_COSQ_GPORT_SCHED_PIR_SHIFT                 0
#define _SHR_COSQ_GPORT_SCHED_PIR_MASK                  0xFFFFF
#define _SHR_COSQ_GPORT_ATTACH_ID_SYSPORT_SHIFT         0
#define _SHR_COSQ_GPORT_ATTACH_ID_SYSPORT_MASK          0xfff
#define _SHR_COSQ_GPORT_ATTACH_ID_FCD_SHIFT             12
#define _SHR_COSQ_GPORT_ATTACH_ID_FCD_MASK              0x3ff
#define _SHR_COSQ_GPORT_VOQ_CONNECTOR_ID_SHIFT          0
#define _SHR_COSQ_GPORT_VOQ_CONNECTOR_ID_MASK           0xFFFFF
#define _SHR_COSQ_GPORT_COMPOSITE_SF2_FLOW_SHIFT        0
#define _SHR_COSQ_GPORT_COMPOSITE_SF2_FLOW_MASK         0x3FFFF
#define _SHR_COSQ_GPORT_COMPOSITE_SF2_RESERVED_SHIFT    20
#define _SHR_COSQ_GPORT_COMPOSITE_SF2_RESERVED_MASK     0x1
#define _SHR_COSQ_GPORT_PORT_TC_SHIFT                   0
#define _SHR_COSQ_GPORT_PORT_TC_MASK                    0x7FF
#define _SHR_COSQ_GPORT_PORT_TCG_SHIFT                  0
#define _SHR_COSQ_GPORT_PORT_TCG_MASK                   0x7FF
#define _SHR_COSQ_GPORT_E2E_PORT_TC_SHIFT               0
#define _SHR_COSQ_GPORT_E2E_PORT_TC_MASK                0x7FF
#define _SHR_COSQ_GPORT_E2E_PORT_TCG_SHIFT              0
#define _SHR_COSQ_GPORT_E2E_PORT_TCG_MASK               0x7FF

/* Extender GPORT subtype shift & mask definitions */
#define _SHR_GPORT_EXTENDER_PORT_SHIFT                  (0)
#define _SHR_GPORT_EXTENDER_PORT_MASK                   (0x7FFFFF)
#define _SHR_EXTENDER_GPORT_TYPE_SHIFT                  (23)
#define _SHR_EXTENDER_GPORT_TYPE_MASK                   (0x7)


#define _SHR_GPORT_TYPE_TRAP                                  ((_SHR_GPORT_TYPE_LOCAL_CPU << 1) | 1)
#define _SHR_GPORT_TYPE_TRAP_SHIFT                      (_SHR_GPORT_TYPE_SHIFT-1)   /* 25 */
#define _SHR_GPORT_TYPE_TRAP_MASK                       (_SHR_GPORT_TYPE_MASK<<1|1)  /* 0x4f*/
#define _SHR_GPORT_TRAP_ID_SHIFT                         0
#define _SHR_GPORT_TRAP_ID_MASK                          0x3ff
#define _SHR_GPORT_TRAP_STRENGTH_SHIFT          	 12
#define _SHR_GPORT_TRAP_STRENGTH_MASK          		 0xf
#define _SHR_GPORT_TRAP_SNOOP_STRENGTH_SHIFT             16
#define _SHR_GPORT_TRAP_SNOOP_STRENGTH_MASK              0xf


#define _SHR_PHY_GPORT_TYPE_SHIFT 20
#define _SHR_PHY_GPORT_TYPE_MASK 0x3F

#define _SHR_PHY_GPORT_TYPE_LANE_PORT 1
#define _SHR_PHY_GPORT_LANE_PORT_PORT_MASK 0x3ff
#define _SHR_PHY_GPORT_LANE_PORT_LANE_MASK 0x3ff
#define _SHR_PHY_GPORT_LANE_PORT_PORT_SHIFT 0
#define _SHR_PHY_GPORT_LANE_PORT_LANE_SHIFT 10

#define _SHR_PHY_GPORT_TYPE_PHYN_PORT 2 
#define _SHR_PHY_GPORT_PHYN_PORT_PHYN_MASK 0x7 
#define _SHR_PHY_GPORT_PHYN_PORT_PHYN_SHIFT 17 
#define _SHR_PHY_GPORT_PHYN_PORT_PORT_MASK 0x3FF 
#define _SHR_PHY_GPORT_PHYN_PORT_PORT_SHIFT 0

#define _SHR_PHY_GPORT_TYPE_PHYN_LANE_PORT 3
#define _SHR_PHY_GPORT_PHYN_LANE_PORT_PHYN_MASK 0x7 
#define _SHR_PHY_GPORT_PHYN_LANE_PORT_PHYN_SHIFT 17
#define _SHR_PHY_GPORT_PHYN_LANE_PORT_LANE_MASK 0xEF
#define _SHR_PHY_GPORT_PHYN_LANE_PORT_LANE_SHIFT 10
#define _SHR_PHY_GPORT_PHYN_LANE_PORT_PORT_MASK 0x3FF
#define _SHR_PHY_GPORT_PHYN_LANE_PORT_PORT_SHIFT 0

#define _SHR_PHY_GPORT_TYPE_PHYN_SYS_SIDE_PORT 4
#define _SHR_PHY_GPORT_PHYN_SYS_SIDE_PORT_PHYN_MASK 0x7 
#define _SHR_PHY_GPORT_PHYN_SYS_SIDE_PORT_PHYN_SHIFT 17 
#define _SHR_PHY_GPORT_PHYN_SYS_SIDE_PORT_PORT_MASK 0x3FF 
#define _SHR_PHY_GPORT_PHYN_SYS_SIDE_PORT_PORT_SHIFT 0

#define _SHR_PHY_GPORT_TYPE_PHYN_SYS_SIDE_LANE_PORT 5
#define _SHR_PHY_GPORT_PHYN_SYS_SIDE_LANE_PORT_PHYN_MASK 0x7 
#define _SHR_PHY_GPORT_PHYN_SYS_SIDE_LANE_PORT_PHYN_SHIFT 17
#define _SHR_PHY_GPORT_PHYN_SYS_SIDE_LANE_PORT_LANE_MASK 0xEF
#define _SHR_PHY_GPORT_PHYN_SYS_SIDE_LANE_PORT_LANE_SHIFT 10
#define _SHR_PHY_GPORT_PHYN_SYS_SIDE_LANE_PORT_PORT_MASK 0x3FF
#define _SHR_PHY_GPORT_PHYN_SYS_SIDE_LANE_PORT_PORT_SHIFT 0

#define _SHR_GPORT_IS_SET(_gport)    \
        (((((_gport) >> _SHR_GPORT_TYPE_SHIFT) & _SHR_GPORT_TYPE_MASK) > 0) && \
         ((((_gport) >> _SHR_GPORT_TYPE_SHIFT) & _SHR_GPORT_TYPE_MASK) <= _SHR_GPORT_TYPE_MAX))

#define _SHR_GPORT_IS_LOCAL(_gport)    \
        (((((_gport) >> _SHR_GPORT_TYPE_SHIFT) & _SHR_GPORT_TYPE_MASK) ==    \
                                                                       _SHR_GPORT_TYPE_LOCAL))

#define _SHR_GPORT_LOCAL_SET(_gport, _port)                                  \
        ((_gport) = (_SHR_GPORT_TYPE_LOCAL     << _SHR_GPORT_TYPE_SHIFT)   | \
         (((_port) & _SHR_GPORT_PORT_MASK)     << _SHR_GPORT_PORT_SHIFT))

#define _SHR_GPORT_LOCAL_GET(_gport)    \
        (((_gport) >> _SHR_GPORT_PORT_SHIFT) & _SHR_GPORT_PORT_MASK)

#define _SHR_GPORT_IS_MODPORT(_gport)    \
        (((_gport) >> _SHR_GPORT_TYPE_SHIFT) == _SHR_GPORT_TYPE_MODPORT)

#define _SHR_GPORT_MODPORT_SET(_gport, _module, _port)                       \
        ((_gport) = (_SHR_GPORT_TYPE_MODPORT   << _SHR_GPORT_TYPE_SHIFT)   | \
         (((_module) & _SHR_GPORT_MODID_MASK)  << _SHR_GPORT_MODID_SHIFT)  | \
         (((_port) & _SHR_GPORT_PORT_MASK)     << _SHR_GPORT_PORT_SHIFT))

#define _SHR_GPORT_MODPORT_MODID_GET(_gport)    \
        (((_gport) >> _SHR_GPORT_MODID_SHIFT) & _SHR_GPORT_MODID_MASK)

#define _SHR_GPORT_MODPORT_PORT_GET(_gport)     \
        (((_gport) >> _SHR_GPORT_PORT_SHIFT) & _SHR_GPORT_PORT_MASK)

#define _SHR_GPORT_IS_TRUNK(_gport)    \
        (((_gport) >> _SHR_GPORT_TYPE_SHIFT) == _SHR_GPORT_TYPE_TRUNK)

#define _SHR_GPORT_TRUNK_SET(_gport, _trunk_id)                              \
        ((_gport) = (_SHR_GPORT_TYPE_TRUNK      << _SHR_GPORT_TYPE_SHIFT)  | \
         (((_trunk_id) & _SHR_GPORT_TRUNK_MASK) << _SHR_GPORT_TRUNK_SHIFT))

#define _SHR_GPORT_TRUNK_GET(_gport)   \
        (((_gport) >> _SHR_GPORT_TRUNK_SHIFT) & _SHR_GPORT_TRUNK_MASK)

#define _SHR_GPORT_IS_MPLS_PORT(_gport)    \
        (((_gport) >> _SHR_GPORT_TYPE_SHIFT) == _SHR_GPORT_TYPE_MPLS_PORT)

#define _SHR_GPORT_MPLS_PORT_ID_SET(_gport, _id)                            \
        ((_gport) = (_SHR_GPORT_TYPE_MPLS_PORT << _SHR_GPORT_TYPE_SHIFT)  | \
         (((_id) & _SHR_GPORT_MPLS_PORT_MASK) << _SHR_GPORT_MPLS_PORT_SHIFT))

#define _SHR_GPORT_MPLS_PORT_ID_GET(_gport)   \
        (((_gport) >> _SHR_GPORT_MPLS_PORT_SHIFT) & _SHR_GPORT_MPLS_PORT_MASK)

#define _SHR_GPORT_IS_SUBPORT_GROUP(_gport)    \
        (((_gport) >> _SHR_GPORT_TYPE_SHIFT) == _SHR_GPORT_TYPE_SUBPORT_GROUP)

#define _SHR_GPORT_SUBPORT_GROUP_SET(_gport, _id)                            \
        ((_gport) = (_SHR_GPORT_TYPE_SUBPORT_GROUP << _SHR_GPORT_TYPE_SHIFT)  | \
         (((_id) & _SHR_GPORT_SUBPORT_GROUP_MASK) << _SHR_GPORT_SUBPORT_GROUP_SHIFT))

#define _SHR_GPORT_SUBPORT_GROUP_GET(_gport)   \
        (((_gport) >> _SHR_GPORT_SUBPORT_GROUP_SHIFT) & _SHR_GPORT_SUBPORT_GROUP_MASK)

#define _SHR_GPORT_IS_SUBPORT_PORT(_gport)    \
        (((_gport) >> _SHR_GPORT_TYPE_SHIFT) == _SHR_GPORT_TYPE_SUBPORT_PORT)

#define _SHR_GPORT_SUBPORT_PORT_SET(_gport, _id)                            \
        ((_gport) = (_SHR_GPORT_TYPE_SUBPORT_PORT << _SHR_GPORT_TYPE_SHIFT)  | \
         (((_id) & _SHR_GPORT_SUBPORT_PORT_MASK) << _SHR_GPORT_SUBPORT_PORT_SHIFT))

#define _SHR_GPORT_SUBPORT_PORT_GET(_gport)   \
        (((_gport) >> _SHR_GPORT_SUBPORT_PORT_SHIFT) & _SHR_GPORT_SUBPORT_PORT_MASK)

#define _SHR_GPORT_IS_SCHEDULER(_gport)    \
        (((_gport) >> _SHR_GPORT_TYPE_SHIFT) == _SHR_GPORT_TYPE_SCHEDULER)

#define _SHR_GPORT_SCHEDULER_SET(_gport, _id)                            \
        ((_gport) = (_SHR_GPORT_TYPE_SCHEDULER << _SHR_GPORT_TYPE_SHIFT)  | \
         (((_id) & _SHR_GPORT_SCHEDULER_MASK) << _SHR_GPORT_SCHEDULER_SHIFT))

#define _SHR_GPORT_SCHEDULER_GET(_gport)   \
        (((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK)

#define _SHR_GPORT_SCHEDULER_NODE_SET(_gport, _level, _node)		\
        ((_gport) = (_SHR_GPORT_TYPE_SCHEDULER << _SHR_GPORT_TYPE_SHIFT)  | \
         (((_level) & _SHR_GPORT_SCHEDULER_LEVEL_MASK) << _SHR_GPORT_SCHEDULER_LEVEL_SHIFT) | \
         (((_node) & _SHR_GPORT_SCHEDULER_NODE_MASK) << _SHR_GPORT_SCHEDULER_NODE_SHIFT))

#define _SHR_GPORT_SCHEDULER_LEVEL_GET(_gport)   \
        (((_gport) >> _SHR_GPORT_SCHEDULER_LEVEL_SHIFT) & _SHR_GPORT_SCHEDULER_LEVEL_MASK)

#define _SHR_GPORT_SCHEDULER_NODE_GET(_gport)   \
        (((_gport) >> _SHR_GPORT_SCHEDULER_NODE_SHIFT) & _SHR_GPORT_SCHEDULER_NODE_MASK)

#define _SHR_GPORT_IS_DEVPORT(_gport)    \
        (((_gport) >> _SHR_GPORT_TYPE_SHIFT) == _SHR_GPORT_TYPE_DEVPORT)

#define _SHR_GPORT_DEVPORT(_device, _port)                       \
        ((_SHR_GPORT_TYPE_DEVPORT   << _SHR_GPORT_TYPE_SHIFT)   | \
         (((_device) & _SHR_GPORT_DEVID_MASK)  << _SHR_GPORT_DEVID_SHIFT)  | \
         (((_port) & _SHR_GPORT_PORT_MASK)     << _SHR_GPORT_PORT_SHIFT))

#define _SHR_GPORT_DEVPORT_SET(_gport, _device, _port)                       \
        ((_gport) = _SHR_GPORT_DEVPORT(_device, _port))

#define _SHR_GPORT_DEVPORT_DEVID_GET(_gport)    \
        (((_gport) >> _SHR_GPORT_DEVID_SHIFT) & _SHR_GPORT_DEVID_MASK)

#define _SHR_GPORT_DEVPORT_PORT_GET(_gport)     \
        (((_gport) >> _SHR_GPORT_PORT_SHIFT) & _SHR_GPORT_PORT_MASK)

#define _SHR_GPORT_BLACK_HOLE    \
        (_SHR_GPORT_TYPE_BLACK_HOLE << _SHR_GPORT_TYPE_SHIFT)

#define _SHR_GPORT_IS_BLACK_HOLE(_gport)  ((_gport) == _SHR_GPORT_BLACK_HOLE)

#define _SHR_GPORT_LOCAL_CPU	    \
        (_SHR_GPORT_TYPE_LOCAL_CPU << _SHR_GPORT_TYPE_SHIFT)

#define _SHR_GPORT_IS_LOCAL_CPU(_gport)   ((_gport) == _SHR_GPORT_LOCAL_CPU)

#define _SHR_GPORT_IS_UCAST_QUEUE_GROUP(_gport)    \
        (((_gport) >> _SHR_GPORT_TYPE_SHIFT) == _SHR_GPORT_TYPE_UCAST_QUEUE_GROUP)

#define _SHR_GPORT_UCAST_QUEUE_GROUP_SET(_gport, _qid)                           \
            _SHR_GPORT_UCAST_QUEUE_GROUP_SYSQID_SET(_gport,                      \
            _SHR_GPORT_UCAST_QUEUE_GROUP_SYSPORTID_MASK, \
             _qid)

#define _SHR_GPORT_UCAST_QUEUE_GROUP_SYSQID_SET(_gport, _sysport_id, _qid)                       \
        ((_gport) = (_SHR_GPORT_TYPE_UCAST_QUEUE_GROUP << _SHR_GPORT_TYPE_SHIFT)   | \
         (((_sysport_id) & _SHR_GPORT_UCAST_QUEUE_GROUP_SYSPORTID_MASK)  << _SHR_GPORT_UCAST_QUEUE_GROUP_SYSPORTID_SHIFT)  | \
         (((_qid) & _SHR_GPORT_UCAST_QUEUE_GROUP_QID_MASK)     << _SHR_GPORT_UCAST_QUEUE_GROUP_QID_SHIFT))

#define _SHR_GPORT_UCAST_QUEUE_GROUP_SYSPORTID_GET(_gport)                       \
        (((_gport) >> _SHR_GPORT_UCAST_QUEUE_GROUP_SYSPORTID_SHIFT) & _SHR_GPORT_UCAST_QUEUE_GROUP_SYSPORTID_MASK)


#define _SHR_GPORT_UCAST_QUEUE_GROUP_QID_GET(_gport)                       \
        (((_gport) >> _SHR_GPORT_UCAST_QUEUE_GROUP_QID_SHIFT) & _SHR_GPORT_UCAST_QUEUE_GROUP_QID_MASK)

#define _SHR_GPORT_UNICAST_QUEUE_GROUP_SET(_gport, _qid)                             \
        ((_gport) = (_SHR_GPORT_TYPE_UCAST_QUEUE_GROUP << _SHR_GPORT_TYPE_SHIFT) |   \
        (((_qid) & _SHR_GPORT_UNICAST_QUEUE_GROUP_QID_MASK) << _SHR_GPORT_UNICAST_QUEUE_GROUP_QID_SHIFT))

#define _SHR_GPORT_UNICAST_QUEUE_GROUP_QID_GET(_gport)                               \
        (((_gport) >> _SHR_GPORT_UNICAST_QUEUE_GROUP_QID_SHIFT) & _SHR_GPORT_UNICAST_QUEUE_GROUP_QID_MASK)

#define _SHR_GPORT_IS_UCAST_SUBSCRIBER_QUEUE_GROUP(_gport)    \
        (((_gport) >> _SHR_GPORT_TYPE_SHIFT) == _SHR_GPORT_TYPE_UCAST_SUBSCRIBER_QUEUE_GROUP)

#define _SHR_GPORT_UCAST_SUBSCRIBER_QUEUE_GROUP_QID_SET(_gport, _qid)          \
        ((_gport) = (_SHR_GPORT_TYPE_UCAST_SUBSCRIBER_QUEUE_GROUP << _SHR_GPORT_TYPE_SHIFT)   | \
        (((_qid) & _SHR_GPORT_UCAST_SUBSCRIBER_QUEUE_GROUP_QID_MASK) << _SHR_GPORT_UCAST_SUBSCRIBER_QUEUE_GROUP_QID_SHIFT))

#define _SHR_GPORT_UCAST_SUBSCRIBER_QUEUE_GROUP_QID_GET(_gport)                       \
        (((_gport) >> _SHR_GPORT_UCAST_SUBSCRIBER_QUEUE_GROUP_QID_SHIFT) & \
                              _SHR_GPORT_UCAST_SUBSCRIBER_QUEUE_GROUP_QID_MASK)

#define _SHR_GPORT_IS_MCAST(_gport)    \
        (((_gport) >> _SHR_GPORT_TYPE_SHIFT) == _SHR_GPORT_TYPE_MCAST)

#define _SHR_GPORT_MCAST_SET(_gport, _mcast_id)                              \
        ((_gport) = (_SHR_GPORT_TYPE_MCAST      << _SHR_GPORT_TYPE_SHIFT)  | \
         (((_mcast_id) & _SHR_GPORT_MCAST_MASK) << _SHR_GPORT_MCAST_SHIFT))

#define _SHR_GPORT_MCAST_GET(_gport)   \
        (((_gport) >> _SHR_GPORT_MCAST_SHIFT) & _SHR_GPORT_MCAST_MASK)

#define _SHR_GPORT_MCAST_DS_ID_SET(_gport, _ds_id) \
        ((_gport) = (_SHR_GPORT_TYPE_MCAST      << _SHR_GPORT_TYPE_SHIFT)  | \
         (((_ds_id) & _SHR_GPORT_MCAST_DS_ID_MASK) << _SHR_GPORT_MCAST_DS_ID_SHIFT))

#define _SHR_GPORT_MCAST_DS_ID_GET(_gport)   \
        (((_gport) >> _SHR_GPORT_MCAST_DS_ID_SHIFT) & _SHR_GPORT_MCAST_DS_ID_MASK)

#define _SHR_GPORT_IS_MCAST_QUEUE_GROUP(_gport) \
        (((_gport) >> _SHR_GPORT_TYPE_SHIFT) == _SHR_GPORT_TYPE_MCAST_QUEUE_GROUP)

#define _SHR_GPORT_MCAST_QUEUE_GROUP_SET(_gport, _qid)            \
        _SHR_GPORT_MCAST_QUEUE_GROUP_SYSQID_SET(_gport,           \
        _SHR_GPORT_MCAST_QUEUE_GROUP_SYSPORTID_MASK,              \
         _qid)

#define _SHR_GPORT_MCAST_QUEUE_GROUP_GET(_gport)   \
        _SHR_GPORT_MCAST_QUEUE_GROUP_QID_GET(_gport) 

#define _SHR_GPORT_MCAST_QUEUE_GROUP_SYSQID_SET(_gport, _sysport_id, _qid)                       \
        ((_gport) = (_SHR_GPORT_TYPE_MCAST_QUEUE_GROUP << _SHR_GPORT_TYPE_SHIFT)   | \
         (((_sysport_id) & _SHR_GPORT_MCAST_QUEUE_GROUP_SYSPORTID_MASK)  << _SHR_GPORT_MCAST_QUEUE_GROUP_SYSPORTID_SHIFT)  | \
         (((_qid) & _SHR_GPORT_MCAST_QUEUE_GROUP_QID_MASK)     << _SHR_GPORT_MCAST_QUEUE_GROUP_QID_SHIFT))

#define _SHR_GPORT_MCAST_QUEUE_GROUP_SYSPORTID_GET(_gport)                       \
        (((_gport) >> _SHR_GPORT_MCAST_QUEUE_GROUP_SYSPORTID_SHIFT) & _SHR_GPORT_MCAST_QUEUE_GROUP_SYSPORTID_MASK)

#define _SHR_GPORT_MCAST_QUEUE_GROUP_QID_GET(_gport)                       \
         (((_gport) >> _SHR_GPORT_MCAST_QUEUE_GROUP_QID_SHIFT) & _SHR_GPORT_MCAST_QUEUE_GROUP_QID_MASK)

#define _SHR_GPORT_IS_MCAST_SUBSCRIBER_QUEUE_GROUP(_gport)    \
        (((_gport) >> _SHR_GPORT_TYPE_SHIFT) == _SHR_GPORT_TYPE_MCAST_SUBSCRIBER_QUEUE_GROUP)

#define _SHR_GPORT_MCAST_SUBSCRIBER_QUEUE_GROUP_QID_SET(_gport, _qid)           	        \
        ((_gport) = (_SHR_GPORT_TYPE_MCAST_SUBSCRIBER_QUEUE_GROUP << _SHR_GPORT_TYPE_SHIFT)   | \
        (((_qid) & _SHR_GPORT_MCAST_SUBSCRIBER_QUEUE_GROUP_QID_MASK) << _SHR_GPORT_MCAST_SUBSCRIBER_QUEUE_GROUP_QID_SHIFT))

#define _SHR_GPORT_MCAST_SUBSCRIBER_QUEUE_GROUP_QID_GET(_gport)                       \
        (((_gport) >> _SHR_GPORT_MCAST_SUBSCRIBER_QUEUE_GROUP_QID_SHIFT) & \
                              _SHR_GPORT_MCAST_SUBSCRIBER_QUEUE_GROUP_QID_MASK)

#define _SHR_GPORT_IS_SPECIAL(_gport)    \
        (((_gport) >> _SHR_GPORT_TYPE_SHIFT) == _SHR_GPORT_TYPE_SPECIAL)

#define _SHR_GPORT_SPECIAL(_value)                              \
        ((_SHR_GPORT_TYPE_SPECIAL << _SHR_GPORT_TYPE_SHIFT)   | \
         (((_value) & _SHR_GPORT_SPECIAL_MASK) << _SHR_GPORT_SPECIAL_SHIFT))

#define _SHR_GPORT_SPECIAL_SET(_gport, _value)  \
        ((_gport) = _SHR_GPORT_SPECIAL(_value))

#define _SHR_GPORT_SPECIAL_GET(_gport)    \
        (((_gport) >> _SHR_GPORT_SPECIAL_SHIFT) & _SHR_GPORT_SPECIAL_MASK)

#define _SHR_GPORT_IS_MIRROR(_gport)    \
        (((_gport) >> _SHR_GPORT_TYPE_SHIFT) == _SHR_GPORT_TYPE_MIRROR)

#define _SHR_GPORT_MIRROR_SET(_gport, _value)                               \
        ((_gport) = (_SHR_GPORT_TYPE_MIRROR << _SHR_GPORT_TYPE_SHIFT)   | \
         (((_value) & _SHR_GPORT_MIRROR_MASK) << _SHR_GPORT_MIRROR_SHIFT))

#define _SHR_GPORT_MIRROR_GET(_gport)    \
        (((_gport) >> _SHR_GPORT_MIRROR_SHIFT) & _SHR_GPORT_MIRROR_MASK)

#define _SHR_GPORT_IS_MIM_PORT(_gport)    \
        (((_gport) >> _SHR_GPORT_TYPE_SHIFT) == _SHR_GPORT_TYPE_MIM_PORT)

#define _SHR_GPORT_MIM_PORT_ID_SET(_gport, _id)                            \
        ((_gport) = (_SHR_GPORT_TYPE_MIM_PORT << _SHR_GPORT_TYPE_SHIFT)  | \
         (((_id) & _SHR_GPORT_MIM_PORT_MASK) << _SHR_GPORT_MIM_PORT_SHIFT))

#define _SHR_GPORT_MIM_PORT_ID_GET(_gport)   \
        (((_gport) >> _SHR_GPORT_MIM_PORT_SHIFT) & _SHR_GPORT_MIM_PORT_MASK)

#define _SHR_GPORT_IS_VLAN_PORT(_gport) \
        (((_gport) >> _SHR_GPORT_TYPE_SHIFT) == _SHR_GPORT_TYPE_VLAN_PORT)

#define _SHR_GPORT_VLAN_PORT_ID_SET(_gport, _id)                            \
        ((_gport) = (_SHR_GPORT_TYPE_VLAN_PORT << _SHR_GPORT_TYPE_SHIFT)  | \
         (((_id) & _SHR_GPORT_VLAN_PORT_MASK) << _SHR_GPORT_VLAN_PORT_SHIFT))

#define _SHR_GPORT_VLAN_PORT_ID_GET(_gport)   \
        (((_gport) >> _SHR_GPORT_VLAN_PORT_SHIFT) & _SHR_GPORT_VLAN_PORT_MASK)

#define _SHR_GPORT_IS_WLAN_PORT(_gport) \
        (((_gport) >> _SHR_GPORT_TYPE_SHIFT) == _SHR_GPORT_TYPE_WLAN_PORT)

#define _SHR_GPORT_WLAN_PORT_ID_SET(_gport, _id)                            \
        ((_gport) = (_SHR_GPORT_TYPE_WLAN_PORT << _SHR_GPORT_TYPE_SHIFT)  | \
         (((_id) & _SHR_GPORT_WLAN_PORT_MASK) << _SHR_GPORT_WLAN_PORT_SHIFT))

#define _SHR_GPORT_WLAN_PORT_ID_GET(_gport)   \
        (((_gport) >> _SHR_GPORT_WLAN_PORT_SHIFT) & _SHR_GPORT_WLAN_PORT_MASK)

#define _SHR_GPORT_IS_TRILL_PORT(_gport) \
        (((_gport) >> _SHR_GPORT_TYPE_SHIFT) == _SHR_GPORT_TYPE_TRILL_PORT)

#define _SHR_GPORT_TRILL_PORT_ID_SET(_gport, _id)                            \
        ((_gport) = (_SHR_GPORT_TYPE_TRILL_PORT << _SHR_GPORT_TYPE_SHIFT)  | \
        (((_id) & _SHR_GPORT_TRILL_PORT_MASK) << _SHR_GPORT_TRILL_PORT_SHIFT))

#define _SHR_GPORT_TRILL_PORT_ID_GET(_gport)   \
        (((_gport) >> _SHR_GPORT_TRILL_PORT_SHIFT) & _SHR_GPORT_TRILL_PORT_MASK)

#define _SHR_GPORT_IS_NIV_PORT(_gport) \
        (((_gport) >> _SHR_GPORT_TYPE_SHIFT) == _SHR_GPORT_TYPE_NIV_PORT)

#define _SHR_GPORT_NIV_PORT_ID_SET(_gport, _id)                            \
        ((_gport) = (_SHR_GPORT_TYPE_NIV_PORT << _SHR_GPORT_TYPE_SHIFT)  | \
        (((_id) & _SHR_GPORT_NIV_PORT_MASK) << _SHR_GPORT_NIV_PORT_SHIFT))

#define _SHR_GPORT_NIV_PORT_ID_GET(_gport)   \
        (((_gport) >> _SHR_GPORT_NIV_PORT_SHIFT) & _SHR_GPORT_NIV_PORT_MASK)

#define _SHR_GPORT_IS_L2GRE_PORT(_gport) \
        (((_gport) >> _SHR_GPORT_TYPE_SHIFT) == _SHR_GPORT_TYPE_L2GRE_PORT)

#define _SHR_GPORT_L2GRE_PORT_ID_SET(_gport, _id)                            \
        ((_gport) = (_SHR_GPORT_TYPE_L2GRE_PORT << _SHR_GPORT_TYPE_SHIFT)  | \
        (((_id) & _SHR_GPORT_L2GRE_PORT_MASK) << _SHR_GPORT_L2GRE_PORT_SHIFT))

#define _SHR_GPORT_L2GRE_PORT_ID_GET(_gport)   \
        (((_gport) >> _SHR_GPORT_L2GRE_PORT_SHIFT) & _SHR_GPORT_L2GRE_PORT_MASK)

#define _SHR_GPORT_IS_VXLAN_PORT(_gport) \
        ((((_gport) >> _SHR_GPORT_TYPE_SHIFT) & _SHR_GPORT_TYPE_MASK) == \
                                                _SHR_GPORT_TYPE_VXLAN_PORT)

#define _SHR_GPORT_VXLAN_PORT_ID_SET(_gport, _id)                            \
        ((_gport) = (_SHR_GPORT_TYPE_VXLAN_PORT << _SHR_GPORT_TYPE_SHIFT)  | \
        (((_id) & _SHR_GPORT_VXLAN_PORT_MASK) << _SHR_GPORT_VXLAN_PORT_SHIFT))

#define _SHR_GPORT_VXLAN_PORT_ID_GET(_gport)   \
        (((_gport) >> _SHR_GPORT_VXLAN_PORT_SHIFT) & _SHR_GPORT_VXLAN_PORT_MASK)


/* Extender GPORT Type & Subtype utility macros */
#define _SHR_GPORT_IS_EXTENDER_PORT(_gport) \
        ((((_gport) >> _SHR_GPORT_TYPE_SHIFT) & _SHR_GPORT_TYPE_MASK) == \
                                                _SHR_GPORT_TYPE_EXTENDER_PORT)
#define _SHR_EXTENDER_GPORT_IS_PORT(_gport)                                                 \
        (_SHR_GPORT_IS_EXTENDER_PORT(_gport) &&                                             \
         ((((_gport) >> _SHR_EXTENDER_GPORT_TYPE_SHIFT) & _SHR_EXTENDER_GPORT_TYPE_MASK) == \
          _SHR_EXTENDER_GPORT_TYPE_PORT))

#define _SHR_EXTENDER_GPORT_IS_FORWARD(_gport)                                              \
        (_SHR_GPORT_IS_EXTENDER_PORT(_gport) &&                                             \
         ((((_gport) >> _SHR_EXTENDER_GPORT_TYPE_SHIFT) & _SHR_EXTENDER_GPORT_TYPE_MASK) == \
          _SHR_EXTENDER_GPORT_TYPE_FORWARD))

#define _SHR_EXTENDER_GPORT_IS_ENCAP(_gport)                                                \
        (_SHR_GPORT_IS_EXTENDER_PORT(_gport) &&                                             \
         ((((_gport) >> _SHR_EXTENDER_GPORT_TYPE_SHIFT) & _SHR_EXTENDER_GPORT_TYPE_MASK) == \
          _SHR_EXTENDER_GPORT_TYPE_ENCAP))

#define _SHR_EXTENDER_GPORT_PORT_SET(_gport, _id)                                           \
        ((_gport) = (_SHR_GPORT_TYPE_EXTENDER_PORT << _SHR_GPORT_TYPE_SHIFT)            |   \
         (_SHR_EXTENDER_GPORT_TYPE_PORT << _SHR_EXTENDER_GPORT_TYPE_SHIFT)              |   \
         (((_id) & _SHR_GPORT_EXTENDER_PORT_MASK) << _SHR_GPORT_EXTENDER_PORT_SHIFT))

#define _SHR_EXTENDER_GPORT_FORWARD_SET(_gport, _id)                                        \
        ((_gport) = (_SHR_GPORT_TYPE_EXTENDER_PORT << _SHR_GPORT_TYPE_SHIFT)            |   \
         (_SHR_EXTENDER_GPORT_TYPE_FORWARD << _SHR_EXTENDER_GPORT_TYPE_SHIFT)           |   \
         (((_id) & _SHR_GPORT_EXTENDER_PORT_MASK) << _SHR_GPORT_EXTENDER_PORT_SHIFT))

#define _SHR_EXTENDER_GPORT_ENCAP_SET(_gport, _id)                                          \
        ((_gport) = (_SHR_GPORT_TYPE_EXTENDER_PORT << _SHR_GPORT_TYPE_SHIFT)            |   \
         (_SHR_EXTENDER_GPORT_TYPE_ENCAP << _SHR_EXTENDER_GPORT_TYPE_SHIFT)             |   \
         (((_id) & _SHR_GPORT_EXTENDER_PORT_MASK) << _SHR_GPORT_EXTENDER_PORT_SHIFT))

    /* Note: Same as _SHR_EXTENDER_GPORT_PORT_SET, exists only for backward competability */
#define _SHR_GPORT_EXTENDER_PORT_ID_SET(_gport, _id)                                        \
        ((_gport) = (_SHR_GPORT_TYPE_EXTENDER_PORT << _SHR_GPORT_TYPE_SHIFT)            |   \
        (((_id) & _SHR_GPORT_EXTENDER_PORT_MASK) << _SHR_GPORT_EXTENDER_PORT_SHIFT))

#define _SHR_GPORT_EXTENDER_PORT_ID_GET(_gport)                                             \
        (((_gport) >> _SHR_GPORT_EXTENDER_PORT_SHIFT) & _SHR_GPORT_EXTENDER_PORT_MASK)


#define _SHR_GPORT_IS_MAC_PORT(_gport) \
        ((((_gport) >> _SHR_GPORT_TYPE_SHIFT) & _SHR_GPORT_TYPE_MASK) == \
                                                _SHR_GPORT_TYPE_MAC_PORT)

#define _SHR_GPORT_MAC_PORT_ID_SET(_gport, _id)                            \
        ((_gport) = (_SHR_GPORT_TYPE_MAC_PORT << _SHR_GPORT_TYPE_SHIFT)  | \
        (((_id) & _SHR_GPORT_MAC_PORT_MASK) << _SHR_GPORT_MAC_PORT_SHIFT))

#define _SHR_GPORT_MAC_PORT_ID_GET(_gport)   \
        (((_gport) >> _SHR_GPORT_MAC_PORT_SHIFT) & _SHR_GPORT_MAC_PORT_MASK)

#define _SHR_GPORT_IS_TUNNEL(_gport)    \
        (((_gport) >> _SHR_GPORT_TYPE_SHIFT) == _SHR_GPORT_TYPE_TUNNEL)

#define _SHR_GPORT_TUNNEL_ID_SET(_gport, _tunnel_id)                       \
        ((_gport) = (_SHR_GPORT_TYPE_TUNNEL   << _SHR_GPORT_TYPE_SHIFT)  | \
         (((_tunnel_id) & _SHR_GPORT_TUNNEL_MASK) << _SHR_GPORT_TUNNEL_SHIFT))

#define _SHR_GPORT_TUNNEL_ID_GET(_gport)   \
        (((_gport) >> _SHR_GPORT_TUNNEL_SHIFT) & _SHR_GPORT_TUNNEL_MASK)

#define _SHR_GPORT_IS_CHILD(_gport)   \
        (((_gport) >> _SHR_GPORT_TYPE_SHIFT) == _SHR_GPORT_TYPE_CHILD)

#define _SHR_GPORT_CHILD_SET(_child_gport,_child_modid, _child_port) \
        ((_child_gport) = (_SHR_GPORT_TYPE_CHILD << _SHR_GPORT_TYPE_SHIFT)  |	\
        (((_child_modid) & _SHR_GPORT_CHILD_MODID_MASK) << _SHR_GPORT_CHILD_MODID_SHIFT) | \
        (((_child_port) & _SHR_GPORT_CHILD_PORT_MASK) << _SHR_GPORT_CHILD_PORT_SHIFT))        

#define _SHR_GPORT_CHILD_MODID_GET(_gport)  \
        (((_gport) >> _SHR_GPORT_CHILD_MODID_SHIFT) & _SHR_GPORT_CHILD_MODID_MASK)

#define _SHR_GPORT_CHILD_PORT_GET(_gport)  \
        (((_gport) >> _SHR_GPORT_CHILD_PORT_SHIFT) & _SHR_GPORT_CHILD_PORT_MASK)

#define _SHR_GPORT_IS_EGRESS_GROUP(_gport)  \
        (((_gport) >> _SHR_GPORT_TYPE_SHIFT) == _SHR_GPORT_TYPE_EGRESS_GROUP)

#define _SHR_GPORT_EGRESS_GROUP_SET(_egress_group_gport, _child_modid, _egress_child) \
        ((_egress_group_gport) = (_SHR_GPORT_TYPE_EGRESS_GROUP << _SHR_GPORT_TYPE_SHIFT)  |	\
        (((_child_modid) & _SHR_GPORT_EGRESS_CHILD_MODID_MASK) << _SHR_GPORT_EGRESS_CHILD_MODID_SHIFT) | \
        (((_egress_child) & _SHR_GPORT_EGRESS_CHILD_PORT_MASK) << (_SHR_GPORT_EGRESS_GROUP_SHIFT)))

#define _SHR_GPORT_EGRESS_GROUP_MODID_GET(_egress_group_gport)  \
        (((_egress_group_gport) >> _SHR_GPORT_EGRESS_GROUP_MODID_SHIFT) & _SHR_GPORT_EGRESS_GROUP_MODID_MASK)

#define _SHR_GPORT_EGRESS_GROUP_GET(_egress_group_gport)  \
        (((_egress_group_gport) >> _SHR_GPORT_EGRESS_GROUP_SHIFT) & _SHR_GPORT_EGRESS_GROUP_MASK)

#define _SHR_GPORT_IS_EGRESS_CHILD(_gport)  \
        (((_gport) >> _SHR_GPORT_TYPE_SHIFT) == _SHR_GPORT_TYPE_EGRESS_CHILD)

#define _SHR_GPORT_EGRESS_CHILD_SET(_egress_child_gport,_child_modid, _child_port) \
        ((_egress_child_gport) = (_SHR_GPORT_TYPE_EGRESS_CHILD << _SHR_GPORT_TYPE_SHIFT)  |	\
        (((_child_modid) & _SHR_GPORT_EGRESS_CHILD_MODID_MASK) << _SHR_GPORT_EGRESS_CHILD_MODID_SHIFT) | \
        (((_child_port) & _SHR_GPORT_EGRESS_CHILD_PORT_MASK) << _SHR_GPORT_EGRESS_CHILD_PORT_SHIFT))        

#define _SHR_GPORT_EGRESS_CHILD_MODID_GET(_egress_child_gport)  \
        (((_egress_child_gport) >> _SHR_GPORT_EGRESS_CHILD_MODID_SHIFT) & _SHR_GPORT_EGRESS_CHILD_MODID_MASK)

#define _SHR_GPORT_EGRESS_CHILD_PORT_GET(_egress_child_gport)  \
        (((_egress_child_gport) >> _SHR_GPORT_EGRESS_CHILD_PORT_SHIFT) & _SHR_GPORT_EGRESS_CHILD_PORT_MASK)

#define _SHR_GPORT_IS_EGRESS_MODPORT(_gport)  \
        (((_gport) >> _SHR_GPORT_TYPE_SHIFT) == _SHR_GPORT_TYPE_EGRESS_MODPORT)

#define _SHR_GPORT_EGRESS_MODPORT_SET(_gport, _module, _port)  \
        ((_gport) = (_SHR_GPORT_TYPE_EGRESS_MODPORT   << _SHR_GPORT_TYPE_SHIFT)   | \
         (((_module) & _SHR_GPORT_EGRESS_MODPORT_MODID_MASK)  << _SHR_GPORT_EGRESS_MODPORT_MODID_SHIFT)  | \
         (((_port) & _SHR_GPORT_EGRESS_MODPORT_PORT_MASK)     << _SHR_GPORT_EGRESS_MODPORT_PORT_SHIFT))

#define _SHR_GPORT_EGRESS_MODPORT_MODID_GET(_gport)  \
        (((_gport) >> _SHR_GPORT_EGRESS_MODPORT_MODID_SHIFT) & _SHR_GPORT_EGRESS_MODPORT_MODID_MASK)

#define _SHR_GPORT_EGRESS_MODPORT_PORT_GET(_gport)  \
        (((_gport) >> _SHR_GPORT_EGRESS_MODPORT_PORT_SHIFT) & _SHR_GPORT_EGRESS_MODPORT_PORT_MASK)

#define _SHR_GPORT_IS_CONGESTION(_gport)    \
        (((_gport) >> _SHR_GPORT_TYPE_SHIFT) == _SHR_GPORT_TYPE_CONGESTION)

#define _SHR_GPORT_CONGESTION_SET(_gport, _port)                                       \
        ((_gport) = (_SHR_GPORT_TYPE_CONGESTION << _SHR_GPORT_TYPE_SHIFT)   |          \
         (((_port) & _SHR_GPORT_CONGESTION_MASK) << _SHR_GPORT_CONGESTION_SHIFT))

#define _SHR_GPORT_CONGESTION_GET(_gport)                                              \
        (((_gport) >> _SHR_GPORT_CONGESTION_SHIFT) & _SHR_GPORT_CONGESTION_MASK)

#define _SHR_GPORT_IS_SYSTEM_PORT(_gport) \
        (((_gport) >> _SHR_GPORT_TYPE_SHIFT) == _SHR_GPORT_TYPE_SYSTEM_PORT)
#define _SHR_GPORT_SYSTEM_PORT_ID_GET(_gport)   \
        (((_gport) >> _SHR_GPORT_SYSTEM_PORT_SHIFT) & _SHR_GPORT_SYSTEM_PORT_MASK)
#define _SHR_GPORT_SYSTEM_PORT_ID_SET(_gport, _id)                            \
        ((_gport) = (_SHR_GPORT_TYPE_SYSTEM_PORT << _SHR_GPORT_TYPE_SHIFT)  | \
         (((_id) & _SHR_GPORT_SYSTEM_PORT_MASK) << _SHR_GPORT_SYSTEM_PORT_SHIFT))

#define _SHR_COSQ_GPORT_ISQ_ROOT_SET(_gport) \
        ((_gport) = (_SHR_GPORT_TYPE_SCHEDULER << _SHR_GPORT_TYPE_SHIFT)  | \
         (((_SHR_GPORT_ISQ_ROOT) & _SHR_GPORT_SCHEDULER_MASK) << _SHR_GPORT_SCHEDULER_SHIFT))

#define _SHR_COSQ_GPORT_IS_ISQ_ROOT(_gport)                                            \
        (_SHR_GPORT_IS_SCHEDULER(_gport) &&                                            \
        ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==     \
                                                _SHR_GPORT_ISQ_ROOT))
#define _SHR_COSQ_GPORT_IS_FMQ_ROOT(_gport)                                            \
        (_SHR_GPORT_IS_SCHEDULER(_gport) &&                                            \
        ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==     \
                                                _SHR_GPORT_FMQ_ROOT))

#define _SHR_COSQ_GPORT_FMQ_ROOT_SET(_gport) \
        ((_gport) = (_SHR_GPORT_TYPE_SCHEDULER << _SHR_GPORT_TYPE_SHIFT)  | \
         (((_SHR_GPORT_FMQ_ROOT) & _SHR_GPORT_SCHEDULER_MASK) << _SHR_GPORT_SCHEDULER_SHIFT))

#define _SHR_COSQ_GPORT_IS_FMQ_GUARANTEED(_gport)                                      \
        (_SHR_GPORT_IS_SCHEDULER(_gport) &&                                            \
        ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==     \
                                                _SHR_GPORT_FMQ_GUARANTEED))

#define _SHR_COSQ_GPORT_FMQ_GUARANTEED_SET(_gport) \
        ((_gport) = (_SHR_GPORT_TYPE_SCHEDULER << _SHR_GPORT_TYPE_SHIFT)  | \
         (((_SHR_GPORT_FMQ_GUARANTEED) & _SHR_GPORT_SCHEDULER_MASK) << _SHR_GPORT_SCHEDULER_SHIFT))

#define _SHR_COSQ_GPORT_IS_FMQ_BESTEFFORT_AGR(_gport)                                  \
        (_SHR_GPORT_IS_SCHEDULER(_gport) &&                                            \
        ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==     \
                                                _SHR_GPORT_FMQ_BESTEFFORT_AGR))

#define _SHR_COSQ_GPORT_FMQ_BESTEFFORT_AGR_SET(_gport) \
        ((_gport) = (_SHR_GPORT_TYPE_SCHEDULER << _SHR_GPORT_TYPE_SHIFT)  | \
         (((_SHR_GPORT_FMQ_BESTEFFORT_AGR) & _SHR_GPORT_SCHEDULER_MASK) << _SHR_GPORT_SCHEDULER_SHIFT))

#define _SHR_COSQ_GPORT_IS_FMQ_BESTEFFORT0(_gport)                                     \
        (_SHR_GPORT_IS_SCHEDULER(_gport) &&                                            \
        ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==     \
                                                _SHR_GPORT_FMQ_BESTEFFORT0))

#define _SHR_COSQ_GPORT_FMQ_BESTEFFORT0_SET(_gport) \
        ((_gport) = (_SHR_GPORT_TYPE_SCHEDULER << _SHR_GPORT_TYPE_SHIFT)  | \
         (((_SHR_GPORT_FMQ_BESTEFFORT0) & _SHR_GPORT_SCHEDULER_MASK) << _SHR_GPORT_SCHEDULER_SHIFT))

#define _SHR_COSQ_GPORT_IS_FMQ_BESTEFFORT1(_gport)                                     \
        (_SHR_GPORT_IS_SCHEDULER(_gport) &&                                            \
        ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==     \
                                                _SHR_GPORT_FMQ_BESTEFFORT1))

#define _SHR_COSQ_GPORT_FMQ_BESTEFFORT1_SET(_gport) \
        ((_gport) = (_SHR_GPORT_TYPE_SCHEDULER << _SHR_GPORT_TYPE_SHIFT)  | \
         (((_SHR_GPORT_FMQ_BESTEFFORT1) & _SHR_GPORT_SCHEDULER_MASK) << _SHR_GPORT_SCHEDULER_SHIFT))

#define _SHR_COSQ_GPORT_IS_FMQ_BESTEFFORT2(_gport)                                     \
        (_SHR_GPORT_IS_SCHEDULER(_gport) &&                                            \
        ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==     \
                                                _SHR_GPORT_FMQ_BESTEFFORT2))

#define _SHR_COSQ_GPORT_FMQ_BESTEFFORT2_SET(_gport) \
        ((_gport) = (_SHR_GPORT_TYPE_SCHEDULER << _SHR_GPORT_TYPE_SHIFT)  | \
         (((_SHR_GPORT_FMQ_BESTEFFORT2) & _SHR_GPORT_SCHEDULER_MASK) << _SHR_GPORT_SCHEDULER_SHIFT))

#define _SHR_GPORT_IS_FABRIC_MESH(_gport)                                                         \
        ( _SHR_GPORT_IS_SCHEDULER(_gport)  &&                                                     \
            ( ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==          \
                                (_SHR_GPORT_FABRIC_MESH_LOCAL & _SHR_GPORT_SCHEDULER_MASK))  ||   \
              ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==          \
                                (_SHR_GPORT_FABRIC_MESH_DEV1 & _SHR_GPORT_SCHEDULER_MASK))   ||   \
              ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==          \
                                (_SHR_GPORT_FABRIC_MESH_DEV2 & _SHR_GPORT_SCHEDULER_MASK))   ||   \
              ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==          \
                                (_SHR_GPORT_FABRIC_MESH_DEV3 & _SHR_GPORT_SCHEDULER_MASK))   ||   \
              ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==          \
                                (_SHR_GPORT_FABRIC_MESH_DEV4 & _SHR_GPORT_SCHEDULER_MASK))   ||   \
              ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==          \
                                (_SHR_GPORT_FABRIC_MESH_DEV5 & _SHR_GPORT_SCHEDULER_MASK))   ||   \
              ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==          \
                                (_SHR_GPORT_FABRIC_MESH_DEV6 & _SHR_GPORT_SCHEDULER_MASK))   ||   \
              ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==          \
                                (_SHR_GPORT_FABRIC_MESH_DEV7 & _SHR_GPORT_SCHEDULER_MASK)) ))  

#define _SHR_GPORT_IS_FABRIC_MESH_LOCAL(_gport)                                                   \
        ( _SHR_GPORT_IS_SCHEDULER(_gport)  &&                                                     \
              ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==          \
                                (_SHR_GPORT_FABRIC_MESH_LOCAL & _SHR_GPORT_SCHEDULER_MASK)))
                                
#define _SHR_GPORT_IS_FABRIC_MESH_DEV1(_gport)                                                    \
        ( _SHR_GPORT_IS_SCHEDULER(_gport)  &&                                                     \
              ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==          \
                                (_SHR_GPORT_FABRIC_MESH_DEV1 & _SHR_GPORT_SCHEDULER_MASK)))

#define _SHR_GPORT_IS_FABRIC_MESH_DEV2(_gport)                                                    \
        ( _SHR_GPORT_IS_SCHEDULER(_gport)  &&                                                     \
              ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==          \
                                (_SHR_GPORT_FABRIC_MESH_DEV2 & _SHR_GPORT_SCHEDULER_MASK)))
                                
#define _SHR_GPORT_IS_FABRIC_MESH_DEV3(_gport)                                                    \
        ( _SHR_GPORT_IS_SCHEDULER(_gport)  &&                                                     \
              ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==          \
                                (_SHR_GPORT_FABRIC_MESH_DEV3 & _SHR_GPORT_SCHEDULER_MASK)))
                                
#define _SHR_GPORT_IS_FABRIC_MESH_DEV4(_gport)                                                    \
        ( _SHR_GPORT_IS_SCHEDULER(_gport)  &&                                                     \
              ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==          \
                                (_SHR_GPORT_FABRIC_MESH_DEV4 & _SHR_GPORT_SCHEDULER_MASK)))
                                
#define _SHR_GPORT_IS_FABRIC_MESH_DEV5(_gport)                                                    \
        ( _SHR_GPORT_IS_SCHEDULER(_gport)  &&                                                     \
              ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==          \
                                (_SHR_GPORT_FABRIC_MESH_DEV5 & _SHR_GPORT_SCHEDULER_MASK)))
                                
#define _SHR_GPORT_IS_FABRIC_MESH_DEV6(_gport)                                                    \
        ( _SHR_GPORT_IS_SCHEDULER(_gport)  &&                                                     \
              ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==          \
                                (_SHR_GPORT_FABRIC_MESH_DEV6 & _SHR_GPORT_SCHEDULER_MASK)))
                                
#define _SHR_GPORT_IS_FABRIC_MESH_DEV7(_gport)                                                    \
        ( _SHR_GPORT_IS_SCHEDULER(_gport)  &&                                                     \
              ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==          \
                                (_SHR_GPORT_FABRIC_MESH_DEV7 & _SHR_GPORT_SCHEDULER_MASK)))


#define _SHR_COSQ_GPORT_FABRIC_MESH_LOCAL_SET(_gport)                             \
        ((_gport) = (_SHR_GPORT_TYPE_SCHEDULER << _SHR_GPORT_TYPE_SHIFT)  |       \
         (((_SHR_GPORT_FABRIC_MESH_LOCAL) & _SHR_GPORT_SCHEDULER_MASK) << _SHR_GPORT_SCHEDULER_SHIFT))

#define _SHR_COSQ_GPORT_FABRIC_MESH_DEV_SET(_gport, _dev_id)                      \
    switch (_dev_id) {                                                            \
        case 1:                                                                   \
            ((_gport) = (_SHR_GPORT_TYPE_SCHEDULER << _SHR_GPORT_TYPE_SHIFT)  |   \
             (((_SHR_GPORT_FABRIC_MESH_DEV1) & _SHR_GPORT_SCHEDULER_MASK) << _SHR_GPORT_SCHEDULER_SHIFT));                                                                            \
            break;                                                                \
        case 2:                                                                   \
            ((_gport) = (_SHR_GPORT_TYPE_SCHEDULER << _SHR_GPORT_TYPE_SHIFT)  | \
             (((_SHR_GPORT_FABRIC_MESH_DEV2) & _SHR_GPORT_SCHEDULER_MASK) << _SHR_GPORT_SCHEDULER_SHIFT));                                                                            \
            break;                                                                \
        case 3:                                                                   \
            ((_gport) = (_SHR_GPORT_TYPE_SCHEDULER << _SHR_GPORT_TYPE_SHIFT)  | \
             (((_SHR_GPORT_FABRIC_MESH_DEV3) & _SHR_GPORT_SCHEDULER_MASK) << _SHR_GPORT_SCHEDULER_SHIFT));                                                                            \
            break;                                                                \
        case 4:                                                                   \
            ((_gport) = (_SHR_GPORT_TYPE_SCHEDULER << _SHR_GPORT_TYPE_SHIFT)  |   \
             (((_SHR_GPORT_FABRIC_MESH_DEV4) & _SHR_GPORT_SCHEDULER_MASK) << _SHR_GPORT_SCHEDULER_SHIFT));                                                                            \
            break;                                                                \
        case 5:                                                                   \
            ((_gport) = (_SHR_GPORT_TYPE_SCHEDULER << _SHR_GPORT_TYPE_SHIFT)  |   \
             (((_SHR_GPORT_FABRIC_MESH_DEV5) & _SHR_GPORT_SCHEDULER_MASK) << _SHR_GPORT_SCHEDULER_SHIFT));                                                                            \
            break;                                                                \
        case 6:                                                                   \
            ((_gport) = (_SHR_GPORT_TYPE_SCHEDULER << _SHR_GPORT_TYPE_SHIFT)  |   \
             (((_SHR_GPORT_FABRIC_MESH_DEV6) & _SHR_GPORT_SCHEDULER_MASK) << _SHR_GPORT_SCHEDULER_SHIFT));                                                                            \
            break;                                                                \
        case 7:                                                                   \
            ((_gport) = (_SHR_GPORT_TYPE_SCHEDULER << _SHR_GPORT_TYPE_SHIFT)  |   \
             (((_SHR_GPORT_FABRIC_MESH_DEV7) & _SHR_GPORT_SCHEDULER_MASK) << _SHR_GPORT_SCHEDULER_SHIFT));                                                                            \
            break;                                                                \
        default:                                                                  \
            ((_gport) = _SHR_GPORT_INVALID);                                      \
            break;                                                                \
    }

#define _SHR_COSQ_GPORT_FMQ_CLASS_SET(_gport, _class)                             \
    switch (_class) {                                                             \
        case 1:                                                                   \
            ((_gport) = (_SHR_GPORT_TYPE_SCHEDULER << _SHR_GPORT_TYPE_SHIFT)  |   \
             (((_SHR_GPORT_FMQ_CLASS1) & _SHR_GPORT_SCHEDULER_MASK) << _SHR_GPORT_SCHEDULER_SHIFT));                                                                                  \
            break;                                                                \
        case 2:                                                                   \
            ((_gport) = (_SHR_GPORT_TYPE_SCHEDULER << _SHR_GPORT_TYPE_SHIFT)  | \
             (((_SHR_GPORT_FMQ_CLASS2) & _SHR_GPORT_SCHEDULER_MASK) << _SHR_GPORT_SCHEDULER_SHIFT));                                                                                  \
            break;                                                                \
        case 3:                                                                   \
            ((_gport) = (_SHR_GPORT_TYPE_SCHEDULER << _SHR_GPORT_TYPE_SHIFT)  | \
             (((_SHR_GPORT_FMQ_CLASS3) & _SHR_GPORT_SCHEDULER_MASK) << _SHR_GPORT_SCHEDULER_SHIFT));                                                                                  \
            break;                                                                \
        case 4:                                                                   \
            ((_gport) = (_SHR_GPORT_TYPE_SCHEDULER << _SHR_GPORT_TYPE_SHIFT)  |   \
             (((_SHR_GPORT_FMQ_CLASS4) & _SHR_GPORT_SCHEDULER_MASK) << _SHR_GPORT_SCHEDULER_SHIFT));                                                                                  \
            break;                                                                \
        default:                                                                  \
            ((_gport) = _SHR_GPORT_INVALID);                                      \
            break;                                                                \
    }

#define _SHR_COSQ_GPORT_IS_FMQ_CLASS(_gport)                                              \
        ( _SHR_GPORT_IS_SCHEDULER(_gport) &&                                              \
          ( ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==    \
                                                _SHR_GPORT_FMQ_CLASS1) ||                 \
            ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==    \
                                                _SHR_GPORT_FMQ_CLASS2) ||                 \
            ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==    \
                                                _SHR_GPORT_FMQ_CLASS3) ||                 \
            ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==    \
                                                _SHR_GPORT_FMQ_CLASS4) ) )

             #define _SHR_GPORT_IS_FABRIC_CLOS(_gport)                                                                        \
        ( _SHR_GPORT_IS_SCHEDULER(_gport)  &&                                                                    \
            ( ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==                         \
                                (_SHR_GPORT_FABRIC_CLOS_UNICAST_LOCAL & _SHR_GPORT_SCHEDULER_MASK))        ||    \
              ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==                         \
                                (_SHR_GPORT_FABRIC_CLOS_UNICAST_LOCAL_LOW & _SHR_GPORT_SCHEDULER_MASK))    ||    \
              ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==                         \
                                (_SHR_GPORT_FABRIC_CLOS_UNICAST_LOCAL_HIGH & _SHR_GPORT_SCHEDULER_MASK))   ||    \
              ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==                         \
                                (_SHR_GPORT_FABRIC_CLOS_FABRIC & _SHR_GPORT_SCHEDULER_MASK))               ||    \
              ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==                         \
                                (_SHR_GPORT_FABRIC_CLOS_FABRIC_HIGH & _SHR_GPORT_SCHEDULER_MASK))          ||    \
              ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==                         \
                                (_SHR_GPORT_FABRIC_CLOS_FABRIC_LOW & _SHR_GPORT_SCHEDULER_MASK))           ||    \
              ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==                         \
                                (_SHR_GPORT_FABRIC_CLOS_UNICAST_FABRIC_HIGH & _SHR_GPORT_SCHEDULER_MASK))  ||    \
              ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==                         \
                                (_SHR_GPORT_FABRIC_CLOS_UNICAST_FABRIC_LOW & _SHR_GPORT_SCHEDULER_MASK))   ||    \
              ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==                         \
                                (_SHR_GPORT_FABRIC_CLOS_FMQ_GUARANTEED & _SHR_GPORT_SCHEDULER_MASK))       ||    \
              ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==                                                       \
                                (_SHR_GPORT_FABRIC_CLOS_FMQ_BESTEFFORT & _SHR_GPORT_SCHEDULER_MASK))))                
                                                                
#define _SHR_GPORT_IS_FABRIC_CLOS_UNICAST_LOCAL(_gport)                                                          \
        ( _SHR_GPORT_IS_SCHEDULER(_gport)  &&                                                                    \
            ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==                           \
                                (_SHR_GPORT_FABRIC_CLOS_UNICAST_LOCAL & _SHR_GPORT_SCHEDULER_MASK)))
                                
#define _SHR_GPORT_IS_FABRIC_CLOS_UNICAST_LOCAL_LOW(_gport)                                                      \
        ( _SHR_GPORT_IS_SCHEDULER(_gport)  &&                                                                    \
            ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==                           \
                                (_SHR_GPORT_FABRIC_CLOS_UNICAST_LOCAL_LOW & _SHR_GPORT_SCHEDULER_MASK)))
                                
#define _SHR_GPORT_IS_FABRIC_CLOS_UNICAST_LOCAL_HIGH(_gport)                                                     \
        ( _SHR_GPORT_IS_SCHEDULER(_gport)  &&                                                                    \
            ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==                           \
                                (_SHR_GPORT_FABRIC_CLOS_UNICAST_LOCAL_HIGH & _SHR_GPORT_SCHEDULER_MASK)))                          

#define _SHR_GPORT_IS_FABRIC_CLOS_FABRIC(_gport)                                                                 \
        ( _SHR_GPORT_IS_SCHEDULER(_gport)  &&                                                                    \
            ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==                           \
                                (_SHR_GPORT_FABRIC_CLOS_FABRIC & _SHR_GPORT_SCHEDULER_MASK)))
                       
#define _SHR_GPORT_IS_FABRIC_CLOS_FABRIC_HIGH(_gport)                                                            \
        ( _SHR_GPORT_IS_SCHEDULER(_gport)  &&                                                                    \
              ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==                         \
                                (_SHR_GPORT_FABRIC_CLOS_FABRIC_HIGH & _SHR_GPORT_SCHEDULER_MASK)))
                                
#define _SHR_GPORT_IS_FABRIC_CLOS_FABRIC_LOW(_gport)                                                             \
        ( _SHR_GPORT_IS_SCHEDULER(_gport)  &&                                                                    \
              ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==                         \
                                (_SHR_GPORT_FABRIC_CLOS_FABRIC_LOW & _SHR_GPORT_SCHEDULER_MASK)))
                                
#define _SHR_GPORT_IS_FABRIC_CLOS_UNICAST_FABRIC_HIGH(_gport)                                                    \
        ( _SHR_GPORT_IS_SCHEDULER(_gport)  &&                                                                    \
              ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==                         \
                                (_SHR_GPORT_FABRIC_CLOS_UNICAST_FABRIC_HIGH & _SHR_GPORT_SCHEDULER_MASK)))       
                                
#define _SHR_GPORT_IS_FABRIC_CLOS_UNICAST_FABRIC_LOW(_gport)                                                     \
        ( _SHR_GPORT_IS_SCHEDULER(_gport)  &&                                                                    \
              ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==                         \
                                (_SHR_GPORT_FABRIC_CLOS_UNICAST_FABRIC_LOW & _SHR_GPORT_SCHEDULER_MASK)))                   
                        
#define _SHR_GPORT_IS_FABRIC_CLOS_FMQ_GUARANTEED(_gport)                                                         \
        ( _SHR_GPORT_IS_SCHEDULER(_gport)  &&                                                                    \
              ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==                         \
                                (_SHR_GPORT_FABRIC_CLOS_FMQ_GUARANTEED & _SHR_GPORT_SCHEDULER_MASK)))             
                                
#define _SHR_GPORT_IS_FABRIC_CLOS_FMQ_BESTEFFORT(_gport)                                                         \
        ( _SHR_GPORT_IS_SCHEDULER(_gport)  &&                                                                    \
              ((((_gport) >> _SHR_GPORT_SCHEDULER_SHIFT) & _SHR_GPORT_SCHEDULER_MASK) ==                         \
                                (_SHR_GPORT_FABRIC_CLOS_FMQ_BESTEFFORT & _SHR_GPORT_SCHEDULER_MASK)))       

#define _SHR_COSQ_GPORT_FABRIC_CLOS_UNICAST_LOCAL_SET(_gport)                     \
        ((_gport) = (_SHR_GPORT_TYPE_SCHEDULER << _SHR_GPORT_TYPE_SHIFT)  |       \
         (((_SHR_GPORT_FABRIC_CLOS_UNICAST_LOCAL) & _SHR_GPORT_SCHEDULER_MASK) << _SHR_GPORT_SCHEDULER_SHIFT))

#define _SHR_COSQ_GPORT_FABRIC_CLOS_UNICAST_LOCAL_LOW_SET(_gport)                 \
        ((_gport) = (_SHR_GPORT_TYPE_SCHEDULER << _SHR_GPORT_TYPE_SHIFT)  |       \
         (((_SHR_GPORT_FABRIC_CLOS_UNICAST_LOCAL_LOW) & _SHR_GPORT_SCHEDULER_MASK) << _SHR_GPORT_SCHEDULER_SHIFT))

#define _SHR_COSQ_GPORT_FABRIC_CLOS_UNICAST_LOCAL_HIGH_SET(_gport)                \
        ((_gport) = (_SHR_GPORT_TYPE_SCHEDULER << _SHR_GPORT_TYPE_SHIFT)  |       \
         (((_SHR_GPORT_FABRIC_CLOS_UNICAST_LOCAL_HIGH) & _SHR_GPORT_SCHEDULER_MASK) << _SHR_GPORT_SCHEDULER_SHIFT))

#define _SHR_COSQ_GPORT_FABRIC_CLOS_FABRIC_SET(_gport)                            \
        ((_gport) = (_SHR_GPORT_TYPE_SCHEDULER << _SHR_GPORT_TYPE_SHIFT)  |       \
         (((_SHR_GPORT_FABRIC_CLOS_FABRIC) & _SHR_GPORT_SCHEDULER_MASK) << _SHR_GPORT_SCHEDULER_SHIFT))

#define _SHR_COSQ_GPORT_FABRIC_CLOS_FABRIC_HIGH_SET(_gport)                       \
        ((_gport) = (_SHR_GPORT_TYPE_SCHEDULER << _SHR_GPORT_TYPE_SHIFT)  |       \
         (((_SHR_GPORT_FABRIC_CLOS_FABRIC_HIGH) & _SHR_GPORT_SCHEDULER_MASK) << _SHR_GPORT_SCHEDULER_SHIFT))

#define _SHR_COSQ_GPORT_FABRIC_CLOS_FABRIC_LOW_SET(_gport)                        \
        ((_gport) = (_SHR_GPORT_TYPE_SCHEDULER << _SHR_GPORT_TYPE_SHIFT)  |       \
         (((_SHR_GPORT_FABRIC_CLOS_FABRIC_LOW) & _SHR_GPORT_SCHEDULER_MASK) << _SHR_GPORT_SCHEDULER_SHIFT))

#define _SHR_COSQ_GPORT_FABRIC_CLOS_UNICAST_FABRIC_HIGH_SET(_gport)               \
        ((_gport) = (_SHR_GPORT_TYPE_SCHEDULER << _SHR_GPORT_TYPE_SHIFT)  |       \
         (((_SHR_GPORT_FABRIC_CLOS_UNICAST_FABRIC_HIGH) & _SHR_GPORT_SCHEDULER_MASK) << _SHR_GPORT_SCHEDULER_SHIFT))

#define _SHR_COSQ_GPORT_FABRIC_CLOS_UNICAST_FABRIC_LOW_SET(_gport)                \
        ((_gport) = (_SHR_GPORT_TYPE_SCHEDULER << _SHR_GPORT_TYPE_SHIFT)  |       \
         (((_SHR_GPORT_FABRIC_CLOS_UNICAST_FABRIC_LOW) & _SHR_GPORT_SCHEDULER_MASK) << _SHR_GPORT_SCHEDULER_SHIFT))

#define _SHR_COSQ_GPORT_FABRIC_CLOS_FMQ_GUARANTEED_SET(_gport)                    \
        ((_gport) = (_SHR_GPORT_TYPE_SCHEDULER << _SHR_GPORT_TYPE_SHIFT)  |       \
         (((_SHR_GPORT_FABRIC_CLOS_FMQ_GUARANTEED) & _SHR_GPORT_SCHEDULER_MASK) << _SHR_GPORT_SCHEDULER_SHIFT))

#define _SHR_COSQ_GPORT_FABRIC_CLOS_FMQ_BESTEFFORT_SET(_gport)                    \
        ((_gport) = (_SHR_GPORT_TYPE_SCHEDULER << _SHR_GPORT_TYPE_SHIFT)  |       \
         (((_SHR_GPORT_FABRIC_CLOS_FMQ_BESTEFFORT) & _SHR_GPORT_SCHEDULER_MASK) << _SHR_GPORT_SCHEDULER_SHIFT))

#define _SHR_GPORT_IS_COSQ(_gport) \
        (((_gport) >> _SHR_GPORT_TYPE_SHIFT) == _SHR_GPORT_TYPE_COSQ)

#define _SHR_COSQ_GPORT_IS_VOQ_CONNECTOR(_gport)                                       \
        (_SHR_GPORT_IS_COSQ(_gport) &&                                                 \
        ((((_gport) >> _SHR_COSQ_GPORT_TYPE_SHIFT) & _SHR_COSQ_GPORT_TYPE_MASK) ==     \
                                                _SHR_COSQ_GPORT_TYPE_VOQ_CONNECTOR))

#define _SHR_COSQ_GPORT_COMPOSITE_SF2_GET(_gport)  \
        (((_gport) >> _SHR_COSQ_GPORT_COMPOSITE_SF2_FLOW_SHIFT) & _SHR_COSQ_GPORT_COMPOSITE_SF2_FLOW_MASK)

#define _SHR_COSQ_GPORT_IS_UCAST_EGRESS_QUEUE(_gport)                                  \
        (_SHR_GPORT_IS_COSQ(_gport) &&                                                 \
        ((((_gport) >> _SHR_COSQ_GPORT_TYPE_SHIFT) & _SHR_COSQ_GPORT_TYPE_MASK) ==     \
                                                _SHR_COSQ_GPORT_TYPE_UCAST_EGRESS_QUEUE))
#define _SHR_COSQ_GPORT_UCAST_EGRESS_QUEUE_SET(_gport, _port)                          \
        ((_gport) = (_SHR_GPORT_TYPE_COSQ << _SHR_GPORT_TYPE_SHIFT)   |                \
         (_SHR_COSQ_GPORT_TYPE_UCAST_EGRESS_QUEUE << _SHR_COSQ_GPORT_TYPE_SHIFT)   |   \
         (((_port) & _SHR_COSQ_GPORT_UCAST_EGRESS_QUEUE_PORT_MASK)  << _SHR_COSQ_GPORT_UCAST_EGRESS_QUEUE_PORT_SHIFT))

#define _SHR_COSQ_GPORT_UCAST_EGRESS_QUEUE_GET(_gport)  \
        (((_gport) >> _SHR_COSQ_GPORT_UCAST_EGRESS_QUEUE_PORT_SHIFT) & _SHR_COSQ_GPORT_UCAST_EGRESS_QUEUE_PORT_MASK)

#define _SHR_COSQ_GPORT_IS_MCAST_EGRESS_QUEUE(_gport)                                  \
        (_SHR_GPORT_IS_COSQ(_gport) &&                                                 \
        ((((_gport) >> _SHR_COSQ_GPORT_TYPE_SHIFT) & _SHR_COSQ_GPORT_TYPE_MASK) ==     \
                                                _SHR_COSQ_GPORT_TYPE_MCAST_EGRESS_QUEUE))
#define _SHR_COSQ_GPORT_MCAST_EGRESS_QUEUE_SET(_gport, _port)                          \
        ((_gport) = (_SHR_GPORT_TYPE_COSQ << _SHR_GPORT_TYPE_SHIFT)   |                \
         (_SHR_COSQ_GPORT_TYPE_MCAST_EGRESS_QUEUE << _SHR_COSQ_GPORT_TYPE_SHIFT)   |   \
         (((_port) & _SHR_COSQ_GPORT_MCAST_EGRESS_QUEUE_PORT_MASK)  << _SHR_COSQ_GPORT_MCAST_EGRESS_QUEUE_PORT_SHIFT))

#define _SHR_COSQ_GPORT_MCAST_EGRESS_QUEUE_GET(_gport)  \
        (((_gport) >> _SHR_COSQ_GPORT_MCAST_EGRESS_QUEUE_PORT_SHIFT) & _SHR_COSQ_GPORT_MCAST_EGRESS_QUEUE_PORT_MASK)

#define _SHR_COSQ_GPORT_IS_E2E_PORT(_gport)                                            \
        (_SHR_GPORT_IS_COSQ(_gport) &&                                                 \
        ((((_gport) >> _SHR_COSQ_GPORT_TYPE_SHIFT) & _SHR_COSQ_GPORT_TYPE_MASK) ==     \
                                                _SHR_COSQ_GPORT_TYPE_E2E_PORT))
#define _SHR_COSQ_GPORT_E2E_PORT_SET(_gport, _port)                                       \
        ((_gport) = (_SHR_GPORT_TYPE_COSQ << _SHR_GPORT_TYPE_SHIFT)   |                   \
         (_SHR_COSQ_GPORT_TYPE_E2E_PORT << _SHR_COSQ_GPORT_TYPE_SHIFT)   |                \
         (((_port) & _SHR_COSQ_GPORT_E2E_PORT_MASK)  << _SHR_COSQ_GPORT_E2E_PORT_SHIFT))

#define _SHR_COSQ_GPORT_E2E_PORT_GET(_gport)  \
        (((_gport) >> _SHR_COSQ_GPORT_E2E_PORT_SHIFT) & _SHR_COSQ_GPORT_E2E_PORT_MASK)


#define _SHR_COSQ_GPORT_IS_ISQ(_gport)                                                 \
        (_SHR_GPORT_IS_COSQ(_gport) &&                                                 \
        ((((_gport) >> _SHR_COSQ_GPORT_TYPE_SHIFT) & _SHR_COSQ_GPORT_TYPE_MASK) ==     \
                                                _SHR_COSQ_GPORT_TYPE_ISQ))
#define _SHR_COSQ_GPORT_ISQ_SET(_gport, _qid)                                          \
        ((_gport) = (_SHR_GPORT_TYPE_COSQ << _SHR_GPORT_TYPE_SHIFT)   |                \
         (_SHR_COSQ_GPORT_TYPE_ISQ << _SHR_COSQ_GPORT_TYPE_SHIFT)   |                  \
         (((_qid) & _SHR_COSQ_GPORT_ISQ_QID_MASK)  << _SHR_COSQ_GPORT_ISQ_QID_SHIFT))
#define _SHR_COSQ_GPORT_ISQ_QID_GET(_gport)                                            \
        (((_gport) >> _SHR_COSQ_GPORT_ISQ_QID_SHIFT) & _SHR_COSQ_GPORT_ISQ_QID_MASK)

#define _SHR_COSQ_GPORT_IS_MULTIPATH(_gport)  \
        (_SHR_GPORT_IS_COSQ(_gport) &&    \
        ((((_gport) >> _SHR_COSQ_GPORT_TYPE_SHIFT) & _SHR_COSQ_GPORT_TYPE_MASK) ==     \
                                                _SHR_COSQ_GPORT_TYPE_MULTIPATH))

#define _SHR_COSQ_GPORT_MULTIPATH_GET(_gport)  \
        (((_gport) >> _SHR_COSQ_GPORT_MULTIPATH_SHIFT) & _SHR_COSQ_GPORT_MULTIPATH_MASK)

#define _SHR_COSQ_GPORT_MULTIPATH_SET(_gport, _id)            \
        ((_gport) = (_SHR_GPORT_TYPE_COSQ << _SHR_GPORT_TYPE_SHIFT)   |                \
         (_SHR_COSQ_GPORT_TYPE_MULTIPATH << _SHR_COSQ_GPORT_TYPE_SHIFT)   |            \
         (((_id) & _SHR_COSQ_GPORT_MULTIPATH_MASK) << _SHR_COSQ_GPORT_MULTIPATH_SHIFT))

#define _SHR_COSQ_GPORT_ATTACH_ID_SET(_attach_id,_fcd,_sysport)		               \
  ((_attach_id) =                                                                      \
   ((((_fcd) & _SHR_COSQ_GPORT_ATTACH_ID_FCD_MASK) << _SHR_COSQ_GPORT_ATTACH_ID_FCD_SHIFT) | \
    (((_sysport) & _SHR_COSQ_GPORT_ATTACH_ID_SYSPORT_MASK) << _SHR_COSQ_GPORT_ATTACH_ID_SYSPORT_SHIFT)))

#define _SHR_COSQ_GPORT_ATTACH_ID_SYSPORT_SET(_attach_id,_sysport)                      \
        _SHR_COSQ_GPORT_ATTACH_ID_SET(_attach_id,_SHR_COSQ_GPORT_ATTACH_ID_FCD_MASK,_sysport)
     
#define _SHR_COSQ_GPORT_ATTACH_ID_SYSPORT_GET(_attach_id)                               \
        (((_attach_id) >> _SHR_COSQ_GPORT_ATTACH_ID_SYSPORT_SHIFT) &                    \
        _SHR_COSQ_GPORT_ATTACH_ID_SYSPORT_MASK) 

#define _SHR_COSQ_GPORT_ATTACH_ID_FCD_GET(_attach_id)                                   \
        (((_attach_id) >> _SHR_COSQ_GPORT_ATTACH_ID_FCD_SHIFT)     &                    \
        _SHR_COSQ_GPORT_ATTACH_ID_FCD_MASK)

#define _SHR_GPORT_IS_COSQ(_gport) \
        (((_gport) >> _SHR_GPORT_TYPE_SHIFT) == _SHR_GPORT_TYPE_COSQ)

#define _SHR_COSQ_GPORT_IS_VOQ_CONNECTOR(_gport)                                       \
        (_SHR_GPORT_IS_COSQ(_gport) &&                                                 \
        ((((_gport) >> _SHR_COSQ_GPORT_TYPE_SHIFT) & _SHR_COSQ_GPORT_TYPE_MASK) ==     \
                                                _SHR_COSQ_GPORT_TYPE_VOQ_CONNECTOR))


#define _SHR_COSQ_GPORT_IS_COMPOSITE_SF2(_gport) \
        (_SHR_GPORT_IS_COSQ(_gport) &&                                                 \
        ((((_gport) >> _SHR_COSQ_GPORT_TYPE_SHIFT) & _SHR_COSQ_GPORT_TYPE_MASK) ==     \
                                                _SHR_COSQ_GPORT_TYPE_COMPOSITE_SF2))

#define _SHR_COSQ_GPORT_IS_UCAST_EGRESS_QUEUE(_gport)                                  \
        (_SHR_GPORT_IS_COSQ(_gport) &&                                                 \
        ((((_gport) >> _SHR_COSQ_GPORT_TYPE_SHIFT) & _SHR_COSQ_GPORT_TYPE_MASK) ==     \
                                                _SHR_COSQ_GPORT_TYPE_UCAST_EGRESS_QUEUE))
#define _SHR_COSQ_GPORT_UCAST_EGRESS_QUEUE_SET(_gport, _port)                          \
        ((_gport) = (_SHR_GPORT_TYPE_COSQ << _SHR_GPORT_TYPE_SHIFT)   |                \
         (_SHR_COSQ_GPORT_TYPE_UCAST_EGRESS_QUEUE << _SHR_COSQ_GPORT_TYPE_SHIFT)   |   \
         (((_port) & _SHR_COSQ_GPORT_UCAST_EGRESS_QUEUE_PORT_MASK)  << _SHR_COSQ_GPORT_UCAST_EGRESS_QUEUE_PORT_SHIFT))

#define _SHR_COSQ_GPORT_IS_MCAST_EGRESS_QUEUE(_gport)                                  \
        (_SHR_GPORT_IS_COSQ(_gport) &&                                                 \
        ((((_gport) >> _SHR_COSQ_GPORT_TYPE_SHIFT) & _SHR_COSQ_GPORT_TYPE_MASK) ==     \
                                                _SHR_COSQ_GPORT_TYPE_MCAST_EGRESS_QUEUE))
#define _SHR_COSQ_GPORT_MCAST_EGRESS_QUEUE_SET(_gport, _port)                          \
        ((_gport) = (_SHR_GPORT_TYPE_COSQ << _SHR_GPORT_TYPE_SHIFT)   |                \
         (_SHR_COSQ_GPORT_TYPE_MCAST_EGRESS_QUEUE << _SHR_COSQ_GPORT_TYPE_SHIFT)   |   \
         (((_port) & _SHR_COSQ_GPORT_MCAST_EGRESS_QUEUE_PORT_MASK)  << _SHR_COSQ_GPORT_MCAST_EGRESS_QUEUE_PORT_SHIFT))

#define _SHR_COSQ_GPORT_IS_VSQ(_gport)                                                 \
        (_SHR_GPORT_IS_COSQ(_gport) &&                                                 \
        ((((_gport) >> _SHR_COSQ_GPORT_TYPE_SHIFT) & _SHR_COSQ_GPORT_TYPE_MASK) ==     \
                                                _SHR_COSQ_GPORT_TYPE_VSQ))
#define _SHR_COSQ_GPORT_VSQ_SET(_gport, _type, _type_info)                             \
        ((_gport) = (_SHR_GPORT_TYPE_COSQ << _SHR_GPORT_TYPE_SHIFT)   |                \
         (_SHR_COSQ_GPORT_TYPE_VSQ << _SHR_COSQ_GPORT_TYPE_SHIFT)   |                  \
         (((_type) & _SHR_COSQ_GPORT_VSQ_TYPE_MASK)  << _SHR_COSQ_GPORT_VSQ_TYPE_SHIFT) |  \
         (((_type_info) & _SHR_COSQ_GPORT_VSQ_TYPE_INFO_MASK)  << _SHR_COSQ_GPORT_VSQ_TYPE_INFO_SHIFT))
#define _SHR_COSQ_GPORT_VSQ_TYPE_GET(_gport)                                           \
        (((_gport) >> _SHR_COSQ_GPORT_VSQ_TYPE_SHIFT) & _SHR_COSQ_GPORT_VSQ_TYPE_MASK)
#define _SHR_COSQ_GPORT_VSQ_TYPE_INFO_GET(_gport)                                      \
        (((_gport) >> _SHR_COSQ_GPORT_VSQ_TYPE_INFO_SHIFT) & _SHR_COSQ_GPORT_VSQ_TYPE_INFO_MASK)
#define _SHR_COSQ_VSQ_TYPE_INFO_GROUPA_SET(_type_info, _category)                      \
        ((_type_info) = _category)
#define _SHR_COSQ_VSQ_TYPE_INFO_GROUPA_CATEGORY_GET(_type_info)                        \
        ((_type_info))
#define _SHR_COSQ_VSQ_TYPE_INFO_GROUPB_SET(_type_info, _category, _tc)                 \
        ((_type_info) = (((_category & _SHR_VSQ_TYPE_INFO_CATEGORY_MASK) << _SHR_VSQ_TYPE_INFO_CATEGORY_SHIFT) | ((_tc & _SHR_VSQ_TYPE_INFO_TC_MASK) << _SHR_VSQ_TYPE_INFO_TC_SHIFT)))
#define _SHR_COSQ_VSQ_TYPE_INFO_GROUPB_CATEGORY_GET(_type_info)                        \
        (((_type_info) >> _SHR_VSQ_TYPE_INFO_CATEGORY_SHIFT) & _SHR_VSQ_TYPE_INFO_CATEGORY_MASK)
#define _SHR_COSQ_VSQ_TYPE_INFO_GROUPB_TC_GET(_type_info)                              \
        (((_type_info) >> _SHR_VSQ_TYPE_INFO_TC_SHIFT) & _SHR_VSQ_TYPE_INFO_TC_MASK)
#define _SHR_COSQ_VSQ_TYPE_INFO_GROUPC_SET(_type_info, _category, _connection)         \
        ((_type_info) = (((_category & _SHR_VSQ_TYPE_INFO_CATEGORY_MASK) << _SHR_VSQ_TYPE_INFO_CATEGORY_SHIFT) | ((_connection & _SHR_VSQ_TYPE_INFO_CONNECTION_MASK) << _SHR_VSQ_TYPE_INFO_CONNECTION_SHIFT)))
#define _SHR_COSQ_VSQ_TYPE_INFO_GROUPC_CATEGORY_GET(_type_info)                        \
        (((_type_info) >> _SHR_VSQ_TYPE_INFO_CATEGORY_SHIFT) & _SHR_VSQ_TYPE_INFO_CATEGORY_MASK)
#define _SHR_COSQ_VSQ_TYPE_INFO_GROUPC_CONNECTION_GET(_type_info)                      \
        (((_type_info) >> _SHR_VSQ_TYPE_INFO_CONNECTION_SHIFT) & _SHR_VSQ_TYPE_INFO_CONNECTION_MASK)
#define _SHR_COSQ_VSQ_TYPE_INFO_GROUPD_SET(_type_info, _statistics_tag)                \
        ((_type_info) = _statistics_tag)
#define _SHR_COSQ_VSQ_TYPE_INFO_GROUPD_STATISTICS_TAG_GET(_type_info)                  \
        ((_type_info))
#define _SHR_COSQ_VSQ_TYPE_INFO_GROUPE_SET(_type_info, _src_port, _tc)         \
        ((_type_info) = (((_src_port & _SHR_VSQ_TYPE_INFO_SRC_PORT_MASK) << _SHR_VSQ_TYPE_INFO_SRC_PORT_SHIFT) | ((_tc & _SHR_VSQ_TYPE_INFO_TC_MASK) << _SHR_VSQ_TYPE_INFO_TC_SHIFT)))
#define _SHR_COSQ_VSQ_TYPE_INFO_GROUPE_SRC_PORT_GET(_type_info)                        \
        (((_type_info) >> _SHR_VSQ_TYPE_INFO_SRC_PORT_SHIFT) & _SHR_VSQ_TYPE_INFO_SRC_PORT_MASK)
#define _SHR_COSQ_VSQ_TYPE_INFO_GROUPE_TC_GET(_type_info)                      \
        (((_type_info) >> _SHR_VSQ_TYPE_INFO_TC_SHIFT) & _SHR_VSQ_TYPE_INFO_TC_MASK)
#define _SHR_COSQ_VSQ_TYPE_INFO_GROUPF_SET(_type_info, _src_port)         \
        ((_type_info) = (((_src_port & _SHR_VSQ_TYPE_INFO_SRC_PORT_MASK) << _SHR_VSQ_TYPE_INFO_SRC_PORT_SHIFT)))
#define _SHR_COSQ_VSQ_TYPE_INFO_GROUPF_SRC_PORT_GET(_type_info)                        \
        (((_type_info) >> _SHR_VSQ_TYPE_INFO_SRC_PORT_SHIFT) & _SHR_VSQ_TYPE_INFO_SRC_PORT_MASK)
#define _SHR_GPORT_IS_EPON_LINK(_gport)    \
        ((((_gport) >> _SHR_GPORT_TYPE_SHIFT) & _SHR_GPORT_TYPE_MASK) == \
                                                _SHR_GPORT_TYPE_EPON_LINK)
#define _SHR_GPORT_EPON_LINK_SET(_gport, _num)                                      \
        ((_gport) = (_SHR_GPORT_TYPE_EPON_LINK << _SHR_GPORT_TYPE_SHIFT)  |         \
         (((_num) & _SHR_GPORT_EPON_LINK_NUM_MASK) << _SHR_GPORT_EPON_LINK_NUM_SHIFT))
#define _SHR_GPORT_EPON_LINK_GET(_gport)   \
        (((_gport) >> _SHR_GPORT_EPON_LINK_SHIFT) & _SHR_GPORT_EPON_LINK_MASK)

#define _SHR_COSQ_GPORT_IS_E2E_PORT(_gport)                                            \
        (_SHR_GPORT_IS_COSQ(_gport) &&                                                 \
        ((((_gport) >> _SHR_COSQ_GPORT_TYPE_SHIFT) & _SHR_COSQ_GPORT_TYPE_MASK) ==     \
                                                _SHR_COSQ_GPORT_TYPE_E2E_PORT))
#define _SHR_COSQ_GPORT_E2E_PORT_SET(_gport, _port)                                       \
        ((_gport) = (_SHR_GPORT_TYPE_COSQ << _SHR_GPORT_TYPE_SHIFT)   |                   \
         (_SHR_COSQ_GPORT_TYPE_E2E_PORT << _SHR_COSQ_GPORT_TYPE_SHIFT)   |                \
         (((_port) & _SHR_COSQ_GPORT_E2E_PORT_MASK)  << _SHR_COSQ_GPORT_E2E_PORT_SHIFT))

#define _SHR_COSQ_GPORT_IS_ISQ(_gport)                                                 \
        (_SHR_GPORT_IS_COSQ(_gport) &&                                                 \
        ((((_gport) >> _SHR_COSQ_GPORT_TYPE_SHIFT) & _SHR_COSQ_GPORT_TYPE_MASK) ==     \
                                                _SHR_COSQ_GPORT_TYPE_ISQ))
#define _SHR_COSQ_GPORT_ISQ_SET(_gport, _qid)                                          \
        ((_gport) = (_SHR_GPORT_TYPE_COSQ << _SHR_GPORT_TYPE_SHIFT)   |                \
         (_SHR_COSQ_GPORT_TYPE_ISQ << _SHR_COSQ_GPORT_TYPE_SHIFT)   |                  \
         (((_qid) & _SHR_COSQ_GPORT_ISQ_QID_MASK)  << _SHR_COSQ_GPORT_ISQ_QID_SHIFT))
#define _SHR_COSQ_GPORT_ISQ_QID_GET(_gport)                                            \
        (((_gport) >> _SHR_COSQ_GPORT_ISQ_QID_SHIFT) & _SHR_COSQ_GPORT_ISQ_QID_MASK)

#define _SHR_COSQ_GPORT_IS_MULTIPATH(_gport)  \
        (_SHR_GPORT_IS_COSQ(_gport) &&    \
        ((((_gport) >> _SHR_COSQ_GPORT_TYPE_SHIFT) & _SHR_COSQ_GPORT_TYPE_MASK) ==     \
                                                _SHR_COSQ_GPORT_TYPE_MULTIPATH))

#define _SHR_COSQ_GPORT_MULTIPATH_GET(_gport)  \
        (((_gport) >> _SHR_COSQ_GPORT_MULTIPATH_SHIFT) & _SHR_COSQ_GPORT_MULTIPATH_MASK)

#define _SHR_COSQ_GPORT_MULTIPATH_SET(_gport, _id)            \
        ((_gport) = (_SHR_GPORT_TYPE_COSQ << _SHR_GPORT_TYPE_SHIFT)   |                \
         (_SHR_COSQ_GPORT_TYPE_MULTIPATH << _SHR_COSQ_GPORT_TYPE_SHIFT)   |            \
         (((_id) & _SHR_COSQ_GPORT_MULTIPATH_MASK) << _SHR_COSQ_GPORT_MULTIPATH_SHIFT))

#define _SHR_COSQ_GPORT_ATTACH_ID_SET(_attach_id,_fcd,_sysport)               \
  ((_attach_id) =                                                                      \
   ((((_fcd) & _SHR_COSQ_GPORT_ATTACH_ID_FCD_MASK) << _SHR_COSQ_GPORT_ATTACH_ID_FCD_SHIFT) | \
    (((_sysport) & _SHR_COSQ_GPORT_ATTACH_ID_SYSPORT_MASK) << _SHR_COSQ_GPORT_ATTACH_ID_SYSPORT_SHIFT)))

#define _SHR_COSQ_GPORT_ATTACH_ID_SYSPORT_SET(_attach_id,_sysport)                      \
        _SHR_COSQ_GPORT_ATTACH_ID_SET(_attach_id,_SHR_COSQ_GPORT_ATTACH_ID_FCD_MASK,_sysport)
     
#define _SHR_COSQ_GPORT_ATTACH_ID_SYSPORT_GET(_attach_id)                               \
        (((_attach_id) >> _SHR_COSQ_GPORT_ATTACH_ID_SYSPORT_SHIFT) &                    \
        _SHR_COSQ_GPORT_ATTACH_ID_SYSPORT_MASK) 

#define _SHR_COSQ_GPORT_ATTACH_ID_FCD_GET(_attach_id)                                   \
        (((_attach_id) >> _SHR_COSQ_GPORT_ATTACH_ID_FCD_SHIFT)     &                    \
        _SHR_COSQ_GPORT_ATTACH_ID_FCD_MASK)

#define _SHR_COSQ_GPORT_VOQ_CONNECTOR_SET(_gport, _cid)                                \
        ((_gport) = (_SHR_GPORT_TYPE_COSQ << _SHR_GPORT_TYPE_SHIFT)   |                \
         (_SHR_COSQ_GPORT_TYPE_VOQ_CONNECTOR << _SHR_COSQ_GPORT_TYPE_SHIFT)   |        \
         (((_cid) & _SHR_COSQ_GPORT_VOQ_CONNECTOR_ID_MASK)  << _SHR_COSQ_GPORT_VOQ_CONNECTOR_ID_SHIFT))

#define _SHR_COSQ_GPORT_VOQ_CONNECTOR_ID_GET(_gport)                                   \
        (((_gport) >> _SHR_COSQ_GPORT_VOQ_CONNECTOR_ID_SHIFT) & _SHR_COSQ_GPORT_VOQ_CONNECTOR_ID_MASK)

#define _SHR_COSQ_GPORT_RESERVED_TYPE_GET(_gport) \
  (((_gport) >> _SHR_COSQ_GPORT_COMPOSITE_SF2_RESERVED_SHIFT) & _SHR_COSQ_GPORT_COMPOSITE_SF2_RESERVED_MASK)

#define _SHR_COSQ_GPORT_COMPOSITE_SF2_SET(_gport_sf2, gport) \
    if (_SHR_GPORT_IS_SCHEDULER(gport))  { \
        (_gport_sf2) = ((_SHR_GPORT_TYPE_COSQ << _SHR_GPORT_TYPE_SHIFT) |                                       \
                        (_SHR_COSQ_GPORT_TYPE_COMPOSITE_SF2 << _SHR_COSQ_GPORT_TYPE_SHIFT) |                    \
                        ( _SHR_COSQ_GPORT_RESERVED_SCHEDULER << _SHR_COSQ_GPORT_COMPOSITE_SF2_RESERVED_SHIFT) | \
                        ((_SHR_GPORT_SCHEDULER_GET(gport)) & _SHR_COSQ_GPORT_COMPOSITE_SF2_FLOW_MASK));         \
    } else if (_SHR_COSQ_GPORT_IS_VOQ_CONNECTOR(gport)) {      \
             (_gport_sf2) = ((_SHR_GPORT_TYPE_COSQ << _SHR_GPORT_TYPE_SHIFT) |                                          \
                          (_SHR_COSQ_GPORT_TYPE_COMPOSITE_SF2 << _SHR_COSQ_GPORT_TYPE_SHIFT) |                       \
                          (_SHR_COSQ_GPORT_RESERVED_VOQ_CONNECTOR << _SHR_COSQ_GPORT_COMPOSITE_SF2_RESERVED_SHIFT) | \
                          (_SHR_COSQ_GPORT_VOQ_CONNECTOR_ID_GET(gport) & _SHR_COSQ_GPORT_COMPOSITE_SF2_FLOW_MASK));  \
    }

#define _SHR_COSQ_GPORT_IS_SCHED_CIR(_gport)                                           \
        (_SHR_GPORT_IS_COSQ(_gport) &&                                                 \
        ((((_gport) >> _SHR_COSQ_GPORT_TYPE_SHIFT) & _SHR_COSQ_GPORT_TYPE_MASK) ==     \
                                                _SHR_COSQ_GPORT_TYPE_SCHED_CIR))

#define _SHR_COSQ_GPORT_IS_SCHED_PIR(_gport)                                           \
        (_SHR_GPORT_IS_COSQ(_gport) &&                                                 \
        ((((_gport) >> _SHR_COSQ_GPORT_TYPE_SHIFT) & _SHR_COSQ_GPORT_TYPE_MASK) ==     \
                                                _SHR_COSQ_GPORT_TYPE_SCHED_PIR))


#define _SHR_COSQ_GPORT_SCHED_CIR_SET(_gport_cir, _gport)                             \
        ((_gport_cir) = (_SHR_GPORT_TYPE_COSQ << _SHR_GPORT_TYPE_SHIFT)   |           \
         (_SHR_COSQ_GPORT_TYPE_SCHED_CIR << _SHR_COSQ_GPORT_TYPE_SHIFT)   |           \
         (((_SHR_GPORT_SCHEDULER_GET(_gport)) & _SHR_COSQ_GPORT_SCHED_CIR_MASK)  << _SHR_COSQ_GPORT_SCHED_CIR_SHIFT))

#define _SHR_COSQ_GPORT_SCHED_CIR_GET(_gport)                                         \
        (((_gport) >> _SHR_COSQ_GPORT_SCHED_CIR_SHIFT) & _SHR_COSQ_GPORT_SCHED_CIR_MASK)


#define _SHR_COSQ_GPORT_SCHED_PIR_SET(_gport_pir, _gport)                             \
        ((_gport_pir) = (_SHR_GPORT_TYPE_COSQ << _SHR_GPORT_TYPE_SHIFT)   |           \
         (_SHR_COSQ_GPORT_TYPE_SCHED_PIR << _SHR_COSQ_GPORT_TYPE_SHIFT)   |           \
         (((_SHR_GPORT_SCHEDULER_GET(_gport)) & _SHR_COSQ_GPORT_SCHED_PIR_MASK)  << _SHR_COSQ_GPORT_SCHED_PIR_SHIFT))

#define _SHR_COSQ_GPORT_SCHED_PIR_GET(_gport)                                         \
        (((_gport) >> _SHR_COSQ_GPORT_SCHED_PIR_SHIFT) & _SHR_COSQ_GPORT_SCHED_PIR_MASK)


#define _SHR_COSQ_GPORT_IS_PORT_TC(_gport)                                             \
        (_SHR_GPORT_IS_COSQ(_gport) &&                                                 \
        ((((_gport) >> _SHR_COSQ_GPORT_TYPE_SHIFT) & _SHR_COSQ_GPORT_TYPE_MASK) ==     \
                                                _SHR_COSQ_GPORT_TYPE_PORT_TC))

#define _SHR_COSQ_GPORT_PORT_TC_SET(_gport, _port)                                     \
        ((_gport) = (_SHR_GPORT_TYPE_COSQ << _SHR_GPORT_TYPE_SHIFT)   |                \
         (_SHR_COSQ_GPORT_TYPE_PORT_TC << _SHR_COSQ_GPORT_TYPE_SHIFT)   |              \
         (((_port) & _SHR_COSQ_GPORT_PORT_TC_MASK)  << _SHR_COSQ_GPORT_PORT_TC_SHIFT))

#define _SHR_COSQ_GPORT_PORT_TC_GET(_gport)                                            \
        (((_gport) >> _SHR_COSQ_GPORT_PORT_TC_SHIFT) & _SHR_COSQ_GPORT_PORT_TC_MASK)

#define _SHR_COSQ_GPORT_IS_PORT_TCG(_gport)                                            \
        (_SHR_GPORT_IS_COSQ(_gport) &&                                                 \
        ((((_gport) >> _SHR_COSQ_GPORT_TYPE_SHIFT) & _SHR_COSQ_GPORT_TYPE_MASK) ==     \
                                                _SHR_COSQ_GPORT_TYPE_PORT_TCG))

#define _SHR_COSQ_GPORT_PORT_TCG_SET(_gport, _port)                                    \
        ((_gport) = (_SHR_GPORT_TYPE_COSQ << _SHR_GPORT_TYPE_SHIFT)   |                \
         (_SHR_COSQ_GPORT_TYPE_PORT_TCG << _SHR_COSQ_GPORT_TYPE_SHIFT)   |             \
         (((_port) & _SHR_COSQ_GPORT_PORT_TCG_MASK)  << _SHR_COSQ_GPORT_PORT_TCG_SHIFT))

#define _SHR_COSQ_GPORT_PORT_TCG_GET(_gport)                                           \
        (((_gport) >> _SHR_COSQ_GPORT_PORT_TCG_SHIFT) & _SHR_COSQ_GPORT_PORT_TCG_MASK)

#define _SHR_COSQ_GPORT_IS_E2E_PORT_TC(_gport)                                         \
        (_SHR_GPORT_IS_COSQ(_gport) &&                                                 \
        ((((_gport) >> _SHR_COSQ_GPORT_TYPE_SHIFT) & _SHR_COSQ_GPORT_TYPE_MASK) ==     \
                                                _SHR_COSQ_GPORT_TYPE_E2E_PORT_TC))

#define _SHR_COSQ_GPORT_E2E_PORT_TC_SET(_gport, _port)                                 \
        ((_gport) = (_SHR_GPORT_TYPE_COSQ << _SHR_GPORT_TYPE_SHIFT)   |                \
         (_SHR_COSQ_GPORT_TYPE_E2E_PORT_TC << _SHR_COSQ_GPORT_TYPE_SHIFT)   |          \
         (((_port) & _SHR_COSQ_GPORT_E2E_PORT_TC_MASK)  << _SHR_COSQ_GPORT_E2E_PORT_TC_SHIFT))

#define _SHR_COSQ_GPORT_E2E_PORT_TC_GET(_gport)                                        \
        (((_gport) >> _SHR_COSQ_GPORT_E2E_PORT_TC_SHIFT) & _SHR_COSQ_GPORT_E2E_PORT_TC_MASK)

#define _SHR_COSQ_GPORT_IS_E2E_PORT_TCG(_gport)                                        \
        (_SHR_GPORT_IS_COSQ(_gport) &&                                                 \
        ((((_gport) >> _SHR_COSQ_GPORT_TYPE_SHIFT) & _SHR_COSQ_GPORT_TYPE_MASK) ==     \
                                                _SHR_COSQ_GPORT_TYPE_E2E_PORT_TCG))

#define _SHR_COSQ_GPORT_E2E_PORT_TCG_SET(_gport, _port)                                \
        ((_gport) = (_SHR_GPORT_TYPE_COSQ << _SHR_GPORT_TYPE_SHIFT)   |                \
         (_SHR_COSQ_GPORT_TYPE_E2E_PORT_TCG << _SHR_COSQ_GPORT_TYPE_SHIFT)   |         \
         (((_port) & _SHR_COSQ_GPORT_E2E_PORT_TCG_MASK)  << _SHR_COSQ_GPORT_E2E_PORT_TCG_SHIFT))

#define _SHR_COSQ_GPORT_E2E_PORT_TCG_GET(_gport)                                       \
        (((_gport) >> _SHR_COSQ_GPORT_E2E_PORT_TCG_SHIFT) & _SHR_COSQ_GPORT_E2E_PORT_TCG_MASK)

#define _SHR_GPORT_TRAP_SET(_gport, _trap_id, _trap_strength, _snoop_strength)     \
        ((_gport) = (_SHR_GPORT_TYPE_TRAP << _SHR_GPORT_TYPE_TRAP_SHIFT)   |      \
         (((_trap_id) & _SHR_GPORT_TRAP_ID_MASK) << _SHR_GPORT_TRAP_ID_SHIFT)  |    \
         (((_trap_strength) & _SHR_GPORT_TRAP_STRENGTH_MASK) << _SHR_GPORT_TRAP_STRENGTH_SHIFT)  |  \
         (((_snoop_strength) & _SHR_GPORT_TRAP_SNOOP_STRENGTH_MASK) << _SHR_GPORT_TRAP_SNOOP_STRENGTH_SHIFT) )

#define _SHR_GPORT_TRAP_GET_ID(_gport)    \
        (((_gport) >> _SHR_GPORT_TRAP_ID_SHIFT) & _SHR_GPORT_TRAP_ID_MASK)

#define _SHR_GPORT_TRAP_GET_STRENGTH(_gport)    \
        (((_gport) >> _SHR_GPORT_TRAP_STRENGTH_SHIFT & _SHR_GPORT_TRAP_STRENGTH_MASK))

#define _SHR_GPORT_TRAP_GET_SNOOP_STRENGTH(_gport)    \
        (((_gport) >> _SHR_GPORT_TRAP_SNOOP_STRENGTH_SHIFT & _SHR_GPORT_TRAP_SNOOP_STRENGTH_MASK))

#define _SHR_GPORT_IS_TRAP(_gport)    \
        (((_gport) >> _SHR_GPORT_TYPE_TRAP_SHIFT) == _SHR_GPORT_TYPE_TRAP)

#define _SHR_COSQ_GPORT_IS_SYSTEM_RED(_gport)                                        \
        (_SHR_GPORT_IS_COSQ(_gport) &&                                                 \
        ((((_gport) >> _SHR_COSQ_GPORT_TYPE_SHIFT) & _SHR_COSQ_GPORT_TYPE_MASK) ==     \
                                                _SHR_COSQ_GPORT_TYPE_SYSTEM_RED))

#define _SHR_COSQ_GPORT_SYSTEM_RED_SET(_gport)                                \
        ((_gport) = (_SHR_GPORT_TYPE_COSQ << _SHR_GPORT_TYPE_SHIFT)   |                \
         (_SHR_COSQ_GPORT_TYPE_SYSTEM_RED << _SHR_COSQ_GPORT_TYPE_SHIFT))

#define _SHR_GPORT_IS_PHY(_gport) \
        ((((_gport) >> _SHR_GPORT_TYPE_SHIFT) & _SHR_GPORT_TYPE_MASK) == \
                                                        _SHR_GPORT_TYPE_PHY)

/* _SHR_PHY_GPORT_TYPE_LANE_PORT */
#define _SHR_PHY_GPORT_IS_LANE(_gport) \
        (_SHR_GPORT_IS_PHY(_gport) && \
        ((((_gport) >> _SHR_PHY_GPORT_TYPE_SHIFT) & \
                        _SHR_PHY_GPORT_TYPE_MASK) == \
                                        _SHR_PHY_GPORT_TYPE_LANE_PORT))

#define _SHR_PHY_GPORT_LANE_PORT_SET(_gport, _phy_lane, _port) \
        ((_gport) = (_SHR_GPORT_TYPE_PHY << _SHR_GPORT_TYPE_SHIFT) | \
         (_SHR_PHY_GPORT_TYPE_LANE_PORT << _SHR_PHY_GPORT_TYPE_SHIFT) | \
         (((_phy_lane) & _SHR_PHY_GPORT_LANE_PORT_LANE_MASK) << \
                                _SHR_PHY_GPORT_LANE_PORT_LANE_SHIFT) | \
         (((_port) & _SHR_PHY_GPORT_LANE_PORT_PORT_MASK) << \
                                _SHR_PHY_GPORT_LANE_PORT_PORT_SHIFT))

#define _SHR_PHY_GPORT_LANE_PORT_LANE_GET(_gport) \
        (((_gport) >> _SHR_PHY_GPORT_LANE_PORT_LANE_SHIFT) & \
                        _SHR_PHY_GPORT_LANE_PORT_LANE_MASK)
#define _SHR_PHY_GPORT_LANE_PORT_PORT_GET(_gport) \
        (((_gport) >> _SHR_PHY_GPORT_LANE_PORT_PORT_SHIFT) & \
                        _SHR_PHY_GPORT_LANE_PORT_PORT_MASK)

/* _SHR_PHY_GPORT_TYPE_PHYN_PORT */
#define _SHR_PHY_GPORT_IS_PHYN(_gport) \
        (_SHR_GPORT_IS_PHY(_gport) && \
        ((((_gport) >> _SHR_PHY_GPORT_TYPE_SHIFT) & \
                        _SHR_PHY_GPORT_TYPE_MASK) == \
                                        _SHR_PHY_GPORT_TYPE_PHYN_PORT))
#define _SHR_PHY_GPORT_PHYN_PORT_SET(_gport, _phyn, _port) \
        ((_gport) = (_SHR_GPORT_TYPE_PHY << _SHR_GPORT_TYPE_SHIFT) | \
         (_SHR_PHY_GPORT_TYPE_PHYN_PORT << _SHR_PHY_GPORT_TYPE_SHIFT) | \
         (((_phyn) & _SHR_PHY_GPORT_PHYN_PORT_PHYN_MASK) << \
                                _SHR_PHY_GPORT_PHYN_PORT_PHYN_SHIFT) | \
         (((_port) & _SHR_PHY_GPORT_PHYN_PORT_PORT_MASK) << \
                                _SHR_PHY_GPORT_PHYN_PORT_PORT_SHIFT))

#define _SHR_PHY_GPORT_PHYN_PORT_PHYN_GET(_gport) \
        (((_gport) >> _SHR_PHY_GPORT_PHYN_PORT_PHYN_SHIFT) & \
                        _SHR_PHY_GPORT_PHYN_PORT_PHYN_MASK)
#define _SHR_PHY_GPORT_PHYN_PORT_PORT_GET(_gport) \
        (((_gport) >> _SHR_PHY_GPORT_PHYN_PORT_PORT_SHIFT) & \
                        _SHR_PHY_GPORT_PHYN_PORT_PORT_MASK)



/* _SHR_PHY_GPORT_TYPE_PHYN_LANE_PORT */
#define _SHR_PHY_GPORT_IS_PHYN_LANE(_gport) \
        (_SHR_GPORT_IS_PHY(_gport) && \
        ((((_gport) >> _SHR_PHY_GPORT_TYPE_SHIFT) & \
                        _SHR_PHY_GPORT_TYPE_MASK) == \
                                        _SHR_PHY_GPORT_TYPE_PHYN_LANE_PORT))
#define _SHR_PHY_GPORT_PHYN_LANE_PORT_SET(_gport, _phyn, _phy_lane, _port) \
        ((_gport) = (_SHR_GPORT_TYPE_PHY << _SHR_GPORT_TYPE_SHIFT) | \
         (_SHR_PHY_GPORT_TYPE_PHYN_LANE_PORT << _SHR_PHY_GPORT_TYPE_SHIFT) | \
         (((_phyn) & _SHR_PHY_GPORT_PHYN_LANE_PORT_PHYN_MASK) << \
                                _SHR_PHY_GPORT_PHYN_LANE_PORT_PHYN_SHIFT) | \
         (((_phy_lane) & _SHR_PHY_GPORT_PHYN_LANE_PORT_LANE_MASK) << \
                                _SHR_PHY_GPORT_PHYN_LANE_PORT_LANE_SHIFT) | \
         (((_port) & _SHR_PHY_GPORT_PHYN_LANE_PORT_PORT_MASK) << \
                                _SHR_PHY_GPORT_PHYN_LANE_PORT_PORT_SHIFT))

#define _SHR_PHY_GPORT_PHYN_LANE_PORT_PHYN_GET(_gport) \
        (((_gport) >> _SHR_PHY_GPORT_PHYN_LANE_PORT_PHYN_SHIFT) & \
                        _SHR_PHY_GPORT_PHYN_LANE_PORT_PHYN_MASK)
#define _SHR_PHY_GPORT_PHYN_LANE_PORT_LANE_GET(_gport) \
        (((_gport) >> _SHR_PHY_GPORT_PHYN_LANE_PORT_LANE_SHIFT) & \
                        _SHR_PHY_GPORT_PHYN_LANE_PORT_LANE_MASK)
#define _SHR_PHY_GPORT_PHYN_LANE_PORT_PORT_GET(_gport) \
        (((_gport) >> _SHR_PHY_GPORT_PHYN_LANE_PORT_PORT_SHIFT) & \
                        _SHR_PHY_GPORT_PHYN_LANE_PORT_PORT_MASK)


/* _SHR_PHY_GPORT_TYPE_PHYN_SYS_SIDE_PORT */
#define _SHR_PHY_GPORT_IS_PHYN_SYS_SIDE(_gport) \
        (_SHR_GPORT_IS_PHY(_gport) && \
        ((((_gport) >> _SHR_PHY_GPORT_TYPE_SHIFT) & \
                        _SHR_PHY_GPORT_TYPE_MASK) == \
                                        _SHR_PHY_GPORT_TYPE_PHYN_SYS_SIDE_PORT))
#define _SHR_PHY_GPORT_PHYN_SYS_SIDE_PORT_SET(_gport, _phyn, _port) \
        ((_gport) = (_SHR_GPORT_TYPE_PHY << _SHR_GPORT_TYPE_SHIFT) | \
         (_SHR_PHY_GPORT_TYPE_PHYN_SYS_SIDE_PORT << _SHR_PHY_GPORT_TYPE_SHIFT) | \
         (((_phyn) & _SHR_PHY_GPORT_PHYN_SYS_SIDE_PORT_PHYN_MASK) << \
                                _SHR_PHY_GPORT_PHYN_SYS_SIDE_PORT_PHYN_SHIFT) | \
         (((_port) & _SHR_PHY_GPORT_PHYN_SYS_SIDE_PORT_PORT_MASK) << \
                                _SHR_PHY_GPORT_PHYN_SYS_SIDE_PORT_PORT_SHIFT))

#define _SHR_PHY_GPORT_PHYN_SYS_SIDE_PORT_PHYN_GET(_gport) \
        (((_gport) >> _SHR_PHY_GPORT_PHYN_SYS_SIDE_PORT_PHYN_SHIFT) & \
                        _SHR_PHY_GPORT_PHYN_SYS_SIDE_PORT_PHYN_MASK)
#define _SHR_PHY_GPORT_PHYN_SYS_SIDE_PORT_PORT_GET(_gport) \
        (((_gport) >> _SHR_PHY_GPORT_PHYN_SYS_SIDE_PORT_PORT_SHIFT) & \
                        _SHR_PHY_GPORT_PHYN_SYS_SIDE_PORT_PORT_MASK)


/* _SHR_PHY_GPORT_TYPE_PHYN_SYS_SIDE_LANE_PORT */
#define _SHR_PHY_GPORT_IS_PHYN_SYS_SIDE_LANE(_gport) \
        (_SHR_GPORT_IS_PHY(_gport) && \
        ((((_gport) >> _SHR_PHY_GPORT_TYPE_SHIFT) & \
                        _SHR_PHY_GPORT_TYPE_MASK) == \
                                        _SHR_PHY_GPORT_TYPE_PHYN_SYS_SIDE_LANE_PORT))
#define _SHR_PHY_GPORT_PHYN_SYS_SIDE_LANE_PORT_SET(_gport, _phyn, _phy_lane, _port) \
        ((_gport) = (_SHR_GPORT_TYPE_PHY << _SHR_GPORT_TYPE_SHIFT) | \
         (_SHR_PHY_GPORT_TYPE_PHYN_SYS_SIDE_LANE_PORT << _SHR_PHY_GPORT_TYPE_SHIFT) | \
         (((_phyn) & _SHR_PHY_GPORT_PHYN_SYS_SIDE_LANE_PORT_PHYN_MASK) << \
                                _SHR_PHY_GPORT_PHYN_SYS_SIDE_LANE_PORT_PHYN_SHIFT) | \
         (((_phy_lane) & _SHR_PHY_GPORT_PHYN_SYS_SIDE_LANE_PORT_LANE_MASK) << \
                                _SHR_PHY_GPORT_PHYN_SYS_SIDE_LANE_PORT_LANE_SHIFT) | \
         (((_port) & _SHR_PHY_GPORT_PHYN_SYS_SIDE_LANE_PORT_PORT_MASK) << \
                                _SHR_PHY_GPORT_PHYN_SYS_SIDE_LANE_PORT_PORT_SHIFT))

#define _SHR_PHY_GPORT_PHYN_SYS_SIDE_LANE_PORT_PHYN_GET(_gport) \
        (((_gport) >> _SHR_PHY_GPORT_PHYN_SYS_SIDE_LANE_PORT_PHYN_SHIFT) & \
                        _SHR_PHY_GPORT_PHYN_SYS_SIDE_LANE_PORT_PHYN_MASK)
#define _SHR_PHY_GPORT_PHYN_SYS_SIDE_LANE_PORT_LANE_GET(_gport) \
        (((_gport) >> _SHR_PHY_GPORT_PHYN_SYS_SIDE_LANE_PORT_LANE_SHIFT) & \
                        _SHR_PHY_GPORT_PHYN_SYS_SIDE_LANE_PORT_LANE_MASK)
#define _SHR_PHY_GPORT_PHYN_SYS_SIDE_LANE_PORT_PORT_GET(_gport) \
        (((_gport) >> _SHR_PHY_GPORT_PHYN_SYS_SIDE_LANE_PORT_PORT_SHIFT) & \
                        _SHR_PHY_GPORT_PHYN_SYS_SIDE_LANE_PORT_PORT_MASK)

#define _SHR_GPORT_IS_PROXY(_gport) \
        ((((_gport) >> _SHR_GPORT_TYPE_SHIFT) & _SHR_GPORT_TYPE_MASK) == \
                                                _SHR_GPORT_TYPE_PROXY)

#define _SHR_GPORT_PROXY_SET(_gport, _module, _port)                       \
        ((_gport) = (_SHR_GPORT_TYPE_PROXY   << _SHR_GPORT_TYPE_SHIFT)   | \
         (((_module) & _SHR_GPORT_MODID_MASK)  << _SHR_GPORT_MODID_SHIFT)  | \
         (((_port) & _SHR_GPORT_PORT_MASK)     << _SHR_GPORT_PORT_SHIFT))

#define _SHR_GPORT_PROXY_MODID_GET(_gport)    \
        (((_gport) >> _SHR_GPORT_MODID_SHIFT) & _SHR_GPORT_MODID_MASK)

#define _SHR_GPORT_PROXY_PORT_GET(_gport)     \
        (((_gport) >> _SHR_GPORT_PORT_SHIFT) & _SHR_GPORT_PORT_MASK)

#define _SHR_GPORT_IS_FORWARD_PORT(_gport) \
        ((((_gport) >> _SHR_GPORT_TYPE_SHIFT) & _SHR_GPORT_TYPE_MASK) == \
                                                _SHR_GPORT_TYPE_FORWARD_PORT)

#define _SHR_GPORT_FORWARD_PORT_SET(_gport, _id)                            \
        ((_gport) = (_SHR_GPORT_TYPE_FORWARD_PORT << _SHR_GPORT_TYPE_SHIFT)  | \
         (((_id) & _SHR_GPORT_FORWARD_PORT_MASK) << _SHR_GPORT_FORWARD_PORT_SHIFT))

#define _SHR_GPORT_FORWARD_PORT_GET(_gport)   \
        (((_gport) >> _SHR_GPORT_FORWARD_PORT_SHIFT) & _SHR_GPORT_FORWARD_PORT_MASK)

#define _SHR_ENCAP_REMOTE_SET(_encap_id)    ((_encap_id) |= (1 << _SHR_ENCAP_REMOTE_SHIFT))
#define _SHR_ENCAP_REMOTE_GET(_encap_id)    (((_encap_id) >> _SHR_ENCAP_REMOTE_SHIFT) & _SHR_ENCAP_REMOTE_MASK)
#define _SHR_ENCAP_ID_SET(_encap_id, _id)                               \
        ((_encap_id) = ((_SHR_ENCAP_REMOTE_MASK << _SHR_ENCAP_REMOTE_SHIFT) & (_encap_id))  | \
        (((_id) & _SHR_ENCAP_ID_MASK) << _SHR_ENCAP_ID_SHIFT))
#define _SHR_ENCAP_ID_GET(_encap_id)        (((_encap_id) >> _SHR_ENCAP_ID_SHIFT) & _SHR_ENCAP_ID_MASK)



#endif  /* !_SHR_GPORT_H */
