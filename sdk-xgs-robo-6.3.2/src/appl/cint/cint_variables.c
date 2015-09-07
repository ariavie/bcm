/*
 * $Id: cint_variables.c 1.14 Broadcom SDK $
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
 * File:        cint_variables.c
 * Purpose:     CINT variable functions
 *
 */

#include "cint_config.h"
#include "cint_variables.h"
#include "cint_porting.h"
#include "cint_debug.h"
#include "cint_interpreter.h"

/*******************************************************************************
 * 
 * Global Static Data Variables
 *
 ******************************************************************************/

typedef struct lscope_list_s {
    struct lscope_list_s* next;     
    cint_variable_t* vlist; 
    cint_variable_t* autolist; 
} lscope_list_t; 

typedef struct vscope_list_s {
    struct vscope_list_s* next; 
    lscope_list_t* lscopes; 
} vscope_list_t; 

/*
 * Global variable scope. 
 *
 * This scope is available at all times. 
 */
static vscope_list_t _global_scope = { NULL, NULL }; 


/*
 * This is the list of all variable scopes. 
 */
static vscope_list_t* _scopes = NULL; 


int 
cint_variable_init(void)
{
    /* Destroy any previous scopes */
    while(cint_variable_scope_pop("") != CINT_E_NOT_FOUND); 

    /* Initialize the global scope */
    _scopes = &_global_scope; 
    cint_variable_lscope_push("global"); 
    return 0; 
}

static int
__destroy_vlist(cint_variable_t* vlist, int force)
{
    cint_variable_t* v; 
    for(v = vlist; v;) {
        cint_variable_t* next = v->next;
        if (force) {
            v->flags &= ~CINT_VARIABLE_F_NODESTROY;
        }
        cint_variable_free(v); 
        v = next; 
    }

    return CINT_E_NONE; 
}


int 
cint_variable_scope_push(const char* msg)
{
    vscope_list_t* vscope = CINT_MALLOC(sizeof(*vscope)); 

    if(vscope == NULL) {
        return CINT_E_MEMORY; 
    }

    vscope->lscopes = NULL; 
    vscope->next = _scopes; 
    _scopes = vscope; 
    
    CINT_DTRACE(("scope_push(%s)=%p", msg, (void *)vscope)); 
    return cint_variable_lscope_push(msg); 
}

int
cint_variable_scope_pop(const char* msg)
{
    vscope_list_t* vscope = _scopes; 
    
    if(vscope) {
        while(cint_variable_lscope_pop(msg) != CINT_E_NOT_FOUND); 
        _scopes = vscope->next;     
        CINT_DTRACE(("scope_pop(%s)=%p", msg, (void *)vscope));
        if (vscope != &_global_scope) {
            CINT_FREE(vscope);
        }
        return CINT_E_NONE; 
    }   
    else {
        return CINT_E_NOT_FOUND; 
    }   
}


int
cint_variable_lscope_push(const char* msg)
{
    lscope_list_t* lscope = CINT_MALLOC(sizeof(*lscope)); 

    if(lscope == NULL) {
        return CINT_E_MEMORY; 
    }

    lscope->vlist = NULL; 
    lscope->autolist = NULL; 

    if(_scopes) {
        lscope->next = _scopes->lscopes; 
    }
    else {
        lscope->next = NULL; 
    }
    _scopes->lscopes = lscope; 
    
    CINT_DTRACE(("lscope_push(%s)", msg)); 
    return CINT_E_NONE; 
}
    
int
cint_variable_lscope_pop(const char* msg)
{
    lscope_list_t* lscope = _scopes->lscopes; 
    if(lscope) {
        _scopes->lscopes = lscope->next; 
        __destroy_vlist(lscope->vlist, 0); 
        __destroy_vlist(lscope->autolist, 0); 
        CINT_FREE(lscope); 
        CINT_DTRACE(("lscope_pop(%s)", msg)); 
        return CINT_E_NONE;     
    }   
    else {
        return CINT_E_NOT_FOUND; 
    }
}        

    
cint_variable_t*
cint_variable_alloc(void)
{
    cint_variable_t* v = (cint_variable_t*)CINT_MALLOC(sizeof(cint_variable_t));
    if (v) {
        CINT_MEMSET(v, 0, sizeof(*v));
    }
    return v; 
}

int
cint_variable_free(cint_variable_t* v)
{
    if(v) {
        
        if(v->flags & CINT_VARIABLE_F_NODESTROY) {
            return 0; 
        }
        /*CINT_PRINTF("free variable '%s' flags=0x%x name=%p data=%p\n", 
          v->name, v->flags, v->name, v->data);*/
        if(v->name && !(v->flags & CINT_VARIABLE_F_SNAME)) {
            CINT_FREE(v->name); 
        }
        if(v->data && !(v->flags & CINT_VARIABLE_F_SDATA)) {
            if(v->flags & CINT_VARIABLE_F_CSTRING) {
                char **sptr = (char **)v->data;
                if (sptr) {
                    CINT_FREE(*sptr);
                }
            }
            CINT_FREE(v->data); 
        }
        CINT_FREE(v); 
    }   
    return 0; 
}

/* Find a variable on a variable list */
static cint_variable_t*
_variable_find_list(cint_variable_t* vlist, const char* name)
{
    cint_variable_t* v; 
    for(v = vlist; v; v = v->next) {
        if(!CINT_STRCMP(v->name, name)) {
            return v; 
        }
    }
    return NULL; 
}

/* Find a variable within a scope on manual and auto lists */
static cint_variable_t*
_variable_find_scope(lscope_list_t* lscope, const char* name)
{
    cint_variable_t* rv = NULL;

    rv = _variable_find_list(lscope->vlist, name); 

    if (rv == NULL) {
        rv = _variable_find_list(lscope->autolist, name);
    }

    return rv;
}

/* Find a variable within a set of scopes on manual and auto lists */
static cint_variable_t*
_variable_find_scopes(lscope_list_t* lscope, const char* name)
{
    lscope_list_t* s; 
    cint_variable_t* rv = NULL;

    for (s = lscope; s; s = s->next) {
        if ((rv = _variable_find_scope(s, name)) != NULL) {
            break; 
        }
    }       

    return rv;
}

cint_variable_t* 
cint_variable_find(const char* name, int current_scope_only)
{
    cint_variable_t* rv = NULL; 
    
    if(_scopes && _scopes->lscopes) {
        if(current_scope_only) {
            rv = _variable_find_scope(_scopes->lscopes, name); 
            return rv; 
        } else {
            rv = _variable_find_scopes(_scopes->lscopes, name);
        }   
    }

    if(rv == NULL) {
        rv = _variable_find_scopes(_global_scope.lscopes, name); 
    }

    return rv; 
}

int
cint_variable_add(cint_variable_t* v)
{
    int is_global = 0;
    v->prev = NULL; 

    if(v->flags & CINT_VARIABLE_F_AUTO) {
        v->next = _scopes->lscopes->autolist; 
        _scopes->lscopes->autolist = v; 
    }   
    else {
        v->next = _scopes->lscopes->vlist; 
        _scopes->lscopes->vlist = v;
        if (_scopes == &_global_scope) {
            is_global = 1;
        }
    }   
    
    if(v->next) {
        v->next->prev = v; 
    }

    if (is_global) {
        cint_interpreter_event(cintEventGlobalVariableAdded); 
    }
    return CINT_E_NONE; 
}

int
cint_variable_rename(cint_variable_t* v, const char *name)
{
    if (name) {
        if (v->name) {
            CINT_FREE(v->name);
        }
        v->name = CINT_STRDUP(name);
    }

    return CINT_E_NONE; 
}

int
cint_variable_list_remove(cint_variable_t* v)
{
    if(v == NULL) {
        return 0; 
    }   
        
    if(v->prev) {
        v->prev->next = v->next; 
    }
    else if(v == _scopes->lscopes->autolist) {
        _scopes->lscopes->autolist = v->next; 
    }       
    else if(v == _scopes->lscopes->vlist) {
        _scopes->lscopes->vlist = v->next; 
    }       
    else {
        cint_internal_error(__FILE__, __LINE__, "CANNOT REMOVE FROM LIST (v=%p, v->next=%p, v->prev=%p)", 
                            (void *)v, (void *)v->next, (void *)v->prev); 
    }       
    return 0; 
}


int
cint_variable_auto_clear(void)
{
    /* Clear all autovars from the current scope */
    __destroy_vlist(_scopes->lscopes->autolist, 0); 
    _scopes->lscopes->autolist = NULL; 
    return 0;
}

int
cint_variable_auto_save(cint_variable_t* v)
{
    /*
     * Move this autovar from the auto list to variable list
     */
    if(v && (v->flags & CINT_VARIABLE_F_AUTO)) {
        
        /* Remove it from it's auto list */
        cint_variable_list_remove(v); 

        v->flags &= ~CINT_VARIABLE_F_AUTO; 
        cint_variable_add(v); 
    }   
    return 0; 
}       
        
 


int 
cint_variable_delete(const char* name) 
{
    cint_variable_t* v = cint_variable_find(name, 1); 
    if(v) {
        cint_variable_list_remove(v);
        cint_variable_free(v); 
    }
    return 0; 
}


int
cint_variable_show(const char* var)
{
    return CINT_E_NONE; 
}       


int
cint_variable_dump(void)
{
    int l, v; 
    cint_variable_t* var; 

    if(_scopes) {
        vscope_list_t* vscopes; 
        for(v = 0, vscopes = _scopes; vscopes; vscopes = vscopes->next, v++) {
            lscope_list_t* lscopes; 
            for(l = 0, lscopes = vscopes->lscopes; lscopes; lscopes = lscopes->next, l++) {
                for(var = lscopes->vlist; var; var = var->next) {
                    if(!CINT_STRCMP(var->name, "NULL")) continue; 
                    CINT_PRINTF("V %d L %d: %s\n", v, l, var->name); 
                }       
                for(var = lscopes->autolist; var; var = var->next) {
                    CINT_PRINTF("A %d L %d: %s\n", v, l, var->name); 
                }       
            }
        }
    }
    
    return CINT_E_NONE; 
}       



int
cint_variable_clear(void)
{
    vscope_list_t* vscopes; 
   
    for (vscopes = _scopes; vscopes; vscopes = vscopes->next) {
        lscope_list_t* lscopes; 
        for (lscopes = vscopes->lscopes; lscopes; lscopes = lscopes->next) {
            __destroy_vlist(lscopes->vlist, 1); 
            __destroy_vlist(lscopes->autolist, 1);
            lscopes->vlist = NULL;
            lscopes->autolist = NULL;
        }
    }
    _scopes = NULL;
    
    return CINT_E_NONE; 
}
