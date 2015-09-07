/*
 * $Id: stktask.h,v 1.21 Broadcom SDK $
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
 * File:        stktask.h
 * Requires:    Definition of disc_pkt_type_get to get type of a disc pkt
 *              DISC packet type definitions
 *              CPUDB module
 */

#ifndef   _STKTASK_H_
#define   _STKTASK_H_

#include <sdk_config.h>
#include <sal/core/thread.h>
#include <sal/core/time.h>

#include <bcm/types.h>

#include <appl/cpudb/cpudb.h>

#include <shared/evlog.h>    /* Event Logging */

#include <appl/discover/disc.h> /* disc_election_cb_t */

/* See requirements above */

/****************************************************************
 *
 * Stack task states and events
 *
 * 
 * The normal transitions are:
 * 
 *             BLOCKED ------> READY --------> DISC
 *                ^                             | 
 *                |                             V
 *                +----------- ATTACH <------- TOPO
 * 
 *     From       -> To            Reason
 * 
 *     BLOCKED    -> READY         Unblock event or blocked timeout
 *     READY      -> DISC          Various events start discovery
 *     DISC       -> TOPO          DISC_SUCCESS
 *     TOPO       -> ATTACH        TOPO_SUCCESS
 *     ATTACH     -> BLOCKED       ATTACH_SUCCESS
 * 
 * The application may provide a callback function which will be called
 * on all state transitions.
 * 
 ****************************************************************/

typedef enum bcm_st_state_e {
    BCM_STS_INVALID = 0,

    BCM_STS_BLOCKED,   /* Record events and wait for unblock */
    BCM_STS_READY,     /* Ready to start discovery */
    BCM_STS_DISC,      /* Discovery is running */
    BCM_STS_TOPO,      /* Topology processing is running */
    BCM_STS_ATTACH,    /* Attach callbacks are running */

    BCM_STS_MAX
} bcm_st_state_t;

#define BCM_ST_STATE_STRINGS {                                \
    "INVALID",                                                \
    "BLOCKED",                                                \
    "READY",                                                  \
    "DISC",                                                   \
    "TOPO",                                                   \
    "ATTACH",                                                 \
    "" }

/*
 * These are the default timeout settings, per state.  Note that discovery
 * has its own timeout mechanism as well.  In addition, st_cfg_flags
 * supports an automatic transition from blocked to ready.
 */

#ifndef BCM_STATE_TIMEOUT_DEFAULTS
#define BCM_STATE_TIMEOUT_DEFAULTS {                          \
        0,                       /* BCM_STS_INVALID */        \
        0,                       /* BCM_STS_BLOCKED */        \
        0,                       /* BCM_STS_READY */          \
        0,                       /* BCM_STS_DISC */           \
        10 * ATP_TIMEOUT_DEFAULT, /* BCM_STS_TOPO */           \
        15 * ATP_TIMEOUT_DEFAULT  /* BCM_STS_ATTACH */         \
    }
#endif


extern char * bcm_st_state_strings[];

#define BCM_STS_VALID(state) ((state) > BCM_STS_INVALID && (state) < BCM_STS_MAX)


/****************************************************************
 *
 * Event precedence:
 *
 * NOTE:  The order of these flags determines the order in which
 * they are processed when multiple events are pending.
 *
 * 
 * Events trigger transitions from one state to another.  The order of
 * events in this list is important because it determines the order in
 * which they are evaluated.  This affects the precedence of the events.
 * For example, UNBLOCK is first so that if UNBLOCK and BLOCK events are
 * seen together, the BLOCK event will be processed second, leaving Stack
 * Task in the BLOCKED state.
 * 
 * On the other hand, SUCCESS is given precedence over FAILURE in
 * general by appearing first (Stack Task will transition due to the
 * success signal first and then the failure will be discarded.)
 *
 * 
 * Below, "internal" means the event is generated by Stack Task;
 * "external" means the event may be expected to be generated by the
 * application via bcm_st_event_send.  Some events may be both.
 * 
 *     UNBLOCK (external)
 *         Transition from BLOCKED to READY.  This is sent by the
 *         application to indicate it is ready to participate in
 *         discovery.
 * 
 *     BLOCK (external)
 *         Transition to the BLOCKED state from any other state.  This is
 *         sent by the application and generally indicates an error
 *         state.  It will terminate a running discovery and prevent
 *         discovery from re-running until an UNBLOCK or TIMEOUT is
 *         received.
 * 
 *     LINK_UP, LINK_DOWN (internal, external)
 *         Stack Task registers with linkscan to receive link events.
 *         When that callback is made for a stack port, this event
 *         occurs. 
 * 
 *     DISC_PKT (internal, external)
 *         A discovery packet was received by Stack Task indicating some
 *         other box may be trying to start discovery.
 * 
 *     DISC_SUCCESS, DISC_FAILURE (internal)
 *         These are sent upon the completion of discovery.
 * 
 *     DISC_RESTART (internal, external)
 *         Indicates discovery should be restarted.
 * 
 *     TOPO_SUCCESS, TOPO_FAILURE (external)
 *         Results of topology processing.
 * 
 *     ATTACH_SUCCESS, ATTACH_FAILURE (external)
 *         Results of device attach processing.
 * 
 *     TIMEOUT (internal)
 *         A timeout has occurred for a phase.  Generally go to BLOCKED.
 * 
 *     COMM_FAILURE (internal, external)
 *         A communications failure has occurred.  This usually results
 *         in a restart of discovery.
 *  
 *****************************************************************/

typedef enum bcm_st_event_e {
    BCM_STE_INVALID = 0,

    BCM_STE_UNBLOCK,           /* Move from BLOCKED to IDLE */
    BCM_STE_BLOCK,             /* Move to BLOCKED from any state */
    BCM_STE_LINK_UP,
    BCM_STE_LINK_DOWN,
    BCM_STE_DISC_PKT,          /* A discovery pkt seen from a stk port */
    BCM_STE_DISC_RESTART,      /* Restart discovery if running */
    BCM_STE_DISC_SUCCESS,
    BCM_STE_DISC_FAILURE,
    BCM_STE_TOPO_SUCCESS,
    BCM_STE_TOPO_FAILURE,
    BCM_STE_ATTACH_SUCCESS,
    BCM_STE_ATTACH_FAILURE,
    BCM_STE_TIMEOUT,
    BCM_STE_COMM_FAILURE,      /* Communication failure seen */

    BCM_STE_MAX
} bcm_st_event_t;

#define BCM_ST_EVENT_STRINGS {    \
    "INVALID",                    \
    "UNBLOCK",                    \
    "BLOCK",                      \
    "LINK_UP",                    \
    "LINK_DOWN",                  \
    "DISC_PKT",                   \
    "DISC_RESTART",               \
    "DISC_SUCCESS",               \
    "DISC_FAILURE",               \
    "TOPO_SUCCESS",               \
    "TOPO_FAILURE",               \
    "ATTACH_SUCCESS",             \
    "ATTACH_FAILURE",             \
    "TIMEOUT",                    \
    "COMM_FAILURE",               \
    "" }

extern char * bcm_st_event_strings[];

#define BCM_STE_VALID(event) ((event) > BCM_STE_INVALID && (event) < BCM_STE_MAX)

#define BCM_STE_FLAG(event) (1 << (event))

extern cpudb_ref_t volatile bcm_st_cur_db;
extern cpudb_ref_t volatile bcm_st_disc_db;

/****************************************************************
 *
 * Configuration function prototypes
 *    and the configuration structure.
 *
 * 
 * int (*bcm_st_disc_f)(cpudb_ref_t db_ref, bcm_st_master_f done_cb);
 * 
 *     The one-shot discovery function.  db_ref must be initialized with
 *     the local information.  By some mechanism, it discovers the system
 *     members and interconnections, filling out db_ref.  Once completed,
 *     it calls the master election done_cb (see below).
 * 
 *     Returns BCM_E_NONE on success.
 * 
 *     Returns DISC_RESTART_NEW_SEQ to request the caller change the
 *     discovery sequence number and call again.
 * 
 *     Returns DISC_RESTART_REQUEST to request the caller restart
 *     discovery (without changing the discovery sequence number).
 * 
 * int (*bcm_st_master_f)(cpudb_ref_t db_ref);
 * 
 *     Takes a completed CPUDB and determines the master entry.  It sets
 *     db_ref->master_entry to this value if found and returns
 *     BCM_E_NONE.
 * 
 * int (*bcm_st_disc_abort_f)(int disc_rv, int timeout_us);
 * 
 *     The caller (Stack Task) uses this to interrupt a running
 *     discovery.  It may intend to abort it or just to restart it.  This
 *     difference is reflected in the disc_rv that is passed which is
 *     used by discovery when it exits.
 * 
 *     If timeout_us > 0, the call will block until discovery
 *     acknowledges that it has exited (returns BCM_E_NONE), or until the
 *     timeout is reached (returns BCM_E_FAIL).
 * 
 * int (*bcm_st_transition_f)(st_state_t from,
 *                            st_event_t event,
 *                            st_state_t to,
 *                            cpudb_ref_t disc_db,
 *                            cpudb_ref_t cur_db);
 * 
 * 
 *     If present, Stack Task calls this each time a state transition
 *     takes place.  Events sent to Stack Task from this callback will be
 *     processed after the transition has occurred.
 * 
 *     The disc_db and cur_db are pointers to the discovery and current
 *     database pointers.  In general, they should be treated as
 *     "read-only".  Either or both may be NULL; note that the case of
 *     "no previous successful discovery" can be detected by testing
 *     (cur_db == NULL).
 * 
 *     If the transition function returns an error (< 0) then ST will
 *     transition to BLOCKED (without an additional callback).
 * 
 * st_topo and st_attach: int (*bcm_st_update_f)(cpudb_ref_t db_ref);
 * 
 *     These are called at the transitions to the TOPO and ATTACH states,
 *     respectively.  For st_topo, the discovery data base is passed as
 *     the parameter.  For st_attach, the new current database is passed
 *     as the parameter.
 * 
 *     The values for these functions in the Broadcom distribution are
 *     bcm_stack_topo_update and bcm_stack_attach_update.
 * 
 *     The return values for these functions are only used to generate
 *     warning messages if they fail.  The application must send a BLOCK
 *     event if it wishes to change the state of Stack Task.
 *
 *
 ****************************************************************/

typedef disc_start_election_cb_t bcm_st_master_f;

typedef int (*bcm_st_disc_f)(cpudb_ref_t db_ref,
                             bcm_st_master_f done_cb);
typedef int (*bcm_st_disc_abort_f)(int disc_rv, int timeout_us);
typedef int (*bcm_st_transition_f)(bcm_st_state_t from,
                                   bcm_st_event_t event,
                                   bcm_st_state_t to,
                                   cpudb_ref_t disc_db,
                                   cpudb_ref_t cur_db);

typedef int (*bcm_st_update_f)(cpudb_ref_t db_ref);

typedef struct bcm_st_config_s {
    bcm_st_disc_f          st_disc_start;  /* Start discovery */
    bcm_st_disc_abort_f    st_disc_abort;  /* Abort or restart discovery */
    bcm_st_master_f        st_master;      /* election functions */

    bcm_st_update_f        st_topo;        /* Start topo processing */
    bcm_st_update_f        st_attach;      /* Start attach processing */

    bcm_st_transition_f    st_transition;  /* All transitions */
    
    cpudb_base_t           base;           /* Local system info */
} bcm_st_config_t;

/****************************************************************
 *
 * API functions
 *
 * config_load may be called before start to set up the
 * configuration; then ports may be deactivated before starting.
 *
 * Start/stop stack task
 *
 * Update the base configuration for the system
 *
 * Change/check timeouts associated with each state.
 *     A timeout may be assigned to each state.  All states will
 * transition to BLOCKED if a timeout occurs except for BLOCKED
 * which will transition to READY.  A timeout of 0 means disabled.  
 *     Default timeouts are all 0 (disabled).
 *
 ****************************************************************/

extern int bcm_st_config_load(bcm_st_config_t *config);

extern int bcm_st_start(bcm_st_config_t *config, int enable);
extern int bcm_st_stop(int timeout_us);

extern int bcm_st_base_update(cpudb_base_t *base, int restart);

extern int bcm_st_event_send(bcm_st_event_t event);

extern int bcm_st_timeout_set(bcm_st_state_t state, sal_usecs_t to);
extern int bcm_st_timeout_get(bcm_st_state_t state, sal_usecs_t *to);

/* See notes below about bcm_st_cfg_flags */
extern int bcm_st_link_state_set(int unit, bcm_port_t port, int link);
extern int bcm_st_link_state_get(int unit, bcm_port_t port, int *link);

/****************************************************************
 *
 * These determine which events allow a transition from
 * READY -> DISC
 *
 * DISC_RESTART is treated specially in some places in the code.
 * It is really a "hard" restart and forces a transition to DISC
 * from TOPO or ATTACH states as well.
 *
 ****************************************************************/

extern uint32 bcm_st_disc_startable_events;
#define BCM_ST_DISC_STARTABLE_EVENTS_DEFAULT (          \
    BCM_STE_FLAG(BCM_STE_LINK_UP)           |           \
    BCM_STE_FLAG(BCM_STE_LINK_DOWN)         |           \
    BCM_STE_FLAG(BCM_STE_DISC_PKT)          |           \
    BCM_STE_FLAG(BCM_STE_DISC_RESTART)      |           \
    BCM_STE_FLAG(BCM_STE_COMM_FAILURE))

/* These are events that may start discovery directly */
#define BCM_STE_DISC_STARTABLE(event) \
    ((BCM_STE_FLAG(event) & bcm_st_disc_startable_events) != 0)

#ifndef BCM_ST_DISC_PRIORITY_DEFAULT
#define BCM_ST_DISC_PRIORITY_DEFAULT        100
#endif
#ifndef BCM_ST_DISC_STACK_SIZE_DEFAULT
#define BCM_ST_DISC_STACK_SIZE_DEFAULT      SAL_THREAD_STKSZ
#endif

extern int bcm_st_disc_priority;
extern int bcm_st_disc_stk_size;

/*
 * If an unexpected event occurs, a warning is produced.  This can
 * be set to 0 to squelch warnings.
 */

#ifndef BCM_ST_MAX_BAD_EVENT_WARNINGS_DEFAULT
#define BCM_ST_MAX_BAD_EVENT_WARNINGS_DEFAULT 10
#endif
extern int bcm_st_max_bad_event_warnings;

/****************************************************************
 *
 * If discovery is stopped, wait for the stack task discovery thread
 * to sleep.  This is the max number of retries, with 10000 usecs of
 * sleep between each.
 *
 ****************************************************************/

#define BCM_ST_DISC_STOP_RETRIES_MAX 1000

/****************************************************************
 *
 * Stack Task state flags.  These are only exposed for debugging.
 *
 ****************************************************************/

#define BCM_STF_CONFIG_LOADED        0x1
#define BCM_STF_RUNNING              0x2
#define BCM_STF_ABORT                0x4
#define BCM_STF_DISC_SLEEPING        0x8
#define BCM_STF_MAX                  4

#define BCM_ST_FLAGS_STRINGS {    \
    "CONFIG LOADED",              \
    "RUNNING",                    \
    "ABORT",                      \
    "DISC SLEEPING",              \
    "" }

extern char * bcm_st_flags_strings[];

/****************************************************************
 *
 * Stack Task configuration parameters
 *
 ****************************************************************/

/*
 * bcm_st_cfg_flags: Configuration flags for stack task.
 * 
 * BCM_STC_AUTO_B2R: Indicates stack task should automatically transition
 * from blocked to ready.  This may change dynamically.
 *
 * BCM_STC_INITIAL_LINK_UP:  When the stack port links are initially
 * checked, should a link up event be sent to stack task if any
 * links are found?
 *
 * Applications may control link and communication failure signalling
 * to stack task by clearing the following bits and setting up
 * their own routines to send signals.  NOTE, however, that link
 * status maintains a lot of state and does its own debounce.  If
 * the application sends link signals, it must call st_link_state_set
 * as well.  Debounce is then disabled.
 *
 * BCM_STC_LINK_REGISTER:  Use the default link change callback
 * handler.
 *
 * BCM_STC_COMM_FAIL_REGISTER:  Use the default communication
 * failure callback handler, registering with ATP.
 *
 * BCM_STC_INITIAL_LINK_UP:  When the stack port links are initially
 * checked, should a link up event be sent to stack task if any
 * links are found?
 *
 * BCM_STC_DISC_PKT_REGISTER:  Register to get next hop pkts, check them
 * and send disc_pkt events to stack task.
 *
 * BCM_STC_START_ATP:  Should stack task check and start ATP if it's not
 * running?
 *
 * bcm_st_link_up_db_usec:  Number of microseconds that a stack port
 * must remain up before an actual link up event is given.  Link down
 * is always payed attention to immediately (if current state is up).
 *
 * Stack ports may be enabled/disabled by calling bcm_st_stk_port_enable_set.
 * The configuration must already be initialized or else the port
 * won't be found.
 */

extern volatile uint32 bcm_st_cfg_flags;

#define BCM_STC_AUTO_B2R            0x1
#define BCM_STC_INITIAL_LINK_UP     0x2
#define BCM_STC_LINK_REGISTER       0x4
#define BCM_STC_COMM_FAIL_REGISTER  0x8
#define BCM_STC_DISC_PKT_REGISTER   0x10
#define BCM_STC_START_ATP           0x20


#define BCM_ST_CFG_FLAGS_STRINGS {     \
    "AUTO B2R",                        \
    "INITIAL LINK UP",                 \
    "LINK REGISTER",                   \
    "COMM FAIL REGISTER",              \
    "DISC PKT REGISTER",               \
    "START ATP",                       \
    "" }

#define BCM_ST_CFG_FLAGS_DEFAULT (     \
        BCM_STC_AUTO_B2R             | \
        BCM_STC_INITIAL_LINK_UP      | \
        BCM_STC_LINK_REGISTER        | \
        BCM_STC_COMM_FAIL_REGISTER   | \
        BCM_STC_DISC_PKT_REGISTER    | \
        BCM_STC_START_ATP              \
    )

extern volatile int bcm_st_link_up_db_usec;
#ifndef BCM_ST_LINK_UP_DB_USEC_DEFAULT
#define BCM_ST_LINK_UP_DB_USEC_DEFAULT 1000000
#endif

extern int bcm_st_stk_port_enable_set(int unit, int port, int enable);
extern int bcm_st_stk_port_enable_get(int unit, int port, int *enable);

/* For stack port flags */
#define ST_SPF_LINK_UP            0x1
#define ST_SPF_LAST_EVENT_UP      0x2
#define ST_SPF_ETHERNET           0x4
#define ST_SPF_DISABLED           0x10000000

extern volatile uint32 bcm_st_atp_flags;
#ifndef BCM_ST_ATP_FLAGS_DEFAULT
#define BCM_ST_ATP_FLAGS_DEFAULT ATP_F_LEARN_SLF
#endif

extern int bcm_st_reserved_modid_enable_set(int value);
extern int bcm_st_reserved_modid_enable_get(void);

extern int bcm_st_transition(bcm_st_state_t from,
                             bcm_st_event_t event,
                             bcm_st_state_t to,
                             cpudb_ref_t disc_db,
                             cpudb_ref_t cur_db);


/* Event logging */
SHARED_EVLOG_EXTERN(st_log)

extern cpudb_ref_t bcm_st_current_db_get(void);
extern cpudb_ref_t bcm_st_discovery_db_get(void);

#endif /* _STKTASK_H_ */
