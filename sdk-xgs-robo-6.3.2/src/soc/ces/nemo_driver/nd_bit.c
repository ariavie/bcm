/* $Id: nd_bit.c 1.8 Broadcom SDK $
 * Copyright 2011 BATM
 */

#ifndef BCM_CES_SDK
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#endif

#include "nd_platform.h"
#include "pub/nd_hw.h"
#include "nd_bit.h"
#include "pub/nd_api.h"
#include "nd_debug.h"


/*///////////////////////////////////////////////////////////////////////////// */
/* ndBitRegs */
/* */
/* performs BIT */
/*  */
/*                                  */
AgResult
ag_nd_opcode_read_bit(AgNdDevice *p_device, AgNdMsgBit *p_msg)
{
    AG_U32 i;
    AgNdRegGlobalControl x_global;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "msg_max=%lu\n", p_msg->n_msg_max);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "verbose=%lu\n", p_msg->n_verbose);


    if (!p_msg)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if (p_msg->n_verbose != AG_ND_BIT_VERBOSE_LEVEL_QUIET    &&
        p_msg->n_verbose != AG_ND_BIT_VERBOSE_LEVEL_BRIEF    &&
        p_msg->n_verbose != AG_ND_BIT_VERBOSE_LEVEL_DETAILED &&
        p_msg->n_verbose != AG_ND_BIT_VERBOSE_LEVEL_INSANE)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);


    p_device->x_bit.n_verbose = p_msg->n_verbose;
    p_device->x_bit.b_passed = AG_TRUE;
    p_device->x_bit.n_msg_max = p_msg->n_msg_max;
    p_device->x_bit.p_print = p_msg->p_print;

    ag_nd_bit_print(p_device, AG_ND_BIT_VERBOSE_LEVEL_QUIET, "\nBIT started");


    /* */
    /* unlock ucode lock bit */
    /*  */
    ag_nd_reg_read(p_device, AG_REG_GLOBAL_CONTROL, &(x_global.n_reg));
    x_global.x_fields.b_ucode_memory_locked = AG_FALSE;
    ag_nd_reg_write(p_device, AG_REG_GLOBAL_CONTROL, x_global.n_reg);

    for (i = 0; i < p_device->n_mem_unit_count; i++) 
        ag_nd_bit_mem(p_device, &(p_device->a_mem_unit[i]));

    ag_nd_bit_all_regs(p_device);


    ag_nd_bit_print(p_device, AG_ND_BIT_VERBOSE_LEVEL_QUIET, "BIT %s", 
        p_device->x_bit.b_passed ? "passed" : "failed");

    p_msg->b_passed = p_device->x_bit.b_passed;

    /* */
    /* all the initialization setting was destroyed ! */
    /* close the device !! */
    /* */
    ag_nd_device_close(p_device->n_handle);

    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ndBitRegs */
/* */
/* formatted output into user supplied callback function */
/*  */
/*                                  */
void 
ag_nd_bit_print(AgNdDevice *p_device, AG_U32 n_verbose, const char *p_format, ...)
{
#ifndef BCM_CES_SDK
    va_list n_argp;
#endif

    assert(p_device);


    if (n_verbose > p_device->x_bit.n_verbose)
        return;

#ifndef BCM_CES_SDK
    va_start(n_argp, p_format);

    vsnprintf(
        p_device->x_bit.a_buf, 
        sizeof(p_device->x_bit.a_buf), 
        p_format, 
        n_argp);

    p_device->x_bit.a_buf[sizeof(p_device->x_bit.a_buf) - 1] = 0;

    if (p_device->x_bit.p_print)
        p_device->x_bit.p_print(p_device->x_bit.a_buf);
#endif
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_bit_one_reg */
/* */
/* Tests one register. In the case of per channel/per circuit register */
/* tests the all the channels/circuits */
/* */
/* */
void
ag_nd_bit_one_reg(AgNdDevice *p_device, AgNdRegProperties *p_reg)
{
    AG_U32 n_id; /* channel/circuit id */
    AG_U32 n_test_count;

    assert(p_device);
    assert(p_reg);


    n_id = 0;
    n_test_count = 0;

    do
    {
        if (n_test_count++ >= p_device->x_bit.n_msg_max)
            break;

        if (p_reg->test) 
            p_reg->test(p_device, p_reg, n_id << 12, AG_TRUE);

        n_id = ag_nd_next_reg_id(p_device, p_reg, n_id);

    }
    while (n_id != (AG_U32)-1);
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_bit_all_regs */
/* */
/* Loops over BIT registers table and performs the dedicated tests. */
/* R/W registers are tested first. */
/* */
/* */
void
ag_nd_bit_all_regs(AgNdDevice *p_device)
{
    AG_U32 i = 0;


    assert(p_device);

    /* */
    /* test rw regs */
    /*  */
    ag_nd_bit_print(p_device, AG_ND_BIT_VERBOSE_LEVEL_BRIEF, "\ntesting registers");
    ag_nd_bit_print(p_device, AG_ND_BIT_VERBOSE_LEVEL_DETAILED, "\ntesting r/w registers");


    for (i = 0; g_ndRegTable[i].n_address != (AG_U32)-1 ; i++)

        if (AG_ND_REG_TYPE_RW == g_ndRegTable[i].e_type && 
            (g_ndRegTable[i].n_chip & p_device->n_chip_mask))
        {
            /* */
            /* test register */
            /* */
            ag_nd_bit_one_reg(p_device, &(g_ndRegTable[i]));
        }                           



    /* */
    /* test other regs */
    /* */
    ag_nd_bit_print(p_device, AG_ND_BIT_VERBOSE_LEVEL_DETAILED, "\ntesting other registers");

    for (i = 0; g_ndRegTable[i].n_address != (AG_U32)-1; i++)

        if (AG_ND_REG_TYPE_RW != g_ndRegTable[i].e_type && 
            (g_ndRegTable[i].n_chip & p_device->n_chip_mask))
        {
            /* */
            /* test register */
            /* */
            ag_nd_bit_one_reg(p_device, &(g_ndRegTable[i]));
        }


    ag_nd_bit_print(p_device, AG_ND_BIT_VERBOSE_LEVEL_BRIEF, "\nregisters test done");
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_bit_rw_reg16_one_pass */
/* */
/* Performs one pass of RW test on array of 16 bit registers.  */
/* The test is as follows: */
/*  1. set all the registers to requested value  */
/*  2. verify */
/* */
void
ag_nd_bit_rw_reg16_one_pass(
    AgNdDevice               *p_device, 
    AgNdRegProperties        *reg, 
    AG_U16                   n_wdata,   /* test data */
    AG_U32                   n_offset)  /* offset from the register base address e.g. channel/circuit offset */
{
    AG_U32 i = 0;
    AG_U16 n_rdata;
    AG_U32 n_addr;
 

    assert(p_device);



    /* */
    /* write the requested value */
    /* */
    n_wdata = n_wdata & (AG_U16)reg->n_mask;

    for (i = 0; i < reg->n_size * 2; i += 2)
    {
        n_addr = reg->n_address + i + n_offset;
        ag_nd_reg_write(p_device, n_addr, n_wdata);
    }   


    /* */
    /* verify read == written */
    /* */
    for (i = 0; i < reg->n_size * 2; i += 2)
    {
        n_addr = reg->n_address + i + n_offset;
        ag_nd_reg_read(p_device, n_addr, &n_rdata);
        n_rdata &= (AG_U16)reg->n_mask;

        ag_nd_bit_rw_word16_report(
            p_device, 
            n_wdata, 
            n_rdata, 
            n_addr, 
            reg->p_name, 
            "%-60s",
            n_rdata == n_wdata);
    }
}


/*///////////////////////////////////////////////////////////////////////////// */
/*  ag_nd_bit_rw_reg16 */
/* */
/* RW register test */
/* */
void
ag_nd_bit_rw_reg16(
    AgNdDevice*         p_device, 
    AgNdRegProperties   *reg, 
    AG_U32              n_offset,
    AG_BOOL             b_test)
{
    assert(p_device);
    assert(reg);

    /* */
    /* test register */
    /* */
    if (b_test)
    {
        ag_nd_bit_rw_reg16_one_pass(p_device, reg, 0x0000, n_offset);
        ag_nd_bit_rw_reg16_one_pass(p_device, reg, 0xffff, n_offset);
        ag_nd_bit_rw_reg16_one_pass(p_device, reg, 0x5555, n_offset);
        ag_nd_bit_rw_reg16_one_pass(p_device, reg, 0xaaaa, n_offset);
    }

    /* */
    /* set register to default value */
    /* */
    ag_nd_bit_rw_reg16_one_pass(p_device, reg, (AG_U16)reg->n_reset, n_offset);
}
    

/*///////////////////////////////////////////////////////////////////////////// */
/*  ag_nd_bit_ro_reg16 */
/* */
/* RW register test */
/* */
void
ag_nd_bit_ro_reg16(
    AgNdDevice*         p_device, 
    AgNdRegProperties  *p_reg, 
    AG_U32              n_offset, 
    AG_BOOL             b_test)
{
    AG_U32 i;
    AG_U16 n_rdata;
    AG_U32 n_addr;
    AG_BOOL b_passed = AG_TRUE;
    AG_U32 n_verbose;

    assert(p_device);
    assert(p_reg);


    return;


    if (b_test)
    {
        for (i = 0; i < p_reg->n_size * 2; i += 2)
        {
            n_addr = p_reg->n_address + i + n_offset;
            ag_nd_reg_read(p_device, n_addr, &n_rdata);
    
            if (b_passed)
                n_verbose = AG_ND_BIT_VERBOSE_LEVEL_INSANE;
            else
            {
                n_verbose = AG_ND_BIT_VERBOSE_LEVEL_QUIET;
                p_device->x_bit.b_passed = AG_FALSE;
            }
    
            
            ag_nd_bit_print(p_device, n_verbose, "%-60s 0x%06lX %s: read 0x%04hX", 
                            p_reg->p_name,
                            n_addr,
                            b_passed ? "passed" : "failed",
                            n_rdata);
        }
    }
}


/*///////////////////////////////////////////////////////////////////////////// */
/*  ag_nd_bit_rw1clr_reg16 */
/* */
/* RW1CLR register test */
/* */
/* works only for 1-word RW1CLR registers.  */
/*  */
void
ag_nd_bit_rw1clr_reg16(
    AgNdDevice*         p_device, 
    AgNdRegProperties   *p_reg, 
    AG_U32              n_offset,
    AG_BOOL             b_test)
{
    AG_U16 n_rdata1;
    AG_U16 n_rdata2;
    AG_U16 n_wdata = 0;
    AG_U32 n_addr;
    AG_BOOL b_passed;
    AG_U32 n_verbose;


    assert(p_device);
    assert(p_reg);


    n_addr = p_reg->n_address + n_offset;

    if (b_test)
    {
        ag_nd_reg_read(p_device, n_addr, &n_rdata1);
        n_wdata = n_rdata1;
    }

    ag_nd_reg_write(p_device, n_addr, (AG_U16)p_reg->n_reset);

    if (b_test)
    {
        ag_nd_reg_read(p_device, n_addr, &n_rdata2);
        b_passed = 0 == n_rdata2;
        
        if (b_passed)
            n_verbose = AG_ND_BIT_VERBOSE_LEVEL_INSANE;
        else
        {
            n_verbose = AG_ND_BIT_VERBOSE_LEVEL_QUIET;
            p_device->x_bit.b_passed = AG_FALSE;

            if ((AG_U32)-1 == p_device->x_bit.n_fail_addr)
                p_device->x_bit.n_fail_addr = n_addr;
        }


        if (n_verbose > p_device->x_bit.n_verbose || !p_device->x_bit.p_print)
            return;


        ag_nd_bit_print(p_device, n_verbose, "%-60s 0x%06lX %s: read 0x%04hX written 0x%04hX read 0x%04hX", 
                   p_reg->p_name,
                   n_addr,
                   b_passed ? "passed" : "failed",
                   n_rdata1,
                   n_wdata,
                   n_rdata2);
    }
}


/*///////////////////////////////////////////////////////////////////////////// */
/*  ag_nd_bit_rclr_reg16 */
/* */
/* RCLR register test */
/* */
void
ag_nd_bit_rclr_reg16(
    AgNdDevice*         p_device, 
    AgNdRegProperties*  p_reg, 
    AG_U32              n_offset,
    AG_BOOL             b_test)
{
    AG_U32 i;
    AG_U16 n_rdata1;
    AG_U16 n_rdata2;
    AG_BOOL b_passed = AG_TRUE;
    AG_U32 n_addr;
    AG_U32 n_verbose;
    AgNdRegGlobalControl x_glob;


    assert(p_device);
    assert(p_reg);


    ag_nd_reg_read(p_device, AG_REG_GLOBAL_CONTROL, &x_glob.n_reg);
    x_glob.x_fields.b_rclr_on_host_access = AG_TRUE;
    ag_nd_reg_write(p_device, AG_REG_GLOBAL_CONTROL, x_glob.n_reg);

    for (i = 0; i < p_reg->n_size * 2; i += 2)
    {
            n_addr = p_reg->n_address + i + n_offset;
            ag_nd_reg_read(p_device, n_addr, &n_rdata1);

            if (b_test)
            {
                ag_nd_reg_read(p_device, n_addr, &n_rdata2);
                b_passed = p_reg->n_default == n_rdata2;
        
                if (b_passed)
                    n_verbose = AG_ND_BIT_VERBOSE_LEVEL_INSANE;
                else
                {
                    n_verbose = AG_ND_BIT_VERBOSE_LEVEL_QUIET;
                    p_device->x_bit.b_passed = AG_FALSE;

                    if ((AG_U32)-1 == p_device->x_bit.n_fail_addr)
                        p_device->x_bit.n_fail_addr = n_addr;
                }
        
                ag_nd_bit_print(p_device, n_verbose, "%-60s 0x%06lX %s: read 0x%04hX read 0x%04hX", 
                           p_reg->p_name,
                           n_addr,
                           b_passed ? "passed" : "failed",
                           n_rdata1,
                           n_rdata2);
            }
    }

    x_glob.x_fields.b_rclr_on_host_access = AG_FALSE;
    ag_nd_reg_write(p_device, AG_REG_GLOBAL_CONTROL, x_glob.n_reg);
}


/*///////////////////////////////////////////////////////////////////////////// */
/*  ag_nd_bit_mem */
/* */
/* memory unit test */
/* */
/* The test consists of two parts: */
/*  1. Works on pairs of words as follows:  */
/*      write aaaa to the first word, 5555 to the second, read them back and verify. */
/*      write 5555 to the first word, aaaa to the second, read them back and verify. */
/*      move to the next pair of words */
/*  2. Fill all the memory with running counter (1,2,3...) and verify */
/* */
void
ag_nd_bit_mem(AgNdDevice *p_device, AgNdMemUnit *p_mu)
{
    AG_U32  n_addr;
    AG_U16  a_wdata[2];
    AG_U16  a_rdata[2];
    AG_U32  n_counter;


    AG_U32  n_start = ag_nd_time_usec();

    assert(p_device);
    assert(p_mu);


    ag_nd_bit_print(
        p_device, 
        AG_ND_BIT_VERBOSE_LEVEL_BRIEF, 
        "\ntesting memory unit: %s: internal=%s bank=%lu start=0x%lX", 
        p_mu->a_name,
        p_mu->b_internal ? "yes" : "no",
        p_mu->n_bank,
        p_mu->n_start);


    n_counter = 0;


    for (n_addr = 0; n_addr < p_mu->n_size; n_addr += 4)
    {
        if (n_counter >= p_device->x_bit.n_msg_max)
            break;


        /* */
        /* write aaaa to the first word, 5555 to the second + verify. */
        /* */
        a_wdata[0] = 0xaaaa;
        a_wdata[1] = 0x5555;
        ag_nd_mem_write(p_device, p_mu, n_addr, a_wdata, 2);
        ag_nd_mem_read(p_device, p_mu, n_addr, a_rdata, 2);
        ag_nd_bit_rw_word16_report(p_device, a_wdata[0], a_rdata[0], n_addr, p_mu->a_name, "%s", a_wdata[0] == a_rdata[0]);
        ag_nd_bit_rw_word16_report(p_device, a_wdata[1], a_rdata[1], n_addr+2, p_mu->a_name, "%s", a_wdata[1] == a_rdata[1]);

        /* */
        /* write 5555 to the first word, aaaa to the second + verify. */
        /* */
        a_wdata[0] = 0x5555;
        a_wdata[1] = 0xaaaa;
        ag_nd_mem_write(p_device, p_mu, n_addr, a_wdata, 2);
        ag_nd_mem_read(p_device, p_mu, n_addr, a_rdata, 2);
        ag_nd_bit_rw_word16_report(p_device, a_wdata[0], a_rdata[0], n_addr, p_mu->a_name, "%s", a_wdata[0] == a_rdata[0]);
        ag_nd_bit_rw_word16_report(p_device, a_wdata[1], a_rdata[1], n_addr+2, p_mu->a_name, "%s", a_wdata[1] == a_rdata[1]);

        /* */
        /* write running counter value */
        /* */
        n_counter++;
        a_wdata[0] = AG_ND_HIGH16(n_counter);
        a_wdata[1] = AG_ND_LOW16(n_counter);
        ag_nd_mem_write(p_device, p_mu, n_addr, &a_wdata, 2);
    }



    /* */
    /* verify running counter values and fill the external memory with default values */
    /* */
    n_counter = 0;

    for (n_addr = 0; n_addr < p_mu->n_size; n_addr += 4)
    {
        if (n_counter >= p_device->x_bit.n_msg_max)
            break;

        n_counter++;

        ag_nd_mem_read(p_device, p_mu, n_addr, a_rdata, 2);

        ag_nd_bit_rw_word16_report(
            p_device, 
            AG_ND_HIGH16(n_counter), 
            a_rdata[0], 
            n_addr, 
            p_mu->a_name, 
            "%s", 
            AG_ND_HIGH16(n_counter) == a_rdata[0]);

        ag_nd_bit_rw_word16_report(
            p_device, 
            AG_ND_LOW16(n_counter), 
            a_rdata[1], 
            n_addr + 2, 
            p_mu->a_name, 
            "%s", 
            AG_ND_LOW16(n_counter) == a_rdata[1]);
    }

    ag_nd_bit_print(
        p_device, 
        AG_ND_BIT_VERBOSE_LEVEL_BRIEF, 
        "memory unit %s: test done (%lu msec)", 
        p_mu->a_name, 
        (ag_nd_time_usec() - n_start)/1000);
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_bit_rw_word16_report */
/* */
void 
ag_nd_bit_rw_word16_report(
    AgNdDevice              *p_device,
    AG_U16                  n_written_data,
    AG_U16                  n_read_data,
    AG_U32                  n_addr,
    AG_CHAR                 *p_name,
    AG_CHAR                 *p_name_format,
    AG_BOOL                 b_passed)
{
    AG_U32 n_verbose;
    AG_CHAR a_format[80];
    

    if (b_passed)
        n_verbose = AG_ND_BIT_VERBOSE_LEVEL_INSANE;
    else
    {
        n_verbose = AG_ND_BIT_VERBOSE_LEVEL_QUIET;
        p_device->x_bit.b_passed = AG_FALSE;

        if ((AG_U32)-1 == p_device->x_bit.n_fail_addr)
            p_device->x_bit.n_fail_addr = n_addr;
    }


    if (n_verbose > p_device->x_bit.n_verbose || !p_device->x_bit.p_print)
        return;


    strncpy(a_format, p_name_format, sizeof(a_format)-1);
    strncat(a_format, " 0x%06lX %s: written 0x%04hX read 0x%04hX", sizeof(a_format)-1);
    a_format[sizeof(a_format)-1] = 0;

    ag_nd_bit_print(
        p_device, 
        n_verbose, 
        a_format,
        p_name,
        n_addr,
        b_passed ? "passed" : "failed",
        n_written_data,
        n_read_data);
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_bit_rw_word32_report */
/* */
void 
ag_nd_bit_rw_word32_report(
    AgNdDevice              *p_device,
    AG_U32                  n_written_data,
    AG_U32                  n_read_data,
    AG_U32                  n_addr,
    AG_CHAR                 *p_name,
    AG_CHAR                 *p_name_format,
    AG_BOOL                 b_passed)
{
    AG_U32 n_verbose;
    AG_CHAR a_format[80];
    

    if (b_passed)
        n_verbose = AG_ND_BIT_VERBOSE_LEVEL_INSANE;
    else
    {
        n_verbose = AG_ND_BIT_VERBOSE_LEVEL_QUIET;
        p_device->x_bit.b_passed = AG_FALSE;

        if ((AG_U32)-1 == p_device->x_bit.n_fail_addr)
            p_device->x_bit.n_fail_addr = n_addr;
    }


    if (n_verbose > p_device->x_bit.n_verbose || !p_device->x_bit.p_print)
        return;


    strncpy(a_format, p_name_format, sizeof(a_format)-1);
    strncat(a_format, " 0x%06lX %s: written 0x%08lX read 0x%08lX", sizeof(a_format)-1);
    a_format[sizeof(a_format)-1] = 0;

    ag_nd_bit_print(
        p_device, 
        n_verbose, 
        a_format,
        p_name,
        n_addr,
        b_passed ? "passed" : "failed",
        n_written_data,
        n_read_data);
}


