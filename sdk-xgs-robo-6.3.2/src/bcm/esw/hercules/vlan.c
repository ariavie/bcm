/*
 * $Id: vlan.c 1.13 Broadcom SDK $
 * $Copyright: Copyright 2012 Broadcom Corporation.
 * This program is the proprietary software of Broadcom Corporation
 * and/or its licensors, and may only be used, duplicated, modified
 * or distributed pursuant to the terms and conditions of a separate,
 * written license agreement executed between you and Broadcom
 * (an "Authorized License").  Except as set forth in an Authorized
 * License, Broadcom grants no license (express or implied), right
 * to use, or waiver of any kind with respect to the Software, and
 * Broadcom expressly reserves all rights in and to the Software
 * and all intellectual property rights therein.  IF YOU HAVE
 * NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE
 * IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE
 * ALL USE OF THE SOFTWARE.  
 *  
 * Except as expressly set forth in the Authorized License,
 *  
 * 1.     This program, including its structure, sequence and organization,
 * constitutes the valuable trade secrets of Broadcom, and you shall use
 * all reasonable efforts to protect the confidentiality thereof,
 * and to use this information only in connection with your use of
 * Broadcom integrated circuit products.
 *  
 * 2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS
 * PROVIDED "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
 * REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY,
 * OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
 * DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
 * NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
 * ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 * CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
 * OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 * 
 * 3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL
 * BROADCOM OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL,
 * INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER
 * ARISING OUT OF OR IN ANY WAY RELATING TO YOUR USE OF OR INABILITY
 * TO USE THE SOFTWARE EVEN IF BROADCOM HAS BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN EXCESS OF
 * THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF OR USD 1.00,
 * WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING
 * ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.$
 *
 * File:        vlan.c
 * Purpose:	Provide low-level access to Hercules VLAN resources
 */

#include <sal/core/boot.h>

#include <soc/drv.h>
#include <soc/mem.h>
#include <soc/debug.h>

#include <bcm/error.h>
#include <bcm/vlan.h>
#include <bcm/port.h>
#include <bcm/trunk.h>

#include <bcm_int/esw/mbcm.h>
#include <bcm_int/esw/hercules.h>

/*
 * Function:
 *	bcm_hercules_vlan_init
 * Purpose:
 *	Initialize the VLAN tables with the default entry in it.
 */

int
bcm_hercules_vlan_init(int unit, bcm_vlan_data_t *vd)
{
    mem_vid_entry_t	ve;

    if ((!SAL_BOOT_PLISIM) && (!SAL_BOOT_QUICKTURN)) {  /* Way too slow! */
        SOC_IF_ERROR_RETURN
	    (soc_mem_clear(unit, MEM_VIDm, MEM_BLOCK_ALL, TRUE));
    } else {
        soc_cm_print("SIMULATION: skipped VLAN table clear "
		     "(assuming hardware did it)\n");
    }

    /* Initialize VLAN 1 */

    sal_memcpy(&ve, soc_mem_entry_null(unit, MEM_VIDm), sizeof (ve));

    soc_MEM_VIDm_field32_set(unit, &ve, VIDBITMAPf,
			     SOC_PBMP_WORD_GET(vd->port_bitmap, 0));

    SOC_IF_ERROR_RETURN
	(soc_mem_write(unit, MEM_VIDm, MEM_BLOCK_ALL, vd->vlan_tag, &ve));

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_hercules_vlan_reload
 * Purpose:
 *	Return a list of VLANs currently installed in the hardware.
 */

int
bcm_hercules_vlan_reload(int unit, vbmp_t *bmp, int *count)
{
    mem_vid_entry_t 	ve;
    uint32 		bmval;
    int 		index;
    int 		blk;

    (*count) = 0;

    for (index = soc_mem_index_min(unit, MEM_VIDm);
         index <= soc_mem_index_max(unit, MEM_VIDm);
	 index++) {

        SOC_MEM_BLOCK_ITER(unit, MEM_VIDm, blk) {

            SOC_IF_ERROR_RETURN(READ_MEM_VIDm(unit, blk, index, &ve));
            soc_MEM_VIDm_field_get(unit, &ve, VIDBITMAPf, &bmval);

            if (bmval) {
                SHR_BITSET(bmp->w, index);
                (*count)++;
                break;
            }
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *	bcm_hercules_vlan_create
 * Purpose:
 *	Create a VLAN entry in MEM_VID
 */

int
bcm_hercules_vlan_create(int unit, bcm_vlan_t vid)
{
    mem_vid_entry_t	ve;

    /* Clear initial port bitmap */
    sal_memset(&ve, 0, sizeof ve);

#ifndef	BCM_VLAN_NO_AUTO_STACK
    soc_MEM_VIDm_field32_set(unit, &ve, VIDBITMAPf,
			     SOC_PBMP_WORD_GET(PBMP_HG_ALL(unit), 0));
#endif	/* BCM_VLAN_NO_AUTO_STACK */

    SOC_IF_ERROR_RETURN(WRITE_MEM_VIDm(unit, MEM_BLOCK_ALL, vid, &ve));

    return BCM_E_NONE;
}

/*
 * Function:
 *	bcm_hercules_vlan_destroy
 * Purpose:
 *	Clear a VLAN entry in MEM_VID
 */

int
bcm_hercules_vlan_destroy(int unit, bcm_vlan_t vid)
{
    mem_vid_entry_t	ve;
    int			rv;

    soc_mem_lock(unit, MEM_VIDm);

    if ((rv = READ_MEM_VIDm(unit, MEM_BLOCK_ANY, vid, &ve)) >= 0) {
	soc_MEM_VIDm_field32_set(unit, &ve, VIDBITMAPf, 0);

	rv = WRITE_MEM_VIDm(unit, MEM_BLOCK_ALL, vid, &ve);
    }

    soc_mem_unlock(unit, MEM_VIDm);

    return rv;
}

/*
 * Function:
 *	bcm_hercules_vlan_port_update
 * Purpose:
 *	Add/remove ports to the VLAN bitmap in a MEM_VID entry.
 * Notes:
 *      Each port has its own forwarding port mask for all VLANs.
 *	If a port is not a member of a VLAN, the corresponding
 *      forwarding port mask is set to zero. This way the table
 *      also acts as an ingress VLAN filter.
 */

STATIC int
bcm_hercules_vlan_port_update(int unit, bcm_vlan_t vid, pbmp_t pbmp, int add)
{
    mem_vid_entry_t	ve[SOC_MAX_NUM_BLKS];
    int			port, blk;
    uint32		bmval = 0;
    int                 rv;

    soc_mem_lock(unit, MEM_VIDm);

    /* Read VLAN entries for all blocks */
    SOC_MEM_BLOCK_ITER(unit, MEM_VIDm, blk) {

        rv = READ_MEM_VIDm(unit, blk, vid, &ve[blk]);

        if (SOC_FAILURE(rv)) {
            soc_mem_unlock(unit, MEM_VIDm);
            return rv;
        }

        if (bmval == 0) {
            /* Pick up ports from existing VLAN member */
            soc_MEM_VIDm_field_get(unit, &ve[blk], VIDBITMAPf, &bmval);
        }
    }

    if (add) {
        bmval |= SOC_PBMP_WORD_GET(pbmp, 0);
    } else {
        bmval &= ~SOC_PBMP_WORD_GET(pbmp, 0);
    }

    /* Write non-zero entries for VLAN members only */
    SOC_MEM_BLOCK_ITER(unit, MEM_VIDm, blk) {
        port = SOC_BLOCK_PORT(unit, blk);
        soc_MEM_VIDm_field32_set(unit, &ve[blk], VIDBITMAPf, 
                                 (bmval & (1 << port)) ? bmval : 0);
        SOC_IF_ERROR_RETURN(WRITE_MEM_VIDm(unit, blk, vid, &ve[blk]));
    }

    soc_mem_unlock(unit, MEM_VIDm);

    return BCM_E_NONE;
}

/*
 * Function:
 *	bcm_hercules_vlan_port_add
 * Purpose:
 *	Add ports to the VLAN bitmap in a MEM_VID entry.
 */

int
bcm_hercules_vlan_port_add(int unit, bcm_vlan_t vid, pbmp_t pbmp, pbmp_t ubmp,
                           pbmp_t ing_pbmp)
{
    BCM_PBMP_OR(pbmp, ing_pbmp);

    return bcm_hercules_vlan_port_update(unit, vid, pbmp, 1);
}

/*
 * Function:
 *	bcm_hercules_vlan_port_remove
 * Purpose:
 *	Remove ports from the VLAN bitmap in a MEM_VID entry.
 */

int
bcm_hercules_vlan_port_remove(int unit, bcm_vlan_t vid, pbmp_t pbmp)
{
    return bcm_hercules_vlan_port_update(unit, vid, pbmp, 0);
}

/*
 * Function:
 *	bcm_hercules_vlan_port_get
 * Purpose:
 *	Read the port bitmap from a MEM_VID entry.
 */

int
bcm_hercules_vlan_port_get(int unit, bcm_vlan_t vid, pbmp_t *pbmp,
                           pbmp_t *ubmp, pbmp_t *ing_pbmp)
{
    mem_vid_entry_t	ve;
    int			blk;
    uint32		bmval = 0;
    int rv;

    soc_mem_lock(unit, MEM_VIDm);

    SOC_MEM_BLOCK_ITER(unit, MEM_VIDm, blk) {

        rv = READ_MEM_VIDm(unit, blk, vid, &ve);

        if (SOC_FAILURE(rv)) {
            soc_mem_unlock(unit, MEM_VIDm);
            return rv;
        }

        bmval = soc_MEM_VIDm_field32_get(unit, &ve, VIDBITMAPf);
        if (bmval) {
            break;
        }
    }

    if (pbmp != NULL) {
	SOC_PBMP_CLEAR(*pbmp);
	SOC_PBMP_WORD_SET(*pbmp, 0, bmval);
    }

    if (ubmp != NULL) {
	SOC_PBMP_CLEAR(*ubmp);
    }

    if (ing_pbmp != NULL) {
	SOC_PBMP_CLEAR(*ing_pbmp);
	SOC_PBMP_WORD_SET(*ing_pbmp, 0, bmval);
    }

    soc_mem_unlock(unit, MEM_VIDm);

    return BCM_E_NONE;
}

/*
 * Function:
 *	bcm_hercules_vlan_stg_get
 * Purpose:
 *	Stub out STG on Hercules
 */

int
bcm_hercules_vlan_stg_get(int unit, bcm_vlan_t vid, bcm_stg_t *stg_ptr)
{
    COMPILER_REFERENCE(unit);
    COMPILER_REFERENCE(vid);

    *stg_ptr = 1;	/* Only one "STG" on Hercules */

    return BCM_E_NONE;
}

/*
 * Function:
 *	bcm_hercules_vlan_stg_set
 * Purpose:
 *	Stub out STG on Hercules
 */

int
bcm_hercules_vlan_stg_set(int unit, bcm_vlan_t vid, bcm_stg_t stg)
{
    COMPILER_REFERENCE(unit);
    COMPILER_REFERENCE(vid);

    /* Only one "STG" on Hercules */

    return (stg == 1) ? BCM_E_NONE : BCM_E_BADID;
}
