/*
 * $Id: i2c_cmds.c,v 1.124 Broadcom SDK $
 *
 * I2C specific commands.
 * These commands are specific to the I2C driver and or I2C slave
 * devices which use that driver. See drv/i2c/ for more details.
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
 */
#ifndef NO_SAL_APPL

#ifdef INCLUDE_I2C

#include <sal/core/libc.h>
#include <sal/core/boot.h>
#include <shared/bsl.h>
#include <sal/appl/sal.h>
#include <sal/appl/io.h>
#include <sal/appl/config.h>
#include <shared/bsl.h>
#include <soc/cmext.h>
#include <soc/drv.h>
#include <soc/debug.h>
#include <soc/i2c.h>
#include <soc/mem.h>

#include <bcm/error.h>
#include <bcm/bcmi2c.h>
#include <bcm/init.h>
#include <bcm/error.h>

#ifdef BCM_CALADAN3_SVK
#include <soc/caladan3_pio_defs.h>
#include <soc/smm665.h>
#endif

#include <appl/diag/system.h>
#ifdef COMPILER_HAS_DOUBLE
#include <stdlib.h>
#endif


/* Decimal point formatting */
#define INT_FRAC_1PT(x) (x) / 10, (x) % 10
#define INT_FRAC_3PT(x) (x) / 1000, (x) % 1000
#define INT_FRAC_3PT_3(x) (x) / 1000000, ((x) / 1000) % 1000
#define INT_FRAC_2PT_4(x) (x) / 1000000, ((x) / 10000) % 100

#define I2C_MUX_VOLTAGE_CONTROL  0 
STATIC char * _i2c_mux_default_dev_name_get(void);
STATIC int dac_devs_init (int unit,int bx, char *mux_dev);
#ifdef BCM_CALADAN3_SVK
extern cmd_result_t cpu_i2c_muxsel(int unit, args_t *a);
#endif


/*
 * Function: cmd_pcie
 * Purpose: Display/Modify pcie config registers via I2C debug BUS
 */
char cmd_pcie_usage[] =
    "Usages:\n\t"
    "       pcie r4 [off] [nbytes]\n\t"
    "           - show specified number of pcie bytes starting at offset.\n\t"
    "       pcie w4 [off] [data] \n\t"
    "           - write data byte to specified pcie offset.\n\t"
    "\n";

cmd_result_t
cmd_pcie(int unit, args_t *a)
{
    char *function = ARG_GET(a);
    char *offset = ARG_GET(a);
    char *dw = ARG_GET(a);
    uint32 addr, data, i ;
    uint8* ptr, *dpb;
    int fd, rv = SOC_E_NONE;
    
    if (! sh_check_attached(ARG_CMD(a), unit))
    return CMD_FAIL;

    if (!function || !offset || !dw)
    return CMD_USAGE ;
    
    if ( (fd = bcm_i2c_open(unit, I2C_PCIE_0, 0,0)) < 0) {
    cli_out("%s: cannot open I2C device %s: %s\n",
            ARG_CMD(a), I2C_PCIE_0, bcm_errmsg(fd));
    return CMD_FAIL;
    }

    /* NOTE: will use first NVM device found : pcie0*/
    cli_out("%s: using device %s\n", ARG_CMD(a), soc_i2c_devname(unit, fd));
    addr = parse_integer(offset);
    data = parse_integer(dw);
    if ( ! sal_strncasecmp(function, "r4", 2) ){
    ptr = (uint8*)sal_alloc(4, "cmd_pcie");
    if(!ptr)
        return CMD_USAGE;
    /* Pretty print data : one 32-byte page at a time*/
    for(i = 0; i < data; i++) {
            unsigned int r_len =4;
            if( (rv = bcm_i2c_read(unit, fd, addr + i * 4, ptr, &r_len)) < 0){
                cli_out("ERROR: read of %d bytes failed:%s\n",
                        parse_integer(dw), bcm_errmsg(rv));
            }
        cli_out("%04x 0x%02x%02x%02x%02x\n",
                addr + i * 4,
                ptr[0],
                ptr[1],
                ptr[2],
                ptr[3]);
    }
    cli_out("\nRead %d bytes total\n", data); 
        sal_free(ptr);
    } else if ( ! sal_strncasecmp(function,"w4",2) ){
        dpb = (uint8 *)&data;
    if( (rv = bcm_i2c_write(unit, fd, addr, dpb, 4)) < 0){
        cli_out("ERROR: write of byte at 0x%x failed:%s\n",
                addr, bcm_errmsg(rv));
    }
    } else {
    return CMD_USAGE;
    }
    return CMD_OK;
}

/*
 * Function: cmd_nvram
 * Purpose: Display/Modify NVRAM via Atmel 24C64 64K I2C EEPROM chip.
 */
char cmd_nvram_usage[] =
    "Usages:\n\t"
    "       nvram r [off] [nbytes]\n\t"
    "           - show specified number of NVRAM bytes starting at offset.\n\t"
    "       nvram w [off] [data] \n\t"
    "           - write data byte to specified NVRAM offset.\n\t"
    "\n";

cmd_result_t
cmd_nvram(int unit, args_t *a)
{
    char *function = ARG_GET(a);
    char *offset = ARG_GET(a);
    char *dw = ARG_GET(a);
    uint32 addr, data, i ;
    uint8* ptr, db;
    int fd, rv = SOC_E_NONE;
    
    if (! sh_check_attached(ARG_CMD(a), unit))
    return CMD_FAIL;

    if (!function || !offset || !dw)
    return CMD_USAGE ;
    
    if ( (fd = bcm_i2c_open(unit, I2C_NVRAM_0, 0,0)) < 0) {
    cli_out("%s: cannot open I2C device %s: %s\n",
            ARG_CMD(a), I2C_NVRAM_0, bcm_errmsg(fd));
    return CMD_FAIL;
    }

    /* NOTE: will use first NVM device found : nvram0*/
    cli_out("%s: using device %s\n", ARG_CMD(a), soc_i2c_devname(unit, fd));
    addr = parse_integer(offset);
    data = parse_integer(dw);
    if ( ! sal_strncasecmp(function, "r", 1) ){
    ptr = (uint8*)sal_alloc(data, "cmd_nvram");
    if(!ptr)
        return CMD_USAGE;
    cli_out("Reading %d bytes at address 0x%x\n\t", data,addr);
    if( (rv = bcm_i2c_read(unit, fd, addr, ptr, &data)) < 0){
        cli_out("ERROR: read of %d bytes failed:%s\n",
                parse_integer(dw), bcm_errmsg(rv));
    }
    /* Pretty print data : one 32-byte page at a time*/
    for(i = 0; i < data; i++){
        cli_out("0x%02x ", ptr[i]);
        if( (i < (data-1)) && ((i % 8) == 7))
        cli_out("\n\t");
    }
    cli_out("\nRead %d bytes total\n", data); 
        sal_free(ptr);
    } else if ( ! sal_strncasecmp(function,"w",1) ){
    db = (uint8)data;
    if( (rv = bcm_i2c_write(unit, fd, addr, &db, 1)) < 0){
        cli_out("ERROR: write of byte at 0x%x failed:%s\n",
                addr, bcm_errmsg(rv));
    }
    } else {
    return CMD_USAGE;
    }
    return CMD_OK;

}

/*
 * Function: cmd_adc
 * Purpose: Show MAX127 A/D Conversions for boards with the chip.
 */
char cmd_adc_usage[] =
    "Usages:\n\t"
    "       adc show num\n\t"
    "           - show MAX127 A/D Conversions (num = 0,1).\n\t"
    "       adc samples num\n\t"
    "           - set MAX127 A/D Conversions sample count.\n\t"
    "\n";
cmd_result_t
cmd_adc(int unit, args_t *a)
{
    int fd;    
    uint32 num;
    char dev[25];
    char *function = ARG_GET(a);
    char *chip = ARG_GET(a);
    
    if (! sh_check_attached(ARG_CMD(a), unit))
    return CMD_FAIL;

    if ((! function) || (!chip))
    return CMD_USAGE ;

    num = parse_integer(chip);


    if(!sal_strncasecmp(function,"samples",sal_strlen(function))){
        if ( (fd = bcm_i2c_open(unit, I2C_ADC_0,0,0)) < 0) {
            cli_out("%s: cannot open I2C device %s: %s\n",
                    ARG_CMD(a), dev, bcm_errmsg(fd));
	    return CMD_FAIL;
        }
        if ( (bcm_i2c_ioctl(unit, fd, I2C_ADC_SET_SAMPLES, &num, 0) < 0)) {
	    cli_out("ERROR: failed to set samples count.\n");
        }
	return CMD_OK;
    }

    sprintf(dev,"%s%d","adc",num);

    if ( (fd = bcm_i2c_open(unit, dev,0,0)) < 0) {
    cli_out("%s: cannot open I2C device %s: %s\n",
            ARG_CMD(a), dev, bcm_errmsg(fd));
    return CMD_FAIL;
    }

    if ( (bcm_i2c_ioctl(unit, fd, I2C_ADC_DUMP_ALL, NULL, 0) < 0)) {
	cli_out("ERROR: failed to perform A/D conversions.\n");
    }

    return CMD_OK ;

}

/*
 * Function: cmd_xclk
 * Purpose: Set W2239x clock speed for PCI, SDRAM, or core clocks
 */
char cmd_xclk_usage[] =
    "Usages:\n\t"
#ifdef COMPILER_STRING_CONST_LIMIT
    "       xclocks dump | r | w | core | sdram | pci [...]\n"
#else
    "       xclocks dump \n\t"
    "           - display all registers on CY22393x.\n\t"
    "       xclocks r <address> \n\t"
    "           - read CY2239x register at address.\n\t"
    "       xclocks w <address> <data> \n\t"
    "           - write CY22393 register data at address.\n\t"
    "       xclocks core [speed] \n\t"
    "           - set BCM56xx core clock speed.\n\t"
    "       xclocks sdram [speed] \n\t"
    "           - set BCM56xx SDRAM clock speed.\n\t"
    "       xclocks pci [speed] \n\t"
    "           - set BCM56xx PCI clock speed.\n"
    "In all cases, if a speed is not specified, the current is shown\n"
#endif
    ;

cmd_result_t
cmd_xclk(int unit, args_t *a)
{
    char *source = ARG_GET(a);
    char *speed = ARG_GET(a);
#ifdef COMPILER_HAS_DOUBLE
    double clockFVal = 0;
#else
    int clockFVal = 0;
#endif
    int cmd = 0;
    int fd ;
    
    if (! sh_check_attached(ARG_CMD(a), unit))
    return CMD_FAIL;

    if (!source ) {
    cli_out("Error: no operation or source (core, etc.) specified!\n");
    return CMD_FAIL;
    }
    /* NOTE: on 5625 BB, MUX (lpt0)=1 */
    
    /* Setup ioctl command code */
    if(!sal_strcmp(source,"core")){
    cmd = (speed != NULL) ? I2C_XPLL_SET_CORE : I2C_XPLL_GET_CORE;
    } else if (!sal_strcmp(source, "pci")){
    cmd = (speed != NULL) ? I2C_XPLL_SET_PCI : I2C_XPLL_GET_PCI;
    } else if (!sal_strcmp(source, "sdram")){
    cmd = (speed != NULL) ? I2C_XPLL_SET_SDRAM : I2C_XPLL_GET_SDRAM;
    } else if (!sal_strcmp(source, "r") || !sal_strcmp(source, "dump")){
    cmd = I2C_XPLL_GET_REG;
    } else if (!sal_strcmp(source, "w")){
    cmd = I2C_XPLL_SET_REG;
    } else {
    cli_out("Invalid subcommand or clock source (%s)\n", source);
    return CMD_USAGE;
    }
    /* Open PLL. This should be a CY22393, but could be an older CY22150 */
    if ( (fd = bcm_i2c_open(unit, I2C_XPLL_0, 0, 0)) < 0) {
    cli_out("%s: cannot open I2C device %s: %s\n",
            ARG_CMD(a), I2C_XPLL_0, bcm_errmsg(fd));
    return CMD_FAIL;
    }
    if ( I2C_XPLL_GET_REG == cmd ) {
    /* Read register */
    char* addr = speed;
    uint8 x;
    uint32 nbytes = 0;

    if( addr) { 
        uint8 off = parse_integer(addr);
        cli_out("Read register @%s\n", addr);
        bcm_i2c_read(unit, fd, off, &x, &nbytes);
        cli_out("pll[%x] = 0x%x\n", off, x);
    }
    else {
        int i;
        /* Assume the CY22393 driver */
        cli_out("Read all registers ...\n");
        for( i = 0x08; i <= 0x17; i++) { 
        bcm_i2c_read(unit, fd, i, &x, &nbytes);
        cli_out("pll[%x] = 0x%x\n", i, x);
        }
        for( i = 0x40; i <= 0x57; i++) { 
        bcm_i2c_read(unit, fd, i, &x, &nbytes);
        cli_out("pll[%x] = 0x%x\n", i, x);
        }
    }

    } else if (I2C_XPLL_SET_REG == cmd ) {
    /* Write register */
    char* addr = speed;
    char* data = ARG_GET(a);
    uint8 off,val;
    cli_out("Write register\n");

    if ( !addr || !data) {
        return CMD_USAGE;
    } else {
        off = parse_integer(addr);
        val = (uint8)parse_integer(data);
        bcm_i2c_write(unit, fd, off, &val, 1);
        cli_out("0x%x => pll[%x]\n", val, off);
    }
    
    } else { /* IOCTL's */

    /* Set clock speed */
    if ( speed) { 
#ifdef COMPILER_HAS_DOUBLE
        clockFVal = atof( speed ) ;
#else
            /* Parse input in MHz and convert to Hz */
        clockFVal = _shr_atof_exp10(speed, 6);
#endif
        /* Set speed */
        if ( (bcm_i2c_ioctl(unit, fd, cmd, &clockFVal, 0) < 0)) {
        cli_out("ERROR: failed to perform "
                "clock speed configuration.\n");
        return CMD_FAIL;
        }        
#ifdef COMPILER_HAS_DOUBLE
        cli_out("%s: %s clock set to %2.2fMhz\n",
                I2C_XPLL_0, source, clockFVal ) ;
#else
        cli_out("%s: %s clock set to %d.%02dMhz\n",
                I2C_XPLL_0, source, 
                INT_FRAC_2PT_4(clockFVal));
#endif
    } else {
        /* Get clock speed */
        if ( (bcm_i2c_ioctl(unit, fd, cmd,&clockFVal, 0) < 0)){
        cli_out("ERROR: failed to retrieve clock configuration.\n");
        }
#ifdef COMPILER_HAS_DOUBLE
        cli_out("%s: %s clock is %2.2fMhz\n",
                I2C_XPLL_0, source, clockFVal ) ;    
#else
        cli_out("%s: %s clock is %d.%02dMhz\n",
                I2C_XPLL_0, source, 
                INT_FRAC_2PT_4(clockFVal));
#endif
    }
    }
    return CMD_OK;
}

/*
 * Function: cmd_poe
 * Purpose: Read and write registers in the LTC4258 chip.
 */
char cmd_poe_usage[] =
    "Usages:\n\t"
#ifdef COMPILER_STRING_CONST_LIMIT
    "       poe poe_num dump | status | clear | r | w\n"
#else
    "       poe poe_num dump \n\t"
    "           - display all registers on LTC4258[poe_num].\n\t"
    "       poe poe_num status \n\t"
    "           - display port status on LTC4258[poe_num].\n\t"
    "       poe poe_num clear \n\t"
    "           - clear interrupt bits on LTC4258[poe_num].\n\t"
    "       poe poe_num r <address> \n\t"
    "           - read LTC4258[poe_num] register at address.\n\t"
    "       poe poe_num w <address> <data> \n\t"
    "           - write LTC4258[poe_num]  register data at address.\n"
    "In all cases, poe_num = [1..6]. MuxSel may also be necessary.\n"
#endif
    ;

cmd_result_t
cmd_poe(int unit, args_t *a)
{

#define I2C_POE_GET_REG    0x10
#define I2C_POE_SET_REG    0x20
#define TEXTBUFFER_SIZE 1024

    char *num_text = ARG_GET(a);
    char *op_text  = ARG_GET(a);
    char *addr_text = ARG_GET(a);
    char *device[6] = {
      I2C_POE_1, I2C_POE_2, I2C_POE_3,
      I2C_POE_4, I2C_POE_5, I2C_POE_6
    };
    char* data_text;
    int poe_num, cmd = 0;
    int fd ;

    if (! sh_check_attached(ARG_CMD(a), unit))
    return CMD_FAIL;

    if (!num_text) {
    cli_out("ERROR: no PoE index specified!\n");
    return CMD_FAIL;
    }

    if ( ((poe_num = parse_integer(num_text)) < 1) ||
     (poe_num > 6) ) {
    cli_out("ERROR: invalid PoE poe_num!\n");
    return CMD_FAIL;
    }

    if (!op_text ) {
    cli_out("ERROR: no operation specified!\n");
    return CMD_FAIL;
    }
    
    /* Setup ioctl command code */
    if (!sal_strcmp(op_text, "dump") || !sal_strcmp(op_text, "r")) {
    cmd = I2C_POE_GET_REG;
    } else if (!sal_strcmp(op_text, "status")) {
    cmd = I2C_POE_IOC_STATUS;
    } else if (!sal_strcmp(op_text, "clear")) {
    cmd = I2C_POE_IOC_CLEAR;
    } else if (!sal_strcmp(op_text, "rescan")) {
    cmd = I2C_POE_IOC_RESCAN;
    } else if (!sal_strcmp(op_text, "w")) {
    cmd = I2C_POE_SET_REG;
    } else {
    cli_out("Invalid operation (%s)\n", op_text);
    return CMD_USAGE;
    }

    /* Open the LTC4258. */
    if ( (fd = bcm_i2c_open(unit, device[poe_num-1], 0, 0)) < 0) {
    cli_out("%s: cannot open I2C device %s: %s\n",
            ARG_CMD(a), device[poe_num-1], bcm_errmsg(fd));
    return CMD_FAIL;
    }
    if ( I2C_POE_GET_REG == cmd ) {
    /* Read register */
    uint8 x;
    uint32 nbytes = 0;

    if( addr_text) { 
        uint8 off = parse_integer(addr_text);
        cli_out("Read register [0x%02x]\n", off);
        bcm_i2c_read(unit, fd, off, &x, &nbytes);
        cli_out("%s[0x%02x] = 0x%02X\n", device[poe_num-1], off, x);
    }
    else {
      data_text = (char *)sal_alloc(TEXTBUFFER_SIZE, "cmd_poe");
      cmd = I2C_POE_IOC_REG_DUMP;
      if ( (bcm_i2c_ioctl(unit, fd, cmd, data_text, TEXTBUFFER_SIZE) < 0)) {
        cli_out("ERROR: failed to dump PoE chip registers.\n");
        if (data_text)
          sal_free(data_text);
        return CMD_FAIL;
      }        
      cli_out("%s\n", data_text);
      sal_free(data_text);
    }
    } 
    else if (I2C_POE_SET_REG == cmd ) {
    /* Write register */
    uint8 off,val;
    data_text = ARG_GET(a);
    cli_out("Write register\n");

    if ( !addr_text || !data_text) {
        return CMD_USAGE;
    } else {
        off = parse_integer(addr_text);
        val = (uint8)parse_integer(data_text);
        bcm_i2c_write(unit, fd, off, &val, 1);
        cli_out("0x%02X => %s[0x%02x]\n", val, device[poe_num-1], off);
    }
    }

    else if (I2C_POE_IOC_CLEAR == cmd ) {
        /* Clear any PoE interrupts */
    if ( (bcm_i2c_ioctl(unit, fd, cmd, NULL, 0) < 0)) {
      cli_out("ERROR: %s Clear_Interrupts IOCTL failed.\n",
              device[poe_num-1]);
      return CMD_FAIL;
    }        
    } 

    else if (I2C_POE_IOC_RESCAN == cmd ) {
        /* Re-scan (detection and class) */
    if ( (bcm_i2c_ioctl(unit, fd, cmd, NULL, 0) < 0)) {
      cli_out("ERROR: %s ReScan IOCTL failed.\n", device[poe_num-1]);
      return CMD_FAIL;
    }        
    } 

    else if (I2C_POE_IOC_STATUS == cmd ) {
        /* Use new IOCTL method to get text information */
    data_text = (char *)sal_alloc(TEXTBUFFER_SIZE, "cmd_poe");
    if ( (bcm_i2c_ioctl(unit, fd, cmd, data_text, TEXTBUFFER_SIZE) < 0)) {
      cli_out("ERROR: %s Status IOCTL failed.\n", device[poe_num-1]);
      if (data_text)
        sal_free(data_text);
      return CMD_FAIL;
    }        
    cli_out("%s\n", data_text);
    sal_free(data_text);
    } 
    
    else { /* other IOCTL's */

      /* Fancy Shmancy IOCTLs under construction 
       * I2C_POE_IOC_ENABLE_PORT
       * I2C_POE_IOC_AUTO
       * I2C_POE_IOC_DISABLE_PORT
       * I2C_POE_IOC_SHUTDOWN
       */

    }
    return CMD_OK;
} /* end cmd_poe() */

typedef enum {
    /* Legacy PD was detected */
    POE_PD_PSTAT_ON_C = 0,

    /* 802.3af-compliant PD was detected */
    POE_PD_PSTAT_ON_R = 1,

    /* Mains voltage is higher than limits; all ports shut down */
    POE_PD_PSTAT_OFF_HI = 6,

    /* Mains voltage is lower than limits; all ports shut down */
    POE_PD_PSTAT_OFF_LO = 7,

    /* Hardware pin disables all ports  */
    POE_PD_PSTAT_OFF_HW = 8,

    /* There are fewer ports available than set */
    POE_PD_PSTAT_OFF_NULL = 12,

    /* Interim state during power up */
    POE_PD_PSTAT_OFF_POWER_UP = 17,

    /* Port does not respond or hardware is at fault */
    POE_PD_PSTAT_OFF_HW_INT = 18,

    /* User command set port to off */
    POE_PD_PSTAT_OFF_USER_CMD = 26,

    /* Interim state during line detection */
    POE_PD_PSTAT_OFF_DET = 27,

    /* Non-standard PD connected */
    POE_PD_PSTAT_OFF_NON_PD = 28,

    /* Succession of under/over load states caused port shutdown */
    POE_PD_PSTAT_OFF_UNDER_OVER = 29,

    /* Underload state according to 802.3af or Signature capacitor < 22uF */
    POE_PD_PSTAT_OFF_UNDER_CAP = 30,

    /* Overload state accourding to 802.3af or Signature capacitor > 1000uF */
    POE_PD_PSTAT_OFF_OVER_CAP = 31,

    /* Power management fn shut down port due to lack of power */
    POE_PD_PSTAT_OFF_LACK = 32,

    /* Hardware problems preventing port operation */
    POE_PD_PSTAT_OFF_INT_HW = 33,

    /* Port fails capacitor detection due to voltage being applied */
    POE_PD_PSTAT_OFF_VOL_INJ = 36,

    /* Port fails capacitor detection due to out-out-range capacitor value */
    POE_PD_PSTAT_OFF_CAP_OUT = 37,

    /* Port fails capacitor detection due to discharged capacitor */
    POE_PD_PSTAT_OFF_CAP_DET = 38,

    /* Port is forced on, unless systems error occurs */
    POE_PD_PSTAT_ON_FORCE = 43,

    /* Underfined error during force on */
    POE_PD_PSTAT_ON_UNDEF = 44,

    /* Supply voltage higher than settings */
    /*   (error appears only after port is forced on) */
    POE_PD_PSTAT_ON_VOL_HI = 45,

    /* Supply voltage lower than settings */
    POE_PD_PSTAT_ON_VOL_LO = 46,

    /* Disable_PDU flags was raised during force on */
    POE_PD_PSTAT_ON_PDU_FLAG = 47,

    /* Disabling is done by "Set on/off" command */
    POE_PD_PSTAT_ON_OFF_CMD = 48,

    /* Overload condition according to 802.3af */
    POE_PD_PSTAT_OFF_OVERLOAD = 49,

    /* Power management mechanism detects out of power budget */
    POE_PD_PSTAT_OFF_BUDGET = 50,

    /* Communication error with ASIC, after force on command */
    POE_PD_PSTAT_ON_ERR_ASIC = 51
} poe_pd_port_status_t;

STATIC char*
poe_pd_decode_port_status(poe_pd_port_status_t status)
{
    switch (status) {
    case POE_PD_PSTAT_ON_C:    
         return "On (valid capacitor detected)";
    case POE_PD_PSTAT_ON_R:   
         return "On (valid resistor detected)";
    case POE_PD_PSTAT_OFF_HI:  
         return "Off (main supply voltage higher than permitted)";
    case POE_PD_PSTAT_OFF_LO: 
         return "Off (main supply voltage lower than permitted)";
    case POE_PD_PSTAT_OFF_HW: 
         return "Off ('disable all ports' pin is active)";
    case POE_PD_PSTAT_OFF_NULL: 
         return "Off (non-existing port number)";
    case POE_PD_PSTAT_OFF_POWER_UP: 
         return "Off (power-up is in process)";
    case POE_PD_PSTAT_OFF_HW_INT: 
         return "Off (internal hardware fault)";
    case POE_PD_PSTAT_OFF_USER_CMD: 
         return "Off (user setting)";
    case POE_PD_PSTAT_OFF_DET: 
         return "Off (detection is in process)";
    case POE_PD_PSTAT_OFF_NON_PD: 
         return "Off (non-802.3af powered device)";
    case POE_PD_PSTAT_OFF_UNDER_OVER: 
         return "Off (overload & underload states)";
    case POE_PD_PSTAT_OFF_UNDER_CAP: 
         return "Off (underload state or capacitor value is too samll)";
    case POE_PD_PSTAT_OFF_OVER_CAP: 
         return "Off (overload state or capacitor value is too large)";
    case POE_PD_PSTAT_OFF_LACK: 
         return "Off (power budget exceeded)";
    case POE_PD_PSTAT_OFF_INT_HW: 
         return "Off (internal hardware fault)";
    case POE_PD_PSTAT_OFF_VOL_INJ: 
         return "Off (voltage injection into the port)";
    case POE_PD_PSTAT_OFF_CAP_OUT: 
         return "Off (improper capacitor detection result)";
    case POE_PD_PSTAT_OFF_CAP_DET: 
         return "Off (discharged load)";
    case POE_PD_PSTAT_ON_FORCE: 
         return "On (force on, unless systems error occurs)";
    case POE_PD_PSTAT_ON_UNDEF: 
         return "Undefined error during force on";
    case POE_PD_PSTAT_ON_VOL_HI: 
         return "Supply voltage higher than settings";
    case POE_PD_PSTAT_ON_VOL_LO : 
         return "Supply voltage lower than settings";
    case POE_PD_PSTAT_ON_PDU_FLAG: 
         return "Disable_PDU flag was raised during force on";
    case POE_PD_PSTAT_ON_OFF_CMD: 
         return "Port is forced on then disabled or disabled then forced on";
    case POE_PD_PSTAT_OFF_OVERLOAD: 
         return "Off (overload condition accouding to 802.3af)";
    case POE_PD_PSTAT_OFF_BUDGET: 
         return "Off (out of power budget while in forced power state)";
    case POE_PD_PSTAT_ON_ERR_ASIC: 
         return "Communication error with ASIC after force on command";
    default:     
         return "Unknown status ?";
    } 
}

STATIC void
poe_pd_pkt_dump(uint8 *src, int len, char *msg)
{
    int i;

    if (msg || !len)
        cli_out("%s (%d):\n", msg, len );
    for (i = 0; i < len; i++) {
         cli_out("0x%02x ", src[i]);
         if (!((i + 1) % 16))
             cli_out("\n");
    }
    cli_out("\n");
}

STATIC void 
poe_pd_pkt_cksum(unsigned char *data)
{
    int i;
    uint16 sum=0;

    for (i = 0; i < 13; i++) {
         sum += data[i]; 
    }
    data[13] = (sum & 0xff00) >> 8; 
    data[14] =  sum & 0xff; 
}

STATIC int
poe_pd_pkt_txrx(int unit, int fd, unsigned char *data, 
                int seq, int debug)
{
    int rv;
    uint32 len;
    soc_timeout_t to;

    if ((rv = bcm_i2c_write(unit, fd, 0, data, 15)) < 0) {
        cli_out("ERROR: write byte failed: %s\n",
                bcm_errmsg(rv));
        return CMD_FAIL;
    }

    if (debug) {
        poe_pd_pkt_dump(data, 15, "Tx to PD63000");
    }

    memset(data, 0x0, 15);
    soc_timeout_init(&to, 100000, 0);
    for (;;) {
         if (soc_timeout_check(&to)) {
             break;
         }
    }

    if((rv = bcm_i2c_read(unit, fd, 0, data, &len)) < 0){
        cli_out("ERROR: read byte failed: %s\n",
                bcm_errmsg(rv));
        return CMD_FAIL;
    }

    if (debug) {
        poe_pd_pkt_dump(data, 15, "Rx from PD63000");
    }

    if (data[1] != seq) {
        seq++;
        cli_out("ERROR: read from PD630000 out of sync.\n");
        cli_out("       use 'xpoe read' to flush data.\n");
        poe_pd_pkt_dump(data, 15, "Rx from PD63000");
        return CMD_FAIL;
    }

    return CMD_OK;
}  

#define  PD63000_KEY_COMMAND     0x00
#define  PD63000_KEY_PROGRAM     0x01
#define  PD63000_KEY_REQUEST     0x02
#define  PD63000_KEY_TELEMETRY   0x03
#define  PD63000_KEY_REPORT      0x52
#define  PD63000_NULL_DATA       0x4e
#define  PD63000_SUB_GLOBAL      0x07
#define  PD63000_SUB_SW          0x21
#define  PD63000_SUB_VERZ        0x1e
#define  PD63000_SUB_STATUS      0x3d
#define  PD63000_SUB_SUPPLY      0x0b
#define  PD63000_SUB_SUPPLY1     0x15
#define  PD63000_SUB_MAIN        0x17
#define  PD63000_SUB_CHANNEL     0x05
#define  PD63000_SUB_PARAMZ      0x25
#define  PD63000_SUB_PORTSTATUS  0x0e

/*
 * Function: cmd_xpoe
 * Purpose: Communication with PowerDsine PD63000 
 *          8-bit PoE Microcontroller Unit
 */
char cmd_xpoe_usage[] =
    "Usages:\n\t"
#ifdef COMPILER_STRING_CONST_LIMIT
    "       xpoe ver | status | power | port | measure | raw | read | debug\n"
#else
    "       xpoe ver\n\t"
    "           - Get PD63000 software version.\n\t"
    "       xpoe status\n\t"
    "           - Get System Status.\n\t"
    "       xpoe power <value>\n\t"
    "           - Get/Set power supply parameters.\n\t"
    "       xpoe power status\n\t"
    "           - Get main power source supply parameters.\n\t"
    "       xpoe port <port_num>\n\t"
    "           - Get port status (use * for all ports).\n\t"
    "       xpoe measure <port_num>\n\t"
    "           - Get port measurements (use * for all ports).\n\t"
    "       xpoe raw <byte0> ... <byte14>  \n\t"
    "           - Send raw data to PD63000.\n\t"
    "       xpoe read\n\t"
    "           - Read one packet from PD63000.\n\t"
    "       xpoe debug on/off\n\t"
    "           - Turn packet dump to/from PD63000 on/off.\n\t"
    "       xpoe verbose on/off\n\t"
    "           - Turn on/off verbose output (default is on).\n"
#endif
    ;

cmd_result_t
cmd_xpoe(int unit, args_t *a)
{
    char *subcmd, *s;
    int  port, i, fd, rv, no_cksum=0;
    uint8 data[15], echo;
    uint32 len;
    uint16 val;
    static uint8 seq=0;
    static int debug=0;
    static int verbose=1;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
        return CMD_FAIL;
    }

    if ((subcmd = ARG_GET(a)) == NULL) {
        return CMD_USAGE;
    }

    if ( (fd = bcm_i2c_open(unit, I2C_POE_0, 0,0)) < 0) {
        cli_out("%s: cannot open I2C device %s: %s\n",
                ARG_CMD(a), I2C_POE_0, bcm_errmsg(fd));
        return CMD_FAIL;
    }

    if (sal_strcasecmp(subcmd, "verbose") == 0) {
        if ((subcmd = ARG_GET(a)) == NULL) {
            return CMD_USAGE;
        }
        if (sal_strcasecmp(subcmd, "on") == 0) {
            verbose = 1;
        } else {
            verbose = 0;
        }
        return CMD_OK;
    }

    if (sal_strcasecmp(subcmd, "debug") == 0) {
        if ((subcmd = ARG_GET(a)) == NULL) {
            return CMD_USAGE;
        }
        if (sal_strcasecmp(subcmd, "on") == 0) {
            debug = 1;
        } else {
            debug = 0;
        }
        return CMD_OK;
    }

    memset(data, 0x0, 15);
    
    if (sal_strcasecmp(subcmd, "ver") == 0) {
        memset(data, PD63000_NULL_DATA, 15);
        data[0] = PD63000_KEY_REQUEST;
        data[1] = seq;
        data[2] = PD63000_SUB_GLOBAL;
        data[3] = PD63000_SUB_VERZ; 
        data[4] = PD63000_SUB_SW;
        poe_pd_pkt_cksum(data);

        if ((rv = poe_pd_pkt_txrx(unit, fd, data, seq, debug)) < 0) {
            return rv;
        }

        if (data[0] == PD63000_KEY_TELEMETRY) {
            if (verbose) {
                cli_out("H.W. Version %d (0x%x)\n", data[2], data[2]);
                cli_out("S.W. Version %d (0x%x)\n", ((data[5]<<8)|data[6]), 
                        ((data[5]<<8)|data[6]));
            }
        }
        seq++;
        return CMD_OK;
    }

    if (sal_strcasecmp(subcmd, "status") == 0) {
        memset(data, PD63000_NULL_DATA, 15);
        data[0] = PD63000_KEY_REQUEST;
        data[1] = seq;
        data[2] = PD63000_SUB_GLOBAL;
        data[3] = PD63000_SUB_STATUS;
        poe_pd_pkt_cksum(data); 
        
        if ((rv = poe_pd_pkt_txrx(unit, fd, data, seq, debug)) < 0) {
            return rv;
        }

        if (data[0] == PD63000_KEY_TELEMETRY) {
            cli_out("%s%s",(data[2] & 0x1) ? "CPU error; " : ""
                    ,(data[2] & 0x2) ? "Programming required; " : "");
            cli_out("%s%s",(data[3] & 0x1) ? "EEPROM error; " : ""
                    ,((data[3] & 0x2) && (data[8] != 0xfe)) ? "ASIC error; " : "");
            cli_out("%s",(data[4] & 0x1) ? "Factory default set; " : "");
            cli_out("%s",(data[5] != 0) ? "Internal error; " : "");
            if ((data[2]|data[3]|data[4]|data[5]) && (data[8] != 0xfe)) {
              cli_out("\n");
              cli_out("ASIC fail = 0x%x\n",data[8]);
            } else {
              if (verbose)
              cli_out("No system status error.\n");
            }
            if (verbose) {
                cli_out("General use = %d\n",data[6]);
                cli_out("User byte = 0x%x\n",data[7]);
            }
        }
        seq++;
        return CMD_OK;
    }

    if (sal_strcasecmp(subcmd, "power") == 0) {
        if ((s = ARG_GET(a)) == NULL) {
            memset(data, PD63000_NULL_DATA, 15);
            data[0] = PD63000_KEY_REQUEST;
            data[1] = seq;
            data[2] = PD63000_SUB_GLOBAL;
            data[3] = PD63000_SUB_SUPPLY;
            data[4] = PD63000_SUB_SUPPLY1;
            poe_pd_pkt_cksum(data);
            
            if ((rv = poe_pd_pkt_txrx(unit, fd, data, seq, debug)) < 0) {
                return rv;
            }

            if (data[0] == PD63000_KEY_TELEMETRY) {
                if (verbose) {
                    cli_out("Power permitted %d W\n", (data[2]<<8)|data[3]);
                    cli_out("Max Voltage %4d.%01d V\n", 
                            INT_FRAC_1PT((data[4]<<8)|data[5]));
                    cli_out("Min Voltage %4d.%01d V\n", 
                            INT_FRAC_1PT((data[6]<<8)|data[7]));
                }
            }
            seq++;
        } else {
           if (sal_strcasecmp(s, "status") == 0) {
               memset(data, PD63000_NULL_DATA, 15);
               data[0] = PD63000_KEY_REQUEST;
               data[1] = seq;
               data[2] = PD63000_SUB_GLOBAL;
               data[3] = PD63000_SUB_SUPPLY;
               data[4] = PD63000_SUB_MAIN;
               poe_pd_pkt_cksum(data);
            
               if ((rv = poe_pd_pkt_txrx(unit, fd, data, seq, debug)) < 0) {
                   return rv;
               }

               if (data[0] == PD63000_KEY_TELEMETRY) {
                   if (verbose) {
                       cli_out("Power Consumption %d W\n", (data[2]<<8)|data[3]);
                       cli_out("Max Shutdown Voltage %4d.%01d V\n", 
                               INT_FRAC_1PT((data[4]<<8)|data[5]));
                       cli_out("Min Shutdown Voltage %4d.%01d V\n", 
                               INT_FRAC_1PT((data[6]<<8)|data[7]));
                       cli_out("Internal Power Loss %d W\n", data[8]);
                       cli_out("Power Source %s\n",(data[9] ? "PS1" : "PS2"));
                   }
               }
               seq++;
           } else {
               int i2c_poe_power;

               i2c_poe_power = soc_property_get(unit, spn_I2C_POE_POWER, 0);
               /* set power source 1 */
               memset(data, PD63000_NULL_DATA, 15);
               data[0] = PD63000_KEY_COMMAND;
               data[1] = seq;
               data[2] = PD63000_SUB_GLOBAL;
               data[3] = PD63000_SUB_SUPPLY;
               data[4] = PD63000_SUB_SUPPLY1;
               if (i2c_poe_power) {
                   data[7] = 0x2;
                   data[8] = 0x39;
                   data[9] = 0x1;
                   data[10] = 0xb7;
                   data[11] = 0x13;
               } else {
                   data[7] = 0x2;
                   data[8] = 0x49;
                   data[9] = 0x1;
                   data[10] = 0xb8;
                   data[11] = 0x1;
               }
               val = parse_integer(s);
               data[5] = val>>8;
               data[6] = val&0xff;
               poe_pd_pkt_cksum(data);
            
               if ((rv = poe_pd_pkt_txrx(unit, fd, data, seq, debug)) < 0) {
                   return rv;
               }

               if (data[0] == PD63000_KEY_REPORT) {
                   val = (data[2] << 8) | data[3];
                   if (val) {
                       cli_out("Return error 0x%x\n", val);
                       if ((val > 0) && (val <= 0x7fff)) {
                           cli_out("\tFailed execution (conflict in subject bytes).\n");
                       }
                       if ((val > 0x8000) && (val <= 0x8fff)) {
                           cli_out("\tFailed execution (wrong data byte value).\n");
                       }
                       if (val == 0xffff) {
                           cli_out("\tFailed execution (undefined key value).\n");
                       }
                   } else {
                       if (verbose) 
                           cli_out("Command received and correctly executed.\n");
                   }
               }
               seq++;
           }
        }
        return CMD_OK;
    }

    if (sal_strcasecmp(subcmd, "port") == 0) {
        if ((s = ARG_GET(a)) != NULL) {
            if (sal_strcasecmp(s, "*") == 0) {
                port = 24;
            } else {
                port = 1;
            }

            for (i = 0; i < port; i++) {
                 memset(data, PD63000_NULL_DATA, 15);
                 data[0] = PD63000_KEY_REQUEST;
                 data[1] = seq;
                 data[2] = PD63000_SUB_CHANNEL;
                 data[3] = PD63000_SUB_PORTSTATUS;
                 if (port == 1)
                     data[4] = parse_integer(s);
                 else
                    data[4] = i;
                 if (data[4] > 24) {
                     cli_out("Channel number:  0 ~ 23.\n");
                     return CMD_FAIL;
                 }
                 poe_pd_pkt_cksum(data);
            
                 if ((rv = poe_pd_pkt_txrx(unit, fd, data, seq, debug)) < 0) {
                     return rv;
                 }

                 if (data[0] == PD63000_KEY_TELEMETRY) {
                     if (port != 1 && verbose)
                         cli_out("Port %d:\n", i);
                     if (verbose)
                         cli_out("Port %s\n", (data[2] ? "enable" : "disable"));
                     s = poe_pd_decode_port_status(data[3]);
                     if (verbose) {
                         cli_out("Actual port status %s\n",s);
                         cli_out("Port in %s\n",(data[4] ? "test mode" : "auto mode"));
                         cli_out("Latch 0x%x\n",data[5]);
                         cli_out("Class %d\n",data[6]);
                     }
                 }
                 seq++;
               }
        } else {
            return CMD_USAGE;
        }
        return CMD_OK;
    }

    if (sal_strcasecmp(subcmd, "measure") == 0) {
        if ((s = ARG_GET(a)) != NULL) {
            if (sal_strcasecmp(s, "*") == 0) {
                port = 24;
            } else { 
                port = 1;
            }

            for (i = 0; i < port; i++) {
                memset(data, PD63000_NULL_DATA, 15);
                data[0] = PD63000_KEY_REQUEST;
                data[1] = seq;
                data[2] = PD63000_SUB_CHANNEL;
                data[3] = PD63000_SUB_PARAMZ;
                if (port == 1)
                    data[4] = parse_integer(s);
                else
                   data[4] = i;
                if (data[4] > 24) {
                    cli_out("Channel number:  0 ~ 23.\n");
                    return CMD_FAIL;
                }
                poe_pd_pkt_cksum(data); 
             
                if ((rv = poe_pd_pkt_txrx(unit, fd, data, seq, debug)) < 0) {
                     return rv;
                }

                if (data[0] == PD63000_KEY_TELEMETRY) {
                    if (port != 1 && verbose)
                        cli_out("Port %d:\n", i);
                    if (verbose) {
                        cli_out("Voltage = %4d.%01d V\t", 
                                INT_FRAC_1PT((data[2]<<8)|data[3]));
                        cli_out("Current = %d mA\t\t", (data[4]<<8)|data[5]);
                        cli_out("Power = %4d.%03d W\n",
                                INT_FRAC_3PT((data[6]<<8)|data[7]));
                    }
                }
                seq++;
            }
         } else {
            return CMD_USAGE;
         }
        return CMD_OK;
    }

    if (sal_strcasecmp(subcmd, "raw") == 0) {
        for (i = 0; i < 15; i++) {
             if ((s = ARG_GET(a)) == NULL) {
                  if (i == 13)  {
                      no_cksum = 1;
                      break;
                  } else {
                      cli_out("Not enough data values (have %d, "
                              "need 15 or 13 (for auto check-sum))\n", i);
                      return CMD_FAIL;
                  }
             }
             data[i] = parse_integer(s);
        }     

        if (no_cksum) {
            poe_pd_pkt_cksum(data);
        }

        echo = data[1];
        if ((rv = poe_pd_pkt_txrx(unit, fd, data, echo, 1)) < 0) {
            return rv;
        }

        return CMD_OK;
    }

    if (sal_strcasecmp(subcmd, "read") == 0) {
    if ((rv = bcm_i2c_read(unit, fd, 0, data, &len)) < 0){
        cli_out("ERROR: read byte failed: %s\n",
                bcm_errmsg(rv));
        return CMD_FAIL;
    }
        poe_pd_pkt_dump(data, 15, "Rx from PD63000");
        return CMD_OK;
    }

    return CMD_USAGE;
} /* end cmd_xpoe() */

/*
 * Pretty-print a byte in binary format.
 */
char *
get_bits(uint8 byte)
{
    static char buf[12];
    int i;
    memset(buf,0x0,12);
    strcat(buf,"0b");
    for(i = 7; i >= 0;i--){
    if( byte & (1 << i)){
        cli_out("bit %d set\n",i);
        strcat(buf,"1");
    }
    else    
        strcat(buf,"0");
    }
    return buf;
}
    
#if defined(SHADOW_SVK) || defined (BCM_CALADAN3_SVK)
#define I2C_MUX_F_OPEN    (1 << 0)
#define I2C_MUX_F_MAP     (1 << 1)   /* MUX must be mapped to bit */
                                     /* corresponding to channel  */
                             
STATIC struct mux_ctrl {
    const char *mux_name;
    uint32 flags;
} i2c_mux_info;

STATIC
int _i2c_mux_info_set(int unit, const char *name, uint32 flags)
{
    i2c_mux_info.mux_name = name;
    i2c_mux_info.flags |= flags;
    return 0;
}

STATIC 
int _i2c_mux_info_get(int unit, const char **name, uint32 *flags)
{
    int rv = 0;

    if (i2c_mux_info.flags & I2C_MUX_F_OPEN) {
        *name = i2c_mux_info.mux_name;
        *flags = i2c_mux_info.flags;
    } else {
        rv = -1;
    }
    return rv;
}
  
/*
 * On most boards, there is a single MUX accessed with I2C_LPT_0 
 * (PCF8574, address 0x20). However, some boards can have multiple MUXes
 * implemented with PCA9548 parts. The Shadow SVKs have
 * two MUXes: I2C_MUX_6, address 0x76, and I2C_MUX_7, address 0x77.
 * This function should be called by all code that programs 
 * the MUX to access devices behind the MUX, instead of assuming
 * I2C_LPT_0 is always the MUX.
 * 
 * Upon success,  this function returns the descriptor for the mux 
 * and the MUX name 
 */

int _i2c_mux_open(int unit, uint32 flags, int speed, 
                  char *name)
{
    int fd = -1;

    fd = bcm_i2c_open(unit, name, flags, speed);
    if (fd >= 0) {
        if (sal_strncmp(name, "mux", 3)==0) {
            _i2c_mux_info_set(unit, name, I2C_MUX_F_OPEN|I2C_MUX_F_MAP);
        } else {
            _i2c_mux_info_set(unit, name, I2C_MUX_F_OPEN);
        }
    } 
    return fd;
}


uint8 _i2c_mux_channel_get(int unit, int mux)
{
    int rv;
    const char *name;
    uint8 channel;
    uint32 flags;
    
    channel = mux;
    rv = _i2c_mux_info_get(unit, &name, &flags);
    if (rv == 0) {
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "name = %s flags %x\n"), name, flags));
        if (flags & I2C_MUX_F_MAP) {
            channel = (1 << mux);
        }
    }
    
    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "mux %d -> channel %d\n"), mux, channel));
    return channel;
}


#endif /* SHADOW_SVK */

/*
 * Use lpt0 (PCF8574 Parallel port) to select a MUX line
 * for multi-clock and multi-dac boards. Some boards have
 * multiple devices at the same slave address, the lpt0 outputs
 * will function in this case as a chip-select.
 */
char cmd_muxsel_usage[] =
    "Usages:\n\t"
    "       muxsel <mux_device> [<channel>]\n\t"
    "           - show muxsel, if channel specified, enable that channel.\n\t"
    "           - mux_device = lpt0, mux0, mux1\n\t"
    "\n";

cmd_result_t
cmd_muxsel(int unit, args_t *a)
{
    char *dataByte;
    int fd, rv = SOC_E_NONE;    
    uint32 len;
    uint8 lptVal;
    char * dev_name;

    dev_name = ARG_GET(a);
    dataByte = ARG_GET(a);
    if (! sh_check_attached(ARG_CMD(a), unit))
    return CMD_FAIL;

#ifdef SHADOW_SVK
    if ((dev_name == NULL) || (sal_strncmp(dev_name, "mux6", 4) != 0) || 
        (sal_strncmp(dev_name, "mux7", 4) != 0)) {
        cli_out("Valid devices are mux6, mux7 \n");
        return CMD_FAIL;
    }
    
    if ( (fd = _i2c_mux_open(unit, 0, 0, dev_name)) < 0) {
 	cli_out("%s: cannot open I2C device %s: %s\n",
                ARG_CMD(a), dev_name, bcm_errmsg(fd));
	return CMD_FAIL;
    }
#else
    if (dev_name == NULL) {
        dev_name = _i2c_mux_default_dev_name_get();
    }
   if ( (fd = bcm_i2c_open(unit, dev_name, 0,0)) < 0) {
    cli_out("%s: cannot open I2C device %s: %s\n",
            ARG_CMD(a), dev_name, bcm_errmsg(fd));
    return CMD_FAIL;
    }

#endif
    if (! dataByte ){
    /* Show LPT bits */
    if( (rv = bcm_i2c_read(unit, fd, 0, &lptVal, &len)) < 0){
        cli_out("ERROR: read byte failed: %s\n",
                bcm_errmsg(rv));
        return CMD_FAIL;
    }
    cli_out("Read I2C MuxSel = 0x%x (%s)\n", lptVal, get_bits(lptVal));
    } else {
    /* Write LPT bits */
    lptVal = (uint8)parse_integer(dataByte);
    if( (rv = bcm_i2c_write(unit, fd, 0, &lptVal, 1)) < 0){
        cli_out("ERROR: write byte failed: %s\n",
                bcm_errmsg(rv));
        return CMD_FAIL;
    }
    
    cli_out("Write I2C MuxSel = 0x%x (%s)\n", lptVal, get_bits(lptVal));
    }
    return CMD_OK ;
}

/*
 * Use lpt2 (PCF8574 Parallel port) to select a MUX line
 * for multi-clock.
 */
char cmd_hclksel_usage[] =
    "Usages:\n\t"
    "       hclksel [value]\n\t"
    "           - show hclksel, if value specified, set to that value.\n\t"
    "\n";

cmd_result_t
cmd_hclksel(int unit, args_t *a)
{
    char *dataByte = ARG_GET(a);
    int fd, rv = SOC_E_NONE;
    uint32 len;
    uint8 lptVal;

    if (! sh_check_attached(ARG_CMD(a), unit))
        return CMD_FAIL;

    if ( (fd = bcm_i2c_open(unit, I2C_LPT_2, 0,0)) < 0) {
        cli_out("%s: cannot open I2C device %s: %s\n",
                ARG_CMD(a), I2C_LPT_2, bcm_errmsg(fd));
        return CMD_FAIL;
    }

    if (! dataByte ){
        /* Show LPT bits */
        if( (rv = bcm_i2c_read(unit, fd, 0, &lptVal, &len)) < 0){
            cli_out("ERROR: read byte failed: %s\n",
                    bcm_errmsg(rv));
            return CMD_FAIL;
        }
        cli_out("Read I2C HClkSel = 0x%x (%s)\n", lptVal, get_bits(lptVal));
    } else {
        /* Write LPT bits */
        lptVal = (uint8)parse_integer(dataByte);
        if( (rv = bcm_i2c_write(unit, fd, 0, &lptVal, 1)) < 0){
            cli_out("ERROR: write byte failed: %s\n",
                    bcm_errmsg(rv));
            return CMD_FAIL;
        }

        cli_out("Write I2C HClkSel = 0x%x (%s)\n", lptVal, get_bits(lptVal));
    }
    return CMD_OK ;
}

/*
 * Use lpt3 (PCF8574 Parallel port) for PoE I2C expander.
 */
char cmd_poesel_usage[] =
    "Usages:\n\t"
    "       poesel enable\n\t"
    "            - Enable POE controller.\n\t"
    "       poesel disble\n\t"
    "            - Disable POE controller.\n\t"
    "       poesel reset\n\t"
    "            - Reset POE controller.\n\t"
    "       poesel [value]  \n\t"
    "            - show poesel, if value specified, set to that value.\n\t"
    "\n";

cmd_result_t
cmd_poesel(int unit, args_t *a)
{
    char *subcmd;
    int subcmd_s = 0;
    int fd, rv = SOC_E_NONE;
    uint32 len;
    uint8 lptVal;
#define I2C_POE_ENABLE    1
#define I2C_POE_DISABLE   2
#define I2C_POE_RESET     3

    if (! sh_check_attached(ARG_CMD(a), unit))
        return CMD_FAIL;

    if ( (fd = bcm_i2c_open(unit, I2C_LPT_3, 0,0)) < 0) {
        cli_out("%s: cannot open I2C device %s: %s\n",
                ARG_CMD(a), I2C_LPT_3, bcm_errmsg(fd));
        return CMD_FAIL;
    }

    if ((subcmd = ARG_GET(a)) == NULL) {
        if( (rv = bcm_i2c_read(unit, fd, 0, &lptVal, &len)) < 0){
            cli_out("ERROR: read byte failed: %s\n",
                    bcm_errmsg(rv));
            return CMD_FAIL;
        }
        cli_out("Read I2C PoeSel = 0x%x (%s)\n", lptVal, get_bits(lptVal));
        return CMD_OK ;
    }

    if (sal_strcasecmp(subcmd, "enable") == 0) {
        subcmd_s = I2C_POE_ENABLE;
    } else if (sal_strcasecmp(subcmd, "disable") == 0) {
        subcmd_s = I2C_POE_DISABLE;
    } else if (sal_strcasecmp(subcmd, "reset") == 0) {
        subcmd_s = I2C_POE_RESET;
    } else {
        lptVal = (uint8)parse_integer(subcmd);
    }

    /*    coverity[new_values]    */
    if (subcmd_s > 0) {
        switch (subcmd_s) {
        case I2C_POE_ENABLE:
            lptVal = 0x8c;
            break;
        case I2C_POE_DISABLE:
            lptVal = 0x8e;
            break;
        case I2C_POE_RESET:
            lptVal = 0x8d;
            break;
        default:
            return CMD_USAGE;
        }
    }

    if ((rv = bcm_i2c_write(unit, fd, 0, &lptVal, 1)) < 0){
        cli_out("ERROR: write byte failed: %s\n",
                bcm_errmsg(rv));
        return CMD_FAIL;
    }

    if (subcmd_s == I2C_POE_RESET) {
        sal_sleep(5);
    }

    if (subcmd_s == 0) {
        cli_out("Write I2C PoeSel = 0x%x (%s)\n", lptVal, get_bits(lptVal));
    }

    return CMD_OK ;
}



#ifdef BCM_ESW_SUPPORT
#define SYN_DELAY 200000
/*
 * Use lpt4 and lpt5 (PCF8574 Parallel port) to select a 
 * synthesizer frequency.
 */
char cmd_synth_usage[] =
    "\n\t"
    "synth [value]\n\t"
    "   - show synth freq, if value specified, set frequency.\n\t"
    "\n";

cmd_result_t
cmd_synth(int unit, args_t *a)
{
#ifdef COMPILER_HAS_DOUBLE
    char *value = ARG_GET(a);
    char *syndbg = ARG_GET(a);
    double cur_freq, freq, m_div, n_div;
    double n_div_map[8] = { 1, 1.5, 2, 3, 4, 6, 8, 12 };
    double freq_min[8] = { 250.0, 166.7, 125.0,  83.3,  62.5, 41.7, 31.3, 20.8 };
    double freq_max[8] = { 500.0, 333.3, 250.0, 166.7, 125.0, 83.3, 62.5, 41.7 };
    int adj[8] = { 9, 6, 3, 3, 2, 1, 1, 1 };
    int rv, sel1, sel2, m, n, cur_n, inc;
    uint8 lptVal;
    uint32 reg, len;
    char *str;

    if (!sh_check_attached(ARG_CMD(a), unit))
        return CMD_FAIL;

    if ((sel1 = bcm_i2c_open(unit, I2C_LPT_4, 0,0)) < 0) {
        cli_out("%s: cannot open I2C device %s: %s\n",
                ARG_CMD(a), I2C_LPT_4, bcm_errmsg(sel1));
        return CMD_FAIL;
    }

    if ((sel2 = bcm_i2c_open(unit, I2C_LPT_5, 0,0)) < 0) {
        cli_out("%s: cannot open I2C device %s: %s\n",
                ARG_CMD(a), I2C_LPT_5, bcm_errmsg(sel2));
        return CMD_FAIL;
    }

    /* Read current frequency */
    if ((rv = bcm_i2c_read(unit, sel1, 0, &lptVal, &len)) < 0) {
        cli_out("ERROR: read byte failed: %s\n",
                bcm_errmsg(rv));
        return CMD_FAIL;
    }
    m = lptVal;
    if ((rv = bcm_i2c_read(unit, sel2, 0, &lptVal, &len)) < 0) {
        cli_out("ERROR: read byte failed: %s\n",
                bcm_errmsg(rv));
        return CMD_FAIL;
    }
    if (lptVal & 0x80) m |= 0x100;
    n = lptVal & 0x7;
    m_div = m;
    n_div = n_div_map[n];
    cur_freq = m_div / n_div;

    if (!value) {
        /* Show frequency */
        freq = cur_freq;
        str = "";
    } else {
        /* Set frequency */
        freq = atof(value);
        if (freq > freq_max[0] || freq < freq_min[7]) {
            cli_out("%s: valid frequency range: %.1f MHz to %.1f MHz\n",
                    ARG_CMD(a), freq_min[7], freq_max[0]);
            return CMD_FAIL;
        }
        if (syndbg) {
            cli_out("Change from %.1f MHz to %.1f MHz\n",
                    cur_freq, freq);
        }

        /*
         * Since the I2C clock is derived from the core clock and the
         * I2C access only works within a certain frequency range, we
         * need to take precautions when changing the core clock in
         * order not to lock ourselves out from the I2C bus. This
         * means that for large changes in core clock frequency we
         * will have to change the core clock in smaller steps and
         * adjust the CMIC divider in between the steps.
         */
        for (cur_n = 0; cur_n < 8; cur_n++) {
            if (cur_freq >= freq_min[cur_n]) {
                break;
            }
        }
        for (n = 0; n < 8; n++) {
            if (freq >= freq_min[n]) {
                break;
            }
        }

        if (cur_n >= 8 || n >= 8) {
            /* should never get here */
            cli_out("ERROR: internal error\n");
            return CMD_FAIL;
        }

        inc = cur_n > n ? -1 : 1;
        do {
            if (n == cur_n) {
                cur_freq = freq;
                inc = 0;
            } else {
              if((cur_n+inc) < 0 || (cur_n+inc) > 7) {
                  cli_out("ERROR: Internal error\n");
                  return CMD_FAIL;
    /*    coverity[overrun-local : FALSE]    */
              } else {
    /* coverity[overrun-local] */
                  cur_freq = (inc < 0) ? freq_min[cur_n+inc] : freq_max[cur_n+inc];
              }
            }
            if (syndbg) {
                cli_out("Adjust to %.1f/%d\n",
                        cur_freq, adj[cur_n+inc]);
            }
            n_div = n_div_map[cur_n+inc];
            m = cur_freq * n_div;
            lptVal = (uint8)(m & 0xff);
            if ((rv = bcm_i2c_write(unit, sel1, 0, &lptVal, 1)) < 0) {
                cli_out("ERROR: write byte failed: %s\n",
                        bcm_errmsg(rv));
                return CMD_FAIL;
            }
            sal_usleep(SYN_DELAY);
            lptVal = (uint8)cur_n;
            if (m & 0x100) lptVal |= 0x80;
            if ((rv = bcm_i2c_write(unit, sel2, 0, &lptVal, 1)) < 0) {
                cli_out("ERROR: write byte failed: %s\n",
                        bcm_errmsg(rv));
                return CMD_FAIL;
            }
            sal_usleep(SYN_DELAY);
            READ_CMIC_RATE_ADJUSTr(unit, &reg);
            soc_reg_field_set(unit, CMIC_RATE_ADJUSTr, &reg, 
                              DIVISORf, adj[cur_n]);
            WRITE_CMIC_RATE_ADJUSTr(unit, reg);
            sal_usleep(SYN_DELAY);
            if (n == cur_n) {
                break;
            }
            cur_n = cur_n + inc;
            if (cur_n < 0 || cur_n > 7) {
                cli_out("ERROR: Internal error\n");
                return CMD_FAIL;
            }
        } while (1);
        /* Dummy read */
        if ((rv = bcm_i2c_read(unit, sel1, 0, &lptVal, &len)) < 0) {
            cli_out("ERROR: dummy read byte failed: %s\n",
                    bcm_errmsg(rv));
            return CMD_FAIL;
        }
        str = "Write I2C ";
    }
    cli_out("%sSynth Freq = %.1f MHz", str, freq);
    cli_out("  [ 0%c%c%c%c%c%c%c%c%c 0%c%c%c ]\n",
            m & 0x100 ? '1' : '0', 
            m & 0x080 ? '1' : '0', 
            m & 0x040 ? '1' : '0', 
            m & 0x020 ? '1' : '0', 
            m & 0x010 ? '1' : '0', 
            m & 0x008 ? '1' : '0', 
            m & 0x004 ? '1' : '0', 
            m & 0x002 ? '1' : '0', 
            m & 0x001 ? '1' : '0', 
            n & 0x4 ? '1' : '0', 
            n & 0x2 ? '1' : '0', 
            n & 0x1 ? '1' : '0');
#endif

    return CMD_OK ;
}

/*
 * Use lpt6 (PCF8574 Parallel port) to set PPD clock delay.
 */
char cmd_ppdclk_usage[] =
    "\n\t"
    "ppdclk\n\t"
    "   - show PPD clock delay and core clock divider\n\t"
    "ppdclk delay value\n\t"
    "   - set PPD clock delay in ps (0-1270)\n\t"
    "ppdclk div value\n\t"
    "   - set core clock divider enable (0 or 1)\n\t"
    "\n";

cmd_result_t
cmd_ppdclk(int unit, args_t *a)
{
    char *source = ARG_GET(a);
    char *value = ARG_GET(a);
    int fd, fd2, rv = SOC_E_NONE;
    uint32 len, ppd_delay;
    uint8 lptVal, lptVal2;

    if (! sh_check_attached(ARG_CMD(a), unit))
        return CMD_FAIL;

    if ((fd = bcm_i2c_open(unit, I2C_LPT_6, 0,0)) < 0) {
        cli_out("%s: cannot open I2C device %s: %s\n",
                ARG_CMD(a), I2C_LPT_6, bcm_errmsg(fd));
        return CMD_FAIL;
    }

    if( (rv = bcm_i2c_read(unit, fd, 0, &lptVal, &len)) < 0){
        cli_out("ERROR: read byte failed: %s\n",
                bcm_errmsg(rv));
        return CMD_FAIL;
    }

    if ((fd2 = bcm_i2c_open(unit, I2C_LPT_7, 0,0)) < 0) {
        cli_out("%s: cannot open I2C device %s: %s\n",
                ARG_CMD(a), I2C_LPT_6, bcm_errmsg(fd));
        return CMD_FAIL;
    }

    if( (rv = bcm_i2c_read(unit, fd2, 0, &lptVal2, &len)) < 0){
        cli_out("ERROR: read byte failed: %s\n",
                bcm_errmsg(rv));
        return CMD_FAIL;
    }

    if (!source){
        /* Show PPD clock delay and core clock divider */
        ppd_delay = lptVal2 & 0x03;
        ppd_delay <<= 8;
        ppd_delay |= lptVal;
        cli_out("PPD Clock Delay    = %d0 ps\n", ppd_delay);
        cli_out("Core Clock Divider = %sabled\n", 
                lptVal2 & 0x80 ? "en" : "dis");
    } else if (!sal_strcmp(source, "delay") && value) {
        /* Write PPD clock delay */
        ppd_delay = parse_integer(value) / 10;
        lptVal = (uint8)(ppd_delay & 0xff);
        lptVal2 &= ~0x03;
        lptVal2 |= (uint8)(ppd_delay >> 8);
        if( (rv = bcm_i2c_write(unit, fd, 0, &lptVal, 1)) < 0){
            cli_out("ERROR: write byte failed: %s\n",
                    bcm_errmsg(rv));
            return CMD_FAIL;
        }
        if( (rv = bcm_i2c_write(unit, fd2, 0, &lptVal2, 1)) < 0){
            cli_out("ERROR: write byte failed: %s\n",
                    bcm_errmsg(rv));
            return CMD_FAIL;
        }
        cli_out("Write I2C PPD Clock Delay = %d0 ps\n", ppd_delay);
    } else if (!sal_strcmp(source, "div") && value) {
        /* Enable/disable core clock divider */
        lptVal &= ~0x80;
        if (parse_integer(value)) lptVal |= 0x80;
        if( (rv = bcm_i2c_write(unit, fd, 0, &lptVal, 1)) < 0){
            cli_out("ERROR: write byte failed: %s\n",
                    bcm_errmsg(rv));
            return CMD_FAIL;
        }
        cli_out("Write I2C Core Clock Divider = %sabled\n", 
                lptVal & 0x80 ? "en" : "dis");
    } else {
        return CMD_USAGE ;
    }
    return CMD_OK ;
}
#endif /* BCM_ESW_SUPPORT */

/*
 * Configure a DAC chip.
 */
char cmd_dac_usage[] =
    "Usages:\n\t"
    "       dac [chipnum] [value]\n\t"
    "           - set DAC chip number to specified value.\n\t"
    "\n";

cmd_result_t
cmd_dac(int unit, args_t *a)
{


    int fd, num, rv = SOC_E_NONE;    
    uint16 dacVal;
    char dev[25];
    char *chip = ARG_GET(a);
    char *dataWord = ARG_GET(a);
    
    if (! sh_check_attached(ARG_CMD(a), unit))
    return CMD_FAIL;

    if ( (! dataWord) || (!chip) )
    return CMD_USAGE ;

    /* Select ADC chip number */
    num = parse_integer(chip);
    sprintf(dev,"%s%d","dac",num);

    dacVal = (uint16)parse_integer(dataWord);
    
    if ( (fd = bcm_i2c_open(unit, dev, 0, 0)) < 0) {
    cli_out("%s: cannot open I2C device %s: %s\n",
            ARG_CMD(a), dev, bcm_errmsg(fd));
    return CMD_FAIL;
    }
    /* Write command and data bytes to DAC */
    if( (rv = bcm_i2c_write(unit, fd, 0, (uint8*)&dacVal, 2)) < 0){
    cli_out("ERROR:write of DAC word failed:%s\n",
            bcm_errmsg(rv));
    return CMD_FAIL;
    }        
    
    return CMD_OK ;
}


/*
 * Use Matrix Orbital LCD to display messages.
 */
char cmd_lcdmsg_usage[] =
    "Usages:\n\t"
    "       lcdmsg [string]\n\t"
    "           - print string on lcd\n\t"
    "       lcdmsg -x [0-255]\n\t"
    "           - set contrast (0-255)\n\t"
    "       lcdmsg -c\n\t"
    "           - clear screen\n\t"
    "       lcdmsg -n\n\t"
    "           - scroll down one line\n\t"
"\n";

cmd_result_t
cmd_lcdmsg(int unit, args_t *a)
{
    char *msg = ARG_GET(a);
    char *dat = ARG_GET(a);
    uint8 db;
    int fd, rv = SOC_E_NONE;    

    if( ! msg)
    return CMD_USAGE;
    
    if (! sh_check_attached(ARG_CMD(a), unit))
    return CMD_FAIL;

    if ( (fd = bcm_i2c_open(unit, I2C_LCD_0, 0,0)) < 0) {
    cli_out("%s: cannot open I2C device %s: %s\n",
            ARG_CMD(a), I2C_LCD_0, bcm_errmsg(fd));
    return CMD_FAIL;
    }    
    /* Echo a newline */
    if(!sal_strncasecmp(msg, "-n", 2)){
    if( (rv = bcm_i2c_write(unit, fd, 0, (uint8 *)"\n",1)) < 0){
        cli_out("ERROR: I2C write failed: %s\n",
                bcm_errmsg(rv));
        return CMD_FAIL;
    }
    }
    /* Set contrast level */
    else if(!sal_strncasecmp(msg,"-x",2)){
    if(!dat)
        return CMD_USAGE;

    db = (uint8) parse_integer(dat);

    if( (rv = bcm_i2c_ioctl(unit, fd, I2C_LCD_CONTRAST,
                (void*)&db,1)) < 0){
        cli_out("ERROR: I2C ioctl failed: %s\n",
                bcm_errmsg(rv));
        return CMD_FAIL;
    }
    }
    /* Clear screen */
    else if(!sal_strncasecmp(msg, "-c",2)){
    if( (rv = bcm_i2c_ioctl(unit, fd, I2C_LCD_CLS, 0,0)) < 0){
        cli_out("ERROR: I2C ioctl failed: %s\n",
                bcm_errmsg(rv));
        return CMD_FAIL;
    }
    }
    else {
    /* Display text */
    if( (rv = bcm_i2c_write(unit, fd, 0, (uint8 *)msg, sal_strlen(msg))) < 0){
        cli_out("ERROR: I2C ioctl failed: %s\n",
                bcm_errmsg(rv));
        return CMD_FAIL;
    }
    }
    return CMD_OK ;
}


/*
 * XGS/StrataSwitch board level support.
 * On Broadcom SDK Systems, the internal I2C controller is used to
 * determine board configuration and configurable options.
 *
 * Some boards have configurable options, other's don't and for
 * those which do, we provide additional support for the SDK
 * with the following tools:
 *
 * G1 - I2C NVRAM for SAL configuration, I2C Temperature monitoring,
 *      W229n clock speed configuration (Core).
 * G2 - All of G1, plus configurable Core Voltage, and PCI, SDRAM amd
 *      core clocks.
 * G3 - All of G1, pluse Turbo clock, Power, Temperature, and Thickness.
 *      In addition, PHY and 2.5V power supplies configurable.
 *
 * See also: "baseboard" or "bb" command.
 */
/*
 * NOTE: To add new boards, edit board_type definition above
 * and be certain that this is used for indexing to work
 * (these tables commented as "INDEXED_BY_BOARD_TYPE").
 */

/*
 * Board identifiers.
 */
typedef enum{
    G5,     /* BCM5670 */
    G13,    /* BCM56504P24_00 */
    G15,    /* BCM56580K16 */
    G16,    /* BCM56800K20C */
    G17,    /* BCM56600K20 */
    G18,    /* BCM56218k48 */
    G19,    /* BCM56224k24 */
    G20,    /* BCM56624K49S, BCM56680K24TS, BCM56628K8XG, BCM56624K49DV */
    G21,    /* BCM56820K24XG, BCM56820K24C, BCM56820K24DV */ 
    G22,    /* BCM56524/BCM56685 */
    G23,    /* BCM88230 */
    G24,    /* BCM56840 */
    G25,    /* BCM56740 */
    G26,    /* BCM56142 */
    G27,    /* BCM88732 */
    G28,    /* BCM56440, BCM56150, BCM56340 */
    G29,    /* BCM56445K */
    G30,    /* BCM56444 */
    G31,    /* BCM56445D */
    G33,    /* BCM56844 */
    G34,    /* BCM56644K */
    G35,    /* BCM56640K */
    G36,    /* BCM988030K */
    G37,    /* BCM988030KC */
    G38,    /* BCM56850K */
    G39,    /* BCM56850D */
    G40,    /* BCM56450k, BCM56450d, BCM56456k, BCM56456d */
    ENG,    /* Special */
    UNK     /* Unknown */
} board_type_t;

/* Board information structure */

typedef struct
bb_type_s{
    int  type;
    int  bbcmd_flags; /* Supported baseboard commands */
    char *name_text;
} bb_type_t;

static char *BaseboardName = NULL;

/* Defines for bbcmd_flags field */
#define BB_NONE 0x00000000
#define BB_CLK  0x00000001
#define BB_VOLT 0x00000002
#define BB_PWR  0x00000004
#define BB_CURR 0x00000008

/* Currently supported platforms 
 * INDEXED_BY_BOARD_TYPE */

bb_type_t
baseboards[] = {
    { G5,  BB_CLK | BB_VOLT | BB_PWR,
     "XGS Hercules BCM5670_P8_00 Platform" },
    { G13, BB_CLK | BB_VOLT | BB_PWR,
     "XGS Firebolt BCM56504P24_00 Platform" },            
    { G15, BB_VOLT | BB_PWR,
     "XGS Goldwing BCM56580K16 Platform" },            
    { G16, BB_VOLT | BB_PWR,
     "XGS Bradley BCM56800K20C Platform" },            
    { G17, BB_VOLT | BB_PWR,
     "XGS Bradley BCM56800K20 Platform" },            
    { G18, BB_VOLT | BB_PWR,
     "XGS raptor BCM56218k48 Platform" },            
    { G19, BB_VOLT,
     "XGS Raven BCM56224k24 Platform" },  
    { G20, BB_CLK | BB_VOLT,
     "XGS Triumph Platform"},
    { G21, BB_CLK | BB_VOLT,
     "XGS Scorpion Platform"},
    { G22, BB_CLK | BB_VOLT,
     "XGS Apollo Platform"},
    { G23, BB_CLK | BB_VOLT,
     "SBX Sirius Platform"},
    { G24, BB_CLK | BB_VOLT,
     "XGS Trident Platform"},
    { G25, BB_CLK | BB_VOLT,
     "XGS Titan Platform"},
    { G26, BB_VOLT,
     "XGS Hurricane Platform"},
    { G27, BB_VOLT,
     "Shadow Platform"},
    { G28, BB_VOLT,
     "XGS Katana / Hurricane2 / Helix4 Platform"},
    { G29, BB_VOLT,
     "XGS Enduro-2 Platform"},
    { G30, BB_VOLT,
     "XGS Stardust-2 Platform"},
    { G31, BB_VOLT,
     "XGS Enduro-2 Platform"},
    { G33, BB_CLK | BB_VOLT,
     "XGS Trident Platform"},
    { G34, BB_VOLT,
     "XGS Triumph3 Legacy Platform"},
    { G35, BB_VOLT,
     "XGS Triumph3 Platform"},
    { G36, BB_VOLT | BB_CURR,
     "Caladan3 48G Platform"},
    { G37, BB_VOLT | BB_CURR,
     "Caladan3 100G Platform"},
    { G38, BB_VOLT,
     "Trident2 Platform"},
    { G39, BB_VOLT,
     "Trident2 DVT Platform" },
    { G40, BB_VOLT | BB_CURR,
     "Katana2 Platform" },
    { ENG, BB_NONE,
     "ENG Version" },
    { UNK, BB_NONE,
     "No Info or I2C unavailable on this platform" }
};

/* Number of possible board types */
#define NUM_BOARD_TYPES COUNTOF(baseboards)

#define MAX_VS_CONFIG_PER_BOARD 6

/* Max number of clocks per board */
#define MAX_CLOCKS_PER_BOARD 10

/* Max number of voltages per board */
#define MAX_VOLTAGES_PER_BOARD 32

/* Set once at bootup */
static int soc_board_idx = -1;


#ifdef BCM_CALADAN3_SVK

/* Caladan3 revisions */
static int soc_c3_board_id = -1;

#endif


/*
 * Get system baseboard info, either by reading it
 * from EEPROM or by probing the PCI and I2C buses.
 * This routine attempts to determine "board type",
 * which is used subsequently as an index by tables
 * commented with INDEXED_BY_BOARD_TYPE.
 */
bb_type_t*
soc_i2c_detect_board(void)
{
    int bd_idx;
    uint32 id_detected =0;
    uint8 board_id =0;
#ifndef BCM_CALADAN3_SVK
    int id, dev;
    int n_adc=0, n_dac=0, n_pll=0, n_temp=0, n_eeprom=0, n_poe=0;
    i2c_device_t* i2c_dev = NULL;
#endif
    int found = 0, board_type = -1;


#ifdef BCM_CALADAN3_SVK
    /*
     * Newer boards have 9505 id on the Cpu I2C bus
     * Older ones have it on the DUT I2C bus
     * Read the I2c device directly without a probe, otherwise it becomes chicken egg problem
     * I2c Drivers use the board id to determine if the access is via cpu bus or dut bus
     */
    int rv = SOC_E_NONE;
    int bus = cpu_i2c_defaultbus();
    int data = 0;
    /* Read the board ID register */
    cpu_i2c_attach(bus, SOC_I2C_NO_PROBE, CMIC_I2C_SPEED_SLOW_IO);
    rv = cpu_i2c_read(I2C_BRD_ID_IOP_SADDR, I2C_BRD_ID_IOP_PORT, 
                       CPU_I2C_ALEN_BYTE_DLEN_BYTE, &data);
    if (rv == 0) {
        board_id = (uint8)data;
        id_detected = 1;
        soc_c3_board_id = board_id;
    } else {
        
        if (soc_ndev > 0) {
            if (!(SOC_CONTROL(0)->soc_flags & SOC_F_INITED)) {
                cli_out("ERROR: SOC, BCM not initialized yet!\n");
                return NULL;
            }
            rv = soc_i2c_attach(0, SOC_I2C_NO_PROBE, 0);
            if (SOC_SUCCESS(rv)) {
                rv = soc_i2c_read_byte_data(0, I2C_BRD_ID_IOP_SADDR, 
                                            I2C_BRD_ID_IOP_PORT, &board_id);
                if (SOC_SUCCESS(rv)) {
                    id_detected = 1;
                    cli_out("\nDetected Board id :%x, Old revision, masking top bit\n", board_id);
                    soc_c3_board_id = board_id;
                    board_id &= 0x3F;
                    soc_i2c_probe(0);
                } else {
                    return NULL;
                }
            }
        }
    }
    if (id_detected) {
        cpu_i2c_probe(bus);
    }
#else

    dev = 0;
    if (soc_ndev > 0) {
        if (!soc_i2c_is_attached(dev)) {
            if (soc_i2c_attach(dev, 0, 0) < 0) {
                cli_out("unit %d soc_i2c_detect_board: "
                        "error attaching to I2C bus\n",
                        dev);
                return NULL;
            }
        }

        if (soc_i2c_probe(dev) < 0) {
            cli_out("unit %d soc_i2c_detect_board: "
                    "error probing the I2C bus\n",
                    dev);
            return NULL;
        }

        for( id = 0; id < soc_i2c_device_count(dev); id++) {
            i2c_dev = soc_i2c_device(dev, id) ;
            if ( i2c_dev ) {

                /* Board ID register */
                if ( ( i2c_dev->driver->id == PCF8574_DEVICE_TYPE ) &&
                    ( i2c_dev->saddr == I2C_LPT_SADDR1 ) ) {
                        /* id_detected will be set to 1 by the following call (length) */
                        /* Read the board ID register */
                        i2c_dev->driver->read(dev, id, 0, &board_id, &id_detected);
                }
#ifdef SHADOW_SVK
                /*
                 * In Shadow SVK, board id is read from port 1 in PCA9505 
                 * at I2C_IOP_SADDR1 
                 */
                if ( ( i2c_dev->driver->id == PCA9505_DEVICE_TYPE ) &&
                    ( i2c_dev->saddr == I2C_IOP_SADDR1 ) ) {
                    /* Read the board ID register */
                    /* cli_out("reading id from 9505\n"); */
                    i2c_dev->driver->read(dev, id, 0, &board_id, &id_detected);
                }
#endif /* SHADOW_SVK */
                /* ADC */
                if ( i2c_dev->driver->id == MAX127_DEVICE_TYPE){
                    n_adc++;
                }
                /* DAC */
                if ( i2c_dev->driver->id == LTC1427_DEVICE_TYPE){
                    n_dac++;
                }

                /* W229B PLL */
                if ( i2c_dev->driver->id == W229B_DEVICE_TYPE ) {
                    n_pll++;
                }

                /* EEPROM */
                if ( i2c_dev->driver->id == LC24C64_DEVICE_TYPE){
                    n_eeprom++;
                    
                }

                /* LM75 Temp */
                if ( i2c_dev->driver->id == LM75_DEVICE_TYPE ){
                    n_temp++;
                }        

                /* POE controller */
                if ( i2c_dev->driver->id == LTC4258_DEVICE_TYPE ){
                    n_poe++;
                }
            }
        }
    }

    

    /* Here if we did not find any board-specific info in the EEPROM.   */
    /* We know what the soc_ndev devices are, and what exists on the    */
    /* I2C bus. Use this information to identify the hardware platform. */

    /**********************************************************/
    /* Specific, uniquely populated, or valid board_id boards */
    /**********************************************************/
#endif
    if (id_detected && (board_id == 0x26)) {
        /* Firebolt    */
        board_type = G13;
    }
#ifdef BCM_CALADAN3_SVK
    else if (id_detected && (board_id == 0x38)){
        /* Caladan3 48G SVK */
        board_type = G36;
    }
    else if (id_detected && (board_id == 0x39)){
        /* Caladan3 100G SVK */
        board_type = G37;
    }
#endif

#ifdef BCM_RAPTOR_SUPPORT
    else if ((soc_ndev > 0) && SOC_IS_RAPTOR(0)) {
        /* Raptor */
        board_type = G18;
    }

    else if ((soc_ndev > 0) && SOC_IS_RAVEN(0)) {
        /* Raven */
        board_type = G19;
    }
#endif

#ifdef SHADOW_SVK
    else if ((soc_ndev > 0) && SOC_IS_SHADOW(0)) {
        /* Alternatively, can check 
         * if (id_detected && (board_id == (RadianSVK Id)) {
         * 
         * Shadow SVK
         */
        board_type = G27;
    }
#endif /* SHADOW_SVK */

#ifdef BCM_FIREBOLT_SUPPORT
    else if ((soc_ndev > 0) && SOC_IS_FB_FX_HX(0)) { /* Fallback */
        /* Firebolt/Helix/Felix */
        board_type = G13;
    }
#endif
    else if (id_detected && (board_id == 0x34)) {
        /* Goldwing     */
        board_type = G15;
    }

    else if (id_detected && (board_id == 0x39)) {
        /* Bradley 1G   */
        board_type = G16;
    }

    else if (id_detected && (board_id == 0x3a)) {
        /* Bradley      */
        board_type = G17;
    }

    else if (id_detected && (board_id == 0x53)){
        i2c_saddr_t saddr_ss[8] = {0x20, 0x27, 0x28, 0x2c, 0x4c, 0x50};
        /* Sirius has i2c devices at these addresses to differentiate */
        if ((soc_i2c_device_present(0, saddr_ss[0]) == 0) &&
            (soc_i2c_device_present(0, saddr_ss[1]) == 0) && 
            (soc_i2c_device_present(0, saddr_ss[2]) == 0) && 
            (soc_i2c_device_present(0, saddr_ss[3]) == 0) &&
            (soc_i2c_device_present(0, saddr_ss[4]) == 0) &&
            (soc_i2c_device_present(0, saddr_ss[5]) == 0)) {
            /* Sirius */
            board_type = G23;
        } else {
            /* Triumph */
            board_type = G20;
        }
    }

    else if (id_detected && (board_id == 0x52)){
        /* Scorpion    */
        board_type = G21;
    }

    else if (id_detected && (board_id == 0x55 || board_id == 0x49)){
        /* Apollo   */
        board_type = G22;
    }

    else if (id_detected && (board_id == 0x5D)){
        /* Trident */
        board_type = G24;
        /* make sure all I2C devices behind the I2C mux device are initialized */
        if (dac_devs_init(0,G24,I2C_MUX_3) < 0) {
            LOG_ERROR(BSL_LS_APPL_I2C,
                      (BSL_META("ERROR: dac_devs_init fail\n")));
            return NULL; 
        }
    }

    else if (id_detected && (board_id == 0x6A)){
        /* Trident */
        board_type = G33;
        /* make sure all I2C devices behind the I2C mux device are initialized */
        if (dac_devs_init(0,G33,I2C_MUX_3) < 0) {
            LOG_ERROR(BSL_LS_APPL_I2C,
                      (BSL_META("ERROR: dac_devs_init fail\n")));
            return NULL; 
        }
    }

    else if (id_detected && (board_id == 0x60 || board_id == 0x61)){
        /* Titan */
        board_type = G25;
    }

    else if (id_detected && (board_id == 0x5E || board_id == 0x5F)){
        /* Hurricane */
        board_type = G26;
    }

    else if (id_detected && ((board_id == 0x65) || (board_id == 0x67) || (board_id == 0x72))){
        /* Katana / Hurricane2 / Helix4*/
        board_type = G28;
    }

    else if (id_detected && (board_id == 0x64)){
        /* Enduro-2 */
        board_type = G29;
    }

    else if (id_detected && (board_id == 0x68)){
        /* Stardust-2 */
        board_type = G30;
    }

    else if (id_detected && (board_id == 0x6D)){
        /* Enduro-2 */
        board_type = G31;
    }

    else if (id_detected && (board_id == 0x69)){
        /* Triumph3 Legacy */
        board_type = G34;
        /* make sure all I2C devices behind the I2C mux device
         * are initialized */
        if (dac_devs_init(0,G34,I2C_MUX_3) < 0) {
            LOG_ERROR(BSL_LS_APPL_I2C,
                      (BSL_META("ERROR: dac_devs_init fail\n")));
            return NULL; 
        }
    }

    else if (id_detected && (board_id == 0x6e)){
        /* Triumph3 Legacy */
        board_type = G35;
        /* make sure all I2C devices behind the I2C mux device
         * are initialized */
        if (dac_devs_init(0,G35,I2C_MUX_3) < 0) {
            LOG_ERROR(BSL_LS_APPL_I2C,
                      (BSL_META("ERROR: dac_devs_init fail\n")));
            return NULL; 
        }
    }

    else if (id_detected && (board_id == 0x6f)){
        /* Trident2 */
        board_type = G38;
        /* make sure all I2C devices behind the I2C mux device
         * are initialized */
        if (dac_devs_init(0,G38,I2C_MUX_3) < 0) {
            LOG_ERROR(BSL_LS_APPL_I2C,
                      (BSL_META("ERROR: dac_devs_init fail\n")));
            return NULL; 
        }
    }

    else if (id_detected && board_id == 0x70) {
        /* Trident2 DVT */
        board_type = G39;
        /* make sure all I2C devices behind the I2C mux device
         * are initialized */
        if (dac_devs_init(0, G39, I2C_MUX_3) < 0) {
            LOG_ERROR(BSL_LS_APPL_I2C,
                      (BSL_META("ERROR: dac_devs_init fail\n")));
            return NULL;
        }
    }

    else if (id_detected && ((board_id == 0x77) || (board_id == 0x78) ||
                             (board_id == 0x79) || (board_id == 0x80))){
        /* Katana2 Boards */
        board_type = G40;
    }

    /**************************/
    /* Other multi-SOC boards */
    /**************************/

    else if (soc_ndev > 1) {
        /***************************************/
        /* Other unidentified multi-soc boards */
        /***************************************/
        board_type = ENG;
    } /* soc_ndev > 1 */

    /*****************************/
    /* Other (single) SOC boards */
    /*****************************/

    else {
        if (SOC_IS_XGS12_FABRIC(0)) {
            /* Hercules */
            board_type = G5;
        }
        /****************************************/
        /* Other unidentified single-soc boards */
        /****************************************/
        else {
            board_type = ENG;
        }
    } 


    for( bd_idx = 0; bd_idx < NUM_BOARD_TYPES; bd_idx++ ) {
        if (baseboards[bd_idx].type == board_type){
            found=1;
            break;
        }
    }
    /* Probably know Board */
    if ( found ) {
        if (BaseboardName == NULL)
            BaseboardName = baseboards[bd_idx].name_text;
        /* cli_out("Detected: %s\n", BaseboardName); */
        return &baseboards[bd_idx];
    }
    BaseboardName = NULL;
    return NULL;
} /* end soc_i2c_detect_board() */


/*
 * Use I2C to detect the correct type of board */
void
soc_i2c_board_probe(void)
{
    bb_type_t* board = soc_i2c_detect_board();
    if ( board ) {
    soc_board_idx = board->type;
    } else {
    cli_out("NOTICE: board type unknown.\n");
    soc_board_idx = UNK;
    }
}

/*
 * Return number of possible boards.
 */
int
soc_num_boards(void)
{
    return NUM_BOARD_TYPES;
}

/*
 * Possibly probe, but always return correct
 * known index.
 */
int
soc_get_board_id(void)
{
    if ((soc_board_idx < 0) || (soc_board_idx == UNK)) {
      soc_i2c_board_probe();
    }
    return soc_board_idx;
}

#ifdef BCM_CALADAN3_SVK

/*
 * C3 SVK, handle multiple revisions of board
 */
int
soc_caladan3_board_id(void) 
{
    if (soc_c3_board_id < 0) {
        soc_i2c_board_probe();
    }
    return soc_c3_board_id;
}

int
soc_caladan3_is_board_rev_a(void)
{
    if (soc_c3_board_id < 0) {
        soc_i2c_board_probe();
    }
    return (soc_c3_board_id & 0x80);
}

#endif

/*
 * Return raw board info struct.
 */
bb_type_t*
soc_get_board(void)
{
    int inx;
    inx = soc_get_board_id();

    if ((inx < 0) || (inx >= NUM_BOARD_TYPES)) {
        return NULL; 
    }
    return &baseboards[inx];
}


/*
 * Function: cmd_temperature
 * Purpose: Temperature and Temperature monitoring commands.
 */
char cmd_temperature_usage[] =
    "Usages:\n\t"
    "       temperature [show]\n\t"
    "           - show current temperature on all devices.\n\t"
    "       temperature [watch|nowatch] [delay]\n\t"
    "           - monitor temperatures in background, reporting changes.\n\t"
    "\n";

cmd_result_t
cmd_temperature(int unit, args_t *a)
{
    char *function = ARG_GET(a);
    char *interval = ARG_GET(a);
    uint32 delay = 5; /* Report stats every 5 seconds */

    if (! sh_check_attached(ARG_CMD(a), unit))
        return CMD_FAIL;
    if (! function || 
            ! sal_strncasecmp(function, "show", sal_strlen(function)) ){

        if (soc_get_board_id() != G23) {
            soc_i2c_lm75_temperature_show(unit);
        } else {
            soc_i2c_max664x_temperature_show(unit);
        }
    } else if ( ! sal_strncasecmp(function,"watch",sal_strlen(function)) ){
        if ( interval ) 
            delay = parse_integer(interval);
        if (soc_get_board_id() != G23) {
            soc_i2c_lm75_monitor(unit, TRUE, delay);
        } else {
            soc_i2c_max664x_monitor(unit, TRUE, delay);
        }
    } else if ( ! sal_strncasecmp(function,"nowatch",sal_strlen(function)) ){
        if (soc_get_board_id() != G23) {
            soc_i2c_lm75_monitor(unit, FALSE, 0);
        } else {
            soc_i2c_max664x_monitor(unit, FALSE, 0);
        }
    } else {
        return CMD_USAGE;
    }
    return CMD_OK;
}


STATIC char *
_i2c_mux_default_dev_name_get(void)
{
    /* Default device locates the DAC */
    char *dev_name = NULL;

    if (soc_get_board_id() == G27) {
        dev_name = I2C_MUX_6;
    } else if ((soc_get_board_id() == G25) || (soc_get_board_id() == G24) ||
               (soc_get_board_id() == G28) || (soc_get_board_id() == G29) ||
               (soc_get_board_id() == G30) || (soc_get_board_id() == G31) ||
               (soc_get_board_id() == G33) || (soc_get_board_id() == G34) ||
               (soc_get_board_id() == G35) || (soc_get_board_id() == G38) ||
               (soc_get_board_id() == G39) || (soc_get_board_id() == G40) ){
        dev_name = I2C_MUX_3;
    } else {
        dev_name = I2C_LPT_0;
    }
    return dev_name;
}

/*
 * Function: cmd_i2c
 * Purpose: I2C probe/attach/show, configuration commands.
 *          Also sets up board index and configures I2C drivers
 *          based on the inferred system board type.
 */
char cmd_i2c_usage[] =
    "Usages:\n\t"
#ifdef COMPILER_STRING_CONST_LIMIT
    "i2c probe | log | backlog | clearlog | reset | speeds | show\n"
#else
    "i2c probe [pio|intr] [speed] [quiet]\n\t"
    "    - probe devices on I2C bus and build device tree.\n\t"
    "      If \"intr\" or \"pio\" is specified, change to that bus mode.\n\t"
    "      If a valid speed is specified, change the bus to that speed.\n\t"
    "      If \"quiet\" is specified, suppresses probe output.\n\t"
#ifdef SHADOW_SVK
    "i2c test board_type\n\t"
    "    - Test presence of I2C devices in the SVK.\n\t"
    "      Supported board types are shadow_40GSVK and shadow_10GSVK\n\t"
#endif
    "i2c scan [pio|intr] [saddr] [quiet]\n\t"
    "    - Scan devices on I2C bus and display the device list.\n\t"
    "i2c log \n\t"
    "    - show I2C bus transaction log.\n\t"
    "i2c backlog \n\t"
    "    - show I2C bus transaction log (in reverse order).\n\t"
    "i2c clearlog \n\t"
    "    - reset I2C bus transaction log.\n\t"
    "i2c reset\n\t"
    "    - reset I2C bus controller core.\n\t"
    "i2c speeds\n\t"
    "    - show supported I2C bus controller clock rates.\n\t"
    "i2c show\n\t"
    "    - show devices found and their attributes.\n\t"
    "i2c read saddr comm len \n\t"
    "    - generic interface to read devices, do probe first \n\t"
    "i2c readb saddr len \n\t"
    "    - generic interface to read devices without register based access, do probe first\n\t"
    "i2c write saddr comm [data] \n\t"
    "    - generic interface to write a byte to devices, do probe first\n\t"
    "    - comm is data when writing directly otherwise command register\n"

#endif
    ;

cmd_result_t
cmd_i2c(int unit, args_t *a)
{
    char *function, *op;
    uint32 flags;
    int rv, quiet, speed;
    int i, parsedata;
    uint8 saddr=0, data=0, comm=0;
    int len=0;
    uint8 buffer[256];

#ifdef SHADOW_SVK
    int svk_40g = 0;
    int svk_10g = 0;
#endif /* SHADOW_SVK */
    
    if (! sh_check_attached(ARG_CMD(a), unit))
    return CMD_FAIL;
    
    function = ARG_GET(a);
    if (! function ) {
    return CMD_USAGE;
    } else if (! sal_strncasecmp(function, "show", sal_strlen(function)) ){
    soc_i2c_show(unit);
    } else if ( ! sal_strncasecmp(function,"reset", sal_strlen(function))) {
    soc_i2c_reset(unit);
    } else if ( !sal_strncasecmp(function, "speeds", sal_strlen(function))) {
    soc_i2c_show_speeds(unit);
    } else if ( !sal_strncasecmp(function, "log", 3)) {
    soc_i2c_show_log(unit, FALSE);
    } else if ( !sal_strncasecmp(function, "clearlog", sal_strlen(function))) {
    soc_i2c_clear_log(unit);
    } else if ( !sal_strncasecmp(function, "backlog", 7)) {
    soc_i2c_show_log(unit, TRUE);
    } else if ( !sal_strncasecmp(function, "readb", 5)) {
        if (!soc_i2c_is_attached(unit)) {
            cli_out("i2c raw read/write commands require probe to be called first\n");
            return CMD_USAGE;
        }
        op = ARG_GET(a);
        if (op == NULL) {
            return CMD_USAGE;
        } else {
            parsedata = parse_integer(op);
            if (parsedata == 0) {
                cli_out("\nNeed Slave address");
                return CMD_FAIL;
            }
            saddr = parsedata;
        }
        op = ARG_GET(a);
        if (op == NULL) {
            return CMD_USAGE;
        } else {
            len = parse_integer(op);
        }
        for(i=0; i<len; i++) {
            rv = soc_i2c_read_byte(unit, saddr, &data);
            if (SOC_FAILURE(rv)) {
                break;
            }
            if (i%16 == 0) {
                cli_out("\n%02x:", i);
            }
            cli_out(" %02x", data);
        }
        cli_out("\n");
        return CMD_OK;
        
    } else if ( !sal_strncasecmp(function, "read", 4)) {
        if (!soc_i2c_is_attached(unit)) {
            cli_out("i2c raw read/write commands require probe to be called first\n");
            return CMD_USAGE;
        }
        op = ARG_GET(a);
        if (op == NULL) {
            return CMD_USAGE;
        } else {
            parsedata = parse_integer(op);
            if (parsedata == 0) {
                cli_out("\nNeed Slave address");
                return CMD_FAIL;
            }
            saddr = parsedata;
        }
        op = ARG_GET(a);
        if (op == NULL) {
            return CMD_USAGE;
        } else {
            comm = parse_integer(op);
        }
        op = ARG_GET(a);
        if (op == NULL) {
            return CMD_USAGE;
        } else {
            len = parse_integer(op);
            /*if (len > 256) {
                cli_out("\nlen too big, Buffer is limited to 256 bytes");
                return CMD_FAIL;
            }*/
        }
        for(i=0; i<len; i++) {
            rv = soc_i2c_read_byte_data(unit, saddr, comm + i, &buffer[i]);
            if (SOC_FAILURE(rv)) {
                break;
            }
            if (i%16 == 0) {
                cli_out("\n%02x:", i);
            }
            cli_out(" %02x", buffer[i]);
        }
        cli_out("\n");
        return CMD_OK;
    } else if ( !sal_strncasecmp(function, "write", 4)) {
        if (!soc_i2c_is_attached(unit)) {
            cli_out("i2c raw read/write commands require probe to be called first\n");
            return CMD_USAGE;
        }
        op = ARG_GET(a);
        if (op == NULL) {
            return CMD_USAGE;
        } else {
            parsedata = parse_integer(op);
            if (parsedata == 0) {
                cli_out("\nNeed Slave address");
                return CMD_FAIL;
            }
            saddr = parsedata;
        }
        op = ARG_GET(a);
        if (op == NULL) {
            return CMD_USAGE;
        } else {
            comm = parse_integer(op);
        }
        op = ARG_GET(a);
        if (op == NULL) {
            rv = soc_i2c_write_byte(unit, saddr, comm);
        } else {
            data = parse_integer(op);
            rv = soc_i2c_write_byte_data(unit, saddr, comm, data);
        }
        if (SOC_SUCCESS(rv)) {
            return CMD_OK;
        } else {
            return CMD_FAIL;
        }

    } else if ( ( !sal_strncasecmp(function,"probe", sal_strlen(function))) ||
#ifdef SHADOW_SVK
                ( !sal_strncasecmp(function,"test",  sal_strlen(function))) ||
#endif
                ( !sal_strncasecmp(function,"scan", sal_strlen(function)))) {
    quiet = 0;
    flags = SOC_I2C_MODE_INTR;
    speed = -1; /* Use current speed, or default initially. */

    while ((op = ARG_GET(a)) != NULL) {
        if (sal_strncasecmp(op, "quiet", sal_strlen(op)) == 0) {
        quiet = 1;
        continue;
        }
        if (sal_strncasecmp(op, "pio", sal_strlen(op)) == 0) {
        flags = SOC_I2C_MODE_PIO;
        continue;
        }
        if (sal_strncasecmp(op, "intr", sal_strlen(op)) == 0) {
        flags = SOC_I2C_MODE_INTR;
        continue;
        }
#ifdef SHADOW_SVK
        if (sal_strncasecmp(op, "shadow_40GSVK", sal_strlen(op)) == 0) {
        svk_40g = 1;
        continue;
        }
        if (sal_strncasecmp(op, "shadow_10GSVK", sal_strlen(op)) == 0) {
        svk_10g = 1;
        continue;
        }
#endif /* SHADOW_SVK */
        if (isdigit((unsigned)op[0])) {
        speed = parse_integer(op);
        continue;
        }
        LOG_ERROR(BSL_LS_APPL_I2C,
                  (BSL_META_U(unit,
                              "%s: ERROR: unknown probe argument: %s\n"),
                              ARG_CMD(a), op));
        return CMD_FAIL;
    }
#ifdef SHADOW_SVK
        if ( !sal_strncasecmp(function,"test", sal_strlen(function))) {
            int i = 0;
            uint8 lptVal = 0;
            int fd;
            char *name;
            rv = soc_i2c_probe(unit); /* Will attach if not yet attached */
            if (rv < 0) {
                cli_out("I2C Test Failed: I2C probe Failed\n");
                return CMD_FAIL;
            }
            if (svk_40g) {
                i2c_saddr_t saddr[5] = {0x77, 0x76, 0x25, 0x20, 0x24};

                for(i = 0; i < 5; i++) {
                    if (soc_i2c_device_present(unit, saddr[i]) < 0) {
                        cli_out("I2C Test Failed: Device NOT at slave address 0X%02X (%s)\n",
                                saddr[i], soc_i2c_saddr_to_string(unit, saddr[i]));
                        return CMD_FAIL;
                    }
                }
                name = I2C_MUX_6;
                if ( (fd = _i2c_mux_open(unit, 0, 0, name)) < 0) {
                    cli_out("I2C Test Failed: MUX at 0x76 Failed\n");
                    return CMD_FAIL;
                }

                /* Go through Mux */
                for(i = 0; i < 4; i++) {
                    lptVal = (1 << i);
                    if( (rv = bcm_i2c_write(unit, fd, 0, &lptVal, 1)) < 0){
                        cli_out("I2C Test Failed:MUX Channel%d couldn't be selected\n", i);
                        return CMD_FAIL;
                    }
                    if (soc_i2c_device_present(unit, 0x50) < 0) {
                        cli_out("QSP not present in Channel %d \n", i);
                    }
                }
                cli_out("I2C Test Passed\n");
                return CMD_OK;
            } else if (svk_10g) {
                i2c_saddr_t saddr[8] = {0x77, 0x76, 0x22, 0x24, 0x27, 0x28,
                0x49, 0x6A};
                for(i = 0; i < 8; i++) {
                    if (soc_i2c_device_present(unit, saddr[i]) < 0) {
                        cli_out("I2C Test Failed: Device NOT at slave address 0X%02X (%s)\n",
                                saddr[i], soc_i2c_saddr_to_string(unit, saddr[i]));
                        return CMD_FAIL;
                    }
                }
                name = I2C_MUX_6;
                if ( (fd = _i2c_mux_open(unit, 0, 0, name)) < 0) {
                    cli_out("I2C Test: Failed: MUX at 0x76 Failed\n");
                    return CMD_FAIL;
                }
                /* Go through Mux */
                for(i = 0; i < 4; i++) {
                    lptVal = (1 << i);
                    if( (rv = bcm_i2c_write(unit, fd, 0, &lptVal, 1)) < 0){
                        cli_out("I2C Test: Failed:MUX Channel%d couldn't be selected\n", i);
                        return CMD_FAIL;
                    }
                    if (soc_i2c_device_present(unit, 0x50) < 0) {
                        cli_out("QSP not present in Channel %d \n", i);
                    }
                }
                /* Go through Mux */
                for(i = 5; i < 8; i++) {
                    lptVal = (1 << i);
                    if( (rv = bcm_i2c_write(unit, fd, 0, &lptVal, 1)) < 0){
                        cli_out("I2C Test: Failed:MUX Channel%d couldn't be selected\n", i);
                        return CMD_FAIL;
                    }
                    if (soc_i2c_device_present(unit, 0x2C) < 0) {
                        cli_out("DAC not present at Channel %d \n", i);
                    }
                }
                name = I2C_MUX_7;
                if ( (fd = _i2c_mux_open(unit, 0, 0, name)) < 0) {
                    cli_out("I2C Test Failed: MUX at 0x77 Failed\n");
                    return CMD_FAIL;
                }

                /* Go through Mux */
                for(i = 0; i < 8; i++) {
                    lptVal = (1 << i);
                    if( (rv = bcm_i2c_write(unit, fd, 0, &lptVal, 1)) < 0){
                        cli_out("I2C Test: Failed:MUX Channel%d couldn't be selected\n", i);
                        return CMD_FAIL;
                    }
                    if (soc_i2c_device_present(unit, 0x50) < 0) {
                        cli_out("SFP+ not present in Channel %d \n", i);
                    }
                }
                cli_out("I2C Test: Passed\n");
                return CMD_OK;
            } else {
                    cli_out("I2C Test: Unknown Board type\n");
                    return CMD_FAIL;
            }
        }
#endif

        if ( !sal_strncasecmp(function,"probe", sal_strlen(function))) {
            /*
             * (Re-)attach to the I2C bus if the INTR/PIO mode or
             * bus speed has been specified (presumably new setting).
             */
            if ((rv=soc_i2c_attach(unit, flags | SOC_I2C_NO_PROBE, speed)) < 0) {
                LOG_ERROR(BSL_LS_APPL_I2C,
                          (BSL_META_U(unit,
                                      "%s: ERROR: attach failed: %s\n"),
                                      ARG_CMD(a), bcm_errmsg(rv)));
            }

            rv = soc_i2c_probe(unit); /* Will attach if not yet attached */
            if (rv < 0) {
                LOG_ERROR(BSL_LS_APPL_I2C,
                          (BSL_META_U(unit,
                                      "%s: ERROR: probe failed: %s\n"),
                                      ARG_CMD(a), bcm_errmsg(rv)));
            } else if (!quiet) {
                cli_out("%s: detected %d device%s\n",
                        ARG_CMD(a), rv, rv==1 ? "" : "s");
            }
        } else {
            i2c_saddr_t saddr = 0x00;
            int saddr_start = 0x00;
            int saddr_end = 0x7F;

            /*
             * (Re-)attach to the I2C bus if the INTR/PIO mode or
             * bus speed has been specified (presumably new setting).
             */
            if (!soc_i2c_is_attached(unit) || (flags != SOC_I2C_MODE_INTR)) {
                if ((rv=soc_i2c_attach(unit, flags, -1)) < 0) {
                    LOG_ERROR(BSL_LS_APPL_I2C,
                              (BSL_META_U(unit,
                                          "%s: ERROR: attach failed: %s\n"),
                               ARG_CMD(a), bcm_errmsg(rv)));
                }
            }

            if  (speed > 0) {
                saddr_start = saddr_end = speed & 0x7F;
            }
            for(;saddr_start <= saddr_end; saddr_start++) {
                saddr = saddr_start & 0x7F;
                if (soc_i2c_device_present(unit, saddr) >= 0) {
                    cli_out("I2C device found at slave address 0X%02X (%s)\n",
                            saddr_start, soc_i2c_saddr_to_string(unit, saddr));
                }
            }
        }
    }
    else {
    return CMD_USAGE;
    }
    return CMD_OK;
}

/*
 * Function: cmd_clk
 * Purpose: Set W229b/W311 clock speed for core clocks
 */
char cmd_clk_usage[] =
    "Usages:\n\t"
    "       clocks w311mode \n\t"
    "           - enable Cypress W311 clock chip driver mode.\n\t"
    "       clocks [speed] \n\t"
    "           - set BCM56xx core clock speed.\n\t"
    "\n";

cmd_result_t
cmd_clk(int unit, args_t *a)
{
    char *speed = ARG_GET(a);
    uint32 clockval = 0;
#ifdef COMPILER_HAS_DOUBLE
    double clockFVal = 0;
#else
    int clockFVal = 0;
#endif
    static int isInW311Mode = 0;
    int fd;
    int    rv;

    if (! sh_check_attached(ARG_CMD(a), unit))
    return CMD_FAIL;

    /* Open PLL */
    if ((fd = bcm_i2c_open(unit, I2C_PLL_0, 0,0)) < 0) {
    cli_out("%s: cannot open I2C device %s: %s\n",
            ARG_CMD(a), I2C_PLL_0, bcm_errmsg(fd));
    return CMD_FAIL;
    }

    /* When detecting P48, set clock mode automagically
     * to W311
     */
    cli_out("Clock configuration: %s mode\n",
            isInW311Mode ? "W311" : "W229b");
    
    if (speed && !sal_strncasecmp(speed, "w311mode", sal_strlen(speed))) {
    isInW311Mode = 1;
    cli_out("Set W311 mode enable\n");
    return CMD_OK;
    }
    if (isInW311Mode) {
    if(!speed){
        cli_out("ERROR: no speed specified!\n");
        return CMD_FAIL;
    }
#ifdef COMPILER_HAS_DOUBLE
    clockFVal = atof( speed ) ;
#else
        clockFVal = _shr_atof_exp10(speed, 6);
#endif
    
    /* Enable W311 mode */
    rv = bcm_i2c_ioctl(unit, fd, I2C_PLL_SETW311,
               &isInW311Mode, sizeof(isInW311Mode));
    if (rv < 0) {
        cli_out("ERROR: clock mode configuration failed: %s\n",
                bcm_errmsg(rv));
    }
    
    /* Set speed */
    rv = bcm_i2c_ioctl(unit, fd, 0, &clockFVal, 0);
    if (rv < 0) {
        cli_out("ERROR: clock speed configuration failed: %s\n",
                bcm_errmsg(rv));
        return CMD_FAIL;
    }
#ifdef COMPILER_HAS_DOUBLE
    cli_out("Set %s clock to %2.2fMhz\n",
            I2C_PLL_0, clockFVal);
#else
        cli_out("Set %s clock to %d.%02dMhz\n",
                I2C_PLL_0,
                INT_FRAC_2PT_4(clockFVal));
#endif
    } else {
    /* Use W229b access mode */
    /* ioctl: will print out all values on bad speed value */
    if (! speed ){
        clockval = -1;
    }  else{
        clockval = parse_integer(speed) ;
    }

    rv = bcm_i2c_ioctl(unit, fd, I2C_PLL_SETW311,
               &isInW311Mode, sizeof(isInW311Mode));
    if (rv < 0) {
        cli_out("ERROR: clock mode configuration failed: %s\n",
                bcm_errmsg(rv));
    }
    rv = bcm_i2c_ioctl(unit, fd, clockval, NULL, 0);
    if (rv < 0) {
        cli_out("ERROR: clock speed configuration failed: %s\n",
                bcm_errmsg(rv));
    }
    }
    return CMD_OK ;
}


#ifdef SHADOW_SVK
#define _SHADOW_OPEN_TEST(_u, _nm, _fl, _sp, _fd, _pf)               \
    do {                                                             \
       if ((_fd = bcm_i2c_open(_u, _nm, _fl, _sp)) < 0) {            \
           _pf = 0;                                                  \
           LOG_VERBOSE(BSL_LS_SOC_COMMON, \
                       (BSL_META_U(_u, \
                                   "Could not open %s: %s\n"),   \
                                   _nm, bcm_errmsg(_fd)));                       \
       }                                                             \
    } while(0)


#define _SHADOW_WRITE_TEST(_u, _f, _r, _d, _l, _nm, _rv, _pf)        \
    do {                                                             \
       _rv = bcm_i2c_write(_u, _f, _r, &_d, _l);                     \
       if (_rv < 0) {                                                \
          _pf = 0;                                                   \
          LOG_VERBOSE(BSL_LS_SOC_COMMON, \
                      (BSL_META_U(_u, \
                                  "%s writing failed: %s\n"), \
                                  _nm, bcm_errmsg(_rv)));                        \
        }                                                            \
    } while (0)


#define _SHADOW_READ_TEST(_u, _f, _r, _d, _l, _nm, _rv, _pf)         \
   do {                                                              \
      _rv = bcm_i2c_read(_u, _f, _r, &_d, &_l);                      \
      if (_rv < 0) {                                                 \
          _pf = 0;                                                   \
          LOG_VERBOSE(BSL_LS_SOC_COMMON, \
                      (BSL_META_U(_u, \
                                  "%s reading failed: %s\n"), \
                                  _nm, bcm_errmsg(_rv)));                        \
      }                                                              \
   } while (0)

#define _SHADOW_IOCTL_TEST(_u, _f, _op, _d, _l, _nm, _rv, _pf)       \
   do {                                                              \
      _rv = bcm_i2c_ioctl(_u, _f, _op, &_d, _l);                     \
      if (_rv < 0) {                                                 \
          _pf = 0;                                                   \
          LOG_VERBOSE(BSL_LS_SOC_COMMON, \
                      (BSL_META_U(_u, \
                                  "%s ioctl failed: %s\n"),   \
                                  _nm, bcm_errmsg(_rv)));                        \
      }                                                              \
   } while (0)

#define _SHADOW_BIT_SET(_b)   (1 << _b)


#define _SHADOW_TEST_STATUS_PRINT(_pf)                               \
    cli_out("%s\n", (_pf == 1) ? "PASSED" : "FAILED")


STATIC cmd_result_t
_bb_shadow_test(int unit, bb_type_t *baseboard)
{
    int fd, dev, pass, rv = SOC_E_NONE;
    uint8 wrval, rdval;
    uint32 len;
    char *name;

    pass = 1;
    cli_out("   Checking board id: ");
    _SHADOW_OPEN_TEST(unit, I2C_IOP_1, 0, 0, fd, pass);
    if (fd >= 0) {
        _SHADOW_READ_TEST(unit, fd, I2C_IOP_1_BRD_ID, 
                          rdval, len, I2C_IOP_1, rv, pass);
    }
#if 0
    if (rv == SOC_E_NONE) {
        if (rdval != /* expected board id(s) */) {
            pass = 0;
            LOG_VERBOSE(BSL_LS_SOC_COMMON_I2C,
                        (BSL_META_U(unit,
                                    "....Invalid board ID %x\n"), buf[0]));
        }
    }
#endif
    _SHADOW_TEST_STATUS_PRINT(pass);

    cli_out("   Checking MAX127: ");
    pass = 1;
    _SHADOW_OPEN_TEST(unit, I2C_ADC_0, 0, 0, fd, pass);
    if (fd >= 0) {
        int i;
        int pass = 1;
        i2c_adc_t adc_data;
        
        for (i = 0; i < 8; i++) {
            _SHADOW_IOCTL_TEST(unit, fd, I2C_ADC_QUERY_CHANNEL, adc_data, i,
                               I2C_ADC_0, rv, pass);
        }
    }
    _SHADOW_TEST_STATUS_PRINT(pass);
    

    for (dev = 0; dev < 2; dev++) {
        char *name;
        /* REMOVE THIS -testing only 
         * name = (dev == 0) ? I2C_LPT_0 : I2C_LPT_1; */
        name = (dev == 0) ? I2C_MUX_6 : I2C_MUX_7;

        pass = 1;
        cli_out("   Checking %s: ", name);
        _SHADOW_OPEN_TEST(unit, name, 0, 0, fd, pass);
        if (fd >= 0) {
            int i;
            /* 
             * 5N bit march 
             * Bit(0) -> Bit(7): write 0
             * Bit(0) -> Bit(7): read  0, write 1
             * Bit(7) -> Bit(0): read  1, write 0
             */
            LOG_VERBOSE(BSL_LS_SOC_COMMON,
                        (BSL_META_U(unit,
                                    "\n1. Bit(0) -> Bit(7) write 0\n")));
            for (i = 0; i < 8; i++) {
                wrval &= ~(_SHADOW_BIT_SET(i));
                LOG_VERBOSE(BSL_LS_SOC_COMMON,
                            (BSL_META_U(unit,
                                        "writing 0x%02x "), wrval));
                _SHADOW_WRITE_TEST(unit, fd, 0, wrval, 1, name, rv, pass);
            }
            wrval = 0;
            LOG_VERBOSE(BSL_LS_SOC_COMMON,
                        (BSL_META_U(unit,
                                    "2. Bit(0) -> Bit(7) read 0 write 1\n")));
            for (i = 0; i < 8; i++) {
                _SHADOW_READ_TEST(unit, fd, 0, rdval, len, name, rv, pass);
                if (rv == SOC_E_NONE) { 
                    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                                (BSL_META_U(unit,
                                            "read 0x%02x\n"), rdval));
                    if ((rdval & _SHADOW_BIT_SET(i)) != 
                        (wrval & _SHADOW_BIT_SET(i))) {  
                        pass = 0;
                        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                                    (BSL_META_U(unit,
                                                "%s bit check failed 0x%02x 0x%02x\n"), 
                                     name, (rdval & _SHADOW_BIT_SET(i)), 
                                     (wrval & _SHADOW_BIT_SET(i)))); 
                    }
                }
                wrval |= _SHADOW_BIT_SET(i);
                LOG_VERBOSE(BSL_LS_SOC_COMMON,
                            (BSL_META_U(unit,
                                        "writing 0x%02x "), wrval));
                _SHADOW_WRITE_TEST(unit, fd, 0, wrval, 1, name, rv, pass);
            }

            LOG_VERBOSE(BSL_LS_SOC_COMMON,
                        (BSL_META_U(unit,
                                    "3. Bit(7) -> Bit(0) read 1 write 0\n")));
            for (i = 7; i > 0 ; i--) {
                _SHADOW_READ_TEST(unit, fd, 0, rdval, len, name, rv, pass);
                if (rv == SOC_E_NONE) { 
                    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                                (BSL_META_U(unit,
                                            "read 0x%02x\n"), rdval));
                    if ((rdval & _SHADOW_BIT_SET(i)) != 
                        (wrval & _SHADOW_BIT_SET(i))) {  
                        pass = 0;
                        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                                    (BSL_META_U(unit,
                                                "%s bit check failed 0x%02x 0x%02x\n"), 
                                     name, (rdval & _SHADOW_BIT_SET(i)), 
                                     (wrval & _SHADOW_BIT_SET(i)))); 
                    }
                }
                wrval &= ~(_SHADOW_BIT_SET(i));
                LOG_VERBOSE(BSL_LS_SOC_COMMON,
                            (BSL_META_U(unit,
                                        "writing 0x%02x "), wrval));
                _SHADOW_WRITE_TEST(unit, fd, 0, wrval, 1, name, rv, pass);
                _SHADOW_READ_TEST(unit, fd, 0, rdval, len, name, rv, pass);
                if (rv == SOC_E_NONE) { 
                    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                                (BSL_META_U(unit,
                                            "read 0x%02x\n"), rdval));
                    if ((rdval & ~(_SHADOW_BIT_SET(i))) != 
                        (wrval & ~(_SHADOW_BIT_SET(i)))) {  
                        pass = 0;
                        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                                    (BSL_META_U(unit,
                                                "%s bit check failed 0x%02x 0x%02x\n"), 
                                     name, (rdval & _SHADOW_BIT_SET(i)), 
                                     (wrval & _SHADOW_BIT_SET(i)))); 
                    }
                }
            }
        }
        _SHADOW_TEST_STATUS_PRINT(pass);
    }


    pass = 1;
    for (dev = 0; dev < 2; dev++) {
        name = (dev == 0) ? I2C_IOP_1 : I2C_IOP_0;
        cli_out("   Checking %s: ", name);
        _SHADOW_OPEN_TEST(unit, name, 0, 0, fd, pass);
        if (fd >= 0) {
            int i;
            /* 
             * Just verify the ports are readable as several ports
             * are read only. Special care is required for the XON_XOFF
             * pins, which are overloaded onto EB_ADDR. These pins are
             * connected to both the FPGA and one of the 9505 ports
             * (addr 0x25 port 0). This port can only drive these pins
             * while the device is in PCIE or VLI mode. The FPGA will
             * tri-state these pins when in PCIE or VLI. Make sure not
             * to drive these pins from the 9505 while in EB3 mode in
             * order to avoid potential damage to the FPGA I/0.
             */
            for (i = 0; i < 5; i++) {
                _SHADOW_READ_TEST(unit, fd, i, rdval, len, name, rv, pass);
            }
        }
        _SHADOW_TEST_STATUS_PRINT(pass);
    }

    return CMD_OK;
}

STATIC cmd_result_t
_bb_test(int unit, bb_type_t *baseboard)
{
    if (!baseboard) {
        return CMD_FAIL;
    }
    if (!SOC_IS_SHADOW(unit)) {
        return CMD_NOTIMPL;
    }

    return _bb_shadow_test(unit, baseboard);
}
#endif /* SHADOW_SVK */

/*
 * I2C specific commands for Baseboards which allow for control of
 * clocks, voltages and provide other measurements like power,
 * temperature, thickness, etc.
 */

/*
 * Tunable voltage parameters for P48 and Gen2 baseboards
 * This table encodes DAC/ADC VCC chip control and comm data.
 */
typedef 
struct bb_vconfig_params{
    int cal_idx;       /* DAC calibration table index: See ltc1427.c */
    char *function; /* Voltage source to configure */
    uint8 mux;         /* MUXSEL (lpt0) value */
    char *adc;       /* ADC device name */
    int  chan;         /* A/D channel #   */
    char *dac;       /* DAC Device name */
    int flags;    /* DAC calibrated, power computation IDs */
} bb_vparams_t;

/* Defines for the bb_vparams_t flags field */
#define DAC_UNCALIBRATED 0x00000000
#define DAC_CALIBRATED   0x00000001
#define PWR_CORE_A_SRC   0x00000002
#define PWR_CORE_A_DROP  0x00000004
#define PWR_CORE_B_SRC   0x00000008
#define PWR_CORE_B_DROP  0x00000010
#define PWR_PHY_SRC      0x00000020
#define PWR_PHY_DROP     0x00000040
#define PWR_CORECAL_SRC  0x00000080
#define PWR_CORECAL_DROP 0x00000100
#define PWR_PHYCAL_SRC   0x00000200
#define PWR_PHYCAL_DROP  0x00000400
#define PWR_IOCAL_SRC    0x00000800
#define PWR_IOCAL_DROP   0x00001000
#define PWR_IO_SRC       0x00002000
#define PWR_IO_DROP      0x00004000
#define PWR_CORE_A_LOAD  0x00008000
#define PWR_CORE_B_LOAD  0x00010000
#define PWR_PHY_LOAD     0x00020000

/*
 * Current configuration table
 */
typedef 
struct bb_current_config_s{
    char *function; /* Current sourec to measure. */
    char *adc;       /* ADC device name */
    int  chan;         /* A/D channel #   */
    int  sens;       /* Sensitivity in mV/A */
    int flags;    /* Flags for expansion */
} bb_current_config_t;

/*
 * Clock chip configuration table for all clock sources.
 * This table encodes the necessary data to communicate with every w311
 */
typedef struct bb_clock_config_s{
    char *name;   /* Clock output name */
    char *alias;  /* Clock output name (alias) */
    char *device; /* I2C device name   */
    int unit;     /* PCI Device */
    uint8 mux;    /* BBMUX (lpt0) value; 0xFF means no MUX control needed */
    int xpll;     /* For CY22393 (& CY22150), the PLL (or clock) to modify */
} bb_clock_config_t;



/* For P48 (G3) and BaseBoard/P48 command, we have a separate
 * Clock chip table since we are using W311's in a Mux'd fashion
 * which is specific to just this board. Other boards use the new
 * Cypress CY22393 clock chip and should use the "xclocks" command.
 * INDEXED_BY_BOARD_TYPE
 */
bb_clock_config_t
bb_clocks[NUM_BOARD_TYPES][MAX_CLOCKS_PER_BOARD] = {
    /* G5, Single (non-MUXed) CY22150, 2 clocks */
    {{"Core",   NULL, "clk0", 0, 0xFF,I2C_XPLL_CORE},
     {"PCI",    NULL, "clk0", 0, 0xFF,I2C_XPLL_PCI},
    },
    /* G13, Single (non-MUXed) CY22393, 3 clocks */
    {{"Test",        NULL,     "clk0", 0, 0xFF,I2C_XPLL_PLL3},
     {"Core",        "Ref133", "clk0", 0, 0xFF,I2C_XPLL_PLL2},
     {"PCI",         NULL,     "clk0", 0, 0xFF,I2C_XPLL_PLL1},
    },
    {{0}}, /* G15, uses synth command for clock control */
    {{0}}, /* G16, uses synth command for clock control */
    {{0}}, /* G17, uses synth command for clock control */
    {{0}}, /* G18, uses synth command for clock control */
    {{0}}, /* G19, uses synth command for clock control */
    {{0}}, /* G20, uses synth command for clock control */
    {{0}}, /* G21, uses synth command for clock control */
    {{0}}, /* G22, uses synth command for clock control */
    {{0}}, /* G23, uses synth command for clock control */
    {{0}}, /* G24, uses synth command for clock control */
    {{0}}, /* G25, uses synth command for clock control */
    {{0}}, /* G26, uses synth command for clock control */
    {{0}}, /* G27, uses synth command for clock control */
    {{0}}, /* G28, uses synth command for clock control */
    {{0}}, /* G29, uses synth command for clock control */
    {{0}}, /* G30, uses synth command for clock control */
    {{0}}, /* G31, uses synth command for clock control */
    {{0}}, /* G33, uses synth command for clock control */
    {{0}}, /* G34, uses synth command for clock control */
    {{0}}, /* G35, uses synth command for clock control */
    {{0}}, /* ENG */
    {{0}}  /* UNK */
};

/* 
 * bb_clock_len[] table contains the number of configurable
 * clocks available for each board type.
 * Used by cmd_bb() for "Baseboard" and "P48" commands.
 * A zero disables the clock function for Baseboard/P48.
 * INDEXED_BY_BOARD_TYPE 
 */
int bb_clock_len[NUM_BOARD_TYPES] = {
    2, /* G5 */
    3, /* G13 */
    0, /* G15 */ /* Uses synth command */
    0, /* G16 */ /* Uses synth command */
    0, /* G17 */ /* Uses synth command */
    0, /* G18 */ /* Uses synth command */
    0, /* G19 */ /* Uses synth command */
    0, /* G20 */ /* Uses synth command */
    0, /* G21 */ /* Uses synth command */
    0, /* G22 */ /* Uses synth command */
    0, /* G23 */ /* Uses synth command */
    0, /* G24 */ /* Uses synth command */
    0, /* G25 */ /* Uses synth command */
    0, /* G26 */ /* Uses synth command */
    0, /* G27 */ /* Uses synth command */
    0, /* G28 */ /* Uses synth command */
    0, /* G29 */ /* Uses synth command */
    0, /* G30 */ /* Uses synth command */
    0, /* G31 */ /* Uses synth command */
    0, /* G33 */ /* Uses synth command */
    0, /* G34 */ /* Uses synth command */
    0, /* G35 */ /* Uses synth command */
    0, /* ENG */    
    0  /* UNK */    
};


/* Calibration table for DAC/ADC Voltage parameters.
 * Used to calibrate the DAC->PowerSupply->ADC loop.
 * NOTE: Turbo is special (MAX dacval = min voltage)
 * INDEXED_BY_BOARD_TYPE 
 * NOTE: For each board type, the order of each DAC entry
 *       must match that of their associated entries in 
 *       the bb_vs_config table.
 */
/* On HUMV/Bradley/GW slow part, max and min should be set to  960 and
 * 64 to prevent the system crashes during voltage calibration.
 */
#define HVSPO 64  /* HUMV Slow Part Offset */
/* Triumph3 56640 board core voltage calibration tweak
 */
#define TR3_640_MAX_ADJUST 0x3  /* TR3 56640 board core voltage limit */
#define TR3_TCAM_ADJUST 0x10  /* TR3 56640 board TCAM voltage is .9V, not 1V */
/* idx, name, max, min, step, dac_last, dac_min, dac_max, dac_mid, dac_use_max */
static dac_calibrate_t
dac_params[NUM_BOARD_TYPES][MAX_VS_CONFIG_PER_BOARD] = {
    /* Hercules: aka G5 */
    {{0,"Analog",-1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {1,"2.5",-1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {2,"Core",-1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {0},{0}},
    /* Firebolt SDK design: aka G13 */
    {{0,"Core",-1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {1,"2.5",-1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {2,"3.3",-1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {3,"Analog",-1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {0}},
    /* Goldwing SDK design: aka G15 */
    {{0,"Core",-1,1,0,0,LTC1427_MIN_DACVAL+HVSPO,LTC1427_MAX_DACVAL-HVSPO,LTC1427_MID_DACVAL,1},
     {1,"Analog",-1,1,0,0,LTC1427_MIN_DACVAL+HVSPO,LTC1427_MAX_DACVAL-HVSPO,LTC1427_MID_DACVAL,1},
     {0},{0},{0}},
    /* Bradley 1G SDK design: aka G16 */
    {{0,"Analog",-1,1,0,0,LTC1427_MIN_DACVAL+HVSPO,LTC1427_MAX_DACVAL-HVSPO,LTC1427_MID_DACVAL,1},
     {1,"2.5",-1,1,0,0,LTC1427_MIN_DACVAL+HVSPO,LTC1427_MAX_DACVAL-HVSPO,LTC1427_MID_DACVAL,1},
     {2,"Core",-1,1,0,0,LTC1427_MIN_DACVAL+HVSPO,LTC1427_MAX_DACVAL-HVSPO,LTC1427_MID_DACVAL,1},
     {3,"1.2",-1,1,0,0,LTC1427_MIN_DACVAL+HVSPO,LTC1427_MAX_DACVAL-HVSPO,LTC1427_MID_DACVAL,1},
     {0}},
    /* Bradley SDK design: aka G17 */
    {{0,"Core",-1,1,0,0,LTC1427_MIN_DACVAL+HVSPO,LTC1427_MAX_DACVAL-HVSPO,LTC1427_MID_DACVAL,1},
     {1,"Analog",-1,1,0,0,LTC1427_MIN_DACVAL+HVSPO,LTC1427_MAX_DACVAL-HVSPO,LTC1427_MID_DACVAL,1},
     {0},{0},{0}},
    /* Raptor SDK design: aka G18 */
    {{0,"1.2",-1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {1,"2.5",-1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {2,"PHY",-1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {3,"Analog",-1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {0}},
    /* Raven SDK design: aka G19 */ 
    {{0,"1.2",-1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {1,"2.5",-1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {2,"PHY",-1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {3,"Analog",-1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {0}},
    /* Triumph SDK design: aka G20 */
    {{0,"Core",-1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {1,"1.5",-1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {2,"2.5",-1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {3,"Analog",-1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {0}},
    /* Scorpion SDK design : aka G21 */
    {{0,"Core",-1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {1,"Analog",-1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {0},{0},{0}},
    /* Apollo SDK design: aka G22 */
    {{0,"Core",-1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {1,"2.5",-1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {2,"Analog",-1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {0},{0}},
    
    {{0,"Core",-1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {1,"1.5",-1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {2,"2.5",-1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {3,"Analog",-1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {0}},
    /* Trident SDK design: aka G24 */
    {{0,"Core",-1,1,0,0,ADP4000_MIN_DACVAL,ADP4000_MAX_DACVAL,ADP4000_MID_DACVAL,1},
     {1,"PHY",-1,1,0,0,ADP4000_MIN_DACVAL,ADP4000_MAX_DACVAL,ADP4000_MID_DACVAL,1},
     {2,"Analog",-1,1,0,0,ADP4000_MIN_DACVAL,ADP4000_MAX_DACVAL,ADP4000_MID_DACVAL,1},
     {0},{0}},
    /* Titan SDK design: aka G25 */
    {{0,"Core",-1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {1,"3.3",-1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {2,"5.0",-1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {3,"Analog",-1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {0}},
    /* Hurricane  SDK design: aka G26 */
    {{0,"Core",  -1,1,0,0,LTC1427_MIN_DACVAL+HVSPO,LTC1427_MAX_DACVAL-HVSPO,LTC1427_MID_DACVAL,1},
     {1,"PHY",   -1,1,0,0,LTC1427_MIN_DACVAL+HVSPO,LTC1427_MAX_DACVAL-HVSPO,LTC1427_MID_DACVAL,1},
     {2,"Analog",-1,1,0,0,LTC1427_MIN_DACVAL+HVSPO,LTC1427_MAX_DACVAL-HVSPO,LTC1427_MID_DACVAL,1},
     {0},{0}},
    /* Shadow SDK design: aka G27*/
    /* Shadow the voltage calibration window is reduced, 128 to 896, A2 parts
     * were failing when Hurricane based calibration windown (64 to 960) was
     * used */
    {{0,"Core",1.198,.805,0,0,LTC1427_MIN_DACVAL+((5*HVSPO)+50),LTC1427_MAX_DACVAL-((5*HVSPO)+50),LTC1427_MID_DACVAL,1},
     {1,"1.0",1.198,.805,0,0,LTC1427_MIN_DACVAL+((5*HVSPO)+50),LTC1427_MAX_DACVAL-((5*HVSPO)+50),LTC1427_MID_DACVAL,1},
     {2,"1.8",2.063,1.538,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {3,"3.3",3.645,2.980,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {0}},
    /* Katana  SDK design: aka G28 */
    {{0,"Core",  -1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {1,"3.3",   -1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {2,"1.5",   -1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {3,"Analog",-1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {0}},
    /* Enduro-2  SDK design: aka G29 */
    {{0,"Core",  -1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {1,"Analog", -1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {3,"3.3",-1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {0},{0}},
    /* Stardust-2  SDK design: aka G30 */
    {{0,"Core",  -1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {1,"3.3",-1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {3,"Analog", -1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {0},{0}},
    /* Enduro-2  SDK design: aka G31 */
    {{0,"Core",  -1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {1,"3.3", -1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {2,"1.5", -1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {3,"Analog",-1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {0}},
    /* Trident SDK design: aka G33 */
    {{0,"Core",-1,1,0,0,ADP4000_MIN_DACVAL,ADP4000_MAX_DACVAL,ADP4000_MID_DACVAL,1},
     {1,"PHY",-1,1,0,0,ADP4000_MIN_DACVAL,ADP4000_MAX_DACVAL,ADP4000_MID_DACVAL,1},
     {2,"Analog",-1,1,0,0,ADP4000_MIN_DACVAL,ADP4000_MAX_DACVAL,ADP4000_MID_DACVAL,1},
     {0},{0}},
    /* Triumph3 Legacy  SDK design: aka G34 */
    {{0,"Core",-1,1,0,0,ADP4000_MIN_DACVAL,ADP4000_MAX_DACVAL,ADP4000_MID_DACVAL,1},
     {1,"3.3", -1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {2,"Analog",-1,1,0,0,ADP4000_MIN_DACVAL,ADP4000_MAX_DACVAL,ADP4000_MID_DACVAL,1},
     {0},{0}},     
    /* Triumph3 SDK design: aka G35 */
    {{0,"Core",-1,1,0,0,ADP4000_MIN_DACVAL,ADP4000_MAX_DACVAL-TR3_640_MAX_ADJUST,ADP4000_MID_DACVAL,1},
     {1,"3.3", -1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {2,"1.8", -1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {3,"Analog",-1,1,0,0,ADP4000_MIN_DACVAL,ADP4000_MAX_DACVAL,ADP4000_MID_DACVAL,1},
     {4,"TCAM",-1,1,0,0,ADP4000_MIN_DACVAL+TR3_TCAM_ADJUST,ADP4000_MAX_DACVAL+TR3_TCAM_ADJUST,ADP4000_MID_DACVAL+TR3_TCAM_ADJUST,1}, {0}},     

    /* Caladan3 48G SVK : aka G36 */
    {{0,"Core",-1,1,0,0,SMM665_CORE_MIN,SMM665_CORE_MAX, SMM665_CORE_MID,1},
     {1,"Analog",-1,1,0,0,SMM665_ANLG_MIN,SMM665_ANLG_MAX,SMM665_ANLG_MID,1},
     {2,"1.5", -1,1,0,0,SMM665_1_5_MIN,SMM665_1_5_MAX,SMM665_1_5_MID,1},
     {3,"3.3", -1,1,0,0,SMM665_3_3_MIN,SMM665_3_3_MAX,SMM665_3_3_MID,1},
     {4,"1.2", -1,1,0,0,SMM665_1_2_MIN,SMM665_1_2_MAX,SMM665_1_2_MID,1},
     {5,"Tcam", -1,1,0,0,SMM665_TCAM_MIN,SMM665_TCAM_MAX,SMM665_TCAM_MID,1},
    },

    /* Caladan3 100G SVK : aka G37 */
    {{0,"Core",-1,1,0,0,SMM665_CORE_MIN,SMM665_CORE_MAX, SMM665_CORE_MID,1},
     {1,"Analog",-1,1,0,0,SMM665_ANLG_MIN,SMM665_ANLG_MAX,SMM665_ANLG_MID,1},
     {2,"1.5", -1,1,0,0,SMM665_1_5_MIN,SMM665_1_5_MAX,SMM665_1_5_MID,1},
     {3,"3.3", -1,1,0,0,SMM665_3_3_MIN,SMM665_3_3_MAX,SMM665_3_3_MID,1},
     {4,"1.2", -1,1,0,0,SMM665_1_2_MIN,SMM665_1_2_MAX,SMM665_1_2_MID,1},
     {5,"Tcam", -1,1,0,0,SMM665_TCAM_MIN,SMM665_TCAM_MAX,SMM665_TCAM_MID,1},
    },
     
    /* Trident2 SVK : aka G38 */
    {{0,"Core",-1,1,0,0,ADP4000_MIN_DACVAL,ADP4000_MAX_DACVAL-TR3_640_MAX_ADJUST,ADP4000_MID_DACVAL,1},
     {1,"Analog",-1,1,0,0,ADP4000_ANLG_MIN_DACVAL,ADP4000_MAX_DACVAL,ADP4000_MID_DACVAL,1},
     {2,"3.3",-1,1,0,0,ADP4000_MIN_DACVAL,ADP4000_MAX_DACVAL,ADP4000_MID_DACVAL,1},
     },     
    /* Trident2 DVT : aka G39 */
    {{0,"Core",-1,1,0,0,ADP4000_MIN_DACVAL,ADP4000_MAX_DACVAL-TR3_640_MAX_ADJUST,ADP4000_MID_DACVAL,1},
     {1,"Analog",-1,1,0,0,ADP4000_ANLG_MIN_DACVAL,ADP4000_MAX_DACVAL,ADP4000_MID_DACVAL,1},
     {2,"3.3", -1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
    },
    /* Katana2 design: aka G40 */
    {{0,"Core",  -1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {1,"3.3",   -1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {2,"1.5",   -1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {3,"Analog",-1,1,0,0,LTC1427_MIN_DACVAL,LTC1427_MAX_DACVAL,LTC1427_MID_DACVAL,1},
     {0}},

    {{0},{0},{0},{0},{0}}, /* ENG */
    {{0},{0},{0},{0},{0}}  /* UNK */            
};

/* Each DAC parameter table, has a number of entries.
 * This value must be set for each board entry for the above table
 * to work correctly.
 * INDEXED_BY_BOARD_TYPE 
 */
static int dac_param_len[NUM_BOARD_TYPES] = {
               3, /* G5 */
               4, /* G13 */
               2, /* G15 */
               4, /* G16 */
               2, /* G17 */
               4, /* G18 */
               4, /* G19 */
               4, /* G20 */	
               2, /* G21 */	
               4, /* G22 */	
               4, /* G23 */	
               3, /* G24 */	
               3, /* G25 */	
               3, /* G26 */	
               4, /* G27 */	
               4, /* G28 */
               3, /* G29 */
               3, /* G30 */
               4, /* G31 */
               3, /* G33 */
               3, /* G34 */
               5, /* G35 */
               6, /* G36 */
               6, /* G37 */
               3, /* G38 */
               3, /* G39 */
               4, /* G40 */
	       0, /* ENG */
	       0  /* UNK */
};

#define MUX_0 0
#define MUX_1 1
#define MUX_2 2
#define MUX_3 3
#define MUX_4 4
#define MUX_5 5
#define MUX_6 6
#define MUX_7 7
#define MUX_8 8
#define MUX_C 0x0C
#define NO_MUX 0xFF

/* Adjustable Voltage Sources parameters.                     */
/* Idx: Index of each table entry (per board)                 */
/* Muxsel: MUX control value needed to access the DAC         */
/*         associated with the desired voltage                */
/*         (0xFF = No MUX control needed)                     */
/* NOTE: For each board type, the order of each voltage entry */
/*       must match that of their associated entries in the   */
/*       dac_params table.                                    */
/* INDEXED_BY_BOARD_TYPE                                      */
bb_vparams_t
bb_vs_config[NUM_BOARD_TYPES][MAX_VS_CONFIG_PER_BOARD] = {
    /* Idx, Name, Muxsel, ADC Device Name, ADC Channel, DAC Device, Calibrated */
    /* G5: Hercules */
    {{0, "Analog",MUX_0, "adc1", I2C_ADC_CH0, "dac0", DAC_UNCALIBRATED},
     {1, "2.5",   MUX_1, "adc1", I2C_ADC_CH1, "dac0", DAC_UNCALIBRATED},
     {2, "Core",  MUX_2, "adc1", I2C_ADC_CH2, "dac0", DAC_UNCALIBRATED},
     {0},{0}},
    /* Firebolt SDK design: aka G13 */
    {{0, "Core",  MUX_0, "adc0", I2C_ADC_CH0, "dac0", DAC_UNCALIBRATED},
     {1, "2.5",   MUX_1, "adc0", I2C_ADC_CH1, "dac0", DAC_UNCALIBRATED},
     {2, "3.3",   MUX_2, "adc0", I2C_ADC_CH2, "dac0", DAC_UNCALIBRATED},
     {3, "Analog",MUX_3, "adc0", I2C_ADC_CH3, "dac0", DAC_UNCALIBRATED},
     {0}},
    /* Goldwing SDK design: aka G15 */
    {{0, "Core",  MUX_0, "adc0", I2C_ADC_CH3, "dac0", DAC_UNCALIBRATED},
     {1, "Analog",MUX_2, "adc0", I2C_ADC_CH0, "dac0", DAC_UNCALIBRATED},
     {0},{0},{0}},
    /* Bradley 1G SDK design: aka G16 */
    {{0, "Analog",MUX_0, "adc0", I2C_ADC_CH3, "dac0", DAC_UNCALIBRATED},
     {1, "2.5",   MUX_1, "adc0", I2C_ADC_CH1, "dac0", DAC_UNCALIBRATED},
     {2, "1.2",   MUX_3, "adc0", I2C_ADC_CH2, "dac0", DAC_UNCALIBRATED},
     {3, "Core",  MUX_2, "adc0", I2C_ADC_CH0, "dac0", DAC_UNCALIBRATED},
     {0}},
    /* Bradley SDK design: aka G17 */
    {{0, "Core",  MUX_0, "adc0", I2C_ADC_CH0, "dac0", DAC_UNCALIBRATED},
     {1, "Analog",MUX_2, "adc0", I2C_ADC_CH3, "dac0", DAC_UNCALIBRATED},
     {0},{0},{0}}, /* Note: MUX_3 is used for clock control on this board */
    /* Raptor SDK G18 */        
    {{0, "Core",  MUX_0, "adc0", I2C_ADC_CH0, "dac0", DAC_UNCALIBRATED},
     {1, "VDD2.5",MUX_1, "adc0", I2C_ADC_CH1, "dac0", DAC_UNCALIBRATED},
     {2, "PHY",   MUX_2, "adc0", I2C_ADC_CH2, "dac0", DAC_UNCALIBRATED},
     {3, "Analog",MUX_3, "adc0", I2C_ADC_CH3, "dac0", DAC_UNCALIBRATED},
     {0}},
    /* Raven SDK G19 */
    {{0, "Core",  MUX_0, "adc0", I2C_ADC_CH0, "dac0", DAC_UNCALIBRATED},
     {1, "VDD2.5",MUX_1, "adc0", I2C_ADC_CH1, "dac0", DAC_UNCALIBRATED},
     {2, "PHY",   MUX_2, "adc0", I2C_ADC_CH2, "dac0", DAC_UNCALIBRATED},
     {3, "Analog",MUX_3, "adc0", I2C_ADC_CH3, "dac0", DAC_UNCALIBRATED},
     {0}},
    /* Triumph  SDK G20 */
    {{0, "Core",  MUX_0, "adc0", I2C_ADC_CH0, "dac0", DAC_UNCALIBRATED},
     {1, "2.5",   MUX_2, "adc0", I2C_ADC_CH1, "dac0", DAC_UNCALIBRATED},
     {2, "1.5",   MUX_1, "adc0", I2C_ADC_CH4, "dac0", DAC_UNCALIBRATED},
     {3, "Analog",MUX_3, "adc0", I2C_ADC_CH3, "dac0", DAC_UNCALIBRATED},
     {0}},
    /* Scorpion SDK G21 */ 
    {{0, "Core",  MUX_0, "adc0", I2C_ADC_CH0, "dac0", DAC_UNCALIBRATED},
     {1, "Analog",MUX_2, "adc0", I2C_ADC_CH3, "dac0", DAC_UNCALIBRATED},
     {0},{0},{0}}, /* Note: MUX_3 is used for clock control on this board */
    /* Apollo SDK G22 */
    {{0, "Core",  MUX_0, "adc0", I2C_ADC_CH0, "dac0", DAC_UNCALIBRATED},
     {1, "2.5",   MUX_1, "adc0", I2C_ADC_CH1, "dac0", DAC_UNCALIBRATED},
     {2, "Analog",MUX_3, "adc0", I2C_ADC_CH3, "dac0", DAC_UNCALIBRATED},
     {0},{0}},
    /* Sirius  SDK G23 */
    {{0, "Core",  MUX_0, "adc0", I2C_ADC_CH0, "dac0", DAC_UNCALIBRATED},
     {1, "2.5",   MUX_2, "adc0", I2C_ADC_CH1, "dac0", DAC_UNCALIBRATED},
     {2, "1.5",   MUX_1, "adc0", I2C_ADC_CH4, "dac0", DAC_UNCALIBRATED},
     {3, "Analog",MUX_3, "adc0", I2C_ADC_CH3, "dac0", DAC_UNCALIBRATED},
     {0}},
    /* Trident SDK G24 */
    {{0, "Core",  1<<MUX_0, "adc0", I2C_ADC_CH0, "pwctrl0", DAC_UNCALIBRATED},
     {1, "PHY",   1<<MUX_2, "adc0", I2C_ADC_CH2, "pwctrl0", DAC_UNCALIBRATED},
     {2, "Analog",1<<MUX_3, "adc0", I2C_ADC_CH3, "pwctrl0", DAC_UNCALIBRATED},
     {0},{0}},
    /* Titan SDK G25 */
    {{0, "Core",  1 << MUX_0, "adc0", I2C_ADC_CH0, "dac0", DAC_UNCALIBRATED},
     {1, "3.3",   1 << MUX_1, "adc0", I2C_ADC_CH5, "dac0", DAC_UNCALIBRATED},
     {2, "5.0",   1 << MUX_2, "adc0", I2C_ADC_CH6, "dac0", DAC_UNCALIBRATED},
     {3, "Analog",1 << MUX_3, "adc0", I2C_ADC_CH3, "dac0", DAC_UNCALIBRATED},
     {0}},
    /* Hurricane SDK G26 */
    {{0, "Core",  1<<MUX_0, "adc0", I2C_ADC_CH0, "dac0", DAC_UNCALIBRATED},
     {1, "PHY",   1<<MUX_2, "adc0", I2C_ADC_CH2, "dac0", DAC_UNCALIBRATED},
     {2, "Analog",MUX_3,    "adc0", I2C_ADC_CH3, "dac0", DAC_UNCALIBRATED},
     {0},{0}},
    /* SHADOW SVK G27 */
    {{0, "Core",  4, "adc0", I2C_ADC_CH0, "dac0", DAC_UNCALIBRATED},
     {1, "1.0",   5, "adc0", I2C_ADC_CH1, "dac0", DAC_UNCALIBRATED},
     {2, "1.8",   7, "adc0", I2C_ADC_CH2, "dac0", DAC_UNCALIBRATED},
     {3, "3.3",   6, "adc0", I2C_ADC_CH3, "dac0", DAC_UNCALIBRATED},
     {0}},
    /* Katana SDK G28 */
    {{0, "Core",  1 << MUX_0, "adc0", I2C_ADC_CH0, "dac0", DAC_UNCALIBRATED},
     {1, "3.3",   1 << MUX_1, "adc0", I2C_ADC_CH5, "dac0", DAC_UNCALIBRATED},
     {2, "1.5",   1 << MUX_2, "adc0", I2C_ADC_CH4, "dac0", DAC_UNCALIBRATED},
     {3, "Analog",1 << MUX_3, "adc0", I2C_ADC_CH3, "dac0", DAC_UNCALIBRATED},
     {0}},
    /* Enduro-2 SDK G29 */
    {{0, "Core",  1 << MUX_0, "adc0", I2C_ADC_CH0, "dac0", DAC_UNCALIBRATED},
     {1, "Analog",1 << MUX_1, "adc0", I2C_ADC_CH3, "dac0", DAC_UNCALIBRATED},
     {2, "3.3",   1 << MUX_3, "adc0", I2C_ADC_CH5, "dac0", DAC_UNCALIBRATED},
     {0},{0}},
    /* Stardust-2 SDK G30 */
    {{0, "Core",  1 << MUX_0, "adc0", I2C_ADC_CH0, "dac0", DAC_UNCALIBRATED},
     {1, "3.3",   1 << MUX_1, "adc0", I2C_ADC_CH5, "dac0", DAC_UNCALIBRATED},
     {2, "Analog",1 << MUX_3, "adc0", I2C_ADC_CH3, "dac0", DAC_UNCALIBRATED},
     {0},{0}},
    /* Enduro-2 SDK G31 */
    {{0, "Core",  1 << MUX_0, "adc0", I2C_ADC_CH0, "dac0", DAC_UNCALIBRATED},
     {1, "3.3",   1 << MUX_1, "adc0", I2C_ADC_CH5, "dac0", DAC_UNCALIBRATED},
     {2, "1.5",   1 << MUX_2, "adc0", I2C_ADC_CH4, "dac0",  DAC_UNCALIBRATED},
     {3, "Analog",1 << MUX_3, "adc0", I2C_ADC_CH3, "dac0",  DAC_UNCALIBRATED},
     {0}},
    /* Trident SDK G33 */
    {{0, "Core",  1<<MUX_0, "adc0", I2C_ADC_CH0, "pwctrl0", DAC_UNCALIBRATED},
     {1, "PHY",   1<<MUX_2, "adc0", I2C_ADC_CH2, "pwctrl0", DAC_UNCALIBRATED},
     {2, "Analog",1<<MUX_3, "adc0", I2C_ADC_CH3, "pwctrl0", DAC_UNCALIBRATED},
     {0},{0}},
    /* Triumph3 Legacy SDK G34 */
    {{0, "Core",  1 << MUX_1, "adc0", I2C_ADC_CH0, "pwctrl0", DAC_UNCALIBRATED},
     {1, "3.3",   1 << MUX_2, "adc0", I2C_ADC_CH5, "dac0",    DAC_UNCALIBRATED},
     {2, "Analog",1 << MUX_3, "adc0", I2C_ADC_CH3, "pwctrl0", DAC_UNCALIBRATED},
     {0},{0}},
    /* Triumph3 SDK G35 */
    {{0, "Core",  1 << MUX_0, "adc0", I2C_ADC_CH0, "pwctrl0", DAC_UNCALIBRATED},
     {1, "3.3",   1 << MUX_1, "adc0", I2C_ADC_CH5, "dac0",    DAC_UNCALIBRATED},
     {2, "1.8",   1 << MUX_2, "adc0", I2C_ADC_CH7, "dac0",    DAC_UNCALIBRATED},
     {3, "Analog",1 << MUX_3, "adc0", I2C_ADC_CH3, "pwctrl0", DAC_UNCALIBRATED},
     {4, "TCAM",  1 << MUX_7, "adc0", I2C_ADC_CH2, "pwctrl0", DAC_UNCALIBRATED},
     {0}},
    /* Caladan3 48G SVK design: aka G37 */
    {{ 0,"1V_Core",   NO_MUX, "adoc0", I2C_ADC_CH0, "adoc0", 0},
     { 0,"1V_Analog", NO_MUX, "adoc0", I2C_ADC_CH1, "adoc0", 0},
     { 0,"1.5",       NO_MUX, "adoc0", I2C_ADC_CH2, "adoc0", 0},
     { 0,"3.3",       NO_MUX, "adoc0", I2C_ADC_CH3, "adoc0", 0},
     { 0,"1.2",       NO_MUX, "adoc0", I2C_ADC_CH4, "adoc0", 0},
     { 0,"Tcam",      NO_MUX, "adoc0", I2C_ADC_CH5, "adoc0", 0},
    },
    /* Caladan3 100G SVK design: aka G37 */
    {{ 0,"1V_Core",   NO_MUX, "adoc0", I2C_ADC_CH0, "adoc0", 0},
     { 0,"1V_Analog", NO_MUX, "adoc0", I2C_ADC_CH1, "adoc0", 0},
     { 0,"1.5",       NO_MUX, "adoc0", I2C_ADC_CH2, "adoc0", 0},
     { 0,"3.3",       NO_MUX, "adoc0", I2C_ADC_CH3, "adoc0", 0},
     { 0,"1.2",       NO_MUX, "adoc0", I2C_ADC_CH4, "adoc0", 0},
     { 0,"Tcam",      NO_MUX, "adoc0", I2C_ADC_CH5, "adoc0", 0},
    },
    /* Trident2 SVK G38 */
    {{0, "Core",  1<<MUX_0, "adc0", I2C_ADC_CH0, "pwctrl2", DAC_UNCALIBRATED},
     {1, "Analog",1<<MUX_1, "adc0", I2C_ADC_CH3, "pwctrl2", DAC_UNCALIBRATED},
     {2, "3.3",   1<<MUX_2, "adc0", I2C_ADC_CH5, "pwctrl2", DAC_UNCALIBRATED},
    },
    /* Trident2 DVT G39 */
    {{0, "Core",  1<<MUX_0, "adc0", I2C_ADC_CH0, "pwctrl2", DAC_UNCALIBRATED},
     {1, "Analog",1<<MUX_1, "adc0", I2C_ADC_CH3, "pwctrl2", DAC_UNCALIBRATED},
     {2, "3.3",   1<<MUX_2, "adc0", I2C_ADC_CH5, "pwctrl2", DAC_UNCALIBRATED},
    },
    /* Katana2 G40 */
    {{0, "Core",  1 << MUX_0, "adc0", I2C_ADC_CH6, "dac0", DAC_UNCALIBRATED},
     {1, "3.3",   1 << MUX_1, "adc0", I2C_ADC_CH5, "dac0", DAC_UNCALIBRATED},
     {2, "1.5",   1 << MUX_2, "adc0", I2C_ADC_CH4, "dac0", DAC_UNCALIBRATED},
     {3, "Analog",1 << MUX_3, "adc0", I2C_ADC_CH7, "dac0", DAC_UNCALIBRATED},
     {0}},
    {{0},{0},{0},{0},{0}}, /* ENG */        
    {{0},{0},{0},{0},{0}}  /* UNK */        
};

/* Map of number of configurable voltages, one per board.
 * Each one of these is the number of voltage parameters
 * (from above) for each table entry for each board.
 * INDEXED_BY_BOARD_TYPE
 */
int bb_vs_config_len[NUM_BOARD_TYPES] = {			
			  3, /* G5 */
			  4, /* G13 */
			  2, /* G15 */
			  4, /* G16 */
			  2, /* G17 */
			  4, /* G18 */
			  4, /* G19 */
			  4, /* G20 */
			  2, /* G21 */
			  4, /* G22 */
			  4, /* G23 */
			  3, /* G24 */
			  4, /* G25 */
			  3, /* G26 */
			  4, /* G27 */
			  4, /* G28 */
                          3, /* G29 */
                          3, /* G30 */
                          4, /* G31 */
                          3, /* G33 */
                          3, /* G34 */
                          5, /* G35 */
                          6, /* G36 */
                          6, /* G37 */
                          3, /* G38 */
                          3, /* G39 */
                          4, /* G40 */
			  0, /* ENG */
			  0  /* UNK */
};

/* All (non-configurable and configurable) voltages, per board.
 * For display only (only name, adc device and channel used).
 * NOTE: The text names which include an asterisk denote a
 *       configurable voltage source. The name in parentheses
 *       should match the name field in the corresponding entry
 *       in the bb_vs_config[] table.
 * INDEXED_BY_BOARD_TYPE
 */
bb_vparams_t
bb_vs_all[NUM_BOARD_TYPES][MAX_VOLTAGES_PER_BOARD] = {
    /* Hercules: aka G5 */
    {{ 0,"VDD_5.0V          ", 0, "adc1", I2C_ADC_CH5, NULL, 0},
     { 0,"VDD_3.3V          ", 0, "adc1", I2C_ADC_CH4, NULL, 0},
     { 0,"VDD_Core (Core) * ", 0, "adc1", I2C_ADC_CH2, NULL, PWR_CORE_A_SRC | PWR_CORECAL_SRC},
     { 0,"VDD_1.2 (Analog) *", 0, "adc1", I2C_ADC_CH0, NULL, PWR_PHY_SRC},
     { 0,"VDD_2.5 (2.5) *   ", 0, "adc1", I2C_ADC_CH1, NULL, 0},     
     { 0,"IVDD_Core         ", 0, "adc0", I2C_ADC_CH0, NULL, PWR_CORE_A_DROP},
     { 0,"IVDD_1.2          ", 0, "adc0", I2C_ADC_CH2, NULL, PWR_PHY_DROP},
     { 0,"ICAL_Core         ", 0, "adc0", I2C_ADC_CH1, NULL, PWR_CORECAL_DROP},
     {0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},
     {0},{0},{0},{0},{0},{0},{0},{0}},
    /* Firebolt SDK design: aka G13 */
    {{ 0,"CORE (Core)*      ", 0, "adc0", I2C_ADC_CH0, NULL, PWR_CORE_A_LOAD},
     { 0,"VDD_2.5V (2.5)*   ", 0, "adc0", I2C_ADC_CH1, NULL, 0},
     { 0,"VDD_3.3V (3.3)*   ", 0, "adc0", I2C_ADC_CH2, NULL, 0},
     { 0,"VDD_1.2V (Analog)*", 0, "adc0", I2C_ADC_CH3, NULL, PWR_PHY_LOAD | PWR_PHY_SRC | PWR_PHYCAL_SRC},
     { 0,"VDD_5V            ", 0, "adc0", I2C_ADC_CH4, NULL, 0},
     { 0,"VDD_3.3V          ", 0, "adc0", I2C_ADC_CH5, NULL, 0},
     { 0,"TP4003            ", 0, "adc0", I2C_ADC_CH6, NULL, 0},
     { 0,"TP4002            ", 0, "adc0", I2C_ADC_CH7, NULL, 0},
     { 0,"ICal_Core         ", 0, "adc1", I2C_ADC_CH0, NULL, PWR_CORECAL_DROP},
     { 0,"IVDD_Core         ", 0, "adc1", I2C_ADC_CH1, NULL, PWR_CORE_A_DROP},
     { 0,"ICal_Analog       ", 0, "adc1", I2C_ADC_CH2, NULL, PWR_PHYCAL_DROP},
     { 0,"IVDD_Analog       ", 0, "adc1", I2C_ADC_CH3, NULL, PWR_PHY_DROP},
     { 0,"Core_SRC          ", 0, "adc1", I2C_ADC_CH5, NULL, PWR_CORE_A_SRC | PWR_CORECAL_SRC},
     { 0,"VDD_2.5           ", 0, "adc1", I2C_ADC_CH6, NULL, 0},
    },
    /* Goldwing SDK design: aka G15 */
    {{ 0,"VDD_1.0V (Analog)*", 0, "adc0", I2C_ADC_CH0, NULL, PWR_PHY_LOAD},
     { 0,"VDD_2.5V          ", 0, "adc0", I2C_ADC_CH1, NULL, 0},
     { 0,"VDD_3.3V          ", 0, "adc0", I2C_ADC_CH2, NULL, 0},
     { 0,"CORE (Core)*      ", 0, "adc0", I2C_ADC_CH3, NULL, PWR_CORE_A_LOAD},
     { 0,"VDD_5V            ", 0, "adc0", I2C_ADC_CH4, NULL, 0},
     { 0,"VDD_3.3V_B        ", 0, "adc0", I2C_ADC_CH5, NULL, 0},
     { 0,"ICal_Core         ", 0, "adc1", I2C_ADC_CH0, NULL, PWR_CORECAL_DROP},
     { 0,"IVDD_Core         ", 0, "adc1", I2C_ADC_CH1, NULL, PWR_CORE_A_DROP},
     { 0,"ICal_Analog       ", 0, "adc1", I2C_ADC_CH2, NULL, PWR_PHYCAL_DROP},
     { 0,"IVDD_Analog       ", 0, "adc1", I2C_ADC_CH3, NULL, PWR_PHY_DROP},
     { 0,"VDD_1.0V_PS       ", 0, "adc1", I2C_ADC_CH5, NULL, PWR_PHY_SRC | PWR_PHYCAL_SRC},
     { 0,"CORE_PS           ", 0, "adc1", I2C_ADC_CH6, NULL, PWR_CORE_A_SRC | PWR_CORECAL_SRC},
    },
    /* Bradley 1G SDK design: aka G16 */
    {{ 0,"CORE (Core)*      ", 0, "adc0", I2C_ADC_CH0, NULL, PWR_CORE_A_LOAD},
     { 0,"VDD_2.5V (2.5)*   ", 0, "adc0", I2C_ADC_CH1, NULL, 0},
     { 0,"VDD_1.2V (1.2)*   ", 0, "adc0", I2C_ADC_CH2, NULL, 0},
     { 0,"VDD_1.0V (Analog)*", 0, "adc0", I2C_ADC_CH3, NULL, PWR_PHY_LOAD},
     { 0,"VDD_5V            ", 0, "adc0", I2C_ADC_CH4, NULL, 0},
     { 0,"VDD_3.3V          ", 0, "adc0", I2C_ADC_CH5, NULL, 0},
     { 0,"ICal_Core         ", 0, "adc1", I2C_ADC_CH0, NULL, PWR_CORECAL_DROP},
     { 0,"IVDD_Core         ", 0, "adc1", I2C_ADC_CH1, NULL, PWR_CORE_A_DROP},
     { 0,"ICal_Analog       ", 0, "adc1", I2C_ADC_CH2, NULL, PWR_PHYCAL_DROP},
     { 0,"IVDD_Analog       ", 0, "adc1", I2C_ADC_CH3, NULL, PWR_PHY_DROP},
     { 0,"VDD_1.0V_PS       ", 0, "adc1", I2C_ADC_CH4, NULL, PWR_PHY_SRC | PWR_PHYCAL_SRC},
     { 0,"CORE_PS           ", 0, "adc1", I2C_ADC_CH5, NULL, PWR_CORE_A_SRC | PWR_CORECAL_SRC},
     { 0,"VDD_2.5V_PS       ", 0, "adc1", I2C_ADC_CH6, NULL, 0},
    },
    /* Bradley SDK design: aka G17 */
    {{ 0,"CORE (Core)*      ", 0, "adc0", I2C_ADC_CH0, NULL, PWR_CORE_A_LOAD},
     { 0,"VDD_2.5V          ", 0, "adc0", I2C_ADC_CH1, NULL, 0},
     { 0,"VDD_3.3V          ", 0, "adc0", I2C_ADC_CH2, NULL, 0},
     { 0,"VDD_1.0V (Analog)*", 0, "adc0", I2C_ADC_CH3, NULL, PWR_PHY_LOAD},
     { 0,"VDD_5V            ", 0, "adc0", I2C_ADC_CH4, NULL, 0},
     { 0,"VDD_3.3V_B        ", 0, "adc0", I2C_ADC_CH5, NULL, 0},
     { 0,"ICal_Core         ", 0, "adc1", I2C_ADC_CH0, NULL, PWR_CORECAL_DROP},
     { 0,"IVDD_Core         ", 0, "adc1", I2C_ADC_CH1, NULL, PWR_CORE_A_DROP},
     { 0,"ICal_Analog       ", 0, "adc1", I2C_ADC_CH2, NULL, PWR_PHYCAL_DROP},
     { 0,"IVDD_Analog       ", 0, "adc1", I2C_ADC_CH3, NULL, PWR_PHY_DROP},
     { 0,"CORE_PS           ", 0, "adc1", I2C_ADC_CH5, NULL, PWR_CORE_A_SRC | PWR_CORECAL_SRC},
     { 0,"VDD_1.0V_PS       ", 0, "adc1", I2C_ADC_CH6, NULL, PWR_PHY_SRC | PWR_PHYCAL_SRC},
    },
    /* Raptor SDK design: aka G18 */
    {{ 0,"CORE (Core)*      ", 0, "adc0", I2C_ADC_CH0, NULL, PWR_CORE_A_SRC | PWR_CORECAL_SRC},
     { 0,"VDD_2.5V (VDD2.5)*", 0, "adc0", I2C_ADC_CH1, NULL, 0},
     { 0,"VDD_1.2V/1.8V (Phy)*", 0, "adc0", I2C_ADC_CH2, NULL, 0},
     { 0,"VDD_1.2V (Analog)*", 0, "adc0", I2C_ADC_CH3, NULL, PWR_PHY_SRC | PWR_PHYCAL_SRC},
     { 0,"VDD_3.3V_B        ", 0, "adc0", I2C_ADC_CH5, NULL, 0},
     { 0,"VDD_5V            ", 0, "adc0", I2C_ADC_CH6, NULL, 0},
     { 0,"ICal_Core         ", 0, "adc1", I2C_ADC_CH0, NULL, PWR_CORECAL_DROP},
     { 0,"IVDD_Core         ", 0, "adc1", I2C_ADC_CH1, NULL, PWR_CORE_A_DROP},
     { 0,"ICal_Analog       ", 0, "adc1", I2C_ADC_CH2, NULL, PWR_PHYCAL_DROP},
     { 0,"IVDD_Analog       ", 0, "adc1", I2C_ADC_CH3, NULL, PWR_PHY_DROP},
    },
    /* Raven SDK design: aka G19 */
    {{ 0,"CORE (Core)*      ", 0, "adc0", I2C_ADC_CH0, NULL, 0},
     { 0,"VDD_2.5V (VDD2.5)*", 0, "adc0", I2C_ADC_CH1, NULL, 0},
     { 0,"VDD_1.0V/1.8V(Phy)*", 0, "adc0", I2C_ADC_CH2, NULL, 0},
     { 0,"VDD_1.2V (Analog)*", 0, "adc0", I2C_ADC_CH3, NULL, 0},
     { 0,"VDD_3.3V_B        ", 0, "adc0", I2C_ADC_CH5, NULL, 0},
     { 0,"VDD_5V            ", 0, "adc0", I2C_ADC_CH6, NULL, 0},
    },
    /* Triumph SDK design : aka G20 */
    {{ 0, "CORE (Core)*      ", 0, "adc0", I2C_ADC_CH0, NULL, 0},
     { 0, "VDD_2.5V (2.5)*   ", 0, "adc0", I2C_ADC_CH1, NULL, 0},
     { 0, "VDD_1.8V          ", 0, "adc0", I2C_ADC_CH2, NULL, 0},
     { 0, "VDD_1.2V (Analog)*", 0, "adc0", I2C_ADC_CH3, NULL, 0},
     { 0, "VDD_1.5V (1.5)*   ", 0, "adc0", I2C_ADC_CH4, NULL, 0},
     { 0, "VDD_3.3V          ", 0, "adc0", I2C_ADC_CH5, NULL, 0},
     { 0, "VDD_5V            ", 0, "adc0", I2C_ADC_CH6, NULL, 0},
    }, 
    /* Scorpion SDK design : aka G21 */
    {{ 0, "CORE (Core)*      ", 0, "adc0", I2C_ADC_CH0, NULL, 0},
     { 0, "VDD_2.5V          ", 0, "adc0", I2C_ADC_CH1, NULL, 0},
     { 0, "VDD_3.3V          ", 0, "adc0", I2C_ADC_CH2, NULL, 0},
     { 0, "VDD_1.2V (Analog)*", 0, "adc0", I2C_ADC_CH3, NULL, 0},
     { 0, "VDD_5V            ", 0, "adc0", I2C_ADC_CH4, NULL, 0},
     { 0, "VDD_3.3V          ", 0, "adc0", I2C_ADC_CH5, NULL, 0},
    },    
    /* Apollo SDK design : aka G22 */
    {{ 0, "CORE (Core)*      ", 0, "adc0", I2C_ADC_CH0, NULL, 0},
     { 0, "VDD_2.5V (2.5)*   ", 0, "adc0", I2C_ADC_CH1, NULL, 0},
     { 0, "VDD_1.2V (Analog)*", 0, "adc0", I2C_ADC_CH3, NULL, 0},
     { 0, "VDD_3.3V          ", 0, "adc0", I2C_ADC_CH5, NULL, 0},
     { 0, "VDD_5V            ", 0, "adc0", I2C_ADC_CH6, NULL, 0},
    }, 
    /* Sirius SDK design : aka G23 */
    {{ 0, "CORE (Core)*      ", 0, "adc0", I2C_ADC_CH0, NULL, 0},
     { 0, "VDD_2.5V (2.5)*   ", 0, "adc0", I2C_ADC_CH1, NULL, 0},
     { 0, "VDD_1.8V          ", 0, "adc0", I2C_ADC_CH2, NULL, 0},
     { 0, "VDD_1.2V (Analog)*", 0, "adc0", I2C_ADC_CH3, NULL, 0},
     { 0, "VDD_1.5V (1.5)*   ", 0, "adc0", I2C_ADC_CH4, NULL, 0},
     { 0, "VDD_3.3V          ", 0, "adc0", I2C_ADC_CH5, NULL, 0},
     { 0, "VDD_5V            ", 0, "adc0", I2C_ADC_CH6, NULL, 0},
    }, 
    /* Trident SDK design : aka G24 */
    {{ 0, "CORE (Core)*      ", 0, "adc0", I2C_ADC_CH0, NULL, 0},
     { 0, "VDD_2.5V (2.5)    ", 0, "adc0", I2C_ADC_CH1, NULL, 0},
     { 0, "PHY*              ", 0, "adc0", I2C_ADC_CH2, NULL, 0},
     { 0, "VDD_1.2V (Analog)*", 0, "adc0", I2C_ADC_CH3, NULL, 0},
     { 0, "VDD_1.5V (1.5)    ", 0, "adc0", I2C_ADC_CH4, NULL, 0},
     { 0, "VDD_3.3V          ", 0, "adc0", I2C_ADC_CH5, NULL, 0},
     { 0, "VDD_5V            ", 0, "adc0", I2C_ADC_CH6, NULL, 0},
     { 0, "VDD_1.8V          ", 0, "adc0", I2C_ADC_CH7, NULL, 0},
    },
    /* Titan SDK design : aka G25 */
    {{ 0, "CORE (Core)*      ", 0, "adc0", I2C_ADC_CH0, NULL, 0},
     { 0, "VDD_2.5V (2.5)    ", 0, "adc0", I2C_ADC_CH1, NULL, 0},
     { 0, "NC                ", 0, "adc0", I2C_ADC_CH2, NULL, 0},
     { 0, "VDD_1.0V (Analog)*", 0, "adc0", I2C_ADC_CH3, NULL, 0},
     { 0, "NC                ", 0, "adc0", I2C_ADC_CH4, NULL, 0},
     { 0, "VDD_3.3V (3.3)*   ", 0, "adc0", I2C_ADC_CH5, NULL, 0},
     { 0, "VDD_5V            ", 0, "adc0", I2C_ADC_CH6, NULL, 0},
     { 0, "NC                ", 0, "adc0", I2C_ADC_CH7, NULL, 0},
    },
    /* Hurricane SDK design: aka G26 */
    {{ 0,"CORE (Core)*      ", 0, "adc0", I2C_ADC_CH0, NULL, 0},
     { 0,"VDD_1.2V (Phy)*   ", 0, "adc0", I2C_ADC_CH2, NULL, 0},
     { 0,"VDD_1.2V (Analog)*", 0, "adc0", I2C_ADC_CH3, NULL, 0},
     { 0,"VDD_3.3V_B        ", 0, "adc0", I2C_ADC_CH5, NULL, 0},
     { 0,"VDD_5V            ", 0, "adc0", I2C_ADC_CH6, NULL, 0},
    },
    /* Shadow SDK design : aka G26 */
    {{ 0, "CORE (Core)*      ", 0, "adc0", I2C_ADC_CH0, NULL, 0},
     { 0, "SERDES1.0V (1.0)* ", 0, "adc0", I2C_ADC_CH1, NULL, 0},
     { 0, "SHAD_1P8 (1.8)*   ", 0, "adc0", I2C_ADC_CH2, NULL, 0},
     { 0, "SHAD_3P3 (3.3)*   ", 0, "adc0", I2C_ADC_CH3, NULL, 0},
     { 0, "VDD_2.5V          ", 0, "adc0", I2C_ADC_CH4, NULL, 0},
     { 0, "VDD_3.3V          ", 0, "adc0", I2C_ADC_CH5, NULL, 0},
     { 0, "VDD_5.0V          ", 0, "adc0", I2C_ADC_CH6, NULL, 0},
     { 0, "VDD_1.2V          ", 0, "adc0", I2C_ADC_CH7, NULL, 0},
     },
    /* Katana SDK design: aka G28 */
    {{ 0,"CORE (Core)*      ", 0, "adc0", I2C_ADC_CH0, NULL, 0},
     { 0,"VDD_1V (Analog)*  ", 0, "adc0", I2C_ADC_CH3, NULL, 0},
     { 0,"VDD_1.5V (1.5)*   ", 0, "adc0", I2C_ADC_CH4, NULL, 0},
     { 0,"VDD_3.3V (3.3)*   ", 0, "adc0", I2C_ADC_CH5, NULL, 0},
     { 0,"VDD_5V            ", 0, "adc0", I2C_ADC_CH6, NULL, 0},
    },
    /* Enduro-2 SDK design: aka G29 */
    {{ 0,"CORE (Core)*      ", 0, "adc0", I2C_ADC_CH0, NULL, 0},
     { 0,"VDD_1V (Analog)*  ", 0, "adc0", I2C_ADC_CH3, NULL, 0},
     { 0,"VDD_1.5V          ", 0, "adc0", I2C_ADC_CH4, NULL, 0},
     { 0,"VDD_3.3V (3.3)*   ", 0, "adc0", I2C_ADC_CH5, NULL, 0},
     { 0,"VDD_5V            ", 0, "adc0", I2C_ADC_CH6, NULL, 0},
    },
    /* Stardust-2 SDK design: aka G30 */
    {{ 0,"CORE (Core)*      ", 0, "adc0", I2C_ADC_CH0, NULL, 0},
     { 0,"VDD_2.5V          ", 0, "adc0", I2C_ADC_CH2, NULL, 0},
     { 0,"VDD_1V (Analog)*  ", 0, "adc0", I2C_ADC_CH3, NULL, 0},
     { 0,"VDD_1.5V          ", 0, "adc0", I2C_ADC_CH4, NULL, 0},
     { 0,"VDD_3.3V (3.3)*   ", 0, "adc0", I2C_ADC_CH5, NULL, 0},
     { 0,"VDD_5V            ", 0, "adc0", I2C_ADC_CH6, NULL, 0},
    },
    /* Enduro-2 SDK design: aka G31 */
    {{ 0,"CORE (Core)*      ", 0, "adc0", I2C_ADC_CH0, NULL, 0},
     { 0,"VDD_1V (Analog)*  ", 0, "adc0", I2C_ADC_CH3, NULL, 0},
     { 0,"VDD_1.5V (1.5)*   ", 0, "adc0", I2C_ADC_CH4, NULL, 0},
     { 0,"VDD_3.3V (3.3)*   ", 0, "adc0", I2C_ADC_CH5, NULL, 0},
     { 0,"VDD_5V            ", 0, "adc0", I2C_ADC_CH6, NULL, 0},
    },
    /* Trident SDK design : aka G33 */
    {{ 0, "CORE (Core)*      ", 0, "adc0", I2C_ADC_CH0, NULL, 0},
     { 0, "VDD_2.5V (2.5)    ", 0, "adc0", I2C_ADC_CH1, NULL, 0},
     { 0, "PHY*              ", 0, "adc0", I2C_ADC_CH2, NULL, 0},
     { 0, "VDD_1.2V (Analog)*", 0, "adc0", I2C_ADC_CH3, NULL, 0},
     { 0, "VDD_1.5V (1.5)    ", 0, "adc0", I2C_ADC_CH4, NULL, 0},
     { 0, "VDD_3.3V          ", 0, "adc0", I2C_ADC_CH5, NULL, 0},
     { 0, "VDD_5V            ", 0, "adc0", I2C_ADC_CH6, NULL, 0},
     { 0, "VDD_1.8V          ", 0, "adc0", I2C_ADC_CH7, NULL, 0},
    },
    /* Triumph3 Legacy SDK design: aka G34 */
    {{ 0,"CORE (Core)*      ", 0, "adc0", I2C_ADC_CH0, NULL, 0},
     { 0,"VDD_1V (Analog)*  ", 0, "adc0", I2C_ADC_CH3, NULL, 0},
     { 0,"VDD_2.5V (2.5)    ", 0, "adc0", I2C_ADC_CH1, NULL, 0},
     { 0,"VDD_3.3V (3.3)*   ", 0, "adc0", I2C_ADC_CH5, NULL, 0},
     { 0,"VDD_5V            ", 0, "adc0", I2C_ADC_CH6, NULL, 0},
    },
    /* Triumph3 SDK design: aka G35 */
    {{ 0,"CORE (Core)*      ", 0, "adc0", I2C_ADC_CH0, NULL, 0},
     { 0,"TCAM_0.9V (Tcam)* ", 0, "adc0", I2C_ADC_CH2, NULL, 0},
     { 0,"VDD_1V (Analog)*  ", 0, "adc0", I2C_ADC_CH3, NULL, 0},
     { 0,"VDD_1.8V (1.8)*   ", 0, "adc0", I2C_ADC_CH7, NULL, 0},
     { 0,"VDD_2.5V (2.5)    ", 0, "adc0", I2C_ADC_CH1, NULL, 0},
     { 0,"VDD_3.3V (3.3)*   ", 0, "adc0", I2C_ADC_CH5, NULL, 0},
     { 0,"VDD_5V            ", 0, "adc0", I2C_ADC_CH6, NULL, 0},
    },
    /* Caladan3 48G SVK design: aka G36 */
    {{ 0,"DUT_1VD  (1V_Core)*   ", 0, "adoc0", I2C_ADC_CH0, NULL, 0},
     { 0,"DUT_1VA  (1V_Analog)* ", 0, "adoc0", I2C_ADC_CH1, NULL, 0},
     { 0,"DUT_1.5V (1.5)*       ", 0, "adoc0", I2C_ADC_CH2, NULL, 0},
     { 0,"DUT_3.3V (3.3)*       ", 0, "adoc0", I2C_ADC_CH3, NULL, 0},
     { 0,"DUT_1.2V (1.2)*       ", 0, "adoc0", I2C_ADC_CH4, NULL, 0},
     { 0,"TCAM     (Tcam)*      ", 0, "adoc0", I2C_ADC_CH5, NULL, 0},
     { 0,"VDD_5V                ", 0, "adc0", I2C_ADC_CH4, NULL, 0},
     { 0,"VDD_3.3V              ", 0, "adc0", I2C_ADC_CH5, NULL, 0},
     { 0,"VDD_1.2V              ", 0, "adc0", I2C_ADC_CH6, NULL, 0},
    },
    /* Caladan3 100G SVK design: aka G37 */
    {{ 0,"DUT_1VD  (1V_Core)*   ", 0, "adoc0", I2C_ADC_CH0, NULL, 0},
     { 0,"DUT_1VA  (1V_Analog)* ", 0, "adoc0", I2C_ADC_CH1, NULL, 0},
     { 0,"DUT_1.5V (1.5)*       ", 0, "adoc0", I2C_ADC_CH2, NULL, 0},
     { 0,"DUT_3.3V (3.3)*       ", 0, "adoc0", I2C_ADC_CH3, NULL, 0},
     { 0,"DUT_1.2V (1.2)*       ", 0, "adoc0", I2C_ADC_CH4, NULL, 0},
     { 0,"TCAM     (Tcam)*      ", 0, "adoc0", I2C_ADC_CH5, NULL, 0},
     { 0,"VDD_5V                ", 0, "adc0", I2C_ADC_CH4, NULL, 0},
     { 0,"VDD_3.3V              ", 0, "adc0", I2C_ADC_CH5, NULL, 0},
    },
    /* Trident2 SVK design : aka G38 */
    {{ 0, "VDD_1V (Core)*    ", 0, "adc0", I2C_ADC_CH0, NULL, 0},
     { 0, "VDD_2.5V (2.5)    ", 0, "adc0", I2C_ADC_CH1, NULL, 0},
     { 0, "NC                ", 0, "adc0", I2C_ADC_CH2, NULL, 0},
     { 0, "VDD_ANA(Analog)*  ", 0, "adc0", I2C_ADC_CH3, NULL, 0},
     { 0, "NC                ", 0, "adc0", I2C_ADC_CH4, NULL, 0},
     { 0, "VDD_3.3V (3.3)*   ", 0, "adc0", I2C_ADC_CH5, NULL, 0},
     { 0, "VDD_5V            ", 0, "adc0", I2C_ADC_CH6, NULL, 0},
     { 0, "NC                ", 0, "adc0", I2C_ADC_CH7, NULL, 0},
    },
    /* Trident2 DVT design : aka G39 */
    {{ 0, "VDD_1V (Core)*    ", 0, "adc0", I2C_ADC_CH0, NULL, 0},
     { 0, "VDD_2.5V (2.5)    ", 0, "adc0", I2C_ADC_CH1, NULL, 0},
     { 0, "NC                ", 0, "adc0", I2C_ADC_CH2, NULL, 0},
     { 0, "VDD_ANA(Analog)*  ", 0, "adc0", I2C_ADC_CH3, NULL, 0},
     { 0, "NC                ", 0, "adc0", I2C_ADC_CH4, NULL, 0},
     { 0, "VDD_3.3V (3.3)*   ", 0, "adc0", I2C_ADC_CH5, NULL, 0},
     { 0, "VDD_5V            ", 0, "adc0", I2C_ADC_CH6, NULL, 0},
     { 0, "NC                ", 0, "adc0", I2C_ADC_CH7, NULL, 0},
    },
    /* Katana2 design : aka G40 */
    {{ 0, "VDD_5V            ", 0, "adc0", I2C_ADC_CH1, NULL, 0},
     { 0, "VDD_1.5V (1.5)*   ", 0, "adc0", I2C_ADC_CH4, NULL, 0},
     { 0, "VDD_3.3V (3.3)*   ", 0, "adc0", I2C_ADC_CH5, NULL, 0},
     { 0, "CORE (Core)*      ", 0, "adc0", I2C_ADC_CH6, NULL, 0},
     { 0, "VDD_1V (Analog)*  ", 0, "adc0", I2C_ADC_CH7, NULL, 0},
    },
    /* ENG */
    {{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},
     {0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0}},
    /* UNK */
    {{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},
     {0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0}}
};

/* MAP of voltage display table lengths.
 * One per board, each one is the number of table entries in the
 * table above.
 * INDEXED_BY_BOARD_TYPE
 */
int bb_vs_all_len[] = {
    8,  /* G5 */
    14, /* G13 */
    12, /* G15 */
    13, /* G16 */
    12, /* G17 */
    10, /* G18 */
    6,  /* G19 */
    7,  /* G20 */    
    6,  /* G21 */    
    5,  /* G22 */    
    7,  /* G23 */    
    8,  /* G24 */    
    8,  /* G25 */    
    5,  /* G26 */
    8,  /* G27 */
    5,  /* G28 */
    5,  /* G29 */
    6,  /* G30 */
    5,  /* G31 */
    8,  /* G33 */    
    5,  /* G34 */
    7,  /* G35 */
    9,  /* G36 */
    8,  /* G37 */
    8,  /* G38 */
    8,  /* G39 */
    5,  /* G40 */
    0,  /* ENG */
    0   /* UNK */
};


/* All Currents, per board.
 * For display only (only name, adc device and channel used).
 * The first element *Must* be the Reference voltage.
 * INDEXED_BY_BOARD_TYPE
 */
bb_current_config_t
bb_current[NUM_BOARD_TYPES][MAX_VOLTAGES_PER_BOARD] = {
    /* Hercules: aka G5 */
    {{0}},
    /* Firebolt SDK design: aka G13 */
    {{0}},
    /* Goldwing SDK design: aka G15 */
    {{0}},
    /* Bradley 1G SDK design: aka G16 */
    {{0}},
    /* Bradley SDK design: aka G17 */
    {{0}},
    /* Raptor SDK design: aka G18 */
    {{0}},
    /* Raven SDK design: aka G19 */
    {{0}},
    /* Triumph SDK design : aka G20 */
    {{0}}, 
    /* Scorpion SDK design : aka G21 */
    {{0}},    
    /* Apollo SDK design : aka G22 */
    {{0}}, 
    /* Sirius SDK design : aka G23 */
    {{0}}, 
    /* Trident SDK design : aka G24 */
    {{0}},
    /* Titan SDK design : aka G25 */
    {{0}},
    /* Hurricane SDK design: aka G26 */
    {{0}},
    /* Shadow SDK design : aka G26 */
    {{0}},
    /* Katana SDK design: aka G28 */
    {{0}},
    /* Enduro-2 SDK design: aka G29 */
    {{0}},
    /* Stardust-2 SDK design: aka G30 */
    {{0}},
    /* Enduro-2 SDK design: aka G31 */
    {{0}},
    /* Trident SDK design : aka G33 */
    {{0}},
    /* Triumph3 Legacy SDK design: aka G34 */
    {{0}},
    /* Triumph3 SDK design: aka G35 */
    {{0}},
    /* Caladan3 48G SVK design: aka G36 */
    {{0}},
    /* Caladan3 100G SVK design: aka G37 */
    {{0}},
    /* Trident2 SVK design : aka G38 */
    {{0}},
    /* Trident2 DVT design : aka G39 */
    {{0}},
    /* Katana2 design : aka G40 */
    {{ "VDD_5V       ", "adc0", I2C_ADC_CH1,   0, 0},
     { "CURR_1.5     ", "adc0", I2C_ADC_CH0, 175, 0},
     { "CURR_CORE    ", "adc0", I2C_ADC_CH2,  66, 0},
     { "CURR_ANALOG  ", "adc0", I2C_ADC_CH3, 175, 0},
    },
    /* ENG */
    {{0}},
    /* UNK */
    {{0}}
};


/* MAP of Current display table lengths.
 * One per board, each one is the number of table entries in the
 * table above.
 * INDEXED_BY_BOARD_TYPE
 */
int bb_current_len[] = {
    0,  /* G5 */
    0,  /* G13 */
    0,  /* G15 */
    0,  /* G16 */
    0,  /* G17 */
    0,  /* G18 */
    0,  /* G19 */
    0,  /* G20 */    
    0,  /* G21 */    
    0,  /* G22 */    
    0,  /* G23 */    
    0,  /* G24 */    
    0,  /* G25 */    
    0,  /* G26 */
    0,  /* G27 */
    0,  /* G28 */
    0,  /* G29 */
    0,  /* G30 */
    0,  /* G31 */
    0,  /* G33 */    
    0,  /* G34 */
    0,  /* G35 */
    0,  /* G36 */
    0,  /* G37 */
    0,  /* G38 */
    0,  /* G39 */
    4,  /* G40 */
    0,  /* ENG */
    0   /* UNK */
};



#ifdef BCM_CALADAN3_SVK

bb_vparams_t
bb_cal3_current[MAX_VOLTAGES_PER_BOARD] = {
    /* Caladan3 SVK : aka G36 */
     { 0,"VDD_5V       ", 0, "adc0", I2C_ADC_CH4, NULL, 0},
     { 0,"CORE         ", 0, "adc0", I2C_ADC_CH0, NULL, 0},
     { 0,"DUT_1V       ", 0, "adc0", I2C_ADC_CH1, NULL, 0},
     { 0,"DUT_1.5V     ", 0, "adc0", I2C_ADC_CH2, NULL, 0},
     { 0,"DUT_3.3V     ", 0, "adc0", I2C_ADC_CH3, NULL, 0},
     { 0 },
};

/*
 * Show the current sensed
 * Parameters:
 *    unit - bcm unit
 *    bx - baseboard index
 */
int bb_caladan3_current(int unit, int bx)
{

    int i, adc = -1;
    i2c_adc_t  adc_data;
    int bus = 0;
    int board_rev_a = 0;
    int cpu_mode = 0;
#ifdef COMPILER_HAS_DOUBLE
    double err = 0.0, slope = 0, ical;
#else
    int err = 0, slope = 0, ical;
#endif

    bus = cpu_i2c_defaultbus();

    /*
     * index 0, is the VDD ref voltage, this is used to compute the error, donot change this! 
     */
    for(i = 0; bb_cal3_current[i].adc ; i++){
        if (soc_caladan3_is_board_rev_a()) {
             /* On rev A, mux is on dut bus */
             cpu_mode = (sal_strncasecmp(bb_vs_all[bx][i].adc, "adc", 3) == 0) ? 0 : 1;
             board_rev_a = I2C_ADC_BOARD_TYPE_C3_REV_A;
        } else {
             cpu_mode = 1;
             board_rev_a = I2C_ADC_BOARD_TYPE_C3_REV_B;
        }
        if (cpu_mode) {
            /* Cpu mode */
            adc = cpu_i2c_dev_open(bus, bb_cal3_current[i].adc, 0, 0);
            if (adc < 0) {
                cli_out("Device open failed for %s\n", bb_vs_all[bx][i].adc);
                return (CMD_FAIL);
            }
            if ( cpu_i2c_dev_ioctl(bus, adc, I2C_ADC_SET_BOARD_TYPE,
                                &board_rev_a, bb_cal3_current[i].chan) < 0) {
                cli_out("Error: failed to set board type.\n");
            }
            if ( cpu_i2c_dev_ioctl(bus, adc, I2C_ADC_QUERY_CHANNEL,
                                &adc_data, bb_cal3_current[i].chan) < 0) {
                cli_out("Error: failed to perform A/D conversions.\n");
            }

        } else {
            if ( (adc = bcm_i2c_open(unit,
                                     bb_cal3_current[i].adc,
                                     0, 0)) < 0) {
                       cli_out("Could not open %s:%s\n",
                               bb_vs_all[bx][i].adc,
                               bcm_errmsg(adc));
                       return CMD_FAIL;
            }
            if ( (bcm_i2c_ioctl(unit, adc, I2C_ADC_SET_BOARD_TYPE,
                                &board_rev_a, bb_cal3_current[i].chan) < 0)) {
                cli_out("Error: failed to set board type.\n");
            }
            /* Display voltage */
            if ( (bcm_i2c_ioctl(unit, adc, I2C_ADC_QUERY_CHANNEL,
                                &adc_data, bb_cal3_current[i].chan) < 0)) {
                cli_out("Error: failed to perform A/D conversions.\n");
            }
        } /* cpu mode */

#ifdef COMPILER_HAS_DOUBLE
        /* From Spec of acs758 */
        if (i == 0) {
            /*err = adc_data.val * 0.1;*/
            err = 0.6;
            slope = 25;
            continue;
        }
        ical = (adc_data.val - err) * slope;
#else
        if (i == 0) {
            /*err = adc_data.val / 100;*/
            err = 0.6;
            slope = 25;
            continue;
        }
        ical = (adc_data.val - err) * slope;
#endif
        if (ical < 0) {
            ical = 0;
        }


#ifdef COMPILER_HAS_DOUBLE
        cli_out("%s%s%-2.3f Amps (ADC: %2.3f)\n",
                bb_cal3_current[i].function,
                ical >= 0 ? "  " : " ",
                ical, adc_data.val);
#else
        cli_out("%s%s%-d.%03d Amps (ADC: %03d) \n",
                bb_cal3_current[i].function,
                ical >= 0 ? "  " : " ",
                INT_FRAC_3PT_3(ical), INT_FRAC_3PT_3(adc_data.val));
#endif

    }

    return CMD_OK;

}

/*
 * Show/set/calibrate the voltages on base board
 * Parameters:
 *    unit - bcm unit
 *    source - voltage source
 *    bx - baseboard index
 */
int bb_caladan3_voltage(int unit, int bx, char *source, char* value)
{

#ifdef COMPILER_HAS_DOUBLE
    double volts;
#else
    int volts;
#endif
    int i, adc = -1, dac = -1;
#if 0
    int show_cal = 0, mux = -1, rv = 0;
#endif
    i2c_adc_t  adc_data;
    int bus = 0;
    int board_rev_a = 0;
    int cpu_mode = 0;
    int cmd = 0, threshold_cmd = 0;
    char threshold[64];


    bus = cpu_i2c_defaultbus();

    /* Show all voltages, note that ADC's are always visible,
     * hence we do not need to set the BBMUX value
     */
    if( ! source){
        for(i = 0; i < bb_vs_all_len[bx]; i++){
            if (soc_caladan3_is_board_rev_a()) {
                 /* On rev A, max127 adc is on dut bus */
                 cpu_mode = (sal_strncasecmp(bb_vs_all[bx][i].adc, "adc", 3) == 0) ? 0 : 1;
                 board_rev_a = I2C_ADC_BOARD_TYPE_C3_REV_A;
            } else {
                 cpu_mode = 1;
                 board_rev_a = I2C_ADC_BOARD_TYPE_C3_REV_B;
            }
            if (cpu_mode) {
                /* Cpu mode */
                adc = cpu_i2c_dev_open(bus, bb_vs_all[bx][i].adc, 0, 0);
                if (adc < 0) {
                    cli_out("Device open failed for %s\n", bb_vs_all[bx][i].adc);
                    return (CMD_FAIL);
                }
                if ( cpu_i2c_dev_ioctl(bus, adc, I2C_ADC_SET_BOARD_TYPE,
                                       &board_rev_a, bb_vs_all[bx][i].chan) < 0) {
                       cli_out("Error: failed to set board type.\n");
                }
                if ( cpu_i2c_dev_ioctl(bus, adc, I2C_ADC_QUERY_CHANNEL,
                                    &adc_data, bb_vs_all[bx][i].chan) < 0) {
                    cli_out("Error: failed to perform A/D conversions.\n");
                }

            } else {
                if ( (adc = bcm_i2c_open(unit,
                                         bb_vs_all[bx][i].adc,
                                         0, 0)) < 0) {
                           cli_out("Could not open %s:%s\n",
                                   bb_vs_all[bx][i].adc,
                                   bcm_errmsg(adc));
                           return CMD_FAIL;
                }
                if ( (bcm_i2c_ioctl(unit, adc, I2C_ADC_SET_BOARD_TYPE,
                                    &board_rev_a, bb_vs_all[bx][i].chan) < 0)) {
                    cli_out("Error: failed to set Board type\n");
                }
                /* Display voltage */
                if ( (bcm_i2c_ioctl(unit, adc, I2C_ADC_QUERY_CHANNEL,
                                    &adc_data, bb_vs_all[bx][i].chan) < 0)) {
                    cli_out("Error: failed to perform A/D conversions.\n");
                }
            } /* cpu mode */


#ifdef COMPILER_HAS_DOUBLE
            cli_out("%-.20s\t%2.3f Volts \n",
                    bb_vs_all[bx][i].function,
                    adc_data.val);
#else
            cli_out("%-.20s\t%d.%03d Volts \n",
                    bb_vs_all[bx][i].function,
                    INT_FRAC_3PT_3(adc_data.val));
#endif

        }

        return CMD_OK;

    } /* Show all voltages */

    /* Here if a Voltage Source was specified.
     * Show or configure just that one voltage.
     * This currently works only for configurable voltages.
     */

    /* First, find voltage table entry ... */
    for( i = 0; i < bb_vs_config_len[bx]; i++) {
        if ( !sal_strcasecmp(bb_vs_config[bx][i].function, source))
            break;
    }
    /* Not found - ERROR! */
    if(i == bb_vs_config_len[bx]){
        cli_out("Unknown Voltage source: %s\n", source);
        return CMD_USAGE;
    }

    if (value) {

        /*
         * Going to calibrate and/or set a voltage;
         * open the correct DAC
         */
        if (cpu_mode) {
            if ( (dac = cpu_i2c_dev_open(bus, bb_vs_config[bx][i].dac,
                                       0, 0)) < 0) {
                
                cli_out("Could not open %s:%s\n",
                        bb_vs_config[bx][i].dac,
                        bcm_errmsg(dac));
                return CMD_FAIL;
            }
        } else
        {
            if ( (dac = bcm_i2c_open(unit, bb_vs_config[bx][i].dac,
                                      0, 0)) < 0) {
                
                cli_out("Could not open %s:%s\n",
                        bb_vs_config[bx][i].dac,
                        bcm_errmsg(dac));
                return CMD_FAIL;
            }
        }

    }

    /* Open the ADC Device */
    if (soc_caladan3_is_board_rev_a()) {
         /* On rev A, max127 adc is on dut bus */
         cpu_mode = (sal_strncasecmp(bb_vs_all[bx][i].adc, "adc", 3) == 0) ? 0 : 1;
    } else {
         cpu_mode = 1;
    }
    if (cpu_mode) {
        if ( (adc = cpu_i2c_dev_open(bus, bb_vs_config[bx][i].adc,
                                 0, 0)) < 0) {
            cli_out("Could not open %s:%s\n",
                    bb_vs_config[bx][i].adc,
                    bcm_errmsg(adc));
            return CMD_FAIL;
        }
    } else
    {

        if ( (adc = bcm_i2c_open(unit, bb_vs_config[bx][i].adc,
                                 0, 0)) < 0) {
            cli_out("Could not open %s:%s\n",
                    bb_vs_config[bx][i].adc,
                    bcm_errmsg(adc));
            return CMD_FAIL;
        }
    }

    /* From this point on, we have an ADC (and possibly its
     * associated DAC) open and ready for commands.
     */

    /* Set and/or possibly calibrate voltage, we need to do this
     * at least once (the first time), and possibly later if the
     * user requests a calibration of a source ...
     */
    if( value ) {

#if 0

        /* Always designate the DAC calibration table. This info
         * is needed for any voltage setting operations. If
         * a calibration is performed, the table is updated.
         */
        if (cpu_mode) {
            if ( (cpu_i2c_dev_ioctl(bus, dac,
                                I2C_DAC_IOC_SET_CALTAB,
                                (void*)dac_params[bx],
                                dac_param_len[bx]) < 0)) {
                cli_out("Error: failed to set DAC calibration table.\n");
            }
        } else
        {
            if ( (bcm_i2c_ioctl(unit, dac,
                                I2C_DAC_IOC_SET_CALTAB,
                                (void*)dac_params[bx],
                                dac_param_len[bx]) < 0)) {
                cli_out("Error: failed to set DAC calibration table.\n");
            }
        }

        /* Calibrate the DAC->ADC loop
         * (if not yet done, or explicitly requested).
         */
        if ( (show_cal = !sal_strcasecmp(value,"calibrate")) ||
             (show_cal = !sal_strcasecmp(value,"cal")) ||
            !(show_cal = bb_vs_config[bx][i].flags & DAC_CALIBRATED) ) {

            if (show_cal) {
                cli_out("calibration: board %d: subsys%d\n",bx, i);
            }

            /* We need to calibrate the DAC to compute the min,
             * max, and the Volts/step values that result at the
             * associated ADC.
             */
            if (cpu_mode) {
                if ( (cpu_i2c_dev_ioctl(bus, dac,
                                    I2C_DAC_IOC_SETDAC_MIN,
                                    &volts, bb_vs_config[bx][i].chan) < 0)) {
                    cli_out("Error: failed to set min DAC Vout.\n");
                }

                /* Read the resultant (minimum) voltage at the ADC. */
                if ( (cpu_i2c_dev_ioctl(bus, adc, I2C_ADC_QUERY_CHANNEL,
                                    &adc_data,  bb_vs_config[bx][i].chan) < 0)) {
                    cli_out("Error: failed to read A/D min voltage:VDD_%s.\n",
                            bb_vs_config[bx][i].function);
                }
            } else
            {
                /* Set the DAC to the (digital) value which         */
                /* generates the minimum analog voltage at the ADC. */
                if ( (bcm_i2c_ioctl(unit, dac,
                                    I2C_DAC_IOC_SETDAC_MIN,
                                    &volts, bb_vs_config[bx][i].chan) < 0)) {
                    cli_out("Error: failed to set min DAC Vout.\n");
                }

                /* Read the resultant (minimum) voltage at the ADC. */
                if ( (bcm_i2c_ioctl(unit, adc, I2C_ADC_QUERY_CHANNEL,
                                    &adc_data, bb_vs_config[bx][i].chan) < 0)) {
                    cli_out("Error: failed to read A/D min voltage:VDD_%s.\n",
                            bb_vs_config[bx][i].function);
                }
            }
            volts = adc_data.val;

            if (show_cal) {
#ifdef COMPILER_HAS_DOUBLE
                cli_out("Min=%2.3fV, ", volts);
#else
                cli_out("Min=%d.%03dV, ", INT_FRAC_3PT_3(volts));
#endif
            }

            /* Update DAC calibration table with this minimum voltage. */
            if (cpu_mode) {
                if ( (cpu_i2c_dev_ioctl(bus, dac,
                                    I2C_DAC_IOC_CALIBRATE_MIN,
                                    &volts, bb_vs_config[bx][i].chan) < 0)) {
                    cli_out("Error: failed to update min DAC Vout.\n");
                }
            } else
            {
                if ( (bcm_i2c_ioctl(unit, dac,
                                    I2C_DAC_IOC_CALIBRATE_MIN,
                                    &volts, bb_vs_config[bx][i].chan) < 0)) {
                    cli_out("Error: failed to update min DAC Vout.\n");
                }
            }

            /* Set the DAC to the (digital) value which
             * generates the maximum analog voltage at the ADC.
             */
            if (cpu_mode) {
                if ( (cpu_i2c_dev_ioctl(bus, dac,
                                    I2C_DAC_IOC_SETDAC_MAX,
                                    &volts, bb_vs_config[bx][i].chan) < 0)) {
                    cli_out("Error: failed to set max DAC Vout.\n");
                }

                /* Read the resultant (maximum) voltage at the ADC. */
                if ( (cpu_i2c_dev_ioctl(bus, adc, I2C_ADC_QUERY_CHANNEL,
                                       &adc_data, bb_vs_config[bx][i].chan) < 0)) {
                    cli_out("Error: failed to read A/D max voltage:VDD_%s.\n",
                            bb_vs_config[bx][i].function);
                }
            } else
            {
                if ( (bcm_i2c_ioctl(unit, dac,
                                    I2C_DAC_IOC_SETDAC_MAX,
                                    &volts, bb_vs_config[bx][i].chan) < 0)) {
                    cli_out("Error: failed to set max DAC Vout.\n");
                }

                /* Read the resultant (maximum) voltage at the ADC. */
                if ( (bcm_i2c_ioctl(unit, adc, I2C_ADC_QUERY_CHANNEL,
                                    &adc_data, bb_vs_config[bx][i].chan) < 0)) {
                    cli_out("Error: failed to read A/D max voltage:VDD_%s.\n",
                            }
                            }
                            volts = adc_data.val;
                            
                            if (show_cal)  {
                            #ifdef COMPILER_HAS_DOUBLE
                            cli_out("Max=%2.3fV\n", volts);
                            #else
                            cli_out("Max=%d.%03dV\n", INT_FRAC_3PT_3(volts));
                            #endif
                            }
                            
                            
                            /*
                            * Update DAC calibration table with this maximum voltage.
                            */
                            if (cpu_mode) {
                            if ( (cpu_i2c_dev_ioctl(bus, dac,
                            I2C_DAC_IOC_CALIBRATE_MAX,
                            &volts, bb_vs_config[bx][i].chan) < 0)) {
                            cli_out("Error: failed to update max DAC Vout.\n");
                            }
                            } else
                            {
                            
                            if ( (bcm_i2c_ioctl(unit, dac,
                            I2C_DAC_IOC_CALIBRATE_MAX,
                            &volts, bb_vs_config[bx][i].chan) < 0)) {
                            cli_out("Error: failed to update max DAC Vout.\n");
                            }
                            }
                            
                            /* Very important, set the DAC back to nominal
                            * (bootup midscale).
                            */
                            if (cpu_mode) {
                            if( (rv = cpu_i2c_dev_ioctl(bus, dac,
                            I2C_DAC_IOC_SETDAC_MID,
                            &volts, bb_vs_config[bx][i].chan) < 0)) {
                            cli_out("Error: resetting of DAC to nominal value failed:%s\n",
                                    bcm_errmsg(rv));
                            return CMD_FAIL;
                            }
                            } else
                            {
                            if( (rv = bcm_i2c_ioctl(unit, dac,
                            I2C_DAC_IOC_SETDAC_MID,
                            &volts, bb_vs_config[bx][i].chan) < 0)) {
                            cli_out("Error: resetting of DAC to nominal value failed:%s\n",
                                    bcm_errmsg(rv));
                            return CMD_FAIL;
                            }
                            }
                            
                            
                            /* Now, update the calibration table with the
                            * analog voltage step per DAC digital step.
                            * (also displays all of the calibration table info)
                            */
                            if (cpu_mode) {
                            if ( (cpu_i2c_dev_ioctl(bus, dac,
                            I2C_DAC_IOC_CALIBRATE_STEP,
                            &volts, bb_vs_config[bx][i].chan) < 0)) {
                            cli_out("Error: failed to calibrate DAC VDD_%s.\n",
                                    bb_vs_config[bx][i].function);
                            }
                            } else
                            {
                            if ( (bcm_i2c_ioctl(unit, dac,
                            I2C_DAC_IOC_CALIBRATE_STEP,
                            &volts, bb_vs_config[bx][i].chan) < 0)) {
                            cli_out("Error: failed to calibrate DAC VDD_%s.\n",
                                    bb_vs_config[bx][i].function);
                            }
                            }
                            
                            /* Indicate that this DAC-ADC pair is calibrated */
                            bb_vs_config[bx][i].flags = DAC_CALIBRATED;
                            } /* DAC->ADC not yet calibrated or calibration was requested */
                            #endif
                            /*
                            * Get the user's desired analog voltage value to set.
                            */
                            
                            if (!sal_strncasecmp(value,"nominal",sal_strlen(value))) {
                            cli_out("Resetting %s voltage to nominal\n", source);
                            
                            if (cpu_mode) {
                            if ( (cpu_i2c_dev_ioctl(bus, dac,
                            I2C_SMM_SET_NOMINAL,
                            &volts, bb_vs_config[bx][i].chan) < 0)) {
                            cli_out("Error: failed to reset VDD_%s DAC to mid-range value.\n",
                                    bb_vs_config[bx][i].function);
                            return CMD_FAIL;
                            }
                            
                            /* Read the ADC to get the actual resultant voltage value. */
                            if ( (cpu_i2c_dev_ioctl(bus, adc, I2C_ADC_QUERY_CHANNEL,
                            &adc_data, bb_vs_config[bx][i].chan) < 0)) {
                            cli_out("Error: failed to perform A/D conversions.\n");
                }
            } else
            {
                if ( (bcm_i2c_ioctl(unit, dac,
                                    I2C_SMM_SET_NOMINAL,
                                    &volts, bb_vs_config[bx][i].chan) < 0)) {
                    cli_out("Error: failed to reset VDD_%s DAC to mid-range value.\n",
                            bb_vs_config[bx][i].function);
                    return CMD_FAIL;
                }

                /* Read the ADC to get the actual resultant voltage value. */
                if ( (bcm_i2c_ioctl(unit, adc, I2C_ADC_QUERY_CHANNEL,
                                    &adc_data, bb_vs_config[bx][i].chan) < 0)) {
                    cli_out("Error: failed to perform A/D conversions.\n");
                }
            }

            return CMD_OK;
        }

        /*
         * Allow setting Margin high and Margin low
         */
       if (!sal_strncasecmp(value,"high",sal_strlen(value))) {
           cli_out("Shifting %s voltage to Margin High\n", source);
           cmd = I2C_SMM_SET_MARGIN_HIGH;
       } else if (!sal_strncasecmp(value,"low",sal_strlen(value))) {
           cli_out("Shifting %s voltage to Margin low\n", source);
           cmd = I2C_SMM_SET_MARGIN_LOW;
       }
       if (cmd > 0) {
            if (cpu_mode) {
                if ( (cpu_i2c_dev_ioctl(bus, dac,
                                    cmd,
                                    &volts, bb_vs_config[bx][i].chan) < 0)) {
                    cli_out("Error: failed to reset VDD_%s DAC to mid-range value.\n",
                            bb_vs_config[bx][i].function);
                    return CMD_FAIL;
                }

                /* Read the ADC to get the actual resultant voltage value. */
                if ( (cpu_i2c_dev_ioctl(bus, adc, I2C_ADC_QUERY_CHANNEL,
                                        &adc_data, bb_vs_config[bx][i].chan) < 0)) {
                    cli_out("Error: failed to perform A/D conversions.\n");
                }
            } else
            {
                if ( (bcm_i2c_ioctl(unit, dac,
                                    cmd,
                                    &volts, bb_vs_config[bx][i].chan) < 0)) {
                    cli_out("Error: failed to reset VDD_%s DAC to %s value.\n",
                            bb_vs_config[bx][i].function, value);
                    return CMD_FAIL;
                }

                /* Read the ADC to get the actual resultant voltage value. */
                if ( (bcm_i2c_ioctl(unit, adc, I2C_ADC_QUERY_CHANNEL,
                                    &adc_data, bb_vs_config[bx][i].chan) < 0)) {
                    cli_out("Error: failed to perform A/D conversions.\n");
                }
            }
            return CMD_OK;
        }

        /* Allow setting Nominal, Margin high and Margin Low threshold values */
        if (!sal_strncasecmp(value,"low=",4))  {
            sal_sprintf(threshold, "%s", value+4);
            cli_out("Setting %s Margin low voltage to %s\n", source, threshold);
            threshold_cmd = I2C_SMM_CFG_MARGIN_LOW;
        } else if (!sal_strncasecmp(value,"high=",5))  {
            sal_sprintf(threshold, "%s", value+5);
            cli_out("Setting %s Margin high voltage to %s\n", source, threshold);
            threshold_cmd = I2C_SMM_CFG_MARGIN_HIGH;
        } else if (!sal_strncasecmp(value,"nominal=",8))  {
            sal_sprintf(threshold, "%s", value+8);
            cli_out("Setting %s Nominal voltage to %s\n", source, threshold);
            threshold_cmd = I2C_SMM_CFG_NOMINAL ;
        }

        if (threshold_cmd > 0) {
#ifdef COMPILER_HAS_DOUBLE
            volts = atof(threshold);
#else
            /* Parse input in V and convert to uV */
            volts = _shr_atof_exp10(threshold, 6);
#endif
            if (cpu_mode) {
                if ( (cpu_i2c_dev_ioctl(bus, dac,
                                    threshold_cmd,
                                    &volts, bb_vs_config[bx][i].chan) < 0)) {
                    cli_out("Error: failed to reset VDD_%s DAC to mid-range value.\n",
                            bb_vs_config[bx][i].function);
                    return CMD_FAIL;
                }
            } else
            {
                if ( (bcm_i2c_ioctl(unit, dac,
                                    I2C_DAC_IOC_SETDAC_MID,
                                    &volts, bb_vs_config[bx][i].chan) < 0)) {
                    cli_out("Error: failed to reset VDD_%s DAC to mid-range value.\n",
                            bb_vs_config[bx][i].function);
                    return CMD_FAIL;
                }
            }
            return CMD_OK;
         }

        /* Here if a voltage level was specified */
        
#ifdef COMPILER_HAS_DOUBLE
        volts = atof(value);
#else
        /* Parse input in V and convert to uV */
        volts = _shr_atof_exp10(value, 6);
#endif
        if(volts > 0){

            cli_out("Configuring voltage on [%s]\n", source);

            /* Finally, set the actual voltage ... */

            if (cpu_mode) {
                if ( (cpu_i2c_dev_ioctl(bus, dac,
                                    I2C_DAC_IOC_SET_VOUT,
                                    &volts, bb_vs_config[bx][i].chan) < 0)) {
#ifdef COMPILER_HAS_DOUBLE
                    cli_out("Error: failed to set VDD_%s to %2.3fV.\n",
                            bb_vs_config[bx][i].function, volts);
#else
                    cli_out("Error: failed to set VDD_%s to %d.%03dV.\n",
                            bb_vs_config[bx][i].function, INT_FRAC_3PT_3(volts));
#endif
                    cli_out("Run \"bb voltage %s calibrate\" to view allowable range.\n",
                            bb_vs_config[bx][i].function);

                    return CMD_FAIL;
                }

                
                /* Read the ADC to get the actual resultant voltage value. */
                if ( (cpu_i2c_dev_ioctl(bus, adc, I2C_ADC_QUERY_CHANNEL,
                                        &adc_data, bb_vs_config[bx][i].chan) < 0)) {
                    cli_out("Error: failed to perform A/D conversions.\n");
                }
            } else
            {
                if ( (bcm_i2c_ioctl(unit, dac,
                                    I2C_DAC_IOC_SET_VOUT,
                                    &volts, bb_vs_config[bx][i].chan) < 0)) {
#ifdef COMPILER_HAS_DOUBLE
                    cli_out("Error: failed to set VDD_%s to %2.3fV.\n",
                            bb_vs_config[bx][i].function, volts);
#else
                    cli_out("Error: failed to set VDD_%s to %d.%03dV.\n",
                            bb_vs_config[bx][i].function, INT_FRAC_3PT_3(volts));
#endif
                    cli_out("Run \"bb voltage %s calibrate\" to view allowable range.\n",
                            bb_vs_config[bx][i].function);

                    return CMD_FAIL;
                }

                
                /* Read the ADC to get the actual resultant voltage value. */
                if ( (bcm_i2c_ioctl(unit, adc, I2C_ADC_QUERY_CHANNEL,
                                    &adc_data, bb_vs_config[bx][i].chan) < 0)) {
                    cli_out("Error: failed to perform A/D conversions.\n");
                }
            }
        }
    } /* Set a single voltage */
    else 
    {

        /* Display a single voltage */
        if (soc_caladan3_is_board_rev_a()) {
             /* On rev A, max127 adc is on dut bus */
             cpu_mode = (sal_strncasecmp(bb_vs_all[bx][i].adc, "adc", 3) == 0) ? 0 : 1;
        } else {
             cpu_mode = 1;
        }

        if (cpu_mode) {
            if ( (cpu_i2c_dev_ioctl(bus, adc, I2C_ADC_QUERY_CHANNEL,
                                    &adc_data, bb_vs_config[bx][i].chan) < 0)) {
                cli_out("Error: failed to perform A/D conversions.\n");
            }
        } else
        {
            if ( (bcm_i2c_ioctl(unit, adc, I2C_ADC_QUERY_CHANNEL,
                                &adc_data, bb_vs_config[bx][i].chan) < 0)) {
                cli_out("Error: failed to perform A/D conversions.\n");
            }
        }
#ifdef COMPILER_HAS_DOUBLE
        cli_out("%s = %2.3f Volts \n",
                bb_vs_all[bx][i].function, adc_data.val);
#else
        cli_out("%s = %d.%03d Volts \n",
                bb_vs_all[bx][i].function,
                INT_FRAC_3PT_3(adc_data.val));
#endif

    } /* Display a single voltage */

    return CMD_OK;
}


#endif



/*
 *    Tc=0.000000
 *
 *    Tp=0.000000
 *
 *    Rp = (10.000000)        - Resistor for calibration current measurement
 *                                XXX measures voltage drop across
 *                                this resistor
 *
 *    TCoeff=0.003900         - Temperature Coefficient
 *
 *    Rcu=0.0000006787        - Resistivity
 *
 *    Lcal=1.000000           - Length of calibration strip
 *
 *    Lpwr=1.000000           - Length of Power Strip
 *
 *    Wcal=0.300000           - Width of calibration strip
 *
 *    Wpwr=0.700000           - Width of Power Strip
 *
 *    Vdrop_cal_core          - Voltage Drop across calibration strip
 *
 *    Vdrop_cal_phy           - Voltage Drop across calibration strip
 *
 *    Vdrop_cal_io            - Voltage Drop across calibration strip
 *
 *    Vdrop_coreA/Vdrop_coreB - Voltage Drop at the core calibration strip.
 *
 *    Vcal_core               - Core Voltage measured at the source.
 *
 *    Vcal_phy                - Phy voltage measured at the source
 *
 *    Vcal_io                 - IO voltage measured at the source.
 *
 *    Ical_core               - Calculated using Vcal_core and Resistance
 *                                value of Rp
 *
 *    Ical_phy                - Calculated using Vcal_core and Resistance
 *                                value of Rp
 *
 *    Ical_io                 - Calculated using Vcal_core and Resistance
 *                                value of Rp
 *
 *    Rseg_coreA/Rseg_coreB   - Resistance value of calibration strip is
 *      Rseg_phy                  estimated using the geometry of the
 *                                calibration strip and the resistance
 *                                computed for the Core calibration strip.
 *
 *    IcoreA                  - Calculated using Vdrop_coreA and Resistance
 *                                value of the calibration strip.
 *                                Resistance value of calibration strip is
 *                                estimated using the geometry of the
 *                                calibration strip and the resistance
 *                                computed for the Core calibration strip.
 *
 *    Rcal_core               - Resistance of calibration strip calculated
 *                                using the drop measured across the
 *                                calibration strip and the current flowing
 *                                through it.
 *                                Can also be extimated using the Resistivity
 *                                of copper, temprature and geometry of the
 *                                calibration strip.
 *
 *    Rcal_phy                - Resistance of calibration strip calculated
 *                                using the drop measured across the
 *                                calibration strip and the current flowing
 *                                through it.
 *                                Can also be extimated using the Resistivity
 *                                of copper, temprature and geometry of the
 *                                calibration strip.
 *
 */

#ifdef COMPILER_HAS_DOUBLE

/* Power computation definitions */
#define STD_RP          10.00         /* 10 Ohms, defines cal currents */
#define STD_LENGTH      2.00          /* Inches, length of CAL/PWR strips */
#define STD_WC          0.100         /* Inches, width of CAL strip */
#define STD_WP          0.700         /* Inches, width of PWR strip */
#define STD_RGAIN_PWR   130.0         /* RGp, sets gain of LT1167A amp */
#define STD_RGAIN_CAL   49.90          /* RGc, sets gain of LT1167A amp */

/* The LT1167s are configured to output 100s of milliamps.  */
/* Lower than this (or negative) indicates a probable empty */
/* socket.                                                  */
#define MIN_AMPLIFIED_VOLTAGE_DROP 0.050

/* Computes actual voltage drop from (amplified) value at LT1167A's output */
#define GAIN(v,r)   ( (v) / (1 + (49900.0/(r)))  )

#define VNULL       -9.99        /* Uninitialized voltage value */
#define RNULL       999.999     /* Uninitialized resistance value */

/* Thickness constants, based on PCB manufacturing parameters */
#define RCu         6.787E-07   /* Resistivity of Copper (Ohms * Inches) */
#define TCoeff      0.0039      /* Thermal Coefficient (1 / C) */   

#else /* !COMPILER_HAS_DOUBLE */

/* Power computation definitions */
#define STD_RP          10000         /* mOhms, defines cal currents */
#define STD_LENGTH      2000          /* 1/1000 Inches, length of CAL/PWR strips */
#define STD_WC          100           /* 1/1000 Inches, width of CAL strip */
#define STD_WP          700           /* 1/1000 Inches, width of PWR strip */
#define STD_RGAIN_PWR   130000        /* RGp, sets gain of LT1167A amp */
#define STD_RGAIN_CAL   49900          /* RGc, sets gain of LT1167A amp */

/* The LT1167s are configured to output 100s of milliamps.  */
/* Lower than this (or negative) indicates a probable empty */
/* socket.                                                  */
#define MIN_AMPLIFIED_VOLTAGE_DROP 50000

/* Computes actual voltage drop from (amplified) value at LT1167A's output */
#define GAIN(v,r)   ( (v) / (1 + (49900000/(r)))  )

#define VNULL       -9990000          /* Uninitialized voltage value */
#define RNULL       999999            /* Uninitialized resistance value */

/* Thickness constants, based on PCB manufacturing parameters */
#define RCu         6787              /* Resistivity of Copper (Ohms * Inches) * 1e-10 */
#define TCoeff      39                /* Thermal Coefficient (1 / C) * 1e-4 */   

#define RCAL_MUL    100               /* Rcal mulyiplier for better precision */

#endif /* COMPILER_HAS_DOUBLE */

/*
 * Control Voltage, and SDRAM/Core Clocks on P48 integrated board.
 */
char cmd_bb_usage[] =
    "Baseboard commands (bb) for I2C satellite devices\n"
    "Usages:\n\t"
#ifdef BCM_CALADAN3_SUPPORT
#ifdef COMPILER_STRING_CONST_LIMIT
    "       bb board | voltage | current \n"
#else
    "       bb board\n\t"
    "         - Prints the detected board type\n\t"
    "       bb voltage [source] [nominal|high|low][=value]\n\t"
    "         - set voltage output, if not specified, value is displayed.\n\t"
    "         - if \"nominal\" is specified, DAC set to mid-range.\n\t"
    "         - if \"high\" is specified, DAC set to margin high.\n\t"
    "         - if \"low\" is specified, DAC set to margin low.\n\t"
    "         - if \"nominal=<value>\" is specified, DAC nominal voltage configured.\n\t"
    "         - if \"high=<value>\" is specified, DAC margin high voltage configured.\n\t"
    "         - if \"low=<value>\" is specified, DAC margin low voltage configured.\n\t"
    "       bb current\n\t"
    "         - Prints the current values measured\n"
#endif /* COMPILER_STRING_CONST_LIMIT */
#else /* CALADAN3 */
#ifdef COMPILER_STRING_CONST_LIMIT
    "       bb board | clock | spread | voltage | power | temperature\n"
#else
    "       bb board\n\t"
    "         - show board type detected\n\t"
    "       bb clock [Core_A SDRAM_A Core_B SDRAM_B Turbo] value\n\t"
    "         - set specified clock speed.\n\t"
    "       bb spread [Core_A SDRAM_A Core_B SDRAM_B Turbo] value\n\t"
    "         - value: 0=normal,1=-0.5%,2=+/-0.5%,3=+/-0.25%,4=+/-0.38%\n\t"
    "       bb voltage [A B TRef PHY] [<volts>|calibrate|nominal]\n\t"
    "         - set voltage output, if not specified, value is displayed.\n\t"
    "         - if \"calibrate\" is specified, calibrate specified DAC.\n\t"
    "         - if \"nominal\" is specified, DAC set to mid-range.\n\t"
    "       bb power\n\t"
    "           - computer power utilization.\n\t"
    "       bb temperature Tp Tc\n\t"
    "           - trace temperature computation.\n\t"     
    "             - Tp :PHY trace thickness.\n\t"     
    "             - Tc :Core trace thickness.\n\t"     
    "       bb thickness  Tp Tc\n\t"
    "           - trace thickness computation.\n\t"
    "             - Tp :PHY trace temperature.\n\t"     
    "             - Tc :Core trace temperature.\n\t"          
    "       bb pot number value\n\t"
    "         - set potentiometer.\n\t"
    "           - number: potentiometer number 0..7\n\t"
    "           - value : value to set 0..255\n\t"
    "NOTE: calibration of DAC is performed (automatically) only once.\n"
#endif
#endif /* CALADAN3 */
    ;


cmd_result_t
cmd_bb(int unit, args_t *a)
{
    int i, bx, clk_unit, pll, adc, dac = -1, mux, rv = SOC_E_NONE;
    char *function = ARG_GET(a);
    char *source = ARG_GET(a);
    char *value = ARG_GET(a);
    uint32 flags = 0;
#ifdef COMPILER_HAS_DOUBLE
    double volts, clockval, curr_zero = 1.0, curr_sens;
#else
    int volts, clockval, curr_zero = 1, curr_sens;
#endif
    int show_cal = 0;
    i2c_adc_t  adc_data;
    bb_type_t* baseboard = NULL;

#ifdef BCM_CALADAN3_SVK
    if (unit < 0) {
        /* run in CPU mode, there is no unit to attach */
    } else {
        if (! sh_check_attached(ARG_CMD(a), unit))
            return CMD_FAIL;
    }
#else
    if (! sh_check_attached(ARG_CMD(a), unit))
        return CMD_FAIL;
#endif

    if ( (! function) )
        return CMD_USAGE ;

    /* Get board type */
    baseboard = soc_get_board();
    if(!baseboard || (baseboard->bbcmd_flags == BB_NONE)){
        cli_out("Error: baseboard command unavailable on %s\n",
                BaseboardName);
        return CMD_FAIL;
    }

    if (!sal_strcasecmp(function, "board")) {
        cli_out("Baseboard: %s (type %d)\n",
                BaseboardName, baseboard->type);
        return CMD_OK;
    }

    /* Get board index */
    bx = baseboard->type;

    /* Configure or display clock frequency or spread-spectrum */
    if(!sal_strcasecmp(function,"clock")  ||
            !sal_strcasecmp(function,"spread") ){
        /* Check for supported clock chips (W311, CY22393, CY22150) */
        if ( !(baseboard->bbcmd_flags & BB_CLK) ) {
            cli_out("Baseboard clock configuration function not supported.\n");
            cli_out("use clocks or xclocks command instead.\n");
            return CMD_FAIL;
        }
        /* Set clock speed or spread spectrum */
        if( source && value ) { 

            /* First, find clock table entry ... */
            for( i = 0; i < bb_clock_len[bx]; i++) {
                if ( !sal_strcasecmp(bb_clocks[bx][i].name, source))
                    break;
                if ( (bb_clocks[bx][i].alias != NULL) &&
                        !sal_strcasecmp(bb_clocks[bx][i].alias, source))
                    break;
            }
            if (i == bb_clock_len[bx]) {    /* unknown board clock */
                cli_out("ERROR: unknown clock source: %s\n", source);
                cli_out("ERROR: valid clocks are");
                for(i = 0; i < bb_clock_len[bx]; i++) {
                    cli_out(" %s", bb_clocks[bx][i].name);
                }
                cli_out("\n");
                return CMD_FAIL;
            }

            /* If indicated, set the MUX appropriately */
            if (bb_clocks[bx][i].mux != NO_MUX) {
                /* Open MUX (lpt0) device */
#ifdef SHADOW_SVK
                char *name;
                uint8 channel;
                /* Open MUX device */
                name = _i2c_mux_default_dev_name_get();
                if ( (mux = _i2c_mux_open(unit, flags, 0, name)) < 0) {
                    cli_out("%s: cannot open I2C device %s: %s\n",
                            ARG_CMD(a), name, bcm_errmsg(mux));
                    return CMD_FAIL;
                }

                /* Set mux value so we can see the device when we try to open it*/
                channel = _i2c_mux_channel_get(unit, bb_clocks[bx][i].mux);
                if( (rv = bcm_i2c_write(unit, mux, 0,
                                &channel, 1)) < 0){
                    cli_out("ERROR: I2C write failed: %s\n",
                            bcm_errmsg(rv));
                    return CMD_FAIL;
                }
#else
                if ( (mux = bcm_i2c_open(unit, _i2c_mux_default_dev_name_get(), flags, 0)) < 0) {
                    cli_out("%s: cannot open I2C device %s: %s\n",
                            ARG_CMD(a), I2C_LPT_0, bcm_errmsg(mux));
                    return CMD_FAIL;
                }

                /* Set mux value so we can see the device when we try to open it*/
                if( (rv = bcm_i2c_write(unit, mux, 0,
                                &bb_clocks[bx][i].mux, 1)) < 0){
                    cli_out("ERROR: I2C write failed: %s\n",
                            bcm_errmsg(rv));
                    return CMD_FAIL;
                }
#endif /* SHADOW_SVK */
            } /* Need to set MUX for the clock chip */

            /* Open clock chip, this could be on the second bus,
             * so we need to check for that here ..
             */
            if ( bb_clocks[bx][i].unit != unit){
                clk_unit = bb_clocks[bx][i].unit;
                cli_out("NOTICE: clock chip on i2c%d\n", clk_unit);
            }
            else
                clk_unit = unit;

            /* Open clock chip, this will cause a probe to happen
             * if not already done.
             */
            if ( (pll = bcm_i2c_open(clk_unit,
                            bb_clocks[bx][i].device, flags, 0)) < 0) {
                cli_out("%s: cannot open I2C device %s: %s\n",
                        ARG_CMD(a), bb_clocks[bx][i].device, bcm_errmsg(pll));
                return CMD_FAIL;
            }

            /*
            * Use Cypress CY22393x (or CY22150)
            * Both devices are named "clk0"
            */
#ifdef COMPILER_HAS_DOUBLE
            clockval = atof( value ) ;
#else
            /* Parse input in MHz and convert to Hz */
            clockval = _shr_atof_exp10(value, 6);
#endif

            /* Set speed */
            if(!sal_strncasecmp(function,"clock",sal_strlen(function))){

                if (!sal_strcasecmp(bb_clocks[bx][i].name, "CPU")) {
                    cli_out("NOTICE: changing CPU clock requires removal"
                            " of jumper JP302.\n");
                }
                /* Set speed */
                if ( (bcm_i2c_ioctl(clk_unit, pll,
                    bb_clocks[bx][i].xpll,
                    &clockval, 0) < 0)) {
#ifdef COMPILER_HAS_DOUBLE
                        cli_out("ERROR: failed to set %s clock to %2.2fMHz\n",
                                bb_clocks[bx][i].name, clockval);
#else
                        cli_out("ERROR: failed to set %s clock to %d.%02dMHz\n",
                                bb_clocks[bx][i].name, INT_FRAC_2PT_4(clockval));
#endif
                        return CMD_FAIL;
                }        
            }  
            else{
                /* Spread spectrum mode */
                cli_out("ERROR: CY22xxx does not have spread spectrum.\n");
                return CMD_FAIL;
            }

            return CMD_OK;

        } /* (source && value), set a clock */ 

        else {
            /* Display single clock or all clocks */

            i = 0; /* Top of table unless a specific clock is requested */

            if (source) {
                /* Request to show a single clock. Find it first... */
                for(  ; i < bb_clock_len[bx]; i++) {
                    if ( !sal_strcasecmp(bb_clocks[bx][i].name, source))
                        break;
                    if ( (bb_clocks[bx][i].alias != NULL) &&
                            !sal_strcasecmp(bb_clocks[bx][i].alias, source))
                        break;
                }
                if (i == bb_clock_len[bx]) {    /* unknown board clock */
                    cli_out("ERROR: unknown clock source: %s\n", source);
                    cli_out("ERROR: valid clocks are");
                    for(i = 0; i < bb_clock_len[bx]; i++) {
                        cli_out(" %s", bb_clocks[bx][i].name);
                    }
                    cli_out("\n");
                    return CMD_FAIL;
                }
            } /* Requested a single clock display */

            /* Already have entry number (single clock), or we'll go */
            /* through the whole clock table entry (all clocks)...   */
            for(  ; i < bb_clock_len[bx]; i++) {

                /* If indicated, set the MUX appropriately */
                if (bb_clocks[bx][i].mux != 0xFF) {
#ifdef SHADOW_SVK
                    char *name = I2C_LPT_0;
                    uint8 channel;
                    mux = _i2c_mux_open(unit, flags, 0, name);
                    if (mux < 0) {
                        name = I2C_MUX_6;
                    }
                    if ( (mux = _i2c_mux_open(unit, flags, 0, name)) < 0) {
                        cli_out("Could not open %s for mux selection:%s\n",
                                name, bcm_errmsg(mux));
                        return CMD_FAIL;
                    }

                    /* Set mux value so we can see the device ...*/
                    channel =  _i2c_mux_channel_get(unit, bb_clocks[bx][i].mux);
                    if( (rv = bcm_i2c_write(unit, mux, 0,
                               &channel, 1)) < 0){
                        cli_out("Error: write of %s byte failed:%s\n",
                                name, bcm_errmsg(rv));
                        return CMD_FAIL;
                    }
#else
                    /* Open MUX (lpt0) device */
                    if ( (mux = bcm_i2c_open(unit, I2C_LPT_0, flags, 0)) < 0) {
                        cli_out("Could not open %s for mux selection:%s\n",
                                I2C_LPT_0, bcm_errmsg(mux));
                        return CMD_FAIL;
                    }

                    /* Set mux value so we can see the device ...*/
                    if( (rv = bcm_i2c_write(unit,
                                    mux, 0,
                                    &bb_clocks[bx][i].mux, 1)) < 0){
                        cli_out("Error: write of lpt byte failed:%s\n",
                                bcm_errmsg(rv));
                        return CMD_FAIL;
                    }
#endif /* SHADOW_SVK */
                } /* Need to set MUX for the clock chip */

                if ( bb_clocks[bx][i].unit != unit){
                    clk_unit = bb_clocks[bx][i].unit;
                    cli_out("NOTICE: clock chip on i2c%d\n", clk_unit);
                }
                else
                    clk_unit = unit;

                /* Open clock chip */
                if ( (pll = bcm_i2c_open(clk_unit,
                                bb_clocks[bx][i].device,
                                flags, 0)) < 0) {
                    cli_out("Could not open %s for PLL speed selection:%s\n",
                            I2C_PLL_0, bcm_errmsg(pll));
                    return CMD_FAIL;
                }

                /* CY2239x or CY22150 clock chip */

                /* Get clock speed */
                clockval = 0;
                if ( (bcm_i2c_ioctl(clk_unit, pll,
                    I2C_XPLL_GET_OP(bb_clocks[bx][i].xpll),
                    &clockval, 0) < 0)){
                        cli_out("Error: failed to retrieve clock speed.\n");
                }
#ifdef COMPILER_HAS_DOUBLE
                cli_out("\t\"%s\" clock running at %2.2fMhz\n",
                        bb_clocks[bx][i].name, clockval);
#else
                cli_out("\t\"%s\" clock running at %d.%02dMhz\n",
                        bb_clocks[bx][i].name, INT_FRAC_2PT_4(clockval));
#endif
                if (source)
                    break; /* ... if showing single clock only */
            } /* For each entry in the bb_clocks[] table */
            return CMD_OK;
        } /* display clock(s) */
    } /* function is "clock" or "spread" */

    /* Power, Thickness, and Temperature computations */
    if(!sal_strncasecmp(function,"power",sal_strlen(function)) ||
            (!sal_strncasecmp(function,"thickness",sal_strlen(function))) ||
            (!sal_strncasecmp(function,"temperature",sal_strlen(function)))){    

        /* Check for support of these functions... */
        if ((baseboard->bbcmd_flags & BB_PWR)) {
            int op = 0;
#define BB_OP_POWER    1
#define BB_OP_TEMP     2
#define BB_OP_THICK    3    

#ifdef COMPILER_HAS_DOUBLE
            double Rp, l_cal, l_pwr, w_cal, w_pwr, RGp, RGc;
            double VcoreA= VNULL, Vdrop_coreA= VNULL;
            double VcoreB= VNULL, Vdrop_coreB= VNULL;
            double Vphy= VNULL, Vdrop_phy= VNULL;
            double Vio= VNULL, Vdrop_io= VNULL;
            double Vcal_core= VNULL, Vdrop_cal_core= VNULL;
            double Vcal_phy= VNULL, Vdrop_cal_phy= VNULL;        
            double Vcal_io= VNULL, Vdrop_cal_io= VNULL;        
            double Ical_core = 0, Rcal_core = 0;
            double Ical_phy = 0, Rcal_phy = 0;
            double Ical_io = 0, Rcal_io = 0;
            double Rseg_coreA, IcoreA, PcoreA;
            double Rseg_coreB, IcoreB, PcoreB;
            double Rseg_phy, Iphy, Pphy;
            double Rseg_io, Iio, Pio;
            double tCore, tPhy, Tp, Tc;
#else
            int Rp, l_cal, l_pwr, w_cal, w_pwr, RGp, RGc;
            int VcoreA= VNULL, Vdrop_coreA= VNULL;
            int VcoreB= VNULL, Vdrop_coreB= VNULL;
            int Vphy= VNULL, Vdrop_phy= VNULL;
            int Vio= VNULL, Vdrop_io= VNULL;
            int Vcal_core= VNULL, Vdrop_cal_core= VNULL;
            int Vcal_phy= VNULL, Vdrop_cal_phy= VNULL;        
            int Vcal_io= VNULL, Vdrop_cal_io= VNULL;        
            int Ical_core = 0, Rcal_core = 0;
            int Ical_phy = 0, Rcal_phy = 0;
            int Ical_io = 0, Rcal_io = 0;
            int Rseg_coreA, IcoreA, PcoreA;
            int Rseg_coreB, IcoreB, PcoreB;
            int Rseg_phy, Iphy, Pphy;
            int Rseg_io, Iio, Pio;
            int tCore, tPhy, Tp, Tc;
            int t1;
            char *s1;

            if (SOC_IS_XGS(unit)) {
                s1 = "Analog";
            } else {
                s1 = "Phy";
            }
#endif

            /* Define the geometries and gains of the    */
            /* calibration and power measurement strips. */

            Rp = STD_RP;         /* Calibration current resistor */
            l_cal = STD_LENGTH;  /* Length of calibration strip */
            l_pwr = STD_LENGTH;  /* Length of power strip */
            w_cal = STD_WC;      /* Width of calibration strip */
            w_pwr = STD_WP;      /* Width of power strip */
            RGc = STD_RGAIN_CAL; /* Gain of calibration strip voltage amp */
            RGp = STD_RGAIN_PWR; /* Gain of power strip voltage amp */

            /* Special case boards with different dimensions/parameters */

#ifdef COMPILER_HAS_DOUBLE
            if (bx == G13) {
                /* Firebolt, Easyrider SDKs */
                l_cal = 1.0;
                l_pwr = 1.0;
                w_cal = 0.3;
            }
            else if ((bx == G15) || (bx == G16) || (bx == G17)) {
                /* Goldwing, Bradley SDKs */
                l_cal = 1.0;
                l_pwr = 1.0;
                w_cal = 0.3;
            } else if ((bx == G18) || (bx == G19)) {
                /* Raptor  and Raven */
                Rp = 5.0;        
                l_cal = 1.0;
                l_pwr = 1.0;
                w_cal = 0.3;
            }
#else
            if (bx == G13) {
                /* Firebolt, Easyrider SDKs */
                l_cal = 1000;
                l_pwr = 1000;
                w_cal = 300;
            } else if ((bx == G15) || (bx == G16) || (bx == G17)) {
                /* Goldwing, Bradley SDKs */
                l_cal = 1000;
                l_pwr = 1000;
                w_cal = 300;
            } else if ((bx == G18) || (bx == G19)) {
                /* Raptor  and Raven */
                Rp = 5.0;        
                l_cal = 1.0;
                l_pwr = 1.0;
                w_cal = 0.3;
            }
#endif

            /* Select computation */
            if(!sal_strncasecmp(function,"thickness",sal_strlen(function))){
                op = BB_OP_THICK;
                if( !source || !value)
                    return CMD_USAGE;
#ifdef COMPILER_HAS_DOUBLE
                Tp = atof(source);
                Tc = atof(value);
#else
                /* Temperature is 1/10 C */
                Tp = _shr_atof_exp10(source, 1);
                Tc = _shr_atof_exp10(value, 1);
#endif
            } else if(!sal_strncasecmp(function,"temperature",
                        sal_strlen(function))){
                op = BB_OP_TEMP;
                if( !source || !value)
                    return CMD_USAGE;
#ifdef COMPILER_HAS_DOUBLE
                Tp = atof(source);
                Tc = atof(value);
#else
                Tp = _shr_atof_exp10(source, 6);
                Tc = _shr_atof_exp10(value, 6);
#endif
            } else {
                op = BB_OP_POWER;
                Tp = Tc = 0;
            }

            /* Collect all voltages */    
            for(i = 0; i < bb_vs_all_len[bx]; i++){
                if ( (adc = bcm_i2c_open(unit,
                                bb_vs_all[bx][i].adc,
                                flags, 0)) < 0) {
                    cli_out("Could not open %s:%s\n",
                            bb_vs_all[bx][i].adc,
                            bcm_errmsg(adc));
                    return CMD_FAIL;
                }
                /* Display voltage */
                if ( (bcm_i2c_ioctl(unit, adc, I2C_ADC_QUERY_CHANNEL,
                                &adc_data, bb_vs_all[bx][i].chan) < 0)) {
                    cli_out("Error: failed to perform A/D conversions.\n");
                }

                /* Assign each voltage to the proper Vxxx variable
                 * based upon bb_vs_all[bx][i].flags.
                 * VcoreA          PWR_CORE_A_SRC
                 * Vdrop_coreA     PWR_CORE_A_DROP
                 * VcoreB          PWR_CORE_B_SRC
                 * Vdrop_coreB     PWR_CORE_B_DROP
                 * Vphy            PWR_PHY_SRC
                 * Vdrop_phy       PWR_PHY_DROP
                 * Vcal_core       PWR_CORECAL_SRC
                 * Vdrop_cal_core  PWR_CORECAL_DROP
                 * Vcal_phy        PWR_PHYCAL_SRC
                 * Vdrop_cal_phy   PWR_PHYCAL_DROP
                 */ 

                /*
                 * PWR_CORE_A_SRC is the voltage at the power supply, and
                 * PWR_CORE_A_LOAD is the voltage at the chip, however,
                 * PWR_CORE_A_LOAD is not available in all designs.
                 *
                 * For the most accurate measurement, PWR_CORE_A_LOAD
                 * should be used if it is available.
                 */
                if (bb_vs_all[bx][i].flags & PWR_CORE_A_LOAD) {
                    VcoreA = adc_data.val;
                } else if ((bb_vs_all[bx][i].flags & PWR_CORE_A_SRC) &&
                           VcoreA == VNULL) {
                    VcoreA = adc_data.val;
                }
                if (bb_vs_all[bx][i].flags & PWR_CORE_A_DROP) { 
                    if (adc_data.val < 0) adc_data.val *= -1;
                    if (adc_data.val > MIN_AMPLIFIED_VOLTAGE_DROP)
                        Vdrop_coreA = GAIN(adc_data.val,RGp);
                    else
                        Vdrop_coreA = 0;
                }
                if (bb_vs_all[bx][i].flags & PWR_CORE_B_SRC)
                    VcoreB = adc_data.val;
                if (bb_vs_all[bx][i].flags & PWR_CORE_B_DROP) {
                    if (adc_data.val < 0) adc_data.val *= -1;
                    if (adc_data.val > MIN_AMPLIFIED_VOLTAGE_DROP)
                        Vdrop_coreB = GAIN(adc_data.val,RGp);
                    else
                        Vdrop_coreB = 0;
                }
                /*
                 * PWR_PHY_SRC is the voltage at the power supply, and
                 * PWR_PHY_LOAD is the voltage at the chip, however,
                 * PWR_PHY_LOAD is not available in all designs.
                 *
                 * For the most accurate measurement, PWR_PHY_LOAD
                 * should be used if it is available.
                 */
                if (bb_vs_all[bx][i].flags & PWR_PHY_LOAD) {
                    Vphy = adc_data.val;
                } else if ((bb_vs_all[bx][i].flags & PWR_PHY_SRC) &&
                           Vphy == VNULL) {
                    Vphy = adc_data.val;
                }
                if (bb_vs_all[bx][i].flags & PWR_PHY_DROP) {
                    if (adc_data.val < 0) adc_data.val *= -1;
                    if (adc_data.val > MIN_AMPLIFIED_VOLTAGE_DROP)
                        Vdrop_phy = GAIN(adc_data.val,RGp);
                    else
                        Vdrop_phy = 0;
                }
                if (bb_vs_all[bx][i].flags & PWR_IO_SRC)
                    Vio = adc_data.val;
                if (bb_vs_all[bx][i].flags & PWR_IO_DROP) {
                    if (adc_data.val < 0) adc_data.val *= -1;
                    if (adc_data.val > MIN_AMPLIFIED_VOLTAGE_DROP)
                        Vdrop_io = GAIN(adc_data.val,RGp);
                    else
                        Vdrop_io = 0;
                }
                if (bb_vs_all[bx][i].flags & PWR_CORECAL_SRC)
                    Vcal_core = adc_data.val;
                if (bb_vs_all[bx][i].flags & PWR_CORECAL_DROP) {
                    if (adc_data.val < 0) adc_data.val *= -1;
                    Vdrop_cal_core = GAIN(adc_data.val,RGc);
		}

		if (bb_vs_all[bx][i].flags & PWR_PHYCAL_SRC)
		    Vcal_phy = adc_data.val;
		if (bb_vs_all[bx][i].flags & PWR_PHYCAL_DROP) {
		    if (adc_data.val < 0) adc_data.val *= -1;
		    Vdrop_cal_phy = GAIN(adc_data.val,RGc);
		}

		if (bb_vs_all[bx][i].flags & PWR_IOCAL_SRC)
		    Vcal_io = adc_data.val;
		if (bb_vs_all[bx][i].flags & PWR_IOCAL_DROP) {
		    if (adc_data.val < 0) adc_data.val *= -1;
		    Vdrop_cal_io = GAIN(adc_data.val,RGc);
		}
        }
        
        /* calculations */

        /* There is at least one calibration strip. If there are multiple */
        /* strips, it is assumed that they have the same geometry.        */

        if (Vdrop_cal_core != VNULL) { 
            /* Current flowing through Rp and core cal strip to ground */
            Ical_core = Vcal_core/Rp;
            /* Resistance of core calibration strip */
#ifdef COMPILER_HAS_DOUBLE
            Rcal_core = Vdrop_cal_core/Ical_core;
#else
            Rcal_core = (RCAL_MUL * Vdrop_cal_core) / Ical_core;
#endif
        }

        if (Vdrop_cal_phy != VNULL) {
        /* Current flowing through Rp and phy cal strip to ground */
        Ical_phy = Vcal_phy/Rp;
        /* Resistance of phy calibration strip */
#ifdef COMPILER_HAS_DOUBLE
        Rcal_phy = Vdrop_cal_phy/Ical_phy;
#else
        Rcal_phy = (RCAL_MUL * Vdrop_cal_phy) / Ical_phy;
#endif
        }

        if (Vdrop_cal_io != VNULL) {
        /* Current flowing through Rp and IO cal strip to ground */
        Ical_io = Vcal_io/Rp;
        /* Resistance of IO calibration strip */
#ifdef COMPILER_HAS_DOUBLE
        Rcal_io = Vdrop_cal_io/Ical_io;
#else
        Rcal_io = (RCAL_MUL * Vdrop_cal_io) / Ical_io;
#endif
        }

        if (Vdrop_cal_core == VNULL) {
          /* No separate core calibration strip. */
          /* Use PHY calibration strip.          */
        Ical_core = Ical_phy;
        Rcal_core = Rcal_phy;
        }
        
        if (Vdrop_cal_phy == VNULL) {
          /* No separate PHY calibration strip. */
          /* Use core calibration strip.        */
        Ical_phy = Ical_core;
        Rcal_phy = Rcal_core;
        }

        if (Vdrop_cal_io == VNULL) {
          /* No separate IO calibration strip. */
          /* Use phy calibration strip.        */
        Ical_io = Ical_phy;
        Rcal_io = Rcal_phy;
        }


        if (Vdrop_coreA != VNULL) {
            /* Power : Core A */
#ifdef COMPILER_HAS_DOUBLE
            Rseg_coreA = Rcal_core * (w_cal/w_pwr) * (l_pwr/l_cal);
        IcoreA = Vdrop_coreA/Rseg_coreA;
        PcoreA = IcoreA * VcoreA;
#else
            Rseg_coreA = (((Rcal_core * w_cal) / w_pwr) * l_pwr) / l_cal;
        IcoreA = ((RCAL_MUL * Vdrop_coreA * w_pwr) / l_pwr) * l_cal;
                IcoreA /= (Rcal_core * w_cal);
        PcoreA = (IcoreA * (VcoreA / 1000)) / 1000; /* mW */
#endif
        }
        else {
        IcoreA = PcoreA = 0;
        Rseg_coreA = RNULL;
        }

        if (Vdrop_coreB != VNULL) {
        /* Power: Core B */
#ifdef COMPILER_HAS_DOUBLE
        Rseg_coreB = Rcal_core * (w_cal/w_pwr) * (l_pwr/l_cal);
        IcoreB = Vdrop_coreB/Rseg_coreB;
        PcoreB = IcoreB * VcoreB;
#else
            Rseg_coreB = (((Rcal_core * w_cal) / w_pwr) * l_pwr) / l_cal;
        IcoreB = ((RCAL_MUL * Vdrop_coreB * w_pwr) / l_pwr) * l_cal;
                IcoreB /= (Rcal_core * w_cal);
        PcoreB = (IcoreB * (VcoreB / 1000)) / 1000; /* mW */
#endif
        }
        else {
        IcoreB = PcoreB = 0;
        Rseg_coreB = RNULL;
        }

        if (Vdrop_phy != VNULL) {
        /* Power: Phy */
#ifdef COMPILER_HAS_DOUBLE
        Rseg_phy = Rcal_phy * (w_cal / w_pwr) * (l_pwr / l_cal);
        Iphy = Vdrop_phy / Rseg_phy;
        Pphy = Iphy * Vphy;
#else
            Rseg_phy = (((Rcal_phy * w_cal) / w_pwr) * l_pwr) / l_cal;
                Iphy = ((RCAL_MUL * Vdrop_phy * w_pwr) / l_pwr) * l_cal;
                Iphy /= (Rcal_phy * w_cal);
                Pphy = (Iphy * (Vphy / 1000)) / 1000; /* mW */
#endif
        }
        else {
        Iphy = Pphy = 0;
        Rseg_phy = RNULL;
        }

        if (Vdrop_io != VNULL) {
        /* Power: IO */
#ifdef COMPILER_HAS_DOUBLE
        Rseg_io = Rcal_io * (w_cal / w_pwr) * (l_pwr / l_cal);
        Iio = Vdrop_io / Rseg_io;
        Pio = Iio * Vio;
#else
            Rseg_io = (((Rcal_io * w_cal) / w_pwr) * l_pwr) / l_cal;
                Iio = ((RCAL_MUL * Vdrop_io * w_pwr) / l_pwr) * l_cal;
                Iio /= (Rcal_io * w_cal);
                Pio = (Iphy * (Vphy / 1000)) / 1000; /* mW */
#endif
        }
        else {
        Iio = Pio = 0;
        Rseg_io = RNULL;
        }

        if (bx == G5) {
        /* For this board, the calibration strip was placed on
         * a different PCB plane than the measurement strips.
         * Adjust current and power values to compensate for
         * the difference in plane thicknesses. 
         */
        PcoreA = (PcoreA * 540) / 1000; /* PcoreA * 0.54 */
        IcoreA = (IcoreA * 540) / 1000; /* IcoreA * 0.54 */
        Pphy = (Pphy * 540) / 1000; /* Pphy * 0.54 */
        Iphy = (Iphy * 540) / 1000; /* Iphy * 0.54 */
        }

#define BB_DEBUG
#ifdef BB_DEBUG  
        /* Debugging */
#ifdef COMPILER_HAS_DOUBLE
#define PFMT "%f"
#else
#define PFMT "%d"
#endif
            /* Constants */
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "\nConstants\n")));
#ifdef COMPILER_HAS_DOUBLE
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "Rcu=%1.10f\n"), RCu));
#else
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "Rcu=%d\n"), RCu));
#endif
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "Lcal=" PFMT "\n"), l_cal));
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "Lpwr=" PFMT "\n"), l_pwr));
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "Wcal=" PFMT "\n"), w_cal));
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "Wpwr=" PFMT "\n"), w_pwr));        
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "Rp=" PFMT "\n"), Rp));

        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "\nMeasured Voltage Values\n")));
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "Vdrop_cal_core=" PFMT "\n"), Vdrop_cal_core));
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "Vdrop_cal_phy=" PFMT "\n"), Vdrop_cal_phy));
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "Vdrop_cal_io=" PFMT "\n"), Vdrop_cal_io));
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "Vcal_core=" PFMT "\n"), Vcal_core));
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "Vcal_phy=" PFMT "\n"), Vcal_phy));
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "Vcal_io=" PFMT "\n"), Vcal_io));
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "Vdrop_coreA=" PFMT "\n"), Vdrop_coreA));

        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "\nCalculated Current Values\n")));
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "Ical_core=" PFMT "\n"), Ical_core));
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "Ical_phy=" PFMT "\n"), Ical_phy));    
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "Ical_io=" PFMT "\n"), Ical_io));    

        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "\nCalculated Resistance Values\n")));
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "Rcal_core=" PFMT "\n"), Rcal_core));
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "Rcal_phy=" PFMT "\n"), Rcal_phy));        
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "Rcal_io=" PFMT "\n"), Rcal_io));        
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "Rseg_coreA=" PFMT "\n"), Rseg_coreA));
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "Rseg_phy=" PFMT "\n"), Rseg_phy));        
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "Rseg_io=" PFMT "\n"), Rseg_io));        

        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "Tc=" PFMT "\n"), Tc));
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "Tp=" PFMT "\n"), Tp));
#endif
#undef BB_DEBUG
        /* Show power */
        if( op == BB_OP_POWER) {

            cli_out("Power Measurement\n");

        if (Vdrop_coreA == VNULL) {
            /* 
             * Only 1 core power measurement (Core B) 
             */
#ifdef COMPILER_HAS_DOUBLE
            cli_out("PCore  = %2.3fW  ICore = %2.3fA  VCore = %2.3fV\n",
                    PcoreB, IcoreB, VcoreB);
#else
            cli_out("PCore  = %d.%03dW  ICore = %d.%03dA  VCore = %d.%03dV\n",
                    INT_FRAC_3PT(PcoreB), INT_FRAC_3PT(IcoreB),
                    INT_FRAC_3PT(VcoreB));
#endif
        }
        else if (Vdrop_coreB == VNULL) {
            /* 
             * Only 1 core power measurement (Core A)
             */
#ifdef COMPILER_HAS_DOUBLE
            cli_out("PCore  = %2.3fW  ICore = %2.3fA  VCore = %2.3fV\n",
                    PcoreA, IcoreA, VcoreA);
#else
            cli_out("PCore  = %d.%03dW  ICore = %d.%03dA  VCore = %d.%03dV\n",
                    INT_FRAC_3PT(PcoreA), INT_FRAC_3PT(IcoreA),
                    INT_FRAC_3PT(VcoreA));
#endif
        }
        else {
            /* 
             * Both core power measurements 
             */
#ifdef COMPILER_HAS_DOUBLE
            cli_out("PCoreA = %2.3fW ICoreA = %2.3fA  VCoreA = %2.3fV\n",
                    PcoreA, IcoreA, VcoreA);
            cli_out("PCoreB = %2.3fW ICoreB = %2.3fA  VCoreB = %2.3fV\n",
                    PcoreB, IcoreB, VcoreB);
#else
            cli_out("PCoreA = %d.%03dW  ICoreA = %d.%03dA  VCoreA = %d.%03dV\n",
                    INT_FRAC_3PT(PcoreA), INT_FRAC_3PT(IcoreA),
                    INT_FRAC_3PT(VcoreA));
            cli_out("PCoreB = %d.%03dW  ICoreB = %d.%03dA  VCoreB = %d.%03dV\n",
                    INT_FRAC_3PT(PcoreB), INT_FRAC_3PT(IcoreB),
                    INT_FRAC_3PT(VcoreB));
#endif
        }

        if (Vdrop_phy != VNULL) { 
            /* 
             * PHY (analog) power measurement 
             */
#ifdef COMPILER_HAS_DOUBLE
                    cli_out("PPhy   = %2.3fW   IPhy = %2.3fA   VPhy = %2.3fV\n",
                            Pphy, Iphy, Vphy);
#else
                    cli_out("PPhy   = %d.%03dW   IPhy = %d.%03dA   VPhy = %d.%03dV\n",
                            INT_FRAC_3PT(Pphy), INT_FRAC_3PT(Iphy), 
                            INT_FRAC_3PT(Vphy));
#endif
        }

        if (Vdrop_io != VNULL) { 
            /* 
             * IO power measurement 
             */
#ifdef COMPILER_HAS_DOUBLE
            cli_out("P_IO   = %2.3fW   I_IO = %2.3fA   V_IO = %2.3fV\n",
                    Pio, Iio, Vio);
#else
            cli_out("P_IO   = %d.%03dW   I_IO = %d.%03dA   V_IO = %d.%03dV\n",
                    INT_FRAC_3PT(Pio), INT_FRAC_3PT(Iio), INT_FRAC_3PT(Vio));
#endif
        }

#ifdef COMPILER_HAS_DOUBLE
        cli_out("%2.3f Watts Total\n",
                (PcoreA + PcoreB + Pphy + Pio));
#else
        cli_out("%d.%03d Watts Total\n",
                INT_FRAC_3PT(PcoreA + PcoreB + Pphy + Pio));
#endif


        } else if (op == BB_OP_THICK) {
        /* Thickness : Tp/Tc are temperatures */
#ifdef COMPILER_HAS_DOUBLE
        tCore = tPhy = (l_cal * RCu);
        tCore /= (Rcal_core * w_cal)/ (1.0 + (Tc - 20.0) * TCoeff);
        tPhy /=  (Rcal_phy * w_cal)/ (1.0 + (Tp - 20.0) * TCoeff);

        if (SOC_IS_XGS(unit)){
            cli_out("Thickness Core (mil) = %2.3f\n",1000*tCore);
            cli_out("Thickness Analog (mil) = %2.3f\n",1000*tPhy);
        } else {
            cli_out("Thickness Core (mil) = %2.3f\n",1000*tCore);
            cli_out("Thickness Phy (mil) = %2.3f\n",1000*tPhy);
        }
#else
        t1 = l_cal * RCu * RCAL_MUL;
        tCore = t1 / (Rcal_core * w_cal);
                tCore *= 10000 + (((Tc - 200) * TCoeff) / 10);
                tCore /= 100000;
        tPhy =  t1 / (Rcal_phy * w_cal);
                tPhy *= 10000 + (((Tp - 200) * TCoeff) / 10);
                tPhy /= 100000;

                cli_out("Thickness Core (mil) = %d.%03d\n", INT_FRAC_3PT(tCore));
                cli_out("Thickness %s (mil) = %d.%03d\n", s1, INT_FRAC_3PT(tPhy));
#endif

        } else {
        /* Temperature */
#ifdef COMPILER_HAS_DOUBLE
        tCore = (((Rcal_core * w_cal * Tc) /
              (l_cal * RCu) - 1.0) / TCoeff) + 20.0;
        tPhy  = (((Rcal_phy *  w_cal * Tp) /
              (l_cal * RCu) - 1.0) / TCoeff) + 20.0;

        if (SOC_IS_HERCULES1(unit) || SOC_IS_DRACO(unit)){
            cli_out("Temperature Core = %2.3fC/%2.3fF\n",tCore,
                    (9.0/5.0) * tCore + 32);
            cli_out("Temperature Analog  = %2.3fC/%2.3fF\n",tPhy,
                    (9.0/5.0) * tPhy + 32);
        } else {
            cli_out("Temperature Core = %2.3fC/%2.3fF\n",tCore,
                    (9.0/5.0) * tCore + 32);
            cli_out("Temperature Phy  = %2.3fC/%2.3fF\n",tPhy,
                    (9.0/5.0) * tPhy + 32);
        }
#else
        t1 = (l_cal * RCu) / (100000 / RCAL_MUL);
        tCore = (Rcal_core * w_cal * Tc) / t1;
                tCore = ((10 * (tCore - 10000)) / TCoeff) + 200;
        tPhy = (Rcal_phy * w_cal * Tc) / t1;
                tPhy = ((10 * (tPhy - 10000)) / TCoeff) + 200;

                t1 = (tCore * 9) / 5 + 32;
                cli_out("Temperature Core = %d.%01dC/%d.%01dF\n", 
                        INT_FRAC_1PT(tCore), INT_FRAC_1PT(t1));
                t1 = (tPhy * 9) / 5 + 32;
                cli_out("Temperature %s = %d.%01dC/%d.%01dF\n", s1,
                        INT_FRAC_1PT(tPhy), INT_FRAC_1PT(t1));
#endif
        }
    } /* Power calculation functions supported */ 
    else {
        cli_out("Platform[%s] does not support the power features.\n",
                BaseboardName);
        return CMD_FAIL;
    }
    
    return CMD_OK;
    } /* function is "power", "thickness" or "temperature" */

    if(!sal_strncasecmp(function,"pot",sal_strlen(function))){
        int pot_num, val, pot;
        uint8  p8, v8;
        max5478_t cmd;

        if (!source || !value) {
            return CMD_USAGE;
        }
        pot_num = sal_ctoi(source, 0);
        if ((pot_num < 0) || (pot_num > 7)) {
            return CMD_USAGE;
        }
        val = sal_ctoi(value, 0);
        if ((val < 0) || (val > 255)) {
            return CMD_USAGE;
        }

        /* Select device in mux */
        if ( (mux = bcm_i2c_open(unit, I2C_LPT_0, flags, 0)) < 0) {
            cli_out("Could not open %s for mux selection:%s\n",
                    I2C_LPT_0, bcm_errmsg(mux));
            return CMD_FAIL;
        }

        /* 
         * Set mux value so we can see the device when we try to open it
         * Device is specified pot_num / 2 
         */
        p8 = pot_num / 2;
        if( (rv = bcm_i2c_write(unit, mux, 0, &p8, 1)) < 0){
            cli_out("Error: write of lpt byte failed:%s\n",
                    bcm_errmsg(rv));
            return CMD_FAIL;
        }

        if ( (pot = bcm_i2c_open(unit, I2C_POT_0, flags, 0)) < 0) {
            cli_out("Could not open %s :%s\n",
                    I2C_POT_0, bcm_errmsg(pot));
            return CMD_FAIL;
        }

        v8 = (uint8) val;
        /* Now set potentiometer */
        cmd.wiper = pot_num % 2;
        cmd.value = v8;
        rv = bcm_i2c_ioctl(unit, pot, I2C_POT_IOC_SET, (void *)&cmd, 0);
        if (rv < 0) {
            cli_out("Error: failed setting pot(%d, %d): %s\n",
                    pot_num, val, bcm_errmsg(rv));
            return CMD_FAIL;
        }

        return CMD_OK;
    } /* pot */
#ifdef SHADOW_SVK
    if(!sal_strncasecmp(function, "test", sal_strlen(function))){
        return _bb_test(unit, baseboard);
    }
#endif
    if(!sal_strncasecmp(function, "current", sal_strlen(function))){

        if( !(baseboard->bbcmd_flags & BB_CURR) || bb_vs_config_len[bx] == 0) {
            cli_out("Error: %s does not support Current Measurement subsystem.\n",
                    BaseboardName);
            return CMD_FAIL;
        }

#ifdef BCM_CALADAN3_SVK
        /* Handle Caladan3 SVK */
        return (bb_caladan3_current(unit, (bx == G37)));
#endif
        
        /* Show all voltages, note that ADC's are always visible,
         * hence we do not need to set the BBMUX value
         */
        for(i = 0; i < bb_current_len[bx]; i++){
            if ( (adc = bcm_i2c_open(unit,
                         bb_current[bx][i].adc,
                         flags, 0)) < 0) {
                cli_out("Could not open %s:%s\n",
                        bb_current[bx][i].adc,
                        bcm_errmsg(adc));
                return CMD_FAIL;
            }
            /* Display Current */
            if ( (bcm_i2c_ioctl(unit, adc, I2C_ADC_QUERY_CHANNEL,
                        &adc_data, bb_current[bx][i].chan) < 0)) {
                cli_out("Error: failed to perform A/D conversions.\n");
            }
            if (i == 0) { /* This is Reference Voltage */
#ifdef COMPILER_HAS_DOUBLE
                curr_zero = adc_data.val * 0.5;
#else
                curr_zero = adc_data.val / 2;
#endif
                continue;
            }
            
#ifdef COMPILER_HAS_DOUBLE
            curr_sens = bb_current[bx][i].sens * 0.001;
            cli_out("%s%s%2.3f Amps "
                    "(Raw=%2.3f min=%2.3f max=%2.3f samples=%d)\n",
                    bb_current[bx][i].function,
                    adc_data.val >= 0 ? "  " : " ",
                    (adc_data.val - curr_zero) / curr_sens,
                    adc_data.val,
                    (adc_data.min - curr_zero) / curr_sens,
                    (adc_data.max - curr_zero) / curr_sens,
                    adc_data.nsamples);
#else
            curr_sens = bb_current[bx][i].sens;
            cli_out("%s%s%d.%03d Amps "
                    "(min=%d.%03d max=%d.%03d delta=%d.%03d samples=%d)\n",
                    bb_current[bx][i].function,
                    adc_data.val >= 0 ? "  " : " ",
                    INT_FRAC_3PT_3(adc_data.val),
                    INT_FRAC_3PT_3(adc_data.min),
                    INT_FRAC_3PT_3(adc_data.max),
                    INT_FRAC_3PT_3(adc_data.delta),
                    adc_data.nsamples);
#endif
        }
        return CMD_OK;
    }
        
    /* Configure or display voltage for any or all BB voltage
     * sources, abbreviate to "v"
     */
    if(!sal_strncasecmp(function,"voltage",sal_strlen(function))){

	/* Check Board Index, which should have voltage config
	 * parameters.
	 */
	if( !(baseboard->bbcmd_flags & BB_VOLT) || bb_vs_config_len[bx] == 0) {
	    cli_out("Error: %s does not support voltage control subsystem.\n",
                    BaseboardName);
	    return CMD_FAIL;
	}

#ifdef BCM_CALADAN3_SVK
        /* Handle Caladan3 SVK */
        return (bb_caladan3_voltage(unit, bx, source, value));
#endif

	
	/* Show all voltages, note that ADC's are always visible,
	 * hence we do not need to set the BBMUX value
	 */
	if( ! source){
	    for(i = 0; i < bb_vs_all_len[bx]; i++){
		if ( (adc = bcm_i2c_open(unit,
					 bb_vs_all[bx][i].adc,
					 flags, 0)) < 0) {
		    cli_out("Could not open %s:%s\n",
                            bb_vs_all[bx][i].adc,
                            bcm_errmsg(adc));
		    return CMD_FAIL;
		}
		/* Display voltage */
		if ( (bcm_i2c_ioctl(unit, adc, I2C_ADC_QUERY_CHANNEL,
				    &adc_data, bb_vs_all[bx][i].chan) < 0)) {
		    cli_out("Error: failed to perform A/D conversions.\n");
		}

		
#ifdef COMPILER_HAS_DOUBLE
		cli_out("%s%s%2.3f Volts "
                        "(min=%2.3f max=%2.3f delta=%2.3f samples=%d)\n",
                        bb_vs_all[bx][i].function,
                        adc_data.val >= 0 ? "  " : " ",
                        adc_data.val,
                        adc_data.min,adc_data.max, adc_data.delta,
                        adc_data.nsamples);
#else
		cli_out("%s%s%d.%03d Volts "
                        "(min=%d.%03d max=%d.%03d delta=%d.%03d samples=%d)\n",
                        bb_vs_all[bx][i].function,
                        adc_data.val >= 0 ? "  " : " ",
                        INT_FRAC_3PT_3(adc_data.val),
                        INT_FRAC_3PT_3(adc_data.min),
                        INT_FRAC_3PT_3(adc_data.max),
                        INT_FRAC_3PT_3(adc_data.delta),
                        adc_data.nsamples);
#endif
               
        }
        return CMD_OK;
    } /* Show all voltages */

    /* Here if a Voltage Source was specified. 
     * Show or configure just that one voltage.
     * This currently works only for configurable voltages.
     */

    /* First, find voltage table entry ... */
    for( i = 0; i < bb_vs_config_len[bx]; i++) {
        if ( !sal_strcasecmp(bb_vs_config[bx][i].function, source))
        break;
    }
    /* Not found - ERROR! */
    if(i == bb_vs_config_len[bx]){
        cli_out("Unknown Voltage source: %s\n", source);
        return CMD_USAGE;
    }

    if (value && (bb_vs_config[bx][i].mux != 0xFF)) {
            char *dev_name;
#ifdef SHADOW_SVK
            uint8 channel;
	    /* Going to set a voltage (need DAC), and the DAC is MUXed */
	    /* Open MUX (lpt0) device */
            dev_name = _i2c_mux_default_dev_name_get();
	    if ( (mux = _i2c_mux_open(unit, flags, 0, dev_name)) < 0) {
		cli_out("Could not open %s for mux selection:%s\n",
                        dev_name?dev_name:"MUX_DEV", bcm_errmsg(mux));
		return CMD_FAIL;
	    }
	  
            channel = _i2c_mux_channel_get(unit, bb_vs_config[bx][i].mux);
        if( (rv = bcm_i2c_write(unit, mux, 0, &channel, 1)) < 0){
        cli_out("Error: write of lpt byte failed:%s\n",
                bcm_errmsg(rv));
        return CMD_FAIL;
        }
#else
	    /* Going to set a voltage (need DAC), and the DAC is MUXed */
	    /* Open MUX (lpt0) device */
            dev_name = _i2c_mux_default_dev_name_get();
	    if ( (mux = bcm_i2c_open(unit, dev_name, flags, 0)) < 0) {
		cli_out("Could not open %s for mux selection:%s\n",
                        dev_name?dev_name:"MUX_DEV", bcm_errmsg(mux));
		return CMD_FAIL;
	    }
	  
	    /* Set mux value so we can see the device when we try to open it*/
	    if( (rv = bcm_i2c_write(unit, mux, 0, &bb_vs_config[bx][i].mux, 1)) < 0){
		cli_out("Error: write of lpt byte failed:%s\n",
                        bcm_errmsg(rv));
		return CMD_FAIL;
	    }
#endif /* SHADOW_SVK */
    } /* Setting a voltage, needed to manipulate MUX for the DACs */

    if (value) {
        /* Going to calibrate and/or set a voltage; open the correct DAC */
        if ( (dac = bcm_i2c_open(unit, bb_vs_config[bx][i].dac,
                     flags, 0)) < 0) {
            
        cli_out("Could not open %s:%s\n",
                bb_vs_config[bx][i].dac,
                bcm_errmsg(dac));
        return CMD_FAIL;
        }
    }

    /* Open the ADC Device (either adc0 or adc1) */
    if ( (adc = bcm_i2c_open(unit, bb_vs_config[bx][i].adc,
                 flags, 0)) < 0) {
        cli_out("Could not open %s:%s\n",
                bb_vs_config[bx][i].adc,
                bcm_errmsg(adc));
        return CMD_FAIL;
    }

    /* From this point on, we have an ADC (and possibly its 
     * associated DAC) open and ready for commands.
     */

	if (value) {
	    /* Going to calibrate and/or set a voltage; open the correct DAC */
	    if ( (dac = bcm_i2c_open(unit, bb_vs_config[bx][i].dac,
				     flags, 0)) < 0) {
	        
		cli_out("Could not open %s:%s\n",
                        bb_vs_config[bx][i].dac,
                        bcm_errmsg(dac));
		return CMD_FAIL;
	    }
	}

	/* Open the ADC Device (either adc0 or adc1) */
	if ( (adc = bcm_i2c_open(unit, bb_vs_config[bx][i].adc,
				 flags, 0)) < 0) {
	    cli_out("Could not open %s:%s\n",
                    bb_vs_config[bx][i].adc,
                    bcm_errmsg(adc));
	    return CMD_FAIL;
	}

	/* From this point on, we have an ADC (and possibly its 
	 * associated DAC) open and ready for commands.
	 */

	/* Set and/or possibly calibrate voltage, we need to do this
	 * at least once (the first time), and possibly later if the
	 * user requests a calibration of a source ...
	 */
	if( value ) {
	    /* cli_out("calibration: board %d: subsys%d\n",bx, i); */

	    /* Always designate the DAC calibration table. This info
	     * is needed for any voltage setting operations. If
	     * a calibration is performed, the table is updated.  
	     */
	    if ( (bcm_i2c_ioctl(unit, dac,
				I2C_DAC_IOC_SET_CALTAB,
				(void*)dac_params[bx],
				dac_param_len[bx]) < 0)) {
		cli_out("Error: failed to set DAC calibration table.\n");
	    }
	    
	    /* Calibrate the DAC->ADC loop 
	     * (if not yet done, or explicitly requested). */
	    if ( (show_cal = !sal_strcasecmp(value,"calibrate")) ||
		 (show_cal = !sal_strcasecmp(value,"cal")) ||
		 !(show_cal = bb_vs_config[bx][i].flags & DAC_CALIBRATED) ) {

	        if (show_cal)
		    cli_out("calibration: board %d: subsys%d\n",bx, i);

		/* We need to calibrate the DAC to compute the min,
		 * max, and the Volts/step values that result at the
		 * associated ADC. 
		 */

		/* Set the DAC to the (digital) value which         */
		/* generates the minimum analog voltage at the ADC. */
		if ( (bcm_i2c_ioctl(unit, dac,
				    I2C_DAC_IOC_SETDAC_MIN,
				    &volts, bb_vs_config[bx][i].cal_idx) < 0)) {
		    cli_out("Error: failed to set min DAC Vout.\n");
		}

		/* Read the resultant (minimum) voltage at the ADC. */
		if ( (bcm_i2c_ioctl(unit, adc, I2C_ADC_QUERY_CHANNEL,
				    &adc_data, bb_vs_config[bx][i].chan) < 0)) {
		    cli_out("Error: failed to read A/D min voltage:VDD_%s.\n",
                            bb_vs_config[bx][i].function);
		}
		volts = adc_data.val;
#ifdef SHADOW_SVK
                if (SOC_IS_SHADOW(unit)) {
                    /* Hard code the min and Max as the voltage rails are
                     * clamped */
                    volts = dac_params[bx][i].min;
                }
#endif

            if (show_cal)
#ifdef COMPILER_HAS_DOUBLE
            cli_out("Min=%2.3fV, ", volts);
#else
            cli_out("Min=%d.%03dV, ", INT_FRAC_3PT_3(volts));
#endif

		/* Update DAC calibration table with this minimum voltage. */
		if ( (bcm_i2c_ioctl(unit, dac,
				    I2C_DAC_IOC_CALIBRATE_MIN,
				    &volts, bb_vs_config[bx][i].cal_idx) < 0)) {
		    cli_out("Error: failed to update min DAC Vout.\n");
		}

		/* Set the DAC to the (digital) value which         */
		/* generates the maximum analog voltage at the ADC. */
		if ( (bcm_i2c_ioctl(unit, dac,
				    I2C_DAC_IOC_SETDAC_MAX,
				    &volts, bb_vs_config[bx][i].cal_idx) < 0)) {
		    cli_out("Error: failed to set max DAC Vout.\n");
		}

		/* Read the resultant (maximum) voltage at the ADC. */
		if ( (bcm_i2c_ioctl(unit, adc, I2C_ADC_QUERY_CHANNEL,
				    &adc_data, bb_vs_config[bx][i].chan) < 0)) {
		    cli_out("Error: failed to read A/D max voltage:VDD_%s.\n",
                            bb_vs_config[bx][i].function);
		}
		volts = adc_data.val;
#ifdef SHADOW_SVK
                if (SOC_IS_SHADOW(unit)) {
                    /* Hard code the min and Max as the voltage rails are
                     * clamped */
                    volts = dac_params[bx][i].max;
                }
#endif
        
            if (show_cal)
#ifdef COMPILER_HAS_DOUBLE
            cli_out("Max=%2.3fV\n", volts);
#else
            cli_out("Max=%d.%03dV\n", INT_FRAC_3PT_3(volts));
#endif

		/* Update DAC calibration table with this maximum voltage. */
		if ( (bcm_i2c_ioctl(unit, dac,
				    I2C_DAC_IOC_CALIBRATE_MAX,
				    &volts, bb_vs_config[bx][i].cal_idx) < 0)) {
		    cli_out("Error: failed to update max DAC Vout.\n");
		}

		/* Very important, set the DAC back to nominal 
		 * (bootup midscale).
		 */
		if( (rv = bcm_i2c_ioctl(unit, dac, 
					I2C_DAC_IOC_SETDAC_MID,
					&volts, bb_vs_config[bx][i].cal_idx) < 0)) {
		    cli_out("Error: resetting of DAC to nominal value failed:%s\n",
                            bcm_errmsg(rv));
		    return CMD_FAIL;
		}		

				
		/* Now, update the calibration table with the
		 * analog voltage step per DAC digital step.
		 * (also displays all of the calibration table info)
		 */
		if ( (bcm_i2c_ioctl(unit, dac,
				    I2C_DAC_IOC_CALIBRATE_STEP,
				    &volts, bb_vs_config[bx][i].cal_idx) < 0)) {
		    cli_out("Error: failed to calibrate DAC VDD_%s.\n",
                            bb_vs_config[bx][i].function);
		}
		/* Indicate that this DAC-ADC pair is calibrated */
		bb_vs_config[bx][i].flags = DAC_CALIBRATED;
	    } /* DAC->ADC not yet calibrated or calibration was requested */

	    /* Get the user's desired analog voltage value to set. */

	    if (!sal_strncasecmp(value,"nominal",sal_strlen(value))) {
		cli_out("Resetting %s voltage to nominal\n", source);
	        if ( (bcm_i2c_ioctl(unit, dac,
				    I2C_DAC_IOC_SETDAC_MID,
				    &volts, bb_vs_config[bx][i].cal_idx) < 0)) {
		    cli_out("Error: failed to reset VDD_%s DAC to mid-range value.\n",
                            bb_vs_config[bx][i].function);
		    
		    return CMD_FAIL;
		}

		/* Read the ADC to get the actual resultant voltage value. */
		if ( (bcm_i2c_ioctl(unit, adc, I2C_ADC_QUERY_CHANNEL,
				    &adc_data, bb_vs_config[bx][i].chan) < 0)) {
		    cli_out("Error: failed to perform A/D conversions.\n");
		}

		/* Display the recently configured voltage */
#ifdef COMPILER_HAS_DOUBLE
		cli_out("VDD_%s = %2.3f Volts "
                        "(min=%2.3f max=%2.3f delta=%2.3f samples=%d)\n",
                        bb_vs_config[bx][i].function, adc_data.val,
                        adc_data.min,adc_data.max, adc_data.delta,
                        adc_data.nsamples);
#else
		cli_out("VDD_%s = %d.%03d Volts "
                        "(min=%d.%03d max=%d.%03d delta=%d.%03d samples=%d)\n",
                        bb_vs_config[bx][i].function,
                        INT_FRAC_3PT_3(adc_data.val),
                        INT_FRAC_3PT_3(adc_data.min),
                        INT_FRAC_3PT_3(adc_data.max),
                        INT_FRAC_3PT_3(adc_data.delta),
                        adc_data.nsamples);
#endif
            return CMD_OK;
        }

        /* Here if a voltage level was specified */
        
#ifdef COMPILER_HAS_DOUBLE
        volts = atof(value); 
#else
            /* Parse input in V and convert to uV */
            volts = _shr_atof_exp10(value, 6);
#endif
        if(volts > 0){

		cli_out("Configuring voltage on [%s]\n", source);
	    
		/* Finally, set the actual voltage ... */
		if ( (bcm_i2c_ioctl(unit, dac,
				    I2C_DAC_IOC_SET_VOUT,
				    &volts, bb_vs_config[bx][i].cal_idx) < 0)) {
#ifdef COMPILER_HAS_DOUBLE
		    cli_out("Error: failed to set VDD_%s to %2.3fV.\n",
                            bb_vs_config[bx][i].function, volts);
#else
		    cli_out("Error: failed to set VDD_%s to %d.%03dV.\n",
                            bb_vs_config[bx][i].function, INT_FRAC_3PT_3(volts));
#endif
		    cli_out("Run \"bb voltage %s calibrate\" to view allowable range.\n",
                            bb_vs_config[bx][i].function);

		    return CMD_FAIL;
		}

		

		/* Read the ADC to get the actual resultant voltage value. */
		if ( (bcm_i2c_ioctl(unit, adc, I2C_ADC_QUERY_CHANNEL,
				    &adc_data, bb_vs_config[bx][i].chan) < 0)) {
		    cli_out("Error: failed to perform A/D conversions.\n");
		}

		/* Display the recently configured voltage */
#ifdef COMPILER_HAS_DOUBLE
		cli_out("VDD_%s = %2.3f Volts "
                        "(min=%2.3f max=%2.3f delta=%2.3f samples=%d)\n",
                        bb_vs_config[bx][i].function, adc_data.val,
                        adc_data.min,adc_data.max, adc_data.delta,
                        adc_data.nsamples);
#else
		cli_out("VDD_%s = %d.%03d Volts "
                        "(min=%d.%03d max=%d.%03d delta=%d.%03d samples=%d)\n",
                        bb_vs_config[bx][i].function,
                        INT_FRAC_3PT_3(adc_data.val),
                        INT_FRAC_3PT_3(adc_data.min),
                        INT_FRAC_3PT_3(adc_data.max),
                        INT_FRAC_3PT_3(adc_data.delta),
                        adc_data.nsamples);
#endif
	    }
	} /* Set a single voltage */
	else {
	    /* Display a single voltage */
	    if ( (bcm_i2c_ioctl(unit, adc, I2C_ADC_QUERY_CHANNEL,
				&adc_data, bb_vs_config[bx][i].chan) < 0)) {
		cli_out("Error: failed to perform A/D conversions.\n");
	    }

#ifdef COMPILER_HAS_DOUBLE
	    cli_out("VDD_%s = %2.3f Volts "
                    "(min=%2.3f max=%2.3f delta=%2.3f samples=%d)\n",
                    bb_vs_config[bx][i].function, adc_data.val,
                    adc_data.min,adc_data.max, adc_data.delta,
                    adc_data.nsamples);
#else
	    cli_out("VDD_%s = %d.%03d Volts "
                    "(min=%d.%03d max=%d.%03d delta=%d.%03d samples=%d)\n",
                    bb_vs_config[bx][i].function, 
                    INT_FRAC_3PT_3(adc_data.val),
                    INT_FRAC_3PT_3(adc_data.min),
                    INT_FRAC_3PT_3(adc_data.max),
                    INT_FRAC_3PT_3(adc_data.delta),
                    adc_data.nsamples);
#endif

    } /* Display a single voltage */
    } /* function is "voltage" */

    else {
    return CMD_USAGE;
    } /* Unknown function */ 

    return CMD_OK ;
} /* end cmd_bb() */

STATIC int dac_devs_init (int unit,int bx,char *dev_name) 
{
    int mux;
    int i;
    int dac;
    int rv = 0;
    int flags = 0;
    
    /* open mux device used for all channel's DAC devices
     * DAC devices behind the mux device need not be the same device.
     */
    if ( (mux = bcm_i2c_open(unit, dev_name, flags, 0)) < 0) {
        cli_out("Could not open %s for mux selection:%s\n",
                dev_name?dev_name:"MUX_DEV", bcm_errmsg(mux));
        return CMD_FAIL;
    }

    /* initialize each dac device behind the I2C mux device */
    for (i = 0; i < bb_vs_config_len[bx]; i++) {
        if (bb_vs_config[bx][i].dac == NULL) {
            break;
        }
        if( (rv = bcm_i2c_write(unit, mux, 0,
                                &bb_vs_config[bx][i].mux, 1)) < 0){
            cli_out("Error: write of mux device byte failed:%s\n",
                    bcm_errmsg(rv));
            return CMD_FAIL;
        }

        /* get dac driver's device id */
        if ((dac = bcm_i2c_open(unit, bb_vs_config[bx][0].dac,
                                flags, 0)) < 0) {
            cli_out("Could not open %s:%s\n",
                    bb_vs_config[bx][0].dac, bcm_errmsg(dac));
            return CMD_FAIL;
        }
        soc_i2c_device(unit, dac)->driver->load(unit,dac,NULL,0);
    }

    return SOC_E_NONE;
}

#if defined(SHADOW_SVK) || defined(BCM_CALADAN3_SVK)

#define PIO_PORT_0   (1<<0)
#define PIO_PORT_1   (1<<1)

/*
 */
char cmd_pio_usage[] =
    "Usages:\n\t"
    "       pio dev port [data]\n\t"
    "           - show and change contents of parallel IO ports\n\t"
#ifdef BCM_CALADAN3_SVK
    "       pio [field=<field name>] [data=<data>]\n\t"
    "           - show and change any field \n\t"
#endif
    "\n";

cmd_result_t
cmd_pio(int unit, args_t *a)
{
    int dev, port, data;
    int dev_mask, port_mask;
    parse_table_t pt;
    int i, max;
    int fd, rv;
    uint32 len;
    int banner;
    uint8 rd_data, wr_data;
    char *name; 
#ifdef BCM_CALADAN3_SVK
    char *field, field_name[64];
    iop_bit_config_t *fields;
    int datamask;
    int pos = 0, size = 0, rw = 0;
    int all_fields = 0;
    int cpui2c_mode = 0;
    int bus = 0;
#endif

#ifdef SHADOW_SVK
    max = 2;
#elif BCM_CALADAN3_SVK
    max = 7;
#endif
    dev = port = data = -1;
    dev_mask = 1 << max;
    port_mask = 0xff;

        parse_table_init(0, &pt);
        parse_table_add(&pt, "DEv", PQ_DFL|PQ_INT, 0,
                        &dev, NULL);
        parse_table_add(&pt, "Port", PQ_DFL|PQ_INT, 0,
                        &port, NULL);
        parse_table_add(&pt, "Data", PQ_DFL|PQ_INT, 0,
                        &data, NULL);
#ifdef BCM_CALADAN3_SVK
        parse_table_add(&pt, "field", PQ_STRING, "all",
                        &field, NULL);
#endif
        if (parse_arg_eq(a, &pt) < 0) {
            cli_out("%s: Invalid argument: %s\n", ARG_CMD(a), ARG_CUR(a));
            parse_arg_eq_done(&pt);
            return(CMD_FAIL);
        }
#ifdef BCM_CALADAN3_SVK
        sal_strncpy(field_name, field, 64);
#endif
        parse_arg_eq_done(&pt);

#ifdef BCM_CALADAN3_SVK

    if ((dev == -1) && (sal_strcmp(field_name, "all")==0)) {
        all_fields = 1;
    }
    if ((sal_strcmp(field_name, "all") != 0) 
           && (dev != -1) && (port != -1)) {
        cli_out("Cannot mix field and dev,port options");
        return(CMD_FAIL);
    }
    if (all_fields && (data != -1)) {
        cli_out("Cannot write to all fields with same value");
        return(CMD_FAIL);
    }
    if ((dev == -1) && (port == -1)) { 
        if ( G36 == soc_get_board_id()) {
            max = PIO_MAX_BIT_FIELDS_48G;
            fields = pio_bit_config_48g;
        } else {
            max = PIO_MAX_BIT_FIELDS_100G;
            fields = pio_bit_config_100g;
        }
        for(i=0; i < max; i++) {
            if ((sal_strstr(fields[i].name, field_name) != NULL) ||
                 (all_fields == 1)) {
                dev = fields[i].dev;
                port = fields[i].port;
                pos = fields[i].pos;
                size = fields[i].size;
                rw = fields[i].rw;
            } else {
                continue; /* locate the given field */
            }
            if ((data != -1) && (!rw)) {
                cli_out("Field %s is Read only\n", 
                        fields[i].name);
                return (CMD_FAIL);
            }
    
            pio_devname(dev, &name);
            /* 40bit IOPs on CPU bus */
            if (soc_caladan3_is_board_rev_a()) {
                /* On rev A, 40bit iop is on dut bus */
                cpui2c_mode = (sal_strncasecmp(name, "iop", 3) == 0) ? 0 : 1;
            } else {
                cpui2c_mode = 1;
            }
            if (!cpui2c_mode) {
                fd = bcm_i2c_open(unit, name, 0, 0);
                if (fd < 0) {
                    continue;
                    if ( G36 == soc_get_board_id()) {
                        /* Caladan3 48G SVK should have this device */
                        cli_out("failed to open %s\n", name);
                        return (CMD_FAIL);
                    } else {
                        /* Caladan3 100G SVK maynot have this device */
                        continue;
                    }
                }
                datamask = ((1 << size)-1) << pos;
                rv = bcm_i2c_read(unit, fd, I2C_IOP_IN(port),
                                   (uint8 *)&rd_data, &len);
                if (rv == SOC_E_NONE) {
                    if ((data >= 0) && (data <= (1<<size)-1)) {
                        wr_data = rd_data & ~datamask;
                        data &= (1 << size) - 1;
                        wr_data |= data << pos;
                        cli_out("writing port dev %d %d <- %d(0x%x)\n",
                                dev, port,  wr_data, wr_data);
                        rv = bcm_i2c_write(unit, fd, I2C_IOP_OUT(port), (uint8 *)&wr_data, 1);
                    } else {
                        rd_data = (rd_data & datamask) >> pos;
                        cli_out("Field: %s =  0x%x\n", 
                                fields[i].name, rd_data);
                    }
                } else {
                    cli_out("Read data failed for %s\n",
                            fields[i].name);
                    return (CMD_FAIL);
                }
            } else {
                bus = cpu_i2c_defaultbus();
                fd = cpu_i2c_dev_open(bus, name, 0, 0);
                if (fd < 0) {
                    if ( G36 == soc_get_board_id()) {
                        cli_out("Device open failed for %s\n", name);
                        return (CMD_FAIL);
                    } else {
                        continue;
                    }
           
                }
                datamask = ((1 << size)-1) << pos;
                rv = cpu_i2c_dev_read(bus, fd, I2C_IOP_IN(port), 
                                   (uint8 *)&rd_data, &len);
                if (rv == SOC_E_NONE) {
                    if ((data >= 0) && (data <= (1<<size)-1)) {
                        wr_data = rd_data & ~datamask;
                        data &= (1 << size) - 1;
                        wr_data |= data << pos;
                        cli_out("writing port dev %d %d <- %d\n",
                                dev, port,  wr_data);
                        rv = cpu_i2c_dev_write(bus, fd, I2C_IOP_OUT(port), (uint8 *)&wr_data, 1);
                    } else {
                        rd_data = (rd_data & datamask) >>  pos;
                        cli_out("Field: %s =  0x%x\n", 
                                fields[i].name, rd_data);
                    }
                } else {
                    cli_out("Read data failed for %s\n",
                            fields[i].name);
                    return (CMD_FAIL);
                }
            }
       }
       return (CMD_OK);
    } 
#endif

    if (dev != -1) {
        if (dev >= 0 && dev < max) {
            dev_mask = (1 << dev);
        } else {
            cli_out("Valid dev 0..%d\n", max);
            return CMD_FAIL;
        }
    }
    if (port != -1) {
        if (port >= 0 && port <= 4) {
            port_mask = (1 << port);
        } else {
            cli_out("Valid port 0..4\n");
            return CMD_FAIL;
        }
    }

    for (dev = 0; dev < max; dev++) {

        pio_devname(dev, &name);

        if (!((1<<dev) & dev_mask)) {
            continue;
        }
#ifdef BCM_CALADAN3_SVK
        /* 40bit IOPs on CPU bus */
        if (SOC_IS_CALADAN3(unit)) {
            pio_devname(dev, &name);
            if (soc_caladan3_is_board_rev_a()) {
                /* On rev A, 40bit iop is on dut bus */
                cpui2c_mode = (sal_strncasecmp(name, "iop", 3) == 0) ? 0 : 1;
            } else {
                cpui2c_mode = 1;
            }
        }
        if (cpui2c_mode) {
            bus = cpu_i2c_defaultbus();
            fd = cpu_i2c_dev_open(bus, name, 0, 0);
            if (fd < 0) {
                cli_out("Device open failed for %s\n", name);
                return (CMD_FAIL);
            }
        } else 
#endif
        {
            fd = bcm_i2c_open(unit, name, 0, 0);
            if (fd < 0) {
                cli_out("Failed opening %s\n", name);
                continue;
            }
        }

        banner = 0;
        for (port = 0; port < 5; port++) {
            int bit;
            if (!((1<<port) & port_mask)) {
                continue;
            }
            if (data != -1) {
                if (data > 255) {
                    cli_out("Data must be 0..255\n");
                    break;
                }
                wr_data = data;
                /*cli_out("writing port dev %d %d <- %d\n", dev, port,
                          * wr_data);*/

#ifdef BCM_CALADAN3_SVK
                if (cpui2c_mode) {
                    rv = cpu_i2c_dev_write(bus, fd, I2C_IOP_OUT(port), (uint8 *)&wr_data, 1);
 
                } else {
#endif
                    rv = bcm_i2c_write(unit, fd, I2C_IOP_OUT(port), (uint8 *)&wr_data, 1);
#ifdef BCM_CALADAN3_SVK
                }
#endif

                if (rv != SOC_E_NONE) {
                    cli_out("%s:Port%d Is INPUT Port\n", name, port);
                }
            } else {
                for(i = 0; i < 2; i++) {
#ifdef BCM_CALADAN3_SVK
                    if (cpui2c_mode) {
                       rv = cpu_i2c_dev_read(bus, fd,  0x8*i +port, 
                                   (uint8 *)&rd_data, &len);
                    } else {
#endif
                        rv = bcm_i2c_read(unit, fd, 0x8 * i + port,
                                       (uint8 *)&rd_data, &len);
#ifdef BCM_CALADAN3_SVK
                    }
#endif
                    if (rv != SOC_E_NONE) {
                        cli_out("Failed reading %s: %s\n", name, bcm_errmsg(rv));
                    }
                    if (! banner) {
                        banner++;
                        cli_out("PIO %s:", name);
                    }
                    cli_out("%s   Port[%d]:  ", 
                            (i == 0) ? "\n Input:": "  Output:",port);
                    for (bit = 7; bit >= 0; bit--) {
                        if (bit == 3) {
                            cli_out(" ");
                        }
                        cli_out("%c", (rd_data & (1 << bit)) ? '1' : '0');
                    }
                }
            }
        }
        cli_out("\n");
    }

    return CMD_OK ;
}
#endif /* SHADOW_SVK */



#endif /* INCLUDE_I2C */


#endif /* NO_SAL_APPL */
