/* $Id: 9f808dd4bfe28f08aa65d526aed45ad5e4f61ad8 $
 * Copyright 2011 BATM
 */


#include "pub/nd_api.h"
#include "pub/nd_hw.h"
#include "nd_registers.h"
#include "nd_debug.h"
#include "nd_platform.h"

extern AgNdDevice *g_ndDevice;

AG_U16 ag_nd_tx_mirror[8][8];
int ag_nd_read_tx_signaling(unsigned long n_circuit_id,unsigned long n_ts,unsigned char * p_ABCD)
{
    AG_U16 n_abcd;
    int index;
    int address_index = (n_ts>>2);

#ifdef AG_ND_ENABLE_VALIDATION
    if (!g_ndDevice->p_circuit_is_valid_id(g_ndDevice, n_circuit_id) || n_circuit_id > 8)
    {
        return AG_ND_ERR(g_ndDevice, AG_E_ND_ARG);
    }
#endif

    index = n_ts & 0x3;

    n_abcd = ag_nd_tx_mirror[n_circuit_id][address_index];

    n_abcd &= (0xf << (index<<2));

    n_abcd >>= (index<<2);

    *p_ABCD = (AG_U8)n_abcd;

    return AG_S_OK; 
}


int ag_nd_write_tx_signaling(unsigned long n_circuit_id,unsigned long n_ts,unsigned char n_new_ABCD)
{
  #ifndef CES16_BCM_VERSION
    AG_U32 n_reg_address_offset = AG_REG_CAS_EGRESS_DATA_BUFFER(n_circuit_id,0);
    AG_U16 *n_abcd_ptr;
    AG_U16 n_abcd;
    AG_U16 n_new_ABCD_U16 = 0;
    int index;
    int address_index = (n_ts>>2);
    /*int address_index = (n_ts/4); */

#ifdef AG_ND_ENABLE_VALIDATION
    if (!g_ndDevice->p_circuit_is_valid_id(g_ndDevice, n_circuit_id) || n_circuit_id > 8)
    {
        return AG_ND_ERR(g_ndDevice, AG_E_ND_ARG);
    }
#endif

    /*n_reg_address_offset += (n_ts/4)*2; */
    n_reg_address_offset += (n_ts>>2)<<1;

    index = n_ts & 0x3; /*%4 */

    n_abcd_ptr = &(ag_nd_tx_mirror[n_circuit_id][address_index]);
    n_abcd = *n_abcd_ptr;

    n_abcd &= ~(0xf << (index<<2));
    
    n_new_ABCD_U16 = (n_new_ABCD << (index<<2));

    n_abcd = (n_abcd | n_new_ABCD_U16);

    /*if (n_abcd != *n_abcd_ptr) */
    {
        *n_abcd_ptr = n_abcd;

        ag_nd_reg_write(g_ndDevice, n_reg_address_offset, n_abcd);
    }
 #endif
    return AG_S_OK;
}


int ag_nd_read_rx_signaling(unsigned long n_circuit_id,unsigned long n_ts,unsigned char * p_ABCD)
{
  #ifndef CES16_BCM_VERSION

    AG_U32 n_reg_address_offset = AG_REG_CAS_INGRESS_DATA_BUFFER(n_circuit_id, 0);
    AG_U16 n_abcd;
    int index;

#ifdef AG_ND_ENABLE_VALIDATION
    if (!g_ndDevice->p_circuit_is_valid_id(g_ndDevice, n_circuit_id) || n_circuit_id > 8)
    {
        return AG_ND_ERR(g_ndDevice, AG_E_ND_ARG);
    }
#endif

    n_reg_address_offset += (n_ts>>2)<<1;
    /*n_reg_address_offset += (n_ts/4)*2; */

    ag_nd_reg_read(g_ndDevice, n_reg_address_offset, &n_abcd);

    /*index = n_ts%4; */
    index = n_ts&0x3;
    
    /**p_ABCD = (n_abcd >> (index*4)) & 0xf; */
    *p_ABCD = (n_abcd >> (index<<2)) & 0xf;
   #endif
    return AG_S_OK;
}

