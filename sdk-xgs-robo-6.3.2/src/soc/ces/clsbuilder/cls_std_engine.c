
/*******************************************************************

Copyright Redux Communications Ltd.
 
Module Name:

File Name: 
    cls_std_engine.c

File Description:
	  This file is the engine module of the classification program.
	  Responsible for adding and deleting paths.
	  Every function, variable and macro exclusive for 8 bits mode
	  of work was sufixed with _8.
	  Every function, variable and macro exclusive for 16 bits mode
	  of work was sufixed with _16.
	  Functions, variables and macros serving both modes of work was
	  not sufixed.

History:
    Date            Name            Comment
    ----            ----            -------
    4/11/2002		Raanan Refua	Addition of alternative pattern matching algorithm
	12/11/2002		Raanan Refua	Improvement of current pattern matching algorithm, using if.

***************************************************************/
#define CLS_STD_ENGINE_C
#define REVISION "$Revision: 1.6 $" /* Clearcase revision number to be used in TRACE & SWERR */

#ifndef BCM_CES_SDK
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

#include "ag_common.h"
#include "pub/cls_engine.h"
#include "classification/clsb_types.h"
#include "utils/memutils.h"


/*Raanan Refua 4/11/2002*/
#define CLS_NAIVE_PATTERN_MATCHING_ALGORITHM 0
#define CLS_TURN_ON_IF_IN_NAIVE 0

#define CLS_ONES_RR_MATCHING_ALGORITHM 1


/********************/
/* Static variables */
/********************/
static const AG_U8 aClsMaskOpc8[NUM_OF_OPCODES_8] =
{ 
    0xFF, 0xFF,0xFF, 0xF0 /*classify high nibble mask*/, 
    0xFF, 0xFF,0xFF, 0xFF 
};

static const AG_U16 aClsMaskTable16[16] = 
{ 
    0x0000 , 0x000F , 0x00F0 , 0x00FF , 0x0F00 , 0x0F0F , 0x0FF0 , 0x0FFF , 
    0xF000 , 0xF00F , 0xF0F0 , 0xF0FF , 0xFF00 , 0xFF0F , 0xFFF0 , 0xFFFF
};

/*#define CLS_TEST_FULL_TRACE 1 */

/******************/
/*Static Functions*/
/******************/
static AgResult add_def_first_8 (AG_U8 opc , AG_U32 tag , ClsbCurrState *curr_state, AgClsbMem *p_mem_handle);
static AgResult add_first_byte_8 (AG_U8 opc , AG_U8 br , AG_U32 tag , ClsbCurrState *curr_state, AgClsbMem *p_mem_handle);
static AgResult add_byte_8 (AG_U8 opc , AG_U8 br , AG_U32 tag , ClsbCurrState *curr_state, AgClsbMem *p_mem_handle);
static AgResult add_def_byte_8 (AG_U8 opc , AG_U32 tag , ClsbCurrState *curr_state, AgClsbMem *p_mem_handle);
static AgResult add_def_first_16 ( AG_U8 opc ,  AG_U32 tag , AG_U8 msk, ClsbCurrState *curr_state, AgClsbMem *p_mem_handle);
static AgResult add_first_word_16 ( AG_U8 opc ,  AG_U16 wr ,  AG_U8 msk,  AG_U32 tag , ClsbCurrState *curr_state, AgClsbMem *p_mem_handle);
static AgResult add_word_16 ( AG_U8 opc ,  AG_U16 wr ,  AG_U8 msk,  AG_U32 tag , ClsbCurrState *curr_state, AgClsbMem *p_mem_handle);
static AgResult add_def_word_16 ( AG_U8 opc ,  AG_U32 tag , AG_U8 msk, ClsbCurrState *curr_state, AgClsbMem *p_mem_handle);

static AgResult cls_search_new_entry_and_allocate_8( AG_U32 *curr_entry ,  ClsbCurrState *curr_state, AgClsbMem *p_mem_handle);
static AgResult cls_search_new_entry_and_allocate_16( AG_U32 *curr_entry ,  ClsbCurrState *curr_state, AgClsbMem *p_mem_handle);

static AgResult find_next_pt_8 (ClsbCurrState *curr_state, AgClsbMem *p_mem_handle);
static AgResult find_next_pt_16(ClsbCurrState *curr_state, AgClsbMem *p_mem_handle);

static AgResult cls_update_and_push(AG_U32 n_ent_num, 
									AG_U16 n_pt,  
									AG_U16 n_rec,
									AG_U16 n_ref_count,  
									AG_U32 n_ent_val, 
									AG_U8 n_table,
									ClsbStack *p_stack);

static AgResult copy_entries_8 ( AG_U32 *curr_entry , ClsbCurrState *curr_state, AgClsbMem *p_mem_handle);
static AgResult copy_entries_16(  AG_U32 *curr_entry ,  ClsbCurrState *curr_state, AgClsbMem *p_mem_handle);
static AgResult reset_records (ClsbCurrState *curr_state);

static void clsb_eng_clear_ent_in_memory_8( ClsbStackElement *p_element, AgClsbMem *p_mem_handle);
static void clsb_eng_write_ent_to_memory_8(ClsbStackElement *p_element, AgClsbMem *p_mem_handle);

static void   inc_ref_count(AgClsbMem* p_mem, AG_U32 n_rec, AG_U32 n_entry);
static AG_U16 dec_ref_count(AgClsbMem* p_mem, AG_U32 n_rec, AG_U32 n_entry);



/*Add IO functions to classifyer memory*/

#define CLS_USE_IO_FUNCTION

#ifdef CLS_USE_IO_FUNCTION
#define CLS_RECORDS_OFFSET_8(n_ent,n_tab,n_rec) ((n_rec)*REC_SIZE_8)+((n_tab)*TBL_SIZE_8)+((n_ent)*ENT_SIZE)
#define CLS_RECORDS_BASE_OFFSET_8(n_base,n_offset) (AG_U32*)(((AG_U32)(n_base)+n_offset))

void    
ag_nd_lstree_mem_read(void *p_device, AG_U32 n_offset, void *p_data, AG_U32 n_word_count);
void    
ag_nd_lstree_mem_write(void *p_device, AG_U32 n_offset, void *p_data, AG_U32 n_word_count);


AgResult CLS_RECORDS_8_READ(AgClsbMem *p_mem_handle, AG_U32 n_ent, AG_U32 n_tab, AG_U32 n_rec, AG_U32 *val)
{
AG_U32 n_offset;
if(p_mem_handle->h_device == NULL)
{
  /*Do it as now, direct memory*/
  *val = CLS_RECORDS_8(p_mem_handle->p_record_base,n_ent,n_tab,n_rec);
}
else
{
 /*Memory base is already set in CES*/   
 n_offset = ((n_rec)*REC_SIZE_8)+((n_tab)*TBL_SIZE_8)+((n_ent)*ENT_SIZE);
 ag_nd_lstree_mem_read(p_mem_handle->h_device, n_offset, val, 2 /*2 words, 4 bytes*/);
}
 return AG_S_OK;
}

AG_U32 CLS_RECORDS_8_GET(AgClsbMem *p_mem_handle, AG_U32 n_ent, AG_U32 n_tab, AG_U32 n_rec)
{
AG_U32 n_offset;
if(p_mem_handle->h_device == NULL)
{
  /*Do it as now, direct memory*/
  return CLS_RECORDS_8(p_mem_handle->p_record_base,n_ent,n_tab,n_rec);
}
else
{
 AG_U32 val;
 /*Memory base is already set in CES*/   
 n_offset = ((n_rec)*REC_SIZE_8)+((n_tab)*TBL_SIZE_8)+((n_ent)*ENT_SIZE);
 ag_nd_lstree_mem_read(p_mem_handle->h_device, n_offset, &val, 2 /*2 words, 4 bytes*/);
 return val;
}
}



AgResult CLS_RECORDS_8_WRITE(AgClsbMem *p_mem_handle, AG_U32 n_ent, AG_U32 n_tab, AG_U32 n_rec, AG_U32 *val)
{
AG_U32 n_offset;
if(p_mem_handle->h_device == NULL)
{
  AG_U32 * entry;   
  /*Do it as now, direct memory*/
  entry = &CLS_RECORDS_8(p_mem_handle->p_record_base,n_ent,n_tab,n_rec);
  *entry = *val;  
}
else
{
 /*Memory base is already set in CES*/   
 n_offset = ((n_rec)*REC_SIZE_8)+((n_tab)*TBL_SIZE_8)+((n_ent)*ENT_SIZE);
 ag_nd_lstree_mem_write(p_mem_handle->h_device, n_offset, val, 2 /*2 words, 4 bytes*/);
}
return AG_S_OK;
}

AgResult CLS_RECORDS_8_PUT(AgClsbMem *p_mem_handle, AG_U32 n_ent, AG_U32 n_tab, AG_U32 n_rec, AG_U32 val)
{
AG_U32 n_offset;
if(p_mem_handle->h_device == NULL)
{
  AG_U32 * entry;   
  /*Do it as now, direct memory*/
  entry = &CLS_RECORDS_8(p_mem_handle->p_record_base,n_ent,n_tab,n_rec);
  *entry = val;  
}
else
{
 /*Memory base is already set in CES*/
 AG_U32 par = val;
 n_offset = ((n_rec)*REC_SIZE_8)+((n_tab)*TBL_SIZE_8)+((n_ent)*ENT_SIZE);
 ag_nd_lstree_mem_write(p_mem_handle->h_device, n_offset, &par, 2 /*2 words, 4 bytes*/);
}
return AG_S_OK;
}



AgResult CLS_RECORDS_8_READ_OFFSET(AgClsbMem *p_mem_handle,AG_U32 n_offset, AG_U32 *val)
{
if(p_mem_handle->h_device == NULL)
{
   AG_U32 * entry;   
  /*Do it as now, direct memory*/
  entry = CLS_RECORDS_BASE_OFFSET_8(p_mem_handle->p_record_base,n_offset);
  *val = *entry;
}
else
{
 /*Memory base is already set in CES*/   
 ag_nd_lstree_mem_read(p_mem_handle->h_device, n_offset, val, 2 /*2 words, 4 bytes*/);
}
 return AG_S_OK;
}

AgResult CLS_RECORDS_8_WRITE_OFFSET(AgClsbMem *p_mem_handle, AG_U32 n_offset, AG_U32 *val)
{
if(p_mem_handle->h_device == NULL)
{
  AG_U32 * entry;   
  /*Do it as now, direct memory*/
  entry = CLS_RECORDS_BASE_OFFSET_8(p_mem_handle->p_record_base,n_offset);
  *entry = *val;  
}
else
{
 /*Memory base is already set in CES*/   
 ag_nd_lstree_mem_write(p_mem_handle->h_device, n_offset, val, 2 /*2 words, 4 bytes*/);
}
return AG_S_OK;
}
 
/*
    Set classifyer memory
     n_offset - offset in classifyer
     data      - data to set
     len       - in bytes, written to lstree 4 bytes on each call
*/   
AgResult CLS_MEMSET_WORD_OFFSET(AgClsbMem *p_mem_handle, AG_U32 n_offset, AG_U32 data ,AG_U32 len)
{
 AG_U32 val = data;
 AG_U32 loop,a_offset=n_offset;
 for(loop=0;loop<len/4;loop++)
 {
  /*Memory base is already set in CES*/
  /*Write 4 bytes of data */
  ag_nd_lstree_mem_write(p_mem_handle->h_device, a_offset, &val, 2 /*2 words, 4 bytes*/);
  a_offset += 4; 
 }
 return AG_S_OK;
}

#endif

/*********************************************************************/
/* The following function adds the first byte to a mini program. It  */
/* first check to see if the entry is empty. if it is not empty and	 */
/* does not equal the byte it allocates a new table and copy all the */
/* entries corelating to the default path tag into it.				 */
/*********************************************************************/
AgResult add_first_byte_8 (AG_U8 opc , AG_U8 br , AG_U32 tag, ClsbCurrState *curr_state, AgClsbMem *p_mem_handle)
{
AG_U32 *entry; /* A pointer to the current entry */
AG_U32 n_aux_ent_val;/*Value to be pushed  stack */


#ifdef CLS_USE_IO_FUNCTION
AG_U32 entry_data; /*Value of entry*/
entry=&entry_data; /* A pointer to the current entry */
#endif


#ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "\n\t\tSTART: add_first_byte_8\n");
	fprintf (full_trace, "\t\t\tParameters:\n\t\t\t\topc %d\n\t\t\t\tbr %d\n\t\t\t\ttag %d\n", opc, br, tag);
	fprintf (full_trace, "\t\t\t\tcurr_state:\n\t\t\t\t\tb_is_extended %d\n",
		curr_state->b_is_extended);
	fprintf (full_trace, "\t\t\t\t\tcurr_data %d\n\t\t\t\t\tcurr_entry_num %d\n", 
		curr_state->curr_data, curr_state->curr_entry_num);
	fprintf (full_trace, "\t\t\t\t\tcurr_pt %d\n\t\t\t\t\tcurr_rec %d\n\t\t\t\t\tcurr_table %d\n", 
		curr_state->curr_pt, curr_state->curr_rec, curr_state->curr_table);
	fprintf (full_trace, "\t\t\t\t\tdef_mask %x\n\t\t\t\t\tdef_pt %d\n\t\t\t\t\tdef_rec %d\n",
		curr_state->def_mask, curr_state->def_pt, curr_state->def_rec);
	fprintf (full_trace, "\t\t\t\t\tnext_pt %d\n\t\t\t\t\tnext_rec %d\n", 
		curr_state->next_pt, curr_state->next_rec);
	fprintf (full_trace, "\t\t\t\t\tn_num_of_ent_to_copy %d\n\t\t\t\t\tnext_entry_num %d\n", 
		curr_state->n_num_of_ent_to_copy, curr_state->next_entry_num);
#endif /* CLS_TEST_FULL_TRACE */

	/* Is the current entry empty ? */
	entry_data = CLS_RECORDS_8_GET(p_mem_handle,(AG_U8)(br + curr_state->curr_pt),PRI_TABLE,curr_state->curr_rec);
    if (*entry == EMPTY_ENTRY_8 )
	{ /*Then needed entry is empty. */

        #ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\t\t\t\t\t- current entry is empty (ent %d table %d rec %d val 0x%08x)\n",
			(br + curr_state->curr_pt),PRI_TABLE,curr_state->curr_rec, *entry);
        #endif /* CLS_TEST_FULL_TRACE */

        if (opc != AG_CLSB_OPC8_TAG)
        {      /*prepare the entry content (without next record and next path tag)*/
			n_aux_ent_val = CLS_ALL_ONES & MAKE_OPCODE_8(opc) & MAKE_PT_8(curr_state->curr_pt);
        }
		else
        {   /*prepare the full entry content (with tag)*/
			n_aux_ent_val = CLS_ALL_ONES & MAKE_OPCODE_8(opc) & MAKE_PT_8(curr_state->curr_pt) & MAKE_TAG_8(tag);
        }

		/*Push the entry to the stack*/
		if(AG_FAILED(cls_stack_push_8((AG_U8)(br+curr_state->curr_pt),
											   curr_state->curr_rec,
											   n_aux_ent_val, 
											   PRI_TABLE,
											   &p_mem_handle->x_new_stack)))
		{

            #ifdef CLS_TEST_FULL_TRACE
		    fprintf (full_trace, "\t\tEND: add_first_byte_8 (after cls_update_and_push) failed\n");
            #endif /* CLS_TEST_FULL_TRACE */

			return AG_E_CLS_FULL;
        }

		/*Update the primary entry as pushed_entry */
        *entry=PUSHED_ENTRY_8;
        CLS_RECORDS_8_PUT(p_mem_handle,(AG_U8)(br + curr_state->curr_pt),PRI_TABLE,curr_state->curr_rec,*entry);
		curr_state->curr_data = br;

        #ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\t\t\t\t\t- mark primary entry as pushed (ent %d table %d rec %d val 0x%08x).\n",
			(br + curr_state->curr_pt),PRI_TABLE,curr_state->curr_rec,
			CLS_RECORDS_8_GET(p_mem_handle,(AG_U8)(br + curr_state->curr_pt),PRI_TABLE,curr_state->curr_rec));
		fprintf (full_trace, "\t\tEND: add_first_byte_8 (AG_S_OK)\n");
        #endif /* CLS_TEST_FULL_TRACE */

		return AG_S_OK;
	} /* of if */
/*	if ((PT_8(*entry) != curr_state->curr_pt) */
/*		||(IS_EXTENDED_8(p_mem_handle->p_extend,curr_state->curr_pt,curr_state->curr_rec,p_mem_handle->n_record_limit)))  */
 	if ((PT_8(*entry) != curr_state->curr_pt)||/* Is the current entry taken ? */
		((PT_8(*entry) == curr_state->curr_pt)&&(IS_EXTENDED_8(p_mem_handle->p_extend,curr_state->curr_pt,curr_state->curr_rec,p_mem_handle->n_record_limit)))) 
     { /*Entry is taken by another path tag or by a skip value */

        /*update the next_entry_num*/
		curr_state->next_entry_num=(AG_U8)(curr_state->curr_pt+br);

        #ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\t\t\t\t\t- current entry is occupied by other PT %d\n", 
			PT_8(*entry));
		fprintf (full_trace, "\t\t\t\t\t- update curr_state->next_entry_num %d\n", 
			curr_state->next_entry_num);
        #endif /* CLS_TEST_FULL_TRACE */

		/* Search for empty entries and allocate a new record */
        if (cls_search_new_entry_and_allocate_8(NULL, curr_state, p_mem_handle)  & AG_E_FAIL)
		{

            #ifdef CLS_TEST_FULL_TRACE
			fprintf (full_trace, "\t\tEND: add_first_byte_8 (AG_E_FAIL after cls_search_new_entry_and_allocate_8)\n");
            #endif /* CLS_TEST_FULL_TRACE */

			return AG_E_FAIL;
		}

		if (copy_entries_8(NULL, curr_state, p_mem_handle) & AG_E_FAIL)	/* Copy all the entries correlating to the */
		{

            #ifdef CLS_TEST_FULL_TRACE
			fprintf (full_trace, "\t\tEND: add_first_byte_8 (AG_E_FAIL after copy_entries_8)\n");
            #endif /* CLS_TEST_FULL_TRACE */

			return AG_E_FAIL;
		}

		/* After handling the existing entries to be copied,
           Now handle the new entry */
        /* Point entry to the the new allocated table*/
        entry_data = CLS_RECORDS_8_GET(p_mem_handle,(AG_U8)(br + curr_state->next_pt),PRI_TABLE,curr_state->next_rec);

		/* Update entry*/
		if (opc != AG_CLSB_OPC8_TAG)
			n_aux_ent_val = CLS_ALL_ONES & MAKE_OPCODE_8(opc) & MAKE_PT_8(curr_state->next_pt);
		else
			n_aux_ent_val = CLS_ALL_ONES & MAKE_OPCODE_8(opc)& MAKE_PT_8(curr_state->next_pt) & MAKE_TAG_8(tag);

		/*update the stack element and push to the stack*/
		if(AG_FAILED(cls_stack_push_8((AG_U8)(br+curr_state->next_pt),
												curr_state->next_rec,
												n_aux_ent_val,
												PRI_TABLE,
												&p_mem_handle->x_new_stack)))
		{

#ifdef CLS_TEST_FULL_TRACE
			fprintf (full_trace, "\t\tEND: add_first_byte_8 (after cls_update_and_push) failed\n");
#endif /* CLS_TEST_FULL_TRACE */

			return AG_E_CLS_FULL;
		}
		/*Update the primary entry as pushed_entry*/
		*entry=PUSHED_ENTRY_8;
        CLS_RECORDS_8_PUT(p_mem_handle,(AG_U8)(br + curr_state->next_pt),PRI_TABLE,curr_state->next_rec,entry_data);
		/* Points pointers to current node */
		curr_state->curr_pt = curr_state->next_pt;
		curr_state->curr_rec = curr_state->next_rec;
		curr_state->curr_data = br;

        #ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\t\t\t\t\t- mark new allocated entry as puched (ent %d table %d rec %d val 0x%08x).\n",
			(br + curr_state->next_pt),PRI_TABLE,curr_state->next_rec,
			CLS_RECORDS_8_GET(p_mem_handle,(AG_U8)(br + curr_state->next_pt),PRI_TABLE,curr_state->next_rec));
		fprintf (full_trace, "\t\t\t\t\t- update curr_state: curr_pt %d, curr_rec %d, curr_data %d\n",
			curr_state->curr_pt, curr_state->curr_rec, curr_state->curr_data);
		fprintf (full_trace, "\t\t\t\tcurr_state:\n\t\t\t\t\tb_is_extended %d\n",
			curr_state->b_is_extended);
		fprintf (full_trace, "\t\t\t\t\tcurr_data %d\n\t\t\t\t\tcurr_entry_num %d\n", 
			curr_state->curr_data, curr_state->curr_entry_num);
		fprintf (full_trace, "\t\t\t\t\tcurr_pt %d\n\t\t\t\t\tcurr_rec %d\n\t\t\t\t\tcurr_table %d\n", 
			curr_state->curr_pt, curr_state->curr_rec, curr_state->curr_table);
		fprintf (full_trace, "\t\t\t\t\tdef_mask %x\n\t\t\t\t\tdef_pt %d\n\t\t\t\t\tdef_rec %d\n",
			curr_state->def_mask, curr_state->def_pt, curr_state->def_rec);
		fprintf (full_trace, "\t\t\t\t\tnext_pt %d\n\t\t\t\t\tnext_rec %d\n", 
			curr_state->next_pt, curr_state->next_rec);
		fprintf (full_trace, "\t\t\t\t\tn_num_of_ent_to_copy %d\n\t\t\t\t\tnext_entry_num %d\n", 
			curr_state->n_num_of_ent_to_copy, curr_state->next_entry_num);

		fprintf (full_trace, "\t\tEND: add_first_byte_8 (AG_S_OK)\n");
        #endif /* CLS_TEST_FULL_TRACE */

		return AG_S_OK;
	}
	else /* Current entry taken by the same path tag */
	{

        #ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\t\t\t\t\t- current entry is occupied by the same PT %d\n",
			PT_8(*entry));
        #endif /* CLS_TEST_FULL_TRACE */

		if (OPCODE_8(*entry) == AG_CLSB_OPC8_TAG)
		{
			curr_state->curr_pt = curr_state->next_pt;
			curr_state->curr_rec = curr_state->next_rec;
			curr_state->curr_data = br;
			curr_state->curr_table = PRI_TABLE;
			if(((*entry) & TAG_MASK_8)==tag)
			{

            #ifdef CLS_TEST_FULL_TRACE
			fprintf (full_trace, "\t\tEND: add_first_byte_8 (AG_S_CLS_PATH_EXIST)\n");
            #endif /* CLS_TEST_FULL_TRACE */

		/*		curr_state->curr_table = PRI_TABLE; */
				return AG_S_CLS_PATH_EXIST;/*Path already exists, ambiguous but ok. */
			}
			else
			{

            #ifdef CLS_TEST_FULL_TRACE
			fprintf (full_trace, "\t\tEND: add_first_byte_8 (AG_E_CLS_TAGGED)\n");
            #endif /* CLS_TEST_FULL_TRACE */

				return AG_E_CLS_TAGGED; /* Already tagged */
			}
		}

		if (OPCODE_8(*entry) == opc)
		{
			curr_state->curr_data = br;

            #ifdef CLS_TEST_FULL_TRACE
			fprintf (full_trace, "\t\t\t\tcurr_state:\n\t\t\t\t\tb_is_extended %d\n",
				curr_state->b_is_extended);
			fprintf (full_trace, "\t\t\t\t\tcurr_data %d\n\t\t\t\t\tcurr_entry_num %d\n", 
				curr_state->curr_data, curr_state->curr_entry_num);
			fprintf (full_trace, "\t\t\t\t\tcurr_pt %d\n\t\t\t\t\tcurr_rec %d\n\t\t\t\t\tcurr_table %d\n", 
				curr_state->curr_pt, curr_state->curr_rec, curr_state->curr_table);
			fprintf (full_trace, "\t\t\t\t\tdef_mask %x\n\t\t\t\t\tdef_pt %d\n\t\t\t\t\tdef_rec %d\n",
				curr_state->def_mask, curr_state->def_pt, curr_state->def_rec);
			fprintf (full_trace, "\t\t\t\t\tnext_pt %d\n\t\t\t\t\tnext_rec %d\n", 
				curr_state->next_pt, curr_state->next_rec);
			fprintf (full_trace, "\t\t\t\t\tn_num_of_ent_to_copy %d\n\t\t\t\t\tnext_entry_num %d\n", 
				curr_state->n_num_of_ent_to_copy, curr_state->next_entry_num);
			fprintf (full_trace, "\t\tEND: add_first_byte_8 (AG_S_OK)\n");
            #endif /* CLS_TEST_FULL_TRACE */

			return AG_S_OK;
		}

        #ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\t\tEND: add_first_byte_8 (AG_E_CLS_AMBIG)\n");
        #endif /* CLS_TEST_FULL_TRACE */

		if (opc == AG_CLSB_OPC8_TAG)
		{
			curr_state->next_pt = (AG_U16)NPT_8(*entry);
			curr_state->next_rec = (AG_U16)NR_8(*entry);
            #ifdef CLS_TEST_FULL_TRACE
		    fprintf (full_trace, "\t\tEND: add_first_byte_8 (AG_E_CLS_PATH_CONT)\n");
            #endif /* CLS_TEST_FULL_TRACE */

			return AG_E_CLS_PATH_CONT;
		}

        #ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\t\tEND: add_first_byte_8 (AG_E_CLS_AMBIG)\n");
        #endif /* CLS_TEST_FULL_TRACE */

		return AG_E_CLS_AMBIG;  /* Same path tag different op code */
	} /* of else */  

} /* of add_first_byte_8 */

/*********************************************************
Name: 
    add_first_word_16
Description:
   	Adds the first word  16 mode of work
Parameters:
	opc - the opcode
	wr- the word read
	msk - the mask for next word
	tag - the tag if opc==TAG_16
	x_curr_state - the actual state.
Returns AgResult:
    Success: AG_S_OK
	Fail : AG_E_FAIL
 ***********************************************************/

AgResult add_first_word_16 ( AG_U8 opc ,  AG_U16 wr ,  AG_U8 msk,  AG_U32 tag , ClsbCurrState *curr_state, AgClsbMem *p_mem_handle)
{
AG_U32 *entry; /* A pointer to the current entry */
AG_U32 n_aux_entry_num;
AG_U16 n_aux_pt;
AG_U32 n_aux_ent_val;/*Value to be pushed  stack */
AgResult n_res;

	/*The entry number of primary entry  the table is calculated as follows: */
	/*	(wr+pt)/2)*4+(wr+pt)%2+1 - odd or even entry. */
	n_aux_entry_num=PRI_ENTRY_NUM_16(wr,curr_state->curr_pt);

	/* Is the current entry empty ? */
	if (EMPTY_ENTRY_16 == *(entry = &CLS_RECORDS_16(p_mem_handle->p_record_base,n_aux_entry_num,curr_state->curr_rec)))
	{ 
		/* verify opcode and stet the value to be pushed to the stack */
		if (opc != AG_CLSB_OPC16_TAG)
			n_aux_ent_val=CLS_ALL_ONES & MAKE_OPCODE_16(opc) & MAKE_MASK_16(msk);
		else
			n_aux_ent_val=CLS_ALL_ONES & MAKE_OPCODE_16(opc) & MAKE_MASK_16(DEF_MSK_16) & MAKE_TAG_16(tag);

		/*update the stack element and push to the stack */
		if(AG_FAILED(n_res=cls_update_and_push(n_aux_entry_num,
											   curr_state->curr_pt,
											   curr_state->curr_rec,
											   CLS_REF_COUNT_16(p_mem_handle->p_ref_count,curr_state->curr_pt,curr_state->curr_rec, p_mem_handle->n_record_limit),
											   n_aux_ent_val,
											   (AG_U8)PRI_TABLE,
											   &p_mem_handle->x_new_stack)))
			return n_res;

		/* Update the use of the path tag - set default as taken   */
		if (CLS_RECORDS_16(p_mem_handle->p_record_base,DEF_ENTRY_NUM_16(curr_state->curr_pt),curr_state->curr_rec) 
			== EMPTY_ENTRY_16) 
        {
			CLS_RECORDS_16(p_mem_handle->p_record_base,DEF_ENTRY_NUM_16(curr_state->curr_pt),curr_state->curr_rec) =  
                CLS_ALL_ONES & MAKE_OPCODE_16(AG_CLSB_OPC16_TAG) & MAKE_MASK_16(DEF_MSK_16) & MAKE_TAG_16(0);
        }

		/*Update the primary entry as pushed_entry */
		*entry=PUSHED_ENTRY_16;

		/*MBCOMP - 11/12/2000 - mark the entry  the image bit table */
		CLS_REC_IMAGE_16(p_mem_handle->p_record_image,BIT_TABLE_ENTRY(n_aux_entry_num/2),curr_state->curr_rec,p_mem_handle->n_record_limit)
			=CLS_REC_IMAGE_16(p_mem_handle->p_record_image,BIT_TABLE_ENTRY(n_aux_entry_num/2),curr_state->curr_rec,p_mem_handle->n_record_limit)
				|BIT_ENTRY_POS(n_aux_entry_num/2);

		curr_state->curr_data = wr;

		CLS_REF_COUNT_16(p_mem_handle->p_ref_count,curr_state->curr_pt,curr_state->curr_rec, p_mem_handle->n_record_limit)++;

		return AG_S_OK;
	} /* of if */

	/*calculate the PT - entry may be even or odd */
	if (((n_aux_entry_num-1)%2)==0)
		n_aux_pt=(AG_U16)PT0_16(CLS_RECORDS_16(p_mem_handle->p_record_base,n_aux_entry_num-1,curr_state->curr_rec));
	else
		n_aux_pt=(AG_U16)PT1_16(CLS_RECORDS_16(p_mem_handle->p_record_base,n_aux_entry_num-2,curr_state->curr_rec));

	if (n_aux_pt != curr_state->curr_pt)/* Is the current entry taken ? */
	{
		/*MBCOMP - 11/12/2000 update the next_entry_num */
		curr_state->next_entry_num=PRI_ENTRY_NUM_16(wr,curr_state->curr_pt);

		/* Allocate a new table */
		if (cls_search_new_entry_and_allocate_16(NULL, curr_state, p_mem_handle)  & AG_E_FAIL) /*Search for empty entries and allocate a new table */
			return AG_E_FAIL;

		/*MB16 - 28/11/2000 - pass the current entry num to copy entries */
		/*don't need to update for null */
		/*curr_state->curr_entry_num=n_aux_entry_num; */

		if (copy_entries_16(NULL, curr_state, p_mem_handle) & AG_E_FAIL)	/* Copy all the entries correlating to the  */
			return AG_E_FAIL;								/* first path tag into the new table */

		/*Calculate the table entry */
		n_aux_entry_num=(AG_U32)PRI_ENTRY_NUM_16(wr,curr_state->next_pt);

		/* Point entry to the the new allocated table */
		entry = &CLS_RECORDS_16(p_mem_handle->p_record_base,n_aux_entry_num,curr_state->next_rec);/*[n_aux_entry_num][curr_state->next_rec]*/

		/* verify opcode and stet the value to be pushed to the stack */
		if (opc != AG_CLSB_OPC16_TAG)
			n_aux_ent_val=CLS_ALL_ONES & MAKE_OPCODE_16(opc) & MAKE_MASK_16(msk);
		else
			n_aux_ent_val=CLS_ALL_ONES & MAKE_OPCODE_16(opc) & MAKE_MASK_16(DEF_MSK_16) & MAKE_TAG_16(tag);

		/*update the stack element and push to the stack */
		if(AG_FAILED(n_res=cls_update_and_push(n_aux_entry_num,
											   curr_state->next_pt,
											   curr_state->next_rec,
											   CLS_REF_COUNT_16(p_mem_handle->p_ref_count,curr_state->next_pt,curr_state->next_rec, p_mem_handle->n_record_limit),
											   n_aux_ent_val,
											   PRI_TABLE,
											   &p_mem_handle->x_new_stack)))
			return n_res;

		/*Update the primary entry as pushed_entry */
		*entry=PUSHED_ENTRY_16;

		/* Points pointers to current node */
		curr_state->curr_pt = curr_state->next_pt;
		curr_state->curr_rec = curr_state->next_rec;
		curr_state->curr_data = wr;

		/*MBCOMP - 11/12/2000 - mark the entry  the image bit table */
		CLS_REC_IMAGE_16(p_mem_handle->p_record_image,BIT_TABLE_ENTRY(n_aux_entry_num/2),curr_state->curr_rec,p_mem_handle->n_record_limit)
			=CLS_REC_IMAGE_16(p_mem_handle->p_record_image,BIT_TABLE_ENTRY(n_aux_entry_num/2),curr_state->curr_rec,p_mem_handle->n_record_limit)
				|BIT_ENTRY_POS(n_aux_entry_num/2);

		CLS_REF_COUNT_16(p_mem_handle->p_ref_count,curr_state->curr_pt,curr_state->curr_rec, p_mem_handle->n_record_limit)++;

		return AG_S_OK;
	}
	else /* Current entry taken by the same path tag */
	{
		if (OPCODE_16(*entry) == AG_CLSB_OPC16_TAG)
		{
			if(TG_16(*entry)==tag)
			{
				return AG_S_CLS_PATH_EXIST;/*Path already exists. Ambig but OK. */
			}
			else
			{
				return AG_E_CLS_TAGGED; /* Already tagged */
			}
		}

		if ((OPCODE_16(*entry) == opc)&&(MASK_16(*entry)==msk))
		{
			curr_state->curr_data = wr;
			return AG_S_OK;
		}

		if (opc == AG_CLSB_OPC16_TAG)
		{
			curr_state->next_pt = (AG_U16)NPT_16(*entry);
			curr_state->next_rec = (AG_U16)NR_16(*entry);
			return AG_E_CLS_PATH_CONT;
		}

		return AG_E_CLS_AMBIG;  /* Same path tag different op code or mask */
	} /* of else */  

} /* of add_first_word_16 */



/*********************************************************************/
/* The following function adds a default byte to a certain path. it  */
/* checks to see that the path tag exist, and if so it adds the      */
/* default action to be taken.ag into it.                            */
/*********************************************************************/
AgResult add_def_byte_8 (AG_U8 opc, 
						 AG_U32 tag, 
						 ClsbCurrState *curr_state, 
						 AgClsbMem *p_mem_handle)
{

AG_U32 curr_entry_data; /*Value of entry*/
AG_U32 *curr_entry_pointer = 0;
AG_U32 curr_entry_offset = 0;

AG_U32 next_entry_data; /*Value of next entry*/

AG_U32 memory_stack; /*0=memory, else stack*/


AG_U32 *curr_entry = &curr_entry_data;
AG_U32 *next_entry = &next_entry_data;
AG_U32 n_aux_ent_val;/*Value to be pushed  stack*/

#ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "\n\t\tSTART: add_def_byte_8\n");
	fprintf (full_trace, "\t\t\tParameters:\n\t\t\t\topc %d\n\t\t\t\ttag %d\n", opc, tag);

	fprintf (full_trace, "\t\t\t\tcurr_state:\n\t\t\t\t\tb_is_extended %d\n",
		curr_state->b_is_extended);
	fprintf (full_trace, "\t\t\t\t\tcurr_data %d\n\t\t\t\t\tcurr_entry_num %d\n", 
		curr_state->curr_data, curr_state->curr_entry_num);
	fprintf (full_trace, "\t\t\t\t\tcurr_pt %d\n\t\t\t\t\tcurr_rec %d\n\t\t\t\t\tcurr_table %d\n", 
		curr_state->curr_pt, curr_state->curr_rec, curr_state->curr_table);
	fprintf (full_trace, "\t\t\t\t\tdef_mask %x\n\t\t\t\t\tdef_pt %d\n\t\t\t\t\tdef_rec %d\n",
		curr_state->def_mask, curr_state->def_pt, curr_state->def_rec);
	fprintf (full_trace, "\t\t\t\t\tnext_pt %d\n\t\t\t\t\tnext_rec %d\n", 
		curr_state->next_pt, curr_state->next_rec);
	fprintf (full_trace, "\t\t\t\t\tn_num_of_ent_to_copy %d\n\t\t\t\t\tnext_entry_num %d\n", 
		curr_state->n_num_of_ent_to_copy, curr_state->next_entry_num);
#endif /* CLS_TEST_FULL_TRACE */

	if (opc != AG_CLSB_OPC8_TAG)
		tag = CLS_ALL_ONES;

	/*if the stack is empty, there is no new entry, point to the record*/
	if(cls_stack_state(&p_mem_handle->x_new_stack)==AG_S_CLS_EMPTY)
	{
		if (cls_stack_state(&p_mem_handle->x_old_stack)==AG_S_CLS_EMPTY)
		{
			if (curr_state->curr_table == PRI_TABLE)
			{
                memory_stack = 0;
                curr_entry_offset = CLS_RECORDS_OFFSET_8((AG_U8)(curr_state->curr_data + curr_state->curr_pt),PRI_TABLE,curr_state->curr_rec);
                CLS_RECORDS_8_READ_OFFSET(p_mem_handle,curr_entry_offset,&curr_entry_data);
            }
			else
			{
                memory_stack = 0;
                curr_entry_offset = CLS_RECORDS_OFFSET_8((AG_U8)(curr_state->curr_pt),DEF_TABLE,curr_state->curr_rec);
                CLS_RECORDS_8_READ_OFFSET(p_mem_handle,curr_entry_offset,&curr_entry_data);
            }
		 #ifdef CLS_TEST_FULL_TRACE
			fprintf (full_trace, "\t\t\t\t\t- stack is empty (there is no new entry).\n");
		 #endif /* CLS_TEST_FULL_TRACE */
		}
		else /* new stack empty, but old stack non empty */
		{
			/*Points to the top of stack; */
            memory_stack = 1; /*In stack*/
            curr_entry_pointer = &(p_mem_handle->x_old_stack.a_stack[p_mem_handle->x_old_stack.n_top].n_entry_value);
            curr_entry_data = *(AG_U32 *)(&(p_mem_handle->x_old_stack.a_stack[p_mem_handle->x_old_stack.n_top].n_entry_value));


		    #ifdef CLS_TEST_FULL_TRACE
			fprintf (full_trace, "\t\t\t\t\t- old stack is not empty.\n");
			#endif /* CLS_TEST_FULL_TRACE */
		}
	}
	else
	{
		/*Points to the top of stack; */

        memory_stack = 1; /*In stack*/
        curr_entry_pointer = &(p_mem_handle->x_new_stack.a_stack[p_mem_handle->x_new_stack.n_top].n_entry_value);
        curr_entry_data = *(AG_U32 *)(&(p_mem_handle->x_new_stack.a_stack[p_mem_handle->x_new_stack.n_top].n_entry_value));

        #ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\t\t\t\t\t- new stack is not empty.\n");
        #endif /* CLS_TEST_FULL_TRACE */

	}

	if (((*curr_entry) & TAG_MASK_8) == TAG_MASK_8)
	{ /* This is only true if the stack is not empty. curr_entry in this case
      points to the top element of the stack.
      The top element on the stack is the start of a new branch. Need to allocate its
      next record and path tag and set it to the element in the stack. */

        #ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\t\t\t\t\t- next entry is not allocated (NPT = 0xFFFF, NR =0x1FFF)\n");
        #endif /* CLS_TEST_FULL_TRACE */

		if (find_next_pt_8(curr_state, p_mem_handle) & AG_E_FAIL)
		{

            #ifdef CLS_TEST_FULL_TRACE
		    fprintf (full_trace, "\t\tEND: add_def_byte_8 (AG_E_FAIL after find_next_pt_8)\n");
            #endif /* CLS_TEST_FULL_TRACE */

			return AG_E_FAIL;
		}

		/*Update the top entry */
		*curr_entry &= (MAKE_NR_8(curr_state->next_rec) & MAKE_NPT_8(curr_state->next_pt));
        if(memory_stack)
         {
         *curr_entry_pointer &= (MAKE_NR_8(curr_state->next_rec) & MAKE_NPT_8(curr_state->next_pt));
         }
         else
         {
            curr_entry_data &= (MAKE_NR_8(curr_state->next_rec) & MAKE_NPT_8(curr_state->next_pt)); 
             CLS_RECORDS_8_WRITE_OFFSET(p_mem_handle,curr_entry_offset,&curr_entry_data);
        }


	}

	curr_state->next_rec =(AG_U16) NR_8(*curr_entry);
	curr_state->next_pt = (AG_U16)NPT_8(*curr_entry);

	/*Points to the next default entry */
	next_entry_data = CLS_RECORDS_8_GET(p_mem_handle,curr_state->next_pt,DEF_TABLE,curr_state->next_rec);

	if ((*next_entry == EMPTY_ENTRY_8) 
		|| ((OPCODE_8(*next_entry) == AG_CLSB_OPC8_TAG) 
		&& (((*next_entry) & TAG_MASK_8) == 0)))
    { /* defualt is free or marked as occupied by yet unclassified. */

        #ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\t\t\t\t\t- default entry (ent %d rec %d) is free or occupied, but yet unclassified\n",
			curr_state->next_pt, curr_state->next_rec);
        #endif /* CLS_TEST_FULL_TRACE */

		n_aux_ent_val=CLS_ALL_ONES & MAKE_OPCODE_8(opc) & MAKE_TAG_8(tag);

		/*update the stack element and push to the stack */
		if(AG_FAILED(cls_stack_push_8(curr_state->next_pt,
											   curr_state->next_rec,
											   n_aux_ent_val,
											   DEF_TABLE,
											   &p_mem_handle->x_new_stack)))
		{

            #ifdef CLS_TEST_FULL_TRACE
			fprintf (full_trace, "\t\tEND: add_def_byte_8 (after cls_update_and_push) failed\n");
            #endif /* CLS_TEST_FULL_TRACE */

			return AG_E_CLS_FULL;
		}
		/*mark as occupied (in any case) even though it may already be set like this. */
        *next_entry = OCCUPIED_ENTRY_8;
        CLS_RECORDS_8_PUT(p_mem_handle,curr_state->next_pt,DEF_TABLE,curr_state->next_rec,OCCUPIED_ENTRY_8);

        #ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\t\t\t\t\t- update entry (ent %d tab %d rec %d val %d)\n",
			curr_state->next_pt, DEF_TABLE, curr_state->next_rec,
			CLS_RECORDS_8_GET(p_mem_handle,curr_state->next_pt,DEF_TABLE,curr_state->next_rec));
        #endif /* CLS_TEST_FULL_TRACE */

	}
	else
	{

        #ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\t\t\t\t\t- default entry (ent %d rec %d) is occupied.\n",
		curr_state->next_pt, curr_state->next_rec);
        #endif /* CLS_TEST_FULL_TRACE */

		if ((OPCODE_8(*next_entry) == AG_CLSB_OPC8_TAG) 
			&& (((*next_entry) & TAG_MASK_8) != 0))
		{
			curr_state->curr_rec = curr_state->next_rec;
			curr_state->curr_pt = curr_state->next_pt;
			curr_state->curr_table = DEF_TABLE;
			curr_state->curr_data = 0;
			if(((*next_entry) & TAG_MASK_8)==tag)
			{

            #ifdef CLS_TEST_FULL_TRACE
			fprintf (full_trace, "\t\tEND: add_def_byte_8 (AG_S_CLS_PATH_EXIST)\n");
            #endif /* CLS_TEST_FULL_TRACE */

			/*	curr_state->curr_table = DEF_TABLE; */
				return AG_S_CLS_PATH_EXIST;/*Path already exists, ambiguous but ok. */
			}
			else
			{

                #ifdef CLS_TEST_FULL_TRACE
			    fprintf (full_trace, "\t\tEND: add_def_byte_8 (AG_E_CLS_TAGGED)\n");
                #endif /* CLS_TEST_FULL_TRACE */

				return AG_E_CLS_TAGGED; /* Already tagged */
			}
		}
		else
			if (OPCODE_8(*next_entry) != opc)
			{

				if (opc == AG_CLSB_OPC8_TAG)
				{

					curr_state->next_pt = (AG_U16)NPT_8(*next_entry);
					curr_state->next_rec =  (AG_U16)NR_8(*next_entry);
                    #ifdef CLS_TEST_FULL_TRACE
					fprintf (full_trace, "\t\tEND: add_def_byte_8 (AG_E_CLS_PATH_CONT)\n");
                    #endif /* CLS_TEST_FULL_TRACE */

					return AG_E_CLS_PATH_CONT;
				}
                #ifdef CLS_TEST_FULL_TRACE
				fprintf (full_trace, "\t\tEND: add_def_byte_8 (AG_S_CLS_PATH_EXIST)\n");
                #endif /* CLS_TEST_FULL_TRACE */

				return AG_E_CLS_AMBIG;
			}
	}

	curr_state->curr_rec = curr_state->next_rec;
	curr_state->curr_pt = curr_state->next_pt;
	curr_state->curr_table = DEF_TABLE;
	curr_state->curr_data = 0;

    #ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "\t\t\t\tcurr_state:\n\t\t\t\t\tb_is_extended %d\n",
		curr_state->b_is_extended);
	fprintf (full_trace, "\t\t\t\t\tcurr_data %d\n\t\t\t\t\tcurr_entry_num %d\n", 
		curr_state->curr_data, curr_state->curr_entry_num);
	fprintf (full_trace, "\t\t\t\t\tcurr_pt %d\n\t\t\t\t\tcurr_rec %d\n\t\t\t\t\tcurr_table %d\n", 
		curr_state->curr_pt, curr_state->curr_rec, curr_state->curr_table);
	fprintf (full_trace, "\t\t\t\t\tdef_mask %x\n\t\t\t\t\tdef_pt %d\n\t\t\t\t\tdef_rec %d\n",
		curr_state->def_mask, curr_state->def_pt, curr_state->def_rec);
	fprintf (full_trace, "\t\t\t\t\tnext_pt %d\n\t\t\t\t\tnext_rec %d\n", 
		curr_state->next_pt, curr_state->next_rec);
	fprintf (full_trace, "\t\t\t\t\tn_num_of_ent_to_copy %d\n\t\t\t\t\tnext_entry_num %d\n", 
		curr_state->n_num_of_ent_to_copy, curr_state->next_entry_num);

	fprintf (full_trace, "\t\tEND: add_def_byte_8 (AG_S_OK)\n");
    #endif /* CLS_TEST_FULL_TRACE */

    return AG_S_OK;
} /* of add_def_byte_8 */
/*********************************************************
Name: 
    add_def_word_16
Description:
   	Adds the default word  16 mode of work
Parameters:
		opc - opcode.
		tag - classification tag if opcode==TAG_16
		curr_state -describes the actual situation.

Returns AgResult:
    Success: AG_S_OK
	Fail : AG_E_FAIL
 ***********************************************************/

AgResult add_def_word_16( AG_U8 opc ,  AG_U32 tag , AG_U8 msk, ClsbCurrState *curr_state, AgClsbMem *p_mem_handle)
 {
AG_U32 *curr_entry;
AG_U32 *next_entry;
AgResult n_res;
AG_U32 n_aux_ent_val;/*Value to be pushed  stack */

	if (opc != AG_CLSB_OPC16_TAG)
	{/*set tag to empty  not leaf */
		tag = CLS_ALL_ONES;/*MBFF - 31/12/2000 */
	}
	else
	{/*set mask to 0xf if opc=15 */
		msk=DEF_MSK_16;
	}
	/*MB16 - if the stack is empty, there is no new entry, point to the record */
	if(cls_stack_state(&p_mem_handle->x_new_stack)==AG_S_CLS_EMPTY)
	{
		if(cls_stack_state(&p_mem_handle->x_old_stack)==AG_S_CLS_EMPTY)
		{
			if (curr_state->curr_table == PRI_TABLE)
			{
				curr_entry = &CLS_RECORDS_16(p_mem_handle->p_record_base,PRI_ENTRY_NUM_16(curr_state->curr_data, curr_state->curr_pt),curr_state->curr_rec);/*[PRI_ENTRY_NUM_16(curr_state->curr_data, curr_state->curr_pt)]*/
			}
			else
			{
				curr_entry = &CLS_RECORDS_16(p_mem_handle->p_record_base,DEF_ENTRY_NUM_16(curr_state->curr_pt),curr_state->curr_rec);/*DEF_ENTRY_NUM_16(curr_state->curr_pt)][curr_state->curr_rec]*/
			}
		}
		else
		{
			/*Points to the top of stack; */
			curr_entry=&(p_mem_handle->x_old_stack.a_stack[p_mem_handle->x_old_stack.n_top].n_entry_value);
		}
	}
	else
	{
		/*Points to the top of stack; */
		curr_entry=&(p_mem_handle->x_new_stack.a_stack[p_mem_handle->x_new_stack.n_top].n_entry_value);
	}

	if (OPCODE_16(*curr_entry) == AG_CLSB_OPC16_TAG)
		return AG_E_CLS_TAGGED;

	if (((*curr_entry) & TAG_MASK_16) == TAG_MASK_16)
	{ /* Find next available path tag */
		if (find_next_pt_16(curr_state, p_mem_handle) & AG_E_FAIL)
			return AG_E_FAIL;
		/*Update the top entry */
		*curr_entry = (*curr_entry) & MAKE_NR_16(curr_state->next_rec) & MAKE_NPT_16(curr_state->next_pt);
	}

	curr_state->next_rec =(AG_U16) NR_16(*curr_entry);
	curr_state->next_pt = (AG_U16)NPT_16(*curr_entry);

	/*Points to the next default entry */
	next_entry = &CLS_RECORDS_16(p_mem_handle->p_record_base,DEF_ENTRY_NUM_16(curr_state->next_pt),curr_state->next_rec);


	if ((*next_entry == EMPTY_ENTRY_16) ||
		 ((OPCODE_16(*next_entry) == AG_CLSB_OPC16_TAG) && (((*next_entry) & TAG_MASK_16) == 0)))
	{

		n_aux_ent_val=CLS_ALL_ONES & MAKE_OPCODE_16(opc) & MAKE_MASK_16(msk) & MAKE_TAG_16(tag);

		/*update the stack element and push to the stack */
		if(AG_FAILED(n_res=cls_update_and_push((AG_U32)DEF_ENTRY_NUM_16(curr_state->next_pt),
												curr_state->next_pt,
												curr_state->next_rec,
												CLS_REF_COUNT_16(p_mem_handle->p_ref_count,curr_state->next_pt,curr_state->next_rec, p_mem_handle->n_record_limit),
												n_aux_ent_val,
												DEF_TABLE,
												&p_mem_handle->x_new_stack)))
			return n_res;

		/*mark entry  default as occupied - pt use */
		*next_entry = CLS_ALL_ONES & MAKE_OPCODE_16(AG_CLSB_OPC16_TAG) & MAKE_MASK_16(DEF_MSK_16) & MAKE_TAG_16(0);

		CLS_REF_COUNT_16(p_mem_handle->p_ref_count,curr_state->next_pt,curr_state->next_rec, p_mem_handle->n_record_limit)++;
	}
	else
	{
		if ((OPCODE_16(*next_entry) == AG_CLSB_OPC16_TAG) && (((*next_entry) & TAG_MASK_16) != 0))
		{
			if(TG_16(*next_entry)==tag)
			{
				return AG_S_CLS_PATH_EXIST;/*Path already exists. Ambig but OK. */
			}
			else
			{
				return AG_E_CLS_TAGGED; /* Already tagged */
			}
		}
		else
			if (OPCODE_16(*next_entry) != opc)
			{
				if (opc == AG_CLSB_OPC16_TAG)
				{
					curr_state->next_pt = (AG_U16)NPT_16(*next_entry);
					curr_state->next_rec =  (AG_U16)NR_16(*next_entry);
#ifdef CLS_TEST_FULL_TRACE
					fprintf (full_trace, "\t\tEND: add_def_byte_16 (AG_E_CLS_PATH_CONT)\n");
#endif /* CLS_TEST_FULL_TRACE */

					return AG_E_CLS_PATH_CONT;
				}

				return AG_E_CLS_AMBIG;
			}
	}

	curr_state->curr_rec = curr_state->next_rec;
	curr_state->curr_pt = curr_state->next_pt;
	curr_state->curr_table = DEF_TABLE;
	return AG_S_OK;

 }/* of add_def_word_16 */
/*********************************************************************/
/* The following function adds a default byte as the first byte.     */
/*********************************************************************/
AgResult add_def_first_8 (AG_U8 opc , AG_U32 tag , ClsbCurrState *curr_state, AgClsbMem *p_mem_handle)
{
AG_U32 curr_entry_data;
AG_U32 *curr_entry = &curr_entry_data;
AG_U32 n_aux_ent_val;/*Value to be pushed  stack*/

#ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "\n\t\tSTART: add_def_first_8\n");
	fprintf (full_trace, "\t\t\tParameters:\n\t\t\t\topc %d\n\t\t\t\ttag %d\n", opc, tag);
	fprintf (full_trace, "\t\t\t\tcurr_state:\n\t\t\t\t\tb_is_extended %d\n",
		curr_state->b_is_extended);
	fprintf (full_trace, "\t\t\t\t\tcurr_data %d\n\t\t\t\t\tcurr_entry_num %d\n", 
		curr_state->curr_data, curr_state->curr_entry_num);
	fprintf (full_trace, "\t\t\t\t\tcurr_pt %d\n\t\t\t\t\tcurr_rec %d\n\t\t\t\t\tcurr_table %d\n", 
		curr_state->curr_pt, curr_state->curr_rec, curr_state->curr_table);
	fprintf (full_trace, "\t\t\t\t\tdef_mask %x\n\t\t\t\t\tdef_pt %d\n\t\t\t\t\tdef_rec %d\n",
		curr_state->def_mask, curr_state->def_pt, curr_state->def_rec);
	fprintf (full_trace, "\t\t\t\t\tnext_pt %d\n\t\t\t\t\tnext_rec %d\n", 
		curr_state->next_pt, curr_state->next_rec);
	fprintf (full_trace, "\t\t\t\t\tn_num_of_ent_to_copy %d\n\t\t\t\t\tnext_entry_num %d\n", 
		curr_state->n_num_of_ent_to_copy, curr_state->next_entry_num);
#endif /* CLS_TEST_FULL_TRACE */

	if (opc != AG_CLSB_OPC8_TAG)
		tag = CLS_ALL_ONES;

	curr_entry_data = CLS_RECORDS_8_GET(p_mem_handle,curr_state->def_pt,DEF_TABLE,curr_state->def_rec);

	if ((*curr_entry == EMPTY_ENTRY_8) || /*??? can never be empty. node is allocated at create std prg*/
		 ((OPCODE_8(*curr_entry) == AG_CLSB_OPC8_TAG) && (((*curr_entry) & TAG_MASK_8) == 0)))
    { /*default is empty or set as occupied but yet unclassified */

#ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\t\t\t\t\t- current entry (ent %d table %d rec %d val 0x%08x) is empty or not classified (0xFFE00000)  \n",
			curr_state->def_pt,DEF_TABLE,curr_state->def_rec, *curr_entry);
#endif /* CLS_TEST_FULL_TRACE */

		n_aux_ent_val=CLS_ALL_ONES & MAKE_OPCODE_8(opc) & MAKE_TAG_8(tag);

		/*update the stack element and push to the stack*/
		if(AG_FAILED(cls_stack_push_8(curr_state->def_pt,
											   curr_state->def_rec,
											   n_aux_ent_val,
											   DEF_TABLE,
											   &p_mem_handle->x_new_stack)))
		{

#ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\t\tEND: add_def_first_8 (after cls_update_and_push) failed\n");
#endif /* CLS_TEST_FULL_TRACE */

			return AG_E_CLS_FULL;
		}

        /*set as occupied in any case (even though it may already be occupied) */
		*curr_entry = OCCUPIED_ENTRY_8;
        CLS_RECORDS_8_PUT(p_mem_handle,curr_state->def_pt,DEF_TABLE,curr_state->def_rec,*curr_entry);

#ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\t\t\t\t\t- mark entry as occupied (ent %d table %d rec %d val 0x%08x)\n",
			curr_state->def_pt,DEF_TABLE,curr_state->def_rec,
			CLS_RECORDS_8_GET(p_mem_handle,curr_state->def_pt,DEF_TABLE,curr_state->def_rec));
#endif /* CLS_TEST_FULL_TRACE */
	}
	else
	{

		if ((OPCODE_8(*curr_entry) == AG_CLSB_OPC8_TAG) 
			&& (((*curr_entry) & TAG_MASK_8) != 0))
		{
#ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\t\t\t\t\t- default byte and tag\n");
#endif /* CLS_TEST_FULL_TRACE */
			curr_state->curr_rec = curr_state->def_rec;
			curr_state->curr_pt = curr_state->def_pt;
			curr_state->curr_table = DEF_TABLE;
			curr_state->curr_data = 0;

			if(((*curr_entry) & TAG_MASK_8)==tag)
			{

#ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\t\tEND: add_def_first_8 (AG_S_CLS_PATH_EXIST)\n");
#endif /* CLS_TEST_FULL_TRACE */
		/*		curr_state->curr_table = DEF_TABLE; */
				return AG_S_CLS_PATH_EXIST;/*Path already exists, ambiguous but ok. */
			}
			else
			{

#ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\t\tEND: add_def_first_8 (AG_E_CLS_TAGGED)\n");
#endif /* CLS_TEST_FULL_TRACE */

				return AG_E_CLS_TAGGED; /* Already tagged */
			}
		}
		else
		{
#ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\t\t\t\t\t- default byte without tag\n");
#endif /* CLS_TEST_FULL_TRACE */

			if (OPCODE_8(*curr_entry) != opc)
			{
				if (opc == AG_CLSB_OPC8_TAG)
				{
					curr_state->next_pt = (AG_U16)NPT_8(*curr_entry);
					curr_state->next_rec = (AG_U16)NR_8(*curr_entry);
#ifdef CLS_TEST_FULL_TRACE
					fprintf (full_trace, "\t\tEND: add_def_first_8 (AG_E_CLS_PATH_CONT)\n");
#endif /* CLS_TEST_FULL_TRACE */

					return AG_E_CLS_PATH_CONT;
				}
#ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\t\tEND: add_def_first_8 (AG_S_CLS_PATH_EXIST)\n");
#endif /* CLS_TEST_FULL_TRACE */

				return AG_E_CLS_AMBIG;
			}
		}
	}

	curr_state->curr_rec = curr_state->def_rec;
	curr_state->curr_pt = curr_state->def_pt;
	curr_state->curr_table = DEF_TABLE;
	curr_state->curr_data = 0;

#ifdef CLS_TEST_FULL_TRACE

	fprintf (full_trace, "\t\t\t\tcurr_state:\n\t\t\t\t\tb_is_extended %d\n",
		curr_state->b_is_extended);
	fprintf (full_trace, "\t\t\t\t\tcurr_data %d\n\t\t\t\t\tcurr_entry_num %d\n", 
		curr_state->curr_data, curr_state->curr_entry_num);
	fprintf (full_trace, "\t\t\t\t\tcurr_pt %d\n\t\t\t\t\tcurr_rec %d\n\t\t\t\t\tcurr_table %d\n", 
		curr_state->curr_pt, curr_state->curr_rec, curr_state->curr_table);
	fprintf (full_trace, "\t\t\t\t\tdef_mask %x\n\t\t\t\t\tdef_pt %d\n\t\t\t\t\tdef_rec %d\n",
		curr_state->def_mask, curr_state->def_pt, curr_state->def_rec);
	fprintf (full_trace, "\t\t\t\t\tnext_pt %d\n\t\t\t\t\tnext_rec %d\n", 
		curr_state->next_pt, curr_state->next_rec);
	fprintf (full_trace, "\t\t\t\t\tn_num_of_ent_to_copy %d\n\t\t\t\t\tnext_entry_num %d\n", 
		curr_state->n_num_of_ent_to_copy, curr_state->next_entry_num);

	fprintf (full_trace, "\t\tEND: add_def_first_8 (AG_S_OK)\n");
#endif /* CLS_TEST_FULL_TRACE */

	return AG_S_OK;
} /* of add_def_first_8 */

 /*********************************************************
Name: 
    add_def_first_16
Description:
   	Adds the first word  default - 16 mode of work
Parameters:
		opc - opcode.
		tag - classification tag if opcode==TAG_16
		curr_state -describes the actual situation.
Returns AgResult:
    Success: AG_S_OK
	Fail : AG_E_FAIL
 ***********************************************************/

AgResult add_def_first_16( AG_U8 opc ,  AG_U32 tag , AG_U8 msk, ClsbCurrState *curr_state, AgClsbMem *p_mem_handle)
 {
	AG_U32 *curr_entry;
	AgResult n_res;
	AG_U32 n_aux_ent_val;/*Value to be pushed  stack */

	if (opc != AG_CLSB_OPC16_TAG)
	{/*set tag to empty  not leaf */
		tag = CLS_ALL_ONES;
	}
	else
	{/*set mask to 0xf if opc=15 */
		msk=DEF_MSK_16;
	}

	/* The position of the default entry  the 16 bit table is calculated as pos=pt*3+4 */
	curr_entry = &CLS_RECORDS_16(p_mem_handle->p_record_base,DEF_ENTRY_NUM_16(curr_state->def_pt),curr_state->def_rec);

	if ((*curr_entry == EMPTY_ENTRY_16) || /*??? can never be empty. node is allocated at create std prg*/
		 ((OPCODE_16(*curr_entry) == AG_CLSB_OPC16_TAG) && (((*curr_entry) & TAG_MASK_16) == 0)))
	{
		n_aux_ent_val=CLS_ALL_ONES & MAKE_OPCODE_16(opc) & MAKE_MASK_16(msk) & MAKE_TAG_16(tag);

		/*update the stack element and push to the stack */
		if(AG_FAILED(n_res=cls_update_and_push((AG_U32)DEF_ENTRY_NUM_16(curr_state->def_pt),
												curr_state->def_pt,
												curr_state->def_rec,
												CLS_REF_COUNT_16(p_mem_handle->p_ref_count,curr_state->def_pt,curr_state->def_rec, p_mem_handle->n_record_limit),
												n_aux_ent_val,
												DEF_TABLE,
												&p_mem_handle->x_new_stack)))
			return n_res;

		/*mark entry  default as occupied - pt use */
		*curr_entry = CLS_ALL_ONES & MAKE_OPCODE_16(AG_CLSB_OPC16_TAG) & MAKE_MASK_16(DEF_MSK_16) & MAKE_TAG_16(0);
		CLS_REF_COUNT_16(p_mem_handle->p_ref_count,curr_state->def_pt,curr_state->def_rec, p_mem_handle->n_record_limit)++;
	}
	else
	{
		if ((OPCODE_16(*curr_entry) == AG_CLSB_OPC16_TAG) && (TG_16(*curr_entry) != 0))
		{
			if(TG_16(*curr_entry)==tag)
			{
				return AG_S_CLS_PATH_EXIST;/*Path already exists. Ambig but OK. */
			}
			else
			{
				return AG_E_CLS_TAGGED; /* Already tagged */
			}
		}

		else
			if ((OPCODE_16(*curr_entry) != opc)||(MASK_16(*curr_entry)!=msk))
			{
				if (opc == AG_CLSB_OPC16_TAG)
				{
					curr_state->next_pt = (AG_U16)NPT_16(*curr_entry);
					curr_state->next_rec = (AG_U16)NR_16(*curr_entry);
#ifdef CLS_TEST_FULL_TRACE
					fprintf (full_trace, "\t\tEND: add_def_first_16 (AG_E_CLS_PATH_CONT)\n");
#endif /* CLS_TEST_FULL_TRACE */

					return AG_E_CLS_PATH_CONT;
				}

				return AG_E_CLS_AMBIG;
			}
	}

	curr_state->curr_rec = curr_state->def_rec;
	curr_state->curr_pt = curr_state->def_pt;
	curr_state->curr_table = DEF_TABLE;
	return AG_S_OK;

 }/*end of add_def_first_16*/


/*********************************************************************/
/* The following function adds a byte to a mini program. It first    */
/* check to see if the next entry is empty. if it is not empty and	 */
/* does not equal the byte it allocates a new table and copy all the */
/* entries corelating to the default path tag into it.				 */
/*********************************************************************/
AgResult add_byte_8 (AG_U8 opc , AG_U8 br , AG_U32 tag , ClsbCurrState *curr_state, AgClsbMem *p_mem_handle)
{
AG_U32 *curr_entry;
AG_U32 *next_entry;

#ifdef CLS_USE_IO_FUNCTION
AG_U32 curr_entry_data; /*Value of entry*/
AG_U32 *curr_entry_pointer = 0;
AG_U32 curr_entry_offset = 0;

AG_U32 next_entry_data; /*Value of next entry*/

AG_U32 memory_stack; /*0=memory, else stack*/

#endif


AG_U32 n_int_tag=0;
AG_U8  n_skip = 0;
AG_U8  n_aux_pt=(AG_U8)curr_state->next_pt;
AG_U32 n_aux_ent_val;/*//Value to be pushed  stack */


#ifdef CLS_USE_IO_FUNCTION
curr_entry = &curr_entry_data; /* A pointer to the current entry */
next_entry = &next_entry_data; /* A pointer to the next entry */
#endif

#ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "\n\t\tSTART: add_byte_8\n");

	fprintf (full_trace, "\t\t\tParameters:\n\t\t\t\topc %d\n\t\t\t\tbr %d\n\t\t\t\ttag %d\n", opc, br, tag);

	fprintf (full_trace, "\t\t\t\tcurr_state:\n\t\t\t\t\tb_is_extended %d\n",
		curr_state->b_is_extended);
	fprintf (full_trace, "\t\t\t\t\tcurr_data %d\n\t\t\t\t\tcurr_entry_num %d\n", 
		curr_state->curr_data, curr_state->curr_entry_num);
	fprintf (full_trace, "\t\t\t\t\tcurr_pt %d\n\t\t\t\t\tcurr_rec %d\n\t\t\t\t\tcurr_table %d\n", 
		curr_state->curr_pt, curr_state->curr_rec, curr_state->curr_table);
	fprintf (full_trace, "\t\t\t\t\tdef_mask %x\n\t\t\t\t\tdef_pt %d\n\t\t\t\t\tdef_rec %d\n",
		curr_state->def_mask, curr_state->def_pt, curr_state->def_rec);
	fprintf (full_trace, "\t\t\t\t\tnext_pt %d\n\t\t\t\t\tnext_rec %d\n", 
		curr_state->next_pt, curr_state->next_rec);
	fprintf (full_trace, "\t\t\t\t\tn_num_of_ent_to_copy %d\n\t\t\t\t\tnext_entry_num %d\n", 
		curr_state->n_num_of_ent_to_copy, curr_state->next_entry_num);
#endif /* CLS_TEST_FULL_TRACE */

	/*Save the intermediate tag,save the number of bytes to skip and set br to zero */
	if (curr_state->b_is_extended==AG_TRUE)
	{
		if(opc==AG_CLSB_OPC8_INT_TAG)
		{
			n_int_tag=tag;
		}

		n_skip=br;
		br=0;

#ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\t\t\t\t\t- update br 0, skip %d and int. tag %d for ext. opc\n",
			n_skip, n_int_tag);
#endif /* CLS_TEST_FULL_TRACE */
	}

	if (opc != AG_CLSB_OPC8_TAG)
		tag = CLS_ALL_ONES;

	/*if the stack is empty, there is no new entry, point to the record */
	if(cls_stack_state(&p_mem_handle->x_new_stack)==AG_S_CLS_EMPTY)
	{
		if(cls_stack_state(&p_mem_handle->x_old_stack)==AG_S_CLS_EMPTY)
		{
			if (curr_state->curr_table == PRI_TABLE)
			{
				/*Update the current entry num - used by copy entries */
				curr_state->curr_entry_num=(AG_U8)(curr_state->curr_data+curr_state->curr_pt);
                memory_stack = 0;
                curr_entry_offset = CLS_RECORDS_OFFSET_8((AG_U8)curr_state->curr_entry_num,PRI_TABLE,curr_state->curr_rec);
                CLS_RECORDS_8_READ_OFFSET(p_mem_handle,curr_entry_offset,&curr_entry_data);
            }
			else
			{
				/*Update the current entry num - used by copy entries */
				curr_state->curr_entry_num=(AG_U8)(curr_state->curr_pt);
                memory_stack = 0;
                curr_entry_offset = CLS_RECORDS_OFFSET_8((AG_U8)(curr_state->curr_pt),DEF_TABLE,curr_state->curr_rec);
                CLS_RECORDS_8_READ_OFFSET(p_mem_handle,curr_entry_offset,&curr_entry_data);    
			}
		}
		else
		{
			/*Points to the top of stack; */
             memory_stack = 1; /*In stack*/
             curr_entry_pointer = &(p_mem_handle->x_old_stack.a_stack[p_mem_handle->x_old_stack.n_top].n_entry_value);
            curr_entry_data = *(AG_U32 *)(&(p_mem_handle->x_old_stack.a_stack[p_mem_handle->x_old_stack.n_top].n_entry_value));
            #ifdef CLS_TEST_FULL_TRACE
			fprintf (full_trace, "\t\t\t\t\t- old stack is not empty.\n");
			#endif /* CLS_TEST_FULL_TRACE */
		}

#ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\t\t\t\t\t- stack is empty (there is no new entry)\n");
		fprintf (full_trace, "\t\t\t\t\t\tcurrent entry is %08x\n", *curr_entry);
#endif /* CLS_TEST_FULL_TRACE */

	}
	else
	{
		/*Points to the top of stack; */
           memory_stack = 1; /*In stack*/ 
           curr_entry_data = *(AG_U32 *) (&(p_mem_handle->x_new_stack.a_stack[p_mem_handle->x_new_stack.n_top].n_entry_value));
           curr_entry_pointer = &(p_mem_handle->x_new_stack.a_stack[p_mem_handle->x_new_stack.n_top].n_entry_value);
#ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\t\t\t\t\t- new stack is not empty.\n");
#endif /* CLS_TEST_FULL_TRACE */
	}

	if ((*curr_entry & TAG_MASK_8) == TAG_MASK_8) 
	{ /* This is only true if the stack is not empty. curr_entry in this case
      points to the top element of the stack.
      The top element on the stack is the start of a new branch. Need to allocate its
      next record and path tag and set it to the element in the stack. */

#ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\t\t\t\t\t- next entry is not allocated (NPT = 0xFFFF, NR =0x1FFF)\n");
#endif /* CLS_TEST_FULL_TRACE */

		/*find a node for the new byte.*/
		if (find_next_ent_8(br , n_skip, curr_state, p_mem_handle) & AG_E_FAIL)
		{

#ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\t\tEND: add_byte_8 (AG_E_FAIL after find_next_ent_8)\n");
#endif /* CLS_TEST_FULL_TRACE */

			return AG_E_FAIL;
		}

		/*Update auxiliary path tag */
		n_aux_pt=(AG_U8)curr_state->next_pt;

		/*If entry is extended */
		/*Update intermediate tag  default table and cls_is_extended */
		/*Change the pt field to number of bytes to skip */
		if(curr_state->b_is_extended==AG_TRUE) 
		{
			if(opc==AG_CLSB_OPC8_INT_TAG)
			{
               n_aux_ent_val = CLS_RECORDS_8_GET(p_mem_handle,curr_state->next_pt,DEF_TABLE,curr_state->next_rec);
                n_aux_ent_val |= MAKE_TAG_8(n_int_tag);
             
              
#ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\t\t\t\t\t- extended entry. intermidiate tag\n");
#endif /* CLS_TEST_FULL_TRACE */

				/*update the stack element and push to the stack */
				if(AG_FAILED(cls_stack_push_8(curr_state->next_pt,
													   curr_state->next_rec,
													   n_aux_ent_val,
													   DEF_TABLE,
													   &p_mem_handle->x_new_stack)))
				{

#ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\t\tEND: cls_update_and_push (after find_next_ent_8) failed\n");
#endif /* CLS_TEST_FULL_TRACE */

					return AG_E_CLS_FULL;
				}

			}


			/*Set extended bit entry to 1. br = 0 for extended command */
			CLS_IS_EXT(p_mem_handle->p_extend,(AG_U8)(br + curr_state->next_pt), curr_state->next_rec, p_mem_handle->n_record_limit)
				|= BIT_ENTRY_POS((AG_U8)(br + curr_state->next_pt));

			n_aux_pt=n_skip;

#ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\t\t\t\t\t- extended entry. skip.\n");
		fprintf (full_trace, "\t\t\t\t\t- mark entry (ent %d, rec %d) as extended in the ext. entry table\n",
			(br + curr_state->next_pt), curr_state->next_rec);
#endif /* CLS_TEST_FULL_TRACE */

		} /* if extended */

        /*create the entry for the new byte */
		n_aux_ent_val= CLS_ALL_ONES 
					   & MAKE_OPCODE_8(opc) 
					   & MAKE_PT_8(n_aux_pt) 
					   & MAKE_TAG_8(tag);

		/* push new byte on the stack (with its NR and NPT set to 1's (if not a tag)*/
		if(AG_FAILED(cls_stack_push_8((AG_U8)(br+curr_state->next_pt),
													   curr_state->next_rec,
													   n_aux_ent_val,
													   PRI_TABLE,
													   &p_mem_handle->x_new_stack)))
		{

#ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\t\tEND: cls_update_and_push (after find_next_ent_8) failed\n");
#endif /* CLS_TEST_FULL_TRACE */

			/* Reset extended bit */
			CLS_IS_EXT(p_mem_handle->p_extend,(AG_U8)(br + curr_state->next_pt), curr_state->next_rec, p_mem_handle->n_record_limit)
				&= ~BIT_ENTRY_POS((AG_U8)(br + curr_state->next_pt));

			return AG_E_CLS_FULL;
		}

		/* Update the primary entry as pushed_entry */
         {
            
             CLS_RECORDS_8_PUT(p_mem_handle,(AG_U8)(br+curr_state->next_pt),PRI_TABLE,curr_state->next_rec,PUSHED_ENTRY_8);
         }
        
		/* Update the previous entry (which was set without NR and NPT with the allocated
        record and path tag.*/
          if(memory_stack)
          {
              *curr_entry_pointer = *curr_entry_pointer 
              & MAKE_NR_8(curr_state->next_rec) 
              & MAKE_NPT_8(curr_state->next_pt);
          }
          else
          {
              curr_entry_data = curr_entry_data 
               & MAKE_NR_8(curr_state->next_rec) 
               & MAKE_NPT_8(curr_state->next_pt);
               CLS_RECORDS_8_WRITE_OFFSET(p_mem_handle,curr_entry_offset,&curr_entry_data);
          }

		/* Update the pointers */
		curr_state->curr_rec   = curr_state->next_rec;
		curr_state->curr_pt    = curr_state->next_pt;
		curr_state->curr_data  = br;
		curr_state->curr_table = PRI_TABLE;

#ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\t\t\t\t\t- mark primary entry as puched (ent %d table %d rec %d val 0x%08x).\n",
			(br+curr_state->next_pt),PRI_TABLE,curr_state->next_rec,
			CLS_RECORDS_8_GET(p_mem_handle,(AG_U8)(br+curr_state->next_pt),PRI_TABLE,curr_state->next_rec));

		fprintf (full_trace, "\t\t\t\t\t- update NPT and NR in the previous entry (val 0x%08x)\n",
			*curr_entry);
		fprintf (full_trace, "\t\t\t\tcurr_state:\n\t\t\t\t\tb_is_extended %d\n",
			curr_state->b_is_extended);
		fprintf (full_trace, "\t\t\t\t\tcurr_data %d\n\t\t\t\t\tcurr_entry_num %d\n", 
			curr_state->curr_data, curr_state->curr_entry_num);
		fprintf (full_trace, "\t\t\t\t\tcurr_pt %d\n\t\t\t\t\tcurr_rec %d\n\t\t\t\t\tcurr_table %d\n", 
			curr_state->curr_pt, curr_state->curr_rec, curr_state->curr_table);
		fprintf (full_trace, "\t\t\t\t\tdef_mask %x\n\t\t\t\t\tdef_pt %d\n\t\t\t\t\tdef_rec %d\n",
			curr_state->def_mask, curr_state->def_pt, curr_state->def_rec);
		fprintf (full_trace, "\t\t\t\t\tnext_pt %d\n\t\t\t\t\tnext_rec %d\n", 
			curr_state->next_pt, curr_state->next_rec);
		fprintf (full_trace, "\t\t\t\t\tn_num_of_ent_to_copy %d\n\t\t\t\t\tnext_entry_num %d\n", 
			curr_state->n_num_of_ent_to_copy, curr_state->next_entry_num);

		fprintf (full_trace, "\t\tEND: add_byte_8 (AG_S_OK)\n");
#endif /* CLS_TEST_FULL_TRACE */
		return AG_S_OK;
	}
	else
	{ /* Next record is already allocated */
        
		curr_state->next_pt =(AG_U16) NPT_8 (*curr_entry);
		curr_state->next_rec = (AG_U16)NR_8 (*curr_entry);

         next_entry_data = CLS_RECORDS_8_GET(p_mem_handle,(AG_U8)(br + curr_state->next_pt),PRI_TABLE,curr_state->next_rec);
		if ( *next_entry == EMPTY_ENTRY_8) /* Entry is vacant */
		{  

#ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\t\t\t\t\t- Next entry is allocated and empty (NPT = %02x, NR =%04x)\n",
			NPT_8 (*curr_entry), NR_8 (*curr_entry));
#endif /* CLS_TEST_FULL_TRACE */

			/*Update entry fields */
			n_aux_ent_val = CLS_ALL_ONES 
							& MAKE_PT_8((AG_U8)curr_state->next_pt) 
							& MAKE_OPCODE_8(opc) 
							& MAKE_TAG_8(tag);

			/*update the stack element and push to the stack */
			if(AG_FAILED(cls_stack_push_8((AG_U8)(br + curr_state->next_pt),
														   curr_state->next_rec,
														   n_aux_ent_val,
														   PRI_TABLE,
														   &p_mem_handle->x_new_stack)))
			{

#ifdef CLS_TEST_FULL_TRACE
				fprintf (full_trace, "\t\tEND: add_byte_8 (after cls_update_and_push) failed\n");
#endif /* CLS_TEST_FULL_TRACE */

				return AG_E_CLS_FULL;
			}


			/*Update the primary entry as pushed_entry */
            *next_entry=PUSHED_ENTRY_8;
            CLS_RECORDS_8_PUT(p_mem_handle,(AG_U8)(br + curr_state->next_pt),PRI_TABLE,curr_state->next_rec,next_entry_data);

			/* Update the pointers */
			curr_state->curr_rec = curr_state->next_rec;
			curr_state->curr_pt = curr_state->next_pt;
			curr_state->curr_data = br;
			curr_state->curr_table = PRI_TABLE;

#ifdef CLS_TEST_FULL_TRACE
			fprintf (full_trace, "\t\t\t\t\t- mark primary entry as puched (ent %d table %d rec %d val 0x%08x).\n",
				(br + curr_state->next_pt),PRI_TABLE,curr_state->next_rec,
				CLS_RECORDS_8_GET(p_mem_handle,(AG_U8)(br + curr_state->next_pt),PRI_TABLE,curr_state->next_rec));
			fprintf (full_trace, "\t\t\t\tcurr_state:\n\t\t\t\t\tb_is_extended %d\n",
				curr_state->b_is_extended);
			fprintf (full_trace, "\t\t\t\t\tcurr_data %d\n\t\t\t\t\tcurr_entry_num %d\n", 
				curr_state->curr_data, curr_state->curr_entry_num);
			fprintf (full_trace, "\t\t\t\t\tcurr_pt %d\n\t\t\t\t\tcurr_rec %d\n\t\t\t\t\tcurr_table %d\n", 
				curr_state->curr_pt, curr_state->curr_rec, curr_state->curr_table);
			fprintf (full_trace, "\t\t\t\t\tdef_mask %x\n\t\t\t\t\tdef_pt %d\n\t\t\t\t\tdef_rec %d\n",
				curr_state->def_mask, curr_state->def_pt, curr_state->def_rec);
			fprintf (full_trace, "\t\t\t\t\tnext_pt %d\n\t\t\t\t\tnext_rec %d\n", 
				curr_state->next_pt, curr_state->next_rec);
			fprintf (full_trace, "\t\t\t\t\tn_num_of_ent_to_copy %d\n\t\t\t\t\tnext_entry_num %d\n", 
				curr_state->n_num_of_ent_to_copy, curr_state->next_entry_num);

			fprintf (full_trace, "\t\tEND: add_byte_8 (AG_S_OK)\n");
#endif /* CLS_TEST_FULL_TRACE */

			return AG_S_OK;
		}
		else
		{ /* Entry is not vacant and is not extended */

/*			if ((curr_state->next_pt != PT_8 (*next_entry)) */
/*				||(IS_EXTENDED_8(p_mem_handle->p_extend,curr_state->next_pt,curr_state->next_rec,p_mem_handle->n_record_limit))) */
			if ((curr_state->next_pt != PT_8 (*next_entry))
				&&(!IS_EXTENDED_8(p_mem_handle->p_extend,curr_state->next_pt,curr_state->next_rec,p_mem_handle->n_record_limit)))
			{ /* Entry is occupied by a different path tag
				 by a skip value */
#ifdef CLS_TEST_FULL_TRACE
			fprintf (full_trace, "\t\t\t\t\t- Next entry is allocated, but occupied by different PT.\n");
#endif /* CLS_TEST_FULL_TRACE */

				/*Update the next_entry_num */
				curr_state->next_entry_num=(AG_U8)(curr_state->next_pt+br);

				if (cls_search_new_entry_and_allocate_8(curr_entry, curr_state, p_mem_handle) & AG_E_FAIL)
				{

#ifdef CLS_TEST_FULL_TRACE
					fprintf (full_trace, "\t\tEND: add_byte_8 (AG_E_FAIL after cls_search_new_entry_and_allocate_8)\n");
#endif /* CLS_TEST_FULL_TRACE */

					return AG_E_FAIL;
				}

				if (copy_entries_8(curr_entry, curr_state, p_mem_handle) & AG_E_FAIL)
				{

#ifdef CLS_TEST_FULL_TRACE
					fprintf (full_trace, "\t\tEND: add_byte_8 (AG_E_FAIL after copy_entries_8)\n");
#endif /* CLS_TEST_FULL_TRACE */

					return AG_E_FAIL;
				}

				/*Update the entry */
				n_aux_ent_val= CLS_ALL_ONES
							   & MAKE_PT_8((AG_U8)curr_state->next_pt) 
							   & MAKE_OPCODE_8(opc) 
							   & MAKE_TAG_8(tag);

				/*update the stack element and push to the stack */
				if(AG_FAILED(cls_stack_push_8((AG_U8)(br+curr_state->next_pt),
															   curr_state->next_rec,
															   n_aux_ent_val,
															   PRI_TABLE,
															   &p_mem_handle->x_new_stack)))
				{

#ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\t\tEND: add_first_byte_8 (after cls_update_and_push) failed\n");
#endif /* CLS_TEST_FULL_TRACE */

					return AG_E_CLS_FULL;
				}

				/*Update the primary entry as pushed_entry */
                 CLS_RECORDS_8_PUT(p_mem_handle,(AG_U8)(br + curr_state->next_pt),PRI_TABLE,curr_state->next_rec,PUSHED_ENTRY_8);
				/*Update the pointers */
				curr_state->curr_pt = curr_state->next_pt;
				curr_state->curr_rec = curr_state->next_rec;
				curr_state->curr_data = br;
				curr_state->curr_table = PRI_TABLE;

#ifdef CLS_TEST_FULL_TRACE
			fprintf (full_trace, "\t\t\t\t\t- mark primary entry as puched (ent %d table %d rec %d val 0x%08x).\n",
				(br + curr_state->next_pt),PRI_TABLE,curr_state->next_rec,
				CLS_RECORDS_8_GET(p_mem_handle,(AG_U8)(br + curr_state->next_pt),PRI_TABLE,curr_state->next_rec));
			fprintf (full_trace, "\t\t\t\tcurr_state:\n\t\t\t\t\tb_is_extended %d\n",
				curr_state->b_is_extended);
			fprintf (full_trace, "\t\t\t\t\tcurr_data %d\n\t\t\t\t\tcurr_entry_num %d\n", 
				curr_state->curr_data, curr_state->curr_entry_num);
			fprintf (full_trace, "\t\t\t\t\tcurr_pt %d\n\t\t\t\t\tcurr_rec %d\n\t\t\t\t\tcurr_table %d\n", 
				curr_state->curr_pt, curr_state->curr_rec, curr_state->curr_table);
			fprintf (full_trace, "\t\t\t\t\tdef_mask %x\n\t\t\t\t\tdef_pt %d\n\t\t\t\t\tdef_rec %d\n",
				curr_state->def_mask, curr_state->def_pt, curr_state->def_rec);
			fprintf (full_trace, "\t\t\t\t\tnext_pt %d\n\t\t\t\t\tnext_rec %d\n", 
				curr_state->next_pt, curr_state->next_rec);
			fprintf (full_trace, "\t\t\t\t\tn_num_of_ent_to_copy %d\n\t\t\t\t\tnext_entry_num %d\n", 
				curr_state->n_num_of_ent_to_copy, curr_state->next_entry_num);

			fprintf (full_trace, "\t\tEND: add_byte_8 (AG_S_OK)\n");
#endif /* CLS_TEST_FULL_TRACE */

				return AG_S_OK;
			}
			else
			{ /* Entry occupied by the same PT*/

#ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\t\t\t\t\t- entry occupied by the same PT\n");
#endif /* CLS_TEST_FULL_TRACE */

				if (OPCODE_8(*next_entry) == AG_CLSB_OPC8_TAG)
				{

			  		curr_state->curr_rec = curr_state->next_rec;
			  		curr_state->curr_pt = curr_state->next_pt;
			  		curr_state->curr_data = br;
					curr_state->curr_table = PRI_TABLE;

					if(((*next_entry) & TAG_MASK_8)==tag)
					{

#ifdef CLS_TEST_FULL_TRACE
					fprintf (full_trace, "\t\tEND: add_byte_8 (AG_S_CLS_PATH_EXIST)\n");
#endif /* CLS_TEST_FULL_TRACE */
				/*		curr_state->curr_table = PRI_TABLE; */
						return AG_S_CLS_PATH_EXIST;/*Path already exists, ambiguous but ok. */
					}
					else
					{

#ifdef CLS_TEST_FULL_TRACE
					fprintf (full_trace, "\t\tEND: add_byte_8 (AG_E_CLS_TAGGED)\n");
#endif /* CLS_TEST_FULL_TRACE */

						return AG_E_CLS_TAGGED; /* Already tagged */
					}
				}

 				if (opc == OPCODE_8(*next_entry))
			  	{
			  		curr_state->curr_rec = curr_state->next_rec;
			  		curr_state->curr_pt = curr_state->next_pt;
			  		curr_state->curr_data = br;
					curr_state->curr_table = PRI_TABLE;

#ifdef CLS_TEST_FULL_TRACE
			fprintf (full_trace, "\t\t\t\tcurr_state:\n\t\t\t\t\tb_is_extended %d\n",
				curr_state->b_is_extended);
			fprintf (full_trace, "\t\t\t\t\tcurr_data %d\n\t\t\t\t\tcurr_entry_num %d\n", 
				curr_state->curr_data, curr_state->curr_entry_num);
			fprintf (full_trace, "\t\t\t\t\tcurr_pt %d\n\t\t\t\t\tcurr_rec %d\n\t\t\t\t\tcurr_table %d\n", 
				curr_state->curr_pt, curr_state->curr_rec, curr_state->curr_table);
			fprintf (full_trace, "\t\t\t\t\tdef_mask %x\n\t\t\t\t\tdef_pt %d\n\t\t\t\t\tdef_rec %d\n",
				curr_state->def_mask, curr_state->def_pt, curr_state->def_rec);
			fprintf (full_trace, "\t\t\t\t\tnext_pt %d\n\t\t\t\t\tnext_rec %d\n", 
				curr_state->next_pt, curr_state->next_rec);
			fprintf (full_trace, "\t\t\t\t\tn_num_of_ent_to_copy %d\n\t\t\t\t\tnext_entry_num %d\n", 
				curr_state->n_num_of_ent_to_copy, curr_state->next_entry_num);

			fprintf (full_trace, "\t\tEND: add_byte_8 (AG_S_OK)\n");
#endif /* CLS_TEST_FULL_TRACE */

			  		return AG_S_OK;
			  	}

				if (opc == AG_CLSB_OPC8_TAG)
				{
					curr_state->next_pt = (AG_U16)NPT_8(*next_entry);
					curr_state->next_rec =  (AG_U16)NR_8(*next_entry);
#ifdef CLS_TEST_FULL_TRACE
					fprintf (full_trace, "\t\tEND: add_byte_8 (AG_E_CLS_PATH_CONT)\n");
#endif /* CLS_TEST_FULL_TRACE */
					return AG_E_CLS_PATH_CONT;
				}

#ifdef CLS_TEST_FULL_TRACE
				fprintf (full_trace, "\t\tEND: add_byte_8 (AG_E_CLS_AMBIG)\n");
#endif /* CLS_TEST_FULL_TRACE */

				return AG_E_CLS_AMBIG;
			} /* of else - path tag equals */
		} /* of else - entry not vacant */
	} /* of else - next record allocated */
} /* of add_byte_8 */


/*********************************************************
Name: 
    add_word_16
Description:
   	Adds an word  16 mode of work
Parameters:
			opc - opcode
			wr - word read
			msk - mask field
			tag -  case of opcode==TAG_16
			curr_state - describes the actual state.
Returns AgResult:
    Success: AG_S_OK
	Fail : AG_E_FAIL
 ***********************************************************/
AgResult add_word_16( AG_U8 opc ,  AG_U16 wr ,  AG_U8 msk,  AG_U32 tag , ClsbCurrState *curr_state, AgClsbMem *p_mem_handle)
{
AG_U32 *curr_entry;
AG_U32 *next_entry;
AG_U32 n_int_tag=0;
AG_U8  n_skip = 0;
AG_U16  n_aux_pt=curr_state->next_pt;
AG_U32 n_aux_entry_num;
AgResult n_res;
AG_U32 n_aux_ent_val;


	/*MBEXT - 25/10/2000 - Save the intermediate tag,save the  */
	/*number of bytes to skip and set wr to zero */
	if (curr_state->b_is_extended==AG_TRUE)
	{
		if(opc==AG_CLSB_OPC16_INT_TAG)
			n_int_tag=tag;
		n_skip=(AG_U8)wr;
		wr=0;
	}

	if (opc != AG_CLSB_OPC16_TAG)
	{/*set tag to empty  not leaf */
		tag = CLS_ALL_ONES;/*MBFF- 31/12/2000 */
	}
	else
	{/*set mask to 0xf if opc=15 */
		msk=DEF_MSK_16;
	}

	/*MB16 - if the stack is empty, there is no new entry, point to the record */
	if(cls_stack_state(&p_mem_handle->x_new_stack)==AG_S_CLS_EMPTY)
	{
		if(cls_stack_state(&p_mem_handle->x_old_stack)==AG_S_CLS_EMPTY)
		{
			if (curr_state->curr_table == PRI_TABLE)
			{/*MBdyn Update the entry num for copy entries */
				curr_state->curr_entry_num=PRI_ENTRY_NUM_16(curr_state->curr_data,curr_state->curr_pt);
			}
			else
			{/*MBdyn Update the entry num for copy entries */
				curr_state->curr_entry_num=DEF_ENTRY_NUM_16(curr_state->curr_pt);
			}
			curr_entry = &CLS_RECORDS_16(p_mem_handle->p_record_base,curr_state->curr_entry_num,curr_state->curr_rec);
		}
		else
		{
			/*Points to the top of stack; */
			curr_entry=&(p_mem_handle->x_old_stack.a_stack[p_mem_handle->x_old_stack.n_top].n_entry_value);
		}
	}
	else
	{
		/*Points to the top of stack; */
		curr_entry=&(p_mem_handle->x_new_stack.a_stack[p_mem_handle->x_new_stack.n_top].n_entry_value);
	}

	/*entry is pointed  the stack */
	if ((*curr_entry & TAG_MASK_16) == TAG_MASK_16) /* Is the next byte occupied */
	{ /* Next entry is empty */
		if (find_next_ent_16(wr , curr_state, p_mem_handle) & AG_E_FAIL) /* Find the next empty entry */
			return (AG_E_FAIL);

		/*MBEXT - 25/10/2000 - Update auxiliary path tag */
		n_aux_pt=curr_state->next_pt;

		/*MBEXT 24/10/2000 If entry is extended */
		/*Update intermediate tag  default entry and cls_is_extended */
		/*Change the pt field to number of bytes to skip */
		if(curr_state->b_is_extended==AG_TRUE) 
		{
			if(opc==AG_CLSB_OPC16_INT_TAG)
			{
				n_aux_ent_val=CLS_RECORDS_16(p_mem_handle->p_record_base,DEF_ENTRY_NUM_16(curr_state->next_pt),curr_state->next_rec)
					| MAKE_TAG_16(n_int_tag);

				/*update the stack element and push to the stack */
				if(AG_FAILED(n_res=cls_update_and_push((AG_U32)DEF_ENTRY_NUM_16(curr_state->next_pt),
														curr_state->next_pt,
														curr_state->next_rec,
														CLS_REF_COUNT_16(p_mem_handle->p_ref_count,curr_state->next_pt, curr_state->next_rec, p_mem_handle->n_record_limit),
														n_aux_ent_val,
														DEF_TABLE,
														&p_mem_handle->x_new_stack)))
					return n_res;
			}

			/*Set bit entry to 1 */
			CLS_IS_EXT(p_mem_handle->p_extend,PRI_ENTRY_NUM_16(wr,curr_state->next_pt)/2,curr_state->next_rec,p_mem_handle->n_record_limit)
				|= BIT_ENTRY_POS(PRI_ENTRY_NUM_16(wr,curr_state->next_pt)/2);


			n_aux_pt=n_skip;
		}

		n_aux_ent_val=CLS_ALL_ONES & MAKE_OPCODE_16(opc) & MAKE_MASK_16(msk) & MAKE_TAG_16(tag);
		n_aux_entry_num=PRI_ENTRY_NUM_16(wr,curr_state->next_pt);
		/*update the stack element and push to the stack */
		if(AG_FAILED(n_res=cls_update_and_push(n_aux_entry_num,
											   n_aux_pt,
											   curr_state->next_rec,
											   CLS_REF_COUNT_16(p_mem_handle->p_ref_count,curr_state->next_pt,curr_state->next_rec, p_mem_handle->n_record_limit),
											   n_aux_ent_val,
											   PRI_TABLE,
											   &p_mem_handle->x_new_stack)))
		{
			CLS_IS_EXT(p_mem_handle->p_extend,PRI_ENTRY_NUM_16(wr,curr_state->next_pt)/2,curr_state->next_rec,p_mem_handle->n_record_limit)
				&= ~BIT_ENTRY_POS(PRI_ENTRY_NUM_16(wr,curr_state->next_pt)/2);

			return n_res;
		}

		/*Update the primary entry as pushed_entry */
		CLS_RECORDS_16(p_mem_handle->p_record_base,n_aux_entry_num,curr_state->next_rec)
			=PUSHED_ENTRY_16;

		/* Update the entry fields */
		*curr_entry = *curr_entry & MAKE_NR_16(curr_state->next_rec) & MAKE_NPT_16(curr_state->next_pt);

		/* Update the pointers */
		curr_state->curr_rec = curr_state->next_rec;
		curr_state->curr_pt = curr_state->next_pt;
		curr_state->curr_data = wr;
		curr_state->curr_table = PRI_TABLE;

		/*MBCOMP - 11/12/2000 - mark the entry  the image bit table */
		CLS_REC_IMAGE_16(p_mem_handle->p_record_image,BIT_TABLE_ENTRY(n_aux_entry_num/2),curr_state->curr_rec,p_mem_handle->n_record_limit)
			|= BIT_ENTRY_POS(n_aux_entry_num/2);

		CLS_REF_COUNT_16(p_mem_handle->p_ref_count,curr_state->curr_pt,curr_state->curr_rec, p_mem_handle->n_record_limit)++; /* Update reference count */

		return (AG_S_OK);
	}
	else
	{ /* Next record is already allocated */
		curr_state->next_pt = (AG_U16)NPT_16 (*curr_entry);
		curr_state->next_rec = (AG_U16)NR_16 (*curr_entry);

		/*MBEXT - 25/10/2000 - Update auxiliary path tag */
		/*n_aux_pt=curr_state->next_pt; */

		/*Calculate the auxiliary entry numb */
		/*The entry number of primary entry  the table is calculated as follows: */
		/*	(wr+pt)/2)*4+(wr+pt)%2+1 - odd or even entry. */
		n_aux_entry_num=PRI_ENTRY_NUM_16(wr,curr_state->next_pt);
		next_entry = &CLS_RECORDS_16(p_mem_handle->p_record_base,n_aux_entry_num,curr_state->next_rec);

		if ( *next_entry == EMPTY_ENTRY_16) /* Entry is vacant */
		{  
			/*Update the entry */
			n_aux_ent_val=CLS_ALL_ONES & MAKE_OPCODE_16(opc) & MAKE_MASK_16(msk) & MAKE_TAG_16(tag);

			/*update the stack element and push to the stack */
			if(AG_FAILED(n_res=cls_update_and_push(n_aux_entry_num,
												   curr_state->next_pt,
												   curr_state->next_rec,
												   CLS_REF_COUNT_16(p_mem_handle->p_ref_count,curr_state->next_pt,curr_state->next_rec, p_mem_handle->n_record_limit),
												   n_aux_ent_val,
												   PRI_TABLE,
												   &p_mem_handle->x_new_stack)))
				return n_res;

			/*Update the primary entry as pushed_entry */
			*next_entry=PUSHED_ENTRY_16;

			/* Update the pointers */
			curr_state->curr_rec = curr_state->next_rec;
			curr_state->curr_pt = curr_state->next_pt;
			curr_state->curr_data = wr;
			curr_state->curr_table = PRI_TABLE;

			/*MBCOMP - 11/12/2000 - mark the entry  the image bit table */
			CLS_REC_IMAGE_16(p_mem_handle->p_record_image,BIT_TABLE_ENTRY(n_aux_entry_num/2),curr_state->curr_rec,p_mem_handle->n_record_limit)
				|= BIT_ENTRY_POS(n_aux_entry_num/2);

			CLS_REF_COUNT_16(p_mem_handle->p_ref_count,curr_state->curr_pt,curr_state->curr_rec, p_mem_handle->n_record_limit)++; /* Update reference count */
		
			return (AG_S_OK);
		}
		else
		{ /* Entry is not vacant and is not extended */

			/*calculate the PT - entry may be even or odd */
			if (((n_aux_entry_num-1)%2)==0)
				n_aux_pt=(AG_U16)PT0_16(CLS_RECORDS_16(p_mem_handle->p_record_base,n_aux_entry_num-1,curr_state->next_rec));/*[n_aux_entry_num-1][curr_state->next_rec]*/
			else
				n_aux_pt=(AG_U16)PT1_16(CLS_RECORDS_16(p_mem_handle->p_record_base,n_aux_entry_num-2,curr_state->next_rec));/*[n_aux_entry_num-2][curr_state->next_rec]*/

			if (curr_state->next_pt != n_aux_pt )
			{ /* Entry is occupied by a different path tag */

				/* update the next_entry_num */
				curr_state->next_entry_num=PRI_ENTRY_NUM_16(wr,curr_state->next_pt);

				if (cls_search_new_entry_and_allocate_16(curr_entry, curr_state, p_mem_handle) & AG_E_FAIL) /*Search for empty entries and allocate a new table */
					return (AG_E_FAIL);


				if (copy_entries_16(curr_entry, curr_state, p_mem_handle) & AG_E_FAIL)
					return AG_E_FAIL;


				/*Update the entry */
				n_aux_ent_val=CLS_ALL_ONES & MAKE_OPCODE_16(opc) & MAKE_MASK_16(msk)& MAKE_TAG_16(tag);
				n_aux_entry_num=PRI_ENTRY_NUM_16(wr,curr_state->next_pt);

				/*update the stack element and push to the stack */
				if(AG_FAILED(n_res=cls_update_and_push(n_aux_entry_num,
													   curr_state->next_pt,
													   curr_state->next_rec,
													   CLS_REF_COUNT_16(p_mem_handle->p_ref_count,curr_state->next_pt,curr_state->next_rec, p_mem_handle->n_record_limit),
													   n_aux_ent_val,
													   PRI_TABLE,
													   &p_mem_handle->x_new_stack)))
					return n_res;

				/*Update the primary entry as pushed_entry */
				CLS_RECORDS_16(p_mem_handle->p_record_base,n_aux_entry_num,curr_state->next_rec)
					=PUSHED_ENTRY_16;

				/*Update the pointers */
				curr_state->curr_pt = curr_state->next_pt;
				curr_state->curr_rec = curr_state->next_rec;
				curr_state->curr_data = wr;
				curr_state->curr_table = PRI_TABLE;

				/*mark the entry  the image bit table */
				CLS_REC_IMAGE_16(p_mem_handle->p_record_image,BIT_TABLE_ENTRY(n_aux_entry_num/2),curr_state->curr_rec,p_mem_handle->n_record_limit)
					|=BIT_ENTRY_POS(n_aux_entry_num/2);

				CLS_REF_COUNT_16(p_mem_handle->p_ref_count,curr_state->curr_pt,curr_state->curr_rec, p_mem_handle->n_record_limit)++;

				return AG_S_OK;
			}
			else
			{ 
				/* Path tag equals */
				if (OPCODE_16(*next_entry) == AG_CLSB_OPC16_TAG)
				{
					if(TG_16(*next_entry)==tag)
					{
						return AG_S_CLS_PATH_EXIST;/*Path already exists. Ambig but OK. */
					}
					else
					{
						return AG_E_CLS_TAGGED; /* Already tagged */
					}

				}

 				if ((opc == OPCODE_16(*next_entry))&&(msk==MASK_16(*next_entry)))
			  	{
			  		curr_state->curr_rec = curr_state->next_rec;
			  		curr_state->curr_pt = curr_state->next_pt;
			  		curr_state->curr_data = wr;
					curr_state->curr_table = PRI_TABLE;
			  		return (AG_S_OK);
			  	}

				if (opc == AG_CLSB_OPC16_TAG)
				{
					curr_state->next_pt = (AG_U16)NPT_16(*next_entry);
					curr_state->next_rec =  (AG_U16)NR_16(*next_entry);
					return AG_E_CLS_PATH_CONT;
				}

				return AG_E_CLS_AMBIG;
			} /* of else - path tag equals */
		} /* of else - entry not vacant */
	} /* of else - next record allocated */
} /* of add_word_16 */


/**********************************************************/
/* The following functions looks for the next available   */
/* entry  the record set and marks the use of path tags */
/**********************************************************/
AgResult find_next_ent_8 ( AG_U8 br ,  AG_U8 n_skip,  ClsbCurrState *curr_state, AgClsbMem *p_mem_handle)
{
AG_U32 entry_index = 0; /*must be larger then AG_U8*/
AG_U32 rec_index = p_mem_handle->n_low_limit_record;

#ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "\n\t\t\tSTART: find_next_ent_8\n");
	fprintf (full_trace, "\t\t\t\tParameters:\n\t\t\t\t\tbr %d\n\t\t\t\t\tn_skip %d\n", br, n_skip);
	fprintf (full_trace, "\t\t\t\t\tcurr_state:\n\t\t\t\t\t\tb_is_extended %d\n",
		curr_state->b_is_extended);
	fprintf (full_trace, "\t\t\t\t\t\tcurr_data %d\n\t\t\t\t\t\tcurr_entry_num %d\n", 
		curr_state->curr_data, curr_state->curr_entry_num);
	fprintf (full_trace, "\t\t\t\t\t\tcurr_pt %d\n\t\t\t\t\t\tcurr_rec %d\n\t\t\t\t\t\tcurr_table %d\n", 
		curr_state->curr_pt, curr_state->curr_rec, curr_state->curr_table);
	fprintf (full_trace, "\t\t\t\t\t\tdef_mask %x\n\t\t\t\t\t\tdef_pt %d\n\t\t\t\t\t\tdef_rec %d\n",
		curr_state->def_mask, curr_state->def_pt, curr_state->def_rec);
	fprintf (full_trace, "\t\t\t\t\t\tnext_pt %d\n\t\t\t\t\t\tnext_rec %d\n", 
		curr_state->next_pt, curr_state->next_rec);
	fprintf (full_trace, "\t\t\t\t\t\tn_num_of_ent_to_copy %d\n\t\t\t\t\t\tnext_entry_num %d\n", 
		curr_state->n_num_of_ent_to_copy, curr_state->next_entry_num);
#endif /* CLS_TEST_FULL_TRACE */

	/* Search for a record where the pt equal to the skip is not used
	   used  extended */
	if(curr_state->b_is_extended)
	{
		while( CLS_RECORDS_8_GET(p_mem_handle,n_skip,DEF_TABLE,rec_index) != EMPTY_ENTRY_8     
            && CLS_RECORDS_8_GET(p_mem_handle,n_skip,DEF_TABLE,rec_index) != SKIPPED_ENTRY_8
            && rec_index < p_mem_handle->n_record_limit)
		{
			rec_index++;
		}
	}

	while (rec_index < p_mem_handle->n_record_limit)
	{
		/* Do not search for empty place in the static record */
		if (p_mem_handle->p_static_rec_image[BIT_TABLE_ENTRY(rec_index)] & BIT_ENTRY_POS(rec_index))
		{
            entry_index = MAX_ENTRY_8;
		}
        else
        { /*if not static record then look for first empty entry.*/
  		    while ((CLS_RECORDS_8_GET(p_mem_handle,entry_index,PRI_TABLE,rec_index) !=EMPTY_ENTRY_8) 
			    && (entry_index < MAX_ENTRY_8))
		    {
			    entry_index++;
		    }
        }

		if (entry_index < MAX_ENTRY_8)
		{ 
			/* Calculate the next path tag */
			curr_state->next_pt = (AG_U8)(entry_index - br);

			/* The extended default must not use the same entry as 
			   the skip default since the skip default may be used
			   by many extended entries */
			if(curr_state->b_is_extended && curr_state->next_pt == n_skip)
			{
				entry_index++;
			}
			/* Path tag FF is used by the program*/
			else if ((curr_state->next_pt != 0xFF) 
				&& (CLS_RECORDS_8_GET(p_mem_handle,curr_state->next_pt,DEF_TABLE,rec_index) 
					== EMPTY_ENTRY_8 ))	/* Check to see if vacant */
			{
				/* Mark as occupied */
				CLS_RECORDS_8_PUT(p_mem_handle,curr_state->next_pt,DEF_TABLE,rec_index, 
					OCCUPIED_ENTRY_8);

				/* Mark the skip  case of extended*/
				if(curr_state->b_is_extended)
				{
					/* Mark entry as busy by skip */
					CLS_RECORDS_8_PUT(p_mem_handle,n_skip,DEF_TABLE,rec_index, 
						SKIPPED_ENTRY_8);
                    inc_ref_count(p_mem_handle, rec_index, n_skip);
				}

				curr_state->next_rec = (AG_U16) rec_index;
				if (p_mem_handle->n_last_record < curr_state->next_rec) /* Update last record we used */
				{
					p_mem_handle->n_last_record = curr_state->next_rec;
				}

#ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "\n\t\t\t\t\t\t- Finded empty entry (ent %d rec %d).\n", 
		entry_index,  rec_index);

	fprintf (full_trace, "\t\t\t\tParameters:\n\t\t\t\t\tbr %d\n\t\t\t\t\tn_skip %d\n", br, n_skip);
	fprintf (full_trace, "\t\t\t\t\tcurr_state:\n\t\t\t\t\t\tb_is_extended %d\n",
		curr_state->b_is_extended);
	fprintf (full_trace, "\t\t\t\t\t\tcurr_data %d\n\t\t\t\t\t\tcurr_entry_num %d\n", 
		curr_state->curr_data, curr_state->curr_entry_num);
	fprintf (full_trace, "\t\t\t\t\t\tcurr_pt %d\n\t\t\t\t\t\tcurr_rec %d\n\t\t\t\t\t\tcurr_table %d\n", 
		curr_state->curr_pt, curr_state->curr_rec, curr_state->curr_table);
	fprintf (full_trace, "\t\t\t\t\t\tdef_mask %x\n\t\t\t\t\t\tdef_pt %d\n\t\t\t\t\t\tdef_rec %d\n",
		curr_state->def_mask, curr_state->def_pt, curr_state->def_rec);
	fprintf (full_trace, "\t\t\t\t\t\tnext_pt %d\n\t\t\t\t\t\tnext_rec %d\n", 
		curr_state->next_pt, curr_state->next_rec);
	fprintf (full_trace, "\t\t\t\t\t\tn_num_of_ent_to_copy %d\n\t\t\t\t\t\tnext_entry_num %d\n", 
		curr_state->n_num_of_ent_to_copy, curr_state->next_entry_num);

	fprintf (full_trace, "\t\t\t\t\t\tn_last_record %d\n", p_mem_handle->n_last_record);

	fprintf (full_trace, "\n\t\t\tEND: find_next_ent_8 (AG_S_OK)\n");
#endif /* CLS_TEST_FULL_TRACE */

				return AG_S_OK; 
			} /* of if */
			else
			{
				entry_index++;
			}
		}
		else
		{
			entry_index = 0;

			rec_index++;
			/* Search for a record where the pt equal to the skip is not used
			   case of extended command.  */
			if(curr_state->b_is_extended)
			{
				while((CLS_RECORDS_8_GET(p_mem_handle,n_skip,DEF_TABLE,rec_index) != EMPTY_ENTRY_8 )
					&& (CLS_RECORDS_8_GET(p_mem_handle,n_skip,DEF_TABLE,rec_index) != SKIPPED_ENTRY_8)
					&& (rec_index < p_mem_handle->n_record_limit))
				{
					rec_index++;
				}

			}/*if is extended*/
	   	} /* of else */
	} /* of while */

#ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "\n\t\t\tEND: find_next_ent_8 (AG_E_CLS_FULL)\n");
#endif /* CLS_TEST_FULL_TRACE */
	return AG_E_CLS_FULL;
} /* of find_next_ent_8 */

 /*********************************************************
Name: 
    find_next_ent_16
Description:
   	Finds the next empty entry
	Marks the default table as occupied
Parameters:

Returns AgResult:
    Success: AG_S_OK
	Fail : E_CLS_FULL if table is full
 ***********************************************************/
AgResult find_next_ent_16(AG_U16 wr, ClsbCurrState *curr_state, AgClsbMem *p_mem_handle)
{
AG_U32 entry_index = 0; /*must be more then AG_U16*/
AG_U32 rec_index = p_mem_handle->n_low_limit_record;

	while (rec_index < p_mem_handle->n_record_limit)
	{
		if (p_mem_handle->p_static_rec_image[BIT_TABLE_ENTRY(rec_index)] & BIT_ENTRY_POS(rec_index))
		{
			entry_index = MAX_PRI_ENT_16;
		}
        else
        {
		    while ((CLS_RECORDS_16(p_mem_handle->p_record_base,PRI_ENTRY_NUM_16(0,entry_index),rec_index) != EMPTY_ENTRY_16)
			    && (entry_index < MAX_PRI_ENT_16))
			    entry_index++;
        }

		if (entry_index < MAX_PRI_ENT_16)
		{ 

				curr_state->next_pt =(AG_U16)PRI_PT_16(wr,PRI_ENTRY_NUM_16(0,entry_index));		/* Calculate the next path tag  */

	   		if ((curr_state->next_pt < 0x7FFF)&&
				(CLS_RECORDS_16(p_mem_handle->p_record_base,DEF_ENTRY_NUM_16(curr_state->next_pt),rec_index) == EMPTY_ENTRY_16) )/* Check to see if vacant*/
			{
				/* Mark as occupied  */
				CLS_RECORDS_16(p_mem_handle->p_record_base,DEF_ENTRY_NUM_16(curr_state->next_pt),rec_index) 
					= OCCUPIED_ENTRY_16;

				curr_state->next_rec = (AG_U16)rec_index;
				if (p_mem_handle->n_last_record < curr_state->next_rec) /* Update last record we used */
					p_mem_handle->n_last_record = curr_state->next_rec;
				return AG_S_OK; 
			} /* of if */
			else
				entry_index++;
		}
		else
		{
			entry_index = 0;
			rec_index++;

	   	} /* of else */
	} /* of while */
	return AG_E_CLS_FULL;

 }/*of find_next_ent_16*/

AgResult find_empty_rec_8 (ClsbCurrState *curr_state, AgClsbMem *p_mem_handle)
{
	AG_U32 entry_index = 0;
	AG_U32 rec_index = p_mem_handle->n_low_limit_record;

#ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "\n\t\t\tSTART: find_empty_rec_8\n");
	fprintf (full_trace, "\t\t\t\tParameters:\n\t\t\t\t\tn_last_record %d\n", 
		p_mem_handle->n_last_record);

	fprintf (full_trace, "\t\t\t\t\tcurr_state:\n\t\t\t\t\t\tb_is_extended %d\n",
		curr_state->b_is_extended);
	fprintf (full_trace, "\t\t\t\t\t\tcurr_data %d\n\t\t\t\t\t\tcurr_entry_num %d\n", 
		curr_state->curr_data, curr_state->curr_entry_num);
	fprintf (full_trace, "\t\t\t\t\t\tcurr_pt %d\n\t\t\t\t\t\tcurr_rec %d\n\t\t\t\t\t\tcurr_table %d\n", 
		curr_state->curr_pt, curr_state->curr_rec, curr_state->curr_table);
	fprintf (full_trace, "\t\t\t\t\t\tdef_mask %x\n\t\t\t\t\t\tdef_pt %d\n\t\t\t\t\t\tdef_rec %d\n",
		curr_state->def_mask, curr_state->def_pt, curr_state->def_rec);
	fprintf (full_trace, "\t\t\t\t\t\tnext_pt %d\n\t\t\t\t\t\tnext_rec %d\n", 
		curr_state->next_pt, curr_state->next_rec);
	fprintf (full_trace, "\t\t\t\t\t\tn_num_of_ent_to_copy %d\n\t\t\t\t\t\tnext_entry_num %d\n", 
		curr_state->n_num_of_ent_to_copy, curr_state->next_entry_num);

#endif /* CLS_TEST_FULL_TRACE */

	/* If there is records which was not used before, 
		take first of them */
	if (p_mem_handle->n_last_record < p_mem_handle->n_record_limit - 1)
	{

		p_mem_handle->n_last_record++;
		curr_state->next_pt = 0;
		curr_state->next_rec = (AG_U16)p_mem_handle->n_last_record;

		CLS_RECORDS_8_PUT(p_mem_handle,curr_state->next_pt,DEF_TABLE,curr_state->next_rec,
			OCCUPIED_ENTRY_8);

        #ifdef CLS_TEST_FULL_TRACE
	    fprintf (full_trace, "\n\t\t\t\t\t\t- finded empty record %d.\n", curr_state->next_rec);
	    fprintf (full_trace, "\t\t\t\t\t\t\t- last record %d\n", p_mem_handle->n_last_record);

	    fprintf (full_trace, "\n\t\t\tEND: find_empty_rec_8 (AG_S_OK)\n");
        #endif /* CLS_TEST_FULL_TRACE */

		return AG_S_OK;
	}

	/* For static prg we need to find empty record */
	while (rec_index < p_mem_handle->n_record_limit)
	{
		entry_index = 0;

		/* To find empty record we need to search in DEF table, because of 
			"unclassified default" does not mark in REC_IMAGE array */
		while ((CLS_RECORDS_8_GET(p_mem_handle,entry_index,DEF_TABLE,rec_index) == EMPTY_ENTRY_8) 
			    && (entry_index < MAX_ENTRY_8))
		{
			entry_index++;
		}

		if (entry_index == MAX_ENTRY_8)
		{
			curr_state->next_pt = 0;
			curr_state->next_rec = (AG_U16)rec_index;
			break;
		}

		rec_index++;
	}

    if (rec_index < p_mem_handle->n_record_limit)
    { /* set default */

		CLS_RECORDS_8_PUT(p_mem_handle,curr_state->next_pt,DEF_TABLE,curr_state->next_rec,
			OCCUPIED_ENTRY_8);
		{

#ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "\n\t\t\t\t\t\t- finded empty record %d.\n", curr_state->next_rec);
	fprintf (full_trace, "\t\t\t\t\t\t\t- last record %d\n", p_mem_handle->n_last_record);

	fprintf (full_trace, "\n\t\t\tEND: find_empty_rec_8 (AG_S_OK)\n");
#endif /* CLS_TEST_FULL_TRACE */

			return AG_S_OK;

		}
    }
    else
    {
#ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\n\t\t\tEND: find_empty_rec_8 (AG_E_FAIL)\n");
#endif /* CLS_TEST_FULL_TRACE */

		return AG_E_FAIL;
    }
}

AgResult find_empty_rec_16 (ClsbCurrState *curr_state, AgClsbMem *p_mem_handle)
{
	AG_U32 entry_index = 0;
	AG_U32 rec_index = p_mem_handle->n_low_limit_record;

	/* If there is records which was not used before, 
		take first of them */
	if (p_mem_handle->n_last_record < p_mem_handle->n_record_limit - 1)
	{
			p_mem_handle->n_last_record++;
			curr_state->next_pt = 0;
			curr_state->next_rec = (AG_U16)p_mem_handle->n_last_record;

            CLS_RECORDS_16(p_mem_handle->p_record_base,DEF_ENTRY_NUM_16(curr_state->next_pt),rec_index) = 
			    OCCUPIED_ENTRY_16;

		return AG_S_OK;
	}

	/* For static prg we need to find empty record */
	while (rec_index < p_mem_handle->n_record_limit)
	{
		entry_index = 0;

		/* To find empty record we need to search in DEF table, because of 
			"unclassified default" does not mark in REC_IMAGE array */
		while ((CLS_RECORDS_16(p_mem_handle->p_record_base,DEF_ENTRY_NUM_16(entry_index),rec_index) == EMPTY_ENTRY_16) 
			    && (entry_index < MAX_DEF_ENT_16))
		{
			entry_index++;
		}

		if (entry_index == MAX_DEF_ENT_16)
		{
			curr_state->next_pt = 0;
			curr_state->next_rec = (AG_U16)rec_index;
			break;
		}

		rec_index++;
	}

    if (rec_index < p_mem_handle->n_record_limit)
    { /* mark as occupied */
		CLS_RECORDS_16(p_mem_handle->p_record_base,DEF_ENTRY_NUM_16(curr_state->next_pt),rec_index) = 
			OCCUPIED_ENTRY_16;
        return AG_S_OK;
    }
    else
    {
	    return AG_E_FAIL;
    }
}

/**********************************************************************************/
/* The following function copies all entries relevant to the current path tag and */
/* current record into a new record pointed by next record and next path tag      */
/**********************************************************************************/
AgResult copy_entries_8 (AG_U32 *curr_entry , 
						 ClsbCurrState *curr_state, 
						 AgClsbMem *p_mem_handle)
{
AG_U32 entry_index = 0;
AG_U16 from_pt;
AG_U16 from_rec;
AG_U32 n_root_value = 0; /*the previous entry value */
AG_U32 n_aux_ent_value;

#ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "\n\t\t\tSTART: copy_entries_8\n");
	fprintf (full_trace, "\t\t\t\tParameters:\n\t\t\t\t\tcurrent entry address 0x%08x\n", curr_entry);

	fprintf (full_trace, "\t\t\t\tcurr_state:\n\t\t\t\t\tb_is_extended %d\n",
		curr_state->b_is_extended);
	fprintf (full_trace, "\t\t\t\t\tcurr_data %d\n\t\t\t\t\tcurr_entry_num %d\n", 
		curr_state->curr_data, curr_state->curr_entry_num);
	fprintf (full_trace, "\t\t\t\t\tcurr_pt %d\n\t\t\t\t\tcurr_rec %d\n\t\t\t\t\tcurr_table %d\n", 
		curr_state->curr_pt, curr_state->curr_rec, curr_state->curr_table);
	fprintf (full_trace, "\t\t\t\t\tdef_mask %x\n\t\t\t\tdef_pt %d\n\t\t\t\t\tdef_rec %d\n",
		curr_state->def_mask, curr_state->def_pt, curr_state->def_rec);
	fprintf (full_trace, "\t\t\t\t\tnext_pt %d\n\t\t\t\t\tnext_rec %d\n", 
		curr_state->next_pt, curr_state->next_rec);
	fprintf (full_trace, "\t\t\t\t\tn_num_of_ent_to_copy %d\n\t\t\t\t\tnext_entry_num %d\n", 
		curr_state->n_num_of_ent_to_copy, curr_state->next_entry_num);

	fprintf (full_trace, "\t\t\t\tmem_handle:\n\t\t\t\t\tn_last_record %d\n\t\t\t\t\tn_update_size %d\n",
		p_mem_handle->n_last_record, p_mem_handle->n_update_size);
	fprintf (full_trace, "\t\t\t\t\tb_leaves %d\n\t\t\t\t\tb_root %d\n", 
		p_mem_handle->x_node_update.b_leaves, p_mem_handle->x_node_update.b_root);
	fprintf (full_trace, "\t\t\t\t\tn_new_pt %d\n\t\t\t\t\tn_new_rec %d\n", 
		p_mem_handle->x_node_update.n_new_pt, p_mem_handle->x_node_update.n_new_rec);
	fprintf (full_trace, "\t\t\t\t\tn_old_pt %d\n\t\t\t\t\tn_old_rec %d\n", 
		p_mem_handle->x_node_update.n_old_pt, p_mem_handle->x_node_update.n_old_rec);
#endif /* CLS_TEST_FULL_TRACE */

	if (curr_entry != NULL)
	{
		from_pt =(AG_U16) NPT_8(*curr_entry);
		from_rec =(AG_U16) NR_8(*curr_entry);
		n_root_value = *curr_entry | TAG_MASK_8; /* Erase old pointers to next record */
		/* Points to new entry */
		n_root_value = n_root_value 
					   & MAKE_NR_8(curr_state->next_rec) 
					   & MAKE_NPT_8(curr_state->next_pt);

		p_mem_handle->x_node_update.b_root = AG_FALSE;

	}
	else
	{
		from_pt = curr_state->def_pt;
		from_rec = curr_state->def_rec;

		curr_state->def_pt = curr_state->next_pt;
		curr_state->def_rec = curr_state->next_rec;
		p_mem_handle->x_node_update.b_root = AG_TRUE;

	}
	p_mem_handle->x_node_update.n_old_pt = from_pt;
	p_mem_handle->x_node_update.n_old_rec = from_rec;
	p_mem_handle->x_node_update.n_new_pt = curr_state->next_pt;
	p_mem_handle->x_node_update.n_new_rec = curr_state->next_rec;

    /*push an empty entry for the defualt of the path tag to be released by the write.*/
	if(AG_FAILED(cls_stack_push_8(from_pt,
										   from_rec,
										   EMPTY_ENTRY_8,
										   DEF_TABLE,
										   &p_mem_handle->x_old_stack)))
	{

        #ifdef CLS_TEST_FULL_TRACE
    	fprintf (full_trace, "\t\t\tEND: copy_entries_8 (after cls_update_and_push) failed\n");
        #endif /* CLS_TEST_FULL_TRACE */

		return AG_E_CLS_FULL;
	}

	if(curr_entry!=NULL)/*check if is not the mini program root */
	{
		AG_U16 n_entry_or_pt;

		if (curr_state->curr_table == DEF_TABLE)
		{
			n_entry_or_pt = curr_state->curr_pt;

		}
		else
		{
			n_entry_or_pt = (AG_U16)curr_state->curr_entry_num;

		}

		/* Update the entry that leads to the copied node */
		if(AG_FAILED(cls_stack_push_8((AG_U8)n_entry_or_pt/*(curr_state->curr_entry_num)*/,
												curr_state->curr_rec,
												n_root_value,
												curr_state->curr_table,
												&p_mem_handle->x_old_stack)))
		{

            #ifdef CLS_TEST_FULL_TRACE
    		fprintf (full_trace, "\t\t\tEND: copy_entries_8 (after cls_update_and_push) failed\n");
            #endif /* CLS_TEST_FULL_TRACE */

			return AG_E_CLS_FULL;
		}
	}


	/*push to stack the new entries - found before by search new entry and allocate*/
	for(entry_index=0;entry_index<(curr_state->n_num_of_ent_to_copy);entry_index++)
	{
        /*create entry value from original entry to copy by replacing old path tag with new one*/
        n_aux_ent_value = (CLS_RECORDS_8_GET(p_mem_handle,(AG_U8)(p_mem_handle->p_entry_to_copy[entry_index]),PRI_TABLE,from_rec) |(~MAKE_PT_8(0x00))) & MAKE_PT_8(curr_state->next_pt);

        /* push the new entries */
		if(AG_FAILED(cls_stack_push_8((AG_U8)(p_mem_handle->p_entry_to_copy[entry_index]-from_pt+curr_state->next_pt),
												curr_state->next_rec,
												n_aux_ent_value,
												PRI_TABLE,
												&p_mem_handle->x_new_stack)))
		{
#ifdef CLS_TEST_FULL_TRACE
			fprintf (full_trace, "\t\t\tEND: copy_entries_8 (after cls_update_and_push) failed\n");
#endif /* CLS_TEST_FULL_TRACE */

			return AG_E_CLS_FULL;
		}

		/* mark the entry as pushed*/
		CLS_RECORDS_8_PUT(p_mem_handle,(AG_U8)(p_mem_handle->p_entry_to_copy[entry_index]-from_pt+curr_state->next_pt),PRI_TABLE,curr_state->next_rec,
			PUSHED_ENTRY_8);

#ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\t\t\t\t\t\t- mark entry (ent %d table %d rec %d val %d) as pushed\n",
			(p_mem_handle->p_entry_to_copy[entry_index]-from_pt+curr_state->next_pt),
			PRI_TABLE, curr_state->next_rec,
			CLS_RECORDS_8_GET(p_mem_handle,(AG_U8)(p_mem_handle->p_entry_to_copy[entry_index]-from_pt+curr_state->next_pt),PRI_TABLE,curr_state->next_rec));

		fprintf (full_trace, "\t\t\t\t\t\t\t- mark entry (ent %d rec %d) in the image bit table\n",
			p_mem_handle->p_entry_to_copy[entry_index]-from_pt+curr_state->next_pt, curr_state->next_rec);
#endif /* CLS_TEST_FULL_TRACE */

	}


	/*no need to push if just occupied since the next statement sets the new allocated
	default to occupied in any case.*/
	if (CLS_RECORDS_8_GET(p_mem_handle,from_pt,DEF_TABLE,from_rec) 
		!= OCCUPIED_ENTRY_8)
	{
		/* push the default entry  to be copied to the new default location.*/
		if(AG_FAILED(cls_stack_push_8(curr_state->next_pt,
											   curr_state->next_rec,
											   CLS_RECORDS_8_GET(p_mem_handle,from_pt,DEF_TABLE,from_rec),
											   DEF_TABLE,
											   &p_mem_handle->x_new_stack)))
		{

#ifdef CLS_TEST_FULL_TRACE
			fprintf (full_trace, "\t\t\tEND: copy_entries_8 (after cls_update_and_push) failed\n");
#endif /* CLS_TEST_FULL_TRACE */

			return AG_E_CLS_FULL;
		}
	}

	/* mark the default entry of the new location as occupied (similar to pushed ???) */
	CLS_RECORDS_8_PUT(p_mem_handle,curr_state->next_pt,DEF_TABLE,curr_state->next_rec,
		OCCUPIED_ENTRY_8);

	p_mem_handle->n_update_size++;

#ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\t\t\t\t\t\t- occupy entry (ent %d table %d rec %d val %d).\n",
			curr_state->next_pt,DEF_TABLE,curr_state->next_rec,
			CLS_RECORDS_8_GET(p_mem_handle,curr_state->next_pt,DEF_TABLE,curr_state->next_rec));

		fprintf (full_trace, "\t\t\t\t\t\t- update mem_handle structure:\n");
		fprintf (full_trace, "\t\t\t\t\t\t\tn_last_record %d\n\t\t\t\t\t\t\tn_update_size %d\n",
			p_mem_handle->n_last_record, p_mem_handle->n_update_size);
		fprintf (full_trace, "\t\t\t\t\t\t\tb_leaves %d\n\t\t\t\t\t\t\tb_root %d\n", 
			p_mem_handle->x_node_update.b_leaves, p_mem_handle->x_node_update.b_root);
		fprintf (full_trace, "\t\t\t\t\t\t\tn_new_pt %d\n\t\t\t\t\t\t\tn_new_rec %d\n", 
			p_mem_handle->x_node_update.n_new_pt, p_mem_handle->x_node_update.n_new_rec);
		fprintf (full_trace, "\t\t\t\t\t\t\tn_old_pt %d\n\t\t\t\t\t\t\tn_old_rec %d\n", 
			p_mem_handle->x_node_update.n_old_pt, p_mem_handle->x_node_update.n_old_rec);

				fprintf (full_trace, "\t\t\tEND: copy_entries_8 (AG_S_OK)\n");
#endif /* CLS_TEST_FULL_TRACE */

	return AG_S_OK;
} /* of copy_entries_8 */
/*********************************************************
Name: 
    copy_entries_16
Description:
   	Move entries with same path were there is no empty place 
   	 the record with the same path tag.
Parameters:
	curr_entry - entry the forces the copy entry
	curr_state - the actual state
Returns AgResult:
    Success: AG_S_OK
	Fail : AG_E_FAIL
 ***********************************************************/
 AgResult copy_entries_16(AG_U32 *curr_entry, ClsbCurrState *curr_state, AgClsbMem *p_mem_handle)
{
AgResult n_res;
AG_U32 entry_index = 0;
AG_U16 from_pt;/*previous pt */
AG_U16 from_rec;/*previous record  */
AG_U32 n_root_value = 0; /*the previous entry value */
AG_U32 n_aux_entry_num;/*for calculation of 16 bit table */

	if (curr_entry != NULL)
	{
		from_pt =(AG_U16) NPT_16(*curr_entry);
		from_rec =(AG_U16) NR_16(*curr_entry);
		/* Points to curr entry */
		n_root_value = *curr_entry | TAG_MASK_16; /* Erase old pointers to next record */
		n_root_value = n_root_value & MAKE_NR_16(curr_state->next_rec) & MAKE_NPT_16(curr_state->next_pt);/*Write new next record and next path tag */
		p_mem_handle->x_node_update.b_root = AG_FALSE;

	}
	else
	{
		from_pt = curr_state->def_pt;
		from_rec = curr_state->def_rec;

		curr_state->def_pt = curr_state->next_pt;	/* Set default path tag to be the new one */
		curr_state->def_rec = curr_state->next_rec;	/* Set default record to be the new one */
		p_mem_handle->x_node_update.b_root = AG_TRUE;

	}

	p_mem_handle->x_node_update.n_old_pt = from_pt;
	p_mem_handle->x_node_update.n_old_rec = from_rec;
	p_mem_handle->x_node_update.n_new_pt = curr_state->next_pt;
	p_mem_handle->x_node_update.n_new_rec = curr_state->next_rec;

	/*write to stack empty default entry of previous PT */
	/*update the stack element and push to the stack */
	if(AG_FAILED(n_res=cls_update_and_push(DEF_ENTRY_NUM_16(from_pt),
											   from_pt,
											   from_rec,
											   CLS_REF_COUNT_16(p_mem_handle->p_ref_count,from_pt,from_rec,p_mem_handle->n_record_limit),
											   EMPTY_ENTRY_16,
											   DEF_TABLE,
											   &p_mem_handle->x_old_stack)))
		return n_res;


	/*write to stack the root entry */
	/*update the stack element and push to the stack */
	if(curr_entry!=NULL)/*check if is not the mini program root */
	{
		if(AG_FAILED(n_res=cls_update_and_push(curr_state->curr_entry_num,
											   curr_state->curr_pt,
											   curr_state->curr_rec,
											   CLS_REF_COUNT_16(p_mem_handle->p_ref_count,curr_state->curr_pt,curr_state->curr_rec, p_mem_handle->n_record_limit),
											   n_root_value,
											   curr_state->curr_table,
											   &p_mem_handle->x_old_stack)))
				return n_res;
	}

	/*write to stack the new entries found before  search and allocate */
	for(entry_index=0;entry_index<(curr_state->n_num_of_ent_to_copy);entry_index++)
	{
		n_aux_entry_num=PRI_ENTRY_NUM_16((AG_U16)PRI_WR_16(from_pt,p_mem_handle->p_entry_to_copy[entry_index]),curr_state->next_pt);

		/* write the new entries with new PT */
		/*update the stack element and push to the stack */
		if(AG_FAILED(n_res=cls_update_and_push(n_aux_entry_num,
											   curr_state->next_pt,
											   curr_state->next_rec,
											   CLS_REF_COUNT_16(p_mem_handle->p_ref_count,from_pt,from_rec, p_mem_handle->n_record_limit),
											   CLS_RECORDS_16(p_mem_handle->p_record_base,p_mem_handle->p_entry_to_copy[entry_index],from_rec),
											   PRI_TABLE,
											   &p_mem_handle->x_new_stack)))
				return n_res;

		/*MBBUG21 - 22/01/2001 - mark the entry as pushed */
		CLS_RECORDS_16(p_mem_handle->p_record_base,n_aux_entry_num,curr_state->next_rec)
			=PUSHED_ENTRY_16;

		/*MBCOMP - 11/12/2000 - mark the entry  the image bit table */
		CLS_REC_IMAGE_16(p_mem_handle->p_record_image,BIT_TABLE_ENTRY(n_aux_entry_num/2),curr_state->next_rec,p_mem_handle->n_record_limit)
			=CLS_REC_IMAGE_16(p_mem_handle->p_record_image,BIT_TABLE_ENTRY(n_aux_entry_num/2),curr_state->next_rec,p_mem_handle->n_record_limit)
				|BIT_ENTRY_POS(n_aux_entry_num/2);
	
	}

	if (CLS_RECORDS_16(p_mem_handle->p_record_base,DEF_ENTRY_NUM_16(curr_state->next_pt),curr_state->next_rec)
		!= (CLS_ALL_ONES & MAKE_OPCODE_16(AG_CLSB_OPC16_TAG) & MAKE_MASK_16(0xF) & MAKE_TAG_16(0)))
	{ 
		/* Mark the new path tag and add the default entry */
		/*update the stack element and push to the stack */
		if(AG_FAILED(n_res=cls_update_and_push((AG_U32)DEF_ENTRY_NUM_16(curr_state->next_pt),
												curr_state->next_pt,
												curr_state->next_rec,
												CLS_REF_COUNT_16(p_mem_handle->p_ref_count,from_pt,from_rec, p_mem_handle->n_record_limit),
												CLS_RECORDS_16(p_mem_handle->p_record_base,DEF_ENTRY_NUM_16(from_pt),from_rec),
												DEF_TABLE,
												&p_mem_handle->x_new_stack)))
				return n_res;
	}

	/*Update the default entry*/
	CLS_RECORDS_16(p_mem_handle->p_record_base,DEF_ENTRY_NUM_16(curr_state->next_pt),curr_state->next_rec)
		= CLS_ALL_ONES & MAKE_OPCODE_16(AG_CLSB_OPC16_TAG) & MAKE_MASK_16(0xF) & MAKE_TAG_16(0);

	/* I can't set this entry as empty since it can be used by
	another entry from the path to be added. After the stack will erase it.
*/
	/*Update the ref_count*/
	CLS_REF_COUNT_16(p_mem_handle->p_ref_count,curr_state->next_pt,curr_state->next_rec, p_mem_handle->n_record_limit) 
		= CLS_REF_COUNT_16(p_mem_handle->p_ref_count,from_pt,from_rec, p_mem_handle->n_record_limit);
	CLS_REF_COUNT_16(p_mem_handle->p_ref_count,from_pt,from_rec, p_mem_handle->n_record_limit) = 0;

	p_mem_handle->n_update_size++;

	return AG_S_OK;
} /* of copy_entries_16 */

 /*********************************************************
Name: 
   add_path_8
Description:
   	Adds a path  the classification tree
Parameters:

Returns AgResult:
    Success: AG_S_OK
	Fail : AG_E_FAIL - called functions return error
		   E_CLS_OPEN - the path was not tagged
		   E_CLS_INVALID_OPC -opcode not valid - extended cmd
		   E_OUT_OF_BOUNDS- the buffers are not consistent		   		   
 ***********************************************************/
AgResult add_path_8 (AG_U8 *buf, AG_U8 *ctl_buf ,AG_U32 buf_len, 
				   AG_U32 *tag_buf,AG_U32 n_taglen,StdPrg *p_stdprg , ClsbCurrState *curr_state)
{


int cb = 1;
AG_U8 n_cls_mask_entry;
AG_U8 n_aux_opc;
AG_U32 n_aux_tag;
AgResult n_res=AG_S_OK;
AG_U32 n_tag_buf_entry=0;

#ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "\n\tSTART: add_path_8\n");
	fprintf (full_trace, "\t\tParameters:\n\t\t\tRoot:\n");
	fprintf (full_trace, "\t\t\t\tdef_pt %d\n\t\t\t\tdef_rec %d\n\t\t\t\tdef_opc %d\n", 
		p_stdprg->x_root.def_pt, p_stdprg->x_root.def_rec, p_stdprg->x_root.n_cls_def_opc);

#endif /* CLS_TEST_FULL_TRACE */

	p_stdprg->p_mem_handle->n_update_size = 0;

	curr_state->def_rec = p_stdprg->x_root.def_rec;
	curr_state->def_pt = p_stdprg->x_root.def_pt;

	reset_records(curr_state);

    #ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "\t\t\tcurr_state:\n\t\t\t\tb_is_extended %d\n",
		curr_state->b_is_extended);
	fprintf (full_trace, "\t\t\t\tcurr_data %d\n\t\t\t\tcurr_entry_num %d\n", 
		curr_state->curr_data, curr_state->curr_entry_num);
	fprintf (full_trace, "\t\t\t\tcurr_pt %d\n\t\t\t\tcurr_rec %d\n\t\t\t\tcurr_table %d\n", 
		curr_state->curr_pt, curr_state->curr_rec, curr_state->curr_table);
	fprintf (full_trace, "\t\t\t\tdef_mask %x\n\t\t\t\tdef_pt %d\n\t\t\t\tdef_rec %d\n",
		curr_state->def_mask, curr_state->def_pt, curr_state->def_rec);
	fprintf (full_trace, "\t\t\t\tnext_pt %d\n\t\t\t\tnext_rec %d\n", 
		curr_state->next_pt, curr_state->next_rec);
	fprintf (full_trace, "\t\t\t\tn_num_of_ent_to_copy %d\n\t\t\t\tnext_entry_num %d\n", 
		curr_state->n_num_of_ent_to_copy, curr_state->next_entry_num);
    #endif /* CLS_TEST_FULL_TRACE */

#ifndef NO_VALIDATION
	/* Verify if TAG >= FIRST_FREE_TAG */
	if((tag_buf[n_taglen-1])<AG_CLSB_FIRST_FREE_TAG)
	{

        #ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\tEND: add_path_8 (AG_E_CLS_INVALID_TAG)\n");
        #endif /* CLS_TEST_FULL_TRACE */

		return AG_E_CLS_INVALID_TAG;
	}

	/* Verify paths */
	if (ctl_buf[buf_len-1] == AG_CLSB_DEFAULT_VAL)
	{
		if(buf[buf_len-1] != AG_CLSB_OPC8_TAG)
		{

            #ifdef CLS_TEST_FULL_TRACE
			fprintf (full_trace, "\tEND: add_path_8 (AG_E_CLS_OPEN)\n");
            #endif /* CLS_TEST_FULL_TRACE */

			return AG_E_CLS_OPEN;
		}

	}
	else
	{
		if(ctl_buf[buf_len-1] != AG_CLSB_OPC8_TAG)
		{

            #ifdef CLS_TEST_FULL_TRACE
			fprintf (full_trace, "\tEND: add_path_8 (AG_E_CLS_OPEN)\n");
            #endif /* CLS_TEST_FULL_TRACE */

			return AG_E_CLS_OPEN;
		}
	}

    /*First opcode cannot be 5 - if is extended must be AG_CLSB_OPC_EXT_CMD */
	if ((ctl_buf[0]==AG_CLSB_OPC8_INT_TAG)||
        ((buf[0]==AG_CLSB_OPC8_INT_TAG) && (ctl_buf[0] == AG_CLSB_DEFAULT_VAL)))
	{

        #ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\tEND: add_path_8 (AG_E_CLS_INVALID_OPC)\n");
        #endif /* CLS_TEST_FULL_TRACE */

		return AG_E_CLS_INVALID_OPC;
    }

    /*Increment tag buf for the last tag - for validation purpose. */
	if(buf_len==1)
		n_tag_buf_entry++;
#endif /*#ifndef NO_VALIDATION*/

    if (ctl_buf[0] == AG_CLSB_DEFAULT_VAL)
	{
		/* If is first entry of extended command-AG_CLSB_OPC_EXT_CMD */
		if (buf[0]== AG_CLSB_OPC_EXT_CMD) 
			n_aux_opc=AG_CLSB_OPC8_EXT_CMD;
		else 
			n_aux_opc=(AG_U8)buf[0];/*MB16 - 19/11/2000 - casting added */

		/*Calls the function with aux */
        n_res=add_def_first_8(n_aux_opc, tag_buf[n_taglen-1],
                              curr_state, p_stdprg->p_mem_handle);
		if (AG_FAILED(n_res))
		{
            #ifdef CLS_TEST_FULL_TRACE
			fprintf (full_trace, "\tEND: add_path_8 (%08x after add_def_first_8)\n", n_res);
            #endif /* CLS_TEST_FULL_TRACE */

			return n_res;
		}
	}
	else
	{

		/* Verify if is first entry of extended command */
		if (ctl_buf[0]== AG_CLSB_OPC_EXT_CMD) 
			n_aux_opc=AG_CLSB_OPC8_EXT_CMD;
		else 
			n_aux_opc=ctl_buf[0];

        n_res=add_first_byte_8 (n_aux_opc,
            (AG_U8)(buf[0] & aClsMaskOpc8[(int)p_stdprg->x_root.n_cls_def_opc]),
            tag_buf[n_taglen-1], curr_state, p_stdprg->p_mem_handle);
		if (AG_FAILED(n_res))
		{ 

            #ifdef CLS_TEST_FULL_TRACE
			fprintf (full_trace, "\tEND: add_path_8 (%08x after add_first_byte_8)\n", n_res);
            #endif /* CLS_TEST_FULL_TRACE */

			return n_res;
		}
	}

	buf_len--;

	while (buf_len > 0)
	{
		/* Reset the b_is_extended will be set to AG_TRUE if leaf is extended */
		curr_state->b_is_extended= AG_FALSE;

		if (ctl_buf[cb] == AG_CLSB_DEFAULT_VAL)
		{
			/* If previous entry was extended this entry can not be the default table. */
			if((ctl_buf[cb-1]==AG_CLSB_OPC_EXT_CMD) ||
               ((ctl_buf[cb-1]== AG_CLSB_DEFAULT_VAL) && 
                (buf[cb-1]==AG_CLSB_OPC_EXT_CMD)))
			{

                #ifdef CLS_TEST_FULL_TRACE
				fprintf (full_trace, "\tEND: add_path_8 (AG_E_CLS_INVALID_OPC)\n");
                #endif /* CLS_TEST_FULL_TRACE */

				return AG_E_CLS_INVALID_OPC;
			}
			/* Verify if is first entry of extended command */
			if (buf[cb]==AG_CLSB_OPC_EXT_CMD) 
				n_aux_opc=AG_CLSB_OPC8_EXT_CMD;
			else 
				n_aux_opc=(AG_U8)buf[cb];/*MB16 - 20/11/2000 -casting added */

            /* If opc==7 update n_tag_buf_entry  */
			if(buf[cb]==AG_CLSB_OPC8_TAG)
				n_tag_buf_entry++;

			/* Calls the function with aux */
			if ((n_res=add_def_byte_8(n_aux_opc,tag_buf[n_taglen-1],curr_state, p_stdprg->p_mem_handle)) & AG_E_FAIL)
			{

                #ifdef CLS_TEST_FULL_TRACE
				fprintf (full_trace, "\tEND: add_path_8 (%08x after add_def_byte_8)\n", n_res);
                #endif /* CLS_TEST_FULL_TRACE */

				return n_res;
			}
		}/*end of if default */
		else/* masked byte as function of opcode */
		{
			/* Validation: if two consecutive first entries of 
			   extended command was entered - 'e''e'-return */
			if(((ctl_buf[cb-1]==AG_CLSB_OPC_EXT_CMD)||
               ((ctl_buf[cb-1]==AG_CLSB_DEFAULT_VAL) && 
                (buf[cb-1]==AG_CLSB_OPC_EXT_CMD))) &&
				(ctl_buf[cb]==AG_CLSB_OPC_EXT_CMD))
			{

                #ifdef CLS_TEST_FULL_TRACE
				fprintf (full_trace, "\tEND: add_path_8 (AG_E_CLS_INVALID_OPC)\n");
                #endif /* CLS_TEST_FULL_TRACE */

				return AG_E_CLS_INVALID_OPC;
			}

			/* Verify if is first entry of extended command */
			if (ctl_buf[cb]==AG_CLSB_OPC_EXT_CMD) 
				n_aux_opc=AG_CLSB_OPC8_EXT_CMD;
			else 
				n_aux_opc=ctl_buf[cb];

			/* Update masks for opcode
			   If previous entry was extended command - AG_CLSB_OPC_EXT_CMD
			   set the opcode and update current_state 
			   if opcode==5 set intermediate tag  */
			if((ctl_buf[cb-1]==AG_CLSB_OPC_EXT_CMD) ||
               ((ctl_buf[cb-1]==AG_CLSB_DEFAULT_VAL) && 
                (buf[cb-1]== AG_CLSB_OPC_EXT_CMD)) )
			{
				curr_state->b_is_extended= AG_TRUE;
				n_cls_mask_entry = AG_CLSB_OPC8_EXT_CMD;

			/* no need to check since buffer is of AG_U8. vale cannot be bigger then 255 */

				if (ctl_buf[cb]==AG_CLSB_OPC8_INT_TAG)
				{
					/* Buffer length validation */
					n_tag_buf_entry++;
				}
			}/* end if is second entry of extended command */
			else 
			{	/* case that previous entry was default */
				if(ctl_buf[cb-1]==AG_CLSB_DEFAULT_VAL)
					n_cls_mask_entry=(AG_U8)buf[cb-1];
				else
					n_cls_mask_entry=ctl_buf[cb-1];

			}/* end else previous default */

#ifndef NO_VALIDATION    
			/*Set the new aux_tag */
			if(n_tag_buf_entry>=n_taglen)
			{

                #ifdef CLS_TEST_FULL_TRACE
				fprintf (full_trace, "\tEND: add_path_8 (AG_E_OUT_OF_RANGE)\n");
                #endif /* CLS_TEST_FULL_TRACE */

				return AG_E_OUT_OF_RANGE;
			}
#endif /*#ifndef NO_VALIDATION */

			/*Increment tag if opcode==7 for the last tag */
			if(ctl_buf[cb]==AG_CLSB_OPC8_TAG)
				n_tag_buf_entry++;

			n_aux_tag=tag_buf[n_tag_buf_entry-1];

			/*Calls the function with aux */
			if ((n_res=add_byte_8 (n_aux_opc,
								   (AG_U8)(buf[cb]&aClsMaskOpc8[(int)n_cls_mask_entry]),
								   n_aux_tag,curr_state, 
								   p_stdprg->p_mem_handle)) 
						& AG_E_FAIL)
			{

                #ifdef CLS_TEST_FULL_TRACE
				fprintf (full_trace, "\tEND: add_path_8 (%08x after add_byte_8)\n", n_res);
                #endif /* CLS_TEST_FULL_TRACE */

				return n_res;
			}
		}/*end of else not default */


		cb++;
		buf_len--;
	}/*end of while */

#ifndef NO_VALIDATION    
	/*Validate buffer size */
	if(n_tag_buf_entry!=n_taglen)
	{

        #ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\tEND: add_path_8 (AG_E_OUT_OF_RANGE)\n");
        #endif /* CLS_TEST_FULL_TRACE */

		return AG_E_OUT_OF_RANGE;
	}
#endif /*#ifndef NO_VALIDATION*/

    p_stdprg->x_root.def_rec = curr_state->def_rec;
	p_stdprg->x_root.def_pt = curr_state->def_pt;

    #ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "\t\t\t\tdef_pt %d\n\t\t\t\tdef_rec %d\n\t\t\t\tdef_opc %d\n", 
		p_stdprg->x_root.def_pt, p_stdprg->x_root.def_rec, p_stdprg->x_root.n_cls_def_opc);
	fprintf (full_trace, "\tEND: add_path_8 (%08x)\n", n_res);
    #endif /* CLS_TEST_FULL_TRACE */

	return n_res;

} /* of add_path_8 */
  /*********************************************************
Name: 
   add_path_16
Description:
   	Adds a path  the classification tree
Parameters:

Returns AgResult:
    Success: AG_S_OK
	Fail : AG_E_FAIL - called functions return error
		   E_CLS_OPEN - the path was not tagged
		   E_CLS_INVALID_OPC -opcode not valid - extended cmd
		   E_OUT_OF_BOUNDS- the buffers are not consistent		   		   
 ***********************************************************/
AgResult add_path_16( AG_U16 *buf,  AG_U8 *ctl_buf , AG_U8 *msk_buf, AG_U32 buf_len,AG_U32 *tag_buf,AG_U32 n_taglen,  StdPrg *p_stdprg ,  ClsbCurrState *curr_state)
{
int cb = 1;
AG_U8 n_cls_mask_entry;
AG_U8 n_aux_opc;
AG_U32 n_aux_tag;
AG_U32 n_tag_buf_entry=0;
AgResult n_res=AG_S_OK;

	p_stdprg->p_mem_handle->n_update_size = 0;


	curr_state->def_rec = p_stdprg->x_root.def_rec;
	curr_state->def_pt = p_stdprg->x_root.def_pt;
	curr_state->def_mask=p_stdprg->x_root.def_mask;

	reset_records(curr_state);

	/*MBFF - 01/01/2001- Verify if TAG>= FIRST_FREE_TAG */
	if((tag_buf[n_taglen-1])<AG_CLSB_FIRST_FREE_TAG)
	{
		return AG_E_CLS_INVALID_TAG;
	}

	/*Verify paths */
	if (ctl_buf[buf_len-1] == AG_CLSB_DEFAULT_VAL)
	{
		if(buf[buf_len-1] != AG_CLSB_OPC16_TAG)
		{
			return AG_E_CLS_OPEN;
		}
	}
	else
	{
		if(ctl_buf[buf_len-1] != AG_CLSB_OPC16_TAG)
		{
			return AG_E_CLS_OPEN;
		}
	}


	/*Increment tag buf for the last tag - for validation purpose. */
	if(buf_len==1)
		n_tag_buf_entry++;

    /*First opcode cannot be 12 - if is extended must be AG_CLSB_OPC_EXT_CMD */
	if ((ctl_buf[0]==AG_CLSB_OPC16_INT_TAG) ||
        ((buf[0]==AG_CLSB_OPC16_INT_TAG) && (ctl_buf[0] == AG_CLSB_DEFAULT_VAL)) )
		return AG_E_CLS_INVALID_OPC;

	if (ctl_buf[0] == AG_CLSB_DEFAULT_VAL)
	{
		/*MBEXT - 24/10/2000 - If is first entry of extended command-AG_CLSB_OPC_EXT_CMD */
		if (buf[0]==AG_CLSB_OPC_EXT_CMD) 
			n_aux_opc=AG_CLSB_OPC16_EXT_CMD;
		else 
			n_aux_opc=(AG_U8)buf[0];/*MB16 - 19/11/2000 - casting added */

		/*Calls the function with aux */
		if ((n_res=add_def_first_16(n_aux_opc,tag_buf[n_taglen-1],msk_buf[0],curr_state, p_stdprg->p_mem_handle)) & AG_E_FAIL)
			return n_res;
	}
	else
	{

		/*MBEXT - 24/10/2000 - Verify if is first entry of extended command */
		if (ctl_buf[0]== AG_CLSB_OPC_EXT_CMD) 
			n_aux_opc=AG_CLSB_OPC16_EXT_CMD;
		else 
			n_aux_opc=ctl_buf[0];

		/*MBINT - 24/10/2000 - Calls the function with aux masked byte as function of opcode */
		if ((n_res=add_first_word_16 (n_aux_opc,(AG_U16)(buf[0]&aClsMaskTable16[(int)p_stdprg->x_root.def_mask]),msk_buf[0],tag_buf[n_taglen-1],curr_state, p_stdprg->p_mem_handle)) & AG_E_FAIL)
		{ 
/*          No need to set changes since if failed, reset stack is called. */
/*			p_stdprg->x_root.def_rec = curr_state->def_rec; */
/*			p_stdprg->x_root.def_pt = curr_state->def_pt; */
			return n_res;
		}
	}

	buf_len--;

	while (buf_len > 0)
	{
		/*Reset the b_is_extended will be set to AG_TRUE if leaf is extended */
		curr_state->b_is_extended= AG_FALSE;

		if (ctl_buf[cb] == AG_CLSB_DEFAULT_VAL)
		{
			/*MBEXT - 24/10/2000 - If previous entry was extended this entry can not */
			/*						be  the default table. */
			if((ctl_buf[cb-1]==AG_CLSB_OPC_EXT_CMD) ||
                ((ctl_buf[cb-1]==AG_CLSB_DEFAULT_VAL) &&
                 (buf[cb-1]==AG_CLSB_OPC_EXT_CMD)))
				return AG_E_CLS_INVALID_OPC;

			/*MBEXT - 24/10/2000 - Verify if is first entry of extended command */
			if (buf[cb]== AG_CLSB_OPC_EXT_CMD) 
				n_aux_opc=AG_CLSB_OPC16_EXT_CMD;
			else 
				n_aux_opc=(AG_U8)buf[cb];/*MB16 - 20/11/2000 -casting added */

			/*MBEXT - 25/10/2000 - If opc==7 update n_tag_buf_entry  */
			if(buf[cb]==AG_CLSB_OPC16_TAG)
				n_tag_buf_entry++;

			/*Calls the function with aux */
			if ((n_res=add_def_word_16(n_aux_opc,tag_buf[n_taglen-1],msk_buf[cb],curr_state, p_stdprg->p_mem_handle)) & AG_E_FAIL)
				return n_res;

		}/*end of if default */
		else/*MBOPC - 11/10/2000 - masked word as function of mask parameter */
		{
			/*MBEXT - 24/10/2000 - Validation: if two consecutive first entries of  */
			/*extended command was entered - 'e''e'-return */
			if(((ctl_buf[cb-1]==AG_CLSB_OPC_EXT_CMD) ||
                ((ctl_buf[cb-1]==AG_CLSB_DEFAULT_VAL) && 
                 (buf[cb-1]==AG_CLSB_OPC_EXT_CMD))) &&
				 (ctl_buf[cb]==AG_CLSB_OPC_EXT_CMD))
				return AG_E_CLS_INVALID_OPC;

			/*MBEXT - 24/10/2000 - Verify if is first entry of extended command */
			if (ctl_buf[cb]== AG_CLSB_OPC_EXT_CMD) 
				n_aux_opc=AG_CLSB_OPC16_EXT_CMD;
			else 
				n_aux_opc=ctl_buf[cb];

			/*Update masks for opcode */
			/*MBEXT - 24/10/2000 - If previous entry was extended command - AG_CLSB_OPC_EXT_CMD */
			/* set the opcode and update current_state  */
			/*if opcode==5 set intermediate tag */
			if((ctl_buf[cb-1]== AG_CLSB_OPC_EXT_CMD) ||
               ((ctl_buf[cb-1]==AG_CLSB_DEFAULT_VAL) && 
                (buf[cb-1]==AG_CLSB_OPC_EXT_CMD)))
			{
				curr_state->b_is_extended= AG_TRUE;
				n_cls_mask_entry = 0xF;/*0000 - to be consistent-> next word ==0 */

				/*MB16 - 22/11/2000 Check  if number of bytes to skip is bigger then 255 */
				if(buf[cb]>MAX_NUM_OF_SKIPS)
						return AG_E_CLS_BAD_SKIP;

				if (ctl_buf[cb]==AG_CLSB_OPC16_INT_TAG)
				{
					/*Buffer length validation */
					n_tag_buf_entry++;
				}

			}/*end if is second entry of extended command */
			else 
			{
				n_cls_mask_entry=msk_buf[cb-1];/*MB16 - 20/11/2000 - previous mask buffer */

			}/*end else  */

			/*MBEXT - 24/10/2000 -Set the new aux_tag */
			if(n_tag_buf_entry>=n_taglen)
				return AG_E_OUT_OF_RANGE;

			/*Increment tag if opcode==7 for the last tag */
			if(ctl_buf[cb]==AG_CLSB_OPC16_TAG)
				n_tag_buf_entry++;

			n_aux_tag=tag_buf[n_tag_buf_entry-1];

			/*Calls the function with aux */
			if ((n_res=add_word_16 (n_aux_opc,(AG_U16)(buf[cb]&aClsMaskTable16[(int)n_cls_mask_entry]),msk_buf[cb],n_aux_tag,curr_state, p_stdprg->p_mem_handle)) & AG_E_FAIL)
				return n_res;
		}/*end of else not default */


		cb++;
		buf_len--;
	}/*end of while */

	/*Validate buffer size */
	if(n_tag_buf_entry!=(n_taglen))
		return AG_E_OUT_OF_RANGE;

	p_stdprg->x_root.def_rec = curr_state->def_rec;
	p_stdprg->x_root.def_pt = curr_state->def_pt;
	return n_res;



}/*of add path 16*/

AgResult validate_delete_path_8 (AG_U8 *buf, AG_U8 *ctl_buf, AG_U32 buf_len , StdPrg *p_stdprg,
                                 Leaf* p_leaf, ClsbDelPoint *p_del_point)
{
AG_U32 pos = 0;
AG_U32 n_pt = p_stdprg->x_root.def_pt;
AG_U32 n_rec = p_stdprg->x_root.def_rec;
AG_U32 curr_entry;
AG_BOOL erase_flag = AG_FALSE;
AG_U8 opc;
AG_U8 n_cls_aux_mask=aClsMaskOpc8[(int)p_stdprg->x_root.n_cls_def_opc];
AgClsbMem *p_mem_handle;

	p_mem_handle = p_stdprg->p_mem_handle;

	while (pos < buf_len)
	{
		if (ctl_buf[pos] == AG_CLSB_DEFAULT_VAL)
		{ /* The default entry */

			curr_entry = CLS_RECORDS_8_GET(p_mem_handle,n_pt,DEF_TABLE,n_rec);

			if (curr_entry == OCCUPIED_ENTRY_8)
			{
				return AG_E_CLS_MISMATCH;
			}

			/* extended command */
			if(buf[pos]== AG_CLSB_OPC_EXT_CMD)
				opc= AG_CLSB_OPC8_EXT_CMD;
			else
				opc =(AG_U8) buf[pos];

		}
		else
		{ /* A standard entry */
			/* if second entry of extended command 
			   byte read is zero  */
			if(IS_EXTENDED_8(p_mem_handle->p_extend,n_pt,n_rec,p_mem_handle->n_record_limit))
				curr_entry = CLS_RECORDS_8_GET(p_mem_handle,n_pt,PRI_TABLE,n_rec);
			else 
				curr_entry = CLS_RECORDS_8_GET(p_mem_handle,(AG_U8)((buf[pos]&n_cls_aux_mask)+n_pt),PRI_TABLE,n_rec);

			/* Validate opcode 5 as first entry */
			if((ctl_buf[pos] == AG_CLSB_OPC8_INT_TAG) &&
               (!IS_EXTENDED_8(p_mem_handle->p_extend,n_pt,n_rec,p_mem_handle->n_record_limit)))
				return AG_E_CLS_INVALID_OPC;

			/* Extended command */
			if (ctl_buf[pos]==AG_CLSB_OPC_EXT_CMD)
				opc = AG_CLSB_OPC8_EXT_CMD;
			else
				opc = ctl_buf[pos];

			/*Validate PT */
			if ((n_pt != PT_8(curr_entry))&&(!IS_EXTENDED_8(p_mem_handle->p_extend,n_pt,n_rec,p_mem_handle->n_record_limit)))
			{
				return AG_E_CLS_MISMATCH;
			}
			/*Validate skip field */
			if ((IS_EXTENDED_8(p_mem_handle->p_extend,n_pt,n_rec,p_mem_handle->n_record_limit))
                && (buf[pos]!=PT_8(curr_entry)))
			{ /* Is this error can indicate not existing path? */
				return AG_E_CLS_INVALID_OPC;
			}

		}

		if (opc != OPCODE_8(curr_entry)) 
		{
			return AG_E_CLS_MISMATCH; /* Opcode doesn't match */
		}

		/* since we allow deleting a path even if there is no tag at the end (like in the case of
		linked leaf) then for the last byte of the path even if the reference count is more then 1
		we need to set the erase position.*/
		if ((pos < buf_len-1) && 
            clsb_get_ref_count(p_mem_handle, NR_8(curr_entry), NPT_8(curr_entry)) != 1)
		{
			erase_flag = AG_FALSE;
		}
		else
		{
			if (erase_flag == AG_FALSE)
			{
				p_del_point->n_pt = n_pt;
				p_del_point->n_rec = n_rec;
				p_del_point->n_pos = pos;
				p_del_point->n_mask = n_cls_aux_mask;
				erase_flag = AG_TRUE;
			}
		}
        if(pos < buf_len-1)
        {
		    n_pt = NPT_8(curr_entry);
		    n_rec = NR_8(curr_entry);
        }
		pos++;
		n_cls_aux_mask = aClsMaskOpc8[OPCODE_8(curr_entry)];

	} /* of while */

	if (erase_flag == AG_FALSE)
		return AG_E_FAIL;

	if (ctl_buf[buf_len - 1] == AG_CLSB_DEFAULT_VAL)
    {
		p_leaf->e_table = CLS_DEF_TBL;
        p_leaf->n_data = 0;
    }
	else
    {
		p_leaf->e_table = CLS_PRI_TBL;

        /*extended the byte read is zero. */
        if(IS_EXTENDED_8(p_mem_handle->p_extend,n_pt,n_rec,p_mem_handle->n_record_limit))
	        p_leaf->n_data = 0;
        else
	        p_leaf->n_data = buf[buf_len-1];
    }

    p_leaf->n_pt = (AG_U16)n_pt;
	p_leaf->n_rec = (AG_U16)n_rec;

	return AG_S_OK;
} /* of validate_delete_path_8 */


/*********************************************************
Name: 
   delete_path_8
Description:
   	Deletes a 8 bits path from the classification tree
Parameters:

Returns AgResult:
    Success: AG_S_OK
	Fail : AG_E_FAIL - path can't be erased
		   E_CLS_INVALID_OPC -opcode not valid - extended cmd
		   E_CLS_MISMATCH - if path has diferent fields from records
 ***********************************************************/
void delete_path_8 (AG_U8 *buf, AG_U8 *ctl_buf, AG_U32 buf_len, 
                       StdPrg *p_stdprg , ClsbDelPoint *p_del_point /*INPUT ONLY*/)
{
    /* It is important that the function will not change the content of *p_del_point
    since it is used after the function ends by the API. this parameter is input only*/
    AG_U32 curr_entry;
    AG_U32 erase_rec = p_del_point->n_rec; 
    AG_U32 erase_pt = p_del_point->n_pt;
    AG_U32 erase_pos = p_del_point->n_pos;
    AG_U8 erase_mask = p_del_point->n_mask;
    AgClsbMem *p_mem_handle = p_stdprg->p_mem_handle;

	while (erase_pos < buf_len)
	{
		if (clsb_get_ref_count(p_mem_handle, erase_rec, erase_pt) == 1)
		{
			if (ctl_buf[erase_pos] != AG_CLSB_DEFAULT_VAL)
			{
				/*if is extended command ignore byte read and reset the extended table */
				if(IS_EXTENDED_8(p_mem_handle->p_extend,erase_pt,erase_rec,p_mem_handle->n_record_limit))
				{
					curr_entry = CLS_RECORDS_8_GET(p_mem_handle,erase_pt,PRI_TABLE,erase_rec);

					/* Check if other paths use this pt for skip */
                    if(dec_ref_count(p_mem_handle, erase_rec, PT_8(curr_entry))==0)
					{
						/*erase the PT skip */
						CLS_RECORDS_8_PUT(p_mem_handle,PT_8(curr_entry),DEF_TABLE,erase_rec,EMPTY_ENTRY_8);
					}

					CLS_RECORDS_8_PUT(p_mem_handle,erase_pt,PRI_TABLE,erase_rec,EMPTY_ENTRY_8);

					/* Reset image bit table */
					CLS_REC_IMAGE_8(p_mem_handle->p_record_image,BIT_TABLE_ENTRY(erase_pt), erase_rec)
						&=(~BIT_ENTRY_POS(erase_pt));

					/* Increase number of empty entries for the record */
					((AG_U16*)p_mem_handle->p_empty_entries)[erase_rec]++;

					/* Set bit entry to 0 */
					CLS_IS_EXT(p_mem_handle->p_extend,erase_pt, erase_rec,p_mem_handle->n_record_limit)
						&=(~BIT_ENTRY_POS(erase_pt));

				}
				else
				{
					/*Mask added */
					curr_entry = CLS_RECORDS_8_GET(p_mem_handle,(AG_U8)((buf[erase_pos]&erase_mask)+erase_pt),PRI_TABLE,erase_rec);
					CLS_RECORDS_8_PUT(p_mem_handle,(AG_U8)((buf[erase_pos]&erase_mask)+erase_pt),PRI_TABLE,erase_rec,
						EMPTY_ENTRY_8);

					/*reset image bit table */
					CLS_REC_IMAGE_8(p_mem_handle->p_record_image,BIT_TABLE_ENTRY((AG_U8)((buf[erase_pos]&erase_mask)+erase_pt)), erase_rec)
						=CLS_REC_IMAGE_8(p_mem_handle->p_record_image,BIT_TABLE_ENTRY((AG_U8)((buf[erase_pos]&erase_mask)+erase_pt)), erase_rec)
							&(~BIT_ENTRY_POS((AG_U8)((buf[erase_pos]&erase_mask)+erase_pt)));

					/* Increase number of empty entries for the record */
					((AG_U16*)p_mem_handle->p_empty_entries)[erase_rec]++;
				}
			}/*if not default */
			else
			{
				curr_entry = CLS_RECORDS_8_GET(p_mem_handle,erase_pt,DEF_TABLE,erase_rec);
			}/*if default */

			if (erase_pos == 0) 
            {   /* the root of the MP*/
                /* Mark as occupied. The root of the MP is released only when MP is deleted. */
			    CLS_RECORDS_8_PUT(p_mem_handle,erase_pt,DEF_TABLE,erase_rec,
					OCCUPIED_ENTRY_8);
            }
            else
            {   /*Not the MP root so mark as free. */
    			CLS_RECORDS_8_PUT(p_mem_handle,erase_pt,DEF_TABLE,erase_rec,
				   EMPTY_ENTRY_8);
            }

		}
		else
		{ /* Reference count is more than 1 only happens at initial erase position */
			if (ctl_buf[erase_pos] != AG_CLSB_DEFAULT_VAL)
			{	/*Mask added */
				curr_entry = CLS_RECORDS_8_GET(p_mem_handle,(AG_U8)((buf[erase_pos]&erase_mask)+erase_pt),PRI_TABLE,erase_rec);
				CLS_RECORDS_8_PUT(p_mem_handle,(AG_U8)((buf[erase_pos]&erase_mask)+erase_pt),PRI_TABLE,erase_rec,
					EMPTY_ENTRY_8);

				/*reset image bit table */
				CLS_REC_IMAGE_8(p_mem_handle->p_record_image,BIT_TABLE_ENTRY((AG_U8)((buf[erase_pos]&erase_mask)+erase_pt)), erase_rec)
					&= (~BIT_ENTRY_POS((AG_U8)((buf[erase_pos]&erase_mask)+erase_pt)));

				/* Increase number of empty entries for the record */
				((AG_U16*)p_mem_handle->p_empty_entries)[erase_rec]++;
			}
			else
			{
				curr_entry = CLS_RECORDS_8_GET(p_mem_handle,erase_pt,DEF_TABLE,erase_rec);
				CLS_RECORDS_8_PUT(p_mem_handle,erase_pt,DEF_TABLE,erase_rec,
					OCCUPIED_ENTRY_8);
			}
		}

        dec_ref_count(p_mem_handle, erase_rec, erase_pt);
        erase_mask = aClsMaskOpc8[OPCODE_8(curr_entry)];/*Update mask */

		if (erase_pos < (buf_len-1))
		{

			erase_pt = NPT_8(curr_entry);
			erase_rec = NR_8(curr_entry);
		}
	   	erase_pos++;
	} /* of while */

} /* of delete_path_8 */

/*********************************************************
Name: 
   delete_path_16
Description:
   	Deletes a 16 bits path from the classification tree
Parameters:

Returns AgResult:
    Success: AG_S_OK
	Fail : AG_E_FAIL - path can't be erased
		   E_CLS_INVALID_OPC -opcode not valid - extended cmd
		   E_CLS_MISMATCH - if path has diferent fields from records
 ***********************************************************/
AgResult delete_path_16( AG_U16 *buf,  AG_U8 *ctl_buf,  AG_U8 *msk_buf,  AG_U32 buf_len ,  StdPrg *p_stdprg , ClsbCurrState *p_currstate, AG_U32 *p_erase_pos, AG_U16 *p_ref_count)
{
AG_U32 pos = 0;
AG_U16 temp_pt = p_stdprg->x_root.def_pt;
AG_U16 temp_rec = p_stdprg->x_root.def_rec;
AG_U16 erase_rec = 0, erase_pt = 0;
AG_U32 curr_entry;
AG_U32 erase_pos = 0;
AG_BOOL erase_flag = AG_FALSE;
AG_U8 opc;
AG_U16 n_cls_aux_mask=aClsMaskTable16[(int)p_stdprg->x_root.def_mask];
AG_U16 erase_mask = 0;
AG_U32 n_aux_entry_num;
AG_U32 n_aux_entry_num_pt;/*Used for PT calaculation and depends with is extended or not */
AG_U16 n_aux_pt;
AG_U16  n_ref_cntr_tmp = 0xFFFF;
AG_U32  n_erase_pos_tmp = 0xFFFFFFFF;
AgClsbMem *p_mem_handle;

	p_mem_handle = p_stdprg->p_mem_handle;

	while (pos < buf_len)
	{
		if (ctl_buf[pos] == AG_CLSB_DEFAULT_VAL)
		{ /* The default entry */

			curr_entry = CLS_RECORDS_16(p_mem_handle->p_record_base, DEF_ENTRY_NUM_16(temp_pt),temp_rec);

			/*MBEXT - 29/10/2000 - extended command */
			if(buf[pos]== AG_CLSB_OPC_EXT_CMD)
				opc= AG_CLSB_OPC16_EXT_CMD;
			else
				opc =(AG_U8) buf[pos];

		}
		else
		{ /* A standard entry */
			/*Calculate entry for extended calculation */
			n_aux_entry_num=PRI_ENTRY_NUM_16(0,temp_pt);


			/*MBEXT - 29/10/2000 - if second entry of extended command  */
			/*byte read is zero */
			if(IS_EXTENDED_16(p_mem_handle->p_extend,n_aux_entry_num,temp_rec,p_mem_handle->n_record_limit))
			{
				curr_entry = CLS_RECORDS_16(p_mem_handle->p_record_base,n_aux_entry_num,temp_rec);
				n_aux_entry_num_pt=n_aux_entry_num;
			}
			else
			{
				n_aux_entry_num_pt=PRI_ENTRY_NUM_16((buf[pos]&n_cls_aux_mask),temp_pt);
				curr_entry = CLS_RECORDS_16(p_mem_handle->p_record_base,n_aux_entry_num_pt,temp_rec);

			}

			/*MBEXT - 29/10/2000 - Validate opcode 16 as first entry */
			if((ctl_buf[pos]== AG_CLSB_OPC16_INT_TAG) &&
               (!IS_EXTENDED_16(p_mem_handle->p_extend,n_aux_entry_num,temp_rec,p_mem_handle->n_record_limit)))
				return AG_E_CLS_INVALID_OPC;

			/*MBEXT - 29/10/2000 - extended command */
			if (ctl_buf[pos]== AG_CLSB_OPC_EXT_CMD)
				opc = AG_CLSB_OPC16_EXT_CMD;
			else
				opc = ctl_buf[pos];



			/*Calculate aux_pt */
			if((n_aux_entry_num_pt+1)%2==0)
				n_aux_pt=(AG_U16)PT0_16(CLS_RECORDS_16(p_mem_handle->p_record_base,n_aux_entry_num_pt-1,temp_rec));
			else
				n_aux_pt=(AG_U16)PT1_16(CLS_RECORDS_16(p_mem_handle->p_record_base,n_aux_entry_num_pt-2,temp_rec));


			/*Validate PT */
			if ((temp_pt != n_aux_pt)&&(!IS_EXTENDED_16(p_mem_handle->p_extend,n_aux_entry_num,temp_rec,p_mem_handle->n_record_limit)))
			{
				return AG_E_CLS_MISMATCH;
			}
			/*Validate skip field */
			if ((IS_EXTENDED_16(p_mem_handle->p_extend,n_aux_entry_num,temp_rec,p_mem_handle->n_record_limit))&&(buf[pos]!=n_aux_pt))
			{
				return AG_E_CLS_MISMATCH;
			}

		}

		if (opc != OPCODE_16(curr_entry)&&(msk_buf[pos]!=MASK_16(curr_entry))) 
		{
			return AG_E_CLS_MISMATCH; /* Opcode doesn't match */
		}


		if ((pos != buf_len-1) && (CLS_REF_COUNT_16(p_mem_handle->p_ref_count,NPT_16(curr_entry),NR_16(curr_entry), p_mem_handle->n_record_limit) != 1))
		{
			erase_flag = AG_FALSE;

		}
		else
		{
			if (erase_flag == AG_FALSE)
			{
				erase_pt = temp_pt;
				erase_rec = temp_rec;
				erase_pos = pos;

				/* Irena 22/5/2001 returns position fot "delete default" */
				n_erase_pos_tmp = erase_pos;
				n_ref_cntr_tmp = CLS_REF_COUNT_16(p_mem_handle->p_ref_count,erase_pt,erase_rec, p_mem_handle->n_record_limit) - 1; 

				erase_mask = n_cls_aux_mask;/*MBOPC - 11/10/2000 -save mask at erase position */
				erase_flag = AG_TRUE;
			}
		}
		pos++;
		temp_pt = (AG_U16)NPT_16(curr_entry);
		temp_rec = (AG_U16)NR_16(curr_entry);
		n_cls_aux_mask = aClsMaskTable16[MASK_16(curr_entry)];/*MBOPC - 11/10/2000 -Update mask from current entry */

	} /* of while */

	if (erase_flag == AG_FALSE)
		return AG_E_FAIL;

	while (erase_pos < buf_len)
	{

		/*MBEXT - 29/10/2000 - Used for leaf as extended command */
		p_currstate->b_is_extended=AG_FALSE;


		if (CLS_REF_COUNT_16(p_mem_handle->p_ref_count,erase_pt,erase_rec, p_mem_handle->n_record_limit) == 1)
		{
			if (ctl_buf[erase_pos] != AG_CLSB_DEFAULT_VAL)
			{

				/*Calculate entry  */
					n_aux_entry_num=PRI_ENTRY_NUM_16(0,erase_pt);

				/*MBEXT - 29/10/2000 if is extended command ignore byte read */
				/*and reset the extended table */
				if(IS_EXTENDED_16(p_mem_handle->p_extend,n_aux_entry_num,erase_rec,p_mem_handle->n_record_limit))
				{

					curr_entry = CLS_RECORDS_16(p_mem_handle->p_record_base,n_aux_entry_num,erase_rec);


					CLS_RECORDS_16(p_mem_handle->p_record_base,n_aux_entry_num,erase_rec)
						= EMPTY_ENTRY_16;
					if((n_aux_entry_num+1)%2==0)
					{

						CLS_RECORDS_16(p_mem_handle->p_record_base,n_aux_entry_num-1,erase_rec)
							=CLS_RECORDS_16(p_mem_handle->p_record_base,n_aux_entry_num-1,erase_rec)|MAKE_PT0_16(0xFFFF);
					}
					else
					{

						CLS_RECORDS_16(p_mem_handle->p_record_base,n_aux_entry_num-2,erase_rec)
							=CLS_RECORDS_16(p_mem_handle->p_record_base,n_aux_entry_num-2,erase_rec)|MAKE_PT1_16(0xFFFF);
					}

					/*MBCOMP - 12/11/2000- reset image bit table */
					CLS_REC_IMAGE_16(p_mem_handle->p_record_image,BIT_TABLE_ENTRY(n_aux_entry_num/2),erase_rec,p_mem_handle->n_record_limit)
						=CLS_REC_IMAGE_16(p_mem_handle->p_record_image,BIT_TABLE_ENTRY(n_aux_entry_num/2),erase_rec,p_mem_handle->n_record_limit)
								&(~BIT_ENTRY_POS(n_aux_entry_num/2));

					/*Set bit entry to 0 */
					CLS_IS_EXT(p_mem_handle->p_extend,n_aux_entry_num/2,erase_rec,p_mem_handle->n_record_limit)
						&= (~BIT_ENTRY_POS(n_aux_entry_num/2));

					p_currstate->b_is_extended=AG_TRUE;
				}/*is extended*/
				else
				{
					/*Calculate entry  */
					/*MBOPC - 11/10/2000 - Mask added */
					n_aux_entry_num=PRI_ENTRY_NUM_16((buf[erase_pos]&erase_mask),erase_pt);

					curr_entry = CLS_RECORDS_16(p_mem_handle->p_record_base,n_aux_entry_num,erase_rec);

					CLS_RECORDS_16(p_mem_handle->p_record_base,n_aux_entry_num,erase_rec)
						=EMPTY_ENTRY_16;

					if((n_aux_entry_num+1)%2==0)
						CLS_RECORDS_16(p_mem_handle->p_record_base,n_aux_entry_num-1,erase_rec)
							|=MAKE_PT0_16(0xFFFF);
					else
						CLS_RECORDS_16(p_mem_handle->p_record_base,n_aux_entry_num-2,erase_rec)
							|=MAKE_PT1_16(0xFFFF);

					/*MBCOMP - 14/11/2000- reset image bit table */
					CLS_REC_IMAGE_16(p_mem_handle->p_record_image,BIT_TABLE_ENTRY(n_aux_entry_num/2),erase_rec,p_mem_handle->n_record_limit)
						&=(~BIT_ENTRY_POS(n_aux_entry_num/2));

				}
			}/*if not default */
			else
			{
				curr_entry = CLS_RECORDS_16(p_mem_handle->p_record_base,DEF_ENTRY_NUM_16(erase_pt),erase_rec);
			}/*if default */

            if(erase_pos == 0)
            {   /* the root of the MP*/
                /* Mark as occupied. The root of the MP is released only when MP is deleted. */
			    CLS_RECORDS_16(p_mem_handle->p_record_base,DEF_ENTRY_NUM_16(erase_pt),erase_rec)
					= OCCUPIED_ENTRY_16;
            }
            else
            {   /*Not the MP root so mark as free. */
			    CLS_RECORDS_16(p_mem_handle->p_record_base,DEF_ENTRY_NUM_16(erase_pt),erase_rec)
                    =EMPTY_ENTRY_16;
            }

		}
		else
		{ /* Reference count is more than 1 only happens at initial erase position */
			if (ctl_buf[erase_pos] != AG_CLSB_DEFAULT_VAL)
			{
				/*Calculate entry  */
				n_aux_entry_num=PRI_ENTRY_NUM_16((buf[erase_pos]&erase_mask),erase_pt);

				/*MBOPC - 11/10/2000 - Mask added */
				curr_entry = CLS_RECORDS_16(p_mem_handle->p_record_base,n_aux_entry_num,erase_rec);

				CLS_RECORDS_16(p_mem_handle->p_record_base,n_aux_entry_num,erase_rec)
					=EMPTY_ENTRY_16;

				if((n_aux_entry_num+1)%2==0)
						CLS_RECORDS_16(p_mem_handle->p_record_base,n_aux_entry_num-1,erase_rec)
							|=MAKE_PT0_16(0xFFFF);
					else
						CLS_RECORDS_16(p_mem_handle->p_record_base,n_aux_entry_num-2,erase_rec)
							|=MAKE_PT1_16(0xFFFF);

				/*MBCOMP - 14/11/2000- reset image bit table */
				CLS_REC_IMAGE_16(p_mem_handle->p_record_image,BIT_TABLE_ENTRY(n_aux_entry_num/2),erase_rec,p_mem_handle->n_record_limit)
					&=(~BIT_ENTRY_POS(n_aux_entry_num/2));

			}
			else
			{
				/*Calculate entry  */
				n_aux_entry_num=DEF_ENTRY_NUM_16(erase_pt);

				curr_entry = CLS_RECORDS_16(p_mem_handle->p_record_base,n_aux_entry_num,erase_rec);


				CLS_RECORDS_16(p_mem_handle->p_record_base,n_aux_entry_num,erase_rec)
					=(CLS_ALL_ONES & MAKE_OPCODE_16(AG_CLSB_OPC16_TAG) & MAKE_MASK_16(0xF) & MAKE_TAG_16(0));
			}
		}

		CLS_REF_COUNT_16(p_mem_handle->p_ref_count,erase_pt,erase_rec, p_mem_handle->n_record_limit)--;

		erase_mask =aClsMaskTable16[MASK_16(curr_entry)];/*MBOPC - 11/10/2000 - Update mask */

		if (OPCODE_16(curr_entry) !=  AG_CLSB_OPC16_TAG)
		{

			erase_pt =(AG_U16) NPT_16(curr_entry);
			erase_rec = (AG_U16)NR_16(curr_entry);
		}
	   	erase_pos++;
	} /* of while */

	/*MBEXT - 29/10/2000 -  extended the byte read is zero. */
	if(p_currstate->b_is_extended==AG_TRUE)
		p_currstate->curr_data = 0;
	else
		p_currstate->curr_data = buf[erase_pos - 1];

	p_currstate->curr_pt = erase_pt;
	p_currstate->curr_rec = erase_rec;

	if (ctl_buf[erase_pos - 1] == AG_CLSB_DEFAULT_VAL)
		p_currstate->curr_table = DEF_TABLE;
	else
		p_currstate->curr_table = PRI_TABLE;

	*p_erase_pos = n_erase_pos_tmp;
	*p_ref_count = n_ref_cntr_tmp;

	return AG_S_OK;
} /* of delete_path_16 */


/*************************************************************/
/* The following function resets the records counter to zero */
/*************************************************************/
AgResult reset_records(ClsbCurrState *curr_state)
{
	if (curr_state == NULL)
		return AG_E_FAIL;
	curr_state->curr_rec = curr_state->def_rec;
	curr_state->curr_pt = curr_state->def_pt;
	curr_state->next_rec = 0;
	curr_state->next_pt = 0;
	curr_state->curr_table = PRI_TABLE;
	curr_state ->b_is_extended= AG_FALSE;
	return AG_S_OK;
} /* of reset_records */

/************************************************************/
/* The following function finds the next available path tag */
/************************************************************/
AgResult find_next_pt_8 ( ClsbCurrState *curr_state, AgClsbMem *p_mem_handle)
{
AG_U8 entry = 0;
AG_U16 rec_index = p_mem_handle->n_low_limit_record;

#ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "\n\t\t\tSTART: find_next_pt_8\n");
	fprintf (full_trace, "\t\t\t\tParameters:\n");
	fprintf (full_trace, "\t\t\t\t\tcurr_state:\n\t\t\t\t\t\tb_is_extended %d\n",
		curr_state->b_is_extended);
	fprintf (full_trace, "\t\t\t\t\t\tcurr_data %d\n\t\t\t\t\t\tcurr_entry_num %d\n", 
		curr_state->curr_data, curr_state->curr_entry_num);
	fprintf (full_trace, "\t\t\t\t\t\tcurr_pt %d\n\t\t\t\t\t\tcurr_rec %d\n\t\t\t\t\t\tcurr_table %d\n", 
		curr_state->curr_pt, curr_state->curr_rec, curr_state->curr_table);
	fprintf (full_trace, "\t\t\t\t\t\tdef_mask %x\n\t\t\t\t\t\tdef_pt %d\n\t\t\t\t\t\tdef_rec %d\n",
		curr_state->def_mask, curr_state->def_pt, curr_state->def_rec);
	fprintf (full_trace, "\t\t\t\t\t\tnext_pt %d\n\t\t\t\t\t\tnext_rec %d\n", 
		curr_state->next_pt, curr_state->next_rec);
	fprintf (full_trace, "\t\t\t\t\t\tn_num_of_ent_to_copy %d\n\t\t\t\t\t\tnext_entry_num %d\n", 
		curr_state->n_num_of_ent_to_copy, curr_state->next_entry_num);

	fprintf (full_trace, "\t\t\t\t\tlast_record %d\n", p_mem_handle->n_last_record);
#endif /* CLS_TEST_FULL_TRACE */

	while (rec_index < p_mem_handle->n_record_limit)
	{
		/* Do not search for empty place in the static record */
		if (p_mem_handle->p_static_rec_image[BIT_TABLE_ENTRY(rec_index)] & BIT_ENTRY_POS(rec_index))
		{
			rec_index++;
			continue;
		}
		/*Path tag FF is used by the program*/
		while ((CLS_RECORDS_8_GET(p_mem_handle,entry,DEF_TABLE,rec_index) != EMPTY_ENTRY_8)
                && (entry < (AG_U8)(MAX_ENTRY_8-1)))
			entry++;

		if (entry < (AG_U8)(MAX_ENTRY_8-1))
		{
			curr_state->next_pt = entry;
			curr_state->next_rec=rec_index;

			if (p_mem_handle->n_last_record < curr_state->next_rec) 
            { /* Update last record we used*/
				p_mem_handle->n_last_record = curr_state->next_rec;
            }

            #ifdef CLS_TEST_FULL_TRACE
	        fprintf (full_trace, "\t\t\t\tcurr_state:\n",
		        curr_state->b_is_extended);
	        fprintf (full_trace, "\t\t\t\t\tnext_pt %d\n\t\t\t\t\tnext_rec %d\n", 
		        curr_state->next_pt, curr_state->next_rec);
	        fprintf (full_trace, "\t\t\t\tlast_record %d\n", p_mem_handle->n_last_record);
	        fprintf (full_trace, "\n\t\t\tEND: find_next_pt_8 (AG_S_OK)\n");
            #endif /* CLS_TEST_FULL_TRACE */

			return AG_S_OK;

		}
		else
		{
			entry=0;
			rec_index++;
		}

	}

#ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "\n\t\t\tEND: find_next_pt_8 (AG_E_CLS_FULL)\n");
#endif /* CLS_TEST_FULL_TRACE */

	return AG_E_CLS_FULL;
} /* of find_next_pt_8 */

 /*********************************************************
Name: 
   find_next_pt_16
Description:
   	finds the next available path tag
Parameters:

Returns AgResult:
    Success: AG_S_OK
	Fail : E_CLS_FULL - path tag can't be found
 ***********************************************************/
AgResult find_next_pt_16(ClsbCurrState *curr_state, AgClsbMem *p_mem_handle)
{
 AG_U16 entry = 0;
 AG_U16 rec_index = p_mem_handle->n_low_limit_record;

	while (rec_index < p_mem_handle->n_record_limit)
	{
		/* Do not search for empty place in the static record */
		if (p_mem_handle->p_static_rec_image[BIT_TABLE_ENTRY(rec_index)] & BIT_ENTRY_POS(rec_index))
		{
			entry = MAX_DEF_ENT_16;
		}
        else
        {
		    while ((CLS_RECORDS_16(p_mem_handle->p_record_base,DEF_ENTRY_NUM_16(entry),rec_index) != EMPTY_ENTRY_16) 
				&& (entry < (MAX_DEF_ENT_16-1)))
			    entry++;
        }
        
		if (entry < (MAX_DEF_ENT_16-1))
		{
			curr_state->next_pt = entry;
			curr_state->next_rec=rec_index;

			if (p_mem_handle->n_last_record < curr_state->next_rec) /* Update last record we used */
				p_mem_handle->n_last_record = curr_state->next_rec;

			return AG_S_OK;
		}
		else
		{
			entry=0;
			rec_index++;
		}
	}

	return AG_E_CLS_FULL;

}/*of find_next_path_16*/


/************************************************/
/* The following fuction gets a tag from a leaf */
/************************************************/
void cls_get_leaf_tag_8 (Leaf *p_leaf,  AG_U32 *p_tag, AgClsbMem *p_mem_handle)
{
AG_U32 entry;

	entry = CLS_RECORDS_8_GET(p_mem_handle,ENTRY(p_leaf->n_pt , p_leaf->n_data),p_leaf->e_table,p_leaf->n_rec);

/* Check changed to AG_ASSERT since it is internal consistency check
   The only thing that can cause failure is memory corruption. */
    AG_ASSERT(OPCODE_8(entry) == AG_CLSB_OPC8_TAG);

    *p_tag = entry & TAG_MASK_8;

/* Check changed to AG_ASSERT since it is internal consistency check
   The only thing that can cause failure is memory corruption. */
    AG_ASSERT(*p_tag != EMPTY_TAG);

} /* of cls_get_leaf_tag_8 */

 /*********************************************************
Name: 
   cls_get_leaf_tag_16
Description:
   	returns the leaf tag
Parameters:

Returns void:
 ***********************************************************/
void cls_get_leaf_tag_16( Leaf *p_leaf, AG_U32 *p_tag, AgClsbMem *p_mem_handle)
{
AG_U32 entry;
AG_U32 aux_entry_num;
	if(p_leaf->e_table==PRI_TABLE)
		aux_entry_num=(AG_U32)PRI_ENTRY_NUM_16(p_leaf->n_data,p_leaf->n_pt);
	else
		aux_entry_num=(AG_U32)DEF_ENTRY_NUM_16(p_leaf->n_pt);

	entry=CLS_RECORDS_16(p_mem_handle->p_record_base, aux_entry_num,p_leaf->n_rec);/*[aux_entry_num][p_leaf->n_rec]*/

/* Check changed to AG_ASSERT since it is internal consistency check
   The only thing that can cause failure is memory corruption. */
    AG_ASSERT(OPCODE_16(entry) == AG_CLSB_OPC16_TAG);

	*p_tag = TG_16(entry);

/* Check changed to AG_ASSERT since it is internal consistency check
   The only thing that can cause failure is memory corruption. */
    AG_ASSERT(*p_tag != EMPTY_TAG);

}

/**********************************************/
/* The following fuction sets a tag  a leaf */
/**********************************************/
void cls_set_leaf_tag_8 ( Leaf *p_leaf, AG_U32 n_tag, AgClsbMem *p_mem_handle)
{
	AG_U8 n_skip;
    AG_U32 entry_data;

	AG_U32 *entry = &entry_data;

	/*MBULINK - 16/10/2000- The function was completely changed  order to perform
	correctly. The old function set the path tag to 0xff*/
	entry_data = CLS_RECORDS_8_GET(p_mem_handle,ENTRY(p_leaf->n_pt , p_leaf->n_data),p_leaf->e_table,p_leaf->n_rec);

#ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "\n\t\t\tSTART: cls_set_leaf_tag_8\n");
	fprintf (full_trace, "\t\t\t\t\t\t- old entry (ent %d tab %d rec %d val %08x)\n",
		ENTRY(p_leaf->n_pt , p_leaf->n_data),p_leaf->e_table,p_leaf->n_rec,
		CLS_RECORDS_8_GET(p_mem_handle,ENTRY(p_leaf->n_pt , p_leaf->n_data),p_leaf->e_table,p_leaf->n_rec));
#endif /* CLS_TEST_FULL_TRACE */

	if (p_leaf->e_table == PRI_TABLE)
	{
		/*MBEXT - 29/10/2000 - Unlink of extended leaf  */
		if(IS_EXTENDED_8(p_mem_handle->p_extend,p_leaf->n_pt,p_leaf->n_rec,p_mem_handle->n_record_limit))
		{
			CLS_RECORDS_8_PUT(p_mem_handle, p_leaf->n_pt,DEF_TABLE,p_leaf->n_rec,
				CLS_RECORDS_8_GET(p_mem_handle, p_leaf->n_pt,DEF_TABLE,p_leaf->n_rec)
				  & MAKE_TAG_8(0) );

			n_skip = (AG_U8)PT_8(*entry);

			*entry = MAKE_OPCODE_8(AG_CLSB_OPC8_TAG) & MAKE_PT_8(n_skip)& MAKE_TAG_8(n_tag);
            CLS_RECORDS_8_PUT(p_mem_handle,ENTRY(p_leaf->n_pt , p_leaf->n_data),p_leaf->e_table,p_leaf->n_rec,*entry);

				 
		}
		else
		{

			*entry = MAKE_OPCODE_8(AG_CLSB_OPC8_TAG)& MAKE_PT_8(p_leaf->n_pt) & MAKE_TAG_8(n_tag)  ;
            CLS_RECORDS_8_PUT(p_mem_handle,ENTRY(p_leaf->n_pt , p_leaf->n_data),p_leaf->e_table,p_leaf->n_rec,*entry);

        }
	}
	else
	{
		*entry =  MAKE_OPCODE_8(AG_CLSB_OPC8_TAG) & MAKE_PT_8(0xFF) & MAKE_TAG_8(n_tag);
        CLS_RECORDS_8_PUT(p_mem_handle,ENTRY(p_leaf->n_pt , p_leaf->n_data),p_leaf->e_table,p_leaf->n_rec,*entry);

    }

#ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "\t\t\t\t\t\t- new entry (ent %d tab %d rec %d val %08x)\n",
		ENTRY(p_leaf->n_pt , p_leaf->n_data),p_leaf->e_table,p_leaf->n_rec,
		CLS_RECORDS_8_GET(p_mem_handle,ENTRY(p_leaf->n_pt , p_leaf->n_data),p_leaf->e_table,p_leaf->n_rec));

	fprintf (full_trace, "\t\t\tEND: cls_set_leaf_tag_8 (AG_S_OK)\n");
#endif /* CLS_TEST_FULL_TRACE */

} /* of cls_set_leaf_tag_8 */

 /*********************************************************
Name: 
   cls_set_leaf_tag_16
Description:
   	Sets the leaf tag
Parameters:

Returns void:
 ***********************************************************/

void cls_set_leaf_tag_16( Leaf *p_leaf, AG_U32 n_tag, AgClsbMem *p_mem_handle)
{

	/*MBULINK - 16/10/2000- The function was completely changed  order to perform
	correctly. The old function set the path tag to 0xff*/

AG_U32 *entry;
AG_U32 n_aux_entry_num; 


	if (p_leaf->e_table == PRI_TABLE)

	{
		n_aux_entry_num=PRI_ENTRY_NUM_16(p_leaf->n_data,p_leaf->n_pt);

		entry=&CLS_RECORDS_16(p_mem_handle->p_record_base, n_aux_entry_num,p_leaf->n_rec);/*[n_aux_entry_num][p_leaf->n_rec]*/

		/*MBEXT - 29/10/2000 - Unlink of extended leaf  */
		if(IS_EXTENDED_16(p_mem_handle->p_extend,n_aux_entry_num,p_leaf->n_rec,p_mem_handle->n_record_limit))
		{
					CLS_RECORDS_16(p_mem_handle->p_record_base,DEF_ENTRY_NUM_16(p_leaf->n_pt),p_leaf->n_rec)=
				CLS_RECORDS_16(p_mem_handle->p_record_base,DEF_ENTRY_NUM_16(p_leaf->n_pt),p_leaf->n_rec)&MAKE_TAG_16(0);/*[DEF_ENTRY_NUM_16(p_leaf->n_pt)][p_leaf->n_rec]*/
				/*MB16 - don't touch the path tag field*/

		}


		*entry = MAKE_OPCODE_16(AG_CLSB_OPC16_TAG) & MAKE_MASK_16(DEF_MSK_16)& MAKE_TAG_16(n_tag);


	}
	else
	{
		entry=&CLS_RECORDS_16(p_mem_handle->p_record_base,DEF_ENTRY_NUM_16(p_leaf->n_pt),p_leaf->n_rec);/*[DEF_ENTRY_NUM_16(p_leaf->n_pt)][p_leaf->n_rec]*/



		*entry = MAKE_OPCODE_16(AG_CLSB_OPC16_TAG) &MAKE_MASK_16(DEF_MSK_16)&MAKE_TAG_16(n_tag);
	}

}/*end of cls_set_leaf_tag_16*/


/*****************************************************************
Name: clsb_is_valid_to_link
Description: 
Parameters:
	 Leaf *p_leaf:
	AgClsbMem *p_mem:
	AG_U8 n_opcode:
Returns AG_BOOL :
*****************************************************************/
AG_BOOL clsb_is_valid_to_link( Leaf *p_leaf, AgClsbMem *p_mem, AG_U8 n_opcode)
{
#ifndef CLS_8_BIT
	if (p_mem->n_cls_mode == AG_CLS_BIT_MODE_16)
	{
        /* Verify if leaf is extended and OPC==12 */
		if(n_opcode == AG_CLSB_OPC16_INT_TAG) 
        {
            if (p_leaf->e_table == PRI_TABLE)
            {
                if (IS_EXTENDED_16(p_mem->p_extend,
                                   PRI_ENTRY_NUM_16(p_leaf->n_data , p_leaf->n_pt),
                                   p_leaf->n_rec,
                                   p_mem->n_record_limit))
                {
			        return AG_TRUE; 
                }
            }
            return AG_FALSE;
        }
    }
    else
#endif /*ndef CLS_8_BIT*/
    {
#ifndef CLS_16_BIT
        /*Verify if leaf is extended and OPC==5 */
	    if(n_opcode==AG_CLSB_OPC8_INT_TAG)
        {
            if (p_leaf->e_table == PRI_TABLE)
            {
                if(IS_EXTENDED_8(p_mem->p_extend, 
                           p_leaf->n_pt,
                           p_leaf->n_rec,
                           p_mem->n_record_limit))
                {
                    return AG_TRUE;
                }
            }
	        return AG_FALSE;
        }
#endif /*ndef CLS_16_BIT */
    }

    return AG_TRUE;

}

/************************************************************/
/* The following fuction links between one entry to another */
/************************************************************/
void cls_set_link_8 ( Leaf *p_leaf, StdPrg *p_mp, AG_U8 n_opcode)
{
AG_U32 tag = 0;

AG_U32 entry_data;
AG_U32 *entry = &entry_data;
AG_U8 prev_opc;
AG_U8 n_skip;
AgClsbMem *p_mem = p_mp->p_mem_handle;

	entry_data = CLS_RECORDS_8_GET(p_mem,ENTRY(p_leaf->n_pt , p_leaf->n_data),p_leaf->e_table,p_leaf->n_rec);

    #ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "\n\t\t\tSTART: cls_set_link_8\n");
	fprintf (full_trace, "\t\t\t\t\t\t- old entry (ent %d tab %d rec %d val %08x)\n",
		ENTRY(p_leaf->n_pt , p_leaf->n_data),p_leaf->e_table,p_leaf->n_rec,
		CLS_RECORDS_8_GET(p_mem,ENTRY(p_leaf->n_pt , p_leaf->n_data),p_leaf->e_table,p_leaf->n_rec));

    #endif /* CLS_TEST_FULL_TRACE */

	prev_opc= (AG_U8)OPCODE_8(*entry);/* dont change OPCODE  case of OPC_KEEPOPC*/

/* Check changed to AG_ASSERT since it is internal consistency check
   The only thing that can cause failure is memory corruption. */
/*  if ((prev_opc != AG_CLSB_OPC8_TAG) && (n_opcode != OPC_KEEPOPC)) 
	Only AG_CLSB_OPC8_TAG opcode can be changed by group opcode.
	If path which will be linked to mp, is not finished by tag, then error.*/
    AG_ASSERT((prev_opc == AG_CLSB_OPC8_TAG) || (n_opcode == (AG_U8)OPC_KEEPOPC));

	if ((prev_opc == AG_CLSB_OPC8_TAG)
		|| (prev_opc==AG_CLSB_OPC8_INT_TAG))
	{
		/* Get tag from the entry */
		tag = (*entry) & TAG_MASK_8;

		/* Check changed to AG_ASSERT since it is internal consistency check
		The only thing that can cause failure is memory corruption. */
		AG_ASSERT(tag != EMPTY_TAG);
	}

	if (n_opcode==(AG_U8)OPC_KEEPOPC)
		n_opcode=prev_opc;

	if (p_leaf->e_table == PRI_TABLE)
	{
		/* write to default table and keep num of bytes to skip */
		/* case of opcode == 5 */
		if((n_opcode!=(AG_U8)OPC_KEEPOPC)&&(IS_EXTENDED_8(p_mem->p_extend,p_leaf->n_pt,p_leaf->n_rec,p_mem->n_record_limit)))
		{
			n_skip = (AG_U8)PT_8(*entry);

			if(n_opcode==AG_CLSB_OPC8_INT_TAG)
            { /*the original leaf tag becomes the intermediate tag so it is written
                in the default entry.*/
                AG_U32 tempval = 
				  CLS_RECORDS_8_GET(p_mem,p_leaf->n_pt,DEF_TABLE,p_leaf->n_rec)
					| MAKE_TAG_8(tag);
                CLS_RECORDS_8_PUT(p_mem,p_leaf->n_pt,DEF_TABLE,p_leaf->n_rec,tempval);
			}

			/*update the leaf entry to point to the next record/pt with the new opcode.
            Since it is extended entry, the PT value is the skip value taken from the entry
            before it is overwritten.*/
			*entry = MAKE_OPCODE_8(n_opcode) & MAKE_PT_8(n_skip)&
				 MAKE_NR_8(p_mp->x_root.def_rec) & MAKE_NPT_8(p_mp->x_root.def_pt);
            CLS_RECORDS_8_PUT(p_mem,ENTRY(p_leaf->n_pt , p_leaf->n_data),p_leaf->e_table,p_leaf->n_rec,entry_data);

        }
		else
		{
            /*update the leaf entry to point to the next record/pt with the new opcode.*/
			*entry = MAKE_OPCODE_8(n_opcode) & MAKE_PT_8(p_leaf->n_pt) &
				 MAKE_NR_8(p_mp->x_root.def_rec) & MAKE_NPT_8(p_mp->x_root.def_pt);
            CLS_RECORDS_8_PUT(p_mem,ENTRY(p_leaf->n_pt , p_leaf->n_data),p_leaf->e_table,p_leaf->n_rec,entry_data);

		}
	}/*end if is primary */
	else
	{
        /*update the leaf entry to point to the next record/pt with the new opcode.*/
		*entry = MAKE_OPCODE_8(n_opcode) & MAKE_PT_8(0xFF) &
				 MAKE_NR_8(p_mp->x_root.def_rec) & MAKE_NPT_8(p_mp->x_root.def_pt);
        CLS_RECORDS_8_PUT(p_mem,ENTRY(p_leaf->n_pt , p_leaf->n_data),p_leaf->e_table,p_leaf->n_rec,entry_data);


	}/*end else - is default */

#ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "\t\t\t\t\t\t- new entry (ent %d tab %d rec %d val %08x)\n",
		ENTRY(p_leaf->n_pt , p_leaf->n_data),p_leaf->e_table,p_leaf->n_rec,
		CLS_RECORDS_8_GET(p_mem,ENTRY(p_leaf->n_pt , p_leaf->n_data),p_leaf->e_table,p_leaf->n_rec));

	fprintf (full_trace, "\t\t\tEND: cls_set_leaf_tag_8 (AG_S_OK)\n");
#endif /* CLS_TEST_FULL_TRACE */

} /* of cls_set_link_8 */

/*********************************************************
Name: 
   cls_set_link_16
Description:
   	Sets the link between a group and a mini program
Parameters:

Returns void:
 ***********************************************************/
void cls_set_link_16( Leaf *p_leaf, StdPrg *p_mp, AG_U8 n_opcode)
{
AG_U32 tag;
AG_U32 *entry;
AG_U8 prev_opc;
AG_U32 n_aux_entry_num = 0;
AgClsbMem *p_mem = p_mp->p_mem_handle;

	if (p_leaf->e_table == PRI_TABLE)
	{
		n_aux_entry_num=PRI_ENTRY_NUM_16(p_leaf->n_data , p_leaf->n_pt);
		entry = &CLS_RECORDS_16(p_mem->p_record_base,n_aux_entry_num,p_leaf->n_rec);

	}
	else
	{
		entry= &CLS_RECORDS_16(p_mem->p_record_base,DEF_ENTRY_NUM_16(p_leaf->n_pt),p_leaf->n_rec);
	}

	prev_opc=(AG_U8) OPCODE_16(*entry);/* dont change OPCODE  case of OPC_KEEPOPC*/

/* Check changed to AG_ASSERT since it is internal consistency check
   The only thing that can cause failure is memory corruption. */
/*	if ((prev_opc != AG_CLSB_OPC16_TAG) && (n_opcode != OPC_KEEPOPC))*/
    AG_ASSERT((prev_opc == AG_CLSB_OPC16_TAG) || (n_opcode == (AG_U8)OPC_KEEPOPC));

	tag = TG_16(*entry);

/* Check changed to AG_ASSERT since it is internal consistency check
   The only thing that can cause failure is memory corruption. */
    AG_ASSERT(tag != EMPTY_TAG);
    
	if (n_opcode==(AG_U8)OPC_KEEPOPC)/*MBLNKROOT - 16/10/2000*/
		n_opcode=prev_opc;

	if (p_leaf->e_table == PRI_TABLE)
	{

		/*write to default table and keep num of bytes to skip */
		/* case of opcode == 16 */
		if((n_opcode!= (AG_U8)OPC_KEEPOPC) &&
           (IS_EXTENDED_16(p_mem->p_extend,n_aux_entry_num,p_leaf->n_rec,p_mem->n_record_limit)))
		{

			if(n_opcode==AG_CLSB_OPC16_INT_TAG)
			{

				CLS_RECORDS_16(p_mem->p_record_base,DEF_ENTRY_NUM_16(p_leaf->n_pt),p_leaf->n_rec)
				    |= MAKE_TAG_16(tag);

			}


			*entry = MAKE_OPCODE_16(n_opcode) &MAKE_MASK_16(p_mp->x_root.def_mask)& 
				 MAKE_NR_16(p_mp->x_root.def_rec) & MAKE_NPT_16(p_mp->x_root.def_pt);
		}
		else
		{

			*entry = MAKE_OPCODE_16(n_opcode) & MAKE_MASK_16(p_mp->x_root.def_mask) &
				 MAKE_NR_16(p_mp->x_root.def_rec) & MAKE_NPT_16(p_mp->x_root.def_pt);
		}
	}/*end if is primary */
	else
	{
		*entry =  MAKE_OPCODE_16(n_opcode) & MAKE_MASK_16(p_mp->x_root.def_mask) &
				 MAKE_NR_16(p_mp->x_root.def_rec) & MAKE_NPT_16(p_mp->x_root.def_pt);

	}/*end else - is default */
    
}/*end of set_link_16*/

/***************************************************************
Name: 
    cls_update_and_push
Author: Marcelo Brafman
Description:
    Updates the stack element and pushes to the stack
Parameters:

	 
Returns AgResult: Stack Push Results
Success: AG_S_OK
Failure: E_OUT_OF_BONDS - invalid boundaries
		 E_CLS_CONSIST  - trying to reset a bit already reset
*****************************************************************/

/***************************************************************
Name: 
    cls_update_and_push
Author: Marcelo Brafman
Description:
    Updates the stack element and pushes to the stack
Parameters:

	 
Returns AgResult: Stack Push Results
Success: AG_S_OK
Failure: E_OUT_OF_BONDS - invalid boundaries
		 E_CLS_CONSIST  - trying to reset a bit already reset
*****************************************************************/
AgResult cls_update_and_push( AG_U32 n_ent_num, 
							  AG_U16 n_pt,  
							  AG_U16 n_rec,
							  AG_U16 n_ref_count,  
							  AG_U32 n_ent_val, 
							  AG_U8 n_table,
							  ClsbStack *p_stack)
{
	ClsbStackElement x_stack_element;
	AgResult n_res;

#ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "\n\t\t\tSTART: cls_update_and_push\n");
	fprintf (full_trace, "\t\t\t\tParameters:\n\t\t\t\t\tn_ent_num %d\n\t\t\t\t\tn_pt %d\n",
		n_ent_num, n_pt);

	fprintf (full_trace, "\t\t\t\t\tn_rec %d\n", n_rec);
	fprintf (full_trace, "\t\t\t\t\tn_ent_val %d\n\t\t\t\t\tn_table %d\n", n_ent_val, n_table);
#endif /* CLS_TEST_FULL_TRACE */
        /*update stack element */
		x_stack_element.n_entry_num=n_ent_num;
		x_stack_element.n_entry_path_tag=n_pt;
		x_stack_element.n_record=n_rec;
		x_stack_element.n_ref_count=n_ref_count;
		x_stack_element.n_entry_value=n_ent_val;
		x_stack_element.n_table=n_table;

		/*push to stack */
		n_res = cls_stack_push(p_stack, &x_stack_element);

#ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "\t\t\tReturned Parameter:\n\t\t\t\t\tStack\n");

	fprintf (full_trace, "\t\t\t\t\tn_size %d\n\t\t\t\t\tn_top %d\n", 
		p_stack->n_size, p_stack->n_top);

	fprintf (full_trace, "\t\t\t\t\tn_entry_num %d\n\t\t\t\t\tn_entry_path_tag %d\n\t\t\t\t\tn_entry_value %08x\n", 
		p_stack->a_stack->n_entry_num, p_stack->a_stack->n_entry_path_tag, p_stack->a_stack->n_entry_value);

	fprintf (full_trace, "\t\t\t\t\tn_record %d\n\t\t\t\t\tn_ref_count_ %d\n\t\t\t\t\tn_table %08x\n", 
		p_stack->a_stack->n_record, p_stack->a_stack->n_ref_count, p_stack->a_stack->n_table);

	fprintf (full_trace, "\t\t\tEND: cls_update_and_push (%08x)\n", n_res);
#endif /* CLS_TEST_FULL_TRACE */

		return(n_res);
}

/***************************************************************
Name: 
    clsb_eng_write_ent_to_memory_8
Description:
    Writes non empty value from the stack to the records.
Parameters:

*****************************************************************/
void clsb_eng_write_ent_to_memory_8(ClsbStackElement *p_element, AgClsbMem *p_mem_handle)
{

#ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "\n\t\tSTART: clsb_eng_write_ent_to_memory_8\n");
#endif /* CLS_TEST_FULL_TRACE */

#ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "\t\t\t\t\t- update entry (ent %d tab %d rec %d val %08x)\n",
		p_element->n_entry_num, p_element->n_table,p_element->n_record,
		CLS_RECORDS_8_GET(p_mem_handle,(AG_U8)p_element->n_entry_num,p_element->n_table,p_element->n_record));
#endif /* CLS_TEST_FULL_TRACE */

	if (p_element->n_table == PRI_TABLE)
	{
		/* Not pushed entry means that it is not new entry and it is not copied entry.
		   The entry value should be updated, but ref counter is not changed */
		if (CLS_RECORDS_8_GET(p_mem_handle,(AG_U8)p_element->n_entry_num,PRI_TABLE,p_element->n_record) 
			== PUSHED_ENTRY_8)
		{
			/* Decrement a number of empty entries in the record */
			((AG_U16*)p_mem_handle->p_empty_entries)[p_element->n_record]--;

			if (IS_EXTENDED_8(p_mem_handle->p_extend,p_element->n_entry_num,p_element->n_record, p_mem_handle->n_record_limit))
			{
				/* Increment ref counter */
                inc_ref_count(p_mem_handle, p_element->n_record, p_element->n_entry_num);
			}
			else
			{
				/* Increment ref counter */
                inc_ref_count(p_mem_handle, p_element->n_record, (AG_U8)PT_8(p_element->n_entry_value));
			}

			/* Set image bit */
			CLS_REC_IMAGE_8(p_mem_handle->p_record_image,BIT_TABLE_ENTRY((AG_U8)(p_element->n_entry_num)), p_element->n_record)
				|= BIT_ENTRY_POS((AG_U8)(p_element->n_entry_num));

		}

#ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\t\t\t\t\t- mark entry (ent %d rec %d) as free\n",
			BIT_TABLE_ENTRY((AG_U8)p_element->n_entry_num), p_element->n_record);
#endif /* CLS_TEST_FULL_TRACE */
	}
	else
	{
		/* If entry is not occupied then only entry value should be updated 
		   (this entry is root of changed) */
		if (CLS_RECORDS_8_GET(p_mem_handle,(AG_U8)p_element->n_entry_num,DEF_TABLE,p_element->n_record)
			== OCCUPIED_ENTRY_8)
		{
			/* Increment ref counter */
            inc_ref_count(p_mem_handle, p_element->n_record, p_element->n_entry_num);
		}
	}
	/* Write the value */
	CLS_RECORDS_8_PUT(p_mem_handle,(AG_U8)p_element->n_entry_num,p_element->n_table,p_element->n_record,
		p_element->n_entry_value);


#ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "\t\tEND: clsb_eng_write_ent_to_memory_8\n");
#endif /* CLS_TEST_FULL_TRACE */
}/* of write non empty entry to memory*/

/***************************************************************
Name: 
    clsb_eng_clear_ent_in_memory_8
Description:
    Clear copied entry in the old position. 
	Call this function for empty elements only.
Parameters:

*****************************************************************/
void clsb_eng_clear_ent_in_memory_8( ClsbStackElement *p_element, AgClsbMem *p_mem_handle)
{

#ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "\n\t\tSTART: clsb_eng_clear_ent_in_memory_8\n");
#endif /* CLS_TEST_FULL_TRACE */

	if (p_element->n_table == PRI_TABLE)
	{

		AG_ASSERT(p_element->n_entry_value == EMPTY_ENTRY_8);


		if (IS_EXTENDED_8(p_mem_handle->p_extend,p_element->n_entry_num,p_element->n_record, p_mem_handle->n_record_limit))
		{
			/* Decrement ref counter */
            dec_ref_count(p_mem_handle, p_element->n_record, p_element->n_entry_num);
		}
		else
		{
			/* Decrement ref counter */
            dec_ref_count(p_mem_handle, p_element->n_record, 
                (AG_U8)PT_8(CLS_RECORDS_8_GET(p_mem_handle,(AG_U8)p_element->n_entry_num,p_element->n_table,p_element->n_record))
                );
		}

		/* Reset image bit */
		CLS_REC_IMAGE_8(p_mem_handle->p_record_image,BIT_TABLE_ENTRY((AG_U8)p_element->n_entry_num), p_element->n_record)
			&= (~BIT_ENTRY_POS((AG_U8)p_element->n_entry_num));

		((AG_U16*)p_mem_handle->p_empty_entries)[p_element->n_record]++;

#ifdef CLS_TEST_FULL_TRACE
			fprintf (full_trace, "\t\t\t\t\t- mark entry (ent %d rec %d) as free\n",
				BIT_TABLE_ENTRY((AG_U8)p_element->n_entry_num), p_element->n_record);
#endif /* CLS_TEST_FULL_TRACE */

	}
	else  /* Default table entry */
	{
		if (CLS_RECORDS_8_GET(p_mem_handle,(AG_U8)p_element->n_entry_num,DEF_TABLE,p_element->n_record)
			!= OCCUPIED_ENTRY_8)
		{
			/* Decrement ref counter */
            dec_ref_count(p_mem_handle, p_element->n_record, (AG_U8)p_element->n_entry_num);
		}
	}

	/* Write the value. value should be EMPTY_ENTRY */
	CLS_RECORDS_8_PUT(p_mem_handle,(AG_U8)p_element->n_entry_num,p_element->n_table,p_element->n_record,
		 p_element->n_entry_value);

#ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "\t\t\t\t\t- update entry (ent %d tab %d rec %d val %08x)\n",
		p_element->n_entry_num, p_element->n_table,p_element->n_record,
		CLS_RECORDS_8_GET(p_mem_handle,(AG_U8)p_element->n_entry_num,p_element->n_table,p_element->n_record));
	fprintf (full_trace, "\t\tEND: clsb_eng_clear_ent_in_memory_8\n");
#endif /* CLS_TEST_FULL_TRACE */
}

/***************************************************************
Name: 
    cls_eng_write_leaf_to_memory_8
Description:
    Write top stack element (should be non empty) to memory
Parameters:

*****************************************************************/
void cls_eng_write_leaf_to_memory_8 (ClsbStack *p_stack,  AgClsbMem *p_mem_handle)
{

    ClsbStackElement* p_element;

	AG_ASSERT(p_stack && p_mem_handle);

#ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "\n\t\tSTART: cls_eng_write_leaf_to_memory_8\n");
#endif /* CLS_TEST_FULL_TRACE */

    p_element = cls_stack_pop_8(p_stack);
	if(p_element)
	{/* Stack is not empty */
		clsb_eng_write_ent_to_memory_8(p_element, p_mem_handle);
	}

#ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "\t\tEND: cls_eng_write_leaf_to_memory_8\n");
#endif /* CLS_TEST_FULL_TRACE */
}

/***************************************************************
Name: 
    cls_eng_write_until_empty_ent_to_memory_8
Description:
    Write non empty (until first empty) elements to the memory
Parameters:

*****************************************************************/
void cls_eng_write_until_empty_ent_to_memory_8 (ClsbStack *p_stack,  AgClsbMem *p_mem_handle)
{

    ClsbStackElement* p_element;

	AG_ASSERT(p_stack && p_mem_handle);

#ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "\n\t\tSTART: cls_eng_write_until_empty_ent_to_memory_8\n");
#endif /* CLS_TEST_FULL_TRACE */

	while( (p_element=cls_stack_pop_8(p_stack)) != NULL) 
	{/* Stack is not empty */
		clsb_eng_write_ent_to_memory_8(p_element, p_mem_handle);
	}

#ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "\t\tEND: cls_eng_write_until_empty_ent_to_memory_8\n");
#endif /* CLS_TEST_FULL_TRACE */
}

/***************************************************************
Name: 
    cls_eng_write_until_empty_ent_to_memory_8
Description:
    Write all empty elements to memory
Parameters:

*****************************************************************/
void cls_eng_write_empty_ent_to_memory_8 (ClsbStack *p_stack,  AgClsbMem *p_mem_handle)
{

    ClsbStackElement* p_element;

	AG_ASSERT(p_stack && p_mem_handle);

#ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "\n\t\tSTART: cls_eng_write_empty_ent_to_memory_8\n");
#endif /* CLS_TEST_FULL_TRACE */

	while( (p_element=cls_stack_pop_8(p_stack)) != NULL) 
	{/* Stack is not empty */
		if (p_element->n_entry_value == EMPTY_ENTRY_8)
		{
			clsb_eng_clear_ent_in_memory_8(p_element, p_mem_handle);
		}
		else
		{
			clsb_eng_write_ent_to_memory_8(p_element, p_mem_handle);
		}
	}

#ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "\t\tEND: cls_eng_write_empty_ent_to_memory_8\n");
#endif /* CLS_TEST_FULL_TRACE */
}

/***************************************************************
Name: 
    cls_write_to_memory_16
Author: Marcelo Brafman
Description:
    Writes from the stack to the records.
	Only writes if the entry vcalue is different of the one  the records
Parameters:

	 
Returns AgResult: Stack Push Results
Success: AG_S_OK
Failure: 
*****************************************************************/
void cls_write_to_memory_16( ClsbStack *p_stack,  AgClsbMem *p_mem_handle)
{
    ClsbStackElement x_element;

	while(cls_stack_pop(p_stack,&x_element)!=AG_E_CLS_EMPTY)
	{
		if((x_element.n_entry_value)!=(CLS_RECORDS_16(p_mem_handle->p_record_base,x_element.n_entry_num,x_element.n_record)))
		{
			CLS_RECORDS_16(p_mem_handle->p_record_base,x_element.n_entry_num,x_element.n_record)=x_element.n_entry_value;
		}

		if(x_element.n_table!=DEF_TABLE)
		{
			if (((x_element.n_entry_num+1)%2)==0)
			{
				if(x_element.n_entry_path_tag!=(AG_U16)PT0_16(CLS_RECORDS_16(p_mem_handle->p_record_base,x_element.n_entry_num-1,x_element.n_record)))
					CLS_RECORDS_16(p_mem_handle->p_record_base,x_element.n_entry_num-1,x_element.n_record)=
						
                         (CLS_RECORDS_16(p_mem_handle->p_record_base,x_element.n_entry_num-1,x_element.n_record)
							&(PATH_TAG_1_MASK_16) )
                         |(x_element.n_entry_path_tag);
				}/*of if*/
				else
				{

					if(x_element.n_entry_path_tag!=(AG_U16)PT1_16(CLS_RECORDS_16(p_mem_handle->p_record_base,x_element.n_entry_num-2,x_element.n_record)))
						CLS_RECORDS_16(p_mem_handle->p_record_base,x_element.n_entry_num-2,x_element.n_record)=

						(	CLS_RECORDS_16(p_mem_handle->p_record_base,x_element.n_entry_num-2,x_element.n_record)
								&(PATH_TAG_0_MASK_16) )
                        | MAKE_PT1_16(x_element.n_entry_path_tag);/*[x_element.n_entry_num-2][x_element.n_record]*/
				}/*of else*/
		}/*of def_table*/

		/*MBCOMP - 13/12/2000 - Unmark the record image for compression */
		if((x_element.n_table==PRI_TABLE)&&(x_element.n_entry_value==EMPTY_ENTRY_16))
		{
			CLS_REC_IMAGE_16(p_mem_handle->p_record_image,BIT_TABLE_ENTRY((x_element.n_entry_num)/2),x_element.n_record,p_mem_handle->n_record_limit)
				=CLS_REC_IMAGE_16(p_mem_handle->p_record_image,BIT_TABLE_ENTRY((x_element.n_entry_num)/2),x_element.n_record,p_mem_handle->n_record_limit)
					&(~BIT_ENTRY_POS((x_element.n_entry_num)/2));
		}

	}/*pf while*/
}/* of write to memory*/

/***************************************************************
Name: 
    cls_reset_stack_8
Description:
    Clean Stack if could not innsert new path
	Only writes if the entry vcalue is different of the one  the records
Parameters:

	 
Returns AgResult: 
Success: AG_S_OK
Failure: AG_E_FAIL
****************************************************************/
AgResult cls_reset_stack_8( ClsbStack *p_stack, StdPrg *p_stdprg)
{
	AgClsbMem *p_mem_handle;
    ClsbStackElement* p_stack_element;

	p_mem_handle = p_stdprg->p_mem_handle;

	AG_ASSERT(p_stack);

	/* Get top element of the stack */

	while( (p_stack_element=cls_stack_pop_8(p_stack)) != NULL) 
	{
		if (p_stack_element->n_entry_value != EMPTY_ENTRY_8)
		{
			AG_U8 n_pt;
			if (p_stack_element->n_table == PRI_TABLE)
			{
				/* EXT of entry_num */
				if (IS_EXTENDED_8(p_mem_handle->p_extend,p_stack_element->n_entry_num,p_stack_element->n_record, p_mem_handle->n_record_limit))
				{
					/* Decrement reference counter of skip entry and check if zero */
                    if( dec_ref_count(p_mem_handle, p_stack_element->n_record, 
                                      (AG_U8)PT_8(p_stack_element->n_entry_value)) == 0)
					{
						/*The PT of the entry value is the skip value.*/
						CLS_RECORDS_8_PUT(p_mem_handle,(AG_U8)PT_8(p_stack_element->n_entry_value),DEF_TABLE,p_stack_element->n_record,
							EMPTY_ENTRY_8);
					}

					/*Set bit entry to 0 */
					CLS_IS_EXT(p_mem_handle->p_extend,p_stack_element->n_entry_num, p_stack_element->n_record,p_mem_handle->n_record_limit)
						&= (~BIT_ENTRY_POS(p_stack_element->n_entry_num));

					/* the PT is equal to entry number becuase of data = 0 for extended */
					n_pt = (AG_U8)p_stack_element->n_entry_num; 
				}
				else
				{
					n_pt = (AG_U8)PT_8(p_stack_element->n_entry_value);
				}

				/* If entry is pushed */
				if (CLS_RECORDS_8_GET(p_mem_handle, (AG_U8)p_stack_element->n_entry_num,PRI_TABLE,p_stack_element->n_record)
						== PUSHED_ENTRY_8)
				{
					/* Set pushed pri entry as empty entry */
					CLS_RECORDS_8_PUT(p_mem_handle, (AG_U8)p_stack_element->n_entry_num,PRI_TABLE,p_stack_element->n_record,
						 EMPTY_ENTRY_8);
				}

				/* If entry is not root entry, then clean default table entry */
				if (!((n_pt == p_stdprg->x_root.def_pt) 
					&& (p_stack_element->n_record==p_stdprg->x_root.def_rec))
					&& (CLS_RECORDS_8_GET(p_mem_handle, n_pt,DEF_TABLE,p_stack_element->n_record) 
						== OCCUPIED_ENTRY_8))
				{
					CLS_RECORDS_8_PUT(p_mem_handle, n_pt,DEF_TABLE,p_stack_element->n_record,
						 EMPTY_ENTRY_8);
				}
			}
			else /* Default entry  */
			{
				/* If entry is not root entry, then clean default table entry */
				if (!((p_stack_element->n_entry_num == p_stdprg->x_root.def_pt) 
					&& (p_stack_element->n_record==p_stdprg->x_root.def_rec))
					&&(	CLS_RECORDS_8_GET(p_mem_handle, (AG_U8)p_stack_element->n_entry_num,DEF_TABLE,p_stack_element->n_record) 
						== OCCUPIED_ENTRY_8))
				{
					CLS_RECORDS_8_PUT(p_mem_handle, (AG_U8)p_stack_element->n_entry_num,DEF_TABLE,p_stack_element->n_record,
						EMPTY_ENTRY_8);
				}
			}
		}
	}
	return AG_S_OK;
}

/***************************************************************
Name: 
    cls_undo_write_to_mem_8
Description:
    Clean Stack if could not innsert new path
	Only writes if the entry vcalue is different of the one  the records
Parameters:

	 
Returns AgResult: 
Success: AG_S_OK
Failure: 
****************************************************************/
AgResult cls_undo_write_to_mem_8(ClsbStack *p_stack, AG_S32 n_org_top, StdPrg *p_stdprg)
{
	AgClsbMem *p_mem_handle;
    ClsbStackElement* p_stack_element;
	AG_S32 n_stack_top = p_stack->n_top;

	p_mem_handle = p_stdprg->p_mem_handle;

	p_stack->n_top = n_org_top;

	AG_ASSERT(p_stack);
    
    p_stack_element=cls_stack_pop_8(p_stack);
	if (!p_stack_element)
    { /*stack is empty.*/
		return AG_E_FAIL;
	}

	while ((p_stack->n_top >= n_stack_top) 
			&& (p_stack_element->n_entry_value != EMPTY_ENTRY_8)) 
	{

		AG_U16 n_pt;


		if (p_stack_element->n_table == PRI_TABLE)
		{

			/* EXT of entry_num */
			if (IS_EXTENDED_8(p_mem_handle->p_extend,p_stack_element->n_entry_num,p_stack_element->n_record, p_mem_handle->n_record_limit))
			{
				/* Decrement reference counter of skip entry and check if zero */
                if( dec_ref_count(p_mem_handle, p_stack_element->n_record, 
                                  (AG_U8)PT_8(p_stack_element->n_entry_value)) == 0)
				{
					/*The PT of the entry value is the skip value.*/
					CLS_RECORDS_8_PUT(p_mem_handle,(AG_U8)PT_8(p_stack_element->n_entry_value),DEF_TABLE,p_stack_element->n_record,
						EMPTY_ENTRY_8);
				}

				/*Set bit entry to 0 */
				CLS_IS_EXT(p_mem_handle->p_extend,p_stack_element->n_entry_num, p_stack_element->n_record,p_mem_handle->n_record_limit)
					&= (~BIT_ENTRY_POS(p_stack_element->n_entry_num));

				/* the PT is equal to entry number becuase of data = 0 for extended */
				n_pt = (AG_U8)p_stack_element->n_entry_num; 
			}
			else
			{
				n_pt = (AG_U8)PT_8(p_stack_element->n_entry_value);
			}

			/* Decrement ref counter of entry */
            dec_ref_count(p_mem_handle, p_stack_element->n_record, n_pt);

			/* reset image bit table for primary entry */
			CLS_REC_IMAGE_8(p_mem_handle->p_record_image,BIT_TABLE_ENTRY((AG_U8)p_stack_element->n_entry_num), p_stack_element->n_record)
				&= (~BIT_ENTRY_POS((AG_U8)p_stack_element->n_entry_num));

			/* Set pushed pri entry as empty entry */
			CLS_RECORDS_8_PUT(p_mem_handle, (AG_U8)p_stack_element->n_entry_num,PRI_TABLE,p_stack_element->n_record,
				 EMPTY_ENTRY_8);

			((AG_U16*)p_mem_handle->p_empty_entries)[p_stack_element->n_record]++;

			/* If entry is not root entry, then clean default table entry */
			if (!((n_pt == p_stdprg->x_root.def_pt) 
				&& (p_stack_element->n_record==p_stdprg->x_root.def_rec))
				&& (CLS_RECORDS_8_GET(p_mem_handle, n_pt,DEF_TABLE,p_stack_element->n_record) 
					== OCCUPIED_ENTRY_8))
			{
				CLS_RECORDS_8_PUT(p_mem_handle, n_pt,DEF_TABLE,p_stack_element->n_record,
					EMPTY_ENTRY_8);
			}
		}
		else
		{
			/* Decrement ref counter of entry */
            dec_ref_count(p_mem_handle, p_stack_element->n_record, p_stack_element->n_entry_num);

			/* If entry is not root entry, then clean default table entry */
			if (!((p_stack_element->n_entry_num == p_stdprg->x_root.def_pt) 
				&& (p_stack_element->n_record==p_stdprg->x_root.def_rec)))
			{
				CLS_RECORDS_8_PUT(p_mem_handle, (AG_U8)p_stack_element->n_entry_num,DEF_TABLE,p_stack_element->n_record,
				 EMPTY_ENTRY_8);
			}
		}

        p_stack_element = cls_stack_pop_8(p_stack);
		if (!p_stack_element)
		{
			return AG_E_FAIL;
		}
	}

	/*return n_top to its value at entring this function */
	p_stack->n_top = n_stack_top;

	return AG_S_OK;
}

/***************************************************************
Name: 
    cls_reset_stack_16
Author: Marcelo Brafman
Description:
    Clean Stack if could not innsert new path
	Only writes if the entry vcalue is different of the one  the records
Parameters:

	 
Returns AgResult: 
Success: AG_S_OK
Failure: 
****************************************************************/
AgResult cls_reset_stack_16(ClsbStack *p_stack, StdPrg *p_stdprg)
{
	AgClsbMem *p_mem_handle;
    ClsbStackElement x_stack_element;

	p_mem_handle = p_stdprg->p_mem_handle;

	if(p_stack==NULL)
		return AG_E_FAIL;

	while(cls_stack_pop(p_stack,&x_stack_element)!=AG_E_CLS_EMPTY)
	{
		/*Only reset entries that are not empty - used by copy entries */
		if(x_stack_element.n_entry_value!=EMPTY_ENTRY_16)
		{

			/*MBEXTBUG - 06/12/2000 - Update old ref_count and skip-pt and reset is_ext */
			if(!IS_EXTENDED_16(p_mem_handle->p_extend,x_stack_element.n_entry_num,x_stack_element.n_record,p_mem_handle->n_record_limit))
			{
				CLS_REF_COUNT_16(p_mem_handle->p_ref_count,
								 x_stack_element.n_entry_path_tag,
								 x_stack_element.n_record, 
								 p_mem_handle->n_record_limit) = x_stack_element.n_ref_count;
			}
			else
			{
				CLS_REF_COUNT_16(p_mem_handle->p_ref_count,
								 PRI_PT_16(0,x_stack_element.n_entry_num),
								 x_stack_element.n_record,
								 p_mem_handle->n_record_limit)=x_stack_element.n_ref_count;

				/*Set bit entry to 0 */
				CLS_IS_EXT(p_mem_handle->p_extend,(x_stack_element.n_entry_num)/2,x_stack_element.n_record,p_mem_handle->n_record_limit)
					=CLS_IS_EXT(p_mem_handle->p_extend,(x_stack_element.n_entry_num)/2,x_stack_element.n_record,p_mem_handle->n_record_limit)
						&(~BIT_ENTRY_POS(x_stack_element.n_entry_num));

			}

			/*Set the entry to empty if it was pushed */
			if(CLS_RECORDS_16(p_mem_handle->p_record_base,x_stack_element.n_entry_num,x_stack_element.n_record)==PUSHED_ENTRY_16)
			{
				CLS_RECORDS_16(p_mem_handle->p_record_base,x_stack_element.n_entry_num,x_stack_element.n_record)=EMPTY_ENTRY_16;

				/*MBCOMP - 14/11/2000- reset image bit table */
				CLS_REC_IMAGE_16(p_mem_handle->p_record_image,BIT_TABLE_ENTRY((x_stack_element.n_entry_num)/2),x_stack_element.n_record,p_mem_handle->n_record_limit)
					=CLS_REC_IMAGE_16(p_mem_handle->p_record_image,BIT_TABLE_ENTRY((x_stack_element.n_entry_num)/2),x_stack_element.n_record,p_mem_handle->n_record_limit)
						&(~BIT_ENTRY_POS((x_stack_element.n_entry_num)/2));
			}

			/*Reset the pt entry if ref_count==0 and if is not the def_pt */
			if((x_stack_element.n_ref_count==0)&&(!(((x_stack_element.n_entry_path_tag)==p_stdprg->x_root.def_pt)&&((x_stack_element.n_record)==p_stdprg->x_root.def_rec))))
			{
					CLS_RECORDS_16(p_stdprg->p_mem_handle->p_record_base,DEF_ENTRY_NUM_16(x_stack_element.n_entry_path_tag),x_stack_element.n_record)=EMPTY_ENTRY_16;
			}
		}
	}/*of while*/
	return AG_S_OK;
}


#if CLS_NAIVE_PATTERN_MATCHING_ALGORITHM
/***************************************************************
Name: 
    cls_search_new_entry_and_allocate_8
Author: Marcelo Brafman
Description:
    Looks for a record that can absorb the entries that must
	be copied  copy entries.
Parameters:
	current_entry - . 
	current_state- .
Returns AgResult:
 	Success: AG_S_OK
    Failure: E_CLS_FULL
*****************************************************************/
AgResult cls_search_new_entry_and_allocate_8( AG_U32 *curr_entry, ClsbCurrState *curr_state, AgClsbMem *p_mem_handle)
{
AG_U32 entry_index;
AG_U16 from_pt;
AG_U16 n_rec=p_mem_handle->n_low_limit_record;
AG_U16 from_rec;
AG_U32 n_carry,n_prev_carry;
AG_U32 i=0;
AG_U32 bit_index;
AG_U8 n_aux_pt;
AgResult n_res;
AG_U32 n_zero = 0;

	AG_MEMSET_WORD(p_mem_handle->p_bit_impression,n_zero,BIT_TABLE_ENTRY(MAX_ENTRY_8)*sizeof(AG_U32));

	if (curr_entry != NULL)
	{
		from_pt = (AG_U16)NPT_8(*curr_entry);
		from_rec = (AG_U16)NR_8(*curr_entry);

	}
	else
	{
		from_pt = curr_state->def_pt;
		from_rec = curr_state->def_rec;

	}

	p_mem_handle->x_node_update.b_leaves = AG_FALSE;

	/* Mark the next entry at the bit impression table */
	p_mem_handle->p_bit_impression[BIT_TABLE_ENTRY(curr_state->next_entry_num)]
		|= BIT_ENTRY_POS(curr_state->next_entry_num);

	/*find the PT entries, store them in the stack as empty (so they are
    removed by the write operation) and mark the impression table	*/
	for (entry_index = 0 ; entry_index < MAX_ENTRY_8 ; entry_index++)
	{
		if ( (PT_8(CLS_RECORDS_8_GET(p_mem_handle,entry_index,PRI_TABLE,from_rec))== from_pt)
			&&(!IS_EXTENDED_8(p_mem_handle->p_extend,entry_index,from_rec,p_mem_handle->n_record_limit)))
		{
			/* Update if is leaf or not for the mini program handler */
			if(OPCODE_8(CLS_RECORDS_8_GET(p_mem_handle,entry_index,PRI_TABLE,from_rec)) == AG_CLSB_OPC8_TAG)
				p_mem_handle->x_node_update.b_leaves = AG_TRUE;

			/*Pushing to the stack the entries of the copied node as empty entries.
            When the cls_write_to_memory_8 function writes from the stack to the record
            the effect is to clear the copied entries.*/
			if(AG_FAILED(cls_stack_push_8(entry_index,
												   from_rec,
												   EMPTY_ENTRY_8, 
												   PRI_TABLE,
												   &p_mem_handle->x_old_stack)))
				return AG_E_CLS_FULL;


			/* Save the index  the array for writing the new PT		*/
			p_mem_handle->p_entry_to_copy[i++]=(AG_U8)entry_index;

			/* Set the bit  the impression table */
			p_mem_handle->p_bit_impression[BIT_TABLE_ENTRY(entry_index)]
				|= BIT_ENTRY_POS(entry_index);
		}

	}

	/* update the number of entries to copy for further use  copy entries */
		curr_state->n_num_of_ent_to_copy=i;

	/* Search for place for the current impression */
	while(n_rec<=p_mem_handle->n_last_record)
	{
		/* Do not search for empty place in the static record */
		if ((p_mem_handle->p_static_rec_image[BIT_TABLE_ENTRY(n_rec)] & BIT_ENTRY_POS(n_rec))
			|| (((AG_U16*)p_mem_handle->p_empty_entries)[n_rec] < (curr_state->n_num_of_ent_to_copy+1)))
		{
			n_rec++;
			continue;
		}

		for(entry_index=0;entry_index<MAX_ENTRY_8;entry_index++)
		{
			n_aux_pt=(AG_U8)(from_pt+entry_index+1);
			/*If the new pt is not taken perform the search*/
			if((n_aux_pt<0xFF)
				&& (CLS_RECORDS_8_GET(p_mem_handle,n_aux_pt,DEF_TABLE,n_rec)
				    ==EMPTY_ENTRY_8))
			{

				n_res=0;
				/* Init previous carry calculate from last entry. */
				n_prev_carry=CALC_CARRY(p_mem_handle->p_bit_impression[MAX_BIT_ENTRIES_8-1]);

				for(bit_index=0;bit_index<MAX_BIT_ENTRIES_8;bit_index++)
				{
					/*calulate the carry  */
					n_carry=CALC_CARRY(p_mem_handle->p_bit_impression[bit_index]);

					/*Rotate the entry - shift left by one position */
					p_mem_handle->p_bit_impression[bit_index]
						<<=1;

					/*write the previous carry to entry  table */
					p_mem_handle->p_bit_impression[bit_index]
						|= n_prev_carry;

					/*Update the prev_carry */
					n_prev_carry=n_carry;

					/*Raanan Refua 12/11/2002*/
#if CLS_TURN_ON_IF_IN_NAIVE					
					if(n_res ==0)
						n_res=(p_mem_handle->p_bit_impression[bit_index]& CLS_REC_IMAGE_8(p_mem_handle->p_record_image,bit_index, n_rec));
#else
					/*Check to see if the image is already taken */
					n_res|=(p_mem_handle->p_bit_impression[bit_index]& CLS_REC_IMAGE_8(p_mem_handle->p_record_image,bit_index, n_rec));
#endif

				}


				/*check to verify if n_res is zero */
				if(n_res==0)
				{
					curr_state->next_pt=n_aux_pt;
					curr_state->next_rec=n_rec;
					return AG_S_OK;
				}
			}/*of if*/
			else/*Only shift the bit impression */
			{
				/*Init previous carry calculate from last entry. */
				n_prev_carry=CALC_CARRY(p_mem_handle->p_bit_impression[MAX_BIT_ENTRIES_8-1]);


				for(bit_index=0;bit_index<MAX_BIT_ENTRIES_8;bit_index++)
				{
					/*calulate the carry  */
					n_carry=CALC_CARRY(p_mem_handle->p_bit_impression[bit_index]);

					/*Rotate the entry - shift left by one position */
					p_mem_handle->p_bit_impression[bit_index]
						<<=1;

					/*write the previous carry to entry  table */
					p_mem_handle->p_bit_impression[bit_index]
						|= n_prev_carry;

					/*Update the prev_carry */
					n_prev_carry=n_carry;

				}


			}/*of else*/

		}/*of first for*/

		/*increment the record */
		n_rec++;
	}/*of while rec*/

	/*allocate new record */
	if(n_rec<p_mem_handle->n_record_limit)
	{
		p_mem_handle->n_last_record = n_rec;
		curr_state->next_pt=0;
		curr_state->next_rec=n_rec;
		return AG_S_OK;
	}

	return AG_E_CLS_FULL;
}
#endif


#if CLS_ONES_RR_MATCHING_ALGORITHM
/***************************************************************
Name: 
    cls_search_new_entry_and_allocate_8
Author: Marcelo Brafman
Modifications: Raanan Refua

Description:
    Looks for a record that can absorb the entries that must
	be copied  copy entries.
Parameters:
	current_entry - . 
	current_state- .
Returns AgResult:
 	Success: AG_S_OK
    Failure: E_CLS_FULL
*****************************************************************/
AgResult cls_search_new_entry_and_allocate_8( AG_U32 *curr_entry, ClsbCurrState *curr_state, AgClsbMem *p_mem_handle)
{
	AG_U32 entry_index;
	AG_U16 from_pt;
	AG_U16 n_rec=p_mem_handle->n_low_limit_record;
	AG_U16 from_rec;
	AG_U32 i=0;
	AG_U8 n_aux_pt;

	AG_U32 n_lower_one_pos=0;

	AG_U32 n_ones_in_pattern=0;

	AG_U32 n_i=0;
	AG_U32 n_text_index=0;
	AG_BOOL b_match_exists=AG_FALSE;

	if (curr_entry != NULL)
	{
		from_pt = (AG_U16)NPT_8(*curr_entry);
		from_rec = (AG_U16)NR_8(*curr_entry);

	}
	else
	{
		from_pt = curr_state->def_pt;
		from_rec = curr_state->def_rec;

	}

	p_mem_handle->x_node_update.b_leaves = AG_FALSE;

	/* Mark the next entry at the bit impression table */
	n_lower_one_pos=curr_state->next_entry_num;
	*(p_mem_handle->p_ones_pos_in_pattern)=n_lower_one_pos;
	n_ones_in_pattern++;

	/*find the PT entries, store them in the stack as empty (so they are
    removed by the write operation) and mark the impression table	*/

/*
This may improve performance, in some cases. However, in some cases it will reduce performance
Maybe(??) under the below condition we need to call find_next_ent_8 instead of performing full search
	if(!((clsb_get_ref_count(p_mem_handle, from_rec, from_pt)==1)
		&&(CLS_RECORDS_8(p_mem_handle->p_record_base,from_pt,DEF_TABLE,from_rec)!=EMPTY_ENTRY_8)))
*/


	for (entry_index = 0 ; entry_index < MAX_ENTRY_8 ; entry_index++)
	{
		if ( (PT_8(CLS_RECORDS_8_GET(p_mem_handle,entry_index,PRI_TABLE,from_rec))== from_pt)
			&&(!IS_EXTENDED_8(p_mem_handle->p_extend,entry_index,from_rec,p_mem_handle->n_record_limit)))
		{
			/* Update if is leaf or not for the mini program handler */
			if(OPCODE_8(CLS_RECORDS_8_GET(p_mem_handle,entry_index,PRI_TABLE,from_rec)) == AG_CLSB_OPC8_TAG)
				p_mem_handle->x_node_update.b_leaves = AG_TRUE;

			/*Pushing to the stack the entries of the copied node as empty entries.
            When the cls_write_to_memory_8 function writes from the stack to the record
            the effect is to clear the copied entries.*/
			if(AG_FAILED(cls_stack_push_8(entry_index,
												   from_rec,
												   EMPTY_ENTRY_8, 
												   PRI_TABLE,
												   &p_mem_handle->x_old_stack)))
				return AG_E_CLS_FULL;


			/* Save the index  the array for writing the new PT		*/
			p_mem_handle->p_entry_to_copy[i++]=(AG_U8)entry_index;

			/* Set the bit  the impression table */
			*(p_mem_handle->p_ones_pos_in_pattern+n_ones_in_pattern)=entry_index;
			n_ones_in_pattern++;

			/*update lower and upper ones position in pattern*/
			if (entry_index<n_lower_one_pos)
				n_lower_one_pos=entry_index;
		}

	}

	/* update the number of entries to copy for further use  copy entries */
	curr_state->n_num_of_ent_to_copy=i;

	/* Search for place for the current impression */
	while(n_rec<=p_mem_handle->n_last_record)
	{
		/* Do not search for empty place in the static record */
		if ((p_mem_handle->p_static_rec_image[BIT_TABLE_ENTRY(n_rec)] & BIT_ENTRY_POS(n_rec))
			|| (((AG_U16*)p_mem_handle->p_empty_entries)[n_rec] < (curr_state->n_num_of_ent_to_copy+1)))
		{
			n_rec++;
			continue;
		}


		for(entry_index=0;entry_index<MAX_ENTRY_8;entry_index++)
		{
			n_aux_pt=(AG_U8)(from_pt+entry_index+1);
			/*If the new pt is not taken perform the search*/

			if((n_aux_pt<(AG_U8)0xFF) && 
				(CLS_RECORDS_8_GET(p_mem_handle,n_aux_pt,DEF_TABLE,n_rec)==EMPTY_ENTRY_8))
			{
				b_match_exists=AG_TRUE;
				for (n_i=0;n_i<n_ones_in_pattern;n_i++)
				{
					/*calc text index*/

					n_text_index=(entry_index+(*(p_mem_handle->p_ones_pos_in_pattern+n_i))+1) & 0xFF;

					if ((CLS_REC_IMAGE_8(p_mem_handle->p_record_image,
										BIT_TABLE_ENTRY((AG_U8)n_text_index),
										n_rec))
						&
						(BIT_ENTRY_POS((AG_U8)(n_text_index))))
						{
							b_match_exists=AG_FALSE;
							break;
						}

				}

				if (b_match_exists)
				{
					curr_state->next_pt=n_aux_pt;
					curr_state->next_rec=n_rec;
					/*AGOS_SWERR(AGOS_SWERR_WARNING,"Pattern found in shift of ",entry_index,0,0,0,0);*/
					return AG_S_OK;
				}
			}/*of if*/
		}/*of first for*/

		/*increment the record */
		n_rec++;
	}/*of while rec*/

	/*allocate new record*/
	if(n_rec<p_mem_handle->n_record_limit)
	{
		p_mem_handle->n_last_record = n_rec;
		curr_state->next_pt=0;
		curr_state->next_rec=n_rec;
		return AG_S_OK;
	}

	return AG_E_CLS_FULL;
}
#endif

/***************************************************************
Name: 
    cls_search_new_entry_and_allocate_16
Author: Marcelo Brafman
Description:
    Looks for a record that can absorb the entries that must
	be copied  copy entries.
Parameters:
	current_entry - . 
	current_state- .
Returns AgResult:
 	Success: AG_S_OK
    Failure: E_CLS_FULL
*****************************************************************/
AgResult cls_search_new_entry_and_allocate_16( AG_U32 *curr_entry, ClsbCurrState *curr_state, AgClsbMem *p_mem_handle)
{
AG_U32 entry_index;
AG_U16 from_pt;
AG_U16 n_rec=p_mem_handle->n_low_limit_record;
AG_U16 from_rec;
AG_U32 n_carry,n_prev_carry;
AG_U32 i=0;
AG_U32 bit_index,n_res;
AG_U16 n_aux_pt;
AG_U32 n_aux_entry_num;
AG_U32 n_zero = 0;

	AG_MEMSET_WORD(p_mem_handle->p_bit_impression,n_zero,BIT_TABLE_ENTRY(MAX_PRI_ENT_16)*sizeof(AG_U32));

	if (curr_entry != NULL)
	{
		from_pt =(AG_U16) NPT_16(*curr_entry);
		from_rec = (AG_U16)NR_16(*curr_entry);

	}
	else
	{
		from_pt = curr_state->def_pt;
		from_rec = curr_state->def_rec;

	}
	/*MBCOMP - 13/12/2000 - this code was taken from copy entries */
	p_mem_handle->x_node_update.b_leaves = AG_FALSE;

	/*Mark the next entry at the bit impression table */

	p_mem_handle->p_bit_impression[BIT_TABLE_ENTRY((curr_state->next_entry_num)/2)]=
	p_mem_handle->p_bit_impression[BIT_TABLE_ENTRY((curr_state->next_entry_num)/2)]|BIT_ENTRY_POS((curr_state->next_entry_num)/2);

	/*MBCOMP - 11/12/2000 - find the PT entries, store it and mark  the impression */
	/*table	and push to stack */

	for (entry_index = 0 ; entry_index < MAX_PRI_ENT_16 ; entry_index++)
	{
		n_aux_entry_num=PRI_ENTRY_NUM_16(0,entry_index);

		/*calculate the PT - entry may be even or odd */
		if (((n_aux_entry_num-1)%2)==0)
			n_aux_pt=(AG_U16)PT0_16(CLS_RECORDS_16(p_mem_handle->p_record_base,n_aux_entry_num-1,from_rec));
		else
			n_aux_pt=(AG_U16)PT1_16(CLS_RECORDS_16(p_mem_handle->p_record_base,n_aux_entry_num-2,from_rec));

		/*MBEXT-25/10/2000 -Check if path tag is the same and if it is not extended command */
		if ( n_aux_pt == from_pt)
		{
			if(OPCODE_16(CLS_RECORDS_16(p_mem_handle->p_record_base,n_aux_entry_num,from_rec))
				== AG_CLSB_OPC16_TAG)
				p_mem_handle->x_node_update.b_leaves = AG_TRUE;

			/*update the stack element and push to the stack */
			if(AG_FAILED(n_res=cls_update_and_push(n_aux_entry_num,
												  /* EMPTY_PT_16, */
												   from_pt,
												   from_rec,
												   CLS_REF_COUNT_16(p_mem_handle->p_ref_count,n_aux_pt,from_rec,p_mem_handle->n_record_limit),
												   EMPTY_ENTRY_16,
												   PRI_TABLE,
												   &p_mem_handle->x_old_stack)))
				return n_res;

			/*Save the index  the array for writing the new PT */
			p_mem_handle->p_entry_to_copy[i++]=n_aux_entry_num;

			/*Set the bit  the impression table */
			p_mem_handle->p_bit_impression[BIT_TABLE_ENTRY(n_aux_entry_num/2)]
			|= BIT_ENTRY_POS(n_aux_entry_num/2);

		}
	}


	/*update the number of entries to copy for further use  copy entries */
		curr_state->n_num_of_ent_to_copy=i;

	/*Search for place for the current impression */
	while(n_rec<=p_mem_handle->n_last_record)
	{
		/* Do not search for empty place in the static record */
		if ((p_mem_handle->p_static_rec_image[BIT_TABLE_ENTRY(n_rec)] & BIT_ENTRY_POS(n_rec))
			|| (((AG_U32*)p_mem_handle->p_empty_entries)[n_rec]<(curr_state->n_num_of_ent_to_copy+1)))
		{
			n_rec++;
			continue;
		}

		for(entry_index=0;entry_index<MAX_PRI_ENT_16;entry_index++)
		{
			n_aux_pt=(AG_U16)(from_pt+entry_index+1);
			/*If the new pt is not taken perform the search */
			if((n_aux_pt<0x7FFF)&&(CLS_RECORDS_16(p_mem_handle->p_record_base,DEF_ENTRY_NUM_16(n_aux_pt),n_rec)==EMPTY_ENTRY_16))/*[DEF_ENTRY_NUM_16(n_aux_pt)][n_rec]*/
			{

				n_res=0;
				/*Init previous carry calculate from last entry. */
				n_prev_carry=CALC_CARRY(p_mem_handle->p_bit_impression[MAX_BIT_ENTRIES_16-1]);


				for(bit_index=0;bit_index<MAX_BIT_ENTRIES_16;bit_index++)
				{
					/*calulate the carry  */
					n_carry=CALC_CARRY(p_mem_handle->p_bit_impression[bit_index]);

					/*Rotate the entry - shift left by one position */
					p_mem_handle->p_bit_impression[bit_index]=
						p_mem_handle->p_bit_impression[bit_index]<<1;

					/*write the previous carry to entry  table */
					p_mem_handle->p_bit_impression[bit_index]
						|= n_prev_carry;

					/*Update the prev_carry */
					n_prev_carry=n_carry;

					/*Check to see if the image is already taken */
					n_res|=(p_mem_handle->p_bit_impression[bit_index]& CLS_REC_IMAGE_16(p_mem_handle->p_record_image,bit_index,n_rec,p_mem_handle->n_record_limit));

				}


				/*check to verify if n_res is zero */
				if(n_res==0)
				{
					curr_state->next_pt=n_aux_pt;
					curr_state->next_rec=n_rec;
					return AG_S_OK;
				}
			}/*of if*/
			else/*Only shift the bit impression - Path Tag already taken */
			{
				/*Init previous carry calculate from last entry. */
				n_prev_carry=CALC_CARRY(p_mem_handle->p_bit_impression[MAX_BIT_ENTRIES_16-1]);


				for(bit_index=0;bit_index<MAX_BIT_ENTRIES_16;bit_index++)
				{
					/*calulate the carry  */
					n_carry=CALC_CARRY(p_mem_handle->p_bit_impression[bit_index]);

					/*Rotate the entry - shift left by one position */
					p_mem_handle->p_bit_impression[bit_index]=
						p_mem_handle->p_bit_impression[bit_index]<<1;

					/*write the previous carry to entry  table */
					p_mem_handle->p_bit_impression[bit_index]
						|= n_prev_carry;

					/*Update the prev_carry */
					n_prev_carry=n_carry;

				}


			}/*of else*/

		}/*of first for*/

		/*increment the record */
		n_rec++;


	}/*of while rec*/
	/*allocate new record*/
	if(n_rec<p_mem_handle->n_record_limit)
	{
		p_mem_handle->n_last_record = n_rec;
		curr_state->next_pt=0;
		curr_state->next_rec=n_rec;
		return AG_S_OK;
	}

	return AG_E_CLS_FULL;
}


/* Irena 29/05/2001 Add find path function */
/**************************************************************/
/* The following function simulates the classifcation machine */
/**************************************************************/
AgResult query_path_8 (StdPrg *p_stdprg, 
					   AG_U8 *p_frame, 
					   AG_U32 n_frame_len, 
					   AG_BOOL n_match_flag, 
					   AG_U32 *p_tag, 
					   AG_U32 *p_pos)

{
AG_U32 entry = 0;
AG_U32 pos = 0;
AG_BOOL ext_flag=AG_FALSE;

/*Get the intial values from StdPrg*/
AG_U16 cr = p_stdprg->x_root.def_rec;
AG_U8 cpt =(AG_U8)p_stdprg->x_root.def_pt;
AG_U8 copc =(AG_U8)p_stdprg->x_root.n_cls_def_opc;

	if (n_frame_len < 1)
	{
		*p_pos = 0;
		return (AG_E_BAD_PARAMS);
	}

	(*p_tag)=AG_CLS_NO_TAG;

	while  (pos < n_frame_len)
	{

		if((copc==AG_CLSB_OPC8_EXT_CMD)&&(ext_flag==AG_FALSE))
		{
			ext_flag=AG_TRUE;
			entry = CLS_RECORDS_8_GET(p_stdprg->p_mem_handle, cpt, 0, cr);

			switch (OPCODE_8(entry)) 
			{
				case AG_CLSB_OPC8_TAG :/*Set the tag and return*/ 
					*p_tag = entry & TAG_MASK_8;
					*p_pos = pos;




					return AG_S_OK; 

			    case AG_CLSB_OPC8_INT_TAG:/*Set the intermediate tag*/
					*p_tag=CLS_RECORDS_8_GET(p_stdprg->p_mem_handle, cpt, 1, cr) & TAG_MASK_8;
				break;

			    default : break;
			}/*of switch*/

			pos += PT_8(entry);
		}
		else
		{
			if (copc==1) pos++;

			entry = CLS_RECORDS_8_GET(p_stdprg->p_mem_handle, (AG_U8)((p_frame[(int)pos]&aClsMaskOpc8[(int)copc])+cpt), 0, cr);



			if (cpt != PT_8(entry))
			{
				if (n_match_flag == CLSB_QUERY_PATH_MATCH)
				{
					*p_pos = pos;
					return AG_E_FAIL;
				}

				entry = CLS_RECORDS_8_GET(p_stdprg->p_mem_handle, cpt, 1, cr);


			}


			if (OPCODE_8(entry) == AG_CLSB_OPC8_TAG)
			{
				*p_tag = entry & TAG_MASK_8;
				*p_pos = pos;
				if ((*p_tag<=5) && (*p_tag != 0))
					return AG_E_FAIL;
				return AG_S_OK; 
			}

			ext_flag=AG_FALSE;
		}

		copc=(AG_U8)OPCODE_8(entry);
		cpt = (AG_U8)NPT_8(entry);
		cr = (AG_U16)NR_8(entry); 
		pos++ ;
	} /* of while */

	/* Pos may be greater than n_frame_len, because of ext. command */
	*p_pos = (n_frame_len > (pos - 1)) ? (pos-1) : (n_frame_len-1);

	if ((*p_tag)==AG_CLS_NO_TAG)
	{
		/*set the tag to late classification*/
		(*p_tag)=AG_CLS_LATE_CLASS;
	}

	return AG_S_OK;

} /* of find path */

/**************************************************************/
/* The following function simulates the classifcation machine */
/**************************************************************/
AgResult query_path_16 (StdPrg *p_stdprg, 
						AG_U16 *p_frame, 
						AG_U32 n_frame_len, 
						AG_BOOL n_match_flag, 
						AG_U32 *p_tag, 
						AG_U32 *p_pos)
{
AG_U32 entry = 0;
AG_U32 pos = 0;
AG_U32 aux_ent_num;
AG_U16 aux_pt;
AG_BOOL ext_flag=AG_FALSE;

/*Get the intial values from StdPrg*/
AG_U16 cr   = p_stdprg->x_root.def_rec;
AG_U16 cpt  = p_stdprg->x_root.def_pt;
AG_U16 copc =(AG_U8)p_stdprg->x_root.n_cls_def_opc;
AG_U8 cmask = p_stdprg->x_root.def_mask;

	if (n_frame_len < 1)
	{
		*p_pos = pos;
		return (AG_E_FAIL);
	}

    (*p_tag)=AG_CLS_NO_TAG;

	while  (pos < n_frame_len)
	{

		if((copc==AG_CLSB_OPC16_EXT_CMD)&&(ext_flag==AG_FALSE))
		{
			ext_flag=AG_TRUE;
			entry = CLS_RECORDS_16(p_stdprg->p_mem_handle->p_record_base,PRI_ENTRY_NUM_16(0,cpt),cr);

			/*Check the opcode*/
			switch (OPCODE_16(entry)) 
			{
				case AG_CLSB_OPC16_TAG :/*Set the tag and return*/ 
					*p_tag = TG_16(entry);
					*p_pos = pos;
					return AG_S_OK;

				case AG_CLSB_OPC16_INT_TAG:/*Set the intermediate tag*/
					*p_tag=TG_16(CLS_RECORDS_16(p_stdprg->p_mem_handle->p_record_base,DEF_ENTRY_NUM_16(cpt),cr));
				break;

				default : break;
			}/*of switch*/

			aux_ent_num=PRI_ENTRY_NUM_16(0,cpt);

			/*Update the position*/
			if (((aux_ent_num+1)%2)==0)
			{/*Print the skip*/
				/*Calculate the PATH Tag Entry*/
				aux_ent_num=aux_ent_num-1;
				pos+=(PT0_16(CLS_RECORDS_16(p_stdprg->p_mem_handle->p_record_base,aux_ent_num,cr)))/2;

			}
			else
			{
				aux_ent_num=aux_ent_num-2;
				pos+=(PT1_16(CLS_RECORDS_16(p_stdprg->p_mem_handle->p_record_base,aux_ent_num,cr)))/2;
			}


		}/*extended*/
		else
		{
			/*For now the simulator only supports even skips*/
			if ((copc<9)&&(copc>1)) pos+=copc/2;

			aux_ent_num=PRI_ENTRY_NUM_16((p_frame[(int)pos]& aClsMaskTable16[(int)cmask]),cpt);
			entry = CLS_RECORDS_16(p_stdprg->p_mem_handle->p_record_base,aux_ent_num,cr);

			if (((aux_ent_num+1)%2)==0)
			{
				aux_ent_num=aux_ent_num-1;
				aux_pt=(AG_U16)PT0_16(CLS_RECORDS_16(p_stdprg->p_mem_handle->p_record_base,aux_ent_num,cr));
			}
			else
			{
				aux_ent_num=aux_ent_num-2;
				aux_pt=(AG_U16)PT1_16(CLS_RECORDS_16(p_stdprg->p_mem_handle->p_record_base,aux_ent_num,cr));
			}

			if (cpt != aux_pt)
			{
				if (n_match_flag == CLSB_QUERY_PATH_MATCH)
				{
					*p_pos = pos;
					return AG_E_FAIL;
				}

				entry = CLS_RECORDS_16(p_stdprg->p_mem_handle->p_record_base,DEF_ENTRY_NUM_16(cpt),cr);

			}

			if (OPCODE_16(entry) == AG_CLSB_OPC16_TAG)
			{
				*p_tag = TG_16(entry);
				*p_pos = pos;

				if ((*p_tag<=5) && (*p_tag!=0))
					return AG_E_FAIL;

				return AG_S_OK;
			}

			ext_flag=AG_FALSE;
		}

		copc=(AG_U8)OPCODE_16(entry);
		cmask=(AG_U8)MASK_16(entry);
		cpt = (AG_U16)NPT_16(entry);
		cr = (AG_U16)NR_16(entry); 
		pos++ ;
	} /* of while */

	/* Pos may be greater than n_frame_len, because of ext. command */
	*p_pos = (n_frame_len > (pos - 1)) ? (pos-1) : (n_frame_len-1);

	if ((*p_tag)==AG_CLS_NO_TAG )
	{
		/*set the tag to late classification*/
		(*p_tag)=AG_CLS_LATE_CLASS;
		return AG_E_FAIL;
	} 

	return AG_S_OK;

} /* of find path */

void clsb_eng_del_mp_8(StdPrg* p_prg)
{

	if (clsb_get_ref_count(p_prg->p_mem_handle, p_prg->x_root.def_rec, p_prg->x_root.def_pt) == 0)
	{
		CLS_RECORDS_8_PUT(p_prg->p_mem_handle,p_prg->x_root.def_pt,DEF_TABLE,p_prg->x_root.def_rec, 
			 EMPTY_ENTRY_8);

		/* unmark allocated record as static (regardless if it was static or not)*/
	    p_prg->p_mem_handle->p_static_rec_image[BIT_TABLE_ENTRY(p_prg->x_root.def_rec)] 
			&= (~BIT_ENTRY_POS(p_prg->x_root.def_rec));

	}
}

void clsb_eng_del_mp_16(StdPrg* p_prg)
{
	if (CLS_REF_COUNT_16(p_prg->p_mem_handle->p_ref_count,p_prg->x_root.def_pt,p_prg->x_root.def_rec,p_prg->p_mem_handle->n_record_limit) == 0)
	{
		CLS_RECORDS_16(p_prg->p_mem_handle->p_record_base,DEF_ENTRY_NUM_16(p_prg->x_root.def_pt),p_prg->x_root.def_rec)
			= EMPTY_ENTRY_16;

		/* unmark allocated record as static (regardless if it was static or not)*/
	    p_prg->p_mem_handle->p_static_rec_image[BIT_TABLE_ENTRY(p_prg->x_root.def_rec)] 
			&= (~BIT_ENTRY_POS(p_prg->x_root.def_rec));
	}
}


AG_U16 clsb_get_ref_count(AgClsbMem* p_mem, AG_U32 n_rec, AG_U32 n_entry)
{

#ifndef CLS_8_BIT
    if(p_mem->n_cls_mode == AG_CLS_BIT_MODE_16)
    {
        return(CLS_REF_COUNT_16(p_mem->p_ref_count,n_entry,n_rec,p_mem->n_record_limit));
    }
    else
#endif /*ndef CLS_8_BIT*/
    {
#ifndef CLS_16_BIT
        AG_U8* p_counter;
        p_counter = &CLS_REF_COUNT_8(p_mem->p_ref_count,n_entry,n_rec);
        
	    if(CLS_REF_COUNT_EXT8(p_mem->p_ref_ext,n_entry,n_rec) & BIT_ENTRY_POS(n_entry))
        {   
            return(*p_counter + 0x100);
        }
        else
        {
            return(*p_counter);
        }
#endif /*ndef CLS_16_BIT */
    }
}

void inc_ref_count(AgClsbMem* p_mem, AG_U32 n_rec, AG_U32 n_entry)
{

#ifndef CLS_8_BIT
    if(p_mem->n_cls_mode == AG_CLS_BIT_MODE_16)
    {
        CLS_REF_COUNT_16(p_mem->p_ref_count,n_entry,n_rec,p_mem->n_record_limit)++;
    }
    else
#endif /*ndef CLS_8_BIT*/
    {
#ifndef CLS_16_BIT
        AG_U8* p_counter;
        p_counter = &CLS_REF_COUNT_8(p_mem->p_ref_count,n_entry,n_rec);
        
        (*p_counter)++;
        if(*p_counter == 0)
        {   /*counter overflow. Set extension*/
            CLS_REF_COUNT_EXT8(p_mem->p_ref_ext,n_entry,n_rec) |= BIT_ENTRY_POS(n_entry);
        }
		/* since we also count the default, the maximum is 257 = 0x101 */
		AG_ASSERT(clsb_get_ref_count(p_mem, n_rec, n_entry) <= 0x101)
#endif /*ndef CLS_16_BIT */
    }
}


AG_U16 dec_ref_count(AgClsbMem* p_mem, AG_U32 n_rec, AG_U32 n_entry)
{

#ifndef CLS_8_BIT
    if(p_mem->n_cls_mode == AG_CLS_BIT_MODE_16)
    {
        CLS_REF_COUNT_16(p_mem->p_ref_count,n_entry,n_rec,p_mem->n_record_limit)--;
        return(CLS_REF_COUNT_16(p_mem->p_ref_count,n_entry,n_rec,p_mem->n_record_limit));
    }
    else
#endif /*ndef CLS_8_BIT*/
    {
#ifndef CLS_16_BIT
        AG_U8* p_counter;
        p_counter = &CLS_REF_COUNT_8(p_mem->p_ref_count,n_entry,n_rec);
        
        (*p_counter)--;
        if(*p_counter == (AG_U8)0xFF)
        {   /*counter overflow. Set extension*/
            CLS_REF_COUNT_EXT8(p_mem->p_ref_ext,n_entry,n_rec) &= ~BIT_ENTRY_POS(n_entry);
        }
        
        /*after decrement operation value must be less then 257 so the extension bit must be zero.*/
		AG_ASSERT(clsb_get_ref_count(p_mem, n_rec, n_entry) <= 0x100)


        return (*p_counter);
#endif /*ndef CLS_16_BIT */
    }
}


void clsb_eng_utilization (AgClsbMem *p_mem_handle, 
							   AG_U32 *p_empty_rec, 
							   AG_U32 *p_non_empty_rec, 
							   AG_U32 *p_pri_compr_ratio,
							   AG_U32 *p_def_compr_ratio,
							   AG_U32 *p_compr_ratio)
{
	AG_U32 i;
	AG_U32 j;
	AG_U32 n_non_empty_entry_total =0;
	AG_U32 n_non_empty_entry_pri = 0;
	AG_U32 n_non_empty_entry_def = 0;
	AG_U32 n_non_empty_entry_in_rec =0;
	/*AG_U32 *entry; BCMout*/

	*p_empty_rec = 0;
	*p_non_empty_rec = 0;
	*p_pri_compr_ratio = 0;
	*p_def_compr_ratio = 0;
    *p_compr_ratio = 0;

	/* Calculate default compression ratio */
	for (i=p_mem_handle->n_low_limit_record; i<=p_mem_handle->n_last_record; i++)
	{
		n_non_empty_entry_in_rec = 0;

		/* Calculate empty records */
		for (j=0; j<MAX_ENTRY_8; j++)
		{
			/*entry = &CLS_RECORDS_8(p_mem_handle->p_record_base,j,DEF_TABLE,i); BCMout*/
			if (CLS_RECORDS_8_GET(p_mem_handle,j,DEF_TABLE,i) != EMPTY_ENTRY_8)
			{
				n_non_empty_entry_in_rec++;
			}
		}

		if (n_non_empty_entry_in_rec != 0)
		{/* Non empty record */
			(*p_non_empty_rec)++;
		}

		n_non_empty_entry_def += n_non_empty_entry_in_rec;
	}


	if (*p_non_empty_rec != 0)
	{
		*p_def_compr_ratio = (100*n_non_empty_entry_def) /(*p_non_empty_rec*MAX_ENTRY_8);
	}

	n_non_empty_entry_total += n_non_empty_entry_def;

	for (i=p_mem_handle->n_low_limit_record; i<=p_mem_handle->n_last_record; i++)
	{
		n_non_empty_entry_in_rec = 0;

		/* Calculate empty records */
		for (j=0; j<MAX_ENTRY_8; j++)
		{
			/*entry = &CLS_RECORDS_8(p_mem_handle->p_record_base,j,PRI_TABLE,i); BCMout*/
			if (CLS_RECORDS_8_GET(p_mem_handle,j,PRI_TABLE,i) != EMPTY_ENTRY_8)
			{
				n_non_empty_entry_in_rec++;
			}
		}

		n_non_empty_entry_pri += n_non_empty_entry_in_rec;
	}

	n_non_empty_entry_total += n_non_empty_entry_pri;

	if (*p_non_empty_rec!=0)
	{
		*p_pri_compr_ratio = (100*n_non_empty_entry_pri) /(*p_non_empty_rec*MAX_ENTRY_8);
		*p_compr_ratio = (100*n_non_empty_entry_total)/(*p_non_empty_rec*2*MAX_ENTRY_8);
	}

	*p_empty_rec = p_mem_handle->n_record_limit - *p_non_empty_rec;


}


void clsb_eng_usage (AgClsbMem *p_mem_handle, 
					 AG_U32 *p_empty_rec, 
					 AG_U32 *p_non_empty_rec, 
					 AG_U32 *p_non_empty_entry_pri,
					 AG_U32 *p_non_empty_entry_def)
{
	AG_U32 i;
	AG_U32 j;
/*	AG_U32 n_non_empty_entry_total =0; */
/*	AG_U32 n_non_empty_entry_pri = 0; */
/*	AG_U32 n_non_empty_entry_def = 0; */
    AG_U32 n_non_empty_entry_in_rec =0;
	/*AG_U32 *entry; BCMout*/

	*p_empty_rec = 0;
	*p_non_empty_rec = 0;
	*p_non_empty_entry_pri = 0;
	*p_non_empty_entry_def = 0;
/*	*p_pri_compr_ratio = 0; */
/*	*p_def_compr_ratio = 0; */
/*  *p_compr_ratio = 0; */

	/* Calculate default compression ratio */
	for (i=p_mem_handle->n_low_limit_record; i<=p_mem_handle->n_last_record; i++)
	{
		n_non_empty_entry_in_rec = 0;

		/* Calculate empty records */
		for (j=0; j<MAX_ENTRY_8; j++)
		{
			/*entry = &CLS_RECORDS_8(p_mem_handle->p_record_base,j,DEF_TABLE,i); BCMout*/
			if (CLS_RECORDS_8_GET(p_mem_handle,j,DEF_TABLE,i) != EMPTY_ENTRY_8)
			{
				n_non_empty_entry_in_rec++;
			}
		}

		if (n_non_empty_entry_in_rec != 0)
		{/* Non empty record */
			(*p_non_empty_rec)++;
		}

		*p_non_empty_entry_def += n_non_empty_entry_in_rec;
	}


/*	if (*p_non_empty_rec != 0) */
/*	{ */
/*		*p_def_compr_ratio = (100*n_non_empty_entry_def) /(*p_non_empty_rec*MAX_ENTRY_8); */
/*	} */

	/*n_non_empty_entry_total += *p_non_empty_entry_def; */

	for (i=p_mem_handle->n_low_limit_record; i<=p_mem_handle->n_last_record; i++)
	{
		n_non_empty_entry_in_rec = 0;

		/* Calculate empty records */
		for (j=0; j<MAX_ENTRY_8; j++)
		{
			/*entry = &CLS_RECORDS_8(p_mem_handle->p_record_base,j,PRI_TABLE,i); BCMout*/
			if (CLS_RECORDS_8_GET(p_mem_handle,j,PRI_TABLE,i) != EMPTY_ENTRY_8)
			{
				n_non_empty_entry_in_rec++;
			}
		}

		*p_non_empty_entry_pri += n_non_empty_entry_in_rec;
	}

	/*n_non_empty_entry_total += *p_non_empty_entry_pri; */

/*	if (*p_non_empty_rec!=0) */
/*	{ */
/*		*p_pri_compr_ratio = (100*n_non_empty_entry_pri) /(*p_non_empty_rec*MAX_ENTRY_8); */
/*		*p_compr_ratio = (100*n_non_empty_entry_total)/(*p_non_empty_rec*2*MAX_ENTRY_8); */
/*	} */

	*p_empty_rec = p_mem_handle->n_record_limit - *p_non_empty_rec;


}
#ifdef AG_MNT

AgResult clsb_eng_rec_dump_bin_8 (const AG_CHAR *p_file_name, AgClsbMem *p_mem_handle)
{
	FILE *p_out_file; /* output file   */
	AG_U32 i, j;
	AG_U32 *p_rec_base = p_mem_handle->p_record_base;
	AG_U32 n_address = 0x00000000;

	if ((p_out_file = fopen (p_file_name,"wb")) == NULL )
	{
		printf ("\n Can't open %s ",p_file_name);
		return (AG_E_FAIL);
	}

	/* Put records to the file */
	for (i=p_mem_handle->n_low_limit_record; i<p_mem_handle->n_record_limit; i++)
	{
		j = 0;

		fprintf (p_out_file, "\nREC %d PRI\n", i);

		/* Primary table */
		while (j<MAX_ENTRY_8)
		{
			fprintf (p_out_file, "%08x: %08x %08x %08x %08x %08x %08x %08x %08x\n",
	/* 01 */			n_address, CLS_RECORDS_8_GET(p_rec_base,j,PRI_TABLE,i),
	/* 02 */			CLS_RECORDS_8_GET(p_rec_base,j+1,PRI_TABLE,i),
	/* 03 */			CLS_RECORDS_8_GET(p_rec_base,j+2,PRI_TABLE,i),
	/* 04 */			CLS_RECORDS_8_GET(p_rec_base,j+3,PRI_TABLE,i),
	/* 05 */			CLS_RECORDS_8_GET(p_rec_base,j+4,PRI_TABLE,i),
	/* 06 */			CLS_RECORDS_8_GET(p_rec_base,j+5,PRI_TABLE,i),
	/* 07 */			CLS_RECORDS_8_GET(p_rec_base,j+6,PRI_TABLE,i),
	/* 08 */			CLS_RECORDS_8_GET(p_rec_base,j+7,PRI_TABLE,i));
			j += 8;
			n_address += 32;
		}

		j = 0;

		fprintf (p_out_file, "\nREC %d DEF\n", i);

		/* Default table */
		while (j<MAX_ENTRY_8)
		{
			fprintf (p_out_file, "%08x: %08x %08x %08x %08x %08x %08x %08x %08x\n",
	/* 01 */			n_address, CLS_RECORDS_8_GET(p_rec_base,j,DEF_TABLE,i),
	/* 02 */			CLS_RECORDS_8_GET(p_rec_base,j+1,DEF_TABLE,i),
	/* 03 */			CLS_RECORDS_8_GET(p_rec_base,j+2,DEF_TABLE,i),
	/* 04 */			CLS_RECORDS_8_GET(p_rec_base,j+3,DEF_TABLE,i),
	/* 05 */			CLS_RECORDS_8_GET(p_rec_base,j+4,DEF_TABLE,i),
	/* 06 */			CLS_RECORDS_8_GET(p_rec_base,j+5,DEF_TABLE,i),
	/* 07 */			CLS_RECORDS_8_GET(p_rec_base,j+6,DEF_TABLE,i),
	/* 08 */			CLS_RECORDS_8_GET(p_rec_base,j+7,DEF_TABLE,i));

			j += 8;

			n_address += 32;

		}
	}

	fclose (p_out_file);
	return AG_S_OK;
}

AgResult clsb_eng_aux_mem_dump_bin_8 (const AG_CHAR *p_file_name, AgClsbMem *p_mem_handle)
{
	FILE *p_out_file; /* output file   */
	AG_U32 i, j;
	AG_U32 n_rec_limit = p_mem_handle->n_record_limit;
	AG_U32 *p_rec_image_base = p_mem_handle->p_record_image;
	AG_U32 *p_extend_base = p_mem_handle->p_extend;
	AG_U16 *p_empty_entries = p_mem_handle->p_empty_entries;

	AG_U32 n_address = 0x00000000;

	if ((p_out_file = fopen (p_file_name,"wb")) == NULL )
	{
		printf ("\n Can't open %s ",p_file_name);
		return (AG_E_FAIL);
	}

	/* Reference counter */

	fprintf (p_out_file, "\nREFERENCE COUNTER\n");
	for (i=p_mem_handle->n_low_limit_record; i<n_rec_limit; i++)
	{
		j = 0;

		while (j<MAX_ENTRY_8)
		{

			fprintf (p_out_file, "%08x: %08x %08x %08x %08x %08x %08x %08x %08x\n",
				n_address, 

    /* 1 */	    	clsb_get_ref_count(p_mem_handle, i, j),
	/* 2 */			clsb_get_ref_count(p_mem_handle, i, j+1),                    
	/* 3 */			clsb_get_ref_count(p_mem_handle, i, j+2), 
	/* 4 */			clsb_get_ref_count(p_mem_handle, i, j+3), 
	/* 5 */			clsb_get_ref_count(p_mem_handle, i, j+4), 
	/* 6 */			clsb_get_ref_count(p_mem_handle, i, j+5), 
	/* 7 */			clsb_get_ref_count(p_mem_handle, i, j+6), 
	/* 8 */			clsb_get_ref_count(p_mem_handle, i, j+7));

			j += 8;
			n_address += 32;
		}
	}

	/* Record image */
	fprintf (p_out_file, "\nRECORD IMAGE\n");
	n_address = 0;
	for (i=p_mem_handle->n_low_limit_record; i<n_rec_limit; i++)
	{
		fprintf (p_out_file, "%08x: %08x %08x %08x %08x %08x %08x %08x %08x\n",
				n_address, 
	/* 1 */		CLS_REC_IMAGE_8(p_rec_image_base,BIT_TABLE_ENTRY((AG_U8) 0), i),
	/* 2 */		CLS_REC_IMAGE_8(p_rec_image_base,BIT_TABLE_ENTRY((AG_U8) 32), i),
	/* 3 */		CLS_REC_IMAGE_8(p_rec_image_base,BIT_TABLE_ENTRY((AG_U8) 64), i),
	/* 4 */		CLS_REC_IMAGE_8(p_rec_image_base,BIT_TABLE_ENTRY((AG_U8) 96), i),
	/* 5 */		CLS_REC_IMAGE_8(p_rec_image_base,BIT_TABLE_ENTRY((AG_U8) 128), i),
	/* 6 */		CLS_REC_IMAGE_8(p_rec_image_base,BIT_TABLE_ENTRY((AG_U8) 160), i),
	/* 7 */		CLS_REC_IMAGE_8(p_rec_image_base,BIT_TABLE_ENTRY((AG_U8) 192), i),
	/* 8 */		CLS_REC_IMAGE_8(p_rec_image_base,BIT_TABLE_ENTRY((AG_U8) 224), i));

		n_address += 32;

	}

	/* EXTENDED */
	fprintf (p_out_file, "\nEXTENDED\n");
	n_address = 0;
	for (i=p_mem_handle->n_low_limit_record; i<n_rec_limit; i++)
	{

		fprintf (p_out_file, "%08x: %08x %08x %08x %08x %08x %08x %08x %08x\n",
				n_address, 
	/* 1 */		IS_EXTENDED_8(p_extend_base,0,i,n_rec_limit),
	/* 2 */		IS_EXTENDED_8(p_extend_base,32,i,n_rec_limit),
	/* 3 */		IS_EXTENDED_8(p_extend_base,64,i,n_rec_limit),
	/* 4 */		IS_EXTENDED_8(p_extend_base,96,i,n_rec_limit),
	/* 5 */		IS_EXTENDED_8(p_extend_base,128,i,n_rec_limit),
	/* 6 */		IS_EXTENDED_8(p_extend_base,160,i,n_rec_limit),
	/* 7 */		IS_EXTENDED_8(p_extend_base,192,i,n_rec_limit),
	/* 8 */		IS_EXTENDED_8(p_extend_base,224,i,n_rec_limit));

		n_address += 32;
	}

	/* Empty Entries */

	fprintf (p_out_file, "\nEMPTY ENTRIES\n");
	for (i=p_mem_handle->n_low_limit_record; i<n_rec_limit; i++)
	{
		fprintf (p_out_file, "record number %d;  number of empty entries %d\n", i, p_empty_entries[i]);
	}

	fclose (p_out_file);
	return AG_S_OK;
}

/*********************************************************************************
	Begin of Print Area - Only prints if CLS_PRINT DEBUG IS DEFINED
*********************************************************************************/
/****************************************************************************/
/* The following function parse the tables and output them to an ascii file */
/****************************************************************************/
AgResult dump_8 (const AG_CHAR *output_file , StdPrg *x_std_prg)
{
FILE *of; /* output file   */
AG_U32 entry_content;
AG_U32 cr = x_std_prg->p_mem_handle->n_low_limit_record;
AG_U32 entry;

	of = fopen (output_file,"w"); /* Opens the input file */
	if ( of == NULL )
	{
		printf ("\n Can't open %s ",output_file);
		return (AG_E_FAIL);
	} /* of if */

	fprintf (of,"Default path tag is %d Default records is %d\n",x_std_prg->x_root.def_pt,x_std_prg->x_root.def_rec);
	do
	{
		fprintf (of,"\nTable number %8xH\n",cr);
		fprintf (of,"entry	OpCode	Path tag	Next record	Next path tag	Reference count\n");
		for (entry = 0; entry < MAX_ENTRY_8; entry ++)
			{
			if ((entry_content = CLS_RECORDS_8_GET(x_std_prg->p_mem_handle,entry,0,cr)) != EMPTY_ENTRY_8)
				fprintf (of,"%8xH\t\t%8xH\t\t%8xH\t\t\t%8xH\t\t\t%8xH\n", entry 
													   , (entry_content & OP_CODE_MASK_8) >> 29 
													   , (entry_content & PATH_TAG_MASK_8) >> 21 
													   , (entry_content & NEXT_RECORD_MASK_8) >> 8 
													   , (entry_content & NEXT_PATH_TAG_MASK_8));
			} /* of for */
		fprintf (of,"\nDefault table number %8xH\n",cr);

		for (entry = 0; entry < MAX_ENTRY_8; entry ++)
			{
			if ((entry_content = CLS_RECORDS_8_GET(x_std_prg->p_mem_handle,entry,1,cr)) != EMPTY_ENTRY_8)
				fprintf (of,"%8xH\t\t%8xH\t\t%8xH\t\t\t%8xH\t\t\t%8xH\t\t\t%8xH\n", entry 
													   , (entry_content & OP_CODE_MASK_8) >> 29 
													   , (entry_content & PATH_TAG_MASK_8) >> 21 
													   , (entry_content & NEXT_RECORD_MASK_8) >> 8 
													   , (entry_content & NEXT_PATH_TAG_MASK_8)
													   , clsb_get_ref_count(x_std_prg->p_mem_handle, cr, entry));
			} /* of for */

	} /* of while */

	while (++cr <= x_std_prg->p_mem_handle->n_last_record);

	/*MBFILE - 01/04/2001 close the file*/
	fclose(of);
	return AG_S_OK;

}	/* of dump_8 */
/*********************************************************
Name: 
    dump_16
Description:
   	Prints the tables.
Parameters:

Returns AgResult:
    Success: AG_S_OK
	Fail : AG_E_FAIL -can't open file
 ***********************************************************/
AgResult dump_16(const AG_CHAR *output_file , StdPrg *x_std_prg)
{
FILE *of; /* output file   */
AG_U32 entry_content;
AG_U32 cr = x_std_prg->p_mem_handle->n_low_limit_record;
AG_U32 entry;

	of = fopen (output_file,"w"); /* Opens the input file */
	if ( of == NULL )
	{
		printf ("\n Can't open %s ",output_file);
		return (AG_E_FAIL);
	} /* of if */

	fprintf (of,"Default path tag is %d Default records is %d\n",x_std_prg->x_root.def_pt,x_std_prg->x_root.def_rec);
	do
	{
		fprintf (of,"\nTable number %8xH\n",cr);
		fprintf (of,"entry	OpCode	Mask	Next record	Next path tag	Reference count\n");
		for (entry = 0; entry < MAX_ENTRY_16; entry ++)
		{
	         if ((entry_content = CLS_RECORDS_16(x_std_prg->p_mem_handle->p_record_base ,entry,cr)) != EMPTY_ENTRY_16)/*[entry][cr]*/
			 {
				 if((entry%4)==0)/*PT entry*/
				 {
					 if(entry_content!=CLS_ALL_ONES)
					 {
						 fprintf (of,"%8xH\t\tPT1=%8xH\t\tPT0=%8xH\n", entry 
										    
										   , (entry_content & PATH_TAG_1_MASK_16>>1) >> 16 
										   ,(entry_content & PATH_TAG_0_MASK_16>>1) );
					 }					   
				 }
				 else
				 {
					 if(((entry-3)%4)==0)/*Default entry */
					 {
						 fprintf (of,"%8xH(%8xH)\t%8xH\t\t%8xH\t\t\t%8xH\t\t\t%8xH\t\t\t%8xH\n", entry,(entry-3)/4 
													   , (entry_content & OP_CODE_MASK_16) >> 28 
													   , (entry_content & MSK_MASK_16) >> 24 
													   , (entry_content & NEXT_RECORD_MASK_16) >> 16 
													   , (entry_content & NEXT_PATH_TAG_MASK_16)
													   ,  CLS_REF_COUNT_16(x_std_prg->p_mem_handle->p_ref_count,(entry-3)/4,cr, x_std_prg->p_mem_handle->n_record_limit));
													   
					 }
					 else /*normal entry */
					 {
						 fprintf (of,"%8xH\t\t%8xH\t\t%8xH\t\t\t%8xH\t\t\t%8xH\n", entry 
													   , (entry_content & OP_CODE_MASK_16) >> 28 
													   , (entry_content & MSK_MASK_16) >> 24 
													   , (entry_content & NEXT_RECORD_MASK_16) >> 16 
													   , (entry_content & NEXT_PATH_TAG_MASK_16));

					 }
				 }
			 } /* of if */
		} /* of for */
	} /* of while */

	while (++cr <= x_std_prg->p_mem_handle->n_last_record);

	/*MBFILE - 01/04/2001 close the file*/
	fclose(of);
	return AG_S_OK;


}/* of dump_16*/

/****************************************************************************/
/* The following function parse the tables and output them to an ascii file */
/****************************************************************************/
AgResult dump_vl_8 (const AG_CHAR *output_file,StdPrg *x_std_prg)
{
FILE *of; /* output file   */
AG_U32 entry_content;
AG_U32 cr = x_std_prg->p_mem_handle->n_low_limit_record;
AG_U32 entry;

	of = fopen (output_file,"w"); /* Opens the input file */
	if ( of == NULL )
	{
		printf ("\n Can't open %s ",output_file);
		return (AG_E_FAIL);
	} /* of if */

	fprintf (of,"\n%d /* Default record \n",x_std_prg->x_root.def_rec);	/* Default record*/ */
	fprintf (of,"\n%d /* Default path tag \n",x_std_prg->x_root.def_pt);		/* Default path tag*/ */
	fprintf (of,"\n%d /* Default opcode \n",x_std_prg->x_root.n_cls_def_opc);/* Default opcode*/ */
	do
	{
		fprintf (of,"\n /* Table number %8x\n",cr); */
		for (entry = 0; entry < MAX_ENTRY_8; entry ++)
		{
			if ((entry_content = CLS_RECORDS_8_GET(x_std_prg->p_mem_handle,entry,0,cr)) != EMPTY_ENTRY_8)/*[entry][0][cr]*/
				fprintf (of,"%8x\t%8x\n", entry + (cr*512),entry_content);
		} /* of for */
		fprintf (of,"\n /* Default table number %8x\n",cr); */

		for (entry = 0; entry < MAX_ENTRY_8; entry ++)
		{
			if ((entry_content = CLS_RECORDS_8_GET(x_std_prg->p_mem_handle,entry,1,cr)) != EMPTY_ENTRY_8)/*[entry][1][cr]*/
			{
				fprintf (of,"%8x\t%8x\n",entry + (cr*512) + 256, entry_content);
			}

		} /* of for */

	} /* of while */

	while (cr++ <= x_std_prg->p_mem_handle->n_last_record);

	/*MBFILE - 01/04/2001 close the file*/
	fclose(of);

    return AG_S_OK;

}	/* of dump_vl */


/****************************************************************************/
/* The following function parse the tables and output them to an ascii file */
/****************************************************************************/
AgResult dump_vl_16 (const AG_CHAR *output_file,StdPrg *x_std_prg)
{
FILE *of; /* output file   */
AG_U32 entry_content;
AG_U32 cr = x_std_prg->p_mem_handle->n_low_limit_record;
AG_U32 entry;

	of = fopen (output_file,"w"); /* Opens the input file */
	if ( of == NULL )
	{
		printf ("\n Can't open %s ",output_file);
		return (AG_E_FAIL);
	} /* of if */

	fprintf (of,"\n%d /* Default record \n",x_std_prg->x_root.def_rec);	// Default record */
	fprintf (of,"\n%d /* Default path tag \n",x_std_prg->x_root.def_pt);		// Default path tag */
	fprintf (of,"\n%d /* Default opcode \n",x_std_prg->x_root.n_cls_def_opc);// Default opcode */
	fprintf (of,"\n%d /* Default mask \n",x_std_prg->x_root.n_cls_def_opc);// Default opcode */
	do
	{
		fprintf (of,"\n /* Table number %8x\n",cr); */
		for (entry = 0; entry < MAX_ENTRY_16; entry ++)
		{
			if ((entry_content = CLS_RECORDS_16(x_std_prg->p_mem_handle->p_record_base,entry,cr)) != EMPTY_ENTRY_16)/*[entry][cr]*/
				fprintf (of,"%8x\t%8x\n", entry + (cr*MAX_ENTRY_16),entry_content);
		} /* of for */

	} /* of while */
	while (++cr <= x_std_prg->p_mem_handle->n_last_record);

	/*MBFILE - 01/04/2001 close the file*/
	fclose(of);
    return AG_S_OK;

}	/* of dump_vl */

/****************************************************************************/
/* The following function parse the tables and output them to an ascii file */
/****************************************************************************/
AgResult dump_bin_8 (const AG_CHAR *output_file , StdPrg *x_std_prg)
{
FILE *of; /* output file   */
AG_U32 cr = x_std_prg->p_mem_handle->n_low_limit_record;
int entry;
AG_U32 n_size;
AG_U32 n_aux_val;
StdPrg x_aux_root;
AG_BOOL b_little_endian = ag_is_little_endian();



	of = fopen (output_file,"wb"); /* Opens the input file */
	if ( of == NULL )
	{
		printf ("\n Can't open %s ",output_file);
		return (AG_E_FAIL);
	} /* of if */
	(n_size)=(x_std_prg->p_mem_handle->n_last_record+1)*2*MAX_ENTRY_8*sizeof(AG_U32);

	/*update auxiliary variables*/
	n_aux_val=AG_CLS_BIT_MODE_8;
	x_aux_root.x_root.def_rec=x_std_prg->x_root.def_rec;
	x_aux_root.x_root.def_pt=x_std_prg->x_root.def_pt;

	/*Revert 16 and 32 bit fields*/
	if(!b_little_endian)
	{

		ag_u32_revert(&n_aux_val);

		ag_u32_revert(&n_size);

		ag_u16_revert(&(x_aux_root.x_root.def_rec));

		ag_u16_revert(&(x_aux_root.x_root.def_pt));

	}

	fwrite(&n_aux_val,4,1,of);
	fwrite(&n_size,4,1,of);
	fwrite(&(x_aux_root.x_root.def_rec),2,1,of);
	fwrite(&(x_aux_root.x_root.def_pt),2,1,of);
	fwrite(&(x_std_prg->x_root.n_cls_def_opc),1,1,of);
	fwrite(&(x_std_prg->x_root.def_mask),1,1,of);


	do
	{

		for (entry = 0; entry < MAX_ENTRY_8; entry ++)
			{
				n_aux_val=CLS_RECORDS_8_GET(x_std_prg->p_mem_handle,entry,0,cr);/*[entry][0][cr];*/
				if(!b_little_endian)
				{
					ag_u32_revert(&n_aux_val);
				}

				/*fprintf (of, "%08x ", n_aux_val); */
				fwrite (&n_aux_val,4,1,of);
			} /* of for */
		for (entry = 0; entry < MAX_ENTRY_8; entry ++)
			{
				n_aux_val=CLS_RECORDS_8_GET(x_std_prg->p_mem_handle,entry,1,cr);/*[entry][1][cr];*/
				if(!b_little_endian)
				{
					ag_u32_revert(&n_aux_val);
				}

/*				fprintf (of, "%08x ", n_aux_val); */

				fwrite (&n_aux_val,4,1,of);
			} /* of for */

	} /* of while */
	while (++cr <= x_std_prg->p_mem_handle->n_last_record);

	/*MBFILE - 01/04/2001 close the file*/
	fclose(of);
	return AG_S_OK;
}	/* of dump_bin_8 */
/*********************************************************
Name: 
    dump_bin_16
Description:
   	Prints the tables.
Parameters:

Returns AgResult:
    Success: AG_S_OK
	Fail : AG_E_FAIL -can't open file
 ***********************************************************/
AgResult dump_bin_16(const AG_CHAR *output_file , StdPrg *x_std_prg)
{
FILE *of; /* output file   */
AG_U32 cr = x_std_prg->p_mem_handle->n_low_limit_record;
int entry;
AG_U32 n_size;
AG_U32 n_aux_val;
StdPrg x_aux_root;
AG_BOOL b_little_endian = ag_is_little_endian();


	of = fopen (output_file,"wb"); /* Opens the input file */
	if ( of == NULL )
	{
		printf ("\n Can't open %s ",output_file);
		return (AG_E_FAIL);
	} /* of if */
	(n_size)=(x_std_prg->p_mem_handle->n_last_record+1)*MAX_ENTRY_16*sizeof(AG_U32);

	/*update auxiliary variables*/
	n_aux_val=AG_CLS_BIT_MODE_16;
	x_aux_root.x_root.def_rec=x_std_prg->x_root.def_rec;
	x_aux_root.x_root.def_pt=x_std_prg->x_root.def_pt;

	/*Revert 16 and 32 bit fields*/
	if(!b_little_endian)
	{

		ag_u32_revert(&n_aux_val);

		ag_u32_revert(&n_size);

		ag_u16_revert(&(x_aux_root.x_root.def_rec));

		ag_u16_revert(&(x_aux_root.x_root.def_pt));

	}

	fwrite(&n_aux_val,4,1,of);
	fwrite(&n_size,4,1,of);
	fwrite(&(x_aux_root.x_root.def_rec),2,1,of);
	fwrite(&(x_aux_root.x_root.def_pt),2,1,of);
	fwrite(&(x_std_prg->x_root.n_cls_def_opc),1,1,of);
	fwrite(&(x_std_prg->x_root.def_mask),1,1,of);


	do
	{
		for (entry = 0; entry < MAX_ENTRY_16; entry ++)
			{
				n_aux_val=CLS_RECORDS_16(x_std_prg->p_mem_handle->p_record_base,entry,cr);/*[entry][cr]*/

				if(!b_little_endian)
				{
					ag_u32_revert(&n_aux_val);
				}

				fwrite (&n_aux_val,4,1,of);
			} /* of for */



	} /* of while */
	while(++cr <= x_std_prg->p_mem_handle->n_last_record);

	/*MBFILE - 01/04/2001 close the file*/
	fclose(of);

	return AG_S_OK;

}

/*********************************************************************************
  End of Print Area - Only prints if CLS_PRINT DEBUG IS DEFINED
*********************************************************************************/
#endif /* AG_MNT */


#undef CLS_STD_ENGINE_C
