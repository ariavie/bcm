/*******************************************************************

Copyright Redux Communications Ltd.
 
Module Name:

File Name: 
    cls_stack.c - created 15/11/2000

File Description:
	The stack procedures  order to provide the dynamic mode of 
	work  the classification program. It is used for both 8 and 16 
	bit mode of work.
	The function stack_state were not implemenented, 
	instead these conditions are directed tested by verifying
	the top element of the stack.

*******************************************************************/
#define REVISION "$Revision: 1.3 $" /* Visual SourceSafe automatic revision number to be used in TRACE & SWERR */

#include "pub/cls_stack.h"

/******************************************************************
Name: 
    cls_stack_init
Description:
	Initializes the stack, set pointers to NULL.
	Set b_init to AG_TRUE.
Parameters:
	 pointer to the stack;
	 size of the stack
Returns void:
    
    

*******************************************************************/
void cls_stack_init( ClsbStack *p_old_stack, ClsbStack *p_new_stack)
{
    /* no need to initialize the stack itself. Nobody goes beyond the n_top. */    
	p_old_stack->n_top = STACK_EMPTY;
	p_new_stack->n_top = STACK_EMPTY;
}

/******************************************************************
Name: 
    cls_stack_push
Description:
	Push an element into the stack
Parameters:
	 pointer to the stack;
	 pointer to the element to be inserted
Returns AgResult:
    AG_S_OK
	AG_E_FAIL
    AG_E_CLS_FULL
	AG_E_NOT_INIT
*******************************************************************/
extern AgResult cls_stack_push( ClsbStack *p_stack,  ClsbStackElement *p_element)
{
 
	AG_ASSERT(p_stack && p_element);

	if(p_stack->n_top < p_stack->n_size-1)
	{
		p_stack->a_stack[++p_stack->n_top]=(*p_element);

		return AG_S_OK;
	}

	return AG_E_CLS_FULL;

}

extern AgResult cls_stack_push_8(AG_U32 n_ent_num, 
								 AG_U16 n_rec,
								 AG_U32 n_ent_val, 
								 AG_U8 n_table,
								 ClsbStack *p_stack)
{
    ClsbStackElement *p_element;

	if(p_stack->n_top < p_stack->n_size-1)
	{

        p_stack->n_top++;
		p_element = &p_stack->a_stack[p_stack->n_top];
   		p_element->n_entry_num = n_ent_num;
		p_element->n_record = n_rec;
		p_element->n_entry_value = n_ent_val;
		p_element->n_table = n_table;

        #ifdef CLS_TEST_FULL_TRACE
	    fprintf (full_trace, "\t\t\tReturned Parameter:\n\t\t\t\t\tStack\n");
		    
	    fprintf (full_trace, "\t\t\t\t\tn_size %d\n\t\t\t\t\tn_top %d\n", 
		     p_stack->n_size, p_stack->n_top);
	    fprintf (full_trace, "\t\t\t\t\tn_entry_num %d %d\n\t\t\t\t\tn_entry_value %08x\n", 
		    n_ent_num, n_ent_val);
	    fprintf (full_trace, "\t\t\t\t\tn_record %d %d\n\t\t\t\t\tn_table %08x\n", 
		    n_rec, n_table);

	    fprintf (full_trace, "\t\t\tEND: cls_stack_push_8 (%08x)\n", AG_S_OK);
        #endif /* CLS_TEST_FULL_TRACE */

		return AG_S_OK;
	}

    #ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "\t\t\tEND: cls_stack_push_8 (%08x)\n", AG_E_CLS_FULL);
    #endif /* CLS_TEST_FULL_TRACE */

    return AG_E_CLS_FULL;

}

/******************************************************************
Name: 
    cls_stack_pop
Description:
	Pop an element from the stack
Parameters:
	 pointer to the stack;
	 pointer to the popped element;
Returns AgResult:
    AG_S_OK
	AG_E_FAIL
    AG_E_CLS_EMPTY
	AG_E_NOT_INIT
*******************************************************************/
AgResult cls_stack_pop( ClsbStack *p_stack,  ClsbStackElement *p_element)
{
	AG_ASSERT(p_stack && p_element);

	if(p_stack->n_top!=STACK_EMPTY)
	{
		(*p_element)=p_stack->a_stack[p_stack->n_top--];/*Pop the element */

			return AG_S_OK;
	}

	return AG_E_CLS_EMPTY;

}

ClsbStackElement* cls_stack_pop_8( ClsbStack *p_stack)
{
	AG_ASSERT(p_stack);
	if(p_stack->n_top!=STACK_EMPTY)
	{
		ClsbStackElement* p_el = &p_stack->a_stack[p_stack->n_top];
        p_stack->n_top--;
        return(p_el);
	}

	return NULL;

}
/******************************************************************
Name: 
    cls_stack_state
Description:
	Returns if the stack is full or empty.
Parameters:
	 pointer to the stack;

Returns AgResult:
     AG_S_CLS_EMPTY
	 AG_S_CLS_FULL
	 AG_S_OK;
*******************************************************************/
AgResult cls_stack_state( ClsbStack *p_stack)
{
	if (p_stack->n_top==STACK_EMPTY) return AG_S_CLS_EMPTY;
	if (p_stack->n_top == p_stack->n_size-1) return AG_S_CLS_FULL;
	return AG_S_OK;
}
