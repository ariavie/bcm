/*
 * Stubs for NVRAM functions for platforms without flash
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
 *
 * $Id: nvram_vxworks.c,v 1.2 Broadcom SDK $
 */

#include <typedefs.h>
#include <bcmendian.h>
#include <bcmnvram.h>
#include <bcmutils.h>
#include <siutils.h>
#include <mipsinc.h>
#include <flashutl.h>
#include <hndsoc.h>
#include <flashDrvLib.h>

#include <osl.h>
#include <flashDrvLib.h>

#include <vxWorks.h>
#include <vxLib.h>
#include <taskLib.h>

#include "config.h"

extern int _nvram_read(void *buf);
extern char *_nvram_get(const char *name);
extern int _nvram_set(const char *name, const char *value);
extern int _nvram_unset(const char *name);
extern int _nvram_getall(char *buf, int count);
extern int _nvram_commit(struct nvram_header *header);
extern int _nvram_init(void *sih);
extern void _nvram_exit(void);

static SEM_ID nvram_lock = 0;
#define NVRAM_LOCK()	do { if (nvram_lock) semTake(nvram_lock, WAIT_FOREVER); } while (0)
#define NVRAM_UNLOCK()	do { if (nvram_lock) semGive(nvram_lock); } while (0)

static struct nvram_header *nvram_header = NULL;

extern unsigned char embedded_nvram[];

static struct nvram_header *
BCMINITFN(find_nvram)(bool embonly, bool *isemb)
{
    struct nvram_header *nvh;
    uint32 off, lim;
    
    if (!embonly) {
        *isemb = FALSE;
        lim = SI_FLASH2_SZ;
        off = FLASH_MIN;
        while (off <= lim) {
            nvh = (struct nvram_header *)KSEG1ADDR(SI_FLASH2 + off);
            if (nvh->magic == NVRAM_MAGIC)
                /* if (nvram_calc_crc(nvh) == (uint8) nvh->crc_ver_init) */{
                nvStartOffset(off);
                return (nvh);
            }
            off <<= 1;
        };
    }
    
    /* Now check embedded nvram */
    *isemb = TRUE;
    nvh = (struct nvram_header *)KSEG1ADDR(SI_FLASH2 + (4 * 1024));
    if (nvh->magic == NVRAM_MAGIC) {
        nvStartOffset((4 * 1024));
        return (nvh);
    }
    nvh = (struct nvram_header *)KSEG1ADDR(SI_FLASH2 + 1024);
    if (nvh->magic == NVRAM_MAGIC) {
        nvStartOffset(1024);
        return (nvh);
    }
    printf("find_nvram: no nvram found\n");
    return (NULL);
}

int
nvram_init(void *si)
{
    bool isemb;
    int ret;
    si_t *sih;
    static int nvram_status = -1;
    
    /* Check for previous 'restore defaults' condition */
    if (nvram_status == 1)
        return 1;
    
    sih = (si_t *)si;
    
    /* Initialize private semaphore */
    if ((nvram_lock = semBCreate(SEM_Q_FIFO, SEM_FULL)) == NULL) {
        printf("nvram_init: semBCreate failed\n");
        return ERROR;
    }
    
    /* Find NVRAM */
    nvram_header = find_nvram(FALSE, &isemb);
    if (!nvram_header) {
#if defined(BUILD_VXBOOT)
        nvram_header = (struct nvram_header *) NV_RAM_ADRS;
#else
        return ERROR;
#endif
    }
    ret = _nvram_init(si);
    if (ret == 0) {
        /* Restore defaults if embedded NVRAM used */
        if (nvram_header && isemb) {
            ret = 1;
        }
    }
    nvram_status = ret;
    return ret;
}

int
nvram_append(void *sb, char *vars, uint varsz)
{
    return 0;
}

void
nvram_exit(void *sih)
{
    if (nvram_header == NULL) {
        return;
    }
    
    _nvram_exit();
}

char *
nvram_get(const char *name)
{
    char *value;

    if (nvram_header == NULL) {
        return (char *)0;
    }

    NVRAM_LOCK();
    value = _nvram_get(name);
    NVRAM_UNLOCK();
    
    return value;
}

int 
nvram_getall(char* buf, int count)
{
    int ret;
    
    if (nvram_header == NULL) {
        buf[0] = 0;
        return 0;
    }
    
    NVRAM_LOCK();
    ret = _nvram_getall(buf, count);
    NVRAM_UNLOCK();
    
    return ret;
}

int
nvram_set(const char *name, const char *value)
{
    int ret;
    
    NVRAM_LOCK();
    ret = _nvram_set(name, value);
    NVRAM_UNLOCK();
    
    return ret;
}

int
nvram_unset(const char *name)
{
    int ret;
    
    NVRAM_LOCK();
    ret = _nvram_unset(name);
    NVRAM_UNLOCK();
    
    return ret;
}

int
nvram_commit(void)
{
    struct nvram_header *header;
    int ret;
    uint32 *src, *dst;
    uint i;
    
    if (!(header = (struct nvram_header *) MALLOC(NULL, NVRAM_SPACE))) {
        printf("nvram_commit: out of memory\n");
        return -12;
    }
    
    NVRAM_LOCK();
    ret = _nvram_commit(header);
    
    if (ret)
        goto done;
    
    src = (uint32 *) &header[1];
    dst = src;
    
    for (i = sizeof(struct nvram_header); i < header->len && i < NVRAM_SPACE;
         i += 4)
        *dst++ = htol32(*src++);
    
    
    if (sysFlashInit(NULL) == 0) {
        /* set/write invalid MAGIC # (in case writing image fails/is interrupted)
         * write the NVRAM image to flash(with invalid magic)
         * set/write valid MAGIC #
         */
        header->magic = NVRAM_CLEAR_MAGIC;
        nvWriteChars((unsigned char *)&header->magic, sizeof(header->magic));
        
        header->magic = NVRAM_INVALID_MAGIC;
        nvWrite((unsigned short *) header, NVRAM_SPACE);
        
        header->magic = NVRAM_MAGIC;
        nvWriteChars((unsigned char *)&header->magic, sizeof(header->magic));
    }
    
done:
    NVRAM_UNLOCK();
    MFREE(NULL, header, NVRAM_SPACE);
    return ret;
}

int _nvram_read(void *buf)
{
    volatile uint32 *src, *dst;
    uint i;
    
    src = (uint32 *) nvram_header;
    dst = (uint32 *) buf;
    
    printf ("_nvram_read: source address is 0x%x 0x%x\n", 
            (unsigned int) src, (unsigned int) dst);
    
    NVRAM_LOCK();
    for (i=0; i< sizeof(struct nvram_header); i +=4) {
        *dst++ = *src++;
    }
    
    for (;i < nvram_header->len && i < NVRAM_SPACE; i+= 4)
        *dst++ = ltoh32(*src++);
    
    NVRAM_UNLOCK();
    
    return 0;
}

struct nvram_tuple *
_nvram_realloc(struct nvram_tuple *t, const char *name, const char *value)
{
    if (!(t = (struct nvram_tuple*)malloc(sizeof(struct nvram_tuple) +
                                          strlen(name) + 1 +
                                          strlen(value) +1)))
        return NULL;

    /* Copy name */
    t->name = (char *)&t[1];
    strcpy(t->name, name);

    /* Copy value */
    t->value = t->name + strlen(name) + 1;
    strcpy(t->value, value);

    return t;
}

void _nvram_free(struct nvram_tuple *t)
{
    if (t)
        free(t);
}
