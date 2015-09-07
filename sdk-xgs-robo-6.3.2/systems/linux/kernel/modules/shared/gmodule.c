/*
 * $Id: gmodule.c 1.21 Broadcom SDK $
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
 * Generic Linux Module Framework
 *
 * Hooks up your driver to the kernel 
 */

#include <lkm.h>
#include <gmodule.h>
#include <linux/init.h>

/* Module Vector Table */
static gmodule_t* _gmodule = NULL;


/* Allow DEVFS Support on 2.4 Kernels */
#if defined(LKM_2_4) && defined(CONFIG_DEVFS_FS)
#define GMODULE_CONFIG_DEVFS_FS
#endif


#ifdef GMODULE_CONFIG_DEVFS_FS
devfs_handle_t devfs_handle = NULL;
#endif



static int _dbg_enable = 0;

static int
gvprintk(const char* fmt, va_list args)
    __attribute__ ((format (printf, 1, 0)));

static int
gvprintk(const char* fmt, va_list args)
{
    static char _buf[256];
  
    strcpy(_buf, "");
    sprintf(_buf, "%s (%d): ", _gmodule->name, current->pid); 
    vsprintf(_buf+strlen(_buf), fmt, args);
    printk(_buf);
  
    return 0;
}

int
gprintk(const char* fmt, ...)
{
    int rv;

    va_list args;
    va_start(args, fmt);
    rv = gvprintk(fmt, args);
    va_end(args);
    return rv;
}

int 
gdbg(const char* fmt, ...)
{
    int rv = 0;

    va_list args;
    va_start(args, fmt);
    if(_dbg_enable) {
	rv = gvprintk(fmt, args);
    }
    va_end(args);
    return rv;
}


/*
 * Proc FS Utilities
 */

int 
gmodule_vpprintf(char** page_ptr, const char* fmt, va_list args)
{
    *page_ptr += vsprintf(*page_ptr, fmt, args);
    return 0;
}

int
gmodule_pprintf(char** page_ptr, const char* fmt, ...)
{
    int rv;

    va_list args;
    va_start(args, fmt);
    rv = gmodule_vpprintf(page_ptr, fmt, args); 
    va_end(args);
    return rv;
}

static char* _proc_buf = NULL;

int 
pprintf(const char* fmt, ...)
{  
    int rv;

    va_list args;
    va_start(args, fmt);
    rv = gmodule_vpprintf(&_proc_buf, fmt, args); 
    va_end(args);
    return rv;
}

#define PSTART(b) _proc_buf = b
#define PPRINT proc_print
#define PEND(b) (_proc_buf-b)

static int
_gmodule_pprint(char* buf)
{
    PSTART(buf);
    _gmodule->pprint();
    return PEND(buf);
}

static int 
_gmodule_read_proc(char *page, char **start, off_t off,
		   int count, int *eof, void *data)
{
    *eof = 1;
    return _gmodule_pprint(page);
}

static int 
_gmodule_write_proc(struct file *file, const char *buffer,
		    unsigned long count, void *data)
{
    /* Workaround to toggle debugging */
    if(count > 2) {
	if(buffer[0] == 'd') {
	    _dbg_enable = buffer[1] - '0';
	    GDBG("Debugging Enabled");
	}
    }
    return 0;
}

static int
_gmodule_create_proc(void)
{
    struct proc_dir_entry* ent;
    struct file_operations fops;
    memset(&fops, 0, sizeof(struct file_operations));
    fops.read = _gmodule_read_proc;
    fops.write = _gmodule_write_proc;
    if((ent = proc_create(_gmodule->name, S_IRUGO | S_IWUGO, NULL, &fops)) != NULL) {
        return 0;
    }

    return -1;
}

static void 
_gmodule_remove_proc(void)
{
    remove_proc_entry(_gmodule->name, NULL);
}

static int
_gmodule_open(struct inode *inode, struct file *filp)
{
    if(_gmodule->open) {
	_gmodule->open();
    }
    return 0;
}

static int 
_gmodule_release(struct inode *inode, struct file *filp)
{
    if(_gmodule->close) {
	_gmodule->close();
    }
    return 0;
}

#ifdef HAVE_UNLOCKED_IOCTL
static long
_gmodule_unlocked_ioctl(struct file *filp,
                        unsigned int cmd, unsigned long arg)
{
    if(_gmodule->ioctl) {
	return _gmodule->ioctl(cmd, arg);
    } else {
	return -1;
    }
}
#else
static int 
_gmodule_ioctl(struct inode *inode, struct file *filp,
	       unsigned int cmd, unsigned long arg)
{
    if(_gmodule->ioctl) {
	return _gmodule->ioctl(cmd, arg);
    } else {
	return -1;
    }
}
#endif

#ifdef HAVE_COMPAT_IOCTL
static long
_gmodule_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    if(_gmodule->ioctl) {
	return _gmodule->ioctl(cmd, arg);
    } else {
	return -1;
    }
}
#endif


static int
_gmodule_mmap(struct file *filp, struct vm_area_struct *vma)
{
    if (_gmodule->mmap) {
        return _gmodule->mmap(filp, vma);
    }
#ifdef BCM_PLX9656_LOCAL_BUS
	vma->vm_flags |= VM_RESERVED | VM_IO;
	pgprot_val(vma->vm_page_prot) |= _PAGE_NO_CACHE | _PAGE_GUARDED;

	if (io_remap_pfn_range(	vma,
				vma->vm_start,
				vma->vm_pgoff,
				vma->vm_end - vma->vm_start,
				vma->vm_page_prot)) {
                return (-EAGAIN);
	}
	return (0);
#else/* BCM_PLX9656_LOCAL_BUS */
    return -EPERM;
#endif/* BCM_PLX9656_LOCAL_BUS */
}

/* FILE OPERATIONS */

struct file_operations _gmodule_fops = {
#ifdef HAVE_UNLOCKED_IOCTL
    unlocked_ioctl: _gmodule_unlocked_ioctl,
#else
    ioctl:      _gmodule_ioctl,
#endif
    open:       _gmodule_open,
    release:    _gmodule_release,
    mmap:       _gmodule_mmap,
#ifdef HAVE_COMPAT_IOCTL
    compat_ioctl: _gmodule_compat_ioctl,
#endif
};


void __exit
cleanup_module(void)
{
    if(!_gmodule) return;
  
    /* Specific Cleanup */
    if(_gmodule->cleanup) {
	_gmodule->cleanup();
    }
  
    /* Remove any proc entries */
    if(_gmodule->pprint) {
	_gmodule_remove_proc();
    }
  
    /* Finally, remove ourselves from the universe */
#ifdef GMODULE_CONFIG_DEVFS_FS
    if(devfs_handle) devfs_unregister(devfs_handle);
#else
    unregister_chrdev(_gmodule->major, _gmodule->name);
#endif
}

int __init
init_module(void)
{  
    int rc;

    /* Get our definition */
    _gmodule = gmodule_get();
    if(!_gmodule) return -ENODEV;


    /* Register ourselves */
#ifdef GMODULE_CONFIG_DEVFS_FS
    devfs_handle = devfs_register(NULL, 
				  _gmodule->name, 
				  DEVFS_FL_NONE, 
				  _gmodule->major,
				  _gmodule->minor, 
				  S_IFCHR | S_IRUGO | S_IWUGO,
				  &_gmodule_fops, 
				  NULL);
    if(!devfs_handle) {
	printk(KERN_WARNING "%s: can't register device with devfs", 
	       _gmodule->name);
    }
    rc = 0;
#else
    rc = register_chrdev(_gmodule->major, 
			 _gmodule->name, 
			 &_gmodule_fops);
    if (rc < 0) {
	printk(KERN_WARNING "%s: can't get major %d",
	       _gmodule->name, _gmodule->major);
	return rc;
    }

    if(_gmodule->major == 0) {
	_gmodule->major = rc;
    }
#endif

    /* Specific module Initialization */
    if(_gmodule->init) {
	int rc;
	if((rc = _gmodule->init()) < 0) {
#ifdef GMODULE_CONFIG_DEVFS_FS
            if(devfs_handle) devfs_unregister(devfs_handle);
#else
            unregister_chrdev(_gmodule->major, _gmodule->name);
#endif
	    return rc;
	}
    }

    /* Add a /proc entry, if valid */
    if(_gmodule->pprint) {    
	_gmodule_create_proc();
    }

    return 0; /* succeed */
}
