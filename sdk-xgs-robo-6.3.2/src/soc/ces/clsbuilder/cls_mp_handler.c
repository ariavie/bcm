/*******************************************************************

Copyright Redux Communications Ltd.

Module Name: 

File Name: 
    cls_mp_handler.c

File Description: 

History:
    Date            Name            Comment
    ----            ----            -------
    08/05/01
*******************************************************************/
#define CLS_MP_HANDLER_C  

#define REVISION "$Revision: 1.8 $" /* Visual SourceSafe automatic revision number to be used in TRACE & SWERR */
#ifdef BCM_CES_SDK
#include "ag_common.h"
#ifndef BCM_CES_SDK
#include <stdio.h>
#include <stdlib.h>
#endif
#include "bcm_ces_sal.h"
#include "classification/cls_results.h"
#include "utils/memutils.h"
#include "pub/cls_engine.h"
#include "pub/clsb_types_priv.h"

#else /*BCM_CES_SDK*/
#include "ag_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "agos/agos.h"
#include "classification/cls_results.h"
#include "drivers/ag_hw_regs.h"
#include "utils/memutils.h"
#include "pub/cls_engine.h"
#include "pub/clsb_types_priv.h"
#endif /*BCM_CES_SDK*/

/*#include "utils/agos_utils.hpp" */
/*#include "classification/cls_mem_mng.hpp" */



#define BCM_SEMAPHORE
/*********************************************************
**          STATIC (PRIVATE) VARIABLES                  **
**********************************************************/

static AG_U32 nMaxMp;  /*The maximal number of MPs that can exist in the
                       system concurrently*/
static AG_U32 nMaxGrp;  /*The maximal number of MPs that can exist in the
                       system concurrently*/

/* static AG_U32 nMaxClsPrg; The maximal number of classification programs.
                         ClsPrg number range is 0 to (nMaxClsPrg-1)  */

/*
The MP Container implementation:
--------------------------------
The size of the table is nMaxMp.
The index of the table is used as the ID.
The table entry value is the p-ointer to the MiniPrgDesc.
A NULL entry means the id is free.
If the implementation of the MiniPrgDesc container changes, the following functions 
should be changed: cls_get_mpdesc,remove_mpdesc, cls_create_mp, mph_configure and
clsb_mph_reset
*/

static MiniPrgDesc **pMpTable; /*array of MiniPrgDesc pointers*/
AgosBiSemaphoreInfo  xMpSem;  /* Semaphore to synchronize access to MP table */

/*
The Group Container implementation:
-----------------------------------
The size of the table is nMaxGrp.
Each table entry is a LeafGrpEntry which has in it the
group id and a pointer to the LeafGrp.
if the LeafGrp pointer of an entry is NULL then the entry 
is free.
(Remember that the IDs are received from the caller.)
If the implementation of the LeafGrp container changes, the following functions 
should be changed: cls_get_leaf_grp,remove_leaf_grp, cls_create_leaf_grp, mph_configure and
clsb_mph_reset
*/

static LeafGrp **pGrpTable; /*array of LeafGrp pointers */
AgosBiSemaphoreInfo  xGrpSem;  /* Semaphore to synchronize access to MP table */

/*********************************************************
**          STATIC (PRIVATE) FUNCTION PROTOTYPES        **
**********************************************************/
static MiniPrgDesc* remove_mpdesc( MpId x_id);
static LeafGrp* remove_leaf_grp( LeafGrpId x_grp_id);

/*nts = non thread safe. Means semaphore is taken.*/
static AgResult cls_get_mpdesc_nts( MpId x_mp_id,  MiniPrgDesc **p_mp);
static MiniPrgDesc* remove_mpdesc_nts( MpId x_id);
static LeafGrp* remove_leaf_grp_nts(LeafGrpId x_grp_id);

static AgResult handle_root_update(MiniPrgDesc *p_mp /*,MpNodeUpdate *p_update*/);
static AgResult handle_leaves_update(MiniPrgDesc *p_mp,MpNodeUpdate *p_update);

static AgResult find_leaf( AG_U32 n_record,        
                           AG_U32 n_pathtag,        
                           TableType e_table,   
                           AG_U16 n_data,          
                           List* p_list, /* the list to search */
                           LeafPtrEl **p_leaf_element);

static AgResult find_this_leaf( Leaf *p_this_leaf, 
                         List* p_list,
                         LeafPtrEl **p_leaf_element);

static AgResult find_this_grp( LeafGrp* p_this_grp,
                         List* p_list,
                         LeafGrpPtrEl **p_grp_element);

static AgResult release_leaf_grp( LeafGrp* p_grp);

/*static AG_BOOL grp_has_members_in_mp( List *p_leaves, MiniPrgDesc* p_mp);*/

#ifdef AG_MNT
static void print_leaves(FILE *p_file, List *p_list);
static void print_mp_info(FILE *p_file, MiniPrgDesc* p_mp,MpId x_id);
static void print_grp_info(FILE *p_file, LeafGrp* p_grp, LeafGrpId x_id);
#endif /* AG_MNT */

/* //////////////////////////// LEAF memory management //////////////////// */
static AgResult alloc_leaf(Leaf** p_leaf)
{
    /* Allocate and initialize the Leaf. */
	AgResult n_res;

/*  Shabtay Alloc */
	n_res = agos_calloc(32,  (void **)p_leaf);

#if 0	/* replaced by Shabtay Alloc */	
#ifdef AG_CLSB_RS120_ALLOC
	n_res = agos_alloc_buffer(&xAgGlobals.x_part.x_part32, (void **)p_leaf);
#else
    if(sizeof(Leaf) > 24)
    {
        /* a larger inheriting class did not implement the operator new */
        AGOS_SWERR(AGOS_SWERR_WARNING, "[mp handler]alloc_leaf: size= expected_size=", sizeof(Leaf), 24 ,0,0,0);
		*p_leaf = NULL;
        return AG_E_FAIL;
    }
	n_res = CAgClsMemMng::instance()->get_buf_pool24().allocate_buffer((void*&)*p_leaf);
#endif
#endif
	if(AG_FAILED(n_res))
    {
		AGOS_SWERR(AGOS_SWERR_WARNING,"alloc_leaf: allocation is failed n_res=",n_res,0,0,0,0);
		*p_leaf = NULL;
    }
    else
    {
#ifdef AG_MEM_MONITOR
        nCurrLeafAlloc++;
        nMaxLeafAlloc = ag_max(nCurrLeafAlloc,nMaxLeafAlloc);
#endif
    }
    return n_res;
}
static void free_leaf(Leaf* p_leaf)
{
#ifdef AG_MEM_MONITOR
    nCurrLeafAlloc--;
#endif

	/* Shabtay Free */
	agos_free(p_leaf);

#if 0	
#ifdef AG_CLSB_RS120_ALLOC
	agos_free_buffer(&xAgGlobals.x_part.x_part32, (void*)p_leaf);
#else
	CAgClsMemMng::instance()->get_buf_pool24().deallocate_buffer((void*)p_leaf);
#endif
#endif
}

 /*////////////////////////// GROUP memory management //////////////////// */ 
static AgResult alloc_grp(LeafGrp** p_grp)
{
	AgResult n_res;

/*  Shabtay Alloc */
	n_res = agos_calloc(64,  (void **)p_grp);

#if 0	/* replaced by Shabtay Alloc */	
#ifdef AG_CLSB_RS120_ALLOC
	n_res = agos_alloc_buffer (&xAgGlobals.x_part.x_part64, (void **)p_grp);
#else
    if(sizeof(LeafGrp) > 24)
    {
        /* a larger inheriting class did not implement the operator new */
        AGOS_SWERR(AGOS_SWERR_WARNING, "[mp handler]alloc_leaf: size= expected_size=", sizeof(LeafGrp), 24 ,0,0,0);
		*p_grp = NULL;
        return AG_E_FAIL;
    }

	n_res = CAgClsMemMng::instance()->get_buf_pool24().allocate_buffer((void*&)*p_grp);
#endif
#endif
	if(AG_FAILED(n_res))
    {
		AGOS_SWERR(AGOS_SWERR_WARNING,"alloc_grp: allocation is failed n_res=",n_res,0,0,0,0);
		*p_grp = NULL;
    }
    else
    {
        nCurrGrpAlloc++;
#ifdef AG_MEM_MONITOR
        nMaxGrpAlloc = ag_max(nCurrGrpAlloc,nMaxGrpAlloc);
#endif
    }
    return n_res;
}

static void free_grp(LeafGrp* p_grp)
{
    nCurrGrpAlloc--;

/* Shabtay Free */
	agos_free(p_grp);

#if 0	
#ifdef AG_CLSB_RS120_ALLOC
	agos_free_buffer(&xAgGlobals.x_part.x_part64, (void*)p_grp);
#else
	CAgClsMemMng::instance()->get_buf_pool24().deallocate_buffer((void*)p_grp);
#endif
#endif
}

/*////////////////////////// LEAF Pointer memory management //////////////////// */
static AgResult alloc_leaf_ptr(LeafPtrEl** p_ptr)
{
	AgResult n_res;

/*	Shabtay Alloc */
	n_res = agos_calloc(32, (void **)p_ptr);

#if 0	
#ifdef AG_CLSB_RS120_ALLOC
    n_res = agos_alloc_buffer(&xAgGlobals.x_part.x_part32, (void **)p_ptr);
#else
    if(sizeof(LeafPtrEl) > 12)
    {
        /* a larger inheriting class did not implement the operator new */
        AGOS_SWERR(AGOS_SWERR_WARNING, "[mp handler]alloc_leaf: size= expected_size=", sizeof(LeafPtrEl), 12 ,0,0,0);
		*p_ptr = NULL;
        return AG_E_FAIL;
    }
	n_res = CAgClsMemMng::instance()->get_buf_pool12().allocate_buffer((void*&)*p_ptr);
#endif
#endif
	if(AG_FAILED(n_res))
    {
		AGOS_SWERR(AGOS_SWERR_WARNING,"alloc_leaf_ptr: allocation is failed n_res=",n_res,0,0,0,0);
		*p_ptr = NULL;
    }
    else
    {
#ifdef AG_MEM_MONITOR
        nCurrLeafPtrAlloc++;
        nMaxLeafPtrAlloc = ag_max(nCurrLeafPtrAlloc,nMaxLeafPtrAlloc);
#endif
    }
    return n_res;
}

static void free_leaf_ptr(LeafPtrEl* p_ptr)
{
#ifdef AG_MEM_MONITOR
    nCurrLeafPtrAlloc--;
#endif

	/*	Shabtay free */
	agos_free(p_ptr);

#if 0
#ifdef AG_CLSB_RS120_ALLOC
	agos_free_buffer(&xAgGlobals.x_part.x_part32, (void*)p_ptr);
#else
	CAgClsMemMng::instance()->get_buf_pool12().deallocate_buffer((void*)p_ptr);
#endif
#endif
}

 /*////////////////////////// GROUP Pointer memory management //////////////////// */ 
static AgResult alloc_grp_ptr(LeafGrpPtrEl** p_ptr)
{
	AgResult n_res;

/*  Shabtay Alloc */
	n_res = agos_calloc(32,  (void **)p_ptr);

#if 0	

#ifdef AG_CLSB_RS120_ALLOC
    n_res = agos_alloc_buffer(&xAgGlobals.x_part.x_part32, (void **)p_ptr);
#else
    if(sizeof(LeafGrpPtrEl) > 12)
    {
        /* a larger inheriting class did not implement the operator new */
        AGOS_SWERR(AGOS_SWERR_WARNING, "[mp handler]alloc_leaf: size= expected_size=", sizeof(LeafGrpPtrEl), 12 ,0,0,0);
		*p_ptr = NULL;
        return AG_E_FAIL;
    }

	n_res = CAgClsMemMng::instance()->get_buf_pool12().allocate_buffer((void*&)*p_ptr);
#endif
#endif

	if(AG_FAILED(n_res))
    {
		AGOS_SWERR(AGOS_SWERR_WARNING,"alloc_grp_ptr: allocation is failed n_res=",n_res,0,0,0,0);
		*p_ptr = NULL;
    }
    else
    {
#ifdef AG_MEM_MONITOR
        nCurrGrpPtrAlloc++;
        nMaxGrpPtrAlloc = ag_max(nCurrGrpPtrAlloc,nMaxGrpPtrAlloc);
#endif
    }
    return n_res;
}
static void free_grp_ptr(LeafGrpPtrEl* p_ptr)
{
#ifdef AG_MEM_MONITOR
    nCurrGrpPtrAlloc--;
#endif

/*  Shabtay Alloc */
	agos_free(p_ptr);

#if 0
#ifdef AG_CLSB_RS120_ALLOC
    agos_free_buffer(&xAgGlobals.x_part.x_part32, (void*)p_ptr);
#else
	CAgClsMemMng::instance()->get_buf_pool12().deallocate_buffer((void*)p_ptr);
#endif
#endif
}

/*********************************************************
**          PUBLIC FUNCTIONS                            **
**********************************************************/
/* alloc/free mp are public since clsb_api needs to use them also. */
AgResult alloc_mp(MiniPrgDesc** p_mp)
{

	AgResult n_res;

/*  Shabtay Alloc */
	n_res = agos_calloc(64, (void **)p_mp);

#if 0	
#ifdef AG_CLSB_RS120_ALLOC
	n_res = agos_alloc_buffer (&xAgGlobals.x_part.x_part64, (void**)p_mp);
#else
    if(sizeof(MiniPrgDesc) > 40)
    {
        /* a larger inheriting class did not implement the operator new */
        AGOS_SWERR(AGOS_SWERR_WARNING, "[mp handler]alloc_leaf: size= expected_size=", sizeof(MiniPrgDesc), 40 ,0,0,0);
		*p_mp = NULL;
        return AG_E_FAIL;
    }

	n_res = CAgClsMemMng::instance()->get_buf_pool40().allocate_buffer((void*&)*p_mp);
#endif /* AG_CLSB_RS120_ALLOC */
#endif

	if(AG_FAILED(n_res))
    {
		AGOS_SWERR(AGOS_SWERR_WARNING,"alloc_mp: allocation is failed n_res=",n_res,0,0,0,0);
		*p_mp = NULL;
    }
    else
    {
        nCurrMpAlloc++;
#ifdef AG_MEM_MONITOR
        nMaxMpAlloc = ag_max(nCurrMpAlloc,nMaxMpAlloc);
#endif
    }
    return n_res;
}

void free_mp(MiniPrgDesc* p_mp)
{
    nCurrMpAlloc--;

	/*  Shabtay Alloc */
	agos_calloc(64,  (void **)p_mp);

#if 0	
#ifdef AG_CLSB_RS120_ALLOC
    agos_free_buffer(&xAgGlobals.x_part.x_part64, (void*)p_mp);
#else
	CAgClsMemMng::instance()->get_buf_pool40().deallocate_buffer((void*)p_mp);
#endif
#endif
}

/*
Name: 
    cls_create_mp
Description:
    
Parameters:
     MiniPrgDesc* p_mp: 
     MpId* p_mp_id: on success the id of the created MP is returned
        in this parameter.
Returns AgResult:
    AG_S_OK
    AG_E_OUT_OF_MEM
    E_CLS_MAX_MP_EXCEEDED
*/
AgResult cls_create_mp( MiniPrgDesc *p_mp, MpId* p_mp_id)
{
    
    MpId n_id = 0;
    AG_U32 i;

    AG_ASSERT(p_mp && p_mp_id);

	/* Shabtay Semaphore Lock*/
	agos_get_bi_semaphore(&xMpSem, AGOS_WAIT_FOR_EVER);
	
    /* find free ID */
    for (i = 0 ; i < nMaxMp ; i++)
    {
        if (!pMpTable[i])
        {
            n_id = i;
            break;
        }
    }
    if(i == nMaxMp)
    {
		/* Shabtay Semaphore UnLock*/
		agos_give_bi_semaphore(&xMpSem);
        return AG_E_CLS_MAX_MP_EXCEEDED;
    }

    /* update the management structure */
    pMpTable[n_id] = p_mp;

    /* initialize the MP    */

    /* Initialize list of leaves */
    list_init(&p_mp->x_leaves);

    /* Initialize list of linked groups */
    list_init(&p_mp->x_linked_grps);

    /* set the out parameter. */
    *p_mp_id = n_id;

	/* Shabtay Semaphore UnLock*/
	agos_give_bi_semaphore(&xMpSem);
	
    return AG_S_OK;
}

/*
Name: 
    cls_get_mpdesc
Description:

Parameters:

Returns AgResult:
    AG_S_OK
    E_CLS_BAD_MP_ID

*/
AgResult cls_get_mpdesc( MpId x_mp_id,  MiniPrgDesc **p_mp)
{
  
    AG_ASSERT(p_mp);

    /* Shabtay Semaphore Lock*/
	agos_get_bi_semaphore(&xMpSem, AGOS_WAIT_FOR_EVER);

	
    if(x_mp_id < nMaxMp)
    {
        if(pMpTable[x_mp_id])
        {
            *p_mp = pMpTable[x_mp_id];

			/* Shabtay Semaphore UnLock*/
			agos_give_bi_semaphore(&xMpSem);
            
			return AG_S_OK;
        }
    }

	/* Shabtay Semaphore UnLock*/
	agos_give_bi_semaphore(&xMpSem);
	
    return AG_E_CLS_BAD_MP_ID;
}

AgResult cls_get_mpdesc_nts( MpId x_mp_id,  MiniPrgDesc **p_mp)
{
    AG_ASSERT(p_mp);
    if(x_mp_id < nMaxMp)
    {
        if(pMpTable[x_mp_id])
        {
            *p_mp = pMpTable[x_mp_id];
            return AG_S_OK;
        }
    }
    return AG_E_CLS_BAD_MP_ID;
}

AgResult cls_get_leaf_grp( LeafGrpId x_grp_id,  LeafGrp **p_grp)
{
	/* Shabtay Semaphore Lock*/
	agos_get_bi_semaphore(&xGrpSem, AGOS_WAIT_FOR_EVER);

    if(x_grp_id < nMaxGrp)
    {
        if(pGrpTable[x_grp_id])
        {
            *p_grp = pGrpTable[x_grp_id];

			/* Shabtay Semaphore UnLock*/
			agos_give_bi_semaphore(&xGrpSem);
            
			return AG_S_OK;
        }
    }

	/* Shabtay Semaphore UnLock*/
	agos_give_bi_semaphore(&xGrpSem);

    return AG_E_FAIL;
}

/*
Name: 
    cls_create_leaf_grp
Description:
    Creates a new LeafGrp. associates it with the given id 
    and inserts it into the management structures.
Parameters:
     AG_U32 x_grp_id: The id for new group
     AG_U32 n_cls_prg: The classification program number.
Returns AgResult:
    AG_S_OK
    E_CLS_MAX_GRP_EXCEEDED
    AG_E_OUT_OF_MEM
*/
AgResult cls_create_leaf_grp( LeafGrpId* p_grp_id,  AgClsbMem* p_cls_mem)
{
    AG_U32 n_entry;
    LeafGrp *p_grp;

    /* allocate and initialize the leaf group. */
    
	if (AG_FAILED(alloc_grp(&p_grp)))
	{
		return AG_E_OUT_OF_MEM;
	}

    /* find a free leaf group entry
     an entry where the pGrpTable[n_entry] is NULL is free. */

	/* Shabtay Semaphore Lock*/

	
	/* Shabtay Semaphore Lock*/
	agos_get_bi_semaphore(&xGrpSem, AGOS_WAIT_FOR_EVER);

    for (n_entry = 0 ; n_entry < nMaxGrp ; n_entry++)
    {
        if (!pGrpTable[n_entry])
        {/* free entry */            
            break;
        }
    }
    if (n_entry == nMaxGrp)
    {
		/* Shabtay Semaphore UnLock*/
		agos_give_bi_semaphore(&xGrpSem);
        
		return AG_E_CLS_MAX_GRP_EXCEEDED;
    }

    /* update the management structure */
    pGrpTable[n_entry] = p_grp;

     /* Initialize list of leaves */
     list_init(&p_grp->x_leaves);

     /* Initialize to not linked */
     p_grp->b_linked = AG_FALSE;

     p_grp->p_linked_mp = NULL;

     p_grp->p_mem_handle = p_cls_mem;

/*     p_grp->x_grp_id = x_grp_id; */

     /* one reference for the table pointer */
     p_grp->n_ref_count = 1;

     /* set the out parameter. */
     *p_grp_id = n_entry;

	/* Shabtay Semaphore UnLock*/
	agos_give_bi_semaphore(&xGrpSem);
	
     return AG_S_OK;
}


/*
Name: 
    cls_add_mp_leaf
Description:

Parameters:
      MiniPrgDesc* p_mp:
      AG_U32 x_grp_id:
      AG_U32 n_record:
      AG_U32 n_pathtag:
      TableType e_table:
      AG_U16 n_data: if e_table==CLS_DEF_TBL then data is ignored.
        if e_table==CLS_PRI_TBL the data is required.
        range is 0..0xFF or 0..0xFFFF (depends on the classification mode)
Returns AgResult:
    AG_S_OK
    E_CLS_BAD_GRP_ID
    E_CLS_GRP_LINKED
    E_CLS_UNMATCHED_PRGS
    AG_E_FAIL
*/

AgResult cls_add_mp_leaf(
              MiniPrgDesc* p_mp,  LeafGrp   *p_grp,
              AG_U32 n_record,  AG_U32 n_pathtag,
              TableType e_table,  AG_U16 n_data
			 
             )
{

    Leaf* p_leaf ;
    LeafPtrEl* p_grp_list_element;
    LeafPtrEl* p_mp_list_element;
    AgClsbMem*  p_mem_handle = p_grp->p_mem_handle;

#ifndef NO_VALIDATION
    if(p_grp->b_linked && !p_grp->p_linked_mp)
    {
        /*group is linked but the MP it is linked to was deleted.
        No more leaves can be added to this group, since the link
        cannot be established. */
        return AG_E_CLS_INVALID_OPER;
    }
#endif

    /* Allocate and initialize the Leaf. */
	if (AG_FAILED(alloc_leaf(&p_leaf)))
	{
		return AG_E_OUT_OF_MEM;
	}

    if (e_table==CLS_DEF_TBL)
    {   /*n_data must be zero so the in the cls_get_leaf_tag_X and cls_set_leaf_tag_X
        functions the entry computation will work correctly.*/
        n_data = 0;
    }

    p_leaf->p_grp = p_grp;
    p_leaf->p_mp = p_mp;
    p_leaf->n_rec = n_record;
    p_leaf->n_pt = n_pathtag;
    p_leaf->e_table = e_table;
    p_leaf->n_data = n_data;


    /* currently the default behvior is to link the added leaf if the */
    /* group was already linked. */
    /* This may change when adding group attributes which can enable/disable */
    /* this property. */
    if(p_grp->b_linked)
    {
        #ifndef NO_VALIDATION
        if(!clsb_is_valid_to_link(p_leaf, p_mem_handle, p_grp->n_opcode))
        {
		    free_leaf(p_leaf);
            return AG_E_FAIL;
        }
        #endif        
#if 0
        if(p_grp->b_enable_unlink)
        {
#ifndef CLS_8_BIT
            if (p_mem_handle->n_cls_mode == AG_CLS_BIT_MODE_16)
			{
                cls_get_leaf_tag_16(p_leaf,&p_leaf->n_cls_tag, p_mem_handle);
            }
			else
#endif /*ndef CLS_8_BIT */
			{
#ifndef CLS_16_BIT
                cls_get_leaf_tag_8(p_leaf,&p_leaf->n_cls_tag, p_mem_handle);
#endif /*ndef CLS_16_BIT */
			}
        }
#endif
#ifndef CLS_8_BIT
		if (p_mem_handle->n_cls_mode == AG_CLS_BIT_MODE_16)
		{
			cls_set_link_16(p_leaf,&p_grp->p_linked_mp->x_stdprg,p_grp->n_opcode);
		}
		else
#endif /* ndef CLS_8_BIT */
        {
#ifndef CLS_16_BIT
			cls_set_link_8(p_leaf,&p_grp->p_linked_mp->x_stdprg,p_grp->n_opcode);
#endif /* ndef CLS_16_BIT */
        }
        
        if(CLSB_IS_STATIC_MP(p_grp->p_linked_mp) && !p_grp->b_enable_unlink)
        { /* if link is to static program with unlink disabled then the leaf structure
            can be freed. The MP cannot move so the leaf structure is not needed and 
            one cannot unlink the group so again the leaf is not needed. */
		    free_leaf(p_leaf);
            return AG_S_OK;
        }
    }

    /*If there is no link or the linked MP is not static or the unlink is enabled then we must
    maintain the leaf sturcture with its pointers. */
    
    /* allocate Leaf list element and add it to the MiniPrgDesc list of leaves. */
	if (AG_FAILED(alloc_leaf_ptr(&p_mp_list_element)))
	{
        free_leaf(p_leaf);
		return AG_E_OUT_OF_MEM;
	}
    
    /* allocate Leaf list element and add it to the LeafGrp list of leaves. */
	if (AG_FAILED(alloc_leaf_ptr(&p_grp_list_element)))
	{        
        free_leaf(p_leaf);
        free_leaf_ptr(p_mp_list_element);
		return AG_E_OUT_OF_MEM;
	}

    p_grp_list_element->p_leaf = p_leaf;
    list_insert_head(&p_grp->x_leaves , &p_grp_list_element->x_element);

	p_mp_list_element->p_leaf = p_leaf;
    list_insert_head(&p_mp->x_leaves , &p_mp_list_element->x_element);
    
    return AG_S_OK;
}


/*
Name: 
    cls_del_mp_leaf
Description:

Parameters:
      MiniPrgDesc* p_mp:
      AG_U32 x_grp_id:
      AG_U32 n_record:
      AG_U32 n_pathtag:
      TableType e_table:
      AG_U16 n_data: if e_table==CLS_DEF_TBL then data is ignored (use default).
        if e_table==CLS_PRI_TBL the data is required.
        range is 0..0xFF or 0..0xFFFF (depends on the classification mode)
Returns AgResult:
    AG_S_OK
    E_CLS_BAD_GRP_ID
    E_CLS_LEAF_NOT_FOUND
*/

AgResult cls_del_mp_leaf(
              MiniPrgDesc* p_mp,  LeafGrp   *p_grp,
              AG_U32 n_record,  AG_U32 n_pathtag,
              TableType e_table,  AG_U16 n_data
             )
{
	LeafPtrEl *p_grp_element=NULL;
	LeafPtrEl *p_mp_element=NULL;


    if (e_table==CLS_DEF_TBL)
    {
        n_data = 0;
    }

    /* find leaf in group list */
    if(AG_SUCCEEDED(find_leaf(n_record,n_pathtag,e_table,n_data,
                           &p_grp->x_leaves,&p_grp_element)))
    {
        /* find the leaf pointed by the grp element in the MP list */
        if(AG_SUCCEEDED(find_this_leaf(p_grp_element->p_leaf,
                                    &p_mp->x_leaves,
                                    &p_mp_element)))
        {
            /* found the element in both lists so remove */
            /* it from both lists */
            Leaf *p_leaf = p_mp_element->p_leaf;

            /* remove the leaf pointer from MP list */
            list_unlink(&p_mp->x_leaves,&p_mp_element->x_element);

			/* Free memory partition */
			free_leaf_ptr(p_mp_element);

            /* remove the leaf pointer from grp list */
            list_unlink(&p_grp->x_leaves,&p_grp_element->x_element);

			/* Free memory partition */
			free_leaf_ptr(p_grp_element);

            /* free the leaf itself */
			/* Free memory partition */
			free_leaf(p_leaf);

            return AG_S_OK;

        } /* if find this leaf AG_SUCCEEDED */

    } /* if find leaf AG_SUCCEEDED */
    else
    { /*leaf is not found in the group.  This is valid only if the group is 
      linked to a static MP with unlink disable.*/
        if(p_grp->b_linked)
        {
            if(CLSB_IS_STATIC_MP(p_grp->p_linked_mp) && !p_grp->b_enable_unlink)
            {
                return AG_S_OK;
            }
        }
    }

    /* leaf not found in one or both lists. */
    AGOS_SWERR(AGOS_SWERR_SERIOUS,"[CLSB mph] delete path inconsistancy progblem",0,0,0,0,0);
    return AG_E_CLS_LEAF_NOT_FOUND;


}


/*
Name: 
    cls_update_mp
Description:

Parameters:

Returns AgResult:
    E_CLS_FATAL
    E_CLS_BAD_MP_ID
    AG_E_FAIL:
*/
AgResult cls_update_mp(
              MiniPrgDesc *p_mp,
              MpNodeUpdate *p_updates
             )
{
    
    AG_ASSERT(p_updates);
        
    if (p_updates->b_root)
    {
#ifndef NO_VALIDATION
        if(CLSB_IS_STATIC_MP(p_mp))
        {/*the root of a static mini program should never move.*/
            return AG_E_CLS_FATAL;
        }
#endif /*not NO_VALIDATION*/
        
       #ifndef BCM_CES_SDK /*Needed only for CPU classification*/
        /* 
        check if this MP is active (for each of the interfaces)
        If it is, set the new record and initial path tag in the relevant
        registers. 
        */
        if(CLSB_IS_ACTIVE_MP_LAN(p_mp))
        {
			/*set the initial record*/
			ag_set_range_value(AG_REG_L_CLS_CFG0, p_updates->n_new_rec, 0, 13);
			/*set the initial path tag*/
			*AG_REG_L_CLS_IPT = p_updates->n_new_pt;
        }
        if(CLSB_IS_ACTIVE_MP_WAN1(p_mp))
        {
			/*set the initial record*/
			ag_set_range_value(AG_REG_W1_CLS_CFG0, p_updates->n_new_rec, 0, 13);
			/*set the initial path tag*/
			*AG_REG_W1_CLS_IPT = p_updates->n_new_pt;
        }
        if(CLSB_IS_ACTIVE_MP_WAN2(p_mp))
        {
			/*set the initial record*/
			ag_set_range_value(AG_REG_W2_CLS_CFG0, p_updates->n_new_rec, 0, 13);
			/*set the initial path tag*/
			*AG_REG_W2_CLS_IPT = p_updates->n_new_pt;
        }
      #endif /*BCM_CES_SDK*/
        /* 
        Handle the case when there are links to the root of this MP.
        Need to update all the linked leaves. 
        */
        if(AG_FAILED(handle_root_update(p_mp/*,p_updates*/)))
        {
            return AG_E_CLS_FATAL;
        }
    } /* end of if(b_root) */

    /* 
    Need to check for leaves anyway since if the leaves are linked
    then they are not identified by the engine as leaves but they still
    exist in the mp handler as leaves. 
    */
    if(AG_FAILED(handle_leaves_update(p_mp,p_updates)))
    {
        return AG_E_CLS_FATAL;
    }
        
    
    return AG_S_OK;
}


/*
Name: 
    cls_link_leaf_grp
Description:

Parameters:
	the n_mask is currently not used. The 16 bit implementation uses
	the default mp mask. If need to change it, update the engine implementation.
Returns AgResult:
    AG_S_OK
    AG_E_FAIL: unexpected errors
    E_CLS_GRP_LINKED
    E_CLS_BAD_GRP_ID
    E_CLS_BAD_MP_ID
    E_CLS_UNMATCHED_PRGS
    AG_E_OUT_OF_MEM : malloc AG_FAILED
*/
AgResult cls_link_leaf_grp(
              LeafGrp *p_grp,  MiniPrgDesc* p_to_mp, 
              AG_U8 n_opcode,  AG_U8 n_mask , AG_BOOL b_enable_unlink
             )
{
    ListElement *p_element=NULL;
    LeafGrpPtrEl* p_grp_element;
    AgClsbMem*  p_mem_handle = p_grp->p_mem_handle;
    
/***********************
   Validity checking    
************************/

    if(p_grp->b_linked)
    {
        if(p_grp->p_linked_mp == p_to_mp)
        { /* the group is already linked to this MP. */
			/*MBOPC - 17/10/2000 - Verify if opcodes match */
			if(p_grp->n_opcode != n_opcode) 
            {
                return AG_E_CLS_INVALID_OPC;
            }
            else
            {
                return AG_S_OK;
            }
        }
        else
        { /* Error: the group is already linked to another MP. */
            return AG_E_CLS_GRP_LINKED;
        }
    }
    /* The group is not linked yet */

#ifndef NO_VALIDATION
    /* verify that each of the leaves in the group may be linked to the MP. */
	p_element=list_head(&p_grp->x_leaves);
	while(p_element)
	{   /* for each leaf */
        Leaf *p_leaf = ((LeafPtrEl*)p_element)->p_leaf;
        if(!clsb_is_valid_to_link(p_leaf, p_mem_handle, p_grp->n_opcode))
        {
            return AG_E_CLS_INVALID_OPER;
        }

		p_element = list_next(&p_grp->x_leaves,p_element);
    } /* end of while */
#endif

/***********************
   Validation DONE    
************************/
    /* allocate a group pointer to be added to the MP list of links */
	if (AG_FAILED(alloc_grp_ptr(&p_grp_element)))
	{
		return AG_E_OUT_OF_MEM;
	}

    /* link each of the leaves in the group to the MP. */
/* N.A.=>  if b_enable_unlink is AG_TRUE then save the current tag in  */
/*     each leaf */
	p_element=list_head(&p_grp->x_leaves);
	while(p_element)
	{   /* for each leaf */
        Leaf *p_leaf = ((LeafPtrEl*)p_element)->p_leaf;
#if 0
        if(b_enable_unlink)
        {
#ifndef CLS_8_BIT
			if(p_mem_handle->n_cls_mode == AG_CLS_BIT_MODE_16)
			{
				cls_get_leaf_tag_16(p_leaf,&p_leaf->n_cls_tag, p_mem_handle);
			}
			else
#endif /*ndef CLS_8_BIT*/
			{
#ifndef CLS_16_BIT
				cls_get_leaf_tag_8(p_leaf,&p_leaf->n_cls_tag, p_mem_handle);
#endif /*ndef CLS_16_BIT  */
			}

        }
#endif
#ifndef CLS_8_BIT
		if (p_mem_handle->n_cls_mode == AG_CLS_BIT_MODE_16)
		{
			cls_set_link_16(p_leaf,&p_to_mp->x_stdprg,n_opcode);
		}
		else
#endif /*ndef CLS_8_BIT*/
		{
#ifndef CLS_16_BIT
			cls_set_link_8(p_leaf,&p_to_mp->x_stdprg,n_opcode);
#endif /*ndef CLS_16_BIT */
		}

		p_element = list_next(&p_grp->x_leaves,p_element);

        if(CLSB_IS_STATIC_MP(p_to_mp) && !b_enable_unlink)
        { /* if link is to static program with unlink disabled then the leaf structure
            can be freed. The MP cannot move so the leaf structure is not needed and 
            one cannot unlink the group so again the leaf is not needed. */
            cls_del_mp_leaf(p_leaf->p_mp,p_grp,p_leaf->n_rec,p_leaf->n_pt,
                p_leaf->e_table,p_leaf->n_data);
        }
    } /* end of while */

    /* update group as linked to the specified MP */
    p_grp->b_linked = AG_TRUE;
    p_grp->p_linked_mp = p_to_mp;
    p_grp->n_opcode = n_opcode;
    p_grp->b_enable_unlink = b_enable_unlink ? AG_TRUE : AG_FALSE; /* needed because group variable is AG_U8 */
    p_grp->n_ref_count++; /* add ref for the pointer in p_mp->x_linked_grps */
    
    /* add group to MP's linked list */
    p_grp_element->p_grp = p_grp;
    list_insert_head(&p_to_mp->x_linked_grps,&p_grp_element->x_element);
    
	n_mask = 0; /* DUMMY operation to avoid warning */
	return AG_S_OK;


}

AgResult cls_unlink_leaf_grp( LeafGrp *p_grp, AG_U32 n_new_tag)
{
    List *p_grp_list;
    ListElement *p_element=NULL;
    LeafGrpPtrEl* p_grp_element = NULL;
	AgResult x_res;
    AgClsbMem*  p_mem_handle = p_grp->p_mem_handle;


/***********************
   Validity checking    
************************/
#ifndef NO_VALIDATION
    if(p_grp->b_linked)
    {
        if(!p_grp->b_enable_unlink || 
            p_grp->p_linked_mp == NULL)
        { /*not valid to unlink when unlink is disabled or if the MP that the group
            is linked to was deleted.*/
            return AG_E_CLS_INVALID_OPER;
        }
        /* at this point we know that the group is valid and linked.
        and that the MP it is linked to is also valid.
        This means that the reference count on it must be 2.*/
        AG_ASSERT(p_grp->n_ref_count == 2);
    }
    else
    {   /* The group is not linked  */
        return AG_E_CLS_GRP_NOT_LINKED;
    }
#endif /* ndef NO_VALIDATION */

/***********************
   Validation DONE
************************/

    /* return the original tags to all the leaves in the group */
	p_element=list_head(&p_grp->x_leaves);
    while(p_element)
    { /* for each leaf in the group */

        Leaf *p_leaf = ((LeafPtrEl*)p_element)->p_leaf;

#ifndef CLS_8_BIT
		if (p_mem_handle->n_cls_mode == AG_CLS_BIT_MODE_16)
		{
			cls_set_leaf_tag_16(p_leaf,n_new_tag, p_mem_handle);
		}
		else
#endif /*ndef CLS_8_BIT*/
		{
#ifndef CLS_16_BIT
			cls_set_leaf_tag_8(p_leaf,n_new_tag, p_mem_handle);
#endif /*ndef CLS_16_BIT */
		}

        p_element = list_next(&p_grp->x_leaves,p_element);
    }
    /* finished setting all the leaves to their original tags */

    /* remove the group pointer from the MP linked groups list */
    p_grp_list = &p_grp->p_linked_mp->x_linked_grps;
    x_res = find_this_grp(p_grp,p_grp_list,&p_grp_element);
    AG_ASSERT(AG_SUCCEEDED(x_res));
    list_unlink(p_grp_list,&p_grp_element->x_element);	        
	free_grp_ptr(p_grp_element);

    /* decrease the reference count on the group because it was removed from
    the list of the MP it was linked to*/
    release_leaf_grp(p_grp);
    
    /* update group as unlinked */
    p_grp->b_linked = AG_FALSE;
    p_grp->p_linked_mp = NULL; 

    return AG_S_OK;
}


/*
Name: 
    cls_del_mp
Description:

Parameters:
    * The b_validate parameter is set to FALSE by the clsb_mph_reset function which
      doesn't care about validation.
    * The b_thread_safe is used to decide which functions (with _nts or without) since the 
      semaphore cannot be taken twice. The "_nts" function does not try to take the semaphore.
Returns AgResult:
    AG_S_OK
    E_CLS_BAD_MP_ID
    E_CLS_FATAL:   inconsistancy error
*/
AgResult cls_del_mp( MpId x_mp_id,MiniPrgDesc** p_mpdesc, AG_BOOL b_validate, 
                    AG_BOOL b_thread_safe)
{
    MiniPrgDesc *p_mp;
    AgResult n_res;
/***********************
   Validity checking    
************************/
    /* check if the ID is valid. if it does then remove it from
     the table and free the table entry. */
    if(b_thread_safe)
    	n_res = cls_get_mpdesc(x_mp_id,&p_mp);
    else 
        n_res = cls_get_mpdesc_nts(x_mp_id,&p_mp);

#ifndef NO_VALIDATION
    if(AG_FAILED(n_res))
    { 
        return AG_E_CLS_BAD_MP_ID;
    }

    AG_ASSERT(p_mpdesc);

    if(b_validate)
    {
        /* 
        if the MP is not active, then if it has leaves then it is logical 
        to delete it only if there are links to it.
        If there are no links to it and it is not active, then it means that there are
        few paths which cannot be used. Nothing goes through them and they cannot be 
        removed. This occupies classification program memory for no reason.
        It can be looked at as a memory leak.
        */

        if(!CLSB_IS_ACTIVE_MP(p_mp))
        {
            if(!list_is_empty(&p_mp->x_leaves))
            {   
                if(list_is_empty(&p_mp->x_linked_grps))
                {   
                    AGOS_SWERR(AGOS_SWERR_WARNING,"[mph] suspicious mp deletion",x_mp_id,0,0,0,0);
                }
            }
        }
    }
#endif /* ndef NO_VALIDATION */
/***********************
   Validation DONE
************************/
    
    if(b_thread_safe)
        remove_mpdesc(x_mp_id);
    else
        remove_mpdesc_nts(x_mp_id);
    
    /* free all the leaf pointers to the leaves of the MP */
    while(!list_is_empty(&p_mp->x_leaves))
    { 
        LeafPtrEl *p_Leaf_el = (LeafPtrEl*)list_remove_head(&p_mp->x_leaves);
        
        /* need to invalidate the MP pointer referenced in the Leaf
         because this id might be allocated later to another craeted MP */
        p_Leaf_el->p_leaf->p_mp = NULL;
        
		free_leaf_ptr(p_Leaf_el);
    }

    /* release all the linked groups */
    while(!list_is_empty(&p_mp->x_linked_grps))
    { 
        LeafGrpPtrEl* p_grp_el = (LeafGrpPtrEl*)list_remove_head(&p_mp->x_linked_grps);
        
        /* need to invalidate the MP ID referenced in the group
         because this id might be allocated later to another created MP */
        p_grp_el->p_grp->p_linked_mp = NULL;
        
        /* if the group was deleted before then the release function
         below will delete the actual group leaves also.
         if the group was not deleted it will just decrease the ref count. */
        release_leaf_grp(p_grp_el->p_grp);
        
		free_grp_ptr(p_grp_el);
    }

    *p_mpdesc = p_mp; 

    /*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    the MP itself is freed by the caller 
    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/

    return AG_S_OK;
}

AgResult cls_del_grp( LeafGrpId x_grp,AG_BOOL b_thread_safe)
{
    LeafGrp* p_grp;
    
    if(b_thread_safe)
        p_grp = remove_leaf_grp(x_grp);
    else
        p_grp = remove_leaf_grp_nts(x_grp);
    
    /*no validation is done since this function is called only after checking that 
    the id is valid*/
    AG_ASSERT(p_grp);

    /* 
     need to invalidate the group id in the list of leaves only
     if the n_ref_count > 1. If the n_ref_count==1 then the release
     operation will free all the leaves anyhow so no need to walk 
     through the list twice.
     Also, only if ref count > 1, need to invalidate the group ID in the group structure itself
    */
    if(p_grp->n_ref_count > 1)
    {
	    LeafPtrEl *p_leaf_el;

		p_leaf_el=(LeafPtrEl*)list_head(&p_grp->x_leaves);
        if(p_leaf_el)
        { 
		    do
		    {   /* for each LeafPtrEl, invalidate the group pointer member */
                /* this group id may be later allocated for another group. */
                p_leaf_el->p_leaf->p_grp = NULL;
                p_leaf_el = (LeafPtrEl*)list_next(&p_grp->x_leaves,
                                                  &p_leaf_el->x_element);
            } while(p_leaf_el);
        }
/*        p_grp->x_grp_id = INVALID_GRP_ID; */
    }

    return (release_leaf_grp(p_grp));
}

AgResult clsb_set_active_mp(AgComIntf n_intf, MiniPrgDesc* p_mpdesc)
{
 #ifndef BCM_CES_SDK
    switch (n_intf) 
    {
	    case AG_LAN :
            p_mpdesc->n_properties |= AG_CLSB_ACTIVE_MP_LAN;
            return AG_S_OK;
        case AG_WAN1 :
            p_mpdesc->n_properties |= AG_CLSB_ACTIVE_MP_WAN1;
            return AG_S_OK;
        case AG_WAN2 :
            p_mpdesc->n_properties |= AG_CLSB_ACTIVE_MP_WAN2;
            return AG_S_OK;
    }
    return AG_E_BAD_PARAMS;
  #else
    return AG_S_OK;
  #endif
  
}
 

AgResult clsb_mph_reset(AgClsbMem*  p_mem_handle)
{
    /*the function continues in its reset even if errors occur, in order to reset as
    much as possible. If any errors occured, then it returns AG_E_FAIL.*/
    AgResult n_res = AG_S_OK;
    MiniPrgDesc* p_mp;
    AG_U32 i;

    /*delete all the MPs. It also frees the list of leaf pointers and the list 
    of linked gropus but it does not free the actual leaves and groups memory.*/

	/* Shabtay Semaphore Lock*/
	agos_get_bi_semaphore(&xMpSem, AGOS_WAIT_FOR_EVER);
	
    for (i = 0 ; i < nMaxMp ; i++)
    {
        if (pMpTable[i])
        {
            if(pMpTable[i]->x_stdprg.p_mem_handle == p_mem_handle)
            {
                n_res |= cls_del_mp( i,&p_mp, AG_FALSE, AG_FALSE);
                /*the caller to cls_del_mp is responsible to free the mp memory.*/
	            free_mp(p_mp);
            }
        }
    }

    /*delete all the groups. At this stage it also frees the leaves and the list 
    of leaf pointers */


	/* Shabtay Semaphore Lock*/
	agos_get_bi_semaphore(&xGrpSem, AGOS_WAIT_FOR_EVER);
	
    for (i = 0 ; i < nMaxGrp ; i++)
    {
        if (pGrpTable[i])
        {
            if(pGrpTable[i]->p_mem_handle == p_mem_handle)
            {
                /*all the memory is freed within the function.*/
                n_res |= cls_del_grp(i,AG_FALSE);
            }
        }
    }

    if(AG_FAILED(n_res))
    {
		/* Shabtay Semaphore UnLock*/
		agos_give_bi_semaphore(&xMpSem);
		/* Shabtay Semaphore UnLock*/
		agos_give_bi_semaphore(&xGrpSem);
        return AG_E_FAIL;
    }

	/* Shabtay Semaphore UnLock*/
	agos_give_bi_semaphore(&xMpSem);
	/* Shabtay Semaphore UnLock*/
	agos_give_bi_semaphore(&xGrpSem);
	
    return AG_S_OK;
}


/*********************************************************
**          STATIC (PRIVATE) FUNCTION IMPLEMENTATION    **
**********************************************************/
AgResult mph_configure(AG_U32 n_max_mp, AG_U32 n_max_grp)
{
	AG_U8 a_name1[9] = "mph_sem1";
	AG_U8 a_name2[9] = "mph_sem2";
	AG_U32 n_zero = 0;

    nMaxMp = n_max_mp;
    nMaxGrp = n_max_grp;


	if (AG_FAILED(agos_malloc((nMaxMp*sizeof(MiniPrgDesc*)), (void*)&pMpTable)))
	{
		AGOS_SWERR(AGOS_SWERR_WARNING,"MPH configuration failed",0,0,0,0,0);
        return AG_E_OUT_OF_MEM;
	}   

	AG_MEMSET_WORD(pMpTable, n_zero, (nMaxMp*sizeof(MiniPrgDesc*)));

    if(AG_FAILED(agos_create_bi_semaphore(&xMpSem, (AG_S8*)a_name1)))
    {
        agos_free((void*)pMpTable);
        AGOS_SWERR(AGOS_SWERR_WARNING,"CLSB: mph1 semaphore creation failed ",0,0,0,0,0);
        return AG_E_FAIL;
    }

	if (AG_FAILED(agos_malloc(nMaxGrp*sizeof(LeafGrp*), (void*)&pGrpTable)))
	{
        agos_free((void*)pMpTable);
		AGOS_SWERR(AGOS_SWERR_WARNING,"MPH configuration failed",0,0,0,0,0);
        return AG_E_OUT_OF_MEM;
	}

	AG_MEMSET_WORD(pGrpTable, n_zero, nMaxGrp*sizeof(LeafGrp*));

    if(AG_FAILED(agos_create_bi_semaphore(&xGrpSem, (AG_S8*)a_name2)))
    {
        agos_free((void*)pMpTable);
        agos_free((void*)pGrpTable);
        AGOS_SWERR(AGOS_SWERR_WARNING,"CLSB: mph2 semaphore creation failed ",0,0,0,0,0);
        return AG_E_FAIL;
    }

    return AG_S_OK;
}


MiniPrgDesc* remove_mpdesc( MpId x_id)
{
	MiniPrgDesc* p_mp;

	
	/* Shabtay Semaphore Lock*/
	agos_get_bi_semaphore(&xMpSem, AGOS_WAIT_FOR_EVER);
	
    p_mp = pMpTable[x_id];
    pMpTable[x_id] = NULL;

	
	/* Shabtay Semaphore UnLock*/
	agos_give_bi_semaphore(&xMpSem);
    return p_mp;
}

MiniPrgDesc* remove_mpdesc_nts( MpId x_id)
{
	MiniPrgDesc* p_mp;
    p_mp = pMpTable[x_id];
    pMpTable[x_id] = NULL;
    return p_mp;
}


LeafGrp* remove_leaf_grp(LeafGrpId x_grp_id)
{
    LeafGrp* p_grp;

	/* Shabtay Semaphore Lock*/
	agos_get_bi_semaphore(&xGrpSem, AGOS_WAIT_FOR_EVER);
    
    p_grp = pGrpTable[x_grp_id];
    pGrpTable[x_grp_id] = NULL;

	/* Shabtay Semaphore UnLock*/
	agos_give_bi_semaphore(&xGrpSem);
    return p_grp;
}

LeafGrp* remove_leaf_grp_nts(LeafGrpId x_grp_id)
{
    LeafGrp* p_grp = pGrpTable[x_grp_id];
    pGrpTable[x_grp_id] = NULL;
    return p_grp;
}


/*
pass on the list of linked leaves and update the actual links in the 
classification program.
*/
AgResult handle_root_update(MiniPrgDesc *p_mp/*,MpNodeUpdate *p_update*/)
{
	LeafGrpPtrEl *p_grp_el=NULL;
    LeafPtrEl    *p_leaf_el = NULL;

    if(list_is_empty(&p_mp->x_linked_grps))
    {   /* no handling is needed because there are no links to this MP. */
        return AG_S_OK;
    }
    
    /* validate that the root returned by p_mp->p_stdprg access functions */
    /* is the same as the new record and new pathtag in the p_update. */
    /*??? */

	p_grp_el=(LeafGrpPtrEl*)list_head(&p_mp->x_linked_grps);
    if(p_grp_el)
    { 
		do
		{   /* for each group update it list of leaves */
            LeafGrp *p_grp = p_grp_el->p_grp;

			p_leaf_el=(LeafPtrEl*)list_head(&p_grp->x_leaves);
            if(p_leaf_el)
            { 
		        do
		        {   /* for each leaf */
                    /* validate that the next record and next pathtag */
                    /* of the p_leaf ( as written in the program) */
                    /* are the same as the old_rec and old_pt in the p_update */
                    /*??? */

#ifndef CLS_8_BIT
					if (p_mp->x_stdprg.p_mem_handle->n_cls_mode == AG_CLS_BIT_MODE_16)
					{
						cls_set_link_16(p_leaf_el->p_leaf,&p_mp->x_stdprg,OPC_KEEPOPC);
					}
					else
#endif /*ndef CLS_8_BIT*/
					{
#ifndef CLS_16_BIT
						cls_set_link_8(p_leaf_el->p_leaf,&p_mp->x_stdprg,OPC_KEEPOPC);
#endif /*ndef CLS_16_BIT */
					}
                    
                    p_leaf_el = (LeafPtrEl*)list_next(&p_grp->x_leaves,
                                                      &p_leaf_el->x_element);
                } while(p_leaf_el);
            }

            p_grp_el = (LeafGrpPtrEl*)list_next(&p_mp->x_linked_grps,
                                                &p_grp_el->x_element);
        } while(p_grp_el);
    }
    else
    {
        return AG_E_FAIL;
    }

    return AG_S_OK;
}

/*
scan all the leaves for those matching the old Record and PT.
Change the Leaf to the new Record and PT
*/
AgResult handle_leaves_update(MiniPrgDesc *p_mp,MpNodeUpdate *p_update)
{
	ListElement *p_element=NULL;

	p_element=list_head(&p_mp->x_leaves);
    if(p_element)
    { 
		do
		{   /* for each leaf check if it matches the old rec and old pt */
            /* if it does, update it to the new ones. */
            /* there may be more than one leaf in this node */

			Leaf *p_leaf = ((LeafPtrEl*)p_element)->p_leaf;
			if(p_leaf->n_rec == p_update->n_old_rec &&
               p_leaf->n_pt == p_update->n_old_pt)
            {
                p_leaf->n_rec = p_update->n_new_rec;
                p_leaf->n_pt = p_update->n_new_pt;
            }
            p_element = list_next(&p_mp->x_leaves,p_element);
        } while(p_element);
    }

   /* since the API supports AG_CLSB_IGNORE_GRP, leaves may be added to the program
    without being monitored by the MP handler. This means that it is possible that
    even if p_update->b_leaves is true, leaves may not exist in the mp handler.*/

    return AG_S_OK;
}

/* find a leaf with the specified properties */
AgResult find_leaf( AG_U32 n_record,  AG_U32 n_pathtag,
                    TableType e_table,  AG_U16 n_data,
                    List* p_list,  LeafPtrEl **p_leaf_element)
{
	ListElement *p_element=NULL;

	p_element=list_head(p_list);
    if(p_element)
    { 
		do
		{   /* for each leaf, check if it matches the given properties.  */
            /* Return the element found */
            /* only one such leaf can exist. */
			Leaf *p_leaf = ((LeafPtrEl*)p_element)->p_leaf;
			if(p_leaf->n_rec == n_record &&
               p_leaf->n_pt == n_pathtag &&
               p_leaf->n_data == n_data &&
               p_leaf->e_table == e_table)
            {
                *p_leaf_element = (LeafPtrEl*)p_element;
                return AG_S_OK;
            }
            p_element = list_next(p_list,p_element);
        } while(p_element);
    }

    return AG_E_CLS_LEAF_NOT_FOUND;
}

/* find a specific leaf pointer (p_this_leaf) */
AgResult find_this_leaf( Leaf *p_this_leaf,  List* p_list, LeafPtrEl **p_leaf_element)
{
	LeafPtrEl *p_leaf_el;

	p_leaf_el=(LeafPtrEl *)list_head(p_list);
    if(p_leaf_el)
    { 
		do
		{   /* for each LeafPtrEl, check if its pointer points to the */
            /* given leaf (p_this_leaf). Return the element found. */
            /* only one such leaf can exist. */
			if (p_this_leaf == p_leaf_el->p_leaf)
            {
                *p_leaf_element = p_leaf_el;
                return AG_S_OK;
            }
            p_leaf_el = (LeafPtrEl *)list_next(p_list,&p_leaf_el->x_element);
        } while(p_leaf_el);
    }

    return AG_E_CLS_LEAF_NOT_FOUND;
}

/**/
AgResult find_this_grp( LeafGrp* p_this_grp,
                   List* p_list,
                   LeafGrpPtrEl **p_grp_element)
{
	LeafGrpPtrEl *p_grp_el;

	p_grp_el=(LeafGrpPtrEl*)list_head(p_list);
    if(p_grp_el)
    { 
		do
		{   /* for each LeafGrpPtrEl, check if its pointer points to the */
            /* given group (p_this_grp). Return the element found. */
            /* only one such group can exist. */
			if (p_this_grp == p_grp_el->p_grp)
            {
                *p_grp_element = p_grp_el;
                return AG_S_OK;
            }
            p_grp_el = (LeafGrpPtrEl*)list_next(p_list,&p_grp_el->x_element);
        } while(p_grp_el);
    }

    return AG_E_FAIL;
}


AgResult release_leaf_grp( LeafGrp* p_grp)
{

    p_grp->n_ref_count--;
    if(p_grp->n_ref_count)
    {
        return AG_S_CLS_RELEASED;
    }
    
    /* group reference dropped to zero ==> need to delete it. */
    /* release all the leaves of the group. If these leaves belong to */
    /* a valid MP then remove them from the MP list */
    while(!list_is_empty(&p_grp->x_leaves))
    { 
        LeafPtrEl *p_leaf_el = (LeafPtrEl*)list_remove_head(&p_grp->x_leaves);
        MiniPrgDesc* p_mp = p_leaf_el->p_leaf->p_mp;
        
        if(p_mp)
        { /* this leaf belongs to a valid MP.  */
          /* need to remove it from the MP list of leaves */
            LeafPtrEl *p_mp_leaf_el;
            /* find the leaf pointed by the leaf element in the MP list */
            if(AG_SUCCEEDED(find_this_leaf(p_leaf_el->p_leaf,
                                        &p_mp->x_leaves,
                                        &p_mp_leaf_el)))
            {
                /* found the element in the MP list. */
                /* remove the leaf pointer from MP list */
                list_unlink(&p_mp->x_leaves,&p_mp_leaf_el->x_element);
				free_leaf_ptr(p_mp_leaf_el);
            }
            else
            { /*  */
                return AG_E_CLS_LEAF_NOT_FOUND;
            }
        } /* leaf belongs to a valid MP */

        /* free the actuall leaf object.*/
		free_leaf(p_leaf_el->p_leaf);
 
        /* free the leaf pointer */
		free_leaf_ptr(p_leaf_el);
    }
   
    /* free the group itself.*/
    free_grp(p_grp);
    
	return AG_S_OK;
}


#if 0
/* scans the list of leaves, looking for a leaf that belongs to  */
/* the given MpId. */
/* returns AG_TRUE if finds one or more. AG_FALSE otherwise. */
AG_BOOL grp_has_members_in_mp( List *p_leaves, MiniPrgDesc* p_mp)
{
	ListElement *p_element=NULL;

	p_element=list_head(p_leaves);
    if(p_element)
    { 
		do
		{   /* for each LeafPtrEl, check if the leaf it points to */
            /* belongs to the specified MP */
            Leaf *p_leaf = ((LeafPtrEl*)p_element)->p_leaf;
			if ( p_leaf->p_mp == p_mp )
            {
                return AG_TRUE;
            }
            p_element = list_next(p_leaves,p_element);
        } while(p_element);
    }

    return AG_FALSE;
}
#endif

#ifdef AG_MNT
void print_leaves(FILE *p_file, List *p_list)
{
    fprintf(p_file,"Leaves:\n");
    fprintf(p_file,"\trec \tpt  \tMpId\tGrpId\tdata\ttable\ttag\n");
    if(list_is_empty(p_list))
    {
        fprintf(p_file,"\tNONE\n");
    }
    else
    {
        ListElement *p_element=list_head(p_list);
		do
		{   /* for each Leaf, print its members */
            Leaf* p_leaf = ((LeafPtrEl*)p_element)->p_leaf;
            fprintf(p_file,"t%4d\t%4d\t%4d\t%4d\t%4d \n",
                p_leaf->n_rec,
                p_leaf->n_pt,
                p_leaf->p_mp, /*this is actually the address and not the ID*/
/*                p_leaf->p_grp->x_grp_id, */
                p_leaf->n_data,
                p_leaf->e_table /*,p_leaf->n_cls_tag */
                );

            p_element = list_next(p_list,p_element);
        } while(p_element);
        fprintf(p_file,"\n");
    } /* NOT list_is_empty() */

}

void print_mp_info(FILE *p_file, MiniPrgDesc* p_mp, MpId x_id)
{
    fprintf(p_file,"MpId = %d ClsPrgNum = %d\n" \
                   "----------------------------\n",
                   x_id,p_mp->x_stdprg.p_mem_handle);  /* Irena MLT PRG 6/5/2001 */
    fprintf(p_file,"Groups linked to this MP:\n\t");
    if(list_is_empty(&p_mp->x_linked_grps))
    {
        fprintf(p_file,"NONE\n");
    }
    else
    {
        ListElement *p_element = list_head(&p_mp->x_linked_grps);
		do
		{   /* for each LeafGrpPtrEl, print its grp pointer */
            fprintf(p_file,"%d  ",
            (AG_U32)((LeafGrpPtrEl*)p_element)->p_grp);

            p_element = list_next(&p_mp->x_linked_grps,p_element);
        } while(p_element);
        fprintf(p_file,"\n");
    } /* NOT list_is_empty(&p_mp->x_linked_grps) */

    /* print list of leaves */
    print_leaves(p_file,&p_mp->x_leaves);
}

void print_grp_info(FILE *p_file, LeafGrp* p_grp, LeafGrpId x_id)
{
    fprintf(p_file,"GrpId = %d ClsPrgNum = %d RefCount = %d\n" \
                   "------------------------------\n",
                   x_id, p_grp->p_mem_handle, p_grp->n_ref_count);
    if(p_grp->b_linked)
    {
        fprintf(p_file,"Linked to MpId %d with opcode %d. unlink enable = %d\n",
                p_grp->p_linked_mp, p_grp->n_opcode,p_grp->b_enable_unlink);
    }
    else
    {
        fprintf(p_file,"Not Linked\n");
    } 

    /* print list of leaves */
    print_leaves(p_file,&p_grp->x_leaves);
}


AgResult mph_dump(const AG_CHAR *p_filename)
{
    FILE *p_file;
    AG_U32 i;
    
    printf("Dumping MPH structures to file: %s\n",p_filename);
    AG_ASSERT(p_filename);
   	p_file = fopen (p_filename,"w");
    if (p_file == NULL)
	{
	    ("Bad file name %s \n",p_filename);
	    return AG_E_FAIL;
	}

    fprintf(p_file,"\n=============\n"  \
                     "MINI PROGRAMS\n" \
                     "=============\n\n");
    
	CAgBiSem x_sem_mp(&xMpSem,CAgBiSem::LOCKED); /* unlocks semaphore in destructor */
    for (i = 0 ; i < nMaxMp ; i++)
    {
		if (pMpTable[i])
        { /* an occupied entry */
            /* print MP information */
            print_mp_info(p_file,pMpTable[i],i);
        }
    } /* for loop on all MPs */

    fprintf(p_file,"\n=============\n"  \
                     "   GROUPS    \n" \
                     "=============\n\n");
    
	CAgBiSem x_sem_grp(&xGrpSem,CAgBiSem::LOCKED); /* unlocks semaphore in destructor */
    for (i = 0 ; i < nMaxGrp ; i++)
    {
        if(pGrpTable[i])
        { /* an occupied entry */

            /* print MP information */
            print_grp_info(p_file,pGrpTable[i],i);

        }
    } /* for loop on all MPs */

    fclose(p_file);
    return AG_S_OK;
}

#endif /* AG_MNT */

AgResult cls_mph_engine_consistence (void)
{
    AgResult n_res = AG_S_OK;
	AG_U32 i = 0;
/*	AG_U32 j = 0; */
/*	AG_U32 n_size = 0; */
    ListElement *p_element=NULL;
	AG_U32 n_entry;
	AG_U32 n_grp_leaves_count = 0;
	AG_U32 n_mp_leaves_count = 0;

	Leaf *p_leaf;

    CLS_PRINTF(("Checking mph and engine consistency\n"));

	/* Shabtay Semaphore Lock*/
	agos_get_bi_semaphore(&xGrpSem, AGOS_WAIT_FOR_EVER);

    for (i = 0 ; i < nMaxGrp ; i++)
    {
		if (pGrpTable[i])
		{
			p_element=list_head(&pGrpTable[i]->x_leaves);

	        if (pGrpTable[i]->b_linked)
			{  /* Linked group */
		    
				if(p_element)
				{	 
					do
					{ /* Check each leaf from the list */

						p_leaf = ((LeafPtrEl*)p_element)->p_leaf;

						/* Get entry from cls prg table, corresponding to leaf data */
						n_entry = CLS_RECORDS_8(pGrpTable[i]->p_mem_handle->p_record_base, ENTRY(p_leaf->n_pt, p_leaf->n_data),p_leaf->e_table,p_leaf->n_rec);

						/* Check if NPT and NR of the leaf are equal to the root of mp , 
						   that leaf linked to it*/
						if ((NPT_8(n_entry) != pGrpTable[i]->p_linked_mp->x_stdprg.x_root.def_pt) 
							|| NR_8(n_entry) != pGrpTable[i]->p_linked_mp->x_stdprg.x_root.def_rec)
						{ /* MPH and Engine are not consistent */
							AGOS_SWERR(AGOS_SWERR_WARNING,"cls_mph_engine_consistence: NPT or NR of leaf does not match to mp root",0,0,0,0,0);
                            n_res = AG_E_FAIL;
						}

						if (p_leaf->p_mp)
						{ 
							LeafPtrEl *p_mp_leaf_el;

							/* increase grp leaves counter */
							n_grp_leaves_count++;

							/* Find this leaf in the mp */
							if (AG_FAILED (find_this_leaf(p_leaf, &(p_leaf->p_mp->x_leaves), &p_mp_leaf_el)))
							{
								AGOS_SWERR(AGOS_SWERR_WARNING,"cls_mph_engine_consistence: Leaf is not found in the MP",0,0,0,0,0);
                                n_res = AG_E_FAIL;
							}
							else 
							{ /* Leaf is found in the MP */
								if (p_leaf != p_mp_leaf_el->p_leaf)
								{
									AGOS_SWERR(AGOS_SWERR_WARNING,"cls_mph_engine_consistence: Leaf is not found in the MP",0,0,0,0,0);
                                    n_res = AG_E_FAIL;
								}
							}
						}

						p_element = list_next(&pGrpTable[i]->x_leaves,p_element);
					} while(p_element);
				}
			}
			else
			{ /* Group is not linked */
				if(p_element)
				{	 
					do
					{ /* Check each leaf from the list */

						p_leaf = ((LeafPtrEl*)p_element)->p_leaf;

						/* Get entry from cls prg table, corresponding to leaf data */
						n_entry = CLS_RECORDS_8(pGrpTable[i]->p_mem_handle->p_record_base, ENTRY(p_leaf->n_pt,p_leaf->n_data),p_leaf->e_table,p_leaf->n_rec);

						/* Check that leaf has tag */
						if (OPCODE_8(n_entry) != AG_CLSB_OPC8_TAG) 
						{ /* MPH and Engine are not consistent */
							AGOS_SWERR(AGOS_SWERR_WARNING,"cls_mph_engine_consistence: Leaf does not contain tag",0,0,0,0,0);
                            n_res = AG_E_FAIL;
						}

						if (p_leaf->p_mp)
						{ 
							LeafPtrEl *p_mp_leaf_el;

							/* increase grp leaves counter */
							n_grp_leaves_count++;

							/* Find this leaf in the mp */
							if (AG_FAILED (find_this_leaf(p_leaf, &(p_leaf->p_mp->x_leaves), &p_mp_leaf_el)))
							{
								AGOS_SWERR(AGOS_SWERR_WARNING,"cls_mph_engine_consistence: Leaf is not found in the MP",0,0,0,0,0);
                                n_res = AG_E_FAIL;
							}
							else 
							{ /* Leaf is found in the MP */
								if (p_leaf != p_mp_leaf_el->p_leaf)
								{
									AGOS_SWERR(AGOS_SWERR_WARNING,"cls_mph_engine_consistence: Leaf is not found in the MP",0,0,0,0,0);
                                    n_res = AG_E_FAIL;
								}
							}
						}

						p_element = list_next(&pGrpTable[i]->x_leaves,p_element);
					} while(p_element);
				} /* if (p_element) */
			}	/* not linked group */
		} /* grp is empty */
	}

    
    /* Count a number of leaves in the MPs which linked to Grps */

	/* Shabtay Semaphore Lock*/
	agos_get_bi_semaphore(&xMpSem, AGOS_WAIT_FOR_EVER);
	
    for (i=0; i<nMaxMp; i++)
	{
		if (pMpTable[i])
		{
			p_element=list_head(&pMpTable[i]->x_leaves);
			if (p_element)
			{
				do 
				{
					p_leaf = ((LeafPtrEl*)p_element)->p_leaf;

					if (p_leaf->p_grp)
					{
						n_mp_leaves_count++;
					}

					p_element = list_next(&pMpTable[i]->x_leaves,p_element);
				}
				while (p_element);
			}
		}
	}

	if (n_mp_leaves_count != n_grp_leaves_count)
	{
		AGOS_SWERR(AGOS_SWERR_WARNING,"cls_mph_engine_consistence: The number of leaves in the MPs is not equal to the number of leaves in the GRPs",0,0,0,0,0);
        n_res = AG_E_FAIL;
	}
    if(AG_SUCCEEDED(n_res))
    {
        CLS_PRINTF(("mph and engine are consistent\n"));
    }

	/* Shabtay Semaphore UnLock*/
	agos_give_bi_semaphore(&xMpSem);
	/* Shabtay Semaphore UnLock*/
	agos_give_bi_semaphore(&xGrpSem);
	
    return n_res;
}

AgResult mph_empty_table (void)
{
	AG_U32 i; 
	AgResult n_res = AG_S_OK;
    
    CLS_PRINTF(("Verifying that MPH is empty.\n"));

	/* Shabtay Semaphore Lock*/
	agos_get_bi_semaphore(&xGrpSem, AGOS_WAIT_FOR_EVER);

    for (i=0; i<nMaxGrp; i++)
	{
		if (pGrpTable[i] != 0)
		{
            AGOS_SWERR(AGOS_SWERR_WARNING,"mph_empty_table: Group table is not empty. i=",i,0,0,0,0); 
			n_res = AG_E_FAIL;
		}
	}

	/* Shabtay Semaphore Lock*/
	agos_get_bi_semaphore(&xMpSem, AGOS_WAIT_FOR_EVER);

	for (i=0; i<nMaxMp; i++)
    {
        if (pMpTable[i] != 0)
        {
            AGOS_SWERR(AGOS_SWERR_WARNING,"mph_empty_table: MP table is not empty. i=",i,0,0,0,0); 
			n_res = AG_E_FAIL;
		}
	}

#ifdef AG_MEM_MONITOR
	if (nCurrLeafAlloc != 0)
	{
        AGOS_SWERR(AGOS_SWERR_WARNING,"mph_empty_table: Leaf allocation not zero. curr=",nCurrLeafAlloc,0,0,0,0); 
        n_res = AG_E_FAIL;
	}
	if (nCurrGrpAlloc != 0)
	{
        AGOS_SWERR(AGOS_SWERR_WARNING,"mph_empty_table: Group allocation not zero. curr=",nCurrGrpAlloc,0,0,0,0); 
        n_res = AG_E_FAIL;
	}
	if (nCurrMpAlloc != 0)
	{
        AGOS_SWERR(AGOS_SWERR_WARNING,"mph_empty_table: MP allocation not zero. curr=",nCurrMpAlloc,0,0,0,0); 
        n_res = AG_E_FAIL;
	}
	if (nCurrLeafPtrAlloc != 0)
	{
        AGOS_SWERR(AGOS_SWERR_WARNING,"mph_empty_table: Leaf Pointer allocation not zero. curr=",nCurrLeafPtrAlloc,0,0,0,0); 
        n_res = AG_E_FAIL;
	}
	if (nCurrGrpPtrAlloc != 0)
	{
        AGOS_SWERR(AGOS_SWERR_WARNING,"mph_empty_table: Group Pointer allocation not zero. curr=",nCurrGrpPtrAlloc,0,0,0,0); 
        n_res = AG_E_FAIL;
	}
#endif

	/* Shabtay Semaphore UnLock*/
	agos_give_bi_semaphore(&xMpSem);	

	/* Shabtay Semaphore UnLock*/
	agos_give_bi_semaphore(&xGrpSem);

	return n_res;
}

#undef CLS_MP_HANDLER_C 
