/* $Id: nd_api.c,v 1.9 Broadcom SDK $
 * Copyright 2011 BATM
 */
#include <sal/core/libc.h>

#include "pub/nd_api.h"
#include "pub/nd_hw.h"
#include "nd_mm.h"
#include "nd_bit.h"
#include "nd_hw_test.h"
#include "nd_global.h"
#include "nd_diag.h"
#include "nd_pm.h"
#include "nd_rcr.h"
#include "nd_debug.h"
#include "nd_mac.h"
#include "nd_rpc.h"
#include "nd_nemo_diff.h"
#include "nd_init.h"
#include "nd_interrupts.h"

#include "nd_platform.h"
#include "nd_util.h"

#include "nd_channelizer.h"
#include "nd_channel.h"
#include "nd_channel_ingress.h"
#include "nd_channel_egress.h"
#include "nd_tdm.h"
#include "nd_tdm_nemo.h"
#include "nd_tdm_neptune.h"
#include "nd_diff.h"
#include "nd_cas.h"
#include "nd_ptp.h"

AG_BOOL      g_ndModuleCreated = AG_FALSE;

AgNdDevice   *g_ndDevice = NULL;

#ifdef AG_ND_ENABLE_VALIDATION
static AgResult ag_nd_profile_update(AgNdOpcodeTableEntry *p_entry, AG_U32 n_start);
#endif

AG_U32 AG_ND_SEM_LOCK_TIMEOUT = (30000); /*30 seconds*/


/*BCMadd*/
#if 0
static AG_U32    aBusAddr[AG_ND_TRACE_BUS_MAX];
static AG_U32    nBusAddrCnt = 0;
#endif

/* */
/* n_opcodes tables */
/* */
#define AG_ND_OPCODE_VAL_NAME(opcode)   opcode, #opcode

AgNdOpcodeTableEntry g_ndOpcodeWrite[AG_ND_OPCODE_MAX];
AgNdOpcodeTableEntry g_ndOpcodeRead[AG_ND_OPCODE_MAX];

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_register_opcode */
/* */
void
ag_nd_register_opcode(
    AgNdOpcode      e_opcode,
    AG_CHAR*        p_name,
    AgNdApi         p_read_callback,
    AgNdApi         p_write_callback,
    AgNdDeviceState e_init_state)
{
    assert(e_opcode < AG_ND_OPCODE_MAX);

    sal_strcpy(g_ndOpcodeRead[e_opcode].a_name, p_name);
    g_ndOpcodeRead[e_opcode].e_required_state = e_init_state;
    g_ndOpcodeRead[e_opcode].p_api = p_read_callback;
    g_ndOpcodeRead[e_opcode].n_prof_avg = 0;
    g_ndOpcodeRead[e_opcode].n_prof_cnt = 0;
    g_ndOpcodeRead[e_opcode].n_prof_max = 0;
    g_ndOpcodeRead[e_opcode].n_prof_min = (AG_U32)-1;

    sal_strcpy(g_ndOpcodeWrite[e_opcode].a_name, p_name);
    g_ndOpcodeWrite[e_opcode].e_required_state = e_init_state;
    g_ndOpcodeWrite[e_opcode].p_api = p_write_callback;
    g_ndOpcodeWrite[e_opcode].n_prof_avg = 0;
    g_ndOpcodeWrite[e_opcode].n_prof_cnt = 0;
    g_ndOpcodeWrite[e_opcode].n_prof_max = 0;
    g_ndOpcodeWrite[e_opcode].n_prof_min = (AG_U32)-1;


}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_module_create */
/* */
AgResult 
ag_nd_module_create(void)
{
    AG_ND_TRACE(0, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);


    if (g_ndModuleCreated)
        return AG_ND_ERR(0, AG_E_ND_MODULE_CREATED);


    ag_nd_sw_log_app_init();


    /* */
    /* initialize table of opcodes */
    /*  */
    ag_nd_memset(g_ndOpcodeRead, 0, sizeof(g_ndOpcodeRead));
    ag_nd_memset(g_ndOpcodeWrite, 0, sizeof(g_ndOpcodeWrite));


    /* */
    /* general opcodes */
    /* */
    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_GLOBAL),
            (AgNdApi)ag_nd_opcode_read_config_global,
            (AgNdApi)ag_nd_opcode_write_config_global,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_INIT),
            NULL,
            (AgNdApi)ag_nd_opcode_write_config_init,
            AG_ND_STATE_OPEN);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_CHIP_INFO),
            (AgNdApi)ag_nd_opcode_read_chip_info,
            NULL,
            AG_ND_STATE_OPEN);

    /* */
    /* TDM */
    /*  */
    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_CIRCUIT_ENABLE),
            (AgNdApi)ag_nd_opcode_read_config_circuit_enable,
            (AgNdApi)ag_nd_opcode_write_config_circuit_enable,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_NEMO_CIRCUIT),
            (AgNdApi)ag_nd_opcode_read_config_nemo_circuit,
            (AgNdApi)ag_nd_opcode_write_config_nemo_circuit,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_NEPTUNE_CIRCUIT),
            (AgNdApi)ag_nd_opcode_read_config_neptune_circuit,
            (AgNdApi)ag_nd_opcode_write_config_neptune_circuit,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_NEPTUNE_OC3_HIERARCHY),
            (AgNdApi)ag_nd_opcode_read_config_neptune_oc3_hierarchy,
            (AgNdApi)ag_nd_opcode_write_config_neptune_oc3_hierarchy,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_STATUS_NEPTUNE_CIRCUIT),
            (AgNdApi)ag_nd_opcode_read_status_neptune_circuit,
            NULL,
            AG_ND_STATE_INIT);

    /* */
    /* CES traffic */
    /*  */
    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_JITTER_BUFFER_PARAMS),
            (AgNdApi)ag_nd_opcode_read_config_jitter_buffer_params,
            (AgNdApi)ag_nd_opcode_write_config_jitter_buffer_params,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_CHANNEL_EGRESS),
            (AgNdApi)ag_nd_opcode_read_config_channel_egress,
            (AgNdApi)ag_nd_opcode_write_config_channel_egress,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_CHANNEL_ENABLE),
            (AgNdApi)ag_nd_opcode_read_config_channel_enable,
            (AgNdApi)ag_nd_opcode_write_config_channel_enable,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_CHANNEL_INGRESS),
            (AgNdApi)ag_nd_opcode_read_config_channel_ingress,
            (AgNdApi)ag_nd_opcode_write_config_channel_ingress,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_CHANNELIZER),
            (AgNdApi)ag_nd_opcode_read_config_channelizer,
            (AgNdApi)ag_nd_opcode_write_config_channelizer,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_COMPUTE_RING_SIZE),
            (AgNdApi)ag_nd_opcode_read_config_compute_ring_size,
            NULL,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_CONTROL_WORD),
            NULL,
            (AgNdApi)ag_nd_opcode_write_config_control_word,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_COMMAND_JBF_RESTART),
            NULL,
            (AgNdApi)ag_nd_opcode_write_command_jbf_restart,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_COMMAND_JBF_SLIP),
            NULL,
            (AgNdApi)ag_nd_opcode_write_command_jbf_slip,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_COMMAND_STOP_TRIMMING),
            NULL,
            (AgNdApi)ag_nd_opcode_write_command_stop_trimming,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_STATUS_EGRESS),
            (AgNdApi)ag_nd_opcode_read_status_egress,
            NULL,
            AG_ND_STATE_INIT);

	ag_nd_register_opcode(
			AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_VLAN_EGRESS),
			NULL,
			(AgNdApi)ag_nd_opcode_write_config_vlan_egress,
			AG_ND_STATE_INIT);
    /* */
    /* PM */
    /* */
    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_CHANNEL_PME),
            (AgNdApi)ag_nd_opcode_read_config_channel_pme,
            (AgNdApi)ag_nd_opcode_write_config_channel_pme,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_PM_EGRESS),
            (AgNdApi)ag_nd_opcode_read_pm_egress,
            NULL,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_PM_GLOBAL),
            (AgNdApi)ag_nd_opcode_read_pm_global,
            NULL,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_PM_INGRESS),
            (AgNdApi)ag_nd_opcode_read_pm_ingress,
            NULL,
            AG_ND_STATE_INIT);

    /* */
    /* CAS  */
    /* */
    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_COMMAND_CAS_CHANNEL_TX),
            NULL,
            (AgNdApi)ag_nd_opcode_write_command_cas_channel_tx,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_CAS_CHANNEL_ENABLE),
            (AgNdApi)ag_nd_opcode_read_config_cas_channel_enable,
            (AgNdApi)ag_nd_opcode_write_config_cas_channel_enable,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_CAS_CHANNEL_INGRESS),
            NULL,
            (AgNdApi)ag_nd_opcode_write_config_cas_channel_ingress,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_CAS_DATA_REPLACE),
            (AgNdApi)ag_nd_opcode_read_config_cas_data_replace,
            (AgNdApi)ag_nd_opcode_write_config_cas_data_replace,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_STATUS_CAS_CHANGE),
            (AgNdApi)ag_nd_opcode_read_status_cas_change,
            NULL,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_STATUS_CAS_DATA),
            (AgNdApi)ag_nd_opcode_read_status_cas_data,
            NULL,
            AG_ND_STATE_INIT);

    /* */
    /* PTP */
    /* */
    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_COMMAND_PTP_ADJUSTMENT),
            NULL,
            (AgNdApi)ag_nd_opcode_write_command_ptp_adjustment,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_COMMAND_PTP_CORRECT_TIMESTAMP),
            NULL,
            (AgNdApi)ag_nd_opcode_write_command_ptp_correct_timestamp,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_PTP_BRG),
            (AgNdApi)ag_nd_opcode_read_config_ptp_brg,
            (AgNdApi)ag_nd_opcode_write_config_ptp_brg,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_PTP_CHANNEL_EGRESS),
            NULL,
            (AgNdApi)ag_nd_opcode_write_config_ptp_channel_egress,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_PTP_CHANNEL_ENABLE_EGRESS),
            (AgNdApi)ag_nd_opcode_read_config_ptp_channel_enable_egress,
            (AgNdApi)ag_nd_opcode_write_config_ptp_channel_enable_egress,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_PTP_CHANNEL_ENABLE_INGRESS),
            (AgNdApi)ag_nd_opcode_read_config_ptp_channel_enable_ingress,
            (AgNdApi)ag_nd_opcode_write_config_ptp_channel_enable_ingress,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_PTP_CHANNEL_INGRESS),
            NULL,
            (AgNdApi)ag_nd_opcode_write_config_ptp_channel_ingress,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_PTP_CLK_SOURCE),
            (AgNdApi)ag_nd_opcode_read_config_ptp_clk_source,
            (AgNdApi)ag_nd_opcode_write_config_ptp_clk_source,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_PTP_STATELESS),
            (AgNdApi)ag_nd_opcode_read_config_ptp_stateless,
            (AgNdApi)ag_nd_opcode_write_config_ptp_stateless,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_PTP_TSG),
            (AgNdApi)ag_nd_opcode_read_config_ptp_tsg,
            (AgNdApi)ag_nd_opcode_write_config_ptp_tsg,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_PTP_TSG_ENABLE),
            (AgNdApi)ag_nd_opcode_read_config_ptp_tsg_enable,
            (AgNdApi)ag_nd_opcode_write_config_ptp_tsg_enable,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_COMMAND_PTP_READ_COUNTERS),
            (AgNdApi)ag_nd_opcode_read_ptp_counters,
            NULL,
            AG_ND_STATE_INIT);

    /* */
    /* RCR */
    /* */
    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_RCR_CONFIG_CHANNEL),
            (AgNdApi)ag_nd_opcode_read_rcr_config_channel,
            (AgNdApi)ag_nd_opcode_write_rcr_config_channel,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_RCR_CONFIG_TPP_TIMESTAMP_RATE),
            (AgNdApi)ag_nd_opcode_read_rcr_config_tpp_timestamp_rate,
            (AgNdApi)ag_nd_opcode_write_rcr_config_tpp_timestamp_rate,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_RCR_FLL),
            (AgNdApi)ag_nd_opcode_read_rcr_fll,
            NULL,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_RCR_PLL),
            (AgNdApi)ag_nd_opcode_read_rcr_pll,
            NULL,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_RCR_PM),
            (AgNdApi)ag_nd_opcode_read_rcr_pm,
            NULL,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_RCR_RECENT_PACKET),
            (AgNdApi)ag_nd_opcode_read_rcr_recent_packet,
            NULL,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_RCR_TIMESTAMP),
            (AgNdApi)ag_nd_opcode_read_rcr_timestamp,
            NULL,
            AG_ND_STATE_INIT);

    /* */
    /* MAC/PHY */
    /* */
    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_MAC),
            (AgNdApi)ag_nd_opcode_read_config_mac,
            (AgNdApi)ag_nd_opcode_write_config_mac,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_PM_MAC),
            (AgNdApi)ag_nd_opcode_read_mac_pm,
            NULL,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_PHY),
            (AgNdApi)ag_nd_opcode_read_config_phy,
            (AgNdApi)ag_nd_opcode_write_config_phy,
            AG_ND_STATE_INIT);


    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_BSG),
            (AgNdApi)ag_nd_opcode_read_bsg_config,
            (AgNdApi)ag_nd_opcode_write_bsg_config,
            AG_ND_STATE_INIT);

    /* */
    /* differential */
    /* */
    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_NEPTUNE_DCR_CONFIGURATIONS),
			(AgNdApi)ag_nd_opcode_read_config_neptune_dcr_configurations,
			(AgNdApi)ag_nd_opcode_write_config_neptune_dcr_configurations,
            AG_ND_STATE_INIT);
	

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_DIFF_TSO_POLL),
            (AgNdApi)ag_nd_opcode_read_diff_tso_poll,
            NULL,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_NEMO_DCR_CLK_SOURCE),
            (AgNdApi)ag_nd_opcode_read_config_nemo_dcr_clk_source,
            (AgNdApi)ag_nd_opcode_write_config_nemo_dcr_clk_source,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_NEMO_DCR_SYSTEM_TSI_CLK_SOURCE),
            (AgNdApi)ag_nd_opcode_read_config_nemo_dcr_system_clk_source,
            (AgNdApi)ag_nd_opcode_write_config_nemo_dcr_system_clk_source,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_NEMO_DCR_CONFIGURATIONS),
            (AgNdApi)ag_nd_opcode_read_config_nemo_dcr_configurations,
            (AgNdApi)ag_nd_opcode_write_config_nemo_dcr_configurations,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_NEMO_DCR_LOCAL_SAMPLE_PERIOD),
            (AgNdApi)ag_nd_opcode_read_config_nemo_dcr_local_sample_period,
            (AgNdApi)ag_nd_opcode_write_config_nemo_dcr_local_sample_period,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_NEMO_DCR_SYSTEM_LOCAL_SAMPLE_PERIOD),
            (AgNdApi)ag_nd_opcode_read_config_nemo_dcr_system_local_sample_period,
            (AgNdApi)ag_nd_opcode_write_config_nemo_dcr_system_local_sample_period,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_NEMO_DCR_LOCAL_TS_AND_FAST_PHASE_TS),
            (AgNdApi)ag_nd_opcode_read_nemo_dcr_local_ts,
            NULL,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_NEMO_DCR_SYSTEM_LOCAL_TS_AND_FAST_PHASE_TS),
            (AgNdApi)ag_nd_opcode_read_nemo_dcr_system_local_ts,
            NULL,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_NEMO_DCR_PRIME_LOCAL_TS),
            (AgNdApi)ag_nd_opcode_read_nemo_dcr_local_prime_ts,
            NULL,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_NEMO_DCR_SYSTEM_PRIME_LOCAL_TS),
            (AgNdApi)ag_nd_opcode_read_nemo_dcr_system_local_prime_ts,
            NULL,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_NEMO_DCR_SYSTEM_TSO_MODE),
            (AgNdApi)ag_nd_opcode_read_config_nemo_dcr_system_tso_mode,
            (AgNdApi)ag_nd_opcode_write_config_nemo_dcr_system_tso_mode,
            AG_ND_STATE_INIT);

    /* */
    /* diagnostic/debug opcodes */
    /* */
    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_DEBUG),
            (AgNdApi)ag_nd_opcode_read_debug,
            (AgNdApi)ag_nd_opcode_write_debug,
            AG_ND_STATE_OPEN);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_DIAG),
            (AgNdApi)ag_nd_opcode_read_diag,
            (AgNdApi)ag_nd_opcode_write_diag,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_DIAG_BIT),
            (AgNdApi)ag_nd_opcode_read_bit,
            NULL,
            AG_ND_STATE_OPEN);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_DIAG_HW_TEST),
            (AgNdApi)ag_nd_opcode_read_hw_test,
            NULL,
            AG_ND_STATE_OPEN);

    /* */
    /* clocks */
    /*  */
    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_NEMO_BRG),
            (AgNdApi)ag_nd_opcode_read_config_nemo_brg,
            (AgNdApi)ag_nd_opcode_write_config_nemo_brg,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_NEMO_CCLK),
            (AgNdApi)ag_nd_opcode_read_config_nemo_cclk,
            (AgNdApi)ag_nd_opcode_write_config_nemo_cclk,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_NEPTUNE_BSG),
            (AgNdApi)ag_nd_opcode_read_config_neptune_bsg,
            (AgNdApi)ag_nd_opcode_write_config_neptune_bsg,
            AG_ND_STATE_INIT);
    
    /* */
    /* packet classification related opcodes */
    /* */
    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_RPC_MAP_LABEL_TO_CHID),
            NULL,
            (AgNdApi)ag_nd_opcode_write_config_rpc_map_label2chid,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_RPC_POLICY),
            (AgNdApi)ag_nd_opcode_read_config_rpc_policy,
            (AgNdApi)ag_nd_opcode_write_config_rpc_policy,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_RPC_PORT_FWD),
            NULL,
            (AgNdApi)ag_nd_opcode_write_config_rpc_port_fwd,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_RPC_UCODE),
            (AgNdApi)ag_nd_opcode_read_config_rpc_ucode,
            (AgNdApi)ag_nd_opcode_write_config_rpc_ucode,
            AG_ND_STATE_INIT);

    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_DESTINATION_MAC),
            NULL,
            (AgNdApi)ag_nd_opcode_write_config_dest_mac,
            AG_ND_STATE_INIT);

	ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CONFIG_SEQUENCE_NUMBER),
            NULL,
            (AgNdApi)ag_nd_opcode_write_config_sequence_number,
            AG_ND_STATE_INIT);	

    /* */
    /* misc opcodes */
    /* */
    ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_REGISTER_ACCESS),
            (AgNdApi)ag_nd_opcode_read_register_access,
            (AgNdApi)ag_nd_opcode_write_register_access,
            AG_ND_STATE_OPEN);


	/*ORI*/
	/*add to read regs for get gs session(pm) */
	ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_CHANNEL_PME_MISSIMG_STATUS),
            (AgNdApi)ag_nd_opcode_read_channel_pme_missing_status,
            NULL,
            AG_ND_STATE_INIT);
	
	/*ORI*/
	/*frmaer config */
	ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_FRAMER_PORT_CONFIG),
            (AgNdApi)ag_nd_opcode_read_framer,
            (AgNdApi)ag_nd_opcode_write_framer,
            AG_ND_STATE_INIT);
	/*ORI*/
	/*frmaer pm */
	ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_FRAMER_PORT_PM_READ),
            (AgNdApi)ag_nd_opcode_read_framer_port_pm,
            NULL,
            AG_ND_STATE_INIT);
	/*get alarms  framer */
	ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_FRMER_PORT_STATUS),
            (AgNdApi)ag_nd_opcode_read_framer_port_status,
            NULL,
            AG_ND_STATE_INIT);
	/*transmit alarms framer  */
	ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_FRAMER_PORT_ALARM_CONTROL),
            (AgNdApi)ag_nd_opcode_read_framer_port_alarm_control,
            (AgNdApi)ag_nd_opcode_write_framer_port_alarm_control,
            AG_ND_STATE_INIT);

	/*ORI*/
	/*frmaer   loopback config */
	ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_FRAMER_PORT_TIME_SLOT_LOOPBACK_CONTROL),
            (AgNdApi)ag_nd_opcode_read_framer_port_loopback_timeslot_control,
            (AgNdApi)ag_nd_opcode_write_framer_port_loopback_timeslot_control,
            AG_ND_STATE_INIT);

	/*ORI*/
	/*frmaer   loopback config */
	ag_nd_register_opcode(
            AG_ND_OPCODE_VAL_NAME(AG_ND_OPCODE_FRAMER_PORT_LOOPBACK_CONTROL),
            (AgNdApi)ag_nd_opcode_read_framer_port_loopback_control,
            (AgNdApi)ag_nd_opcode_write_framer_port_loopback_control,
            AG_ND_STATE_INIT);
    g_ndModuleCreated = AG_TRUE;

    return AG_S_OK;
}



/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_module_remove */
/* */
AgResult 
ag_nd_module_remove(void)
{
    AG_ND_TRACE(0, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);

    if (!g_ndModuleCreated)
        return AG_ND_ERR(0, AG_E_ND_MODULE_REMOVED);

    if (g_ndDevice)
        return AG_ND_ERR(0, AG_E_ND_DEVICE_ISOPEN);

    g_ndModuleCreated = AG_FALSE;

    return AG_S_OK;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_module_trace */
/* */
AgResult 
ag_nd_module_trace(AG_U32 n_mask, AG_U16 n_level, AG_U32 *p_bus_addr, AG_U16 n_bus_addr_size)
{
    AG_U16 i;

    
    if (n_bus_addr_size > AG_ND_TRACE_BUS_MAX)
        return AG_ND_ERR(0, AG_E_ND_ARG);

    if (!p_bus_addr && n_bus_addr_size)
        return AG_ND_ERR(0, AG_E_ND_ARG);


    g_ndTraceLevel = n_level;
    g_ndTraceMask = n_mask;
    g_ndTraceBusAddrSize = n_bus_addr_size;


    for (i = 0; i < n_bus_addr_size; i++)
        g_ndTraceBusAddr[i] = p_bus_addr[i];


    return AG_S_OK;
}

/**********ORI************/
#define MAX_CES_DEVICES  1
AgNdDevice device_control[MAX_CES_DEVICES];/*GOOD ONLY TO ONE */

#ifndef BCM_CES_SDK
/*///////////////////////////////////////////////////////////////////////////// */
void ag_nd_print_all_regs()
{
  AG_U32      n_max_channel = AG_ND_NO_SPECIFIC_ENTITY;
  AG_U32      n_specific_channel = AG_ND_NO_SPECIFIC_ENTITY;
  AgNdRegAddressFormat e_format = AG_ND_REG_FMT_GLOBAL;
  AG_BOOL     b_writable =0;
  AG_BOOL     b_non_default =0;

  if(g_ndDevice == NULL)
  {
      return;	
  }
  else
  {
      n_max_channel = 3;
  	  ag_nd_all_regs_print(g_ndDevice, b_writable, b_non_default,n_max_channel,n_specific_channel,e_format);
  }
}
#endif


/* ag_nd_device_open */
/* */
AgResult 
ag_nd_device_open(AgNdMsgOpen *p_msg, AgNdHandle *p_handle)
{
    AgNdRegFpgaId                        x_id;
    AgResult                             n_ret = AG_S_OK;
    AgNdDevice                           *p_device;
    AG_U32                               i;



    AG_ND_TRACE(0, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);

    /* */
    /* sanity checks */
    /* */
    if (!g_ndModuleCreated)
        return AG_ND_ERR(0, AG_E_ND_MODULE_REMOVED);

    if (g_ndDevice)
        return AG_ND_ERR(0, AG_E_ND_DEVICE_ISOPEN);

    if (!p_msg)
        return AG_ND_ERR(0, AG_E_ND_ARG);


    /* */
    /* initialize device structure */
    /* */
    /*g_ndDevice = (AgNdDevice*)ag_nd_mm_ram_alloc(sizeof(AgNdDevice)); */
    /*ORI**********/
	g_ndDevice = &device_control[0];
    if (!g_ndDevice)
        return AG_ND_ERR(0, AG_E_ND_ALLOC);

    p_device = g_ndDevice;


    ag_nd_memset(p_device, 0, sizeof(*p_device));
    p_device->n_base = p_msg->n_base;
    p_device->b_use_hw = p_msg->b_use_hw;
    p_device->n_ext_mem_bank0_size = p_msg->n_ext_mem_bank0_size;
    p_device->n_ext_mem_bank1_size = p_msg->n_ext_mem_bank1_size;

    p_device->n_handle = *p_handle = (AgNdHandle)p_device;


    /* */
    /* temporary: enable traces by default in order to see assertions  */
    /* */
    p_device->b_trace = AG_TRUE;


    /* */
    /* initialize protection stuff */
    /* */
    n_ret = ag_nd_mutex_create(&(p_device->x_api_lock), "NdApiLck");
    if (AG_FAILED(n_ret))
    {
        ag_nd_mm_ram_free(p_device);
        return AG_ND_ERR(0, n_ret);
    }


    /* */
    /* obtain chip type */
    /*                         */
    x_id.n_reg = 0;
    ag_nd_reg_read(p_device, AG_REG_FPGA_ID, &x_id.n_reg);
    p_device->n_arch = (AG_U16)x_id.x_fields.n_architecture;
    p_device->n_code_id = (AG_U16)x_id.x_fields.n_fpga_code_id;
    p_device->n_revision = (AG_U16)x_id.x_fields.n_fpga_revision;


    /* */
    /* initialize chip dependent fields */
    /* */
    for (i = 0; AG_ND_ARCH_TYPE_NONE != g_ndChipInfoTable[i].n_code_id; i++)
        if (g_ndChipInfoTable[i].n_code_id == p_device->n_code_id)
            break;

    if (AG_ND_ARCH_TYPE_NONE == g_ndChipInfoTable[i].n_code_id)
    {
        ag_nd_mm_ram_free(p_device);
        return AG_ND_ERR(0, AG_E_ND_UNSUPPORTED);
    }

    p_device->n_total_channels = g_ndChipInfoTable[i].n_total_channels;
    p_device->n_total_ports = g_ndChipInfoTable[i].n_total_ports;
    p_device->n_chip_mask = g_ndChipInfoTable[i].n_mask;


    /* */
    /* initialize circuit architecture dependent callbacks */
    /* */
    if (AG_ND_ARCH_MASK_NEPTUNE == p_device->n_chip_mask)   /*BCM as NEMO*/
    {
        ag_nd_circuit_neptune_create_maps(p_device);

        p_device->p_circuit_is_valid_id = ag_nd_circuit_neptune_is_valid_id;
        p_device->p_circuit_get_max_idx = ag_nd_circuit_neptune_get_max_idx;
        p_device->p_circuit_id_to_idx = ag_nd_circuit_neptune_id_to_idx;
        p_device->p_circuit_idx_to_id = ag_nd_circuit_neptune_idx_to_id;
    }
    else
    {
        p_device->p_circuit_is_valid_id = ag_nd_circuit_nemo_is_valid_id;
        p_device->p_circuit_get_max_idx = ag_nd_circuit_nemo_get_max_idx;
        p_device->p_circuit_id_to_idx = ag_nd_circuit_nemo_id_to_idx;
        p_device->p_circuit_idx_to_id = ag_nd_circuit_nemo_idx_to_id;
    }


    p_device->e_state = AG_ND_STATE_OPEN;

    return n_ret;
}

/*///////////////////////////////////////////////////////////////////////////// */
/*  */
/* */
AgResult 
ag_nd_device_close(AgNdHandle n_handle)
{   
    AgResult    n_ret;
    AgNdDevice  *p_device = g_ndDevice;


    AG_ND_TRACE(0, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(0, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "handle=0x%lx\n", n_handle);


    if (!g_ndModuleCreated)
        return AG_ND_ERR(0, AG_E_ND_MODULE_REMOVED);

    if (!p_device)
        return AG_ND_ERR(0, AG_E_ND_DEVICE_ISCLOSE);

    if (p_device->e_state > AG_ND_STATE_OPEN)
    {
        /* */
        /* stop the interrupts task */
        /* */
        ag_nd_interrupts_stop(p_device);
    
        /* */
        /* free timeslot info */
        /* */
        ag_nd_mm_ram_free(p_device->a_ts_info_table);
        
        /* */
        /* free allocators */
        /* */
        #ifndef CES16_BCM_VERSION
        ag_nd_allocator_destroy(&(p_device->x_allocator_jbf));
        ag_nd_allocator_destroy(&(p_device->x_allocator_pbf));
		if (!p_device->b_dynamic_memory)
		{
        	ag_nd_allocator_destroy(&(p_device->x_allocator_vba));
		}
    
        #endif    
        /* */
        /* destroy label search tree miniprogram */
        /* */
        ag_nd_mem_unit_set(p_device, p_device->p_mem_lstree);
        n_ret = ag_clsb_del_hw_mem(p_device->n_lstree_handle);
        assert(AG_SUCCEEDED(n_ret));
        /* bail if asserts are disabled */
        if (AG_FAILED(n_ret)) {
            return AG_E_FAIL;
        }
    }

    /* */
    /* destroy protection stuff */
    /* */
    ag_nd_mutex_destroy(&(p_device->x_api_lock));


    /*  */
    /* reset registers */
    /*  */
    ag_nd_regs_reset(p_device);


    /* */
    /* destroy the device itself */
    /* */
    /*ag_nd_mm_ram_free(p_device); Not dynamically allocated*/


    g_ndDevice = NULL;


    return AG_S_OK;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_device_read */
/* ag_nd_device_read_ext */
/* */
AgResult
ag_nd_device_read(AgNdHandle n_handle, AG_U16 n_opcode, void *p_msg)
{
    return ag_nd_device_read_ext(n_handle, n_opcode, p_msg, 0);
}

AgResult 
ag_nd_device_read_ext(AgNdHandle n_handle, AG_U16 n_opcode, void *p_msg, AG_U32 n_flags)
{
    AgResult n_ret;
    AgResult n_mutex;


#ifdef AG_ND_ENABLE_VALIDATION
    AG_U32 n_timer;
    /* */
    /* profiling */
    /* */
    n_timer = ag_nd_time_usec();


    /* */
    /* sanity */
    /* */
    if (!g_ndModuleCreated)
        return AG_ND_ERR(0, AG_E_ND_MODULE_REMOVED);

    if (!g_ndDevice)
        return AG_ND_ERR(0, AG_E_ND_DEVICE_ISCLOSE);

    if (!p_msg)
        return AG_ND_ERR(g_ndDevice, AG_E_ND_ARG);

    if (n_opcode > AG_ND_OPCODE_MAX)
        return AG_ND_ERR(g_ndDevice, AG_E_ND_ARG);
#endif

    if (g_ndOpcodeRead[n_opcode].p_api)
    {
        if (g_ndDevice->e_state < g_ndOpcodeRead[n_opcode].e_required_state)
            n_ret = AG_E_ND_STATE;
        else
        {
            if (0 == (n_flags & AG_ND_OPFLAG_NOLOCK))
            {
                n_mutex = ag_nd_mutex_lock(&(g_ndDevice->x_api_lock), AG_ND_SEM_LOCK_TIMEOUT);
                if (AG_FAILED(n_mutex))
                    n_ret = AG_ND_ERR(g_ndDevice, n_mutex);
            }
			/**********ORI*******************/
           	AG_ND_TRACE(g_ndDevice, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s%s%d\n", "****ORI*** "," DEBUG "," READ**ENETR******* ",n_opcode);
            n_ret = g_ndOpcodeRead[n_opcode].p_api(g_ndDevice, (void*)p_msg);
			AG_ND_TRACE(g_ndDevice, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s%s%d\n", "****ORI*** "," DEBUG "," READ**OUT******* ",n_opcode);
            if (0 == (n_flags & AG_ND_OPFLAG_NOLOCK))
            {
                n_mutex = ag_nd_mutex_unlock(&(g_ndDevice->x_api_lock));
                assert(AG_SUCCEEDED(n_mutex));
            }
        }
    }
    else
        n_ret = AG_E_ND_UNSUPPORTED;


#ifdef AG_ND_ENABLE_VALIDATION
    /* */
    /* profiling */
    /* */
    ag_nd_profile_update(&g_ndOpcodeRead[n_opcode], n_timer);
#endif

    return n_ret;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_device_write */
/* ag_nd_device_write_ext */
/* */
AgResult 
ag_nd_device_write(AgNdHandle n_handle, AG_U16 n_opcode, void *p_msg)
{
    return ag_nd_device_write_ext(n_handle, n_opcode, p_msg, 0);
}

AgResult 
ag_nd_device_write_ext(AgNdHandle n_handle, AG_U16 n_opcode, void *p_msg, AG_U32 n_flags)
{
    AgResult n_ret;
    AgResult n_mutex;


#ifdef AG_ND_ENABLE_VALIDATION
    AG_U32 n_timer;
    /* */
    /* profiling */
    /* */
    n_timer = ag_nd_time_usec();


    /* */
    /* sanity */
    /* */
    if (!g_ndModuleCreated)
        return AG_ND_ERR(0, AG_E_ND_MODULE_REMOVED);

    if (!g_ndDevice)
        return AG_ND_ERR(0, AG_E_ND_DEVICE_ISCLOSE);

    if (!p_msg)
        return AG_ND_ERR(g_ndDevice, AG_E_ND_ARG);

    if (n_opcode > AG_ND_OPCODE_MAX)
        return AG_ND_ERR(g_ndDevice, AG_E_ND_ARG);
#endif


    if (g_ndOpcodeWrite[n_opcode].p_api)
    {
        if (g_ndDevice->e_state < g_ndOpcodeWrite[n_opcode].e_required_state)
            n_ret = AG_E_ND_STATE;
        else
        {
            if (0 == (n_flags & AG_ND_OPFLAG_NOLOCK))
            {
                n_mutex = ag_nd_mutex_lock(&(g_ndDevice->x_api_lock), AG_ND_SEM_LOCK_TIMEOUT);
                if (AG_FAILED(n_mutex))
                    n_ret = AG_ND_ERR(g_ndDevice, n_mutex);
            }
            /***********************ORI****************************/
			/*AG_ND_TRACE(g_ndDevice, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s%s%d\n", "****ORI*** "," DEBUG "," WRITE**ENETR******* ",n_opcode); */
            n_ret = g_ndOpcodeWrite[n_opcode].p_api(g_ndDevice, (void*)p_msg);
			/*AG_ND_TRACE(g_ndDevice, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s%s%d\n", "****ORI*** "," DEBUG "," WRITE**OUT*********",n_opcode); */
            if (0 == (n_flags & AG_ND_OPFLAG_NOLOCK))
            {
                n_mutex = ag_nd_mutex_unlock(&(g_ndDevice->x_api_lock));
                assert(AG_SUCCEEDED(n_mutex));
            }
        }
    }
    else
        n_ret = AG_ND_ERR(g_ndDevice, AG_E_ND_UNSUPPORTED);

#ifdef AG_ND_ENABLE_VALIDATION

    /* */
    /* profiling */
    /* */
    ag_nd_profile_update(&g_ndOpcodeWrite[n_opcode], n_timer);
#endif

    return n_ret;
}



/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_profile_update */
/* */
#ifdef AG_ND_ENABLE_VALIDATION
static AgResult
ag_nd_profile_update(AgNdOpcodeTableEntry *p_entry, AG_U32 n_start)
{
    AG_U32 n_stop; 
    AG_U32 n_elapsed;
    AG_U64 n_avg;

    n_stop = ag_nd_time_usec();

    if (n_stop < n_start)
        n_elapsed = (AG_U32)-1 - n_start + n_stop;
    else
        n_elapsed = n_stop - n_start;


    n_avg = (AG_U64)p_entry->n_prof_avg * p_entry->n_prof_cnt + n_elapsed;
    p_entry->n_prof_cnt++;
    p_entry->n_prof_avg = (AG_U32)(n_avg / p_entry->n_prof_cnt);

    if (n_elapsed > p_entry->n_prof_max)
        p_entry->n_prof_max = n_elapsed;

    if (n_elapsed < p_entry->n_prof_min)
        p_entry->n_prof_min = n_elapsed;


    return AG_S_OK;
}
#endif
/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_create_circuit  */
/* */
AgNdCircuit 
ag_nd_create_circuit_id_nemo(AG_U32 n_port)
{
    return ag_nd_circuit_nemo_create_id(n_port);
}

AgNdCircuit 
ag_nd_create_circuit_id_neptune(AG_U32 n_spe, AG_U32 n_vtg, AG_U32 n_vt)
{
    return ag_nd_circuit_neptune_create_id(n_spe,n_vtg,n_vt);
}
/*ORI*/
int ag_nd_get_device_handle(int unit_index,AgNdHandle * phandle)
{
	*phandle = device_control[unit_index].n_handle;
	return 1;
}
int ag_nd_get_device(int unit_index,AgNdDevice *pdevice)
{
	*pdevice= device_control[unit_index];
	return 1;
}

