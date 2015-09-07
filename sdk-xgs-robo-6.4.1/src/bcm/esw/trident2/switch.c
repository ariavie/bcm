/*
 * $Id: switch.c,v 1.6 Broadcom SDK $
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
 * Module : Switch 
 * 
 * Purpose: 
 * This file has the flexible hashing apis and its accessory functions to configure
 * field match data and relative hash offsets for bucket 2 and 3.
 * 
 * The below shows the configurations of flexible hashing mechanism in UDF and Flex
 * tables. Data qualifier apis are used to configure the UDF table with relative
 * packet offsets from L4 header. UDF chunk4 - 7 is being used to configure hash
 * bucket 2 and 3. Flexible hashing offset mapping is done with functions
 * implemented in this file.
 * 
 * 
 * 
 * FLEXIBLE HASHING PACKET PARSER (with UDF)
 * 
 *     Packet            L4
 *     +--------+--------+------------+---------+
 *     |        |        | L4 header  | Payload |
 *     +--------+--------+-+-+--------+---------+
 *                 offset  2 2 ....                                      
 *                  ||                                            -----+ 
 *                  ||                                                 | 
 *                           +----Chunk4----+           PKT.field1(16b)| 
 *         PKT.offset1  -->  | +--Chunk5-----+                         | 
 *                           | | +-Chunk6------+    ==> PKT.field2(16b)|
 *     +   PKT.offset2  -->  | | | +-Chunk7------+                    -+ 
 *                           +-| | |             |      PKT.field3(16b)| 
 *         PKT.offset3  ---->  +-| |UDF_TCAM/OFF |                     | 
 *                               +-|  (4 x 16bit)|      PKT.field4(16b)| 
 *         PKT.offset4  ------>    +-------------+                     | 
 *                                                                -----+ 
 *                                                                      
 *                                                                                           
 * FLEXIBLE HASHING OFFSET MAPPING
 *                                                                                               * 
 *                        (64bx8)  [UDF_CONDITIONAL_CHECK_TABLE_CAMm]
 *                  -----+         +--0-------------+                                    
 *        PKT.field1(16b)|          +-|+-2--------------+                                
 *                       |           +-|+-3--------------+                               
 *        PKT.field2(16b)| 64bit       +| +-4--------------+    ==> offset 2
 *                      -+---- Key      +-| +-5--------------+                           
 *        PKT.field3(16b)|                +-| +-6--------------+   
 *                       |                  +-| +-7--------------+                       
 *        PKT.field4(16b)|                    +-|                |                       
 *                       |                      +----------------+                       
 *                  -----+                 [UDF_CONDITIONAL_CHECK_TABLE_RAMm]
 *   
 */

#include <soc/defs.h>

#if defined(BCM_TRIDENT2_SUPPORT)
#include <soc/mem.h>
#include <soc/mcm/memregs.h>
#include <soc/debug.h>
#include <soc/field.h>
#include <bcm/switch.h>

#include <bcm_int/esw/udf.h>
#include <bcm_int/esw/field.h>
#include <bcm_int/esw/switch.h>
#include <bcm_int/esw/trident2.h>


/* Flex enable regmem and fields */
static soc_reg_t _bcm_td2_flex_hash_control = RTAG7_HASH_CONTROL_3r;
static soc_field_t _bcm_td2_flex_hash_enable_fields[] = {
	ENABLE_FLEX_FIELD_1_Af,
	ENABLE_FLEX_FIELD_1_Bf,
	ENABLE_FLEX_FIELD_2_Af,
	ENABLE_FLEX_FIELD_2_Bf
};
/* register to enable flex hasing scheme */
static soc_reg_t _bcm_td2_ing_hash_config = ING_HASH_CONFIG_0r; 
static soc_field_t _bcm_td2_ing_hash_config_fields[] = {
    ENABLE_FLEX_HASHINGf
};
/* 
 *   TCAM: FIELD_1_MASK, FIELD_2_MASK
 *   FIELD_1_DATA, FIELD_2_DATA

 *   RAM: FIELD_1_MASK, FIELD_2_MASK
 *   FIELD_1_OFFSET, FIELD_2_OFFSET
 */

static soc_field_t _bcm_td2_flex_hash_data[] = {
	FIELD_2_DATAf, 
	FIELD_1_DATAf 
};

static soc_field_t _bcm_td2_flex_hash_mask[] = {
	FIELD_2_MASKf,
	FIELD_1_MASKf
};

static soc_field_t _bcm_td2_flex_hash_offset[] = {
	FIELD_2_OFFSETf, 
	FIELD_1_OFFSETf 
};
static soc_mem_t _bcm_td2_flex_hash_table[] = {
	UDF_CONDITIONAL_CHECK_TABLE_CAMm,
	UDF_CONDITIONAL_CHECK_TABLE_RAMm
};

#define FLEX_HASH_TCAM (_bcm_td2_flex_hash_table[0])
#define FLEX_HASH_OFFSET (_bcm_td2_flex_hash_table[1])
#define FLEX_HASH_TCAM_ENTRY_T udf_conditional_check_table_cam_entry_t
#define FLEX_HASH_OFFSET_ENTRY_T udf_conditional_check_table_ram_entry_t

/*
 * Flex Index usage bitmap operations. The 8 indexes of flex tcam table are
 * being managed with index bitmap operations.
 */
#define _BCM_SWITCH_FLEX_IDX_USED_GET(_u_, _idx_) \
        SHR_BITGET(SWITCH_HASH_INFO(_u_)->hash_idx_bitmap, (_idx_))

#define _BCM_SWITCH_FLEX_IDX_USED_SET(_u_, _idx_) \
        SHR_BITSET(SWITCH_HASH_INFO((_u_))->hash_idx_bitmap, (_idx_))

#define _BCM_SWITCH_FLEX_IDX_USED_CLR(_u_, _idx_) \
        SHR_BITCLR(SWITCH_HASH_INFO((_u_))->hash_idx_bitmap, (_idx_))


#define FLEX_HASH_MAX_FIELDS 4
#define FLEX_HASH_MAX_OFFSET 2
#define FLEX_QUAL_BIT_PER_SIZE 8
typedef struct _bcm_flex_hash_match_arr_s {
	int             qual_id;	/* Data Qualifier Id */
	int             qual_size;	/* Data qualifier size: xbytes, 2byte max */
	uint32          qual_match;	/* Qualifier match data */
	uint32          qual_mask;	/* Qualifier mask */
} _bcm_flex_hash_match_arr_t;

typedef struct _bcm_flex_hash_entry_s {
	int             entry_id;	/* Logical eid for flex hash entry */
	int             hw_idx;	/* Associated flex table index */
	int             entry_count;	/* Reference count of the entry */
	int             offset_count;	/* Flex hash offset count */
	int             hash_offset[FLEX_HASH_MAX_OFFSET]; /* Flex hash offset */
	int             hash_mask[FLEX_HASH_MAX_OFFSET]; /* Flex hash mask */
	_bcm_flex_hash_match_arr_t match_arr[FLEX_HASH_MAX_FIELDS];
	struct _bcm_flex_hash_entry_s *next;	/* Next in flex hash list */
} flex_hash_entry_t, *flex_hash_entry_p;

/* Switch flex hash bookkeeping structure */
typedef struct _bcm_td2_switch_hash_bookeeping_s {
	int             init;
        int             api_ver;
	flex_hash_entry_p hash_entry_list;
	SHR_BITDCL     *hash_idx_bitmap;
} _bcm_td2_switch_hash_bookeeping_t;

STATIC          _bcm_td2_switch_hash_bookeeping_t
    _bcm_td2_switch_hash_bk_info[BCM_MAX_NUM_UNITS];

#define SWITCH_HASH_INFO(_unit_)   (&_bcm_td2_switch_hash_bk_info[_unit_])
#define SWITCH_HASH_INIT(_unit_) \
            (_bcm_td2_switch_hash_bk_info[_unit_].init)
#define SWITCH_HASH_ENTRY_LIST(_unit_) \
            (_bcm_td2_switch_hash_bk_info[_unit_].hash_entry_list)
#define SWITCH_HASH_IDX_BITMAP(_unit_) \
            (_bcm_td2_switch_hash_bk_info[_unit_].hash_idx_bitmap)

/* Global defines */
STATIC uint32   last_hash_entry_id = 0;

/* forward declarations */

STATIC int      _flex_hash_entry_get(int unit, bcm_hash_entry_t hash_entry_id,
				     flex_hash_entry_t ** hash_entry);

STATIC int      _bcm_hash_entry_delete(int unit, bcm_hash_entry_t entry_id);

STATIC int      _flex_hash_entry_destroy(int unit, flex_hash_entry_t * entry);


/*
 * The below listed flexible hashing support routines are modularized for table
 * and list management routines.  
 *  1. Flex hash TCAM table read/write routines
 *     _bcm_flex_hash_table_write
 *     _bcm_flex_hash_table_read
 *     _bcm_flex_hash_control_enable
 *     _bcm_flex_hash_control_disable
 *    
 *  2. Flex hash Entry list management routines 
 *     _flex_hash_entry_get 
 *     _flex_hash_entry_destroy
 *     _flex_hash_entry_destroy_all
 *     _flex_hash_entry_alloc
 *     _flex_hash_entry_add
 *     _flex_hash_entry_delete
 * 
 *  3. Flex hash management routines 
 *     _bcm_hash_entry_create
 *     _bcm_hash_entry_delete
 *     _bcm_hash_entry_qual_set
 *     _bcm_hash_entry_qual_get
 *     _bcm_hash_entry_qual_data_set
 *     _bcm_hash_entry_qual_data_get
 * 
 * 
 *  4. Flexible hash Switch API support routines
 *     bcm_td2_switch_hash_entry_init
 *     bcm_td2_switch_hash_entry_detach
 *     bcm_td2_switch_hash_entry_create
 *     bcm_td2_switch_hash_entry_destroy
 *     bcm_td2_switch_hash_qualify_data
 * 
 *     bcm_td2_switch_hash_entry_install
 *     bcm_td2_switch_hash_entry_reinstall
 *     bcm_td2_switch_hash_entry_remove
 */

/*
 * 
 *  Flex hash TCAM table read/write routines 
 *
 */
STATIC int
_bcm_flex_hash_table_write(int unit, flex_hash_entry_t * hash_entry, uint8 delete)
{
	int             fld, rv = BCM_E_NONE;
	FLEX_HASH_TCAM_ENTRY_T flex_tcam_entry;
	FLEX_HASH_OFFSET_ENTRY_T flex_offset_entry;
	uint32          data[2] = { 0, 0 };
	uint32          mask[2] = { 0, 0 };

	if (NULL == hash_entry) {
		return BCM_E_FAIL;
	}

/*  
    note :qual_size_range is 0~2. maximum 4 match field data in a flexkey.
    it is required to create a flexkey in a such order
    field_data2: 1st qual_data, 2nd qual_data
    field_data1: 3rd qual_data, 4th qual_data; thus
    first and third match field data is shifed to left for 16 bits insdie field data.
    in addition, if qual_size is 1, qual_data is shifed to left for 8 bits inside field data 
    if qual_size is 0 or 2, there won't be any shift inside field data
*/
    data[0] = (hash_entry->match_arr[0].qual_match <<
                  ((hash_entry->match_arr[0].qual_size & 1) * FLEX_QUAL_BIT_PER_SIZE)) << 16;
    data[0] |= (hash_entry->match_arr[1].qual_match <<
                  ((hash_entry->match_arr[1].qual_size & 1) * FLEX_QUAL_BIT_PER_SIZE)) << 0;
    data[1] = (hash_entry->match_arr[2].qual_match <<
                  ((hash_entry->match_arr[2].qual_size & 1) * FLEX_QUAL_BIT_PER_SIZE)) << 16;
    data[1] |= (hash_entry->match_arr[3].qual_match <<
                  ((hash_entry->match_arr[3].qual_size & 1) * FLEX_QUAL_BIT_PER_SIZE)) << 0;

    mask[0] = (hash_entry->match_arr[0].qual_mask <<
                  ((hash_entry->match_arr[0].qual_size & 1) * FLEX_QUAL_BIT_PER_SIZE)) << 16;
    mask[0] |= (hash_entry->match_arr[1].qual_mask <<
                  ((hash_entry->match_arr[1].qual_size & 1) * FLEX_QUAL_BIT_PER_SIZE)) << 0;
    mask[1] = (hash_entry->match_arr[2].qual_mask <<
                  ((hash_entry->match_arr[2].qual_size & 1) * FLEX_QUAL_BIT_PER_SIZE)) << 16;
    mask[1] |= (hash_entry->match_arr[3].qual_mask <<
                  ((hash_entry->match_arr[3].qual_size & 1) * FLEX_QUAL_BIT_PER_SIZE)) << 0;

	sal_memset(&flex_tcam_entry, 0, sizeof(flex_tcam_entry));
	sal_memset(&flex_offset_entry, 0, sizeof(flex_offset_entry));

	rv = soc_mem_read(unit, FLEX_HASH_TCAM, MEM_BLOCK_ANY,
			  hash_entry->hw_idx, &flex_tcam_entry);
	
    if (BCM_FAILURE(rv)) {
		return BCM_E_FAIL;
	}

	rv = soc_mem_read(unit, FLEX_HASH_OFFSET, MEM_BLOCK_ANY,
			  hash_entry->hw_idx, &flex_offset_entry);
	
    if (BCM_FAILURE(rv)) {
		return BCM_E_FAIL;
	}
    
    if (soc_mem_field_valid(unit, FLEX_HASH_TCAM, VALIDf)) { 
		soc_mem_field32_set(unit, FLEX_HASH_TCAM, &flex_tcam_entry,
				    VALIDf, delete? 0:1);
    }

	for (fld = 0; fld < FLEX_HASH_MAX_OFFSET; fld++) {
		soc_mem_field32_set(unit, FLEX_HASH_TCAM, &flex_tcam_entry,
				    _bcm_td2_flex_hash_data[fld], delete ? 0 : data[fld]);
		soc_mem_field32_set(unit, FLEX_HASH_TCAM, &flex_tcam_entry,
				    _bcm_td2_flex_hash_mask[fld], delete ? 0 : mask[fld]);
		if (fld < hash_entry->offset_count) {


            if (hash_entry->hash_offset[fld] >= 
                (FLEX_QUAL_BIT_PER_SIZE *  FLEX_HASH_MAX_OFFSET)) {
                return BCM_E_PARAM; 

            }  
			soc_mem_field32_set(unit, FLEX_HASH_OFFSET,
					    &flex_offset_entry,
					    _bcm_td2_flex_hash_offset[fld],
					    delete ? 0 : hash_entry->hash_offset[fld]);
    		soc_mem_field32_set(unit, FLEX_HASH_OFFSET, 
					    &flex_offset_entry,
					    _bcm_td2_flex_hash_mask[fld],
					    delete ? 0 : hash_entry->hash_mask[fld]);
		}
	}
    
	rv = soc_mem_write(unit, FLEX_HASH_TCAM, MEM_BLOCK_ALL,
			   hash_entry->hw_idx, &flex_tcam_entry);
	
    if (BCM_FAILURE(rv)) {
		return BCM_E_FAIL;
	}

	rv = soc_mem_write(unit, FLEX_HASH_OFFSET, MEM_BLOCK_ANY,
			   hash_entry->hw_idx, &flex_offset_entry);

    if (BCM_FAILURE(rv)) {
		return BCM_E_FAIL;
	}

	return BCM_E_NONE;
}

#if 0 /* Code required for warmboot when implemented */
STATIC int
_bcm_flex_hash_table_read(int unit, int hw_index,
			  flex_hash_entry_t * hash_entry)
{
	int             fld, rv = BCM_E_NONE;
	uint32          data[2], mask[2];
	FLEX_HASH_TCAM_ENTRY_T flex_tcam_entry;
	FLEX_HASH_OFFSET_ENTRY_T flex_offset_entry;

	sal_memset(&flex_tcam_entry, 0, sizeof(flex_tcam_entry));
	sal_memset(&flex_offset_entry, 0, sizeof(flex_offset_entry));
	sal_memset(hash_entry, 0, sizeof(flex_hash_entry_t));

	hash_entry->hw_idx = hw_index;

	rv = soc_mem_read(unit, FLEX_HASH_TCAM, MEM_BLOCK_ANY,
			  hash_entry->hw_idx, &flex_tcam_entry);
	if (BCM_FAILURE(rv)) {
		return BCM_E_FAIL;
	}

	rv = soc_mem_read(unit, FLEX_HASH_OFFSET, MEM_BLOCK_ANY,
			  hash_entry->hw_idx, &flex_offset_entry);
	if (BCM_FAILURE(rv)) {
		return BCM_E_FAIL;
	}

	for (fld = 0; fld < FLEX_HASH_MAX_OFFSET; fld++) {
		data[fld] =
		    soc_mem_field32_get(unit, FLEX_HASH_TCAM, &flex_tcam_entry,
					_bcm_td2_flex_hash_data[fld]);
		mask[fld] =
		    soc_mem_field32_get(unit, FLEX_HASH_TCAM, &flex_tcam_entry,
					_bcm_td2_flex_hash_mask[fld]);
		hash_entry->hash_offset[fld] =
		    soc_mem_field32_get(unit, FLEX_HASH_OFFSET,
					&flex_offset_entry,
					_bcm_td2_flex_hash_offset[fld]);
		if (hash_entry->hash_offset[fld] > 0) {
			hash_entry->offset_count++;
		}
	}

	for (fld = 0; fld < FLEX_HASH_MAX_FIELDS; fld++) {
		hash_entry->match_arr[fld].qual_size = 2;
		hash_entry->match_arr[fld].qual_match = data[fld / 2] >>
		    (((fld & 0x1) * hash_entry->match_arr[fld].qual_size));
		hash_entry->match_arr[fld].qual_mask = mask[fld / 2] >>
		    (((fld & 0x1) * hash_entry->match_arr[fld].qual_size));
	}

	return BCM_E_NONE;
}
#endif

/*
    enable RTAG7_HASH_CONTROL and ING_HASH_CONFIG_0 registers
*/
STATIC int
_bcm_flex_hash_control_enable(int unit)
{

	int             cnt;
	soc_reg_t       reg;
	uint32          hash_control, hash_config;

	/* 
	 * Enable Flexible/UDF controls
	 * Flex hash controls are enabled for Hash A and Hash B, which has 
	 * high precedence over Flex UDF controls.
	 */
	reg = _bcm_td2_flex_hash_control;
	BCM_IF_ERROR_RETURN(soc_reg32_get
			    (unit, reg, REG_PORT_ANY, 0, &hash_control));
	for (cnt = 0;
	     cnt <
	     (sizeof(_bcm_td2_flex_hash_enable_fields) /
	      sizeof(_bcm_td2_flex_hash_enable_fields[0])); cnt++) {
		if (SOC_REG_FIELD_VALID
		    (unit, reg, _bcm_td2_flex_hash_enable_fields[cnt])) {
			soc_reg_field_set(unit, reg, &hash_control,
					  _bcm_td2_flex_hash_enable_fields[cnt],
					  1);
		}
	}
	BCM_IF_ERROR_RETURN(soc_reg32_set
			    (unit, reg, REG_PORT_ANY, 0, hash_control));

    /* Enable Actual TCAM lookup using flex key*/
	reg = _bcm_td2_ing_hash_config;
    BCM_IF_ERROR_RETURN(READ_ING_HASH_CONFIG_0r(unit, &hash_config));

    if (SOC_REG_FIELD_VALID
        (unit, reg, _bcm_td2_ing_hash_config_fields[0])) {
        soc_reg_field_set(unit, reg, &hash_config,
            _bcm_td2_ing_hash_config_fields[0], 1);
    } 

    BCM_IF_ERROR_RETURN(WRITE_ING_HASH_CONFIG_0r(unit, hash_config));

	return BCM_E_NONE;
}

STATIC int
_bcm_flex_hash_control_disable(int unit)
{

	int             cnt;
	soc_reg_t       reg;
	uint32          hash_control, hash_config;

	/* 
	 * Disable Flexible/UDF controls
	 * Flex hash controls are enabled for Hash A and Hash B, which has 
	 * high precedence over Flex UDF controls.
	 */
	reg = _bcm_td2_flex_hash_control;
	BCM_IF_ERROR_RETURN(soc_reg32_get
			    (unit, reg, REG_PORT_ANY, 0, &hash_control));
	for (cnt = 0;
	     cnt <
	     (sizeof(_bcm_td2_flex_hash_enable_fields) /
	      _bcm_td2_flex_hash_enable_fields[0]); cnt++) {
		if (SOC_REG_FIELD_VALID
		    (unit, reg, _bcm_td2_flex_hash_enable_fields[cnt])) {
			soc_reg_field_set(unit, reg, &hash_control,
					  _bcm_td2_flex_hash_enable_fields[cnt],
					  0);
		}
	}
	BCM_IF_ERROR_RETURN(soc_reg32_set
			    (unit, reg, REG_PORT_ANY, 0, hash_control));

    /* Disable TCAM lookup using flex key*/
	reg = _bcm_td2_ing_hash_config;
    BCM_IF_ERROR_RETURN(READ_ING_HASH_CONFIG_0r(unit, &hash_config));

    if (SOC_REG_FIELD_VALID
        (unit, reg, _bcm_td2_ing_hash_config_fields[0])) {

        soc_reg_field_set(unit, reg, &hash_config,
            _bcm_td2_ing_hash_config_fields[0], 0);
    } 

    BCM_IF_ERROR_RETURN(WRITE_ING_HASH_CONFIG_0r(unit, hash_config));
	return BCM_E_NONE;
}

/*
 * 
 *  Flex hash Entry list management routines 
 *
 */

STATIC int
_flex_hash_entry_get(int unit, bcm_hash_entry_t hash_entry_id,
		     flex_hash_entry_t ** hash_entry)
{
	*hash_entry = SWITCH_HASH_ENTRY_LIST(unit);
	while (*hash_entry != NULL) {
		if ((*hash_entry)->entry_id == hash_entry_id) {
			return BCM_E_NONE;
		}
		*hash_entry = (*hash_entry)->next;
	}
	return BCM_E_NOT_FOUND;
}

STATIC int
_flex_hash_entry_destroy(int unit, flex_hash_entry_t * entry)
{

	if (NULL == entry) {
		return BCM_E_NOT_FOUND;
	}

	if (entry->entry_count == 0) {
		sal_free(entry);
	}
	return BCM_E_NONE;
}

STATIC int
_flex_hash_entry_destroy_all(int unit)
{

	flex_hash_entry_t *hash_entry = NULL;
	flex_hash_entry_t *hash_curr = NULL;

	hash_entry = SWITCH_HASH_ENTRY_LIST(unit);

	SWITCH_HASH_ENTRY_LIST(unit) = NULL;
	if (NULL != hash_entry) {
		while (hash_entry != NULL) {
			hash_curr = hash_entry;
			hash_entry = hash_curr->next;
			_BCM_SWITCH_FLEX_IDX_USED_CLR(unit, hash_curr->hw_idx);
			sal_free(hash_curr);
		}
	}
	BCM_IF_ERROR_RETURN(_bcm_flex_hash_control_disable(unit));
	return BCM_E_NONE;
}

STATIC int
_flex_hash_entry_alloc(int unit, flex_hash_entry_t ** entry)
{
	int             entry_size;
	int             entry_wrap = 0;
	flex_hash_entry_p hash_entry = NULL;

	if (NULL == entry) {
		return BCM_E_FAIL;
	}

	entry_size = sizeof(flex_hash_entry_t);
	*entry = sal_alloc(entry_size, "flex hash entry");
	if (*entry == NULL) {
		return BCM_E_MEMORY;
	}
	sal_memset(*entry, 0, entry_size);

	while (BCM_SUCCESS
	       (_flex_hash_entry_get(unit, last_hash_entry_id, &hash_entry))) {
		if (last_hash_entry_id++ == 0) {
			if (entry_wrap++ >= 2) {
				sal_free(*entry);
				return BCM_E_RESOURCE;
			}
		}
	}
	(*entry)->entry_id = last_hash_entry_id;
	(*entry)->next = NULL;   

	return BCM_E_NONE;
}


STATIC int
_flex_hash_entry_add(int unit, flex_hash_entry_t * entry, int hw_index)
{
	flex_hash_entry_t *hash_entry = NULL;
	flex_hash_entry_t *hash_prev = NULL;

	if (NULL == entry) {
		return BCM_E_FAIL;
	}

	hash_entry = SWITCH_HASH_ENTRY_LIST(unit);
    if (NULL == hash_entry) {
        entry->hw_idx = hw_index;
		SWITCH_HASH_ENTRY_LIST(unit) = entry;
		BCM_IF_ERROR_RETURN(_bcm_flex_hash_control_enable(unit));
    } else {
		while (hash_entry != NULL) {
			if (hash_entry->entry_id == entry->entry_id) {
				entry->hw_idx = hw_index;
				/* Increment entry count */
				entry->entry_count++;
			} else if (hash_entry->entry_id < entry->entry_id) {
				if (NULL == hash_prev) {
					SWITCH_HASH_ENTRY_LIST(unit) = entry;
				} else {
					hash_prev->next = entry;
				}
				entry->next = hash_entry;
				entry->hw_idx = hw_index;
				/* Increment entry count */
				entry->entry_count++;
				break;
			} 
			hash_prev = hash_entry;
			hash_entry = hash_entry->next;
		}
	}
	return BCM_E_NONE;
}

STATIC int
_flex_hash_entry_delete(int unit, flex_hash_entry_t * entry)
{
	flex_hash_entry_t *hash_entry = NULL;
	flex_hash_entry_t *hash_prev = NULL;

	if (NULL == entry) {
		return BCM_E_FAIL;
	}

	hash_entry = SWITCH_HASH_ENTRY_LIST(unit);
	if (NULL != hash_entry) {
	
	        while (hash_entry != NULL) {
			if (hash_entry->entry_id == entry->entry_id) {
				if (NULL == hash_prev) {
					SWITCH_HASH_ENTRY_LIST(unit) = hash_entry->next;
				} else {
					hash_prev->next = hash_entry->next;
				}
	            if (SWITCH_HASH_ENTRY_LIST(unit) == NULL) {
					BCM_IF_ERROR_RETURN
					    (_bcm_flex_hash_control_disable(unit));

                }
	            break;	
        }
			hash_prev = hash_entry;
			hash_entry = hash_entry->next;
		}
	}
	return BCM_E_NONE;
}

/*
 * 
 *  Flexible Hashing management routines 
 *
 */

STATIC int
_bcm_hash_entry_create(int unit, bcm_hash_entry_t * entry)
{
	int             num_indexes, idx_cnt;
	int             hw_index = -1;
	flex_hash_entry_p hash_entry = NULL;
	int             rv = BCM_E_NONE;

	/* Allocate hardware index into flex tcam table. */
	num_indexes = soc_mem_index_count(unit, FLEX_HASH_TCAM);
	for (idx_cnt = 0; idx_cnt < num_indexes; idx_cnt++) {
		if (!_BCM_SWITCH_FLEX_IDX_USED_GET(unit, idx_cnt)) {
			hw_index = idx_cnt;	/* free index */
			break;
		}
	}
	if (hw_index == -1) {
		return BCM_E_FULL;
	}
	_BCM_SWITCH_FLEX_IDX_USED_SET(unit, hw_index);

	/* Allocate entry id and associate with hardware index. */
	rv = _flex_hash_entry_alloc(unit, &hash_entry);
	if (BCM_FAILURE(rv)) {
		return rv;
	}

    /* Add in hash entry list. 
       If RTAG7_HASH_CONTROL and ING_HASH_CONFIG_0 are not set,
       the registers will be programemed in this routine */
	rv = _flex_hash_entry_add(unit, hash_entry, hw_index);
	if (BCM_FAILURE(rv)) {
		_flex_hash_entry_destroy(unit, hash_entry);
		return rv;
	}

	/* return entry id */
	*entry = hash_entry->entry_id;
	return rv;
}

STATIC int
_bcm_hash_entry_delete(int unit, bcm_hash_entry_t entry_id)
{
	int             rv = BCM_E_NONE;
	flex_hash_entry_t *hash_entry;

	rv = _flex_hash_entry_get(unit, entry_id, &hash_entry);
	if (BCM_FAILURE(rv)) {
		return rv;
	}

	_BCM_SWITCH_FLEX_IDX_USED_CLR(unit, hash_entry->hw_idx);

	rv = _flex_hash_entry_delete(unit, hash_entry);
	if (BCM_FAILURE(rv)) {
		return rv;
	}
 	
    rv = _bcm_flex_hash_table_write(unit, hash_entry, TRUE);
	if (BCM_FAILURE(rv)) {
		return rv;
	}

	rv = _flex_hash_entry_destroy(unit, hash_entry);
	if (BCM_FAILURE(rv)) {
		return rv;
	}

   
	return rv;
}

STATIC int
_bcm_hash_entry_udf_set(int unit, bcm_hash_entry_t entry_id,
             int qual_count, int *qual_array)
{
    int             id = 0;
    int             rv = BCM_E_NONE;
    int             idx, udf_id, udf_base, udf_max;
    flex_hash_entry_t *hash_entry;
    bcmi_xgs5_udf_offset_info_t *offset_info;

    rv = _flex_hash_entry_get(unit, entry_id, &hash_entry);
    if (BCM_FAILURE(rv)) {
        return rv;
    }

    if (qual_count > FLEX_HASH_MAX_FIELDS) {
        /* reset outbound size */
        qual_count = FLEX_HASH_MAX_FIELDS;
    }

    udf_base = 12; /* UDF2.4 */
    udf_max  = udf_base + FLEX_HASH_MAX_FIELDS - 1;

    for (id = 0; id < qual_count; id++) {
        /* Get data qualifier info. */
        rv = bcmi_xgs5_udf_offset_node_get(unit, qual_array[id],
                                           &offset_info);
        BCM_IF_ERROR_RETURN(rv);

        if (!(BCMI_XGS5_UDF_OFFSET_FLEXHASH == offset_info->flags) ||
            !(offset_info->hw_bmap & (0xf << udf_base))) {
            return BCM_E_PARAM;
        }

        for (udf_id = udf_base; udf_id <= udf_max; udf_id++) {
            if (offset_info->hw_bmap & (0x1 << udf_id)) {
                idx = udf_id - udf_base;
                hash_entry->match_arr[idx].qual_id = qual_array[id];
                hash_entry->match_arr[idx].qual_size = offset_info->width;
                break;
            }
        }
    }
    
    return BCM_E_NONE;
}

STATIC int
_bcm_hash_entry_qual_set(int unit, bcm_hash_entry_t entry_id,
             int qual_count, int *qual_array)
{
    int             id = 0;
    int             rv = BCM_E_NONE;
    int             idx, udf_id, udf_base, udf_max;
    flex_hash_entry_t *hash_entry;
    _field_data_qualifier_t data_qual;

    rv = _flex_hash_entry_get(unit, entry_id, &hash_entry);
    if (BCM_FAILURE(rv)) {
        return rv;
    }

    if (qual_count > FLEX_HASH_MAX_FIELDS) {
        /* reset outbound size */
        qual_count = FLEX_HASH_MAX_FIELDS;
    }

    udf_base = 12; /* UDF2.4 */
    udf_max  = udf_base + FLEX_HASH_MAX_FIELDS - 1;

    for (id = 0; id < qual_count; id++) {
        /* Get data qualifier info. */
        rv = _field_data_qualifier_get(unit, qual_array[id],
                              &data_qual);
        if (BCM_FAILURE(rv)) {
            return (rv);
        }
        if (!
            (BCM_FIELD_DATA_QUALIFIER_OFFSET_FLEX_HASH ==
             data_qual.flags) ||
            !(data_qual.hw_bmap & (0xf << udf_base))) {
            return BCM_E_FAIL;
        }

        for (udf_id = udf_base; udf_id <= udf_max; udf_id++) {
            if (data_qual.hw_bmap & (0x1 << udf_id)) {
                idx = udf_id - udf_base;
                hash_entry->match_arr[idx].qual_id = qual_array[id];
                hash_entry->match_arr[idx].qual_size = data_qual.length;
                break;
            }
        }
    }

    return BCM_E_NONE;
}

#if 0 /* Code required for warmboot when implemented */
STATIC int
_bcm_hash_entry_qual_get(int unit, bcm_hash_entry_t entry_id,
			 int *qual_array, int *qual_count)
{
	int             id = 0;
	int             rv = BCM_E_NONE;
	flex_hash_entry_t *hash_entry;
	bcm_field_data_qualifier_t data_qual;

	*qual_count = 0;
	rv = _flex_hash_entry_get(unit, entry_id, &hash_entry);
	if (BCM_FAILURE(rv)) {
		return rv;
	}

	for (id = 0; id < FLEX_HASH_MAX_FIELDS; id++) {
		if (bcm_esw_field_data_qualifier_get(unit,
						 hash_entry->match_arr[id].
						 qual_id, &data_qual)) {
			qual_array[id] = hash_entry->match_arr[id].qual_id;
			*qual_count += 1;
            return BCM_E_NONE;
		}
	}
    return BCM_E_NOT_FOUND;
}
#endif

STATIC int
_bcm_hash_entry_qual_data_set(int unit, bcm_hash_entry_t entry_id, int qual_id,
			      uint32 data, uint32 mask)
{
	int             id = 0;
	int             rv = BCM_E_NONE;
	flex_hash_entry_t *hash_entry;

	rv = _flex_hash_entry_get(unit, entry_id, &hash_entry);
	if (BCM_FAILURE(rv)) {
		return rv;
	}

	for (id = 0; id < FLEX_HASH_MAX_FIELDS; id++) {
		if (hash_entry->match_arr[id].qual_id == qual_id) {
			hash_entry->match_arr[id].qual_match = data;
			hash_entry->match_arr[id].qual_mask = mask;

	        return BCM_E_NONE;
		}
	}
    return BCM_E_NOT_FOUND;
}

#if 0 /* Code required for warmboot when implemented */
STATIC int
_bcm_hash_entry_qual_data_get(int unit, bcm_hash_entry_t entry_id, int qual_id,
			      uint32 * data, uint32 * mask)
{
	int             id = 0;
	int             rv = BCM_E_NONE;
	flex_hash_entry_t *hash_entry;

	rv = _flex_hash_entry_get(unit, entry_id, &hash_entry);
	if (BCM_FAILURE(rv)) {
		return rv;
	}

	for (id = 0; id < FLEX_HASH_MAX_FIELDS; id++) {
		if (hash_entry->match_arr[id].qual_id == qual_id) {
			*data = hash_entry->match_arr[id].qual_match;
			*mask = hash_entry->match_arr[id].qual_mask;
		}
	}

	if (FLEX_HASH_MAX_FIELDS == id) {
		return BCM_E_NOT_FOUND;
	}

	return BCM_E_NONE;
}
#endif

/*
 * 
 *  Flexible hash Switch API support routines 
 *
 */
int
bcm_td2_switch_hash_entry_detach(int unit)
{

	if (SWITCH_HASH_ENTRY_LIST(unit)) {
		_flex_hash_entry_destroy_all(unit);
	}

	if (SWITCH_HASH_IDX_BITMAP(unit)) {
		sal_free(SWITCH_HASH_IDX_BITMAP(unit));
		SWITCH_HASH_IDX_BITMAP(unit) = NULL;
	}

	SWITCH_HASH_INIT(unit) = FALSE;

	return BCM_E_NONE;
}

int
bcm_td2_switch_hash_entry_init(int unit)
{
	int             hash_entries;

	if (SWITCH_HASH_INIT(unit) == TRUE ) {
		bcm_td2_switch_hash_entry_detach(unit);
	}


	if (SWITCH_HASH_IDX_BITMAP(unit)) {

    }
	/* Initialize bookeeping */
	sal_memset(SWITCH_HASH_INFO(unit), 0, sizeof(*SWITCH_HASH_INFO(unit)));

	/* Initialize the hash entry list */
	SWITCH_HASH_ENTRY_LIST(unit) = NULL;

	hash_entries = soc_mem_index_count(unit, FLEX_HASH_TCAM);
	SWITCH_HASH_IDX_BITMAP(unit) = sal_alloc(SHR_BITALLOCSIZE(hash_entries),
						 "hash index bitmap");
	if (NULL == SWITCH_HASH_IDX_BITMAP(unit)) {
		bcm_td2_switch_hash_entry_detach(unit);
		return BCM_E_FAIL;
	}

	SWITCH_HASH_INIT(unit) = TRUE;

    last_hash_entry_id = 0;

	return BCM_E_NONE;
}


/*
 * Function: 
 *      bcm_td2_switch_hash_entry_create
 * Purpose: 
 *      Create a blank flex hash entry.
 * Parameters: 
 *      unit       - (IN) bcm device
 *      group      - (IN)  BCM field group
 *      *entry     - (OUT) BCM hash entry
 *
 * Returns:
 *      BCM_E_NONE - Success
 *      BCM_E_XXX
 * Notes:
 */
int
bcm_td2_switch_hash_entry_create(int unit,
				 bcm_field_group_t group,
				 bcm_hash_entry_t * entry_id)
{
	int             rv = BCM_E_NONE;
	bcm_field_qset_t qset;
	int             qual_array[FLEX_HASH_MAX_FIELDS];
	int             qual_count;


	/* Validate unit, group of qualifier Ids */
	if (!SOC_UNIT_VALID(unit)) {
		return BCM_E_UNIT;
	}

	rv = bcm_esw_field_group_get(unit, group, &qset);
	if (BCM_FAILURE(rv)) {
		return BCM_E_BADID;
	}

        /* Either this API or the create_qset should be used
         * Users should not intermix the APIs.
         */
        if (SWITCH_HASH_INFO(unit)->api_ver == 0) {
            SWITCH_HASH_INFO(unit)->api_ver = 1;
        } else if (SWITCH_HASH_INFO(unit)->api_ver == 2) {
            return BCM_E_CONFIG;
        }


	/* 
	 * Check for the required qualifier fields, the assumption is qual_array
	 * has the same sequence of offsets from UDF2.4 .. UDF2.7 for flex hash
	 */
	rv = bcm_esw_field_qset_data_qualifier_get(unit, qset,
						   FLEX_HASH_MAX_FIELDS,
						   qual_array, &qual_count);
	if (BCM_FAILURE(rv)) {
		return BCM_E_FAIL;
	}

	if (0 == qual_count) {
		return BCM_E_FAIL;
	}

	/* Create flexible hash entry */
	/* Get free index from the flex hash tcam */
	BCM_IF_ERROR_RETURN(_bcm_hash_entry_create(unit, entry_id));

	/* Add qualifier ids in the hash entry */
	BCM_IF_ERROR_RETURN(_bcm_hash_entry_qual_set
			    (unit, *entry_id, qual_count, qual_array));
	return rv;
}

/*
 * Function: 
 *      bcm_td2_switch_hash_entry_create_qset
 * Purpose: 
 *      Create a blank flex hash entry.
 * Parameters: 
 *      unit       - (IN)  bcm device
 *      qset       - (IN)  Qset
 *      *entry     - (OUT) BCM hash entry
 *
 * Returns:
 *      BCM_E_NONE - Success
 *      BCM_E_XXX
 * Notes:
 */
int
bcm_td2_switch_hash_entry_create_qset(int unit,
				      bcm_field_qset_t qset,
                                      bcm_hash_entry_t * entry_id)
{
    int             rv = BCM_E_NONE;
    int             qual_array[FLEX_HASH_MAX_FIELDS];
    int             qual_count;

    /* Either this API or the create_qset should be used
     * Users should not intermix the APIs.
     */
    if (SWITCH_HASH_INFO(unit)->api_ver == 0) {
        SWITCH_HASH_INFO(unit)->api_ver = 2;
    } else if (SWITCH_HASH_INFO(unit)->api_ver == 1) {
        return BCM_E_CONFIG;
    }

    /*
     * Check for the required qualifier fields, the assumption is qual_array
     * has the same sequence of offsets from UDF2.4 .. UDF2.7 for flex hash
     */
    rv = bcm_esw_field_qset_id_multi_get(unit, qset, bcmFieldQualifyUdf,
                                    FLEX_HASH_MAX_FIELDS,
                                    qual_array, &qual_count);
    BCM_IF_ERROR_RETURN(rv);

    if (0 == qual_count) {
        return BCM_E_PARAM;
    }

    /* Create flexible hash entry */
    /* Get free index from the flex hash tcam */
    BCM_IF_ERROR_RETURN(_bcm_hash_entry_create(unit, entry_id));

    /* Add qualifier ids in the hash entry */
    BCM_IF_ERROR_RETURN(_bcm_hash_entry_udf_set
                        (unit, *entry_id, qual_count, qual_array));

    return rv;
}


/*
 * Function: 
 *      bcm_td2_switch_hash_entry_destroy
 * Purpose: 
 *      Destroy a flex hash entry.
 * Parameters: 
 *      unit       - (IN) bcm device
 *      entry_id   - (IN) BCM hash entry
 *
 * Returns:
 *      BCM_E_NONE - Success
 *      BCM_E_XXX
 * Notes:
 */
int
bcm_td2_switch_hash_entry_destroy(int unit, bcm_hash_entry_t entry_id)
{

	return _bcm_hash_entry_delete(unit, entry_id);
}

/*
 * Function: 
 *      bcm_td2_switch_hash_entry_install
 * Purpose: 
 *      Install a flex hash entry into hardware tables.
 * Parameters: 
 *      unit       - (IN) bcm device
 *      entry      - (IN) BCM hash entry
 *
 * Returns:
 *      BCM_E_NONE - Success
 *      BCM_E_XXX
 * Notes:
 */
int
bcm_td2_switch_hash_entry_install(int unit, bcm_hash_entry_t entry_id,
				  uint32 offset, uint32 mask)
{
	int             rv = BCM_E_NONE;
	flex_hash_entry_t *hash_entry;

	rv = _flex_hash_entry_get(unit, entry_id, &hash_entry);
	if (BCM_FAILURE(rv)) {
		return rv;
	}

	if (++hash_entry->offset_count > FLEX_HASH_MAX_OFFSET) {
		return BCM_E_RESOURCE;
	}
	hash_entry->hash_offset[hash_entry->offset_count - 1] = offset & 0xf;
	hash_entry->hash_mask[hash_entry->offset_count - 1] = mask & 0xffff;

	rv = _bcm_flex_hash_table_write(unit, hash_entry, FALSE);
	if (BCM_FAILURE(rv)) {
		return rv;
	}

	return rv;
}

/*
 * Function: 
 *      bcm_td2_switch_hash_entry_reinstall
 * Purpose: 
 *      Re-install a flex hash entry into hardware tables.
 * Parameters: 
 *      unit       - (IN) bcm device
 *      entry      - (IN) BCM hash entry
 *      offset     - (IN) BCM hash offset
 *
 * Returns:
 *      BCM_E_NONE - Success
 *      BCM_E_XXX
 * Notes:
 */
int
bcm_td2_switch_hash_entry_reinstall(int unit,
				    bcm_hash_entry_t entry_id, uint32 offset, uint32 mask)
{
	int             cnt, rv = BCM_E_NONE, reinstall;
	flex_hash_entry_t *hash_entry;

	rv = _flex_hash_entry_get(unit, entry_id, &hash_entry);
	if (BCM_FAILURE(rv)) {
		return rv;
	}

    reinstall = 1;
	for (cnt = 0; cnt < FLEX_HASH_MAX_OFFSET; cnt++) {
		if (hash_entry->hash_offset[cnt] == -1) {
			reinstall = 0;
		}
	}
	if ((reinstall == 1) && (cnt == FLEX_HASH_MAX_OFFSET)) {
		for (cnt = 0; cnt < FLEX_HASH_MAX_OFFSET; cnt++) {
			hash_entry->hash_offset[cnt] = -1;
			hash_entry->offset_count = 0;
		}
	}

	return bcm_td2_switch_hash_entry_install(unit, entry_id, offset, mask);
}

/*
 * Function: 
 *      bcm_td2_switch_hash_entry_remove
 * Purpose: 
 *      Remove a flex hash entry from hardware tables.
 * Parameters: 
 *      unit       - (IN) bcm device
 *      entry      - (IN) BCM hash entry
 *
 * Returns:
 *      BCM_E_NONE - Success
 *      BCM_E_XXX
 * Notes:
 */
int
bcm_td2_switch_hash_entry_remove(int unit, bcm_hash_entry_t entry_id)
{
	int             rv = BCM_E_NONE;

	rv = _bcm_hash_entry_delete(unit, entry_id);
	if (BCM_FAILURE(rv)) {
		return rv;
	}
	return rv;
}

/*
 * Function: 
 *      bcm_td2_switch_hash_qualify_data
 * Purpose: 
 *      Add flex hash field qualifiers into hardware tables.
 * Parameters: 
 *      unit       - (IN) bcm device
 *      entry      - (IN) BCM hash entry
 *      qual_id    - (IN) BCM qualifier id
 *      data       - (IN) BCM hash field qualifier
 *      mask       - (IN) BCM hash field mask
 * Returns:
 *      BCM_E_NONE - Success
 *      BCM_E_XXX
 * Notes:
 */

int
bcm_td2_switch_hash_qualify_data(int unit, bcm_hash_entry_t entry_id,
				 int qual_id, uint32 data, uint32 mask)
{
	int             rv = BCM_E_NONE;

	rv = _bcm_hash_entry_qual_data_set(unit, entry_id, qual_id, data, mask);
	if (BCM_FAILURE(rv)) {
		return rv;
	}

	return rv;
}

#endif				/* defined(BCM_TRIDENT2_SUPPORT) */
