/* $Id: 3303e702cb8f15c88b6644483191c894a4da07c9 $
 * Copyright 2011 BATM
 */

#include "pub/nd_api.h"
#include "nd_hw_test.h"
#include "nd_debug.h"
#include "nd_bit.h"
#include "nd_util.h"
#include "nd_platform.h"


void ag_nd_hw_test_mem(AgNdDevice *p_device, AgNdMsgHwTest *p_msg, AG_U32 n_bank, AG_U32 n_bank_size, AG_BOOL b_bank_internal);
void ag_nd_hw_test_mem_address_fast(AgNdDevice *device, AgNdMsgHwTest *p_msg, AG_U32 n_size);
void ag_nd_hw_test_mem_address_full(AgNdDevice *device, AgNdMsgHwTest *p_msg, AG_U32 n_size);
void ag_nd_hw_test_mem_data(AgNdDevice *p_device, AgNdMsgHwTest *p_msg, AG_U32 n_size);
void ag_nd_hw_test_quick_access(AgNdDevice *p_device, AgNdMsgHwTest *p_msg);
void ag_nd_hw_test_reset(AgNdDevice *p_device, AgNdMsgHwTest *p_msg);

AG_BOOL ag_nd_hw_test_is_mem_word_ok(AG_U32 n_address);


AgResult 
ag_nd_opcode_read_hw_test(AgNdDevice *p_device, AgNdMsgHwTest *p_msg)
{
    p_msg->b_passed = AG_TRUE;


    switch(p_msg->n_test_case)
    {
    case AG_ND_TEST_QUICK_ACCESS:

        ag_nd_hw_test_quick_access(p_device, p_msg);

        break;


    case AG_ND_TEST_REGISTERS_FAST:
    case AG_ND_TEST_REGISTERS_FULL:

        p_device->x_bit.b_passed = AG_TRUE;
        p_device->x_bit.n_msg_max = AG_ND_TEST_REGISTERS_FAST == p_msg->n_test_case ? 1 : (AG_U32)-1;
        p_device->x_bit.n_verbose = AG_ND_BIT_VERBOSE_LEVEL_QUIET;
        p_device->x_bit.p_print = NULL;
        p_device->x_bit.n_fail_addr = (AG_U32)-1;

        ag_nd_bit_all_regs(p_device);

        p_msg->b_passed = p_device->x_bit.b_passed;
        p_msg->n_fail_addr = p_device->x_bit.n_fail_addr;

        break;


    case AG_ND_TEST_MEMORY_FAST:
    case AG_ND_TEST_MEMORY_FULL:

        if (AG_ND_ARCH_MASK_NEMO == p_device->n_chip_mask)
        {
            /* */
            /* test SSRAM */
            /* */
            ag_nd_hw_test_mem(p_device, p_msg, 0, p_device->n_ext_mem_bank0_size, AG_FALSE);
            if (!p_msg->b_passed)
                return AG_S_OK;

            /* */
            /*  test ucode bank */
            /*  */
            ag_nd_hw_test_mem(p_device, p_msg, 0, 1024, AG_TRUE);
            if (!p_msg->b_passed)
                return AG_S_OK;
        }
        else
        {
            /* */
            /* test SSRAM */
            /* */
            ag_nd_hw_test_mem(p_device, p_msg, 0, p_device->n_ext_mem_bank0_size, AG_FALSE);
            if (!p_msg->b_passed)
                return AG_S_OK;

            ag_nd_hw_test_mem(p_device, p_msg, 1, p_device->n_ext_mem_bank1_size, AG_FALSE);
            if (!p_msg->b_passed)
                return AG_S_OK;

            /* */
            /* test ucode bank */
            /*  */
            ag_nd_hw_test_mem(p_device, p_msg, 0, 1024, AG_TRUE);
            if (!p_msg->b_passed)
                return AG_S_OK;

            /* */
            /* test label search tree bank */
            /*  */
            ag_nd_hw_test_mem(p_device, p_msg, 1, 0x10000, AG_TRUE);
            if (!p_msg->b_passed)
                return AG_S_OK;

            /* */
            /* test strict check bank */
            /*  */
            ag_nd_hw_test_mem(p_device, p_msg, 2, 0x10000, AG_TRUE);
            if (!p_msg->b_passed)
                return AG_S_OK;
        }

        break;


    case AG_ND_TEST_RESET:

        ag_nd_hw_test_reset(p_device, p_msg);

        break;


    default:

        return AG_ND_ERR(p_device, AG_E_ND_ARG);
    }


    return AG_S_OK;
}


/* */
/* Schedules memory data and address test for one memory bank */
/* The test will be full or fast according to the test case in p_msg */
/*  */
void ag_nd_hw_test_mem(
    AgNdDevice      *p_device, 
    AgNdMsgHwTest   *p_msg, 
    AG_U32          n_bank, 
    AG_U32          n_bank_size,
    AG_BOOL         b_bank_internal)
{
    AgNdRegExtendedAddress x_addr;


    /* */
    /* validate bank number and size */
    /*  */
    assert(0 == n_bank || 1 == n_bank);
    assert(n_bank_size > 0);


    /* */
    /* prepare bank id to return in the case of test failure */
    /*  */
    p_msg->n_fail_bank = n_bank;
    p_msg->n_fail_bank_internal = b_bank_internal;


    /* */
    /* set memory access to the bank */
    /*  */
    x_addr.n_reg = 0;
    x_addr.x_fields.b_internal_memory = (field_t) b_bank_internal;
    x_addr.x_fields.n_bank = (field_t) n_bank;
    ag_nd_reg_write(p_device, AG_REG_EXTENDED_ADDRESS, x_addr.n_reg);


    /* */
    /* memory test */
    /*  */
    ag_nd_hw_test_mem_data(
        p_device, 
        p_msg, 
        AG_ND_TEST_MEMORY_FAST == p_msg->n_test_case ? 2 : n_bank_size);

    if (!p_msg->b_passed)
        return;


    /* */
    /* address test */
    /*  */
    if (AG_ND_TEST_MEMORY_FAST == p_msg->n_test_case)
        ag_nd_hw_test_mem_address_fast(p_device, p_msg, n_bank_size);
    else
        ag_nd_hw_test_mem_address_full(p_device, p_msg, n_bank_size);

    if (!p_msg->b_passed)
        return;
}


#define AG_ND_HW_TEST_WORD(addr) (*(volatile AG_U16*)(addr))

/* */
/* test one memory word by write/read/verify 0, FFFF, AAAA and 5555 value */
/*  */
AG_BOOL 
ag_nd_hw_test_is_mem_word_ok(AG_U32 n_addr)
{
    AG_ND_HW_TEST_WORD(n_addr) = 0x0000;
    if (0x0000 != AG_ND_HW_TEST_WORD(n_addr))
        return AG_FALSE;

    AG_ND_HW_TEST_WORD(n_addr) = 0xFFFF;
    if (0xFFFF != AG_ND_HW_TEST_WORD(n_addr))
        return AG_FALSE;

    AG_ND_HW_TEST_WORD(n_addr) = 0xAAAA;
    if (0xAAAA != AG_ND_HW_TEST_WORD(n_addr))
        return AG_FALSE;

    AG_ND_HW_TEST_WORD(n_addr) = 0x5555;
    if (0x5555 != AG_ND_HW_TEST_WORD(n_addr))
        return AG_FALSE;

    return AG_TRUE;
}

/* */
/* performs memory data test */
/*  */
void 
ag_nd_hw_test_mem_data(AgNdDevice *p_device, AgNdMsgHwTest *p_msg, AG_U32 n_size)
{
    AG_U32 n_addr;
    AG_U32 n_addr_last;


    n_addr = p_device->n_base | 0x800000;
    n_addr_last = n_addr + n_size;


    for (;n_addr < n_addr_last; n_addr += 2)
    {
        if (!ag_nd_hw_test_is_mem_word_ok(n_addr))
        {
            p_msg->b_passed = AG_FALSE;
            p_msg->n_fail_addr = n_addr;
            return;
        }
    }
}


/* */
/* performs full address test by writing the address value to data */
/* each iteration writes reads and verifies two consequtive words, */
/* address sequence is 0, 4, 8, 12, 16, 20,... */
/*  */
void 
ag_nd_hw_test_mem_address_full(
    AgNdDevice      *p_device, 
    AgNdMsgHwTest   *p_msg, 
    AG_U32          n_size)
{
    AG_U32 n_addr_last;
    AG_U32 n_addr_first;
    AG_U32 n_addr;


    n_addr_first = p_device->n_base | 0x800000;
    n_addr_last = n_addr_first + n_size - 4;

    /* */
    /* first stage: write to the requested addreses  */
    /*  */
    for (n_addr = n_addr_last; n_addr >= n_addr_first; n_addr -= 4)
    {
        AG_ND_HW_TEST_WORD(n_addr) = AG_ND_HIGH16(n_addr);
        AG_ND_HW_TEST_WORD(n_addr + 2) = AG_ND_LOW16(n_addr);
    }


    /* */
    /* second stage: read the addreses back */
    /*  */
    for (n_addr = n_addr_first; n_addr <= n_addr_last; n_addr += 4)
    {
        if (AG_ND_HIGH16(n_addr) != AG_ND_HW_TEST_WORD(n_addr) ||
            AG_ND_LOW16(n_addr) != AG_ND_HW_TEST_WORD(n_addr + 2))
        {
            p_msg->b_passed = AG_FALSE;
            p_msg->n_fail_addr = n_addr;
            return;
        }
    }
}

/* */
/* performs fast address test by writing the address value to data */
/* each iteration writes reads and verifies two consequtive words, */
/* address sequence is 4, 8, 16, 32, 64, ... */
/*  */
void 
ag_nd_hw_test_mem_address_fast(
    AgNdDevice      *p_device, 
    AgNdMsgHwTest   *p_msg, 
    AG_U32          n_size)
{
    AG_U32 n_addr_first;
    AG_U32 n_addr;
    AG_U32 n_offset;


    n_addr_first = p_device->n_base | 0x800000;

    /* */
    /* first stage: write to the requested addreses  */
    /*  */
    for (n_offset = ag_nd_flp2(n_size - 4); n_offset >= 4; n_offset >>= 1)
    {
        n_addr = n_addr_first + n_offset;
        AG_ND_HW_TEST_WORD(n_addr) = AG_ND_HIGH16(n_addr);
        AG_ND_HW_TEST_WORD(n_addr + 2) = AG_ND_LOW16(n_addr);
    }


    /* */
    /* second stage: read the addreses back */
    /*  */
    for (n_offset = 4; n_offset <= ag_nd_flp2(n_size - 4); n_offset <<= 1)
    {
        n_addr = n_addr_first + n_offset;

        if (AG_ND_HIGH16(n_addr) != AG_ND_HW_TEST_WORD(n_addr) ||
            AG_ND_LOW16(n_addr) != AG_ND_HW_TEST_WORD(n_addr + 2))
        {
            p_msg->b_passed = AG_FALSE;
            p_msg->n_fail_addr = n_addr;
            return;
        }
    }
}

/* */
/* performs write/read/verify of one r/w register */
/*  */
void 
ag_nd_hw_test_quick_access(AgNdDevice *p_device, AgNdMsgHwTest *p_msg)
{
    AG_U16 n_reg;

    ag_nd_reg_write(p_device, AG_REG_PAYLOAD_REPLACEMENT_POLICY, 0x5555);
    ag_nd_reg_read(p_device, AG_REG_PAYLOAD_REPLACEMENT_POLICY, &n_reg);
    if (0x5555 != n_reg)
        p_msg->b_passed = AG_FALSE;

    ag_nd_reg_write(p_device, AG_REG_PAYLOAD_REPLACEMENT_POLICY, 0xAAAA);
    ag_nd_reg_read(p_device, AG_REG_PAYLOAD_REPLACEMENT_POLICY, &n_reg);
    if (0xAAAA != n_reg)
        p_msg->b_passed = AG_FALSE;
}

void 
ag_nd_hw_test_reset(AgNdDevice *p_device, AgNdMsgHwTest *p_msg)
{
    AG_U16 n_reg;

    ag_nd_reg_write(p_device, AG_REG_PAYLOAD_REPLACEMENT_POLICY, 0xFFFF);
    ag_nd_reg_read(p_device, AG_REG_PAYLOAD_REPLACEMENT_POLICY, &n_reg);
    if (0xFFFF != n_reg)
        p_msg->b_passed = AG_FALSE;

    p_msg->p_reset_cb();
    ag_nd_sleep_msec(100);

    ag_nd_reg_read(p_device, AG_REG_PAYLOAD_REPLACEMENT_POLICY, &n_reg);
    if (0x0 != n_reg)
        p_msg->b_passed = AG_FALSE;
}

