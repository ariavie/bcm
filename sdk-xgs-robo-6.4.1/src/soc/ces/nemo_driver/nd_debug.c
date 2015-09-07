/* $Id: nd_debug.c,v 1.7 Broadcom SDK $
 * Copyright 2011 BATM
 */


#ifndef BCM_CES_SDK
#include <stdarg.h>
#include <stdio.h>
#else
#include "bcm_ces_sal.h"
#endif

#include "pub/nd_api.h"
#include "pub/nd_hw.h"
#include "nd_mm.h"
#include "nd_registers.h"
#include "nd_debug.h"
#include "nd_ucode.h"
#include "nd_rpc.h"
#include "nd_util.h"


AG_U32 g_ndTraceMask  = AG_ND_TRACE_API;
AG_U16 g_ndTraceLevel = AG_ND_TRACE_ERROR;
AG_U32 g_ndTraceBusAddr[AG_ND_TRACE_BUS_MAX];
AG_U16 g_ndTraceBusAddrSize = 0;
AG_BOOL g_force_kiss_out = AG_FALSE;
void ag_nd_set_kiss_debug(AG_BOOL force)  /*BCMadd*/
{
  g_force_kiss_out = force;
}

/*///////////////////////////////////////////////////////////////////////////// */
/*  */
/* */
void
ag_nd_vtrace(AgNdDevice *p_device, 
        AG_U32 n_mask, 
        AG_U16 n_level, 
        const char *p_file, 
        int n_line, 
        const char *p_format,
        va_list argp)
{
#ifndef BCM_CES_SDK
    char a_buf[AG_ND_PRINTF_MAX];
#endif

   #if 1
    if (g_ndTraceLevel < n_level)
        return;

    if (!(g_ndTraceMask & n_mask))
        return;

    if (p_device)
        if (!p_device->b_trace)
            return;
   #endif
#ifndef BCM_CES_SDK
    vsnprintf(a_buf, sizeof(a_buf), p_format, argp);



    if (AG_ND_TRACE_DEBUG <= n_level)
        printf("%s/%d %s", p_file, n_line, a_buf);
    else
        ag_nd_sw_log(n_level, a_buf, p_file, n_line);
#endif
}

void
ag_nd_trace(AgNdDevice *p_device, 
        AG_U32 n_mask, 
        AG_U16 n_level, 
        const char *p_file, 
        int n_line, 
        const char *p_format, ...)
{
    va_list argp;
    va_start(argp, p_format);
    ag_nd_vtrace(p_device, n_mask, n_level, p_file, n_line, p_format, argp);
}

#if defined WIN32
void
AG_ND_TRACE(AgNdDevice *p_device, AG_U32 n_mask, AG_U16 n_level, const char *p_format, ...)
{
    va_list argp;
    va_start(argp, p_format);
    ag_nd_vtrace(p_device, n_mask, n_level, "unknown", 0, p_format, argp);
}
#endif


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_debug */
/* */
AgResult
ag_nd_opcode_write_debug(AgNdDevice *p_device, AgNdMsgDebug *p_msg)
{
    AgNdRegTestBusControl   x_test;
    AgNdRegGlobalControl    x_glb_ctl;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_rclr_pm_counters=%lu\n", p_msg->b_rclr_pm_counters);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_trace=%lu\n", p_msg->b_trace);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_select_block=%hu\n", p_msg->n_select_block);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_select_group=%hu\n", p_msg->n_select_group);


    x_test.n_reg = 0;
    x_test.x_fields.n_block_select = p_msg->n_select_block;
    x_test.x_fields.n_group_select = p_msg->n_select_group;
    ag_nd_reg_write(p_device, AG_REG_TEST_BUS_CONTROL, x_test.n_reg);


    ag_nd_reg_read(p_device, AG_REG_GLOBAL_CONTROL, &(x_glb_ctl.n_reg));
    x_glb_ctl.x_fields.b_rclr_on_host_access = (field_t) p_msg->b_rclr_pm_counters;
    ag_nd_reg_write(p_device, AG_REG_GLOBAL_CONTROL, x_glb_ctl.n_reg);


    p_device->b_trace = p_msg->b_trace;

    return AG_S_OK;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_debug */
/* */
AgResult
ag_nd_opcode_read_debug(AgNdDevice *p_device, AgNdMsgDebug *p_msg)
{
    AgNdRegTestBusControl   x_test;
    AgNdRegGlobalControl    x_glb_ctl;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);


    ag_nd_reg_read(p_device, AG_REG_TEST_BUS_CONTROL, &(x_test.n_reg));
    p_msg->n_select_block = x_test.x_fields.n_block_select;
    p_msg->n_select_group = x_test.x_fields.n_group_select;


    ag_nd_reg_read(p_device, AG_REG_GLOBAL_CONTROL, &(x_glb_ctl.n_reg));
    p_msg->b_rclr_pm_counters = x_glb_ctl.x_fields.b_rclr_on_host_access;
    

    p_msg->b_trace = p_device->b_trace;


    return AG_S_OK;
}
#ifndef BCM_CES_SDK
/*///////////////////////////////////////////////////////////////////////////// */
/* dump all registers */
/* */
AgResult ag_nd_debug_regs_print(
    AgNdDevice *p_device, 
    AG_BOOL b_writableRegistersOnly,
    AG_BOOL b_nonDefaultValuesOnly,
    AG_U32 n_max_entity,
    AG_U32 n_specific_entity,
    AgNdRegAddressFormat e_format)
{
    AG_U32  i;
    AG_U32  n_id; /* circuit/channel id */
    AG_U32  n_addr;
    AG_U32  n_value32;
    AG_U16  n_value16;
    AG_U32  n_entry;
    AG_BOOL b_name_printed;


    assert(p_device);


    printf("\n-- registers dump start\n");
    printf("%s registers\n", b_writableRegistersOnly ? "rw" : "other");


    /* */
    /* iterate over all registers */
    /* */
    for (i = 0; (AG_U32)-1 != g_ndRegTable[i].n_address; i++)
    {
        if (e_format != AG_ND_REG_FMT_ALL &&
            g_ndRegTable[i].e_format != e_format)
        {
            continue;
        }
        
        if (b_writableRegistersOnly && 
            AG_ND_REG_TYPE_RW != g_ndRegTable[i].e_type)
            continue;

        if (!(g_ndRegTable[i].n_chip & p_device->n_chip_mask)) 
            continue;

        strtok(g_ndRegTable[i].p_name, "(");
        b_name_printed = AG_FALSE;


        /*  */
        /* iterate over all circuit/channel ids */
        /* */
        n_id = 0;

        do
        {   
            /* */
            /* iterate over all entries in register array */
            /* */
            if (n_specific_entity != AG_ND_NO_SPECIFIC_ENTITY &&
                n_id != n_specific_entity)
            {
                n_id = ag_nd_next_reg_id(p_device, &(g_ndRegTable[i]), n_id);
                if (n_id == (AG_U32)-1)
                    break;
                continue;
            }

            for (n_entry = 0; n_entry < g_ndRegTable[i].n_size; n_entry++)
            {
                n_addr = g_ndRegTable[i].n_address;                 /* base register address */
                n_addr += n_entry * g_ndRegTable[i].n_width >> 3;   /* entry offset */
                n_addr += n_id << 12;                               /* channel/circuit id offset */


                if (32 == g_ndRegTable[i].n_width)
                    ag_nd_reg_read_32bit(p_device, n_addr, &n_value32);
                else
                    ag_nd_reg_read(p_device, n_addr, &n_value16);


                if (b_nonDefaultValuesOnly && 
                    32 == g_ndRegTable[i].n_width &&
                    g_ndRegTable[i].n_default == n_value32)
                    continue;

                if (b_nonDefaultValuesOnly && 
                    16 == g_ndRegTable[i].n_width &&
                    g_ndRegTable[i].n_default == n_value16)
                    continue;


                if (!b_name_printed)
                {
                    printf("%-52s ", g_ndRegTable[i].p_name);
                    b_name_printed = AG_TRUE;
                }
                else
                    printf("%-52s ", "");

                if (g_ndRegTable[i].e_format != AG_ND_REG_FMT_GLOBAL)
                    printf("%4lu ", n_id);
                else
                    printf("     ");

                if (g_ndRegTable[i].n_size > 1)
                    printf("%2lu ", n_entry);
                else
                    printf("   ");

                if (32 == g_ndRegTable[i].n_width)
                    printf("%06lX %08lX\n", n_addr, n_value32);
                else
                    printf("%06lX %04hX\n", n_addr, n_value16);
            }

            n_id = ag_nd_next_reg_id(p_device, &(g_ndRegTable[i]), n_id);
            if (n_max_entity != AG_ND_NO_SPECIFIC_ENTITY && n_id > n_max_entity)
                n_id = -1;
        }
        while (n_id != (AG_U32)-1);
    }


    printf("\n-- registers dump end\n");

    return AG_S_OK;
}

/**********************ORI***********************************/
AgResult ag_nd_all_regs_print(
    AgNdDevice *p_device, 
    AG_BOOL b_writableRegistersOnly,
    AG_BOOL b_nonDefaultValuesOnly,
    AG_U32 n_max_entity,
    AG_U32 n_specific_entity,
    AgNdRegAddressFormat e_format)
{
    AG_U32  i;
    AG_U32  n_id; /* circuit/channel id */
    AG_U32  n_addr;
    AG_U32  n_value32;
    AG_U16  n_value16;
    AG_U32  n_entry;
    AG_BOOL b_name_printed;

	assert(p_device);


    printf("\n-- registers dump start\n");
    


    /* */
    /* iterate over all registers */
    /* */
    for (i = 0; (AG_U32)-1 != g_ndRegTable[i].n_address; i++)
    {
        /*if (e_format != AG_ND_REG_FMT_ALL && */
        /*    g_ndRegTable[i].e_format != e_format) */
        /*{ */
        /*    continue; */
        /*} */
        

        if (!(g_ndRegTable[i].n_chip & p_device->n_chip_mask)) 
            continue;

        strtok(g_ndRegTable[i].p_name, "(");
        b_name_printed = AG_FALSE;


        /*  */
        /* iterate over all circuit/channel ids */
        /* */
        n_id = 0;

        do
        {   
            /* */
            /* iterate over all entries in register array */
            /* */
            if (n_specific_entity != AG_ND_NO_SPECIFIC_ENTITY &&
                n_id != n_specific_entity)
            {
                n_id = ag_nd_next_reg_id(p_device, &(g_ndRegTable[i]), n_id);
                if (n_id == (AG_U32)-1)
                    break;
                continue;
            }
            for (n_entry = 0; n_entry < g_ndRegTable[i].n_size; n_entry++)
            {
                n_addr = g_ndRegTable[i].n_address;                 /* base register address */
                n_addr += n_entry * g_ndRegTable[i].n_width >> 3;   /* entry offset */
                n_addr += n_id << 12;                               /* channel/circuit id offset */


                if (32 == g_ndRegTable[i].n_width)
                    ag_nd_reg_read_32bit(p_device, n_addr, &n_value32);
                else
                    ag_nd_reg_read(p_device, n_addr, &n_value16);

                if (!b_name_printed)
                {
                    printf("%-52s ", g_ndRegTable[i].p_name);
                    b_name_printed = AG_TRUE;
                }
                else
                    printf("%-52s ", "");

                if (g_ndRegTable[i].e_format != AG_ND_REG_FMT_GLOBAL)
                    printf("%4lu ", n_id);
                else
                    printf("     ");

                if (g_ndRegTable[i].n_size > 1)
                    printf("%2lu ", n_entry);
                else
                    printf("   ");

                if (32 == g_ndRegTable[i].n_width)
                    printf("%06lX %08lX\n", n_addr, n_value32);
                else
                    printf("%06lX %04hX\n", n_addr, n_value16);
            }

            n_id = ag_nd_next_reg_id(p_device, &(g_ndRegTable[i]), n_id);
            if (n_max_entity != AG_ND_NO_SPECIFIC_ENTITY && n_id > n_max_entity)
                n_id = -1;
        }
        while (n_id != (AG_U32)-1);
	
	}
	printf("\n-- registers dump end\n");

    return AG_S_OK;
}
#endif
/*******************************************************************************/
/*///////////////////////////////////////////////////////////////////////////// */


/*///////////////////////////////////////////////////////////////////////////// */
/* dump device structure */
/* */
AgResult ag_nd_debug_device_print(AgNdDevice *p_device)
{
    AG_U32      n_channel_id;
    char        *a_path_name[AG_ND_PATH_MAX];
    AgNdPath    e_path;
    AgNdList    *p_entry;
    AgNdTsInfo  *p_ts_info;
    AG_U32      i;


    assert(p_device);


    a_path_name[AG_ND_PATH_EGRESS] = "Egress";
    a_path_name[AG_ND_PATH_INGRESS] = "Ingress";


    printf("\n-- p_device dump start\n");


    for (e_path = AG_ND_PATH_FIRST; e_path <= AG_ND_PATH_LAST; e_path++)
    {
        printf("\n\n");
        printf("%s path\n", a_path_name[e_path]);

        for (n_channel_id = 0; n_channel_id < p_device->n_pw_max; n_channel_id++)
        {
            /* */
            /* do not show channels that don't have enabled timeslots and not enabled */
            /* */
            if (ag_nd_list_is_empty(&(p_device->a_channel[n_channel_id].a_channelizer[e_path].a_ts_list)) &&
                AG_FALSE == p_device->a_channel[n_channel_id].a_enable[e_path])
                continue;

            printf("\n");
            printf("channel           : %lu\n", n_channel_id);
            printf("enable            : %lu\n", p_device->a_channel[n_channel_id].a_enable[e_path]);
            printf("ftip              : %02hhX/%02hhX\n", 
                p_device->a_channel[n_channel_id].a_channelizer[e_path].x_first.n_circuit_id,
                p_device->a_channel[n_channel_id].a_channelizer[e_path].x_first.n_slot_idx);
            
            i = 0;
            AG_ND_LIST_FOREACH(p_entry, &p_device->a_channel[n_channel_id].a_channelizer[e_path].a_ts_list)
            {
                p_ts_info = AG_ND_LIST_ENTRY(p_entry, AgNdTsInfo, x_list);
                printf("%lu %02hhX/%02hhX\n", i, p_ts_info->x_id.n_circuit_id, p_ts_info->x_id.n_slot_idx);
                i++;
            }
            printf("\n");
            printf("ts total : %lu\n", i);
        }

        printf("\ndisabled timeslots:\n");

        i = 0;
        AG_ND_LIST_FOREACH(p_entry, &p_device->a_ts_list[e_path])
        {
            p_ts_info = AG_ND_LIST_ENTRY(p_entry, AgNdTsInfo, x_list);
            printf("%lu %02hhX/%02hhX\n", i, p_ts_info->x_id.n_circuit_id, p_ts_info->x_id.n_slot_idx);
            i++;
        }
        printf("\n");
        printf("disabled ts total : %lu\n", i);
    }


    printf("\n-- p_device dump end\n");

    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* dump mm pool */
/* */

void ag_nd_debug_allocator_print(AgNdAllocator *p_allocator)
{
#ifndef BCM_CES_SDK
    AG_U16  i = 0;
    AgNdAllocatorBlock  *p_block;
#endif
    char    a_block[10];
    char    a_start10[10];
    char    a_start16[10];
    char    a_size10[10];
    char    a_size16[10];
    char    a_allocated[10];
    char    *p_format = "%10s %10s %10s %10s %10s %10s\n";

    AgNdList            *p_list_entry;


    printf(p_format, "block#", "start(10)", "start(16)", "size(10)", "size(16)", "allocated");


    AG_ND_LIST_FOREACH(p_list_entry, &p_allocator->x_block_list)
    {
#ifndef BCM_CES_SDK
        p_block = AG_ND_LIST_ENTRY(p_list_entry, AgNdAllocatorBlock, x_list);
        sprintf(a_block, "%u", i++);
        sprintf(a_start10, "%lu", p_block->n_start);
        sprintf(a_start16, "%lx", p_block->n_start);
        sprintf(a_size10, "%lu", p_block->n_size);
        sprintf(a_size16, "%lx", p_block->n_size);
        sprintf(a_allocated, "%s", p_block->b_free ? "no" : "yes");
#endif
        printf(p_format, a_block, a_start10, a_start16, a_size10, a_size16, a_allocated);
    }
}

void ag_nd_debug_mu_print(AgNdDevice *p_device)
{
    char    a_bank[10];
    char    a_internal[10];
    char    a_start10[10];
    char    a_start16[10];
    char    a_size10[10];
    char    a_size16[10];
    char    *p_format = "%-15s %5s %10s %10s %10s %10s %10s\n";
    unsigned     i;


    printf("\nmemory units:\n");
    printf(p_format, "name", "bank", "internal", "start(10)", "start(16)", "size(10)", "size(16)");

    for (i = 0; i < p_device->n_mem_unit_count; i++)
    {
        sprintf(a_bank, "%lu", p_device->a_mem_unit[i].n_bank);
        sprintf(a_internal, "%s", p_device->a_mem_unit[i].b_internal? "yes" : "no");
        sprintf(a_start10, "%lu", p_device->a_mem_unit[i].n_start);
        sprintf(a_start16, "%lx", p_device->a_mem_unit[i].n_start);
        sprintf(a_size10, "%lu", p_device->a_mem_unit[i].n_size);
        sprintf(a_size16, "%lx", p_device->a_mem_unit[i].n_size);
        printf(p_format, p_device->a_mem_unit[i].a_name, a_bank, a_internal, a_start10, a_start16, a_size10, a_size16);
    }
}

AgResult ag_nd_debug_mm_print(AgNdDevice *p_device)
{
    ag_nd_debug_mu_print(p_device);

    printf("\npbf allocator:\n");
    ag_nd_debug_allocator_print(&p_device->x_allocator_pbf);

    printf("\njbf allocator:\n");
    ag_nd_debug_allocator_print(&p_device->x_allocator_jbf);

	if (!p_device->b_dynamic_memory)
	{
	    printf("\nvba allocator:\n");
	    ag_nd_debug_allocator_print(&p_device->x_allocator_vba);
	}
	
    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* profiling */
/* */
extern AgNdOpcodeTableEntry g_ndOpcodeWrite[];
extern AgNdOpcodeTableEntry g_ndOpcodeRead[];

void ag_nd_debug_profile_data_print_helper(AgNdOpcodeTableEntry *p_entry, AG_U32 n_size)
{
    char    a_count[10];
    char    a_avg[10];
    char    a_min[10];
    char    a_max[10];
    char    *p_format = "%-30s %10s %10s %10s %10s\n";
    int     i;


    printf(p_format, "Name", "Calls", "Avg", "Min", "Max");

    for (i = 0; i < n_size; i++, p_entry++)
    {
        if (!p_entry->p_api)
            continue;

        sprintf(a_count, "%lu", p_entry->n_prof_cnt);

        if (p_entry->n_prof_cnt)
        {
            sprintf(a_avg, "%lu", p_entry->n_prof_avg);
            sprintf(a_min, "%lu", p_entry->n_prof_min);
            sprintf(a_max, "%lu", p_entry->n_prof_max);
        }
        else
        {
            sprintf(a_avg,"%s","");
            sprintf(a_min,"%s", "");
            sprintf(a_max,"%s", "");
        }

        printf(p_format, 
               p_entry->a_name + 13, /* skip the "AG_ND_OPCODE_" part */
               a_count, 
               a_avg, 
               a_min, 
               a_max);
    }
}

AgResult ag_nd_debug_profile_data_print(void)
{
    printf("\nag_nd_device_read profile data (usec):\n");
    ag_nd_debug_profile_data_print_helper(g_ndOpcodeRead, AG_ND_OPCODE_MAX);

    printf("\nag_nd_device_write profile data (usec):\n");
    ag_nd_debug_profile_data_print_helper(g_ndOpcodeWrite, AG_ND_OPCODE_MAX);

    return AG_S_OK;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* lstree print routine */
/*  */
AgResult ag_nd_debug_lstree_print(AgNdDevice *p_device, AG_U32 n_table_idx)
{
    AG_U32 i;
    AgNdRegLstreeEntry x_entry;

    char idx[10];
    char field_OPC[10];
    char field_PT[10];
    char field_NRADDR[10];
    char field_NPT[10];

    char *format="%2s %3s %2s %6s %3s\n";

    printf("\nLSTREE table 0x%lx\n\n", n_table_idx);

    printf(format, "#", "OPC", "PT", "NRADDR", "NPT");

    for (i = 0; i < 0x100; i++)
    {
        ag_nd_mem_read(p_device, p_device->p_mem_lstree, n_table_idx * 0x800 + 4 * i, x_entry.n_reg, 4);
#ifndef BCM_CES_SDK
        sprintf(idx,            "%lx", i);
        sprintf(field_OPC,      "%x", x_entry.x_fields.n_opcode);
        sprintf(field_PT,       "%x", x_entry.x_fields.n_path_tag);
        sprintf(field_NRADDR,   "%x", x_entry.x_fields.n_next_record_address);
        sprintf(field_NPT,      "%x", x_entry.x_fields.n_next_path_tag);
#endif
        printf(format, idx, field_OPC, field_PT, field_NRADDR, field_NPT);
    }

    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ucode print routine */
/*  */
AgResult ag_nd_debug_ucode_print(AgNdDevice *p_device)
{
    AG_U32 i;
    AgNdRegUcodeInstruction x_instr;

    char idx[10];
    char field_S[10];
    char field_CS[10];
    char field_M[10];
    char field_FRC[10];
    char field_BRC[10];
    char field_BRADDR[10];
    char field_SFC[10];
    char field_FLAG[10];
    char field_OXC[10];
    char field_OPCODE[10];
    char field_VALUE[10];
    char field_MASK[10];

    char *format="%2s %1s %2s %1s %3s %3s %6s %3s %4s %3s %6s %5s %4s\n";

    printf("\nucode version %lu.%lu\n\n", nNdUcodeMajorVersionNumber, nNdUcodeMinorVersionNumber);

    printf(format, "#", "S", "CS", "M", "FRC", "BRC", "BRADDR", "SFC", "FLAG", "OXC", "OPCODE", "VALUE", "MASK");

    for (i = 0; i < nNdUcodeSize; i++)
    {
        ag_nd_mem_read(p_device, p_device->p_mem_ucode, 8 * i, x_instr.n_reg, 8);
#ifndef BCM_CES_SDK
        sprintf(idx,            "%lx", i);
        sprintf(field_S,        "%x", x_instr.x_fields.b_stop);
        sprintf(field_CS,       "%x", x_instr.x_fields.n_comparison_source);
        sprintf(field_M,        "%x", x_instr.x_fields.b_use_mask);
        sprintf(field_FRC,      "%x", x_instr.x_fields.n_freeze_condition_code);
        sprintf(field_BRC,      "%x", x_instr.x_fields.b_branch_condition_code);
        sprintf(field_BRADDR,   "%x", x_instr.x_fields.n_branch_address);
        sprintf(field_SFC,      "%x", x_instr.x_fields.n_set_flasg_condition);
        sprintf(field_FLAG,     "%x", x_instr.x_fields.n_flag);
        sprintf(field_OXC,      "%x", x_instr.x_fields.n_opcode_exec_condition_code);
        sprintf(field_OPCODE,   "%x", x_instr.x_fields.n_opcode);
        sprintf(field_VALUE,    "%x", x_instr.x_fields.n_value);
        sprintf(field_MASK,     "%x", x_instr.x_fields.n_mask);
#endif

        printf(format, idx, field_S, field_CS, field_M, field_FRC, field_BRC, field_BRADDR, 
               field_SFC, field_FLAG, field_OXC, field_OPCODE,field_VALUE, field_MASK);
    }

    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/*  */
/*  */
AgResult 
ag_nd_debug_ucode_flags_find(AgNdDevice *p_device, AG_U32 n_delay)
{
    AgNdMsgConfigRpcPolicy  x_rpc_policy;
    AgNdMsgConfigRpcPolicy  x_rpc_policy_saved;
    AgNdRegGlobalControl    x_glb_ctl;
    AgNdRegGlobalControl    x_glb_ctl_saved;
   #ifndef CES16_BCM_VERSION
    AgNdRegHostPacketLinkConfiguration   x_hpl_cfg;
    AgNdRegHostPacketLinkConfiguration   x_hpl_cfg_saved;
   #endif 
    AgResult n_ret;
    AG_U32 i;
    AG_U16 n_counter;


    /* */
    /* lock the device access in order to prevent application from  */
    /* clearing the counters */
    /*  */
/*  n_ret = ag_nd_mutex_lock(&(p_device->x_api_lock), 1000); */
/*  if (AG_FAILED(n_ret)) */
/*      (void)AG_ND_ERR(p_device, n_ret); */


    /* */
    /* save policy settings  */
    /*  */
    n_ret = ag_nd_opcode_read_config_rpc_policy(p_device, &(x_rpc_policy_saved));
    assert(AG_SUCCEEDED(n_ret));
    /* bail if asserts are disabled */
    if (AG_FAILED(n_ret)) {
        return AG_E_FAIL;
    }


    /* */
    /* drop all packets while counting flags */
    /*  */
/*    ag_nd_memset(&x_rpc_policy, 0, sizeof(x_rpc_policy)); */
/*    x_rpc_policy.x_policy_matrix.n_status_polarity = 0xffffffff; */
/*    x_rpc_policy.x_policy_matrix.n_drop_unconditional = 0xffffffff; */

    /* */
    /* disable PME, otherwise it will RCLR the RPC counters */
    /*  */
    #ifndef CES16_BCM_VERSION
    ag_nd_reg_read(p_device, AG_REG_HOST_PACKET_LINK_CONFIGURATION, &(x_hpl_cfg.n_reg));
    x_hpl_cfg_saved = x_hpl_cfg;
    x_hpl_cfg.x_fields.b_pme_export_enable = AG_FALSE;
    ag_nd_reg_write(p_device, AG_REG_HOST_PACKET_LINK_CONFIGURATION, x_hpl_cfg.n_reg);
    #endif /*CES16_BCM_VERSION*/

    /* */
    /* set PM counters "clear on read" to true */
    /*  */
    ag_nd_reg_read(p_device, AG_REG_GLOBAL_CONTROL, &(x_glb_ctl.n_reg));
    x_glb_ctl_saved = x_glb_ctl;
    x_glb_ctl.x_fields.b_rclr_on_host_access = AG_TRUE;
    ag_nd_reg_write(p_device, AG_REG_GLOBAL_CONTROL, x_glb_ctl.n_reg);

    for (i = 0; i < 32; i++)
    {
        x_rpc_policy.x_policy_matrix.n_global_counter_1 = 1 << i;

        n_ret = ag_nd_opcode_write_config_rpc_policy(p_device, &(x_rpc_policy));
        assert(AG_SUCCEEDED(n_ret));
        /* bail if asserts are disabled */
        if (AG_FAILED(n_ret)) {
            return AG_E_FAIL;
        }

        ag_nd_reg_read(p_device, AG_REG_CLASSIFIER_GLOBAL_PM_COUNTER(0), &n_counter);
        ag_nd_sleep_msec(n_delay);
        ag_nd_reg_read(p_device, AG_REG_CLASSIFIER_GLOBAL_PM_COUNTER(0), &n_counter);

        if (n_counter)
            printf("flag %lu: %hu occurences in %lu msec\n", i, n_counter, n_delay);
    }

    /* */
    /* restore policy, PME and PM counters RCLR settings */
    /*  */
   #ifndef CES16_BCM_VERSION
    ag_nd_reg_write(p_device, AG_REG_HOST_PACKET_LINK_CONFIGURATION, x_hpl_cfg_saved.n_reg);
   #endif /*CES16_BCM_VERSION*/
 
    ag_nd_reg_write(p_device, AG_REG_GLOBAL_CONTROL, x_glb_ctl_saved.n_reg);

    n_ret = ag_nd_opcode_write_config_rpc_policy(p_device, &(x_rpc_policy_saved));
    assert(AG_SUCCEEDED(n_ret));
    /* bail if asserts are disabled */
    if (AG_FAILED(n_ret)) {
        return AG_E_FAIL;
    }

    /* */
    /* unlock the device */
    /*  */
/*  ag_nd_mutex_unlock(&(p_device->x_api_lock)); */


    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* prints RPC policy evaluation matrix content */
/*  */

#define FLAG_SET_STR_LEN 5

void 
ag_nd_debug_policy_print_helper(
	AgNdDevice 				*p_device,
    char                    *p_format,
    char                    *p_flag_name,
    AG_U32                  n_flag_mask,
    AgNdMsgConfigRpcPolicy  *p_policy)
{

	if (p_device->b_ptp_support)
	{
/*#ifdef AG_ND_PTP_SUPPORT */
    printf(
        p_format,
        p_flag_name,
        (p_policy->x_policy_matrix.n_status_polarity      & n_flag_mask) ? "X" : ".",
        (p_policy->x_policy_matrix.n_drop_unconditional   & n_flag_mask) ? "X" : ".",
        (p_policy->x_policy_matrix.n_forward_to_ptp       & n_flag_mask) ? "X" : ".",
        (p_policy->x_policy_matrix.n_forward_to_host_high & n_flag_mask) ? "X" : ".",
        (p_policy->x_policy_matrix.n_forward_to_host_low  & n_flag_mask) ? "X" : ".",
        (p_policy->x_policy_matrix.n_drop_if_not_forward  & n_flag_mask) ? "X" : ".",
        (p_policy->x_policy_matrix.n_global_counter_1     & n_flag_mask) ? "X" : ".",
        (p_policy->x_policy_matrix.n_global_counter_2     & n_flag_mask) ? "X" : ".",
        (p_policy->x_policy_matrix.n_global_counter_3     & n_flag_mask) ? "X" : ".",
        (p_policy->x_policy_matrix.n_global_counter_4     & n_flag_mask) ? "X" : ".",
        (p_policy->x_policy_matrix.n_global_counter_5     & n_flag_mask) ? "X" : ".",
        (p_policy->x_policy_matrix.n_global_counter_6     & n_flag_mask) ? "X" : ".",
        (p_policy->x_policy_matrix.n_global_counter_7     & n_flag_mask) ? "X" : ".",
        (p_policy->x_policy_matrix.n_global_counter_8     & n_flag_mask) ? "X" : ".",
        (p_policy->x_policy_matrix.n_channel_counter_1    & n_flag_mask) ? "X" : ".",
        (p_policy->x_policy_matrix.n_channel_counter_2    & n_flag_mask) ? "X" : ".",
        (p_policy->x_policy_matrix.n_channel_counter_3    & n_flag_mask) ? "X" : ".",
        (p_policy->x_policy_matrix.n_channel_counter_4    & n_flag_mask) ? "X" : ".");
	}
	else
	{
	  printf(
	    p_format,
	    p_flag_name,
	    (p_policy->x_policy_matrix.n_status_polarity      & n_flag_mask) ? "X" : ".",
	    (p_policy->x_policy_matrix.n_drop_unconditional   & n_flag_mask) ? "X" : ".",
	    (p_policy->x_policy_matrix.n_forward_to_host_high & n_flag_mask) ? "X" : ".",
	    (p_policy->x_policy_matrix.n_forward_to_host_low  & n_flag_mask) ? "X" : ".",
	    (p_policy->x_policy_matrix.n_drop_if_not_forward  & n_flag_mask) ? "X" : ".",
	    (p_policy->x_policy_matrix.n_global_counter_1     & n_flag_mask) ? "X" : ".",
	    (p_policy->x_policy_matrix.n_global_counter_2     & n_flag_mask) ? "X" : ".",
	    (p_policy->x_policy_matrix.n_global_counter_3     & n_flag_mask) ? "X" : ".",
	    (p_policy->x_policy_matrix.n_global_counter_4     & n_flag_mask) ? "X" : ".",
	    (p_policy->x_policy_matrix.n_global_counter_5     & n_flag_mask) ? "X" : ".",
	    (p_policy->x_policy_matrix.n_global_counter_6     & n_flag_mask) ? "X" : ".",
	    (p_policy->x_policy_matrix.n_global_counter_7     & n_flag_mask) ? "X" : ".",
	    (p_policy->x_policy_matrix.n_global_counter_8     & n_flag_mask) ? "X" : ".",
	    (p_policy->x_policy_matrix.n_channel_counter_1    & n_flag_mask) ? "X" : ".",
	    (p_policy->x_policy_matrix.n_channel_counter_2    & n_flag_mask) ? "X" : ".",
	    (p_policy->x_policy_matrix.n_channel_counter_3    & n_flag_mask) ? "X" : ".",
	    (p_policy->x_policy_matrix.n_channel_counter_4    & n_flag_mask) ? "X" : ".");
	}
}


AgResult 
ag_nd_debug_policy_print(AgNdDevice *p_device)
{
	char                    *p_format;
	AgNdMsgConfigRpcPolicy	x_rpc_policy;
	AgResult				n_ret;

/*#ifdef AG_ND_PTP_SUPPORT */
	if (p_device->b_ptp_support)
	    p_format="%-25s %2s %2s %2s %2s %2s %2s %2s %2s %2s %2s %2s %2s %2s %2s %2s %2s %2s %2s\n";
	else
	    p_format="%-25s %2s %2s %2s %2s %2s %2s %2s %2s %2s %2s %2s %2s %2s %2s %2s %2s %2s\n";


    /* */
    /* read policy */
    /*  */
    n_ret = ag_nd_device_read(0, AG_ND_OPCODE_CONFIG_RPC_POLICY, &(x_rpc_policy));
    if (AG_FAILED(n_ret))
        return n_ret;

    /* */
    /* print header */
    /*  */
/*/#ifdef AG_ND_PTP_SUPPORT */
	if (p_device->b_ptp_support)
	    printf(
	        p_format, 
	        "   FLAG",
	        "PL",
	        "DR",
	        "PT", /* PTP */
	        "HP",
	        "LP",
	        "FW",
	        "G1",
	        "G2",
	        "G3",
	        "G4",
	        "G5",
	        "G6",
	        "G7",
	        "G8",
	        "C1",
	        "C2",
	        "C3",
	        "C4");
	else               
	    printf(
	      p_format, 
	      "   FLAG",
	      "PL",
	      "DR",
	      "HP",
	      "LP",
	      "FW",
	      "G1",
	      "G2",
	      "G3",
	      "G4",
	      "G5",
	      "G6",
	      "G7",
	      "G8",
	      "C1",
	      "C2",
	      "C3",
	      "C4");

    ag_nd_debug_policy_print_helper(p_device,p_format, " 0 Found MPLS           ", AG_ND_RPC_FLAG_FOUND_MPLS              , &x_rpc_policy);
    ag_nd_debug_policy_print_helper(p_device,p_format, " 1 Found IP             ", AG_ND_RPC_FLAG_FOUND_IP                , &x_rpc_policy);
    ag_nd_debug_policy_print_helper(p_device,p_format, " 2 Found VLAN           ", AG_ND_RPC_FLAG_FOUND_VLAN              , &x_rpc_policy);
    ag_nd_debug_policy_print_helper(p_device,p_format, " 3 Found ECID           ", AG_ND_RPC_FLAG_FOUND_ECID              , &x_rpc_policy);
    ag_nd_debug_policy_print_helper(p_device,p_format, " 4 Found UDP            ", AG_ND_RPC_FLAG_FOUND_UDP               , &x_rpc_policy);
    ag_nd_debug_policy_print_helper(p_device,p_format, " 5 Eth not unicast pkt  ", AG_ND_RPC_FLAG_ETH_NOT_UNICAST_PACKET  , &x_rpc_policy);
    ag_nd_debug_policy_print_helper(p_device,p_format, " 6 Eth destaddr mismatch", AG_ND_RPC_FLAG_ETH_DESTADDR_MISMATCH   , &x_rpc_policy);
    ag_nd_debug_policy_print_helper(p_device,p_format, " 7 Unknown ethertype    ", AG_ND_RPC_FLAG_UNKNOWN_ETHER_TYPE      , &x_rpc_policy);
    ag_nd_debug_policy_print_helper(p_device,p_format, " 8 IP not unicast pkt   ", AG_ND_RPC_FLAG_IP_NOT_UNICAST_PACKET   , &x_rpc_policy);
    ag_nd_debug_policy_print_helper(p_device,p_format, " 9 Bad IP header        ", AG_ND_RPC_FLAG_BAD_IP_HEADER           , &x_rpc_policy);
    ag_nd_debug_policy_print_helper(p_device,p_format, "10 IP wrong protocol    ", AG_ND_RPC_FLAG_IP_WRONG_PROTOCOL       , &x_rpc_policy);
    ag_nd_debug_policy_print_helper(p_device,p_format, "11 IP address err       ", AG_ND_RPC_FLAG_IP_ADDRESS_ERROR        , &x_rpc_policy);
    ag_nd_debug_policy_print_helper(p_device,p_format, "12 MPLS TTL zero        ", AG_ND_RPC_FLAG_MPLS_TTL_ZERO           , &x_rpc_policy);
    ag_nd_debug_policy_print_helper(p_device,p_format, "13 Host designated pkt  ", AG_ND_RPC_FLAG_HOST_DESIGNATED_PACKET  , &x_rpc_policy);
    ag_nd_debug_policy_print_helper(p_device,p_format, "14 RTP flag err         ", AG_ND_RPC_FLAG_RTP_FLAG_ERROR          , &x_rpc_policy);
    ag_nd_debug_policy_print_helper(p_device,p_format, "15 Found PTP            ", AG_ND_RPC_FLAG_FOUND_PTP               , &x_rpc_policy);
    ag_nd_debug_policy_print_helper(p_device,p_format, "16 RAI M bits are 10    ", AG_ND_RPC_FLAG_RAI_M_BITS_ARE_10       , &x_rpc_policy);
    ag_nd_debug_policy_print_helper(p_device,p_format, "17 Disabled CES channel ", AG_ND_RPC_FLAG_DISABLED_CES_CHANNEL    , &x_rpc_policy);
    ag_nd_debug_policy_print_helper(p_device,p_format, "18 UDP forward LP queue ", AG_ND_RPC_FLAG_UDP_FORWARD_LP_QUEUE    , &x_rpc_policy);
    ag_nd_debug_policy_print_helper(p_device,p_format, "19 UDP forward HP queue ", AG_ND_RPC_FLAG_UDP_FORWARD_HP_QUEUE    , &x_rpc_policy);
    ag_nd_debug_policy_print_helper(p_device,p_format, "20 UDP length err       ", AG_ND_RPC_FLAG_UDP_LENGTH_ERROR        , &x_rpc_policy);
    ag_nd_debug_policy_print_helper(p_device,p_format, "21 IP len err           ", AG_ND_RPC_FLAG_IP_LEN_ERROR            , &x_rpc_policy);
    ag_nd_debug_policy_print_helper(p_device,p_format, "22 UDP chksum err       ", AG_ND_RPC_FLAG_UDP_CHECKSUM_ERROR      , &x_rpc_policy);
    ag_nd_debug_policy_print_helper(p_device,p_format, "23 CRC err              ", AG_ND_RPC_FLAG_CRC_ERROR               , &x_rpc_policy);
    ag_nd_debug_policy_print_helper(p_device,p_format, "24 Strict check results ", AG_ND_RPC_FLAG_STRICT_CHECK_RESULTS    , &x_rpc_policy);
    ag_nd_debug_policy_print_helper(p_device,p_format, "25 RAI R bit is 1       ", AG_ND_RPC_FLAG_RAI_R_BIT_IS_1          , &x_rpc_policy);
    ag_nd_debug_policy_print_helper(p_device,p_format, "26 AIS dba packet       ", AG_ND_RPC_FLAG_AIS_DBA_PACKET          , &x_rpc_policy);
    ag_nd_debug_policy_print_helper(p_device,p_format, "27 Signaling pkt        ", AG_ND_RPC_FLAG_SIGNALING_PACKET        , &x_rpc_policy);
    ag_nd_debug_policy_print_helper(p_device,p_format, "28 CES CW err           ", AG_ND_RPC_FLAG_CES_CW_ERROR            , &x_rpc_policy);
    ag_nd_debug_policy_print_helper(p_device,p_format, "29 Unknown PW label     ", AG_ND_RPC_FLAG_UNKNOWN_PW_LABEL        , &x_rpc_policy);
    ag_nd_debug_policy_print_helper(p_device,p_format, "30 IP header chksum err ", AG_ND_RPC_FLAG_IP_HEADER_CHECKSUM_ERROR, &x_rpc_policy); 
    ag_nd_debug_policy_print_helper(p_device,p_format, "31 Unexpected end of pkt", AG_ND_RPC_FLAG_UNEXPECTED_END_OF_PACKET, &x_rpc_policy);

    return AG_S_OK;
}

AgResult 
ag_nd_debug_buf_print(
    AgNdDevice          *p_device, 
    AgNdDebugBufType    e_buf,
    AG_U32              n_addr,
    AG_U32              n_size)
{
    AgResult            n_ret;
    AG_U32              i;
#ifndef BCM_CES_SDK
    AG_U16              *p_mem;
    AG_U32 j;
#endif
    char                *p_format = "%10s    %4s    %4s    %4s    %4s    %4s    %4s    %4s    %4s\n";
    char                a_str_address[20];
    char                a_str_value[8][20];

    /* */
    /* round up size to multiple of 16 */
    /* round down address to multiple of 16 */
    /*  */
#ifndef BCM_CES_SDK
    p_mem = (AG_U16*)((n_addr >> 4) << 4);
#endif
    n_size = ag_nd_round_up2(n_size, 4);


    /* */
    /* lock API access (because of ext addr register) */
    /*  */
    n_ret = ag_nd_mutex_lock(&(p_device->x_api_lock), 10000);
    if (AG_FAILED(n_ret))
        (void)AG_ND_ERR(p_device, n_ret);


    /* */
    /* set r/w access to the right bank */
    /*  */
   #ifndef CES16_BCM_VERSION
    if (AG_ND_DEBUG_BUF_PBF == e_buf)
        ag_nd_mem_unit_set(p_device, p_device->p_mem_pbf);
    else
        ag_nd_mem_unit_set(p_device, p_device->p_mem_jbf);
   #else
    if (AG_ND_DEBUG_BUF_PBF != e_buf)
      return AG_S_OK;
    ag_nd_mem_unit_set(p_device, p_device->p_mem_pbf);
   #endif   

    printf("\n");
    printf(p_format, "", "0", "2",  "4", "6", "8", "A", "C", "E");

    for (i = 0; i < n_size/16; i++)
    {
#ifndef BCM_CES_SDK
        sprintf(a_str_address, "0x%08lX", (AG_U32)p_mem);

        for (j = 0; j < 8; j++)
            sprintf(a_str_value[j], "%04X", *p_mem++);
#endif
        printf(
            p_format, 
            a_str_address,
            a_str_value[0],
            a_str_value[1],
            a_str_value[2],
            a_str_value[3],
            a_str_value[4],
            a_str_value[5],
            a_str_value[6],
            a_str_value[7]);
    }

    printf("\n");


    n_ret = ag_nd_mutex_unlock(&(p_device->x_api_lock));
    if (AG_FAILED(n_ret))
        return AG_ND_ERR(p_device, n_ret);


    return AG_S_OK;
}



