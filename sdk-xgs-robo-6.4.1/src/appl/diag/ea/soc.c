/*
 * $Id: soc.c,v 1.20 Broadcom SDK $
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

#include <shared/bsl.h>

#include <appl/diag/system.h>
#include <appl/diag/parse.h>
#include <bcm/error.h>
#include <soc/mem.h>
#include <sal/types.h>
	
#include <soc/ea/tk371x/onu.h>
#define MAX_FW_FILE_NAME_LEN 255

#if defined(BROADCOM_DEBUG)
typedef struct fw_upgrade_s{	
	uint32 fw_file_type;	
	char fw_file_name[MAX_FW_FILE_NAME_LEN + 1];	
	int unit;
}fw_upgrade_t;
#ifdef NO_FILEIO
/*
 * Nothing does
 */
#else
#define FW_FILE_TYPE_BOOT	0
#define FW_FILE_TYPE_APP	1
#define FW_FILE_TYPE_PERS	2	
#define BUF_SIZE	800000
static uint8 buff[BUF_SIZE] = {0};    
static fw_upgrade_t fw;
extern int _soc_ea_firmware_ver_get(int unit, uint16 *ver);
extern int soc_ea_firmware_upgrade(int unit, int type, int length, uint8* buf);
int fw_upgrade_file(void *fw){    	
	FILE *fpointer = NULL;    	
	uint16 fw_ver, fw_ver_file;			
	int len = 0;    	
	int rv = 0;        	
	fw_upgrade_t *f = (fw_upgrade_t *)fw;
        int unit = f->unit;

	fw_ver = fw_ver_file = 0;
	rv = _soc_ea_firmware_ver_get(unit, &fw_ver);	
	if (CMD_OK != rv){
		cli_out("Get firmware version FAIL!\n");
		return CMD_FAIL;
	}
	fpointer = fopen(f->fw_file_name,"r");    	
	if(NULL == fpointer){        		
		cli_out("Open file failed.\n\t");        	
		return CMD_FAIL;
	}       	
	len = fread(buff, 1, BUF_SIZE, fpointer);
	fclose(fpointer);
	memcpy((uint8*)&fw_ver_file, &buff[2], 2);
	if (fw_ver == fw_ver_file){
		switch (f->fw_file_type){
			case FW_FILE_TYPE_BOOT:
				cli_out("Soc boot firmware also is the latest version!\n");
				break;
			case FW_FILE_TYPE_APP:
				cli_out("Soc app firmware also is the latest version!\n");
				break;
			case FW_FILE_TYPE_PERS:
				cli_out("Soc pers firmware also is the latest version!\n");
				break;
			default:
				break;	
			}
		return CMD_OK;
	}	
	rv = soc_ea_firmware_upgrade(unit, f->fw_file_type, len, buff);   	
	if (CMD_OK != rv){		
		cli_out("Upgrade Failed.\n");
		return CMD_FAIL;
	}else{
		cli_out("Upgrade Success...\n");
	}	
	return CMD_OK;
}
#endif /* NO_FILEIO */
#endif /* BROADCOM_DEBUG */

/*
 * Function:    cmd_ea_soc
 * Purpose: Print soc control information if compiled in debug mode.
 * Parameters:  u - unit #
 *              a - pointer to args, expects <unit> ...., if none passed,
 *                  default unit # used.
 * Returns: CMD_OK/CMD_FAIL
 */
cmd_result_t
cmd_ea_soc(int u, args_t *a)
{
#if defined(BROADCOM_DEBUG)
    char *subcmd;

	if (! sh_check_attached(ARG_CMD(a), u)) {
		return CMD_FAIL;
	}

	if ((subcmd = ARG_GET(a)) == NULL) {
		return CMD_USAGE;
	}
    
    if(!sal_strncasecmp(subcmd, "debug",5)){
        uint32 dbg_msk = 0;
        if ((subcmd = ARG_GET(a)) == NULL) {
		    return CMD_USAGE;
	    }
        dbg_msk = parse_integer(subcmd);
        soc_ea_dbg_level_set(dbg_msk);
        return CMD_OK;   
    }else if(!sal_strncasecmp(subcmd, "dbgdump",7)){
       soc_ea_dbg_level_dump();
       return CMD_OK;
    }else if(!sal_strncasecmp(subcmd, "gpio",4)){
        uint32 mask;
        uint32 value;
        uint32 flag;

        if ((subcmd = ARG_GET(a)) == NULL) {
		    return CMD_USAGE;
	    }
        if(!sal_strncasecmp(subcmd, "write",5)){
            if ((subcmd = ARG_GET(a)) == NULL) {
    		    return CMD_USAGE;
    	    }
            
            flag = parse_integer(subcmd);
			
            if ((subcmd = ARG_GET(a)) == NULL) {
    		    return CMD_USAGE;
    	    }
           
            mask = parse_integer(subcmd);
            if ((subcmd = ARG_GET(a)) == NULL) {
    		    return CMD_USAGE;
    	    }
            value = parse_integer(subcmd);
            if(SOC_E_NONE != soc_gpio_write(u,flag,mask,value)){
                return CMD_FAIL;
            }else{
                return CMD_OK;
            }
        }else if(!sal_strncasecmp(subcmd, "read",4)){
            if ((subcmd = ARG_GET(a)) == NULL) {
    		    return CMD_USAGE;
    	    }
            flag = parse_integer(subcmd);
            
            if(SOC_E_NONE != soc_gpio_read(u,flag,&value)){
                return CMD_FAIL;
            }else{
                cli_out("gipo value = %08x\n",value);
                return CMD_OK;
            }
        }else{
            return CMD_USAGE;
        }
    }else if(!sal_strncasecmp(subcmd, "reset",5)){
        if(soc_chip_tk371x_reset(u) != SOC_E_NONE){
            return CMD_FAIL;
        }else{
            return CMD_OK;
        } 
    }else if(!sal_strncasecmp(subcmd, "nvserase",8)){
        if(soc_nvs_erase(u) != SOC_E_NONE){
            return CMD_FAIL;
        }else{
            return CMD_OK;
        } 
    }else if(!sal_strncasecmp(subcmd,"dbshow",6)){
        soc_ea_private_db_dump(u);
        return CMD_OK;
    }else if (!sal_strncasecmp(subcmd, "upgrade", 7)){
#ifdef NO_FILEIO
	/*nothing does*/
#else		
		int rv;

		if ((subcmd = ARG_GET(a)) == NULL){			
			return CMD_USAGE;		
		}		
		if (!sal_strncasecmp(subcmd, "app", 3)){       			
			if((subcmd = ARG_GET(a)) != NULL){				
				strncpy(fw.fw_file_name, subcmd, MAX_FW_FILE_NAME_LEN);			
			}else{				
				return CMD_USAGE;			
			}			
			fw.fw_file_type = FW_FILE_TYPE_BOOT;		
		}else if (!sal_strncasecmp(subcmd, "boot", 4)){			
			if((subcmd = ARG_GET(a)) != NULL){				
				strncpy(fw.fw_file_name, subcmd, MAX_FW_FILE_NAME_LEN);			
			}else{				
				return CMD_USAGE;			
			}			
			fw.fw_file_type = FW_FILE_TYPE_APP;		
		}else if (!sal_strncasecmp(subcmd, "pers", 4)){			
			if((subcmd = ARG_GET(a)) != NULL){				
				strncpy(fw.fw_file_name, subcmd, MAX_FW_FILE_NAME_LEN);			
			}else{				
				return CMD_USAGE;			
			}			
			fw.fw_file_type = FW_FILE_TYPE_PERS;		
		}else{			
			uint16 fw_ver;	
					
			rv = _soc_ea_firmware_ver_get(u, &fw_ver);			
			if (CMD_OK != rv){				
				return CMD_USAGE;			
			}			
			cli_out("%04x\n", fw_ver);			
			return CMD_OK;		
		}		
		fw.unit = 1;	
		fw.fw_file_name[MAX_FW_FILE_NAME_LEN] = 0;
		rv = fw_upgrade_file((void *)&fw);
		if (CMD_OK != rv){
			return rv;
		}
#undef FW_FILE_TYPE_BOOT
#undef FW_FILE_TYPE_APP
#undef FW_FILE_TYPE_PERS
#endif /* NO_FILEIO */
	}else{
        return CMD_USAGE;
    }
	return CMD_OK;
#else
    cli_out("%s: Warning: Not compiled with BROADCOM_DEBUG enabled\n", ARG_CMD(a));
    return(CMD_OK);
#endif
}
