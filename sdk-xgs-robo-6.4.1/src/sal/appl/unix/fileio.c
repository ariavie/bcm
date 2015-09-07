/*
 * $Id: fileio.c,v 1.14 Broadcom SDK $
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

#include <sys/types.h>
#include <sys/stat.h>		/* mkdir */
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <assert.h>

#include <sal/appl/io.h>

#if defined(__STRICT_ANSI__)
#define NO_ENV_SUPPORT
#endif

int
sal_homedir_set(char *dir)
{
#ifndef NO_ENV_SUPPORT
    char *buf = NULL;
    if (dir != NULL) {
        if (dir[0] != '/') {
            return -1;
        }
        
        if ((buf = malloc(5 + strlen(dir) + 1)) == NULL) {
            return -1;
        }
        sprintf(buf, "HOME=%s", dir);
        if (putenv(buf)) {
            free(buf);
            return -1;
        }
        
    }
#endif    
    return 0; 
    
}

char *
sal_homedir_get(char *buf, size_t size)
{
#ifndef NO_ENV_SUPPORT
    char	*s;

    if ((s = getenv("HOME")) != NULL) {
	strncpy(buf, s, size);
	buf[size - 2] = 0;
    } else {
	strcpy(buf, "/");
    }

    if (buf[strlen(buf) - 1] != '/') {
	strcat(buf, "/");
    }
#endif
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
    if (getcwd(buf, size - 1) == NULL)
	return NULL;

    if (buf[strlen(buf) - 1] != '/')
	strcat(buf, "/");

    return buf;
}

int sal_ls(char *f, char *flags)
{
    char cmd[256];
    
    sprintf(cmd, "ls %s %s\n", flags ? flags : "", f);
    return(system(cmd));
}

int sal_cd(char *f)
{
#ifdef NO_ENV_SUPPORT
    return chdir("/");
#endif
    int rv;
    char *s = NULL;
    if (f == NULL) {
        f = getenv("HOME");
        if (f == NULL) {
            f = "/";
        } else {
            s = strdup(f);
            f = s;
        }
    }
    rv = chdir(f);
    if (s) {
        free(s);
    }
    return rv;
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
    return fopen(file, mode);
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
    return fclose(fp);
}

int
sal_fread(void *buf, int size, int num, FILE *fp)
/*
 * Function: 	sal_fread
 * Purpose:	read() a file.
 * Parameters:	name - name of file to open
 *		mode - file mode.
 * Returns:	number of bytes read
 */
{
    return fread(buf, size, num, fp);
}

int
sal_feof(FILE *fp)
/*
 * Function: 	sal_feof
 * Purpose:	Return TRUE if EOF of a file is reached.
 * Parameters:	FILE * pointer.
 * Returns:	TRUE or FALSE
 */
{
    return feof(fp);
}

int
sal_ferror(FILE *fp)
/*
 * Function: 	sal_ferror
 * Purpose:	Return TRUE if an error condition for a file pointer is set.
 * Parameters:	FILE * pointer.
 * Returns:	TRUE or FALSE
 */
{
    return ferror(fp);
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
    int rv;
    
    if (0 != fseek(fp, 0, SEEK_END)) {
        return -1;
    } 
    rv = (int)ftell(fp);
    if (0 != fseek(fp, 0, SEEK_SET)) {
        return -1;
    }
    return rv < 0 ? -1 : rv;
}

int
sal_remove(char *f)
{
    return(unlink(f));
}

int
sal_rename(char *f_old, char *f_new)
{
    return rename(f_old, f_new);
}

int
sal_mkdir(char *path)
{
    return mkdir(path, 0777);
}

int
sal_rmdir(char *path)
{
    return rmdir(path);
}

SAL_DIR *
sal_opendir(char *dirName)
{
    return (SAL_DIR *) opendir(dirName);
}

int
sal_closedir(SAL_DIR *dirp)
{
    return closedir((DIR *) dirp);
}

struct sal_dirent *
sal_readdir(SAL_DIR *dirp)
{
    static struct sal_dirent dir;
    struct dirent *d;

    if ((d = readdir((DIR *) dirp)) == NULL) {
	return NULL;
    }

    strncpy(dir.d_name, d->d_name, sizeof (dir.d_name));
    dir.d_name[sizeof (dir.d_name) - 1] = 0;

    return &dir;
}

void
sal_rewinddir(SAL_DIR *dirp)
{
    rewinddir((DIR *) dirp);
}
