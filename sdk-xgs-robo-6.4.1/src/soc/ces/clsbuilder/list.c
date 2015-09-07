
/*******************************************************************

Copyright Redux Communications Ltd.
 
Module Name:

File Name: 
    list.c

File Description:


*******************************************************************/
#define REVISION "$Revision: 1.5 $" /* Clearcase revision number to be used in TRACE & SWERR */

#include "ag_common.h"
#ifdef BCM_CES_SDK
#include "bcm_ces_sal.h"
#endif
#define DECLARE_LIST
#include "utils/List.h"
#undef DECLARE_LIST

AgResult list_init( List *p_list)
{
    AG_ASSERT(p_list);
    p_list->n_size = 0;
    p_list->p_head = NULL;
    p_list->p_tail = NULL;
    return AG_S_OK;
}


AG_U32 list_get_size ( List* p_list)
{
    AG_ASSERT(p_list);
    return p_list->n_size;
}

void list_insert_head ( List *p_list,  ListElement *p_element)
{
    AG_ASSERT(p_element && p_list);

    p_element->p_prev = NULL; 
    p_element->p_next = p_list->p_head;
/*            p_element->p_list  = p_list; */

    if(p_list->n_size)
    { /* the list is not empty */
        p_list->p_head->p_prev = p_element;
    }
    else
    { /* the list was empty before this insertion */
        AG_ASSERT(!p_list->p_tail && !p_list->p_head);
        p_list->p_tail = p_element;
    }

    p_list->n_size++;
    p_list->p_head = p_element;
}

void list_insert_tail ( List *p_list,  ListElement *p_element)
{
    AG_ASSERT(p_element && p_list);

    p_element->p_prev = p_list->p_tail;
    p_element->p_next = NULL; 
/*            p_element->p_list = p_list; */

    if(p_list->n_size)
    {
        p_list->p_tail->p_next = p_element;
    }
    else
    { /* the list was empty before this insertion */
        AG_ASSERT(!p_list->p_tail && !p_list->p_head);
        p_list->p_head = p_element;
    }

    p_list->n_size++;
    p_list->p_tail = p_element;
}

void list_insert_after ( List *p_list,
                             ListElement *p_element,
                             ListElement *p_new_element)
{
    AG_ASSERT(p_list && p_element && p_new_element );
/*        AG_ASSERT(p_element->p_list == p_list);*/
    /*update the new element members */
    p_new_element->p_prev = p_element;
    p_new_element->p_next = p_element->p_next; 
/*            p_new_element->p_list = p_list;*/

    /* update the p_element */
    p_element->p_next = p_new_element;

    if(p_new_element->p_next)
    {/* there is an element following the new element */
     /* need to set its back pointer */
        p_new_element->p_next->p_prev = p_new_element;
    }
    else
    { /* p_element was the last element in the list */
      /* now p_new_element is the last element */
        AG_ASSERT(p_element == p_list->p_tail);
        p_list->p_tail = p_new_element;
    }
    
    p_list->n_size++;
}

ListElement* list_remove_head ( List* p_list)
{
    ListElement* p_el;
    
    AG_ASSERT(p_list);

    if(p_list->n_size)
    { /* list is not empty */
        AG_ASSERT(p_list->p_head && p_list->p_tail);

        /* keep the removed head */
        p_el = p_list->p_head;

        /* update the list members */
        /*///////////////////////// */
        p_list->p_head = p_el->p_next; /* NULL if p_el was the only list element */
        p_list->n_size--;         /* Zero if p_el was the only list element */
        if(p_list->n_size)
        { /* there are still elements in the list */
            AG_ASSERT(p_list->p_head);
            p_list->p_head->p_prev = NULL;
        }
        else
        { /* p_el was the only list element */
            AG_ASSERT(p_list->p_tail == p_el && !p_list->p_head);
            p_list->p_tail = NULL; 
        }
        

        /* disconnect the removed element from the list */
/*        p_el->p_list = NULL;*/
        p_el->p_next = NULL;
        p_el->p_prev = NULL;
		
		return p_el;
    }
    return NULL;

}

ListElement* list_remove_tail ( List* p_list)
{
    ListElement* p_el;

    AG_ASSERT(p_list && p_list->n_size);
    if(p_list->n_size)
    { /* list is not empty */
        AG_ASSERT(p_list->p_head && p_list->p_tail);

        /* keep the removed tail */
        p_el = p_list->p_tail;

        /* update the list members */
        /*///////////////////////// */
        p_list->p_tail = p_el->p_prev; /* NULL if p_el was the only list element */
        p_list->n_size--;         /* Zero if p_el was the only list element */
        if(p_list->n_size)
        { /* there are still elements in the list */
            AG_ASSERT(p_list->p_tail);
            p_list->p_tail->p_next = NULL;
        }
        else
        { /* p_el was the only list element */
            AG_ASSERT(p_list->p_head == p_el && !p_list->p_tail);
            p_list->p_head = NULL; 
        }
        

        /* disconnect the removed element from the list */
/*        p_el->p_list = NULL;*/
        p_el->p_next = NULL;
        p_el->p_prev = NULL;
	
        return p_el;
    }
    return NULL;

}

void list_unlink ( List* p_list,  ListElement *p_element)
{
    AG_ASSERT(p_element && p_list && p_list->n_size);
/*        AG_ASSERT(p_element->p_list == p_list);*/

    if(p_list->n_size)
    { /* list is not empty */
		if(p_element->p_next)
		{ /* not the last element */
			/* backward link the next element to the previous one */
			p_element->p_next->p_prev = p_element->p_prev;
		}
		else
		{ /* this is the last element */
			AG_ASSERT(p_list->p_tail == p_element);
			/* Set tail to the previous element */
			/* Tail is NULL if this is the only element in the list */
			p_list->p_tail = p_element->p_prev; 
		}
        
		if(p_element->p_prev)
		{ /* not the first element */
			/* forward link the previous element to the next one. */
			p_element->p_prev->p_next = p_element->p_next;
		}
		else
		{/* the first element */
			AG_ASSERT(p_list->p_head == p_element);
			/* set head to the next element */
			/* NULL if this is the only element in the list */
			p_list->p_head = p_element->p_next;
		}

		p_list->n_size--;

		/* sanity verification */
		AG_ASSERT((p_list->n_size && p_list->p_head && p_list->p_tail) ||
			   (!p_list->n_size && !p_list->p_head && !p_list->p_tail)); 

    
		/* disconnect the removed element from the list */
	/*        p_element->p_list = NULL;*/
		p_element->p_next = NULL;
		p_element->p_prev = NULL;
	}
}


ListElement* list_head( List* p_list)
{
    AG_ASSERT(p_list);
    return p_list->p_head;
}

ListElement* list_tail( List* p_list)
{
    AG_ASSERT(p_list);
    return p_list->p_tail;
}

ListElement* list_next( List *p_list, ListElement *p_element)
{
    AG_ASSERT(p_list && p_element);
/*        AG_ASSERT(p_element->p_list == p_list);*/
    return p_element->p_next;
}

ListElement* list_prev( List *p_list,  ListElement *p_element)
{
    AG_ASSERT(p_list && p_element);
/*        AG_ASSERT(p_element->p_list == p_list);*/
    return p_element->p_prev;
}

AG_BOOL list_is_empty( List* p_list)
{
    AG_ASSERT(p_list);
    return(!p_list->n_size);
}

AgResult list_test( List* p_list)
{
    /* not implemented yet */
    AG_ASSERT(p_list);
    return AG_S_OK;
}

