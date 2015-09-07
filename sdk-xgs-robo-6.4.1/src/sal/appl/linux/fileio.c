/*
 * $Id: fileio.c,v 1.5 Broadcom SDK $
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
 * File: 	fileio.c
 * Purpose:	File I/O
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/string.h>

#include <sal/appl/io.h>

static char _homedir[256];

int
sal_homedir_set(char *dir)
{
    strncpy(_homedir, dir, sizeof(_homedir));
    return 0;
}

char *
sal_homedir_get(char *buf, size_t size)
{
    strncpy(buf, _homedir, size);
    return buf;
}

/*
 * Get current working directory.
 *
 * NOTE: this version of cwd always includes the trailing slash so
 * a filename should be directly appended.  This is in order to
 * interoperate better with vxWorks ftp filenames which may sometimes
 * have a trailing colon instead of slash.
 */

char *sal_getcwd(char *buf, size_t size)
{
    strcpy(buf, "/");
    return buf;
}

int sal_ls(char *f, char *flags)
{
    sal_printf("no filesystem\n");
    return 0;
}

int sal_cd(char *f)
{
    return 0;
}

FILE *
sal_fopen(char *file, char *mode)
/*
 * Function: 	sal_fopen
 * Purpose:	"fopen" a file.
 * Parameters:	name - name of file to open
 *		mode - file mode.
 * Returns:	NULL or FILE * pointer.
 */
{
    return NULL;
}

int
sal_fclose(FILE *fp)
/*
 * Function: 	sal_fclose
 * Purpose:	Close a file opened with sal_fopen
 * Parameters:	fp - FILE pointer.
 * Returns:	non-zero on error
 */
{
    return 0;
}


int
sal_fread(void *buf, int size, int num, FILE *fp)
/*
 * Function: 	sal_fread
 * Purpose:	read() a file
 * Parameters:	buf - buffer
 *		size - size of an object
 *		num - number of objects
 *		fp - FILE * pointer
 * Returns:	number of bytes read
 */
{
    return 0;
}

int
sal_feof(FILE *fp)
/*
 * Function: 	sal_feof
 * Purpose:	Return TRUE if EOF of a file is reached
 * Parameters:	FILE * pointer
 * Returns:	TRUE or FALSE
 */
{
    return 0;
}

int
sal_ferror(FILE *fp)
/*
 * Function: 	sal_ferror
 * Purpose:	Return TRUE if an error condition for a file pointer is set
 * Parameters:	FILE * pointer
 * Returns:	TRUE or FALSE
 */
{
    return 0;
}

int
sal_fsize(FILE *fp)
/*
 * Function: 	sal_fsize
 * Purpose:	Return the size of a file if possible
 * Parameters:	FILE * pointer.
 * Returns:	File size or -1 in case of failure
 */
{
    return 0;
}

int
sal_remove(char *f)
{
    return 0;
}

int
sal_rename(char *f_old, char *f_new)
{
    return 0;
}

int
sal_mkdir(char *path)
{
    return 0;
}

int
sal_rmdir(char *path)
{
    return 0;
}

SAL_DIR *
sal_opendir(char *dirName)
{
    return NULL;
}

int
sal_closedir(SAL_DIR *dirp)
{
    return 0;
}

struct sal_dirent *
sal_readdir(SAL_DIR *dirp)
{
    return NULL;
}

void
sal_rewinddir(SAL_DIR *dirp)
{
}










