/* $Id: 3a601bdcc1079d6b8c98ec296f66a5cf749371f7 $
 * Copyright 2011 BATM
 */

#include "pub/nd_hw.h"
#include "nd_platform.h"
#include "nd_mm.h"
#include "nd_debug.h"


/*///////////////////////////////////////////////////////////////////////////// */
/* */
/* */
AgResult    
ag_nd_allocator_init(
    AgNdAllocator       *p_allocator, 
    AG_U32              n_start, 
    AG_U32              n_size,
    AG_U32              n_block_size)
{
    AG_U32              i;
    AgNdAllocatorBlock  *p_block;


    assert(p_allocator);


    p_allocator->n_block_count = n_size/n_block_size;
    ag_nd_list_init(&p_allocator->x_block_list);


    for (i = 0; i < p_allocator->n_block_count; i++)
    {
        p_block = ag_nd_mm_ram_alloc(sizeof(*p_block));
        if (!p_block)
        {
            p_allocator->n_block_count = i;
            return AG_E_ND_ALLOC;
        }

        ag_nd_list_add_tail(&p_allocator->x_block_list, &p_block->x_list);
        p_block->b_free = AG_TRUE;
        p_block->n_size = n_block_size;
        p_block->n_start = n_start;

        n_start += n_block_size;
    }

    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* */
/* */
AgResult    
ag_nd_allocator_destroy(
    AgNdAllocator       *p_allocator)
{
    AgNdList            *p_list_entry;
    AgNdAllocatorBlock  *p_block;
    AgNdList            *p_aux;


    assert(p_allocator);


    AG_ND_LIST_FOREACH_SAFE(p_list_entry, p_aux, &p_allocator->x_block_list)
    {
        ag_nd_list_del(p_list_entry);
        p_block = AG_ND_LIST_ENTRY(p_list_entry, AgNdAllocatorBlock, x_list);
        ag_nd_mm_ram_free(p_block);
    }

    return AG_S_OK;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* */
/* */
AgResult    
ag_nd_allocator_alloc(
    AgNdAllocator       *p_allocator, 
    AG_U32              n_size, 
    AG_U32              n_alignment, 
    AG_U32              *p_address)
{
    AgNdList            *p_list_entry;
    AgNdAllocatorBlock  *p_block;


    assert(p_allocator);
    assert(p_address);


    AG_ND_LIST_FOREACH(p_list_entry, &p_allocator->x_block_list)
    {
        p_block = AG_ND_LIST_ENTRY(p_list_entry, AgNdAllocatorBlock, x_list);
        if (p_block->b_free && p_block->n_size >= n_size)
        {
            p_block->b_free = AG_FALSE;
            *p_address = p_block->n_start;
            return AG_S_OK;
        }
    }


    *p_address = 0xffffffff;
    return AG_E_ND_ALLOC;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* */
/* */
AgResult    
ag_nd_allocator_free(
    AgNdAllocator       *p_allocator, 
    AG_U32              n_address)
{
    AgNdList            *p_list_entry;
    AgNdAllocatorBlock  *p_block;


    assert(p_allocator);


    AG_ND_LIST_FOREACH(p_list_entry, &p_allocator->x_block_list)
    {
        p_block = AG_ND_LIST_ENTRY(p_list_entry, AgNdAllocatorBlock, x_list);
        if (p_block->n_start == n_address)
        {
            p_block->b_free = AG_TRUE;
            return AG_S_OK;
        }
    }


    return AG_E_ND_ALLOC;
}



/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_mm_ram_alloc */
/* */
void*
ag_nd_mm_ram_alloc(size_t n_size)
{
    void *p_ptr = NULL;

    agos_malloc(n_size, (void **)&p_ptr);
    return p_ptr;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_mm_ram_free */
/* */
void    
ag_nd_mm_ram_free(void *p_ptr)
{
    if (p_ptr)
        agos_free(p_ptr);
}
