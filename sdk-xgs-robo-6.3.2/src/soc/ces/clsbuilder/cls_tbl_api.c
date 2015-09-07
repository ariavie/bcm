#ifndef BCM_CES_SDK

/*******************************************************************

Copyright Redux Communications Ltd.

Module Name: 

File Name: 
    cls_tbl_api.c

File Description: 

History:
    Date            Name            Comment
    ----            ----            -------
    10/05/01
*******************************************************************/
#define CLS_TBL_API_C 
#define REVISION "$Revision: 1.5 $" /* Clearcase revision number to be used in TRACE & SWERR */

#include "ag_common.h"
#ifdef BCM_CES_SDK
#include "bcm_ces_sal.h"
#else
#include "agos/agos.h"
#include "flow/flow.h"
#endif

#include "classification/cls_tbl_api.h"
#include "drivers/ag_hw_regs.h"
#include "utils/memutils.h"
#include "classification/clsb_types.h"
/*#include "classification/cls_mem_mng.hpp" */

static AG_U32 nCtSize;  /*the number of entries in the table.*/
static const AgClsTblEntry xDefEntry =
{
    0x100,0,0,0,0,0,0,NULL
};

static const AgClsTblEntry xAllocFrameEntry =
{
    0xFF,0,0,0,0,0,0,NULL
};


#define   CT_VALID_ENTRY(idx)   (idx < nCtSize ? AG_TRUE : AG_FALSE)


/*
Name: ag_ct_config
Description: 
    sets the base address and the size of the classification table.
    Note that each entry has the size of AgClsTblEntry structure.
    The function sets the entries before AG_CLSB_DEF_ALLOC_FRAME_TAG to the cls table 
    default entry (silent discard). it is possible to later overwrite some of them.
    The function sets the default alloc-frame (important to have correct values on CPU transmit) tag 
    The other entries are not initizlized (Use ag_ct_set_entries for setting the entries).
    Note: It is possible to set any entry later (including the well known) 
Parameters:
	void* p_cls_tbl_base: A pointer to an allocated memory where the classification
        table starts. This is the base address of the classification table.
        It must be aligned to 4KB.
	AG_U32 n_num_of_entries: The number of classification table entries.
Returns AgResult :
    AG_S_OK on success
    AG_E_NULL_PTR: p_cls_tbl_base is NULL.
    AG_E_BAD_PARAMS : if the base is not aligned or the number of entries is
        larger then AG_CT_MAX_SIZE.
*/
AgResult ag_ct_config(void* p_cls_tbl_base, AG_U32 n_num_of_entries)
{

#ifndef NO_VALIDATION
    AG_U32 n_align_mask = AG_REG_BASE_ALIGN - 1;
    if(!p_cls_tbl_base)
    {
        return AG_E_NULL_PTR;
    }
    if ((AG_U32)p_cls_tbl_base & n_align_mask)
    { /*not aligned properly*/
   		AGOS_SWERR(AGOS_SWERR_WARNING,"[Cls_Tbl:ag_ct_config] Base not aligned",(AG_U32)p_cls_tbl_base,0,0,0,0);
        return AG_E_BAD_PARAMS;
    }
    if(n_num_of_entries > AG_CT_MAX_SIZE || n_num_of_entries < AG_CT_MIN_SIZE)
    {
       	AGOS_SWERR(AGOS_SWERR_WARNING,"[Cls_Tbl:ag_ct_config] invalid table size",n_num_of_entries,0,0,0,0);
        return AG_E_BAD_PARAMS;
    }
#endif

    /*set the HW register*/
    *AG_REG_CT_BASE = AG_PTR_TO_HW_BASE(p_cls_tbl_base);

    /* set the global variables. needed for the functions that are caled next*/
    pCtBase = (AgClsTblEntry *)p_cls_tbl_base;
    nCtSize = n_num_of_entries;

    /* No need to set the general cls table entries to any specific value */
    /* Need to set well known entries with their needed values. */

    /* setting the entries before AG_CLSB_DEF_ALLOC_FRAME_TAG to the cls table  */
    /* default entry (silent discard). it is possible to later overwrite some of them. */
    ag_ct_set_entries(0, AG_CLSB_DEF_ALLOC_FRAME_TAG , NULL);

    /* set the default alloc-frame (important to have correct values on CPU transmit) tag  */
    /* cast used to remove const qualifier. */
    ag_ct_set_entries(AG_CLSB_DEF_ALLOC_FRAME_TAG, 1 , (AgClsTblEntry*)&xAllocFrameEntry);


    return AG_S_OK;

}


/*
Name: ag_ct_get_config
Description: 
    gets the configuration parameters that were previouly set by the
    ag_ct_config functions. These are the classification table base
    and the number of entries in the classification table.
Parameters:
	void** p_cls_tbl_base: In this parameter the base address is returned
	AG_U32* p_num_of_entries: In this parameter the number of entries is returned
Returns AgResult :
    AG_S_OK on success
    AG_E_FAIL: if there is an inconsistency between the HW register value and the
        internal global value. This is an indication that some one over-wrote by 
        mistake one of the two.
*/
AgResult ag_ct_get_config(void** p_cls_tbl_base, AG_U32* p_num_of_entries)
{

#ifndef NO_VALIDATION
    if(pCtBase != AG_HW_BASE_TO_PTR(*AG_REG_CT_BASE))
    {
        void* p_hw_base = AG_HW_BASE_TO_PTR(*AG_REG_CT_BASE);
   		AGOS_SWERR(AGOS_SWERR_WARNING,"[Cls_Tbl:ag_ct_get_config] Base inconsistency",(AG_U32)pCtBase,(AG_U32)p_hw_base,0,0,0);
        return AG_E_FAIL;
    }
#endif

    *p_cls_tbl_base = (void*)pCtBase;
    *p_num_of_entries = nCtSize;
    return AG_S_OK;
}


/*
Name: ag_ct_set_entries
Description: 
    The function sets the content of a given number of entries, starting from a given
    index. The content of the structure pointed by the p_entry_val parameter is copied
    to all the requested entries. If no entry content is given as an input then the 
    default entry content will be set.
Parameters:
	AG_U32 n_start_tag: The index to start from
	AG_U32 num_of_entries: The number of entries to be set
	AgClsTblEntry* p_entry_val: A pointer to a structure whose content is to be copied
        to each of the specified entries.
        If this parameter is NULL then the default entry is set in each of the requested 
        entries.
Returns AgResult :
    AG_S_OK on success
    AG_E_OUT_OF_RANGE - the requested range is out of the table range.
*/
AgResult ag_ct_set_entries(AG_U32 n_start_tag, AG_U32 num_of_entries, AgClsTblEntry* p_entry_val)
{

#ifndef NO_VALIDATION
    if (!CT_VALID_ENTRY(n_start_tag + num_of_entries -1))
    {
   		AGOS_SWERR(AGOS_SWERR_WARNING,"[Cls_Tbl:ag_ct_set_entries] out of range",0,0,0,0,0);
        return AG_E_OUT_OF_RANGE;
    }

#endif

    if (!p_entry_val)
    {
        p_entry_val = (AgClsTblEntry*) &xDefEntry; /*cast to remove const specifier*/
    }

    AG_MEMSET_8WORD(&pCtBase[n_start_tag],p_entry_val, (AG_CT_ENT_SIZE * sizeof(AG_U32)) * num_of_entries);
    return AG_S_OK;
}

/*
Name: ag_ct_set_entry
Description: 
    The function sets the content of an entry. 
	The content of the structure pointed by the p_entry_val parameter is copied
    to the entry. 
Parameters:
	AG_U32 n_start_tag: The index to start from
	AgClsTblEntry* p_entry_val: A pointer to a structure whose content is to be copied
        to each of the specified entry.
Returns AgResult :
    AG_S_OK on success
	AG_E_FAIL - if athe pointer to the structure is NULL
*/
AgResult ag_ct_set_hw_act(AG_U32 n_tag, const AgClsTblEntry* p_entry_val)
{
	AG_U32 i;

#ifndef NO_VALIDATION
    if (!CT_VALID_ENTRY(n_tag))
    {
   		AGOS_SWERR(AGOS_SWERR_WARNING,"[Cls_Tbl:ag_ct_set_hw_act] tag out of range",n_tag,0,0,0,0);
        return AG_E_OUT_OF_RANGE;
    }
    if (!p_entry_val)
    {
        return AG_E_FAIL;
    }
#endif


	for (i=0;i<AG_CT_ENT_SIZE-1;i++)
	{
		pCtBase[n_tag].a_val[i] = p_entry_val->a_val[i];
	}

    return AG_S_OK;
}
/*
Name: ag_ct_set_gpc_act
Description: 
    Sets GPC action for the specified GPC group.
    The function receives the group number, the tag and the action.
    Note that this action overrides existing GPC index and control
Parameters:
	AG_U32 n_tag: Specifies which table entry should be set. The tag is an index 
        to the table.
	AgGpcCntGrp e_group_num: The group number.
	AgClsGpcAct* p_gpc_act: A pointer to a structure that contains the GPC index
        and the GPC control. For the control, use a bitwise OR of the following flags:
        AG_CLS_BYTE_CNT_EN - to enable byte count.
        AG_CLS_FRM_CNT_EN - to enable frame count
        AG_CLS_TS_EN      - to enable time stamp update.
Returns AgResult :
    AG_S_OK on success
    AG_E_BAD_PARAMS - if group number is not in the range of 1 to 6 or p_gpc_act is NULL
    AG_E_OUT_OF_RANGE - If the tag is out of the table range.

*/
AgResult ag_ct_set_gpc_act(AG_U32 n_tag, AgGpcCntGrp e_group_num, AgClsGpcAct* p_gpc_act)
{

#ifndef NO_VALIDATION
    if (!CT_VALID_ENTRY(n_tag))
    {
   		AGOS_SWERR(AGOS_SWERR_WARNING,"[Cls_Tbl:ag_ct_set_gpc_act] tag out of range",n_tag,nCtSize,0,0,0);
        return AG_E_OUT_OF_RANGE;
    }
    if(e_group_num > AG_GPC_COUNTER_GROUP6  || !p_gpc_act)
    {
        AGOS_SWERR(AGOS_SWERR_WARNING,"[Cls_Tbl:ag_ct_set_gpc_act] bad group num or null pointer",e_group_num,(AG_U32)p_gpc_act,0,0,0);
        return AG_E_BAD_PARAMS;
    }
#endif

    switch(e_group_num)
    {
        case AG_GPC_COUNTER_GROUP1:
            /*set grp1 index*/
            ag_set_range_value((AG_U32*)&pCtBase[n_tag].a_val[4], p_gpc_act->n_grp_index, 11, 21);
            /*set grp1 control*/
            ag_set_range_value((AG_U32*)&pCtBase[n_tag].a_val[5], p_gpc_act->n_grp_ctrl, 13, 3);
            break;
        case AG_GPC_COUNTER_GROUP2:
			/*set grp2 index*/
            ag_set_range_value((AG_U32*)&pCtBase[n_tag].a_val[4], p_gpc_act->n_grp_index, 0, 11);
			/* set grp2 control */
            ag_set_range_value((AG_U32*)&pCtBase[n_tag].a_val[5], p_gpc_act->n_grp_ctrl, 10, 3);
            break;
        case AG_GPC_COUNTER_GROUP3:
            /*set grp3 index*/
            ag_set_range_value((AG_U32*)&pCtBase[n_tag].a_val[5], p_gpc_act->n_grp_index, 16, 16);
            /* set grp3 control */
            ag_set_range_value((AG_U32*)&pCtBase[n_tag].a_val[5], p_gpc_act->n_grp_ctrl, 7, 3);
            break;
        case AG_GPC_COUNTER_GROUP4:
            /*set grp4 index*/
            ag_set_range_value((AG_U32*)&pCtBase[n_tag].a_val[6], p_gpc_act->n_grp_index, 20, 12);
            /* set grp4 control */
            ag_set_range_value((AG_U32*)&pCtBase[n_tag].a_val[5], p_gpc_act->n_grp_ctrl, 4, 3);
            break;
        case AG_GPC_COUNTER_GROUP5:
            /*set grp5 index*/
            ag_set_range_value((AG_U32*)&pCtBase[n_tag].a_val[6], p_gpc_act->n_grp_index, 10, 10);
            /* set grp5 control */
            ag_set_range_value((AG_U32*)&pCtBase[n_tag].a_val[5], p_gpc_act->n_grp_ctrl, 2, 2);
            break;
        case AG_GPC_COUNTER_GROUP6:
            /*set grp6 index*/
            ag_set_range_value((AG_U32*)&pCtBase[n_tag].a_val[6], p_gpc_act->n_grp_index, 0, 10);
            /* set grp6 control */
            ag_set_range_value((AG_U32*)&pCtBase[n_tag].a_val[5], p_gpc_act->n_grp_ctrl, 0, 2);
            break;
    }

    return AG_S_OK;
}

/*
Name: ag_ct_get_gpc_act
Description: 
    Gets GPC action for the specified GPC group.
    The function receives the group number, the tag and an allocated GPC action
    structure that will be filled with the action.
Parameters:
	AG_U32 n_tag: Specifies which table entry should be read. The tag is an index 
        to the table.
	AgGpcCntGrp e_group_num: The group number
	AgClsGpcAct* p_gpc_act: A pointer to an allocated structure that will be filled
        with the action specified in the classification table entry.
        To check the control value, use the following flags:
        if AG_CLS_BYTE_CNT_EN bit is set, then byte count is enabled.
        if AG_CLS_FRM_CNT_EN bit is set, then frame count is enabled.
        if AG_CLS_TS_EN bit is set, then time stamp update is enabled.
Returns AgResult :
    AG_S_OK on success
    AG_E_BAD_PARAMS - if group number is not in the range of 1 to 6 or p_gpc_act is NULL 
    AG_E_OUT_OF_RANGE - If the tag is out of the table range.

*/
AgResult ag_ct_get_gpc_act(AG_U32 n_tag, AgGpcCntGrp e_group_num, AgClsGpcAct* p_gpc_act)
{
    AG_U32 n_grp_ctrl;

#ifndef NO_VALIDATION
    if (!CT_VALID_ENTRY(n_tag))
    {
   		AGOS_SWERR(AGOS_SWERR_WARNING,"[Cls_Tbl:ag_ct_get_gpc_act] tag out of range",n_tag,nCtSize,0,0,0);
        return AG_E_OUT_OF_RANGE;
    }
    if(e_group_num > AG_GPC_COUNTER_GROUP6 || !p_gpc_act)
    {
        AGOS_SWERR(AGOS_SWERR_WARNING,"[Cls_Tbl:ag_ct_get_gpc_act] bad group num or null pointer",e_group_num,(AG_U32)p_gpc_act,0,0,0);
        return AG_E_BAD_PARAMS;
    }
#endif

    switch(e_group_num)
    {
        case AG_GPC_COUNTER_GROUP1:
            /*get grp1 index*/
            ag_read_range_value((AG_U32*)&pCtBase[n_tag].a_val[4], 11, &p_gpc_act->n_grp_index, 21);
            /*get grp1 control*/
            ag_read_range_value((AG_U32*)&pCtBase[n_tag].a_val[5], 13, &n_grp_ctrl, 3);
            break;
        case AG_GPC_COUNTER_GROUP2:
			/*get grp2 index*/
            ag_read_range_value((AG_U32*)&pCtBase[n_tag].a_val[4], 0, &p_gpc_act->n_grp_index, 11);
			/* get grp2 control */
            ag_read_range_value((AG_U32*)&pCtBase[n_tag].a_val[5], 10, &n_grp_ctrl, 3);
            break;
        case AG_GPC_COUNTER_GROUP3:
            /*get grp3 index*/
            ag_read_range_value((AG_U32*)&pCtBase[n_tag].a_val[5], 16, &p_gpc_act->n_grp_index, 16);
            /* get grp3 control */
            ag_read_range_value((AG_U32*)&pCtBase[n_tag].a_val[5], 7, &n_grp_ctrl, 3);
            break;
        case AG_GPC_COUNTER_GROUP4:
            /*get grp4 index*/
            ag_read_range_value((AG_U32*)&pCtBase[n_tag].a_val[6], 20, &p_gpc_act->n_grp_index, 12);
            /* get grp4 control */
            ag_read_range_value((AG_U32*)&pCtBase[n_tag].a_val[5],4, &n_grp_ctrl, 3);
            break;
        case AG_GPC_COUNTER_GROUP5:
            /*get grp5 index*/
            ag_read_range_value((AG_U32*)&pCtBase[n_tag].a_val[6], 10, &p_gpc_act->n_grp_index, 10);
            /* get grp5 control */
            ag_read_range_value((AG_U32*)&pCtBase[n_tag].a_val[5], 2, &n_grp_ctrl, 2);
            break;
        case AG_GPC_COUNTER_GROUP6:
            /*get grp6 index*/
            ag_read_range_value((AG_U32*)&pCtBase[n_tag].a_val[6], 0, &p_gpc_act->n_grp_index, 10);
            /* get grp6 control */
            ag_read_range_value((AG_U32*)&pCtBase[n_tag].a_val[5], 0, &n_grp_ctrl, 2);
            break;
    }
    p_gpc_act->n_grp_ctrl = (AgMtrType)n_grp_ctrl;

    return AG_S_OK;
}


/*
Name: ag_ct_set_sw_act
Description: 
    Sets the SW action for the specified tag.
    The function receives the tag and the SW action.
    if sw action already exists it is overridden.
    If an action does not exist yet, its memory is allocated and set.
    This memory can be freed by ag_ct_del_sw_act.
Parameters:
	AG_U32 n_tag: Specifies which table entry should be set. The tag is an index 
        to the table.
	AgClsSwAct* p_sw_act: A pointer to the structure that contains the parameters 
    of the SW action.
Returns AgResult :
    AG_S_OK on success
    AG_E_OUT_OF_RANGE - If the tag is out of the table range.
    AG_E_BAD_PARAMS - For invalid FlowQFullAction
    AG_E_OUT_OF_MEM - failed allocating buffer for SW action struct.
*/

AgResult ag_ct_set_sw_act(AG_U32 n_tag, const AgClsSwAct* p_sw_act)
{
    AgFlow* p_act;

#ifndef NO_VALIDATION
    if (!CT_VALID_ENTRY(n_tag))
    {
   		AGOS_SWERR(AGOS_SWERR_WARNING,"[Cls_Tbl:ag_ct_set_sw_act] tag out of range",n_tag,nCtSize,0,0,0);
        return AG_E_OUT_OF_RANGE;
    }
    if(p_sw_act->e_action > 2)
    {
       	AGOS_SWERR(AGOS_SWERR_WARNING,"[Cls_Tbl:ag_ct_set_sw_act] bad e_action parameter",p_sw_act->e_action,0,0,0,0);
        return AG_E_BAD_PARAMS;
    }
#endif

    p_act = pCtBase[n_tag].p_sw_act; 
    if(!p_act)
    { 
        if(AG_FAILED(ag_alloc_flow_struct(&p_act)))
        {
            AGOS_SWERR(AGOS_SWERR_WARNING,"[Cls_Tbl:ag_ct_set_sw_act] failed flow allocation ",0,0,0,0,0);
            return AG_E_OUT_OF_MEM;
        }
        /*in case this is a newly allocated structure, need to set it in the entry.*/
        pCtBase[n_tag].p_sw_act = p_act;
    }

    /*set the SW action parameters.*/
    p_act->x_cls_sw_act.e_action =   p_sw_act->e_action;
    p_act->x_cls_sw_act.p_arg    =   p_sw_act->p_arg;
    p_act->x_cls_sw_act.p_queue  =   p_sw_act->p_queue;

    return AG_S_OK;

}


/*
Name: ag_ct_get_sw_act
Description: 
    returns the SW action for the specified tag (if it exists).
Parameters:
	AG_U32 n_tag:
	AgClsSwAct* p_sw_act: an allocated structure that will be set with the SW action
        parameters
Returns AgResult :
    AG_S_OK
    AG_S_EMPTY - no SW action is set for the tag. !!!!!!!!!! note that this is a success value.
    AG_E_NULL_PTR
    AG_E_OUT_OF_RANGE
*/
AgResult ag_ct_get_sw_act(AG_U32 n_tag, AgClsSwAct* p_sw_act)
{
    AgFlow* p_act;
#ifndef NO_VALIDATION
    if (!CT_VALID_ENTRY(n_tag))
    {
   		AGOS_SWERR(AGOS_SWERR_WARNING,"[Cls_Tbl:ag_ct_get_sw_act] tag out of range",n_tag,nCtSize,0,0,0);
        return AG_E_OUT_OF_RANGE;
    }
    if(!p_sw_act)
    {
       	AGOS_SWERR(AGOS_SWERR_WARNING,"[Cls_Tbl:ag_ct_get_sw_act] NULL pointer parameter",0,0,0,0,0);
        return AG_E_NULL_PTR;
    }
#endif

    p_act = pCtBase[n_tag].p_sw_act; 
    if(p_act)
    {
        p_sw_act->e_action = p_act->x_cls_sw_act.e_action;
        p_sw_act->p_arg= p_act->x_cls_sw_act.p_arg;
        p_sw_act->p_queue = p_act->x_cls_sw_act.p_queue;
        return AG_S_OK;
    }
    else
    {
        return AG_S_EMPTY;
    }


}


/*
Name: ag_ct_del_sw_act
Description: 
    Deletes the SW action for the specified tag (if it exists).
    This function free's the memory allocated in ag_ct_set_sw_act
    If p_sw_act is not NULL, the SW action parameters are returned in this structure.
Parameters:
	AG_U32 n_tag:
	AgClsSwAct* p_sw_act: 
Returns AgResult :
    AG_S_OK
    AG_S_EMPTY - no SW action is set for the tag.
    AG_E_NULL_PTR
    AG_E_OUT_OF_RANGE
*/
AgResult ag_ct_del_sw_act(AG_U32 n_tag, AgClsSwAct* p_sw_act)
{
    AgFlow* p_act;

#ifndef NO_VALIDATION
    if (!CT_VALID_ENTRY(n_tag))
    {
   		AGOS_SWERR(AGOS_SWERR_WARNING,"[Cls_Tbl:ag_ct_del_sw_act] tag out of range",n_tag,nCtSize,0,0,0);
        return AG_E_OUT_OF_RANGE;
    }

#endif
    p_act = pCtBase[n_tag].p_sw_act; 
    if(p_act)
    {
        if(p_sw_act)
        {
            p_sw_act->e_action = p_act->x_cls_sw_act.e_action;
            p_sw_act->p_arg= p_act->x_cls_sw_act.p_arg;
            p_sw_act->p_queue = p_act->x_cls_sw_act.p_queue;
        }

        ag_free_flow_struct(p_act);
		pCtBase[n_tag].p_sw_act = NULL;
        return AG_S_OK;
    }
    else
    {
        return AG_S_EMPTY;
    }

}
/*
Name: ag_ct_init_entry
Description: 
    Initialize the structure pointer by p_entry to the default entry value.
    This function basically returns a copy of the default entry.
Parameters:
	AgClsTblEntry* p_entry: a pointer to an allocated structre to be initialized.
Returns AgResult :
    AG_S_OK on success
    AG_E_NULL_PTR: if p_entry is NULL.
*/
AgResult ag_ct_init_entry(AgCtEntryType e_entry_type, AgClsTblEntry* p_entry)
{
    AG_U32 i;
    AgClsTblEntry* p_copied_entry;

#ifndef NO_VALIDATION
    if(!p_entry)
    {
   		AGOS_SWERR(AGOS_SWERR_WARNING,"[Cls_Tbl:ag_ct_init_entry] p_entry is NULL.",0,0,0,0,0);
        return AG_E_NULL_PTR;
    }
    if(e_entry_type > AG_CT_ALLOC_FRAME_ENTRY)
    {
        return AG_E_BAD_PARAMS;
    }
#endif
    
    if (e_entry_type == AG_CT_DEF_ENTRY)
    {
        p_copied_entry = (AgClsTblEntry*)&xDefEntry;
    }
    else if (e_entry_type == AG_CT_ALLOC_FRAME_ENTRY)
    {
        p_copied_entry = (AgClsTblEntry*)&xAllocFrameEntry;
    }
	else
	{
		return AG_E_FAIL;
	}

    for (i = 0; i < AG_CT_ENT_SIZE ; i++)
    {
        p_entry->a_val[i] = p_copied_entry->a_val[i];
    }
    return AG_S_OK;
}


AgResult ag_ct_get_queue(AG_U32 n_tag, AG_U8* p_val)
{

#ifndef NO_VALIDATION
    if (!CT_VALID_ENTRY(n_tag))
    {
   		AGOS_SWERR(AGOS_SWERR_WARNING,"[Cls_Tbl:ag_ct_get_queue] tag out of range",n_tag,nCtSize,0,0,0);
        return AG_E_OUT_OF_RANGE;
    }
    if(!p_val)
    {
        AGOS_SWERR(AGOS_SWERR_WARNING,"[Cls_Tbl:ag_ct_get_queue] null pointer",0,0,0,0,0);
        return AG_E_NULL_PTR;
    }
#endif

    *p_val = (AG_U8)pCtBase[n_tag].a_val[0];
    return AG_S_OK;
}


AgResult ag_ct_set_queue(AG_U32 n_tag, AG_U8 n_val)
{
#ifndef NO_VALIDATION
    if (!CT_VALID_ENTRY(n_tag))
    {
   		AGOS_SWERR(AGOS_SWERR_WARNING,"[Cls_Tbl:ag_ct_set_queue] tag out of range",n_tag,nCtSize,0,0,0);
        return AG_E_OUT_OF_RANGE;
    }
#endif

    ag_set_range_value((AG_U32*)&pCtBase[n_tag].a_val[0], n_val, 0, 8);
    return AG_S_OK;
}

/****************************************************************
Name: ag_alloc_flow_struct
Description: 
    allocates a flow structure. The purpose of this function is to give abstruction
    to the way the flow structre is allocated.
Parameters:
	AgFlow** p_flow:
        the parameter in which the allocated pointer will be returned.
Returns AgResult :
****************************************************************/
AgResult ag_alloc_flow_struct(AgFlow** p_flow)
{
	AgResult n_res;

	/*  Shabtay Alloc */
	n_res = agos_calloc(32,  (void **)p_flow);

#if 0
#ifdef AG_CLSB_RS120_ALLOC
	n_res = agos_alloc_buffer(&xAgGlobals.x_part.x_part32, (void **)p_flow);
#else
    if(sizeof(AgFlow) > 12)
    {
        /* a larger inheriting class did not implement the operator new */
        AGOS_SWERR(AGOS_SWERR_WARNING, "[mp handler]alloc_leaf: size= expected_size=", sizeof(AgFlow), 12 ,0,0,0);
		*p_flow = NULL;
        return AG_E_FAIL;
    }
	n_res = CAgClsMemMng::instance()->get_buf_pool12().allocate_buffer((void*&)*p_flow);
#endif
#endif
	if(AG_FAILED(n_res))
    {
		AGOS_SWERR(AGOS_SWERR_WARNING,"ag_alloc_flow_struct: allocation is failed n_res=",n_res,0,0,0,0);
    }
    else
    {
        nCurrFlowAlloc++;
#if defined AG_MEM_MONITOR
        nMaxFlowAlloc = ag_max(nCurrFlowAlloc,nMaxFlowAlloc);
#endif
    }
    return n_res;
}


/****************************************************************
Name: ag_free_flow_struct
Description: 
    frees a flow structure. The purpose of this function is to give abstruction
    to the way the flow structre is freed.
Parameters:
	AgFlow* p_flow: the pointer to free
Returns AgResult :
****************************************************************/
void ag_free_flow_struct(AgFlow* p_flow)
{
    nCurrFlowAlloc--;

/* Shabtay Free */
	agos_free(p_flow);

#if 0	
#ifdef AG_CLSB_RS120_ALLOC
	agos_free_buffer(&xAgGlobals.x_part.x_part32, (void*)p_flow);
#else
	CAgClsMemMng::instance()->get_buf_pool12().deallocate_buffer((void*)p_flow);
#endif
#endif
}

#undef CLS_TBL_API_C 
int cls_tbl_dummy(int j)
{
    return j;
}
#endif /*BCM_CES_SDK*/
