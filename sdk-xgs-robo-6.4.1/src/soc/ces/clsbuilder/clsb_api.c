/*******************************************************************

Copyright Redux Communications Ltd.

Module Name: 

File Name: 
    clsb_api.c

File Description: 

$Revision: 1.6 $ - Visual SourceSafe revision number 

History:
    Date            Name            Comment
    ----            ----            -------
    17/05/01
	4/11/2002		Raanan Refua	allocation of p_ones_pos_in_pattern for more efficient pattern matching
*******************************************************************/
#define CLSB_API_C 
#define REVISION "$Revision: 1.6 $" /* Clearcase revision number to be used in TRACE & SWERR */

#include "ag_common.h"
#ifndef BCM_CES_SDK
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "drivers/ag_hw_regs.h"
#else
#include "bcm_ces_sal.h"
#endif
#include "utils/memutils.h"
#include "classification/clsb_api.h"
#include "pub/cls_engine.h"
#include "pub/cls_mp_handler.h"
#include "pub/cls_stack.h"
#include "pub/clsb_types_priv.h"


/*#include "classification/cls_mem_mng.hpp" */
/*#include "classification/serialize.hpp" */
/*#include "utils/agos_utils.hpp" */


#define OPT_COPY_SIZE  32  /*optimum memcpy size*/

/***************************/
/* static functions        */
/***************************/
#ifndef CLS_16_BIT
static AgResult ag_clsb_alloc_aux_mem_8 (AgClsbMem* p_cls_mem);
static void ag_clsb_reset_eng_memory_8(AgClsbMem *p_cls_mem);
#endif

#ifndef CLS_8_BIT
static AgResult ag_clsb_alloc_aux_mem_16 (AgClsbMem* p_cls_mem);
static void ag_clsb_reset_eng_memory_16(AgClsbMem *p_cls_mem);
#endif

static void ag_clsb_free_aux_mem(AgClsbMem* p_cls_mem);


void ag_clsb_print_mem_usage()
{
#ifdef AG_MEM_MONITOR
	AGOS_TRACE(AGOS_TRACE_LEVEL1, "MiniPrg allocation: sizeof= max= curr=", 
        sizeof(MiniPrgDesc),nMaxMpAlloc ,nCurrMpAlloc,0,0);
	AGOS_TRACE(AGOS_TRACE_LEVEL1, "Group allocation: sizeof= max= curr=", 
        sizeof(LeafGrp),nMaxGrpAlloc ,nCurrGrpAlloc,0,0);
	AGOS_TRACE(AGOS_TRACE_LEVEL1, "Leaf allocation: sizeof= max= curr=", 
        sizeof(Leaf), nMaxLeafAlloc ,nCurrLeafAlloc,0,0);
	AGOS_TRACE(AGOS_TRACE_LEVEL1, "Leaf Pointer allocation: sizeof= max= curr=", 
        sizeof(LeafPtrEl), nMaxLeafPtrAlloc ,nCurrLeafPtrAlloc,0,0);
	AGOS_TRACE(AGOS_TRACE_LEVEL1, "Group Pointer allocation: sizeof= max= curr=", 
        sizeof(LeafGrpPtrEl), nMaxGrpPtrAlloc ,nCurrGrpPtrAlloc,0,0);
     
#else
	AGOS_TRACE(AGOS_TRACE_LEVEL1, "ag_clsb_get_mem_usage: memory monitor is not supported in this compilation version",0,0,0,0,0);
#endif
}

AgResult ag_clsb_get_hw_mem_utilization(AgClsbMemHndl n_cls_mem_handle, AgClsbHwMemUtil* p_utilization)
{
    if(!CLSB_VALID_MEM_HNDL(n_cls_mem_handle) || !p_utilization)
    {
        return AG_E_BAD_PARAMS;
    }
    
    clsb_eng_utilization(aClsMem[n_cls_mem_handle], 
						   &p_utilization->n_empty_rec, 
						   &p_utilization->n_non_empty_rec, 
						   &p_utilization->n_pri_compr_ratio,
						   &p_utilization->n_def_compr_ratio,
						   &p_utilization->n_comp_ratio);
    return AG_S_OK;
}


/****************************************************************
Name: ag_clsb_get_hw_mem_usage
Description: 
Parameters:
	AgClsbMemHndl n_cls_mem_handle:
	AgClsbHwMemUse* p_usage:
Returns AgResult :
****************************************************************/
AgResult ag_clsb_get_hw_mem_usage(AgClsbMemHndl n_cls_mem_handle, AgClsbHwMemUse* p_usage)
{
    if(!CLSB_VALID_MEM_HNDL(n_cls_mem_handle) || !p_usage)
    {
        return AG_E_BAD_PARAMS;
    }
    
    clsb_eng_usage(aClsMem[n_cls_mem_handle], 
						   &p_usage->n_empty_rec, 
						   &p_usage->n_non_empty_rec, 
						   &p_usage->n_non_empty_entry_pri,
						   &p_usage->n_non_empty_entry_def);
    return AG_S_OK;
}


/*********************************************************/
/* The following function allocate memory and initialize */
/* parameters used in the StdPrg structure 				 */
/*********************************************************/
AgResult ag_clsb_create_std_prg (AgClsbMemHndl n_cls_mem_handle , AG_U8 n_init_mask, AG_U8 n_init_opcode,  MpId *p_mpid, AG_U32 n_static_flag)
{
MiniPrgDesc *p_mp;
ClsbCurrState x_curr_state;
AgClsbMem *p_mem;
    
	x_curr_state.b_is_extended = AG_FALSE; /* needed for the find_next_ent function.*/

#ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "START: ag_clsb_create_std_prg\n");
	fprintf (full_trace, "\tParameters:\n\t\tn_cls_mem_handle %d\n\t\tdef_mask %x\n\t\tdef_opc %d\n\t\tstatic_flag %d\n", 
		n_cls_mem_handle, n_init_mask, n_init_opcode, n_static_flag);
#endif /* CLS_TEST_FULL_TRACE */

#ifndef NO_VALIDATION
    if(!CLSB_VALID_MEM_HNDL(n_cls_mem_handle))
    {

        #ifdef CLS_TEST_FULL_TRACE
    	fprintf (full_trace, "END: ag_clsb_create_std_prg (AG_E_BAD_PARAMS)\n");
        #endif /* CLS_TEST_FULL_TRACE */

        return AG_E_BAD_PARAMS;
    }
    if (!p_mpid)
    {

        #ifdef CLS_TEST_FULL_TRACE
	    fprintf (full_trace, "END: ag_clsb_create_std_prg (AG_E_NULL_PTR)\n");
        #endif /* CLS_TEST_FULL_TRACE */

		return AG_E_NULL_PTR;
    }
#endif /* ndef NO_VALIDATION  */    
    
    p_mem = aClsMem[n_cls_mem_handle];

	/* Allocate structure */
	if (AG_FAILED(alloc_mp(&p_mp)))
	{
        #ifdef CLS_TEST_FULL_TRACE
    	fprintf (full_trace, "END: ag_clsb_create_std_prg (AG_E_OUT_OF_MEM)\n");
        #endif /* CLS_TEST_FULL_TRACE */

		return AG_E_OUT_OF_MEM;
	}

    #ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "\t\t\t- allocate 64-bytes buffer for MiniPrgDesc p_mp\n");
    #endif /* CLS_TEST_FULL_TRACE */

/*	p_mp->x_stdprg.str_prg = AG_FALSE; */ /* Not a string program */

	/* Initialize to base of classification program boundaries */
	p_mp->x_stdprg.p_mem_handle = p_mem; 
    /* init the n_properties field */
    p_mp->n_properties = 0;

    /* Wait for semaphore */
#ifdef AG_NO_HL
	CAgBiSem x_sem(&p_mem->x_semaphore,CAgBiSem::LOCKED);  /* unlocks semaphore in destructor */
#endif

#ifndef CLS_16_BIT
    if(p_mem->n_cls_mode == AG_CLS_BIT_MODE_8)
    {
        #ifndef NO_VALIDATION    
	    if(( n_init_opcode>= AG_CLSB_OPC8_TAG)||
            (n_init_opcode==AG_CLSB_OPC8_INT_TAG))
        {

            #ifdef CLS_TEST_FULL_TRACE
            fprintf (full_trace, "END: ag_clsb_create_std_prg (AG_E_CLS_INVALID_OPC)\n");
            fprintf (full_trace, "\tParameters:\n\t\tmp_id %d\n", *p_mpid);
            fprintf (full_trace, "\tGlobals:\n\\ttStdPrg->def_pt %d\n\t\tStdPrg->def_rec %d\n",
	            p_mp->x_stdprg.x_root.def_pt, p_mp->x_stdprg.x_root.def_rec);
            #endif /* CLS_TEST_FULL_TRACE */

            free_mp(p_mp);
		    return AG_E_CLS_INVALID_OPC;
        }
        #endif /* ndef NO_VALIDATION*/

        p_mp->x_stdprg.x_root.def_mask = 0; /* mask is not used in 8 bit*/

        /* Find default PT and default record */
		if (n_static_flag == AG_CLSB_STATIC_PRG)
		{

			if (AG_FAILED(find_empty_rec_8(&x_curr_state, p_mem)))
			{

                #ifdef CLS_TEST_FULL_TRACE
	            fprintf (full_trace, "\t\t\t- release 64-bytes buffer of MiniPrgDesc p_mp\n");
	            fprintf (full_trace, "END: ag_clsb_create_std_prg (AG_E_FAIL)\n");
	            fprintf (full_trace, "\tParameters:\n\t\tmp_id %d\n", *p_mpid);
	            fprintf (full_trace, "\tGlobals:\n\t\tStdPrg->def_pt %d\n\t\tStdPrg->def_rec %d\n",
		            p_mp->x_stdprg.x_root.def_pt, p_mp->x_stdprg.x_root.def_rec);
                #endif /* CLS_TEST_FULL_TRACE */

                free_mp(p_mp);
				return AG_E_FAIL;
			}
		}
		else
        { /*not static program */
			if (AG_FAILED(find_next_ent_8(0 , 0, &x_curr_state, p_mem)))
			{

                #ifdef CLS_TEST_FULL_TRACE
                fprintf (full_trace, "\t\t\t- release 64-bytes buffer of MiniPrgDesc p_mp\n");
	            fprintf (full_trace, "END: ag_clsb_create_std_prg (AG_E_FAIL)\n");
	            fprintf (full_trace, "\tParameters:\n\t\tmp_id %d\n", *p_mpid);
	            fprintf (full_trace, "\tGlobals:\n\t\tStdPrg->def_pt %d\n\t\tStdPrg->def_rec %d\n",
		            p_mp->x_stdprg.x_root.def_pt, p_mp->x_stdprg.x_root.def_rec);
                #endif /* CLS_TEST_FULL_TRACE */

                free_mp(p_mp);
				return AG_E_FAIL;
			}
		}

    }
    else 
#endif /*ndef CLS_16_BIT*/

    { /* 16 bit */

#ifndef CLS_8_BIT
        #ifndef NO_VALIDATION
	    if((n_init_opcode >= AG_CLSB_OPC16_TAG) ||
            (n_init_opcode==AG_CLSB_OPC16_INT_TAG))
        {

            #ifdef CLS_TEST_FULL_TRACE
	        fprintf (full_trace, "END: ag_clsb_create_std_prg (AG_E_CLS_INVALID_OPC)\n");
	        fprintf (full_trace, "\tParameters:\n\t\tmp_id %d\n", *p_mpid);
	        fprintf (full_trace, "\tGlobals:\n\t\tStdPrg->def_pt %d\n\t\tStdPrg->def_rec %d\n",
		        p_mp->x_stdprg.x_root.def_pt, p_mp->x_stdprg.x_root.def_rec);
            #endif /* CLS_TEST_FULL_TRACE */

            free_mp(p_mp);
		    return AG_E_CLS_INVALID_OPC;
        }
       	if(n_init_mask > 0xF)
        {

            #ifdef CLS_TEST_FULL_TRACE
	        fprintf (full_trace, "END: ag_clsb_create_std_prg (AG_E_CLS_INVALID_MASK)\n");
	        fprintf (full_trace, "\tParameters:\n\t\tmp_id %d\n", *p_mpid);
	        fprintf (full_trace, "\tGlobals:\n\t\tStdPrg->def_pt %d\n\t\tStdPrg->def_rec %d\n",
		        p_mp->x_stdprg.x_root.def_pt, p_mp->x_stdprg.x_root.def_rec);
            #endif /* CLS_TEST_FULL_TRACE */

            free_mp(p_mp);
            return AG_E_CLS_INVALID_MASK;
        }
        #endif /*ndef NO_VALIDATION*/

        p_mp->x_stdprg.x_root.def_mask =n_init_mask; 

        /* Find default PT and default record */
		if (n_static_flag == AG_CLSB_STATIC_PRG)
		{

			if (AG_FAILED(find_empty_rec_16(&x_curr_state, p_mem)))
			{
    
                #ifdef CLS_TEST_FULL_TRACE
	            fprintf (full_trace, "\t\t\t- release 64-bytes buffer of MiniPrgDesc p_mp\n");
	            fprintf (full_trace, "END: ag_clsb_create_std_prg (AG_E_FAIL)\n");
	            fprintf (full_trace, "\tParameters:\n\t\tmp_id %d\n", *p_mpid);
	            fprintf (full_trace, "\tGlobals:\n\t\tStdPrg->def_pt %d\n\t\tStdPrg->def_rec %d\n",
		            p_mp->x_stdprg.x_root.def_pt, p_mp->x_stdprg.x_root.def_rec);
                #endif /* CLS_TEST_FULL_TRACE */
                
                free_mp(p_mp);
				return AG_E_FAIL;
			}


		}
		else
		{
			if (AG_FAILED(find_next_ent_16(0 , &x_curr_state, p_mem)))
			{
                #ifdef CLS_TEST_FULL_TRACE
	            fprintf (full_trace, "\t\t\t- release 64-bytes buffer of MiniPrgDesc p_mp\n");
	            fprintf (full_trace, "END: ag_clsb_create_std_prg (AG_E_FAIL)\n");
	            fprintf (full_trace, "\tParameters:\n\t\tmp_id %d\n", *p_mpid);
	            fprintf (full_trace, "\tGlobals:\n\t\tStdPrg->def_pt %d\n\t\tStdPrg->def_rec %d\n",
		            p_mp->x_stdprg.x_root.def_pt, p_mp->x_stdprg.x_root.def_rec);
                #endif /* CLS_TEST_FULL_TRACE */

                free_mp(p_mp);
				return AG_E_FAIL;
			}

		}

#endif /*ndef CLS_8_BIT*/

    } /* end of 16 bit */


    p_mp->x_stdprg.x_root.n_cls_def_opc = n_init_opcode;

    p_mp->x_stdprg.x_root.def_pt = x_curr_state.next_pt;
	p_mp->x_stdprg.x_root.def_rec = x_curr_state.next_rec;

	if(AG_FAILED(cls_create_mp (p_mp, p_mpid)))
    {
    
        #ifdef CLS_TEST_FULL_TRACE
	    fprintf (full_trace, "\t\t\t- release 64-bytes buffer of MiniPrgDesc p_mp\n");
	    fprintf (full_trace, "END: ag_clsb_create_std_prg (AG_E_FAIL)\n");
	    fprintf (full_trace, "\tParameters:\n\t\tmp_id %d\n", *p_mpid);
	    fprintf (full_trace, "\tGlobals:\n\t\tStdPrg->def_pt %d\n\t\tStdPrg->def_rec %d\n",
		    p_mp->x_stdprg.x_root.def_pt, p_mp->x_stdprg.x_root.def_rec);
        #endif /* CLS_TEST_FULL_TRACE */

        free_mp(p_mp);
		return AG_E_BAD_PARAMS;
    }

	if (n_static_flag == AG_CLSB_STATIC_PRG)
	{
        /*mark static for mph*/
        CLSB_SET_STATIC_MP(p_mp);

		/* mark default record as static */
		p_mem->p_static_rec_image[BIT_TABLE_ENTRY(x_curr_state.next_rec)]
			|= BIT_ENTRY_POS(x_curr_state.next_rec);

        #ifdef CLS_TEST_FULL_TRACE
        fprintf (full_trace, "\t\t\t- mark record %d as static \n", x_curr_state.next_rec);
        #endif /* CLS_TEST_FULL_TRACE */
    }

    #ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "END: ag_clsb_create_std_prg (S_OK)\n");
	fprintf (full_trace, "\tParameters:\n\t\tmp_id %d\n", *p_mpid);
	fprintf (full_trace, "\tGlobals:\n\t\tStdPrg->def_pt %d\n\t\tStdPrg->def_rec %d\n",
		p_mp->x_stdprg.x_root.def_pt, p_mp->x_stdprg.x_root.def_rec);
    #endif /* CLS_TEST_FULL_TRACE */

	n_init_mask = 0; /* dummy operation to avoid compilation warning. */

	return AG_S_OK;

} /* of cls_create_std_prg */

/*************************************************************/
/* The following function initialize a group woth a group id */
/*************************************************************/
AgResult ag_clsb_create_grp (LeafGrpId* p_grpid, AgClsbMemHndl n_cls_mem_handle)
{
	AgResult n_res;

    #ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "START: ag_clsb_create_grp\n");
	fprintf (full_trace, "\tParameters:\n\t\tn_cls_mem_handle %d\n", n_cls_mem_handle);
    #endif /* CLS_TEST_FULL_TRACE */

#ifndef NO_VALIDATION
    if(!CLSB_VALID_MEM_HNDL(n_cls_mem_handle))
    {
       
        #ifdef CLS_TEST_FULL_TRACE
        fprintf (full_trace, "END: ag_clsb_create_grp (AG_E_BAD_PARAMS)\n");
        #endif /* CLS_TEST_FULL_TRACE */

		return AG_E_BAD_PARAMS;
    }
#endif /*NOT NO_VALIDATION*/

#ifdef AG_NO_HL
	CAgBiSem x_sem(&aClsMem[n_cls_mem_handle]->x_semaphore,CAgBiSem::LOCKED);  /* unlocks semaphore in destructor */
#endif
	n_res = cls_create_leaf_grp (p_grpid , aClsMem[n_cls_mem_handle]);

    #ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "END: ag_clsb_create_grp id=%d, res=%08x\n",*p_grpid,n_res);
    #endif /* CLS_TEST_FULL_TRACE */

    return n_res;

} /* of cls_create_grp */


/****************************************************************
Name: ag_clsb_get_memory_handle
Description: 
Parameters:
	MpId x_mpid:
	AgClsbMemHndl* p_cls_mem_handle:
Returns AgResult :
****************************************************************/
AgResult ag_clsb_get_memory_handle(MpId x_mpid, AgClsbMemHndl* p_cls_mem_handle)
{
	MiniPrgDesc *p_mp; /* The mini program structure pointer */

	 AgResult x_res = cls_get_mpdesc(x_mpid , &p_mp);
#ifndef NO_VALIDATION
    if (AG_FAILED(x_res))
	{
		return AG_E_CLS_BAD_MP_ID;
	}
#endif /* NO_VALIDATION */

	*p_cls_mem_handle = p_mp->x_stdprg.p_mem_handle->x_mem_hndl;
	
	return AG_S_OK;
}
/********************************************************************/
/* The following function adds a path to a mini program and         */  
/* updates the program handler                                      */ 
/* MBEXT - 24/10/2000 - The function parameters were                */  
/* changed in order to support the intermediate tag                 */ 
/* MB16 -19/11/200 - Parameters Changed in order to support 16 bits */
/********************************************************************/
AgResult cls_add_path_8 ( MpId x_mpid ,  
						  AG_U8 *p_path ,  
						  AG_U8 *p_ctl,
						  AG_U32 n_pathlen ,  
						  AG_U32 *p_tag,  
						  AG_U32 n_taglen, 
						  LeafGrpId n_grp)
{
MiniPrgDesc *p_mp; /* The mini program structure pointer */
AgResult x_res;
ClsbCurrState x_currstate;
TableType e_table = CLS_PRI_TBL;
ClsbStack *p_old_stack;
ClsbStack *p_new_stack;
LeafGrp *p_grp;
AgClsbMem   *p_mem_handle;
AG_S32 n_org_new_stack_top;

    #ifdef CLS_TEST_FULL_TRACE
	AG_U16 i;
	fprintf (full_trace, "START: cls_add_path_8\n");
	fprintf (full_trace, "\tParameters:\n\t\tmpid %d\n\t\tpath:",x_mpid);
	for (i = 0; i < n_pathlen; i++)
    {
		fprintf (full_trace, " %02x", p_path[i]);
    }
	fprintf (full_trace, "\n\t\ttag %d\n\t\tgrp %d\n", *p_tag, n_grp);
    #endif /* CLS_TEST_FULL_TRACE */

    #ifndef NO_VALIDATION
	if((n_taglen==0)||(n_pathlen==0))
	{

        #ifdef CLS_TEST_FULL_TRACE
	    fprintf (full_trace, "END: cls_add_path_8 (AG_E_BAD_PARAMS)\n");
        #endif /* CLS_TEST_FULL_TRACE */

		return AG_E_BAD_PARAMS;
	}
    #endif
    
    x_res = cls_get_mpdesc(x_mpid , &p_mp);
    #ifndef NO_VALIDATION
    if (AG_FAILED(x_res))
	{

        #ifdef CLS_TEST_FULL_TRACE
        fprintf (full_trace, "END: cls_add_path_8 (AG_E_CLS_BAD_MP_ID)\n");
        #endif /* CLS_TEST_FULL_TRACE */

		return AG_E_CLS_BAD_MP_ID;
	}
    #endif

    if(n_grp != AG_CLSB_IGNORE_GRP)
    {	    
        x_res = cls_get_leaf_grp(n_grp,&p_grp);

        #ifndef NO_VALIDATION
        if(AG_FAILED(x_res))
        { 
            #ifdef CLS_TEST_FULL_TRACE
            fprintf (full_trace, "END: cls_add_path_8 (AG_E_CLS_BAD_GRP_ID)\n");
            #endif /* CLS_TEST_FULL_TRACE */

            return AG_E_CLS_BAD_GRP_ID;
        }
        if( p_mp->x_stdprg.p_mem_handle != p_grp->p_mem_handle)
        { /* classification memory areas not matching */

            #ifdef CLS_TEST_FULL_TRACE
            fprintf (full_trace, "END: cls_add_path_8 (AG_E_CLS_UNMATCHED_PRGS)\n");
            #endif /* CLS_TEST_FULL_TRACE */

            return AG_E_CLS_UNMATCHED_PRGS;
        }
        #endif /* ndef NO_VALIDATION */
                
    }

    p_mem_handle = p_mp->x_stdprg.p_mem_handle;
    p_old_stack = &p_mem_handle->x_old_stack;
    p_new_stack = &p_mem_handle->x_new_stack;

	/* Wait for semaphore */
#ifdef AG_NO_HL
	CAgBiSem x_sem(&p_mem_handle->x_semaphore,CAgBiSem::LOCKED);  /* unlocks semaphore in destructor */
#endif
	/* Stack should be initialized before "add path". 
	   The garbage can stay in the stack becouse of previous "add path" operation*/
    cls_stack_init(p_old_stack, p_new_stack);

	x_res = add_path_8(p_path, p_ctl, n_pathlen, p_tag, n_taglen, &p_mp->x_stdprg, &x_currstate);
	if(x_res==AG_S_CLS_PATH_EXIST) /* The added path already exists. */
	{

        #ifdef CLS_TEST_FULL_TRACE
        fprintf (full_trace, "END: cls_add_path_8 (AG_S_CLS_PATH_EXIST)\n");
        #endif /* CLS_TEST_FULL_TRACE */

		/* Reset stack. nothing was change in the memory */
	/*	cls_stack_init(p_stack); */

		/* Link path if needed */
		if(n_grp != AG_CLSB_IGNORE_GRP)
		{
			if (x_currstate.curr_table == DEF_TABLE)
				e_table = CLS_DEF_TBL;

			x_res = cls_add_mp_leaf(p_mp, p_grp,(AG_U32)x_currstate.curr_rec, 
									(AG_U32)x_currstate.curr_pt, e_table,
									(AG_U16)x_currstate.curr_data);
		}

		/* Release semaphore */
		return x_res;
	}

	if (x_res == AG_E_CLS_PATH_CONT)
    { /* the path exists but without a tag in its end. If the path is about to
         be linked implicitly by the MPH then it might not be an error. 
         Need to check it.*/
		if((n_grp != AG_CLSB_IGNORE_GRP)
			&& (p_grp->b_linked))
		{	/* Path continues in an other MP */
			if ((p_grp->p_linked_mp->x_stdprg.x_root.def_pt == x_currstate.next_pt)
				    && (p_grp->p_linked_mp->x_stdprg.x_root.def_rec == x_currstate.next_rec))
			    {
					/* Reset stack. nothing was change in the memory */
				/*	cls_stack_init(p_stack); */

				    /* Release semaphore */
				    return AG_S_OK;
			    }
                /* The path point to a node different from the linked MP node
                It is an error*/
		}
		/* Group ignore means the path must end with a tag. Since it does not,
			it is an error */
		/*the group is not linked, so the path cannot be linked implicitly.
			It is an error.*/
	}

	if (AG_FAILED(x_res))
	{
		/* Reset the stack and undo changes in the memory */
	/*	cls_reset_stack_8(p_old_stack, &p_mp->x_stdprg); */

		/* Reset the stack and undo changes in the memory */
		cls_reset_stack_8(p_new_stack, &p_mp->x_stdprg);

        #ifdef CLS_TEST_FULL_TRACE
        fprintf (full_trace, "END: cls_add_path_8 (%08x)\n", x_res);
        #endif /* CLS_TEST_FULL_TRACE */

		return x_res;
	}

	/* Save top of the stack */
	n_org_new_stack_top = p_new_stack->n_top;
    
	/* Write leaf from the stack */
	cls_eng_write_leaf_to_memory_8 (p_new_stack, p_mem_handle);

	if(n_grp != AG_CLSB_IGNORE_GRP)
    {
		if (x_currstate.curr_table == DEF_TABLE)
			e_table = CLS_DEF_TBL;

		if (AG_FAILED(cls_add_mp_leaf(p_mp, p_grp,(AG_U32)x_currstate.curr_rec, 
									(AG_U32)x_currstate.curr_pt, e_table,
									(AG_U16)x_currstate.curr_data)))
		{ 
            /* Undo the write of leaf entry */
			cls_undo_write_to_mem_8(p_new_stack, n_org_new_stack_top, &p_mp->x_stdprg);

		    /* Reset the rest of the stack (undo changes in the memory) */
		  /*  cls_reset_stack_8(p_old_stack, &p_mp->x_stdprg); */
			cls_reset_stack_8(p_new_stack, &p_mp->x_stdprg);
			return AG_E_FAIL;
		}
    }

	/* Write until stack value is empty (all new entries and copied entries) */
	cls_eng_write_until_empty_ent_to_memory_8 (p_new_stack, p_mem_handle);

	/* call cls_update_mp only if n_update_size>0 
	  (can be only one update from copy entries) */
	if(p_mem_handle->n_update_size)
    {
		x_res = cls_update_mp(p_mp , &p_mem_handle->x_node_update );
		if (AG_FAILED(x_res))
		{

            #ifdef CLS_TEST_FULL_TRACE
            fprintf (full_trace, "END: cls_add_path_8 (%08x)\n", x_res);
            #endif /* CLS_TEST_FULL_TRACE */

			/* Undo write of new and copied entries */
			cls_undo_write_to_mem_8(p_new_stack, n_org_new_stack_top, &p_mp->x_stdprg);

            /*no need to call reset_stack since only elements with
            entry == EMPTY_ENTRY are left in the stack.*/

			/* Release semaphore */
			return x_res;
		}
        
        /*Empty entries exist only of copy entries happened.*/
        cls_eng_write_empty_ent_to_memory_8 (p_old_stack, p_mem_handle);
    }

    #ifdef CLS_TEST_FULL_TRACE
    fprintf (full_trace, "END: cls_add_path_8 (%08x)\n", x_res);
    #endif /* CLS_TEST_FULL_TRACE */

	/* Release semaphore */
	return x_res;
}

/********************************************************************/
/* The following function adds a path to a mini program and         */
/* updates the program handler                                      */
/* MBEXT - 24/10/2000 - The function parameters were                */
/* changed in order to support the intermediate tag                 */  
/* MB16 -19/11/200 - Parameters Changed in order to support 16 bits */
/********************************************************************/
#ifdef CLS_16_BIT
AgResult cls_add_path_16 ( MpId x_mpid ,  
						   AG_U16 *p_path ,  
						   AG_U8 *p_ctl , 
						   AG_U8 *p_mask,
						   AG_U32 n_pathlen ,  
						   AG_U32 *p_tag,  
						   AG_U32 n_taglen, 
						   LeafGrpId n_grp)
{
MiniPrgDesc *p_mp; /* The mini program structure pointer */
AgResult x_res;
ClsbCurrState x_currstate;
TableType e_table = CLS_PRI_TBL;
ClsbStack *p_old_stack;
ClsbStack *p_new_stack;
LeafGrp *p_grp;
AgClsbMem   *p_mem_handle;

    #ifndef NO_VALIDATION
	if((n_taglen==0)||(n_pathlen==0))
	{
		return AG_E_BAD_PARAMS;
	}
    #endif

    x_res = cls_get_mpdesc(x_mpid , &p_mp);
    #ifndef NO_VALIDATION
    if (AG_FAILED(x_res))
	{
		return AG_E_CLS_BAD_MP_ID;
	}
    #endif /*not NO_VALIDATION*/

    if(n_grp != AG_CLSB_IGNORE_GRP)
    {
        x_res = cls_get_leaf_grp(n_grp,&p_grp);
        
        #ifndef NO_VALIDATION
        if(AG_FAILED(x_res))
        { 
            return AG_E_CLS_BAD_GRP_ID;
        }

        if(p_mp->x_stdprg.p_mem_handle != p_grp->p_mem_handle)
        { /* classification memory areas not matching */
            return AG_E_CLS_UNMATCHED_PRGS;
        }
        /* matching classification programs */
        #endif /*not NO_VALIDATION*/
    }
    
    p_mem_handle = p_mp->x_stdprg.p_mem_handle;
    p_old_stack = &p_mem_handle->x_old_stack;
    p_new_stack = &p_mem_handle->x_new_stack;

	/* Wait for semaphore */
#ifdef AG_NO_HL
	CAgBiSem x_sem(&p_mem_handle->x_semaphore,CAgBiSem::LOCKED);  /* unlocks semaphore in destructor */
#endif
    cls_stack_init(p_old_stack, p_new_stack);

	x_res = add_path_16(p_path , p_ctl ,p_mask, n_pathlen ,p_tag ,n_taglen, &p_mp->x_stdprg , &x_currstate);

	if(x_res==AG_S_CLS_PATH_EXIST)/*MB16 03/12/2000 -  two similar paths. */
	{
		/* Release semaphore */
		return x_res;
	}

	if (x_res == AG_E_CLS_PATH_CONT)
	{
		if(n_grp != AG_CLSB_IGNORE_GRP)
		{
            if(p_grp->b_linked)
            {
			    /* Path continues in an other MP */
			    if ((p_grp->p_linked_mp->x_stdprg.x_root.def_pt == x_currstate.next_pt)
				    && (p_grp->p_linked_mp->x_stdprg.x_root.def_rec == x_currstate.next_rec))
			    {
				    return AG_S_OK;
			    }
            }
		}

		return x_res;
	}

	if (AG_FAILED(x_res))
	{
		cls_reset_stack_16(p_new_stack, &p_mp->x_stdprg);
		cls_reset_stack_16(p_old_stack, &p_mp->x_stdprg);

		return x_res;
	}
	cls_write_to_memory_16(p_new_stack, p_mem_handle);
	cls_write_to_memory_16(p_old_stack, p_mem_handle);

	if(p_mem_handle->n_update_size)
    {
		x_res = cls_update_mp(p_mp , &(p_mem_handle->x_node_update) );
		if (AG_FAILED(x_res))
		{
			return x_res;
		}
    }
    if(n_grp != AG_CLSB_IGNORE_GRP)
    {
		if (x_currstate.curr_table == DEF_TABLE)
			e_table = CLS_DEF_TBL;

		x_res = cls_add_mp_leaf(p_mp , p_grp ,(AG_U32)x_currstate.curr_rec , 
                                (AG_U32)x_currstate.curr_pt ,
								e_table , (AG_U16)x_currstate.curr_data);
    }

	/* Release semaphore */
	return x_res;
} /* of cls_add_path */
#endif /*CLS_16_BIT*/
/* Irena 31/5/2001 */
AgResult ag_clsb_query_path_8 (MpId x_mpid, 
							   AG_U8 *p_frame, 
							   AG_U32 n_frame_len, 
							   AG_BOOL n_match_flag, 
							   AG_U32 *p_tag, 
							   AG_U32 *p_pos)
{
	AgResult x_res;
	MiniPrgDesc* p_mp;

    #ifdef CLS_TEST_FULL_TRACE
	AG_U16 i;

	fprintf (full_trace, "START: ag_clsb_query_path_8 \n");
	fprintf (full_trace, "\tParameters:\n\t\tmpid %d\n\t\tpath:", x_mpid);
	for (i = 0; i < n_frame_len; i++)
		fprintf (full_trace, " %02x", p_frame[i]);
	fprintf (full_trace, "\n");

    #endif /* CLS_TEST_FULL_TRACE */
    
    x_res = cls_get_mpdesc(x_mpid , &p_mp);
    #ifndef NO_VALIDATION
    if(AG_FAILED(x_res))
    {

        #ifdef CLS_TEST_FULL_TRACE
	    fprintf (full_trace, "END: ag_clsb_query_path_8 (AG_E_CLS_BAD_MP_ID)\n");
        #endif /* CLS_TEST_FULL_TRACE */

		return AG_E_CLS_BAD_MP_ID;
    }
    #endif

	/* Wait for semaphore */
#ifdef AG_NO_HL
	CAgBiSem x_sem(&p_mp->x_stdprg.p_mem_handle->x_semaphore,CAgBiSem::LOCKED);  /* unlocks semaphore in destructor */
#endif
	x_res = query_path_8(&p_mp->x_stdprg, p_frame, n_frame_len, n_match_flag, p_tag, p_pos);

    #ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "END: ag_clsb_query_path_8 (%08x)\n", x_res);
	fprintf (full_trace, "\tReturned parameters:\n\t\ttag %d\n\t\tpos %d\n", *p_tag, *p_pos);
    #endif /* CLS_TEST_FULL_TRACE */

	/* Release semaphore */
	return x_res;
}

#ifdef CLS_16_BIT
/* Irena 31/5/2001 */
AgResult ag_clsb_query_path_16 (MpId x_mpid, 
								AG_U16 *p_frame, 
								AG_U32 n_frame_len, 
								AG_BOOL n_match_flag, 
								AG_U32 *p_tag, 
								AG_U32 *p_pos)
{
	AgResult x_res;
	MiniPrgDesc* p_mp;

    x_res = cls_get_mpdesc(x_mpid , &p_mp);
    #ifndef NO_VALIDATION
    if(AG_FAILED(x_res))
    {
		return AG_E_CLS_BAD_MP_ID;
    }
    #endif
	/* Wait for semaphore */
#ifdef AG_NO_HL
	CAgBiSem x_sem(&p_mp->x_stdprg.p_mem_handle->x_semaphore,CAgBiSem::LOCKED);  /* unlocks semaphore in destructor */
#endif

	x_res = query_path_16(&p_mp->x_stdprg, p_frame, n_frame_len, n_match_flag, p_tag, p_pos);

	/* Release semaphore */
	return x_res;
}
#endif /*CLS_16_BIT*/
/*************************************************************/
/* The following function deletes a path from a mini program */
/* and updates the program handler							 */
/************************************************************/
/* Irena 22/5/2001 add two returned parameters */
AgResult cls_delete_path_8 ( MpId x_mpid, 
							 AG_U8 *p_path, 
							 AG_U8 *p_ctl, 
							 AG_U32 n_pathlen,  
							 LeafGrpId n_grp, 
							 AG_U32 *p_erase_pos, 
							 AG_U16 *p_ref_count)
{
MiniPrgDesc* p_mp; 
AgResult x_res;
LeafGrp *p_grp;
AgClsbMem   *p_mem_handle;
Leaf x_leaf;
ClsbDelPoint x_del_point;


    #ifdef CLS_TEST_FULL_TRACE
	AG_U16 i;
	fprintf (full_trace, "START: cls_delete_path_8\n");
	fprintf (full_trace, "\tParameters:\n\t\tmpid %d\n\t\\tpath:", x_mpid);

	for (i = 0; i < n_pathlen; i++)
		fprintf (full_trace, " %02x", p_path[i]);

	fprintf (full_trace, "\n\t\tgrp %d\n", n_grp);
    #endif /* CLS_TEST_FULL_TRACE */

    #ifndef NO_VALIDATION
	if(n_pathlen==0)
	{

        #ifdef CLS_TEST_FULL_TRACE
        fprintf (full_trace, "END : cls_delete_path_8 (AG_E_BAD_PARAMS)\n");
        #endif /* CLS_TEST_FULL_TRACE */

		return AG_E_BAD_PARAMS;
	}
    #endif

    x_res = cls_get_mpdesc(x_mpid , &p_mp);
    #ifndef NO_VALIDATION
    if (AG_FAILED(x_res))
	{

        #ifdef CLS_TEST_FULL_TRACE
        fprintf (full_trace, "END : cls_delete_path_8 (AG_E_CLS_BAD_MP_ID)\n");
        #endif /* CLS_TEST_FULL_TRACE */

		return AG_E_CLS_BAD_MP_ID;
	}
    #endif    

    if(n_grp != AG_CLSB_IGNORE_GRP)
    {	    
        x_res = cls_get_leaf_grp(n_grp,&p_grp);

        #ifndef NO_VALIDATION
        if(AG_FAILED(x_res))
        { 

            #ifdef CLS_TEST_FULL_TRACE
            fprintf (full_trace, "END : cls_delete_path_8 (AG_E_CLS_BAD_GRP_ID)\n");
            #endif /* CLS_TEST_FULL_TRACE */

            return AG_E_CLS_BAD_GRP_ID;
        }
        if(p_mp->x_stdprg.p_mem_handle != p_grp->p_mem_handle)
        { /* classification memory areas not matching */

            #ifdef CLS_TEST_FULL_TRACE
            fprintf (full_trace, "END : cls_delete_path_8 (AG_E_CLS_UNMATCHED_PRGS)\n");
            #endif /* CLS_TEST_FULL_TRACE */

			return AG_E_CLS_UNMATCHED_PRGS;
        }
        #endif
        /* matching classification programs */
    }

    p_mem_handle = p_mp->x_stdprg.p_mem_handle;
	/* Wait for semaphore */
#ifdef AG_NO_HL
	CAgBiSem x_sem(&p_mem_handle->x_semaphore,CAgBiSem::LOCKED);  /* unlocks semaphore in destructor */
#endif
    x_res = validate_delete_path_8 (p_path, p_ctl, n_pathlen , &p_mp->x_stdprg,
                                    &x_leaf, &x_del_point);

	if (AG_FAILED(x_res))
	{

        #ifdef CLS_TEST_FULL_TRACE
        fprintf (full_trace, "END : cls_delete_path_8 (%08x)\n", x_res);
        #endif /* CLS_TEST_FULL_TRACE */

		/* Release semaphore */
		return x_res;
	}

    if(n_grp != AG_CLSB_IGNORE_GRP)
    {
		x_res = cls_del_mp_leaf(p_mp , p_grp ,x_leaf.n_rec, x_leaf.n_pt ,
							x_leaf.e_table , x_leaf.n_data);
        if(AG_FAILED(x_res))
        {	        
            #ifdef CLS_TEST_FULL_TRACE
            fprintf (full_trace, "END : cls_delete_path_8 - mph failure (%08x)\n", x_res);
            #endif /* CLS_TEST_FULL_TRACE */
	        
            /* Release semaphore */
	        return x_res;
        }
    }

	/* Delete the path from the mini program */
	delete_path_8(p_path, p_ctl, n_pathlen, &p_mp->x_stdprg, &x_del_point);

	if (p_ref_count)
	{
		*p_ref_count = clsb_get_ref_count(p_mem_handle, x_del_point.n_rec, x_del_point.n_pt);
	    #ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\tReturned Parameters:\n\t\tref counter %d\n",
			*p_ref_count);
		#endif /* CLS_TEST_FULL_TRACE */

	}

	if (p_erase_pos)
	{
		*p_erase_pos = x_del_point.n_pos;
	    #ifdef CLS_TEST_FULL_TRACE
		fprintf (full_trace, "\tReturned Parameters:\n\t\terase_pos %d\n",
			*p_erase_pos);
		#endif /* CLS_TEST_FULL_TRACE */

    }

    #ifdef CLS_TEST_FULL_TRACE
    fprintf (full_trace, "END : cls_delete_path_8 (%08x)\n", x_res);
    #endif /* CLS_TEST_FULL_TRACE */

	/* Release semaphore */
	return AG_S_OK;
} /* of cls_delete_path */

/*************************************************************/
/* The following function deletes a path from a mini program */
/* and updates the program handler							 */
/************************************************************/
#ifdef CLS_16_BIT
/* Irena 22/5/2001 add two returned parameters */
AgResult cls_delete_path_16 ( MpId x_mpid , 
							  AG_U16 *p_path , 
							  AG_U8 *p_ctl,  
							  AG_U8 *p_mask,  
							  AG_U32 n_pathlen , 
							  LeafGrpId n_grp, 
							  AG_U32 *p_erase_pos, 
							  AG_U16 *p_ref_count)
{
MiniPrgDesc* p_mp; 
AgResult x_res;
ClsbCurrState x_currstate;
TableType e_table = CLS_PRI_TBL;
LeafGrp *p_grp;
AgClsbMem   *p_mem_handle;

        #ifndef NO_VALIDATION
		if(n_pathlen==0)
		{
			return AG_E_BAD_PARAMS;
		}
        #endif

        x_res = cls_get_mpdesc(x_mpid , &p_mp);
        #ifndef NO_VALIDATION
        if (AG_FAILED(x_res))
		{
			return AG_E_CLS_BAD_MP_ID;
		}
        #endif

        if(n_grp != AG_CLSB_IGNORE_GRP)
        {	    
            x_res = cls_get_leaf_grp(n_grp,&p_grp);
            #ifndef NO_VALIDATION
            if(AG_FAILED(x_res))
            { 
                return AG_E_CLS_BAD_GRP_ID;
            }
            /* The Group and MP exist  */

            if(p_mp->x_stdprg.p_mem_handle != p_grp->p_mem_handle)
            { /* classification memory areas not matching */
                return AG_E_CLS_UNMATCHED_PRGS;
            }
            #endif
            /* matching classification programs */
        }

        p_mem_handle = p_mp->x_stdprg.p_mem_handle;
		/* Wait for semaphore */
#ifdef AG_NO_HL
	CAgBiSem x_sem(&p_mem_handle->x_semaphore,CAgBiSem::LOCKED);  /* unlocks semaphore in destructor */
#endif
		/* Delete the path from the mini program */
		x_res = delete_path_16(p_path, p_ctl, p_mask, n_pathlen, &p_mp->x_stdprg, &x_currstate, p_erase_pos, p_ref_count);
		if (AG_FAILED(x_res))
		{
			/* Release semaphore */
			return x_res;
		}

        if(n_grp != AG_CLSB_IGNORE_GRP)
        {
		    if (x_currstate.curr_table == DEF_TABLE)
			    e_table = CLS_DEF_TBL;
		    x_res = cls_del_mp_leaf(p_mp , p_grp ,(AG_U32)x_currstate.curr_rec , (AG_U32)x_currstate.curr_pt ,
								e_table , (AG_U16)x_currstate.curr_data);
        }

		/* Release semaphore */
		return x_res;
} /* of cls_delete_path */
#endif /*CLS_16_BIT*/
/*************************************************************************/
/* The following function enables to link a group to a mini program root */
/*************************************************************************/
AgResult ag_clsb_link ( AG_U8 n_opc ,  AG_U8 n_mask, LeafGrpId n_grp,  MpId x_mpid ,  AG_BOOL b_allow_unlink)
{
    AgResult x_res;
    MiniPrgDesc* p_mp; 
    LeafGrp* p_grp;

    #ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "START: ag_clsb_link\n");
	fprintf (full_trace, "\tParameters:\n\t\topc %d\n\t\\tmask %d\n\t\tgrp %d\n\t\tmpid %d\n\t\tunlink option %d\n",
		n_opc, n_grp, x_mpid, b_allow_unlink);
    #endif /* CLS_TEST_FULL_TRACE */

    x_res = cls_get_mpdesc(x_mpid , &p_mp);
    #ifndef NO_VALIDATION
	if(AG_FAILED(x_res))
	{

        #ifdef CLS_TEST_FULL_TRACE
	    fprintf (full_trace, "END: ag_clsb_link (AG_E_CLS_BAD_MP_ID)\n");
        #endif /* CLS_TEST_FULL_TRACE */

		return AG_E_CLS_BAD_MP_ID;
	}
    #endif

    x_res = cls_get_leaf_grp(n_grp,&p_grp);
    #ifndef NO_VALIDATION
    if(AG_FAILED(x_res))
    { 

        #ifdef CLS_TEST_FULL_TRACE
	    fprintf (full_trace, "END: ag_clsb_link (AG_E_CLS_BAD_GRP_ID)\n");
        #endif /* CLS_TEST_FULL_TRACE */

        return AG_E_CLS_BAD_GRP_ID;
    }
    #endif

#ifndef NO_VALIDATION
    if(p_mp->x_stdprg.p_mem_handle != p_grp->p_mem_handle)
    { /* Error: trying to link between parts of different memory areas.*/

        #ifdef CLS_TEST_FULL_TRACE
	    fprintf (full_trace, "END: ag_clsb_link (AG_E_CLS_UNMATCHED_PRGS)\n");
        #endif /* CLS_TEST_FULL_TRACE */

        return AG_E_CLS_UNMATCHED_PRGS;
    }
    /* matching memory areas */


    #ifndef CLS_8_BIT
    if(p_grp->p_mem_handle->n_cls_mode == AG_CLS_BIT_MODE_16)
    {
        if(n_opc == AG_CLSB_OPC16_TAG)
        {
            #ifdef CLS_TEST_FULL_TRACE
            fprintf (full_trace, "END: ag_clsb_link (AG_E_BAD_PARAMS)\n");
            #endif /* CLS_TEST_FULL_TRACE */

		    return AG_E_BAD_PARAMS;

	    }
    }
    else
    #endif /* #ifndef CLS_8_BIT */
    { /* 8 bit*/
    #ifndef CLS_16_BIT
        if(n_opc == AG_CLSB_OPC8_TAG)
        {
            #ifdef CLS_TEST_FULL_TRACE
            fprintf (full_trace, "END: ag_clsb_link (AG_E_BAD_PARAMS)\n");
            #endif /* CLS_TEST_FULL_TRACE */

		    return AG_E_BAD_PARAMS;

	    }
    #endif /*#ifndef CLS_16_BIT*/
    }
#endif /*#ifndef NO_VALIDATION*/


#ifdef AG_NO_HL
	CAgBiSem x_sem(&p_mp->x_stdprg.p_mem_handle->x_semaphore,CAgBiSem::LOCKED);  /* unlocks semaphore in destructor */
#endif

	x_res = cls_link_leaf_grp (p_grp , p_mp , n_opc ,n_mask, b_allow_unlink);

    #ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "END: ag_clsb_link (%08x)\n", x_res);
    #endif /* CLS_TEST_FULL_TRACE */

	return x_res;
} /* of cls_link */

/******************************************************************/
/* The following function unlink a group from a mini program root */
/******************************************************************/
AgResult ag_clsb_unlink ( LeafGrpId n_grp, AG_U32 n_new_tag)
{
    AgResult x_res;
    LeafGrp* p_grp;

    #ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "START: ag_clsb_unlink\n");
	fprintf (full_trace, "\tParameters:\n\t\tgrp %d\n", n_grp);
    #endif /* CLS_TEST_FULL_TRACE */
    
    x_res = cls_get_leaf_grp(n_grp,&p_grp);

    #ifndef NO_VALIDATION
    if(AG_FAILED(x_res))
    { 

        #ifdef CLS_TEST_FULL_TRACE
	    fprintf (full_trace, "END: ag_clsb_unlink (AG_E_CLS_BAD_GRP_ID)\n");
        #endif /* CLS_TEST_FULL_TRACE */

        return AG_E_CLS_BAD_GRP_ID;
    }
    #endif

#ifdef AG_NO_HL
	CAgBiSem x_sem(&p_grp->p_mem_handle->x_semaphore,CAgBiSem::LOCKED);  /* unlocks semaphore in destructor */
#endif
	x_res = cls_unlink_leaf_grp (p_grp,n_new_tag);

    #ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "END: ag_clsb_unlink (%08x)\n", x_res);
    #endif /* CLS_TEST_FULL_TRACE */

	return x_res;
} /* of cls_unlink */

/***********************************************************/
/* The following function deletes a mini program reference */
/* and container from the prgoram handler database         */
/***********************************************************/
AgResult ag_clsb_del_prg ( MpId x_mpid )
{
    AgResult n_res;
    MiniPrgDesc *p_mp;

	n_res = cls_get_mpdesc(x_mpid , &p_mp);
    #ifndef NO_VALIDATION
    if(AG_FAILED(n_res))
	{
		return AG_E_CLS_BAD_MP_ID;
	}
    #endif

#ifdef AG_NO_HL
	CAgBiSem x_sem(&p_mp->x_stdprg.p_mem_handle->x_semaphore,CAgBiSem::LOCKED);  /* unlocks semaphore in destructor */
#endif

    if(AG_SUCCEEDED(n_res = cls_del_mp(x_mpid,&p_mp,bClsbValidate,AG_TRUE)))
    {
        /* release the allocated record and path tag of the MP root.*/
		if (p_mp->x_stdprg.p_mem_handle->n_cls_mode == AG_CLS_BIT_MODE_8)
		{
			clsb_eng_del_mp_8(&p_mp->x_stdprg);
		}
		else
		{
			clsb_eng_del_mp_16(&p_mp->x_stdprg);
		}

        free_mp(p_mp);

        return AG_S_OK;
    }
    
    return n_res;
} /* of cls_del_prg */

/**************************************************/
/* The following function deletes a group */
/* from the prgoram handler database              */
/**************************************************/
AgResult ag_clsb_del_grp( LeafGrpId x_grp)
{
    LeafGrp* p_grp;
    AgResult n_res;
    
    n_res = cls_get_leaf_grp(x_grp,&p_grp);
    #ifndef NO_VALIDATION
    if(AG_FAILED(n_res))
    { 
        return AG_E_CLS_BAD_GRP_ID;
    }
    #endif
    
#ifdef AG_NO_HL
	CAgBiSem x_sem(&p_grp->p_mem_handle->x_semaphore,CAgBiSem::LOCKED);  /* unlocks semaphore in destructor */
#endif
    
	n_res = cls_del_grp (x_grp,AG_TRUE);

    return n_res;

} /* of cls_del_grp */


/**********************************************************************
Name: ag_cls_set_active_prg
Description: 
Write to the configuration register the initial parameters +mode of work.
Parameters:
	IN AG_U32 n_interf:
	IN AG_U8 n_mode of work:
	IN StdPrg *p_root:
Returns AgResult :
************************************************************/
#ifndef BCM_CES_SDK /*Needed only for CPU classification*/
AgResult ag_clsb_set_active_prg(AgComIntf n_interf,PrgRoot *p_root, void * p_prg, AG_U32 n_mode_of_work)
{
#ifndef NO_VALIDATION

    #ifndef CLS_16_BIT
    if (n_mode_of_work == AG_CLS_BIT_MODE_8) /* 8 bit mode */
	{
		if ((p_root->def_rec > 0x1FFF) ||
			(p_root->def_pt > 0xFF) || 
			(p_root->n_cls_def_opc >= AG_CLSB_OPC8_TAG) || 
			(p_root->n_cls_def_opc == AG_CLSB_OPC8_INT_TAG) ||
			(p_root->def_mask != 0)) /*no masks are used in 8 bit mode*/

		{
			return AG_E_BAD_PARAMS;
		}
	}
    else   /* 16 bit mode */
    #endif /* not CLS_16_BIT */
	{
    #ifndef CLS_8_BIT
		if ((p_root->def_rec > 0x1FFF) ||
			(p_root->def_pt > 0x7FFF) || 
			(p_root->n_cls_def_opc >= AG_CLSB_OPC16_TAG) || 
			(p_root->n_cls_def_opc == AG_CLSB_OPC16_INT_TAG) ||
		    (p_root->def_mask > 0xF) )
		{
			return AG_E_BAD_PARAMS;
		}
    #endif  /*#ifndef CLS_8_BIT */ 
	}

#endif /* #ifndef NO_VALIDATION */
    
	switch (n_interf) 
    {
	    case AG_LAN :
			/*set the initial record*/
			ag_set_range_value(AG_REG_L_CLS_CFG0, p_root->def_rec, 0, 13);
			/*set the initial path tag*/
			*AG_REG_L_CLS_IPT = p_root->def_pt;
			/*set the initial opcode*/
			ag_set_range_value(AG_REG_L_CLS_CFG1, p_root->n_cls_def_opc, 4, 4);
			/*set the initial mask*/
			ag_set_range_value(AG_REG_L_CLS_CFG1, p_root->def_mask, 0, 4);
            /* set program base */
			(*AG_REG_L_CLS_BASE)=AG_PTR_TO_HW_BASE(p_prg);
			/* set program mode */
			ag_set_bit_value(AG_REG_L_CLS_CFG0, n_mode_of_work,13);
 
			return AG_S_OK;

    	case AG_WAN1 :
			/*set the initial record*/
			ag_set_range_value(AG_REG_W1_CLS_CFG0, p_root->def_rec, 0, 13);
			/*set the initial path tag*/
			*AG_REG_W1_CLS_IPT = p_root->def_pt;
			/*set the initial opcode*/
			ag_set_range_value(AG_REG_W1_CLS_CFG1, p_root->n_cls_def_opc, 4, 4);
			/*set the initial mask*/
			ag_set_range_value(AG_REG_W1_CLS_CFG1, p_root->def_mask, 0, 4);
			/* set program base */
			(*AG_REG_W1_CLS_BASE)=AG_PTR_TO_HW_BASE(p_prg);
			/* set program mode */
			ag_set_bit_value(AG_REG_W1_CLS_CFG0, n_mode_of_work,13);
 
			return AG_S_OK;

    	case AG_WAN2  : 
			/*set the initial record*/
			ag_set_range_value(AG_REG_W2_CLS_CFG0, p_root->def_rec, 0, 13);
			/*set the initial path tag*/
			*AG_REG_W2_CLS_IPT = p_root->def_pt;
			/*set the initial opcode*/
			ag_set_range_value(AG_REG_W2_CLS_CFG1, p_root->n_cls_def_opc, 4, 4);
			/*set the initial mask*/
			ag_set_range_value(AG_REG_W2_CLS_CFG1, p_root->def_mask, 0, 4);
			/* set program base */
			(*AG_REG_W2_CLS_BASE)=AG_PTR_TO_HW_BASE(p_prg);
			/* set program mode */
			ag_set_bit_value(AG_REG_W2_CLS_CFG0, n_mode_of_work,13);

			return AG_S_OK;
    
    	default : 
            return AG_E_BAD_PARAMS;
	}


}


/*
Name: ag_clsb_set_overrun_hndl
Description: 
    Configures what to do with a frame when an overrun condition occurs.
    The configuration is per interface.
    The frame can be discarded or be passed to the CPU with tag 1.
Parameters:
	AgComIntf e_rx_intf: The interface that this configuration applies to.
	AG_BOOL b_pass_to_cpu: When true, such frames are passed to a predefined
        CPU queue.
        When false, such frames are discarded
Returns AgResult :
    AG_S_OK - on success.
    AG_E_BAD_PARAMS - on invalid interface.
*/
AgResult ag_clsb_set_overrun_hndl(AgComIntf e_rx_intf,AG_BOOL b_pass_to_cpu)
{
    /*
      1 - forward to CPU with tag 1
      0 - discard 
    */
    AG_U32 n_val = b_pass_to_cpu ? 1 : 0;
    
    switch (e_rx_intf) 
    {
        case AG_LAN :
            *AG_REG_L_CLS_OVRN = n_val;
            break;
        case AG_WAN1 :
            *AG_REG_W1_CLS_OVRN = n_val;
            break;
        case AG_WAN2  : 
            *AG_REG_W2_CLS_OVRN = n_val;
            break;
        #ifndef NO_VALIDATION
        default : 
            return AG_E_BAD_PARAMS;
        #endif
	}

    return AG_S_OK;
}


/*
Name: ag_cls_get_overrun_hndl
Description: 
    returns in *p_pass_to_cpu the configuration of how to handle frames in case of classification 
    overrun.
    TRUE means that such frames are passed to the CPU.
    FALSE means that such frames are discarded
Parameters:
	AgComIntf e_rx_intf: The rx interface whose configuration is read
	AG_BOOL* p_pass_to_cpu: The function returns in *p_pass_to_cpu 
        a boolean value specifying the frame handling.
Returns AgResult :
    AG_S_OK - on success.
    AG_E_BAD_PARAMS - on invalid interface.
    AG_E_NULL_PTR - if p_pass_to_cpu is NULL
*/
AgResult ag_clsb_get_overrun_hndl(AgComIntf e_rx_intf,AG_BOOL* p_pass_to_cpu)
{
    /*
      1 - forward to CPU with tag 1
      0 - discard 
    */

    #ifndef NO_VALIDATION
    if(!p_pass_to_cpu)
    {
        return AG_E_NULL_PTR;
    }
    #endif

    switch (e_rx_intf) 
    {
        case AG_LAN :
            *p_pass_to_cpu = *AG_REG_L_CLS_OVRN & 0x1;
            break;
        case AG_WAN1 :
            *p_pass_to_cpu = *AG_REG_W1_CLS_OVRN & 0x1;
            break;
        case AG_WAN2  : 
            *p_pass_to_cpu = *AG_REG_W2_CLS_OVRN & 0x1;
            break;
        #ifndef NO_VALIDATION
        default : 
            return AG_E_BAD_PARAMS;
        #endif
	}

    return AG_S_OK;
}

/*
Name: ag_cls_set_late_cls_hndl
Description: 
    Configures what to do with a frame when a late classification condition occurs.
    The configuration is per interface.
    The frame can be discarded or be passed to the CPU with tag 2.
Parameters:
	AgComIntf e_rx_intf: The interface that this configuration applies to.
	AG_BOOL b_pass_to_cpu: When true, such frames are passed to a predefined
        CPU queue.
        When false, such frames are discarded
Returns AgResult :
    AG_S_OK - on success.
    AG_E_BAD_PARAMS - on invalid interface.
*/
AgResult ag_clsb_set_late_cls_hndl(AgComIntf e_rx_intf,AG_BOOL b_pass_to_cpu)
{
    /*
      1 - forward to CPU with tag 2
      0 - discard 
    */
  AG_U32 n_val = b_pass_to_cpu ? 1 : 0;

  switch (e_rx_intf) 
    {
        case AG_LAN :
            *AG_REG_L_CLS_LCTYPE = n_val;
            break;
        case AG_WAN1 :
            *AG_REG_W1_CLS_LCTYPE = n_val;
            break;
        case AG_WAN2  : 
            *AG_REG_W2_CLS_LCTYPE = n_val;
            break;
        #ifndef NO_VALIDATION
        default : 
            return AG_E_BAD_PARAMS;
        #endif
	}

    return AG_S_OK;
}

/*
Name: ag_cls_get_late_cls_hndl
Description: 
    returns in *p_pass_to_cpu the configuration of how to handle frames in 
    case of late classification.
    TRUE means that such frames are passed to the CPU.
    FALSE means that such frames are discarded
Parameters:
	AgComIntf e_rx_intf: The rx interface whose configuration is read
	AG_BOOL* p_pass_to_cpu: The function returns in *p_pass_to_cpu 
        a boolean value specifying the frame handling.
Returns AgResult :
    AG_S_OK - on success.
    AG_E_BAD_PARAMS - on invalid interface.
    AG_E_NULL_PTR - if p_pass_to_cpu is NULL
*/
AgResult ag_clsb_get_late_cls_hndl(AgComIntf e_rx_intf,AG_BOOL* p_pass_to_cpu)
{
    /*
      1 - forward to CPU with tag 2
      0 - discard 
    */
    #ifndef NO_VALIDATION
    if(!p_pass_to_cpu)
    {
        return AG_E_NULL_PTR;
    }
    #endif

    switch (e_rx_intf) 
    {
        case AG_LAN :
            *p_pass_to_cpu = *AG_REG_L_CLS_LCTYPE & 0x1;
            break;
        case AG_WAN1 :
            *p_pass_to_cpu = *AG_REG_W1_CLS_LCTYPE & 0x1;
            break;
        case AG_WAN2  : 
            *p_pass_to_cpu = *AG_REG_W2_CLS_LCTYPE & 0x1;
            break;
        #ifndef NO_VALIDATION
        default : 
            return AG_E_BAD_PARAMS;
        #endif            
	}

    return AG_S_OK;
}
#endif /*BCM_CES_SDK*/

AgResult ag_clsb_create_hw_mem (void *h_device /*BCM-CLS:CPU=NULL/else in CES*/ ,void* p_mem_start, AG_U32 n_mem_size, AG_U32 n_cls_mode, AgClsbMemHndl* p_mem_handle)
{
	AgClsbMem *p_cls_mem;
    AG_U32 i,n_handle = 0;

    /* following code assumes that a_sem_name is initialized to 6 chars.*/
    AG_U8 a_sem_name[9] = "clsbSm";

    #ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "START: ag_clsb_create_hw_mem\n");
	fprintf (full_trace, "\tParameters:\n\t\tp_mem_start %08x\n\t\tn_mem_size %d\n\t\tn_cls_mode %d\n",
		p_mem_start, n_mem_size, n_cls_mode);
    #endif /* CLS_TEST_FULL_TRACE */

    #ifndef NO_VALIDATION
	#ifndef AG_PURIFY
    if (n_cls_mode == AG_CLS_BIT_MODE_8)
	{
        if (n_mem_size < AG_CLS_MEM_BASE_SIZE_8 || 
            n_mem_size > AG_CLS_MAX_MEM_SIZE_8 ||
            n_mem_size % AG_CLS_MEM_BASE_SIZE_8    ||    /*size not a multiple of REC_SIZE_8*/
            (AG_U32)p_mem_start % ag_max(AG_REG_BASE_ALIGN,AG_CLS_MEM_BASE_SIZE_8)  /*mem start not aligned properly*/
            )
	    {

            #ifdef CLS_TEST_FULL_TRACE
	        fprintf (full_trace, "END: ag_clsb_create_hw_mem (AG_E_BAD_PARAMS)\n");
            #endif /* CLS_TEST_FULL_TRACE */

		    return AG_E_BAD_PARAMS;
	    }
    } 
    else if(n_cls_mode == AG_CLS_BIT_MODE_16)
    {
	    if (n_mem_size < AG_CLS_MEM_BASE_SIZE_16 || 
            n_mem_size > AG_CLS_MAX_MEM_SIZE_16 ||
            n_mem_size % AG_CLS_MEM_BASE_SIZE_16   ||  /*size not a multiple of REC_SIZE_8*/
            (AG_U32)p_mem_start % ag_max(AG_REG_BASE_ALIGN,AG_CLS_MEM_BASE_SIZE_16) /*mem start not aligned properly*/
            )
	    {

            #ifdef CLS_TEST_FULL_TRACE
	        fprintf (full_trace, "END: ag_clsb_create_hw_mem (AG_E_BAD_PARAMS)\n");
            #endif /* CLS_TEST_FULL_TRACE */

			return AG_E_BAD_PARAMS;
	    }
    }
    else
    {

        #ifdef CLS_TEST_FULL_TRACE
	    fprintf (full_trace, "END: ag_clsb_create_hw_mem (AG_E_BAD_PARAMS)\n");
        #endif /* CLS_TEST_FULL_TRACE */

        return AG_E_BAD_PARAMS;
    }
	#endif /*AG_PURIFY */
	#endif /*NO_VALIDATION */

    /* find free ID */
    for (i = 0 ; i < nMaxMem ; i++)
    {
        if (!aClsMem[i])
        {
            n_handle = i;
            break;
        }
    }
    if(i == nMaxMem)
    {

        #ifdef CLS_TEST_FULL_TRACE
	    fprintf (full_trace, "END: ag_clsb_create_hw_mem (AG_E_FAIL)\n");
        #endif /* CLS_TEST_FULL_TRACE */

        return AG_E_MAX_EXCEEDED;
    }

    /* using calloc since semaphore member must be set to zero before its creation. */
    if (AG_FAILED(agos_calloc(sizeof(AgClsbMem), (void*)&p_cls_mem)))
	{
		AGOS_SWERR(AGOS_SWERR_WARNING,"Memory allocation for CLS PRG is failed",0,0,0,0,0);

        #ifdef CLS_TEST_FULL_TRACE
	    fprintf (full_trace, "END: ag_clsb_create_hw_mem (AG_E_OUT_OF_MEM)\n");
        #endif /* CLS_TEST_FULL_TRACE */

		return AG_E_OUT_OF_MEM;
	}

    /*the below code assumes a_sem_name was initialized to 6 chars.*/
/*    itoa(n_handle,&a_sem_name[6],16);*/ /*convert number to string in hex.*/ 

    /* Create prg memory semaphore */
	if(AG_FAILED(agos_create_bi_semaphore(&p_cls_mem->x_semaphore, (AG_S8*)a_sem_name)))
    {
        AGOS_SWERR(AGOS_SWERR_WARNING,"CLSB: semaphore creation failed ",0,0,0,0,0);
        agos_free((void*)p_cls_mem);
        return AG_E_FAIL;
    }


	/* Update memory handle structure */
	p_cls_mem->n_cls_mode = n_cls_mode;
	p_cls_mem->p_record_base  = (AG_U32*)p_mem_start;  	/* Set record base */
    p_cls_mem->n_last_record = 0;
    p_cls_mem->n_update_size = 0;
    /*BCM-CLS: set default device*/
    p_cls_mem->h_device =  h_device; /*Not in CES memory*/


#ifndef CLS_16_BIT
    if (p_cls_mem->n_cls_mode == AG_CLS_BIT_MODE_8)
	{

	    p_cls_mem->n_record_limit = n_mem_size / REC_SIZE_8;
		/* In 8 bit mode the base register is set differently per each memory area.*/
        p_cls_mem->n_low_limit_record = 0; 

        if(AG_FAILED(ag_clsb_alloc_aux_mem_8(p_cls_mem)))
        {
            agos_delete_bi_semaphore(&p_cls_mem->x_semaphore);
            agos_free((void*)p_cls_mem);
            AGOS_SWERR(AGOS_SWERR_WARNING,"Aux memory allocation for CLS PRG is failed",0,0,0,0,0);
            return AG_E_OUT_OF_MEM;
        }
        ag_clsb_reset_eng_memory_8(p_cls_mem);
	}
	else /*  16 bit mode */
#endif /*#ifndef CLS_16_BIT*/
	{
#ifndef CLS_8_BIT

	    p_cls_mem->n_record_limit = n_mem_size / REC_SIZE_16;
        p_cls_mem->n_low_limit_record = (AG_U16)(((AG_U32)p_mem_start - AG_HW_DRAM_BASE) / REC_SIZE_16);

        ag_clsb_alloc_aux_mem_16(p_cls_mem);
        ag_clsb_reset_eng_memory_16(p_cls_mem);
#endif /* #ifndef CLS_8_BIT */
	}

	/* Init memory handle */
	p_cls_mem->x_mem_hndl = n_handle;
    aClsMem[n_handle] = p_cls_mem;
	/* Return memory handle */
	*p_mem_handle = n_handle;
#ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "END: ag_clsb_create_hw_mem (AG_S_OK)\n");

	fprintf (full_trace, "\tReturned Parameters\n\t\tp_mem_handle %d\n\t\tn_cls_mode %d\n", 
		*p_mem_handle, p_cls_mem->n_cls_mode);

	fprintf (full_trace, "\t\tn_last_record %d\n\t\tn_record_limit %d\n\t\tn_update_size %d\n",
		p_cls_mem->n_last_record, p_cls_mem->n_record_limit, p_cls_mem->n_update_size);

	fprintf (full_trace, "\t\tp_bit_impression %08x\n\t\tp_entry_to_copy %08x\n",
		p_cls_mem->p_bit_impression, p_cls_mem->p_entry_to_copy);

	fprintf (full_trace, "\t\tp_extend %08x\n\t\tp_record_base %08x\n\t\tp_record_image %08x\n",
		p_cls_mem->p_extend, p_cls_mem->p_record_base, p_cls_mem->p_record_image);

	fprintf (full_trace, "\t\tp_ref_count %08x\n\t\tp_static_rec_image %08x\n",
		p_cls_mem->p_ref_count, p_cls_mem->p_static_rec_image);

	fprintf (full_trace, "\t\tx_old_stack %08x\n\t\tx_node_update %08x\n", 
		p_cls_mem->x_old_stack, p_cls_mem->x_node_update);
	fprintf (full_trace, "\t\tx_new_stack %08x\n", 
		p_cls_mem->x_new_stack);
#endif /* CLS_TEST_FULL_TRACE */

	return AG_S_OK;
}

/*BCMadd:*/
/*BCM: temporary pacth to initalize classifyer memory*/
/*The following number were copyed from CES application */
AG_U32 bcm_cls_n_max_mem = 30; 
AgClsMemUsage   bcm_cls_init_mem_req;
AgResult bcm_ag_clsb_cfg_create(AG_U32 n_max_mem, AgClsMemUsage* p_mem_req)     /*Copy from ag_clsb_cfg*/
{
    AG_U32 n_zero = 0;
	if(!p_mem_req || !n_max_mem)
	{
		return AG_E_BAD_PARAMS;
	}

    /*allocate and initialize the array of cls memory pointers.*/
    nMaxMem = n_max_mem;
	agos_malloc(nMaxMem * sizeof(AgClsbMem*), (void*)&aClsMem);
	memset(aClsMem, n_zero, nMaxMem * sizeof(AgClsbMem*));
	mph_configure (p_mem_req->n_mp, p_mem_req->n_grp);
	return AG_S_OK;
} /* of cls_init_api */

/*This is how memory was initalized from CES application*/
/*BCM: temporary pacth to initalize classifyer memory*/
/*The following number were copyed from CES application */

AgResult bcm_ag_clsb_cfg(void)
{   
    if( nMaxMem )
              return AG_S_OK; /*Already initalized*/
    memset(&bcm_cls_init_mem_req,0, sizeof(AgClsMemUsage));
    bcm_cls_init_mem_req.n_mp = 1000;
    bcm_cls_init_mem_req.n_grp = 1000;
   #if 0 /*Are not used in BCM*/
    bcm_cls_init_mem_req.n_buffer_96 = 3000;
    bcm_cls_init_mem_req.n_buffer_40 = 18000;
    bcm_cls_init_mem_req.n_buffer_24 = 5000;
    bcm_cls_init_mem_req.n_buffer_12 = 20000;
   #endif 
    return bcm_ag_clsb_cfg_create(bcm_cls_n_max_mem,&bcm_cls_init_mem_req);
}



AgResult ag_clsb_reset_memory(AgClsbMemHndl x_mem_handle)
{
	AgClsbMem *p_cls_mem;

    #ifndef NO_VALIDATION
    if(!CLSB_VALID_MEM_HNDL(x_mem_handle))
    {
       return AG_E_BAD_PARAMS; 
    }
    #endif

    p_cls_mem = aClsMem[x_mem_handle];

    /*get semaphore*/
#ifdef AG_NO_HL
	CAgBiSem x_sem(&p_cls_mem->x_semaphore,CAgBiSem::LOCKED);  /* unlocks semaphore in destructor */
#endif
    
    p_cls_mem->n_update_size = 0;
    p_cls_mem->n_last_record = 0;

#ifndef CLS_8_BIT
	if (p_cls_mem->n_cls_mode == AG_CLS_BIT_MODE_16)
	{
        ag_clsb_reset_eng_memory_16(p_cls_mem);
    }
	else  /* 8 bit mode */
#endif
	{
#ifndef CLS_16_BIT
        ag_clsb_reset_eng_memory_8(p_cls_mem);        
#endif
	}

    clsb_mph_reset(p_cls_mem);
    
    /*give semaphore*/
    return AG_S_OK;
}

AgResult ag_clsb_del_hw_mem(AgClsbMemHndl x_mem_handle)
{
	AgClsbMem *p_cls_mem;

    #ifndef NO_VALIDATION
    if(!CLSB_VALID_MEM_HNDL(x_mem_handle))
    {
       AGOS_SWERR(AGOS_SWERR_WARNING,"ag_clsb_del_hw_mem failed: Bad memory handle",0,0,0,0,0);
       return AG_E_BAD_PARAMS; 
    }
    #endif

    p_cls_mem = aClsMem[x_mem_handle];
    
    /*get semaphore*/
#ifdef AG_NO_HL
	CAgBiSem x_sem(&p_cls_mem->x_semaphore,CAgBiSem::LOCKED);  /* unlocks semaphore in destructor */
#endif

    aClsMem[x_mem_handle] = NULL;
    clsb_mph_reset(p_cls_mem);
    ag_clsb_free_aux_mem(p_cls_mem);

    /*give semaphore*/
   agos_delete_bi_semaphore(&p_cls_mem->x_semaphore);
    
    agos_free((void*)p_cls_mem);
    return AG_S_OK;

}
#ifndef BCM_CES_SDK /*Needed only for CPU classification*/
AgResult ag_clsb_activate_prg(AgComIntf n_interf, MpId n_mp_id)
{
	MiniPrgDesc* p_mp;

	if(AG_SUCCEEDED(cls_get_mpdesc(n_mp_id ,&p_mp)))
    {      
		/* Wait for semaphore */
#ifdef AG_NO_HL
	CAgBiSem x_sem(&p_mp->x_stdprg.p_mem_handle->x_semaphore,CAgBiSem::LOCKED);  /* unlocks semaphore in destructor */
#endif
		if(AG_SUCCEEDED(ag_clsb_set_active_prg(n_interf, &p_mp->x_stdprg.x_root, 
                                           p_mp->x_stdprg.p_mem_handle->p_record_base,
                                           p_mp->x_stdprg.p_mem_handle->n_cls_mode)))
        {
            clsb_set_active_mp(n_interf,p_mp);
            return AG_S_OK;
        }
        else
        {
            return AG_E_BAD_PARAMS;
        }
    }
    else
    {
   		return AG_E_CLS_BAD_MP_ID;
    }
    
}
#endif /*BCM_CES_SDK*/

AgResult ag_clsb_is_active(MpId n_mp_id, AG_BOOL *p_active)
{
	MiniPrgDesc* p_mp;

	if(AG_SUCCEEDED(cls_get_mpdesc(n_mp_id ,&p_mp)) && p_active)
    {      
		/* Wait for semaphore */
#ifdef AG_NO_HL
		CAgBiSem x_sem(&p_mp->x_stdprg.p_mem_handle->x_semaphore,CAgBiSem::LOCKED);  /* unlocks semaphore in destructor */
#endif
		*p_active = CLSB_IS_ACTIVE_MP(p_mp) ? AG_TRUE : AG_FALSE ;
		return AG_S_OK;
	}
	return AG_E_BAD_PARAMS;

}
 
AgResult ag_clsb_alloc_aux_mem_8 (AgClsbMem* p_cls_mem)
{

	/* Allocate memory for reference count array */
	if (AG_FAILED(agos_malloc(MAX_ENTRY_8*p_cls_mem->n_record_limit, (void **)&(p_cls_mem->p_ref_count))))
	{
		return AG_E_OUT_OF_MEM;
	}
    
    /*allocate memory for bit extension of the reference counter*/
	if (AG_FAILED(agos_malloc(AG_CLSB_BIT_ARRAY_SIZE_8(p_cls_mem), (void **)&(p_cls_mem->p_ref_ext))))
	{
		return AG_E_OUT_OF_MEM;
	}

	/* Allocate memory for array of extended */
	if (AG_FAILED(agos_malloc(AG_CLSB_BIT_ARRAY_SIZE_8(p_cls_mem), (void **)&(p_cls_mem->p_extend))))
	{
		return AG_E_OUT_OF_MEM;
	}

	/* Allocate memory for array of bit impression */
	if (AG_FAILED(agos_malloc(sizeof(AG_U32)*BIT_TABLE_ENTRY(MAX_ENTRY_8), (void **)&(p_cls_mem->p_bit_impression))))
	{
		return AG_E_OUT_OF_MEM;
	}

	/*Raanan Refua 4/11/2002*/
	/* Allocate memory for compact presentation of array of bit impression */
	if (AG_FAILED(agos_malloc(sizeof(AG_U32)*(MAX_ENTRY_8), (void **)&(p_cls_mem->p_ones_pos_in_pattern))))
	{
		return AG_E_OUT_OF_MEM;
	}

	/* Allocate memory for array of record images */
	if (AG_FAILED(agos_malloc(AG_CLSB_BIT_ARRAY_SIZE_8(p_cls_mem), (void **)&(p_cls_mem->p_record_image))))
	{
		return AG_E_OUT_OF_MEM;
	}

	/* Allocate array of entries to copy */
	if (AG_FAILED(agos_malloc(sizeof(AG_U32)*MAX_ENTRY_8, (void **)&(p_cls_mem->p_entry_to_copy))))
	{
		return AG_E_OUT_OF_MEM;
	}

	/* Allocate array of static records */
	if (AG_FAILED(agos_malloc(sizeof(AG_U32)*BIT_TABLE_ENTRY_GEN(p_cls_mem->n_record_limit), (void **)&(p_cls_mem->p_static_rec_image))))
	{
		return AG_E_OUT_OF_MEM;
	}

    /* allocate stack for add path operation.*/
	if (AG_FAILED(agos_malloc(sizeof(ClsbStackElement) * MAX_OLD_STACK_LEN_8 , (void **)&(p_cls_mem->x_old_stack.a_stack))))
	{
		return AG_E_OUT_OF_MEM;
	}
    p_cls_mem->x_old_stack.n_size = MAX_OLD_STACK_LEN_8;

	 /* allocate stack for add path operation.*/
	if (AG_FAILED(agos_malloc(sizeof(ClsbStackElement) * MAX_NEW_STACK_LEN_8 , (void **)&(p_cls_mem->x_new_stack.a_stack))))
	{
		return AG_E_OUT_OF_MEM;
	}
    p_cls_mem->x_new_stack.n_size = MAX_NEW_STACK_LEN_8;

	/* Allocate memory for empty entries count array */
	if (AG_FAILED(agos_malloc(sizeof(AG_U16)*p_cls_mem->n_record_limit, (void **)&(p_cls_mem->p_empty_entries))))
	{
		return AG_E_OUT_OF_MEM;
	}
    
    /*allocate memory for bit extension of the empty entries counter*/
/*	if (AG_FAILED(agos_malloc(sizeof(AG_U32)*BIT_TABLE_ENTRY_GEN(p_cls_mem->n_record_limit), (void **)&(p_cls_mem->p_empty_entries_ext)))) */
/*	{ */
/*		return AG_E_OUT_OF_MEM; */
/*	} */

#ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "\t\t\t-auxiliary memory size:\n");

	fprintf (full_trace, "\t\t\t\tp_ref_count %d\n\t\t\t\tp_extend %d\n",
		sizeof(AG_U16)*MAX_ENTRY_8*p_cls_mem->n_record_limit,
		AG_CLSB_BIT_ARRAY_SIZE_8(p_cls_mem));

	fprintf (full_trace, "\t\t\t\tp_bit_impression %d\n\t\t\t\tp_record_image %d\n",
		sizeof(AG_U32)*BIT_TABLE_ENTRY(MAX_ENTRY_8),
		AG_CLSB_BIT_ARRAY_SIZE_8(p_cls_mem));

	fprintf (full_trace, "\t\t\t\tp_entry_to_copy %d\n\t\t\t\tp_static_rec_image %d\n",
		sizeof(AG_U32)*MAX_ENTRY_8, 
		sizeof(AG_U32)*BIT_TABLE_ENTRY_GEN(p_cls_mem->n_record_limit));

	fprintf (full_trace, "\t\t\t\tx_old_stack.a_stack %d\n",
		sizeof(ClsbStackElement) * MAX_OLD_STACK_LEN_8);

	fprintf (full_trace, "\t\t\t\tx_new_stack.a_stack %d\n",
		sizeof(ClsbStackElement) * MAX_NEW_STACK_LEN_8);

#endif /* CLS_TEST_FULL_TRACE */
    return AG_S_OK;
}
#ifdef CLS_16_BIT
AgResult ag_clsb_alloc_aux_mem_16 (AgClsbMem* p_cls_mem)
{

	/* Allocate memory for reference count array */
	if (AG_FAILED(agos_malloc(sizeof(AG_U16)*MAX_PRI_ENT_16*p_cls_mem->n_record_limit, (void **)&(p_cls_mem->p_ref_count))))
	{
		return AG_E_OUT_OF_MEM;
	}

	/* Allocate memory for array of extended */
	if (AG_FAILED(agos_malloc(sizeof(AG_U32)*BIT_TABLE_ENTRY(MAX_PRI_ENT_16)*p_cls_mem->n_record_limit, (void **)&(p_cls_mem->p_extend))))
	{
		return AG_E_OUT_OF_MEM;
	}

	/* Allocate memory for array of bit impression */
	if (AG_FAILED(agos_malloc(sizeof(AG_U32)*BIT_TABLE_ENTRY(MAX_PRI_ENT_16), (void **)&(p_cls_mem->p_bit_impression))))
	{
		return AG_E_OUT_OF_MEM;
	}

	/* Allocate memory for array of record images */
	if (AG_FAILED(agos_malloc(sizeof(AG_U32)*BIT_TABLE_ENTRY(MAX_PRI_ENT_16)*p_cls_mem->n_record_limit, (void **)&(p_cls_mem->p_record_image))))
	{
		return AG_E_OUT_OF_MEM;
	}

	/* Allocate array of entries to copy */
	if (AG_FAILED(agos_malloc(sizeof(AG_U32)*MAX_ENTRY_16/4, (void **)&(p_cls_mem->p_entry_to_copy))))
	{
		return AG_E_OUT_OF_MEM;
	}

	/* Allocate array of static records */
	if (AG_FAILED(agos_malloc(sizeof(AG_U32)*BIT_TABLE_ENTRY_GEN(p_cls_mem->n_record_limit), (void **)&(p_cls_mem->p_static_rec_image))))
	{
		return AG_E_OUT_OF_MEM;
	}

    /* allocate stack for add path operation.*/
	if (AG_FAILED(agos_malloc(sizeof(ClsbStackElement) * MAX_OLD_STACK_LEN_16 , (void **)&(p_cls_mem->x_old_stack.a_stack))))
	{
		return AG_E_OUT_OF_MEM;
	}
    p_cls_mem->x_old_stack.n_size = MAX_OLD_STACK_LEN_16;

	 /* allocate stack for add path operation.*/
	if (AG_FAILED(agos_malloc(sizeof(ClsbStackElement) * MAX_NEW_STACK_LEN_16 , (void **)&(p_cls_mem->x_new_stack.a_stack))))
	{
		return AG_E_OUT_OF_MEM;
	}
    p_cls_mem->x_new_stack.n_size = MAX_NEW_STACK_LEN_16;

	/* Allocate memory for empty entries count array */
	if (AG_FAILED(agos_malloc(sizeof(AG_U32)*p_cls_mem->n_record_limit, (void **)&(p_cls_mem->p_empty_entries))))
	{
		return AG_E_OUT_OF_MEM;
	}
    
    /*allocate memory for bit extension of the empty entries counter*/
/*	if (AG_FAILED(agos_malloc(sizeof(AG_U32)*BIT_TABLE_ENTRY_GEN(p_cls_mem->n_record_limit), (void **)&(p_cls_mem->p_empty_entries_ext)))) */
/*	{ */
/*		return AG_E_OUT_OF_MEM; */
/*	} */

    return AG_S_OK;
}
#endif /*CLS_16_BIT*/

AgResult CLS_MEMSET_WORD_OFFSET(AgClsbMem *p_mem_handle, AG_U32 n_offset, AG_U32 data ,AG_U32 len);
void ag_clsb_reset_eng_memory_8(AgClsbMem *p_cls_mem)
{
    AG_U32 a_rst_8 = EMPTY_ENTRY_8;
	AG_U32 a_rst_empty_rec_8 = 0x01000100;

    /*used for setting memory to zero. Not using memset to save code size and for better efficiency */
    AG_U32 n_zero = 0;

	/* Clear records - ansi memset does not do the job since it works in bytes.*/
    CLS_MEMSET_WORD_OFFSET(p_cls_mem,0 /*offset*/,a_rst_8,p_cls_mem->n_record_limit * REC_SIZE_8);

	/*Clear reference count*/
	AG_MEMSET_WORD(p_cls_mem->p_ref_count,n_zero,MAX_ENTRY_8*p_cls_mem->n_record_limit);
    
    /* Clear bit impression*/
    AG_MEMSET_WORD(p_cls_mem->p_bit_impression,n_zero,BIT_TABLE_ENTRY(MAX_ENTRY_8)*sizeof(AG_U32)); 

    /*clear memory for bit extension of the reference counter*/
    AG_MEMSET_WORD(p_cls_mem->p_ref_ext,n_zero,AG_CLSB_BIT_ARRAY_SIZE_8(p_cls_mem)); 
    
    /* Clear record image */
    AG_MEMSET_WORD(p_cls_mem->p_record_image,n_zero,AG_CLSB_BIT_ARRAY_SIZE_8(p_cls_mem)); 

    /*clear extended command array.*/
	AG_MEMSET_WORD(p_cls_mem->p_extend,n_zero,AG_CLSB_BIT_ARRAY_SIZE_8(p_cls_mem));

	/* Init bit map of static records */
	AG_MEMSET_WORD(p_cls_mem->p_static_rec_image,n_zero, BIT_TABLE_ENTRY_GEN(p_cls_mem->n_record_limit)*sizeof(AG_U32));
    
    /* init the old stack for the first time - This is not really needed */
    AG_MEMSET_WORD(p_cls_mem->x_old_stack.a_stack, n_zero, sizeof(ClsbStackElement) * p_cls_mem->x_old_stack.n_size);
 
	/* init the new stack for the first time - This is not really needed */
    AG_MEMSET_WORD(p_cls_mem->x_new_stack.a_stack, n_zero, sizeof(ClsbStackElement) * p_cls_mem->x_new_stack.n_size);

	/* Init empty entries counter */
    AG_MEMSET_WORD(p_cls_mem->p_empty_entries,a_rst_empty_rec_8,p_cls_mem->n_record_limit * sizeof (AG_U16));

#ifdef CLS_TEST_FULL_TRACE
	fprintf (full_trace, "\t\t\t-reset auxiliary memory (size):\n");

	fprintf (full_trace, "\t\t\t\tp_ref_count %d\n\t\t\t\tp_extend %d\n",
		MAX_ENTRY_8*p_cls_mem->n_record_limit*sizeof(AG_U8),
		AG_CLSB_BIT_ARRAY_SIZE_8(p_cls_mem));

	fprintf (full_trace, "\t\t\t\tp_bit_impression %d\n\t\t\t\tp_record_image %d\n",
		BIT_TABLE_ENTRY(MAX_ENTRY_8)*sizeof(AG_U32),
		BIT_TABLE_ENTRY(MAX_ENTRY_8)*p_cls_mem->n_record_limit*sizeof(AG_U32));

	fprintf (full_trace, "\t\t\t\tp_static_rec_image %d\n\t\t\t\tx_old_stack.a_stack %d\n",
		BIT_TABLE_ENTRY_GEN(p_cls_mem->n_record_limit)*sizeof(AG_U32),
		sizeof(ClsbStackElement) * p_cls_mem->x_old_stack.n_size);
	fprintf (full_trace, "\t\t\t\tx_new_stack.a_stack %d\n",
		sizeof(ClsbStackElement) * p_cls_mem->x_new_stack.n_size);
#endif /* CLS_TEST_FULL_TRACE */

}

#ifndef CLS_16_BIT
void ag_clsb_reset_eng_memory_16(AgClsbMem *p_cls_mem)
{
    static const AG_U32 a_reset_16[4] = 
    {   
        CLS_ALL_ONES , EMPTY_ENTRY_16, EMPTY_ENTRY_16, EMPTY_ENTRY_16
    };
	AG_U32 a_rst_empty_rec_16 = 0x00010000;
    AG_U32 n_zero = 0;

    AG_U32 n_rec,n_index;

    /*clear program memory*/
    AG_MEMSET_4WORD(p_cls_mem->p_record_base,&a_reset_16,p_cls_mem->n_record_limit* REC_SIZE_16);
    
	/* Set path tags from 0 to 255 as occupied - used as skip fields */
	/* 16 bits the path tags 1 to 255 */
	for(n_rec=0;n_rec<p_cls_mem->n_record_limit /*MAX_REC_16*/;n_rec++)
	{
		for(n_index=MIN_RES_SKIP_16;n_index<=MAX_RES_SKIP_16;n_index++)
		{
			CLS_RECORDS_16(p_cls_mem->p_record_base,DEF_ENTRY_NUM_16(n_index),n_rec)=
                OCCUPIED_ENTRY_16;
		}
	}

	/*Clear reference count*/
    AG_MEMSET_WORD(p_cls_mem->p_ref_count,n_zero,MAX_PRI_ENT_16*p_cls_mem->n_record_limit*sizeof(AG_U16)); 
    
    /* Clear bit impression */
	AG_MEMSET_WORD(p_cls_mem->p_bit_impression,n_zero,BIT_TABLE_ENTRY(MAX_PRI_ENT_16)*sizeof(AG_U32)); 

    /* Clear record image */
    AG_MEMSET_WORD(p_cls_mem->p_record_image,n_zero,BIT_TABLE_ENTRY(MAX_PRI_ENT_16)*p_cls_mem->n_record_limit*sizeof(AG_U32)); 

    /*clear extended command array.*/
    AG_MEMSET_WORD(p_cls_mem->p_extend,n_zero,sizeof(AG_U32)*BIT_TABLE_ENTRY(MAX_PRI_ENT_16)*p_cls_mem->n_record_limit);

	/* Init bit map of static records */
	AG_MEMSET_WORD(p_cls_mem->p_static_rec_image, n_zero, BIT_TABLE_ENTRY_GEN(p_cls_mem->n_record_limit)*sizeof(AG_U32));
    
    /* init the old stack for the first time - This is not really needed */
    AG_MEMSET_WORD(p_cls_mem->x_old_stack.a_stack, n_zero, sizeof(ClsbStackElement) * p_cls_mem->x_old_stack.n_size);
 
    /* init the new stack for the first time - This is not really needed */
    AG_MEMSET_WORD(p_cls_mem->x_new_stack.a_stack, n_zero, sizeof(ClsbStackElement) * p_cls_mem->x_new_stack.n_size);
 
	/* Init empty entries counter */
    AG_MEMSET_WORD(p_cls_mem->p_empty_entries,a_rst_empty_rec_16,p_cls_mem->n_record_limit * sizeof(AG_U32));

}
#endif /*CLS_16_BIT*/

void ag_clsb_free_aux_mem(AgClsbMem* p_cls_mem)
{
	    
    if(p_cls_mem->p_ref_count)
    {
        agos_free((void*)p_cls_mem->p_ref_count);
    }

	if(p_cls_mem->p_ref_ext)
    {
        agos_free((void*)p_cls_mem->p_ref_ext);
    }

	if(p_cls_mem->p_extend)
    {
        agos_free((void*)p_cls_mem->p_extend);
    }

	if(p_cls_mem->p_bit_impression)
    {
        agos_free((void*)p_cls_mem->p_bit_impression);
    }

	/*Raanan Refua 4/11/2002*/
	if(p_cls_mem->p_ones_pos_in_pattern)
    {
        agos_free((void*)p_cls_mem->p_ones_pos_in_pattern);
    }


	if(p_cls_mem->p_record_image)
    {
        agos_free((void*)p_cls_mem->p_record_image);
    }

	if(p_cls_mem->p_entry_to_copy)
    {
        agos_free((void*)p_cls_mem->p_entry_to_copy);
    }

    if(p_cls_mem->p_static_rec_image)
    {
        agos_free((void*)p_cls_mem->p_static_rec_image);
    }

	if(p_cls_mem->x_old_stack.a_stack)
    {
        agos_free((void*)p_cls_mem->x_old_stack.a_stack);
	}

	if(p_cls_mem->x_new_stack.a_stack)
    {
        agos_free((void*)p_cls_mem->x_new_stack.a_stack);
	}

	if(p_cls_mem->p_empty_entries)
    {
        agos_free((void*)p_cls_mem->p_empty_entries);
	}

	/*
	if(p_cls_mem->p_empty_entries_ext)
    {
        agos_free((void*)p_cls_mem->p_empty_entries_ext);
	}
*/
}


/****************************************************************
Name: ag_clsb_change_tag_8
Description: 
Parameters:
	MpId x_mpid:
	AG_U8 *p_path:
	AG_U8* p_opc:
	AG_U32 n_pathlen:
	AG_U32 n_old_tag:
	AG_U32 n_new_tag:
Returns AgResult :
****************************************************************/
AgResult ag_clsb_change_tag_8(MpId x_mpid, AG_U8 *p_path, AG_U8* p_opc, AG_U32 n_pathlen, 
							  AG_U32 n_old_tag, AG_U32 n_new_tag)
{
AgResult n_res = AG_S_OK;
MiniPrgDesc* p_mp;
ClsbCurrState x_currstate;
ClsbStack *p_old_stack;
ClsbStack *p_new_stack;
/*LeafGrp *p_grp; */
AgClsbMem   *p_mem_handle;
/*AG_S32 n_org_old_stack_top; */
/*AG_S32 n_org_new_stack_top; */

Leaf x_leaf;


#ifndef NO_VALIDATION
	if(n_pathlen==0)
	{
		return AG_E_BAD_PARAMS;
	}
#endif
    
    n_res = cls_get_mpdesc(x_mpid , &p_mp);
#ifndef NO_VALIDATION
    if (AG_FAILED(n_res))
	{
		return AG_E_CLS_BAD_MP_ID;
	}
#endif

    p_mem_handle = p_mp->x_stdprg.p_mem_handle;
    p_old_stack = &p_mem_handle->x_old_stack;
    p_new_stack = &p_mem_handle->x_new_stack;

	/* Wait for semaphore */
#ifdef AG_NO_HL
	CAgBiSem x_sem(&p_mp->x_stdprg.p_mem_handle->x_semaphore,CAgBiSem::LOCKED);  /* unlocks semaphore in destructor */
#endif
	/* Stack should be initialized before "add path". 
	   The garbage can stay in the stack becouse of previous "add path" operation*/
    cls_stack_init(p_old_stack, p_new_stack);

	n_res = add_path_8(p_path, p_opc, n_pathlen, &n_old_tag, 1, &p_mp->x_stdprg, &x_currstate);
/*	n_res = query_path_8(&p_mp->x_stdprg, p_path, n_pathlen, CLSB_QUERY_PATH_REG, &x_info); */

	if (AG_FAILED(n_res))
	{
		return n_res;
	}

	/* The path was found. Change the tag */

	x_leaf.e_table = (TableType)x_currstate.curr_table;
	x_leaf.n_data  = x_currstate.curr_data;
	x_leaf.n_pt    = x_currstate.next_pt;
	x_leaf.n_rec   = x_currstate.next_rec;

	cls_set_leaf_tag_8 (&x_leaf, n_new_tag, p_mp->x_stdprg.p_mem_handle);
	/* Release semaphore */

	return n_res;
}

AgResult ag_clsb_get_number_of_added_paths(MpId x_mpid, AG_U32* p_num)
{
	MiniPrgDesc *p_mp;

	AgResult n_res = cls_get_mpdesc(x_mpid ,&p_mp);

	if (p_num)
	{
		*p_num = clsb_get_ref_count(p_mp->x_stdprg.p_mem_handle, p_mp->x_stdprg.x_root.def_rec, p_mp->x_stdprg.x_root.def_pt);
		return n_res;
	}
	else
		return AG_E_NULL_PTR;
}


#ifdef AG_MNT
/*********************************************************************************
	Begin of Print Area
*********************************************************************************/
/****************************************************************************/
/* The following function parse the tables and output them to an ascii file */
/****************************************************************************/

/**************************************************************
Name: ag_cls_sim_load_file

Description: 
Read the file generated by the dump_bin in the classification program.
Write the program to p_prg and update the initial parameters of the root as well
as the mode of work(8 or 16 bits).
 
Parameters:
	IN AG_U8 *p_file_name:
	OUT AG_U8 *p_prg:
	IO AG_U32 prg_size:
	OUT StdPrg *p_root:
	OUT AG_U8 *p_mode_of_work:

Returns AgResult :
	AG_S_OK - Success.
	AG_E_FAIL - The file could not be loaded or error on reading.
	AG_E_BAD_PARAMS - One of the parameters is not OK.

*****************************************************************/
AgResult ag_clsb_load_file(const AG_CHAR *p_file_name, ClsEntry *p_prg,AG_U32 *p_prg_size,PrgRoot *p_root,AG_U32 *p_mode_of_work)/*Used for the AG SIMULATOR only*/
{

	AG_U32 n_aux_size=(*p_prg_size);/*Set the initial size*/
	AG_U32 n_index;
	AG_BOOL b_little;
	FILE* bin_file; /*The program source file*/

		/*check validity*/
		if((p_prg==NULL)||(p_file_name==NULL)||(p_root==NULL)||(p_mode_of_work==NULL)||(p_prg_size==NULL))
		{
			return AG_E_BAD_PARAMS;
		}

		if ((bin_file = fopen(p_file_name,"rb")) == NULL)
		{
			return AG_E_FAIL;
		}

		b_little=ag_is_little_endian();

		/*read the mode of work*/
		fread(p_mode_of_work,4,1,bin_file);
		/*read and check the size*/
		fread(p_prg_size,4,1,bin_file);
		/*read the initial parameters*/
		fread(&(p_root->def_rec),2,1,bin_file);
		fread(&(p_root->def_pt),2,1,bin_file);
		fread(&(p_root->n_cls_def_opc),1,1,bin_file);
		fread(&(p_root->def_mask),1,1,bin_file);


		/*revert values if big endian*/
		if(b_little==AG_FALSE)
		{
			ag_u32_revert(p_mode_of_work);

			ag_u32_revert(p_prg_size);

			ag_u16_revert(&(p_root->def_rec));

			ag_u16_revert(&(p_root->def_pt));
		}


		if(n_aux_size<(*p_prg_size))
		{
			fclose(bin_file);
			return AG_E_BAD_PARAMS;
		}/*if size*/




		n_aux_size=(*p_prg_size)/sizeof(ClsEntry);
		for(n_index=0;n_index<n_aux_size;n_index++)
		{
			if(fread ((p_prg+n_index) ,4 , 1 , bin_file) < 1)
			{
				return AG_E_FAIL;
			}/*read*/


			if(b_little==AG_FALSE)
			{

				ag_u32_revert((p_prg+n_index));

			}
		}
		fclose(bin_file);
		return AG_S_OK;

}


AgResult ag_clsb_dump (const AG_CHAR *output_file , MpId x_mpid)
{
	AgResult x_res;
	MiniPrgDesc* p_mp;

	x_res = cls_get_mpdesc(x_mpid , &p_mp);

	if (p_mp->x_stdprg.p_mem_handle->n_cls_mode == AG_CLS_BIT_MODE_8)
	{
		return (dump_8(output_file, &(p_mp->x_stdprg)));
	}
	else
	{
		return (dump_16(output_file, &(p_mp->x_stdprg)));
	}
}

AgResult ag_clsb_cls_prg_dump(const AG_CHAR *output_file, AgClsbMemHndl x_mem_handle)
{
	AgClsbMem *p_cls_mem;

#ifndef NO_VALIDATION
    if(!CLSB_VALID_MEM_HNDL(x_mem_handle))
    {
       return AG_E_BAD_PARAMS; 
    }
#endif

    p_cls_mem = aClsMem[x_mem_handle];

	return clsb_eng_rec_dump_bin_8 (output_file, p_cls_mem);
}


AgResult ag_clsb_dump_bin (const AG_CHAR *output_file , MpId x_mpid)
{
	AgResult x_res;
	MiniPrgDesc* p_mp;

	x_res = cls_get_mpdesc(x_mpid , &p_mp);

	if (p_mp->x_stdprg.p_mem_handle->n_cls_mode == AG_CLS_BIT_MODE_8)
	{
		return (dump_bin_8(output_file, &(p_mp->x_stdprg)));
	}
	else
	{
		return (dump_bin_16(output_file, &(p_mp->x_stdprg)));
	}
}

AgResult ag_clsb_dump_vl (const AG_CHAR *output_file ,MpId x_mpid)
{
	AgResult x_res;
	MiniPrgDesc* p_mp;

	x_res = cls_get_mpdesc(x_mpid , &p_mp);

	if (p_mp->x_stdprg.p_mem_handle->n_cls_mode == AG_CLS_BIT_MODE_8)
	{
		return (dump_vl_8(output_file, &(p_mp->x_stdprg)));
	}
	else
	{
		return (dump_vl_16(output_file, &(p_mp->x_stdprg)));
	}
}

AgResult ag_clsb_mph_dump(const AG_CHAR *p_filename)
{
	AgResult x_res;

	x_res = mph_dump(p_filename);

	return x_res;
}
#endif /* AG_MNT */
/*********************************************************************************
  End of Print Area - Only prints if CLS_PRINT DEBUG IS DEFINED
*********************************************************************************/
#undef CLSB_API_C 

