/*
 * $Id: bcmnvram.h 1.3 Broadcom SDK $
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
 * NVRAM variable manipulation
 */

#ifndef _bcmnvram_h_
#define _bcmnvram_h_

#ifndef _LANGUAGE_ASSEMBLY

#include <shared/et/typedefs.h>

struct nvram_header {
	unsigned long magic;
	unsigned long len;
	unsigned long crc_ver_init;	/* 0:7 crc, 8:15 ver, 16:27 init, mem. test 28, 29-31 reserved */
	unsigned long config_refresh;	/* 0:15 config, 16:31 refresh */
	unsigned long reserved;
};

struct nvram_tuple {
	char *name;
	char *value;
	struct nvram_tuple *next;
};

/* Compatibility */
typedef struct nvram_tuple EnvRec;

/*
 * Get the value of an NVRAM variable
 * @param	name	name of variable to get
 * @return	value of variable or NULL if undefined
 */
extern char * nvram_get(const char *name);

/* 
 * Get the value of an NVRAM variable
 * @param	name	name of variable to get
 * @return	value of variable or NUL if undefined
 */
#define nvram_safe_get(name) (nvram_get(name) ? : "")

/*
 * Set the value of an NVRAM variable
 * @param	name	name of variable to set
 * @param	value	value of variable
 * @return	0 on success and errno on failure
 * NOTE: use nvram_commit to commit this change to flash.
 */
extern int nvram_set(const char *name, const char *value);

/*
 * Unset an NVRAM variable
 * @param	name	name of variable to unset
 * @return	0 on success and errno on failure
 * NOTE: use nvram_commit to commit this change to flash.
 */
extern int nvram_unset(const char *name);

/*
 * Permanently commit NVRAM variables
 * @return	0 on success and errno on failure
 */
extern int nvram_commit(void);

/*
 * Get all NVRAM variables (format name=value\0 ... \0\0)
 * @param	buf	buffer to store variables
 * @param	count	size of buffer in bytes
 * @return	0 on success and errno on failure
 */
extern int nvram_getall(char *buf, int count);

/*
 * Invalidate the current NVRAM header
 * @return	0 on success and errno on failure
 */
extern int nvram_invalidate(void);

/*
 * Dump NVRAM data including header into a buffer for file backup. 
 * The data format is the same as what is in the flash - 5 32-bit words 
 * header followed by name=value pairs (see nvram_getall() for name=value 
 * pairs format).
 *
 * @param	buf	- buffer to nvram data
 * @param	count	- size of buffer in bytes
 * @return	0 on success and others on failure
 *		-1 - buffer pointer is NULL or buffer is not 4 byte aligned,
 * 		-2 - no nvram,
 *		-3 - buffer too small
 * @param	count	- buffer used in bytes
 */
extern int nvram_dump(char *buf, int *count);

/*
 * Verify NVRAM data read from backup file and update nvram. 
 * The data format is the same as what is in the flash - 5 32-bit words 
 * header followed by name=value pairs (see nvram_getall() for name=value 
 * pairs format).
 *
 * @param	buf 	- buffer to nvram data
 * @param	count	- size of buffer in bytes
 * @return	0 on success and others on failure
 *		-1 - buffer pointer is NULL, buffer is not 4 byte aligned,
 *		-2 - wrong magic,
 *		-3 - wrong buffer size,
 *		-4 - CRC checking failure,
 *		-5 - flash init failed
 */
extern int nvram_restore(char *buf, int count);

#endif /* _LANGUAGE_ASSEMBLY */

#define NVRAM_MAGIC		0x48534C46	/* 'FLSH' */
#define NVRAM_VERSION		1
#define NVRAM_HEADER_SIZE	20
#define NVRAM_FIRST_LOC		0xbfcf8000
#define NVRAM_LAST_LOC		0xbfff8000
#define NVRAM_LOC_GAP		0x100000
#define NVRAM_SPACE		0x8000

#endif /* _bcmnvram_h_ */
