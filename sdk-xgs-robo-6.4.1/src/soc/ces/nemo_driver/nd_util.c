/* $Id: 6e3347de43c7bcc2c6931c6d329b4be6a1a453e8 $
 * Copyright 2011 BATM
 */


#ifndef BCM_CES_SDK
#include <stdarg.h>
#include <stdio.h>
#endif
#include "nd_platform.h"
#include "nd_util.h"
#include "nd_debug.h"


/*///////////////////////////////////////////////////////////////////////////// */
/* Rounding down to the next power of 2 */
/* */
AG_U32 
ag_nd_flp2(AG_U32 n) 
{
    n = n | (n >> 1); 
    n = n | (n >> 2); 
    n = n | (n >> 4); 
    n = n | (n >> 8); 
    return n - (n >> 1); 
} 

/*///////////////////////////////////////////////////////////////////////////// */
/* Rounding down to the next power of 2 */
/* */
AG_U32 
ag_nd_clp2(AG_U32 n) 
{
   n = n - 1; 
   n = n | (n >>  1); 
   n = n | (n >>  2); 
   n = n | (n >>  4); 
   n = n | (n >>  8); 
   n = n | (n >> 16); 
   return n + 1; 
} 

/*///////////////////////////////////////////////////////////////////////////// */
/* Log2. The agrument assumed to be a power of 2: counting the number of */
/* trailing zeros */
/*  */
AG_U32
ag_nd_log2(AG_U32 x)
{
    int n; 

    if (x == 0) 
        return(32);

    n = 1; 

    if ((x & 0x0000FFFF) == 0)
    {
        n = n + 16; x = x >>16;
    }

    if ((x & 0x000000FF) == 0)
    {
        n = n + 8; x = x >> 8;
    }

    if ((x & 0x0000000F) == 0)
    {
        n = n + 4; x = x >> 4;
    }

    if ((x & 0x00000003) == 0)
    {
        n = n + 2; x = x >> 2;
    }

    return n - (x & 1); 
} 


/*///////////////////////////////////////////////////////////////////////////// */
/* Rounding up to a multiple of a known power of 2 */
/* */
AG_U32 
ag_nd_round_up2(AG_U32 n, AG_U32 p)
{
    return n + ( (AG_S32)(-(AG_S32)n) & ((1<<p)-1) );
}

/*///////////////////////////////////////////////////////////////////////////// */
/* Rounding down to a multiple of a known power of 2 */
/* */
AG_U32 
ag_nd_round_down2(AG_U32 n, AG_U32 p)
{
    return (n >> p) << p;
}

AG_BOOL
ag_nd_is_power_of_2(AG_U32 n)
{
    return 0 == (n & (n - 1));
}

/*///////////////////////////////////////////////////////////////////////////// */
/* assumed: memory pointed by pos initially was zeroed */
/* */
void 
ag_nd_bitwise_write(AgNdBitwise *p_bw, AG_U32  n_data, AG_U32  n_len)
{
    assert(n_len <= 32);
    assert(p_bw);
    assert(p_bw->p_addr);
    assert(p_bw->n_offset < 32);
    assert(32 - p_bw->n_offset >= n_len); 


    /* */
    /* zero unused bits */
    /* */
    n_data <<= 32 - n_len;
    n_data >>= 32 - n_len;

    /* */
    /* adjust the data so that data msb will fit free bit position */
    /* in the current word */
    /*  */
    n_data <<= 32 - p_bw->n_offset - n_len;

    *(p_bw->p_addr + 0) |= (AG_U8) ((n_data >> 24) & 0xff);     /*BCM Be Le check*/
    *(p_bw->p_addr + 1) |= (AG_U8) ((n_data >> 16) & 0xff);
    *(p_bw->p_addr + 2) |= (AG_U8) ((n_data >>  8) & 0xff);
    *(p_bw->p_addr + 3) |= (AG_U8) ((n_data >>  0) & 0xff);

    p_bw->p_addr +=  (p_bw->n_offset + n_len) >> 3;
    p_bw->n_offset = (p_bw->n_offset + n_len) & 0x7;
}

/*void  */
/*ag_nd_bitwise_write(AgNdBitwisePos *pos, AG_U32  data, AG_U32  len) */
/*{ */
/*    AG_S32   n; */
/*    AG_U32  w1, w2; */
/* */
/*    assert(len <= 32); */
/*    assert(pos); */
/*    assert(pos->p_addr); */
/*    assert(pos->n_offset < 32); */
/* */
/* */
/*    // */
/*    // zero unused bits */
/*    // */
/*    data <<= 32 - len; */
/*    data >>= 32 - len; */
/* */
/*    n = 32 - (pos->n_offset + len);  // amount of free space in current word */
/* */
/*    if (n >= 0) */
/*    { */
/*        // */
/*        // adjust the data so that data msb will fit free bit position */
/*        // in the current 32 bit word */
/*        //  */
/*        w1 = data << (32 - pos->n_offset - len); */
/*     */
/*        *(pos->p_addr + 0) |= (AG_U8) ((w1 >> 24) & 0xff); */
/*        *(pos->p_addr + 1) |= (AG_U8) ((w1 >> 16) & 0xff); */
/*        *(pos->p_addr + 2) |= (AG_U8) ((w1 >>  8) & 0xff); */
/*        *(pos->p_addr + 3) |= (AG_U8) ((w1 >>  0) & 0xff); */
/*    } */
/*    else */
/*    { */
/*        n = -n; // amount of bits to carry to the next 32 bit word */
/* */
/*        w1 = data >> n; */
/*        w2 = data << (32 - n); */
/* */
/*        *(pos->p_addr + 0) |= (AG_U8) ((w1 >> 24) & 0xff); */
/*        *(pos->p_addr + 1) |= (AG_U8) ((w1 >> 16) & 0xff); */
/*        *(pos->p_addr + 2) |= (AG_U8) ((w1 >>  8) & 0xff); */
/*        *(pos->p_addr + 3) |= (AG_U8) ((w1 >>  0) & 0xff); */
/* */
/*        *(pos->p_addr + 4) |= (AG_U8) ((w2 >> 24) & 0xff); */
/*        *(pos->p_addr + 5) |= (AG_U8) ((w2 >> 16) & 0xff); */
/*        *(pos->p_addr + 6) |= (AG_U8) ((w2 >>  8) & 0xff); */
/*        *(pos->p_addr + 7) |= (AG_U8) ((w2 >>  0) & 0xff); */
/*    } */
/* */
/*    pos->p_addr += ((pos->n_offset + len) >> 5) << 2; */
/*    pos->n_offset = (pos->n_offset + len) & 0x1f; */
/*} */

