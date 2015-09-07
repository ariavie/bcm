/* $Id: 243c0190112cc3b88929b178fbf9fed42fca97d9 $
 * Copyright 2011 BATM
 */


#include <stddef.h>
#include "pub/nd_api.h"
#include <nd_list.h>

#ifdef __cplusplus
extern "C"
{
#endif


/* */
/* initializes (empties) list */
/* */
void 
ag_nd_list_init(AgNdList *p_head)      
{ 
    p_head->p_next = p_head; 
    p_head->p_prev = p_head; 
}

/* */
/* tests whether a list is empty */
/* */
AG_BOOL 
ag_nd_list_is_empty(AgNdList *p_head)     
{
    return p_head->p_next == p_head;
}

/* */
/* deletes element from list */
/* */
void 
ag_nd_list_del_helper(AgNdList *p_prev, AgNdList *p_next) 
{ 
    p_prev->p_next = p_next; 
    p_next->p_prev = p_prev; 
}

void 
ag_nd_list_del(AgNdList *p_ptr)
{
    ag_nd_list_del_helper(p_ptr->p_prev, p_ptr->p_next);
}


/* */
/* adds element to list */
/* */
void 
ag_nd_list_add_helper(AgNdList *p_prev, AgNdList *p_next, AgNdList *p_item)
{
    p_prev->p_next = p_item;
    p_next->p_prev = p_item;
    p_item->p_next = p_next;
    p_item->p_prev = p_prev;
}

void 
ag_nd_list_add(AgNdList *p_head, AgNdList *p_ptr)
{
    ag_nd_list_add_helper(p_head, p_head->p_next, p_ptr);
}

void 
ag_nd_list_add_tail(AgNdList *p_head, AgNdList *p_ptr)
{
    ag_nd_list_add_helper(p_head->p_prev, p_head, p_ptr);
}


#ifdef __cplusplus
}
#endif



