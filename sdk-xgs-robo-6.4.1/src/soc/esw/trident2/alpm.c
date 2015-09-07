/*
 * $Id: alpm.c,v 1.49 Broadcom SDK $
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
 * File:    alpm.c
 * Purpose: Primitives for LPM management in ALPM - Mode.
 * Requires:
 */

/* Implementation notes:
 */
#include <shared/bsl.h>

#include <soc/mem.h>
#include <soc/drv.h>
#include <soc/debug.h>
#include <soc/error.h>
#include <soc/lpm.h>
#include <soc/trident2.h>
#include <shared/bsl.h>
#ifdef ALPM_ENABLE
#include <shared/util.h>
#include <shared/l3.h>
#include <soc/alpm.h>
#include <soc/esw/alpm_int.h>
#include <soc/esw/trie.h>

#if 1
soc_alpm_bucket_t soc_alpm_bucket[SOC_MAX_NUM_DEVICES];void l1(int l2);static
int l3(int l2);static int l4(int l2);static int l5(int l2,void*l6,int*index);
static int l7(int l2,void*l8);static int l9(int l2,void*l8,int l10,int l11,
int l12,defip_aux_scratch_entry_t*l13);static int l14(int l2,void*l8,void*l15
,int*l16,int*l17,int*l10);static int l18(int l2,void*l8,void*l15,int*l16);
static int l19(int l2);static int l20(int l21,void*lpm_entry,void*l22,void*
l23,soc_mem_t l24,uint32 l25,uint32*l26);static int l27(int l21,void*l22,
soc_mem_t l24,int l10,int l28,int l29,int index,void*lpm_entry);static int l30
(int l21,uint32*key,int len,int l28,int l10,defip_entry_t*lpm_entry,int l31,
int l32);static int l33(int l2,int l28,int l34);static int l35(int l2,void*
entry,int*l17);typedef struct l36{int l37;int l38;int l39;int next;int l40;
int l41;}l42,*l43;static l43 l44[SOC_MAX_NUM_DEVICES];typedef struct l45{
soc_field_info_t*l46;soc_field_info_t*l47;soc_field_info_t*l48;
soc_field_info_t*l49;soc_field_info_t*l50;soc_field_info_t*l51;
soc_field_info_t*l52;soc_field_info_t*l53;soc_field_info_t*l54;
soc_field_info_t*l55;soc_field_info_t*l56;soc_field_info_t*l57;
soc_field_info_t*l58;soc_field_info_t*l59;soc_field_info_t*l60;
soc_field_info_t*l61;soc_field_info_t*l62;soc_field_info_t*l63;
soc_field_info_t*l64;soc_field_info_t*l65;soc_field_info_t*l66;
soc_field_info_t*l67;soc_field_info_t*l68;soc_field_info_t*l69;
soc_field_info_t*l70;soc_field_info_t*l71;soc_field_info_t*l72;
soc_field_info_t*l73;soc_field_info_t*l74;soc_field_info_t*l75;
soc_field_info_t*l76;soc_field_info_t*l77;soc_field_info_t*l78;
soc_field_info_t*l79;soc_field_info_t*l80;soc_field_info_t*l81;
soc_field_info_t*l82;soc_field_info_t*l83;soc_field_info_t*l84;
soc_field_info_t*l85;soc_field_info_t*l86;soc_field_info_t*l87;
soc_field_info_t*l88;soc_field_info_t*l89;soc_field_info_t*l90;
soc_field_info_t*l91;soc_field_info_t*l92;soc_field_info_t*l93;}l94,*l95;
static l95 l96[SOC_MAX_NUM_DEVICES];typedef struct l97{int l21;int l98;int l99
;uint16*l100;uint16*l101;}l102;typedef uint32 l103[5];typedef int(*l104)(l103
l105,l103 l106);static l102*l107[SOC_MAX_NUM_DEVICES];static void l108(int l2
,void*l15,int index,l103 l109);static uint16 l110(uint8*l111,int l112);static
int l113(int l21,int l98,int l99,l102**l114);static int l115(l102*l116);
static int l117(l102*l118,l104 l119,l103 entry,int l120,uint16*l121);static
int l122(l102*l118,l104 l119,l103 entry,int l120,uint16 l123,uint16 l124);
static int l125(l102*l118,l104 l119,l103 entry,int l120,uint16 l126);
alpm_vrf_handle_t alpm_vrf_handle[SOC_MAX_NUM_DEVICES][MAX_VRF_ID+1];
alpm_pivot_t*tcam_pivot[SOC_MAX_NUM_DEVICES][MAX_PIVOT_COUNT];int
soc_alpm_mode_get(int l2){if(soc_trident2_alpm_mode_get(l2) == 1){return 1;}
else{return 0;}}static int l127(int l2,const void*entry,int*l120){int l128;
uint32 l129;int l10;l10 = soc_mem_field32_get(l2,L3_DEFIPm,entry,MODE0f);if(
l10){l129 = soc_mem_field32_get(l2,L3_DEFIPm,entry,IP_ADDR_MASK0f);if((l128 = 
_ipmask2pfx(l129,l120))<0){return(l128);}l129 = soc_mem_field32_get(l2,
L3_DEFIPm,entry,IP_ADDR_MASK1f);if(*l120){if(l129!= 0xffffffff){return(
SOC_E_PARAM);}*l120+= 32;}else{if((l128 = _ipmask2pfx(l129,l120))<0){return(
l128);}}}else{l129 = soc_mem_field32_get(l2,L3_DEFIPm,entry,IP_ADDR_MASK0f);
if((l128 = _ipmask2pfx(l129,l120))<0){return(l128);}}return SOC_E_NONE;}int
_soc_alpm_rpf_entry(int l2,int l130){int l131;l131 = (l130>>2)&0x3fff;l131+= 
SOC_ALPM_BUCKET_COUNT(l2);return(l130&~(0x3fff<<2))|(l131<<2);}int
soc_alpm_physical_idx(int l2,soc_mem_t l24,int index,int l132){int l133 = 
index&1;if(l132){return soc_trident2_l3_defip_index_map(l2,l24,index);}index
>>= 1;index = soc_trident2_l3_defip_index_map(l2,l24,index);index<<= 1;index
|= l133;return index;}int soc_alpm_logical_idx(int l2,soc_mem_t l24,int index
,int l132){int l133 = index&1;if(l132){return
soc_trident2_l3_defip_index_remap(l2,l24,index);}index>>= 1;index = 
soc_trident2_l3_defip_index_remap(l2,l24,index);index<<= 1;index|= l133;
return index;}static int l134(int l2,void*entry,uint32*prefix,uint32*l17,int*
l26){int l135,l136,l10;int l120 = 0;int l128 = SOC_E_NONE;uint32 l137,l133;
prefix[0] = prefix[1] = prefix[2] = prefix[3] = prefix[4] = 0;l10 = 
soc_mem_field32_get(l2,L3_DEFIPm,entry,MODE0f);l135 = soc_mem_field32_get(l2,
L3_DEFIPm,entry,IP_ADDR0f);l136 = soc_mem_field32_get(l2,L3_DEFIPm,entry,
IP_ADDR_MASK0f);prefix[1] = l135;l135 = soc_mem_field32_get(l2,L3_DEFIPm,
entry,IP_ADDR1f);l136 = soc_mem_field32_get(l2,L3_DEFIPm,entry,IP_ADDR_MASK1f
);prefix[0] = l135;if(l10){prefix[4] = prefix[1];prefix[3] = prefix[0];prefix
[1] = prefix[0] = 0;l136 = soc_mem_field32_get(l2,L3_DEFIPm,entry,
IP_ADDR_MASK0f);if((l128 = _ipmask2pfx(l136,&l120))<0){return(l128);}l136 = 
soc_mem_field32_get(l2,L3_DEFIPm,entry,IP_ADDR_MASK1f);if(l120){if(l136!= 
0xffffffff){return(SOC_E_PARAM);}l120+= 32;}else{if((l128 = _ipmask2pfx(l136,
&l120))<0){return(l128);}}l137 = 64-l120;if(l137<32){prefix[4]>>= l137;l133 = 
(((32-l137) == 32)?0:(prefix[3])<<(32-l137));prefix[3]>>= l137;prefix[4]|= 
l133;}else{prefix[4] = (((l137-32) == 32)?0:(prefix[3])>>(l137-32));prefix[3]
= 0;}}else{l136 = soc_mem_field32_get(l2,L3_DEFIPm,entry,IP_ADDR_MASK0f);if((
l128 = _ipmask2pfx(l136,&l120))<0){return(l128);}prefix[1] = (((32-l120) == 
32)?0:(prefix[1])>>(32-l120));prefix[0] = 0;}*l17 = l120;*l26 = (prefix[0] == 
0)&&(prefix[1] == 0)&&(l120 == 0);return SOC_E_NONE;}int _soc_alpm_find_in_bkt
(int l2,soc_mem_t l24,int bucket_index,int l138,uint32*l15,void*l139,int*l121
,int l34){int l128;l128 = soc_mem_alpm_lookup(l2,l24,bucket_index,
MEM_BLOCK_ANY,l138,l15,l139,l121);if(SOC_SUCCESS(l128)){return l128;}if(
SOC_ALPM_V6_SCALE_CHECK(l2,l34)){return soc_mem_alpm_lookup(l2,l24,
bucket_index+1,MEM_BLOCK_ANY,l138,l15,l139,l121);}return l128;}static int l140
(int l2,uint32*prefix,uint32 l141,int l34,int l28,int*l142,int*l143,int*
bucket_index){int l128 = SOC_E_NONE;trie_t*l144;trie_node_t*l145 = NULL;
alpm_pivot_t*l146;if(l34){l144 = VRF_PIVOT_TRIE_IPV6(l2,l28);}else{l144 = 
VRF_PIVOT_TRIE_IPV4(l2,l28);}l128 = trie_find_lpm(l144,prefix,l141,&l145);if(
SOC_FAILURE(l128)){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"Pivot find failed\n")));return l128;}l146 = (alpm_pivot_t*)l145;*l142 = 1;*
l143 = PIVOT_TCAM_INDEX(l146);*bucket_index = PIVOT_BUCKET_INDEX(l146);return
SOC_E_NONE;}static int l147(int l2,void*l8,soc_mem_t l24,void*l139,int*l143,
int*bucket_index,int*l16,int l148){uint32 l15[SOC_MAX_MEM_FIELD_WORDS];int
l149,l28,l34;int l121;uint32 l11,l138;int l128 = SOC_E_NONE;int l142 = 0;l34 = 
soc_mem_field32_get(l2,L3_DEFIPm,l8,MODE0f);if(l34){if(!(l34 = 
soc_mem_field32_get(l2,L3_DEFIPm,l8,MODE1f))){return(SOC_E_PARAM);}}
SOC_IF_ERROR_RETURN(soc_alpm_lpm_vrf_get(l2,l8,&l149,&l28));if(l149 == 0){if(
soc_alpm_mode_get(l2)){return SOC_E_PARAM;}}if(l28 == SOC_VRF_MAX(l2)+1){l11 = 
0;SOC_ALPM_GET_GLOBAL_BANK_DISABLE(l2,l138);}else{l11 = 2;
SOC_ALPM_GET_VRF_BANK_DISABLE(l2,l138);}if(l149!= SOC_L3_VRF_OVERRIDE){if(
l148){uint32 prefix[5],l141;int l26 = 0;l128 = l134(l2,l8,prefix,&l141,&l26);
if(SOC_FAILURE(l128)){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"_soc_alpm_insert: prefix create failed\n")));return l128;}l128 = l140(l2,
prefix,l141,l34,l28,&l142,l143,bucket_index);SOC_IF_ERROR_RETURN(l128);}else{
defip_aux_scratch_entry_t l13;sal_memset(&l13,0,sizeof(
defip_aux_scratch_entry_t));SOC_IF_ERROR_RETURN(l9(l2,l8,l34,l11,0,&l13));
SOC_IF_ERROR_RETURN(_soc_alpm_aux_op(l2,PREFIX_LOOKUP,&l13,TRUE,&l142,l143,
bucket_index));}if(l142){l20(l2,l8,l15,0,l24,0,0);l128 = 
_soc_alpm_find_in_bkt(l2,l24,*bucket_index,l138,l15,l139,&l121,l34);if(
SOC_SUCCESS(l128)){*l16 = l121;}}else{l128 = SOC_E_NOT_FOUND;}}return l128;}
static int l150(int l2,void*l8,void*l139,void*l151,soc_mem_t l24,int l121){
defip_aux_scratch_entry_t l13;int l149,l34,l28;int bucket_index;uint32 l11,
l138;int l128 = SOC_E_NONE;int l142 = 0,l133 = 0;int l143;l34 = 
soc_mem_field32_get(l2,L3_DEFIPm,l8,MODE0f);if(l34){if(!(l34 = 
soc_mem_field32_get(l2,L3_DEFIPm,l8,MODE1f))){return(SOC_E_PARAM);}}
SOC_IF_ERROR_RETURN(soc_alpm_lpm_vrf_get(l2,l8,&l149,&l28));if(l28 == 
SOC_VRF_MAX(l2)+1){l11 = 0;SOC_ALPM_GET_GLOBAL_BANK_DISABLE(l2,l138);}else{
l11 = 2;SOC_ALPM_GET_VRF_BANK_DISABLE(l2,l138);}if(!soc_alpm_mode_get(l2)){
l11 = 2;}if(l149!= SOC_L3_VRF_OVERRIDE){sal_memset(&l13,0,sizeof(
defip_aux_scratch_entry_t));SOC_IF_ERROR_RETURN(l9(l2,l8,l34,l11,0,&l13));
SOC_IF_ERROR_RETURN(soc_mem_write(l2,l24,MEM_BLOCK_ANY,l121,l139));if(
SOC_URPF_STATUS_GET(l2)){SOC_IF_ERROR_RETURN(soc_mem_write(l2,l24,
MEM_BLOCK_ANY,_soc_alpm_rpf_entry(l2,l121),l151));if(l128!= SOC_E_NONE){
return SOC_E_FAIL;}}l133 = soc_mem_field32_get(l2,L3_DEFIP_AUX_SCRATCHm,&l13,
IP_LENGTHf);soc_mem_field32_set(l2,L3_DEFIP_AUX_SCRATCHm,&l13,REPLACE_LENf,
l133);SOC_IF_ERROR_RETURN(_soc_alpm_aux_op(l2,DELETE_PROPAGATE,&l13,TRUE,&
l142,&l143,&bucket_index));if(SOC_URPF_STATUS_GET(l2)){l133 = 
soc_mem_field32_get(l2,L3_DEFIP_AUX_SCRATCHm,&l13,DB_TYPEf);l133+= 1;
soc_mem_field32_set(l2,L3_DEFIP_AUX_SCRATCHm,&l13,DB_TYPEf,l133);
SOC_IF_ERROR_RETURN(_soc_alpm_aux_op(l2,DELETE_PROPAGATE,&l13,TRUE,&l142,&
l143,&bucket_index));}}return l128;}int alpm_mem_prefix_array_cb(trie_node_t*
node,void*l152){alpm_mem_prefix_array_t*l153 = (alpm_mem_prefix_array_t*)l152
;if(node->type == PAYLOAD){l153->prefix[l153->count] = (payload_t*)node;l153
->count++;}return SOC_E_NONE;}int alpm_delete_node_cb(trie_node_t*node,void*
l152){if(node!= NULL){sal_free(node);}return SOC_E_NONE;}static int l154(int
l2,int l155,int l34,int l156){int l128,l133,index;defip_aux_table_entry_t
entry;index = l155>>(l34?0:1);l128 = soc_mem_read(l2,L3_DEFIP_AUX_TABLEm,
MEM_BLOCK_ANY,index,&entry);SOC_IF_ERROR_RETURN(l128);if(l34){
soc_mem_field32_set(l2,L3_DEFIP_AUX_TABLEm,&entry,BPM_LENGTH0f,l156);
soc_mem_field32_set(l2,L3_DEFIP_AUX_TABLEm,&entry,BPM_LENGTH1f,l156);l133 = 
soc_mem_field32_get(l2,L3_DEFIP_AUX_TABLEm,&entry,DB_TYPE0f);}else{if(l155&1)
{soc_mem_field32_set(l2,L3_DEFIP_AUX_TABLEm,&entry,BPM_LENGTH1f,l156);l133 = 
soc_mem_field32_get(l2,L3_DEFIP_AUX_TABLEm,&entry,DB_TYPE1f);}else{
soc_mem_field32_set(l2,L3_DEFIP_AUX_TABLEm,&entry,BPM_LENGTH0f,l156);l133 = 
soc_mem_field32_get(l2,L3_DEFIP_AUX_TABLEm,&entry,DB_TYPE0f);}}l128 = 
soc_mem_write(l2,L3_DEFIP_AUX_TABLEm,MEM_BLOCK_ANY,index,&entry);
SOC_IF_ERROR_RETURN(l128);if(SOC_URPF_STATUS_GET(l2)){l133++;if(l34){
soc_mem_field32_set(l2,L3_DEFIP_AUX_TABLEm,&entry,BPM_LENGTH0f,l156);
soc_mem_field32_set(l2,L3_DEFIP_AUX_TABLEm,&entry,BPM_LENGTH1f,l156);}else{if
(l155&1){soc_mem_field32_set(l2,L3_DEFIP_AUX_TABLEm,&entry,BPM_LENGTH1f,l156)
;}else{soc_mem_field32_set(l2,L3_DEFIP_AUX_TABLEm,&entry,BPM_LENGTH0f,l156);}
}soc_mem_field32_set(l2,L3_DEFIP_AUX_TABLEm,&entry,DB_TYPE0f,l133);
soc_mem_field32_set(l2,L3_DEFIP_AUX_TABLEm,&entry,DB_TYPE1f,l133);index+= (2*
soc_mem_index_count(l2,L3_DEFIP_PAIR_128m)+soc_mem_index_count(l2,L3_DEFIPm))
/2;l128 = soc_mem_write(l2,L3_DEFIP_AUX_TABLEm,MEM_BLOCK_ANY,index,&entry);}
return l128;}static int l157(int l2,int l158,void*entry,void*l159,int l160){
uint32 l133,l136,l34,l11,l161 = 0;soc_mem_t l24 = L3_DEFIPm;soc_mem_t l162 = 
L3_DEFIP_AUX_TABLEm;defip_entry_t l163;int l128 = SOC_E_NONE,l120,l164,l28,
l165;SOC_IF_ERROR_RETURN(soc_mem_read(l2,l162,MEM_BLOCK_ANY,l158,l159));l133 = 
soc_mem_field32_get(l2,l24,entry,VRF_ID_0f);soc_mem_field32_set(l2,l162,l159,
VRF0f,l133);l133 = soc_mem_field32_get(l2,l24,entry,VRF_ID_1f);
soc_mem_field32_set(l2,l162,l159,VRF1f,l133);l133 = soc_mem_field32_get(l2,
l24,entry,MODE0f);soc_mem_field32_set(l2,l162,l159,MODE0f,l133);l133 = 
soc_mem_field32_get(l2,l24,entry,MODE1f);soc_mem_field32_set(l2,l162,l159,
MODE1f,l133);l34 = l133;l133 = soc_mem_field32_get(l2,l24,entry,VALID0f);
soc_mem_field32_set(l2,l162,l159,VALID0f,l133);l133 = soc_mem_field32_get(l2,
l24,entry,VALID1f);soc_mem_field32_set(l2,l162,l159,VALID1f,l133);l133 = 
soc_mem_field32_get(l2,l24,entry,IP_ADDR_MASK0f);if((l128 = _ipmask2pfx(l133,
&l120))<0){return l128;}l136 = soc_mem_field32_get(l2,l24,entry,
IP_ADDR_MASK1f);if((l128 = _ipmask2pfx(l136,&l164))<0){return l128;}if(l34){
soc_mem_field32_set(l2,l162,l159,IP_LENGTH0f,l120+l164);soc_mem_field32_set(
l2,l162,l159,IP_LENGTH1f,l120+l164);}else{soc_mem_field32_set(l2,l162,l159,
IP_LENGTH0f,l120);soc_mem_field32_set(l2,l162,l159,IP_LENGTH1f,l164);}l133 = 
soc_mem_field32_get(l2,l24,entry,IP_ADDR0f);soc_mem_field32_set(l2,l162,l159,
IP_ADDR0f,l133);l133 = soc_mem_field32_get(l2,l24,entry,IP_ADDR1f);
soc_mem_field32_set(l2,l162,l159,IP_ADDR1f,l133);l133 = soc_mem_field32_get(
l2,l24,entry,ENTRY_TYPE0f);soc_mem_field32_set(l2,l162,l159,ENTRY_TYPE0f,l133
);l133 = soc_mem_field32_get(l2,l24,entry,ENTRY_TYPE1f);soc_mem_field32_set(
l2,l162,l159,ENTRY_TYPE1f,l133);if(!l34){sal_memcpy(&l163,entry,sizeof(l163))
;l128 = soc_alpm_lpm_vrf_get(l2,(void*)&l163,&l28,&l120);SOC_IF_ERROR_RETURN(
l128);SOC_IF_ERROR_RETURN(soc_alpm_lpm_ip4entry1_to_0(l2,&l163,&l163,
PRESERVE_HIT));l128 = soc_alpm_lpm_vrf_get(l2,(void*)&l163,&l165,&l120);
SOC_IF_ERROR_RETURN(l128);}else{l128 = soc_alpm_lpm_vrf_get(l2,entry,&l28,&
l120);}if(SOC_URPF_STATUS_GET(l2)){if(l160>= (soc_mem_index_count(l2,
L3_DEFIPm)>>1)){l161 = 1;}}switch(l28){case SOC_L3_VRF_OVERRIDE:
soc_mem_field32_set(l2,l162,l159,VALID0f,0);l11 = 0;break;case
SOC_L3_VRF_GLOBAL:l11 = l161?1:0;break;default:l11 = l161?3:2;break;}
soc_mem_field32_set(l2,l162,l159,DB_TYPE0f,l11);if(!l34){switch(l165){case
SOC_L3_VRF_OVERRIDE:soc_mem_field32_set(l2,l162,l159,VALID1f,0);l11 = 0;break
;case SOC_L3_VRF_GLOBAL:l11 = l161?1:0;break;default:l11 = l161?3:2;break;}
soc_mem_field32_set(l2,l162,l159,DB_TYPE1f,l11);}else{if(l28 == 
SOC_L3_VRF_OVERRIDE){soc_mem_field32_set(l2,l162,l159,VALID1f,0);}
soc_mem_field32_set(l2,l162,l159,DB_TYPE1f,l11);}if(l161){l133 = 
soc_mem_field32_get(l2,l24,entry,ALG_BKT_PTR0f);if(l133){l133+= 
SOC_ALPM_BUCKET_COUNT(l2);soc_mem_field32_set(l2,l24,entry,ALG_BKT_PTR0f,l133
);}if(!l34){l133 = soc_mem_field32_get(l2,l24,entry,ALG_BKT_PTR1f);if(l133){
l133+= SOC_ALPM_BUCKET_COUNT(l2);soc_mem_field32_set(l2,l24,entry,
ALG_BKT_PTR1f,l133);}}}return SOC_E_NONE;}static int l166(int l2,int l167,int
index,int l168,void*entry){defip_aux_table_entry_t l159;l168 = 
soc_alpm_physical_idx(l2,L3_DEFIPm,l168,1);SOC_IF_ERROR_RETURN(l157(l2,l168,
entry,(void*)&l159,index));SOC_IF_ERROR_RETURN(WRITE_L3_DEFIPm(l2,
MEM_BLOCK_ANY,index,entry));index = soc_alpm_physical_idx(l2,L3_DEFIPm,index,
1);SOC_IF_ERROR_RETURN(WRITE_L3_DEFIP_AUX_TABLEm(l2,MEM_BLOCK_ANY,index,&l159
));return SOC_E_NONE;}int _soc_alpm_insert_in_bkt(int l2,soc_mem_t l24,int
bucket_index,int l138,void*l139,uint32*l15,int*l121,int l34){int l128;l128 = 
soc_mem_alpm_insert(l2,l24,bucket_index,MEM_BLOCK_ANY,l138,l139,l15,l121);if(
l128 == SOC_E_FULL){if(SOC_ALPM_V6_SCALE_CHECK(l2,l34)){return
soc_mem_alpm_insert(l2,l24,bucket_index+1,MEM_BLOCK_ANY,l138,l139,l15,l121);}
}return l128;}int _soc_alpm_mem_index(int l2,soc_mem_t l24,int bucket_index,
int l169,uint32 l138,int*l170){int l171,l172 = 0;int l173[4] = {0};int l174 = 
0;int l175 = 0;int l176;int l34 = 0;int l177 = 6;switch(l24){case
L3_DEFIP_ALPM_IPV6_64m:l177 = 4;l34 = 1;break;case L3_DEFIP_ALPM_IPV6_128m:
l177 = 2;l34 = 1;break;default:break;}if(SOC_ALPM_V6_SCALE_CHECK(l2,l34)){if(
l169>= ALPM_RAW_BKT_COUNT*l177){bucket_index++;l169-= ALPM_RAW_BKT_COUNT*l177
;}}l176 = (4)-_shr_popcount(l138&((1<<(4))-1));if(bucket_index>= (1<<16)||
l169>= l176*l177){return SOC_E_FULL;}l175 = l169%l177;for(l171 = 0;l171<(4);
l171++){if((1<<l171)&l138){continue;}l173[l172++] = l171;}l174 = l173[l169/
l177];*l170 = (l175<<16)|(bucket_index<<2)|(l174);return SOC_E_NONE;}int l178
(int l21,int l34){if(SOC_ALPM_V6_SCALE_CHECK(l21,l34)){return
ALPM_RAW_BKT_COUNT_DW;}else{return ALPM_RAW_BKT_COUNT;}}int l179(soc_mem_t l24
,int index){uint32 l180 = 0x7;if(l24 == L3_DEFIP_ALPM_IPV6_128m){l180 = 0x3;}
return((uint32)index>>16)&l180;}int l181(int l21,int l34,int index){return
index%l178(l21,l34);}void _soc_alpm_raw_mem_read(int l21,soc_mem_t l24,void*
l182,int index,void*entry){int l10 = 1;int l183;int l184;
defip_alpm_raw_entry_t*l185;soc_mem_info_t l186;soc_field_info_t l187;int l188
= soc_mem_entry_bits(l21,l24)-1;if(l24 == L3_DEFIP_ALPM_IPV4m){l10 = 0;}l183 = 
l181(l21,l10,index);l184 = l179(l24,index);l185 = &((defip_alpm_raw_entry_t*)
l182)[l183];l186.flags = 0;l186.bytes = sizeof(defip_alpm_raw_entry_t);l187.
flags = SOCF_LE;l187.bp = l188*l184;l187.len = l188;(void)
soc_meminfo_fieldinfo_field_get((void*)l185,&l186,&l187,entry);}void
_soc_alpm_raw_mem_write(int l21,soc_mem_t l24,void*l182,int index,void*entry)
{int l10 = 1;int l183;int l184;defip_alpm_raw_entry_t*l185;soc_mem_info_t l186
;soc_field_info_t l187;int l188 = soc_mem_entry_bits(l21,l24)-1;if(l24 == 
L3_DEFIP_ALPM_IPV4m){l10 = 0;}(void)soc_mem_cache_invalidate(l21,l24,
MEM_BLOCK_ANY,index);l183 = l181(l21,l10,index);l184 = l179(l24,index);l185 = 
&((defip_alpm_raw_entry_t*)l182)[l183];l186.flags = 0;l186.bytes = sizeof(
defip_alpm_raw_entry_t);l187.bp = l188*l184;l187.len = l188;l187.flags = 
SOCF_LE;(void)soc_meminfo_fieldinfo_field_set((void*)l185,&l186,&l187,entry);
}int l189(int l2,soc_mem_t l24,int index){return SOC_ALPM_BKT_ENTRY_TO_IDX(
index%(1<<16));}int _soc_alpm_raw_bucket_read(int l2,soc_mem_t l24,int
bucket_index,void*l185,void*l190){int l171,l34 = 1;int l183,l191;
defip_alpm_raw_entry_t*l192 = l185;defip_alpm_raw_entry_t*l193 = l190;if(l24
== L3_DEFIP_ALPM_IPV4m){l34 = 0;}l183 = SOC_ALPM_BKT_IDX_TO_ENTRY(
bucket_index);for(l171 = 0;l171<l178(l2,l34);l171++){SOC_IF_ERROR_RETURN(
soc_mem_read(l2,L3_DEFIP_ALPM_RAWm,MEM_BLOCK_ANY,l183+l171,&l192[l171]));if(
SOC_URPF_STATUS_GET(l2)){l191 = _soc_alpm_rpf_entry(l2,l183+l171);
SOC_IF_ERROR_RETURN(soc_mem_read(l2,L3_DEFIP_ALPM_RAWm,MEM_BLOCK_ANY,l191,&
l193[l171]));}}return SOC_E_NONE;}int _soc_alpm_raw_bucket_write(int l2,
soc_mem_t l24,int bucket_index,uint32 l138,void*l185,void*l190,int l194){int
l171 = 0,l195,l34 = 1;int l183,l191,l196;defip_alpm_raw_entry_t*l192 = l185;
defip_alpm_raw_entry_t*l193 = l190;int l197 = 6;switch(l24){case
L3_DEFIP_ALPM_IPV4m:l197 = 6;l34 = 0;break;case L3_DEFIP_ALPM_IPV6_64m:l197 = 
4;l34 = 1;break;case L3_DEFIP_ALPM_IPV6_128m:l197 = 2;l34 = 1;break;default:
break;}l183 = SOC_ALPM_BKT_IDX_TO_ENTRY(bucket_index);if(l194 == -1){l196 = 
l178(l2,l34);}else{l196 = (l194/l197)+1;}for(l195 = 0;l195<l178(l2,l34);l195
++){if((1<<(l195%(4)))&l138){continue;}SOC_IF_ERROR_RETURN(soc_mem_write(l2,
L3_DEFIP_ALPM_RAWm,MEM_BLOCK_ANY,l183+l195,&l192[l195]));
_soc_trident2_alpm_bkt_view_set(l2,l183+l195,l24);if(SOC_URPF_STATUS_GET(l2))
{l191 = _soc_alpm_rpf_entry(l2,l183+l195);_soc_trident2_alpm_bkt_view_set(l2,
l191,l24);SOC_IF_ERROR_RETURN(soc_mem_write(l2,L3_DEFIP_ALPM_RAWm,
MEM_BLOCK_ANY,l191,&l193[l195]));}if(++l171 == l196){break;}}return SOC_E_NONE
;}void _soc_alpm_raw_parity_set(int l2,soc_mem_t l24,void*l139){int l171,l198
,l199 = 0;uint32*entry = l139;l198 = soc_mem_entry_words(l2,l24);for(l171 = 0
;l171<l198;l171++){l199+= _shr_popcount(entry[l171]);}if(l199&0x1){
soc_mem_field32_set(l2,l24,l139,EVEN_PARITYf,1);}}static int l200(int l2,void
*l8,soc_mem_t l24,void*l139,void*l151,int*l16,int bucket_index,int l143){
alpm_pivot_t*l146,*l201,*l202;defip_aux_scratch_entry_t l13;uint32 l15[
SOC_MAX_MEM_FIELD_WORDS];uint32 prefix[5],l203,l141;uint32 l204[5];int l34,
l28,l149;int l121;int l128 = SOC_E_NONE,l205;uint32 l11,l138,l156 = 0;int l142
=0;int l155;int l206 = 0;trie_t*trie,*l207;trie_node_t*l208,*l209 = NULL,*
l145 = NULL;payload_t*l210,*l211,*l212;defip_entry_t lpm_entry;
alpm_bucket_handle_t*l213;int l171,l214 = -1,l26 = 0;alpm_mem_prefix_array_t
l153;defip_alpm_ipv4_entry_t l215,l216;defip_alpm_ipv6_64_entry_t l217,l218;
void*l219,*l220;int*l124 = NULL;trie_t*l144 = NULL;int l221;
defip_alpm_raw_entry_t*l222 = NULL;defip_alpm_raw_entry_t*l185;
defip_alpm_raw_entry_t*l190;defip_alpm_raw_entry_t*l223;
defip_alpm_raw_entry_t*l224;l34 = soc_mem_field32_get(l2,L3_DEFIPm,l8,MODE0f)
;if(l34){if(!(l34 = soc_mem_field32_get(l2,L3_DEFIPm,l8,MODE1f))){return(
SOC_E_PARAM);}}SOC_IF_ERROR_RETURN(soc_alpm_lpm_vrf_get(l2,l8,&l149,&l28));if
(l28 == SOC_VRF_MAX(l2)+1){l11 = 0;SOC_ALPM_GET_GLOBAL_BANK_DISABLE(l2,l138);
}else{l11 = 2;SOC_ALPM_GET_VRF_BANK_DISABLE(l2,l138);}l24 = (l34)?
L3_DEFIP_ALPM_IPV6_64m:L3_DEFIP_ALPM_IPV4m;l219 = ((l34)?((uint32*)&(l217)):(
(uint32*)&(l215)));l220 = ((l34)?((uint32*)&(l218)):((uint32*)&(l216)));l128 = 
l134(l2,l8,prefix,&l141,&l26);if(SOC_FAILURE(l128)){LOG_ERROR(BSL_LS_SOC_ALPM
,(BSL_META_U(l2,"_soc_alpm_insert: prefix create failed\n")));return l128;}
sal_memset(&l13,0,sizeof(defip_aux_scratch_entry_t));SOC_IF_ERROR_RETURN(l9(
l2,l8,l34,l11,0,&l13));if(bucket_index == 0){l128 = l140(l2,prefix,l141,l34,
l28,&l142,&l143,&bucket_index);SOC_IF_ERROR_RETURN(l128);if(l142 == 0){
LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l2,"_soc_alpm_insert: "
"Could not find bucket to insert prefix\n")));return SOC_E_NOT_FOUND;}}l128 = 
_soc_alpm_insert_in_bkt(l2,l24,bucket_index,l138,l139,l15,&l121,l34);if(l128
== SOC_E_NONE){*l16 = l121;if(SOC_URPF_STATUS_GET(l2)){l205 = soc_mem_write(
l2,l24,MEM_BLOCK_ANY,_soc_alpm_rpf_entry(l2,l121),l151);if(SOC_FAILURE(l205))
{return l205;}}}if(l128 == SOC_E_FULL){l206 = 1;}l146 = ALPM_TCAM_PIVOT(l2,
l143);trie = PIVOT_BUCKET_TRIE(l146);l202 = l146;l210 = sal_alloc(sizeof(
payload_t),"Payload for Key");if(l210 == NULL){LOG_ERROR(BSL_LS_SOC_ALPM,(
BSL_META_U(l2,"_soc_alpm_insert: Unable to allocate memory for "
"trie node \n")));return SOC_E_MEMORY;}l211 = sal_alloc(sizeof(payload_t),
"Payload for pfx trie key");if(l211 == NULL){LOG_ERROR(BSL_LS_SOC_ALPM,(
BSL_META_U(l2,"_soc_alpm_insert: Unable to allocate memory for "
"pfx trie node \n")));sal_free(l210);return SOC_E_MEMORY;}sal_memset(l210,0,
sizeof(*l210));sal_memset(l211,0,sizeof(*l211));l210->key[0] = prefix[0];l210
->key[1] = prefix[1];l210->key[2] = prefix[2];l210->key[3] = prefix[3];l210->
key[4] = prefix[4];l210->len = l141;l210->index = l121;sal_memcpy(l211,l210,
sizeof(*l210));l211->bkt_ptr = l210;l128 = trie_insert(trie,prefix,NULL,l141,
(trie_node_t*)l210);if(SOC_FAILURE(l128)){goto l225;}if(l34){l207 = 
VRF_PREFIX_TRIE_IPV6(l2,l28);}else{l207 = VRF_PREFIX_TRIE_IPV4(l2,l28);}if(!
l26){l128 = trie_insert(l207,prefix,NULL,l141,(trie_node_t*)l211);}else{l145 = 
NULL;l128 = trie_find_lpm(l207,0,0,&l145);l212 = (payload_t*)l145;if(
SOC_SUCCESS(l128)){l212->bkt_ptr = l210;}}l203 = l141;if(SOC_FAILURE(l128)){
goto l226;}if(l206){l128 = alpm_bucket_assign(l2,&bucket_index,l34);if(
SOC_FAILURE(l128)){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"_soc_alpm_insert: Unable to allocate""new bucket for split\n")));
bucket_index = -1;goto l227;}l128 = trie_split(trie,l34?_MAX_KEY_LEN_144_:
_MAX_KEY_LEN_48_,FALSE,l204,&l141,&l208,NULL,FALSE);if(SOC_FAILURE(l128)){
LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"_soc_alpm_insert: Could not split bucket""for prefix 0x%08x 0x%08x\n"),
prefix[0],prefix[1]));goto l227;}l145 = NULL;l128 = trie_find_lpm(l207,l204,
l141,&l145);l212 = (payload_t*)l145;if(SOC_FAILURE(l128)){LOG_ERROR(
BSL_LS_SOC_ALPM,(BSL_META_U(l2,"unit %d Unable to find lpm for pivot: "
"0x%08x 0x%08x\n 0x%08x 0x%08x 0x%08x length: %d\n"),l2,l204[0],l204[1],l204[
2],l204[3],l204[4],l141));goto l227;}if(l212->bkt_ptr){if(l212->bkt_ptr == 
l210){sal_memcpy(l219,l139,l34?sizeof(defip_alpm_ipv6_64_entry_t):sizeof(
defip_alpm_ipv4_entry_t));}else{l128 = soc_mem_read(l2,l24,MEM_BLOCK_ANY,((
payload_t*)l212->bkt_ptr)->index,l219);}if(SOC_FAILURE(l128)){goto l227;}l128
= l27(l2,l219,l24,l34,l149,bucket_index,0,&lpm_entry);if(SOC_FAILURE(l128)){
goto l227;}l156 = ((payload_t*)(l212->bkt_ptr))->len;}else{l128 = 
soc_mem_read(l2,L3_DEFIPm,MEM_BLOCK_ANY,soc_alpm_logical_idx(l2,L3_DEFIPm,
l143>>1,1),&lpm_entry);if((!l34)&&(l143&1)){l128 = 
soc_alpm_lpm_ip4entry1_to_0(l2,&lpm_entry,&lpm_entry,0);}}l213 = sal_alloc(
sizeof(alpm_bucket_handle_t),"ALPM Bucket Handle");if(l213 == NULL){LOG_ERROR
(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"_soc_alpm_insert: Unable to allocate memory ""for PIVOT trie node \n")));
l128 = SOC_E_MEMORY;goto l227;}sal_memset(l213,0,sizeof(*l213));l146 = 
sal_alloc(sizeof(alpm_pivot_t),"Payload for new Pivot");if(l146 == NULL){
LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"_soc_alpm_insert: Unable to allocate memory ""for PIVOT trie node \n")));
l128 = SOC_E_MEMORY;goto l227;}sal_memset(l146,0,sizeof(*l146));
PIVOT_BUCKET_HANDLE(l146) = l213;if(l34){l128 = trie_init(_MAX_KEY_LEN_144_,&
PIVOT_BUCKET_TRIE(l146));}else{l128 = trie_init(_MAX_KEY_LEN_48_,&
PIVOT_BUCKET_TRIE(l146));}PIVOT_BUCKET_TRIE(l146)->trie = l208;
PIVOT_BUCKET_INDEX(l146) = bucket_index;PIVOT_BUCKET_VRF(l146) = l28;
PIVOT_BUCKET_IPV6(l146) = l34;PIVOT_BUCKET_DEF(l146) = FALSE;l146->key[0] = 
l204[0];l146->key[1] = l204[1];l146->len = l141;l146->key[2] = l204[2];l146->
key[3] = l204[3];l146->key[4] = l204[4];if(l34){l144 = VRF_PIVOT_TRIE_IPV6(l2
,l28);}else{l144 = VRF_PIVOT_TRIE_IPV4(l2,l28);}do{if(!(l34)){l204[0] = (((32
-l141) == 32)?0:(l204[1])<<(32-l141));l204[1] = 0;}else{int l228 = 64-l141;
int l229;if(l228<32){l229 = l204[3]<<l228;l229|= (((32-l228) == 32)?0:(l204[4
])>>(32-l228));l204[0] = l204[4]<<l228;l204[1] = l229;l204[2] = l204[3] = 
l204[4] = 0;}else{l204[1] = (((l228-32) == 32)?0:(l204[4])<<(l228-32));l204[0
] = l204[2] = l204[3] = l204[4] = 0;}}}while(0);l30(l2,l204,l141,l28,l34,&
lpm_entry,0,0);soc_L3_DEFIPm_field32_set(l2,&lpm_entry,ALG_BKT_PTR0f,
bucket_index);sal_memset(&l153,0,sizeof(l153));l128 = trie_traverse(
PIVOT_BUCKET_TRIE(l146),alpm_mem_prefix_array_cb,&l153,_TRIE_INORDER_TRAVERSE
);if(SOC_FAILURE(l128)){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"_soc_alpm_insert: Bucket split failed"
"for prefix 0x%08x 0x%08x 0x%08x 0x%08x\n"),prefix[1],prefix[2],prefix[3],
prefix[4]));goto l227;}l124 = sal_alloc(sizeof(*l124)*l153.count,
"Temp storage for location of prefixes in new bucket");if(l124 == NULL){l128 = 
SOC_E_MEMORY;goto l227;}sal_memset(l124,-1,sizeof(*l124)*l153.count);l221 = 
sizeof(defip_alpm_raw_entry_t)*ALPM_RAW_BKT_COUNT_DW;l222 = sal_alloc(4*l221,
"Raw memory buffer");if(l222 == NULL){l128 = SOC_E_MEMORY;goto l227;}
sal_memset(l222,0,4*l221);l185 = (defip_alpm_raw_entry_t*)l222;l190 = (
defip_alpm_raw_entry_t*)((uint8*)l185+l221);l223 = (defip_alpm_raw_entry_t*)(
(uint8*)l190+l221);l224 = (defip_alpm_raw_entry_t*)((uint8*)l223+l221);l128 = 
_soc_alpm_raw_bucket_read(l2,l24,PIVOT_BUCKET_INDEX(l202),(void*)l185,(void*)
l190);if(SOC_FAILURE(l128)){goto l227;}for(l171 = 0;l171<l153.count;l171++){
payload_t*l120 = l153.prefix[l171];if(l120->index>0){_soc_alpm_raw_mem_read(
l2,l24,l185,l120->index,l219);_soc_alpm_raw_mem_write(l2,l24,l185,l120->index
,soc_mem_entry_null(l2,l24));if(SOC_URPF_STATUS_GET(l2)){
_soc_alpm_raw_mem_read(l2,l24,l190,_soc_alpm_rpf_entry(l2,l120->index),l220);
_soc_alpm_raw_mem_write(l2,l24,l190,_soc_alpm_rpf_entry(l2,l120->index),
soc_mem_entry_null(l2,l24));}l128 = _soc_alpm_mem_index(l2,l24,bucket_index,
l171,l138,&l121);if(SOC_SUCCESS(l128)){_soc_alpm_raw_mem_write(l2,l24,l223,
l121,l219);if(SOC_URPF_STATUS_GET(l2)){_soc_alpm_raw_mem_write(l2,l24,l224,
_soc_alpm_rpf_entry(l2,l121),l220);}}}else{l128 = _soc_alpm_mem_index(l2,l24,
bucket_index,l171,l138,&l121);if(SOC_SUCCESS(l128)){l214 = l171;*l16 = l121;
_soc_alpm_raw_parity_set(l2,l24,l139);_soc_alpm_raw_mem_write(l2,l24,l223,
l121,l139);if(SOC_URPF_STATUS_GET(l2)){_soc_alpm_raw_parity_set(l2,l24,l151);
_soc_alpm_raw_mem_write(l2,l24,l224,_soc_alpm_rpf_entry(l2,l121),l151);}}}
l124[l171] = l121;}l128 = _soc_alpm_raw_bucket_write(l2,l24,bucket_index,l138
,(void*)l223,(void*)l224,l153.count);if(SOC_FAILURE(l128)){goto l230;}l128 = 
l5(l2,&lpm_entry,&l155);if(SOC_FAILURE(l128)){LOG_ERROR(BSL_LS_SOC_ALPM,(
BSL_META_U(l2,"_soc_alpm_insert: Unable to add new""pivot to tcam\n")));if(
l128 == SOC_E_FULL){VRF_PIVOT_FULL_INC(l2,l28,l34);}goto l230;}l155 = 
soc_alpm_physical_idx(l2,L3_DEFIPm,l155,l34);l128 = l154(l2,l155,l34,l156);if
(SOC_FAILURE(l128)){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"_soc_alpm_insert: Unable to init bpm_len ""for index %d\n"),l155));goto l231
;}l128 = trie_insert(l144,l146->key,NULL,l146->len,(trie_node_t*)l146);if(
SOC_FAILURE(l128)){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"failed to insert into pivot trie\n")));goto l231;}ALPM_TCAM_PIVOT(l2,l155<<(
l34?1:0)) = l146;PIVOT_TCAM_INDEX(l146) = l155<<(l34?1:0);VRF_PIVOT_REF_INC(
l2,l28,l34);for(l171 = 0;l171<l153.count;l171++){l153.prefix[l171]->index = 
l124[l171];}sal_free(l124);l128 = _soc_alpm_raw_bucket_write(l2,l24,
PIVOT_BUCKET_INDEX(l202),l138,(void*)l185,(void*)l190,-1);if(SOC_FAILURE(l128
)){goto l231;}if(l214 == -1){l128 = _soc_alpm_insert_in_bkt(l2,l24,
PIVOT_BUCKET_INDEX(l202),l138,l139,l15,&l121,l34);if(SOC_FAILURE(l128)){
LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"_soc_alpm_insert: Could not insert new ""prefix into trie after split\n")));
goto l231;}if(SOC_URPF_STATUS_GET(l2)){l128 = soc_mem_write(l2,l24,
MEM_BLOCK_ANY,_soc_alpm_rpf_entry(l2,l121),l151);}*l16 = l121;l210->index = 
l121;}sal_free(l222);PIVOT_BUCKET_ENT_CNT_UPDATE(l146);VRF_BUCKET_SPLIT_INC(
l2,l28,l34);}VRF_TRIE_ROUTES_INC(l2,l28,l34);if(l26){sal_free(l211);}
SOC_IF_ERROR_RETURN(_soc_alpm_aux_op(l2,DELETE_PROPAGATE,&l13,TRUE,&l142,&
l143,&bucket_index));SOC_IF_ERROR_RETURN(_soc_alpm_aux_op(l2,INSERT_PROPAGATE
,&l13,FALSE,&l142,&l143,&bucket_index));if(SOC_URPF_STATUS_GET(l2)){l141 = 
soc_mem_field32_get(l2,L3_DEFIP_AUX_SCRATCHm,&l13,DB_TYPEf);l141+= 1;
soc_mem_field32_set(l2,L3_DEFIP_AUX_SCRATCHm,&l13,DB_TYPEf,l141);
SOC_IF_ERROR_RETURN(_soc_alpm_aux_op(l2,DELETE_PROPAGATE,&l13,TRUE,&l142,&
l143,&bucket_index));SOC_IF_ERROR_RETURN(_soc_alpm_aux_op(l2,INSERT_PROPAGATE
,&l13,FALSE,&l142,&l143,&bucket_index));}PIVOT_BUCKET_ENT_CNT_UPDATE(l202);
return l128;l231:l205 = l7(l2,&lpm_entry);if(SOC_FAILURE(l205)){LOG_ERROR(
BSL_LS_SOC_ALPM,(BSL_META_U(l2,"_soc_alpm_insert: Failure to free new prefix"
"at %d\n"),soc_alpm_logical_idx(l2,L3_DEFIPm,l155,l34)));}if(ALPM_TCAM_PIVOT(
l2,l155<<(l34?1:0))!= NULL){l205 = trie_delete(l144,l146->key,l146->len,&l209
);if(SOC_FAILURE(l205)){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"_soc_alpm_insert: trie delete failure ""in bkt move rollback\n")));}}
ALPM_TCAM_PIVOT(l2,l155<<(l34?1:0)) = NULL;VRF_PIVOT_REF_DEC(l2,l28,l34);l230
:l201 = l202;for(l171 = 0;l171<l153.count;l171++){payload_t*prefix = l153.
prefix[l171];if(l124[l171]!= -1){if(l34){sal_memset(l219,0,sizeof(
defip_alpm_ipv6_64_entry_t));}else{sal_memset(l219,0,sizeof(
defip_alpm_ipv4_entry_t));}l205 = soc_mem_write(l2,l24,MEM_BLOCK_ANY,l124[
l171],l219);_soc_trident2_alpm_bkt_view_set(l2,l124[l171],INVALIDm);if(
SOC_FAILURE(l205)){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"_soc_alpm_insert: mem write failure""in bkt move rollback\n")));}if(
SOC_URPF_STATUS_GET(l2)){l205 = soc_mem_write(l2,l24,MEM_BLOCK_ANY,
_soc_alpm_rpf_entry(l2,l124[l171]),l219);_soc_trident2_alpm_bkt_view_set(l2,
_soc_alpm_rpf_entry(l2,l124[l171]),INVALIDm);if(SOC_FAILURE(l205)){LOG_ERROR(
BSL_LS_SOC_ALPM,(BSL_META_U(l2,"_soc_alpm_insert: urpf mem write "
"failure in bkt move rollback\n")));}}}l209 = NULL;l205 = trie_delete(
PIVOT_BUCKET_TRIE(l146),prefix->key,prefix->len,&l209);l210 = (payload_t*)
l209;if(SOC_FAILURE(l205)){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"_soc_alpm_insert: trie delete failure""in bkt move rollback\n")));}if(prefix
->index>0){l205 = trie_insert(PIVOT_BUCKET_TRIE(l201),prefix->key,NULL,prefix
->len,(trie_node_t*)l210);if(SOC_FAILURE(l205)){LOG_ERROR(BSL_LS_SOC_ALPM,(
BSL_META_U(l2,"_soc_alpm_insert: trie reinsert failure"
"in bkt move rollback\n")));}}else{if(l210!= NULL){sal_free(l210);}}}if(l214
== -1){l209 = NULL;l205 = trie_delete(PIVOT_BUCKET_TRIE(l201),prefix,l203,&
l209);l210 = (payload_t*)l209;if(SOC_FAILURE(l205)){LOG_ERROR(BSL_LS_SOC_ALPM
,(BSL_META_U(l2,"_soc_alpm_insert: expected to clear prefix"
" 0x%08x 0x%08x\n from old trie. Failed\n"),prefix[0],prefix[1]));}if(l210!= 
NULL){sal_free(l210);}}l205 = alpm_bucket_release(l2,PIVOT_BUCKET_INDEX(l146)
,l34);if(SOC_FAILURE(l205)){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"_soc_alpm_insert: new bucket release failure: %d\n"),PIVOT_BUCKET_INDEX(l146
)));}trie_destroy(PIVOT_BUCKET_TRIE(l146));sal_free(l213);sal_free(l146);
sal_free(l124);sal_free(l222);l209 = NULL;l205 = trie_delete(l207,prefix,l203
,&l209);l211 = (payload_t*)l209;if(SOC_FAILURE(l205)){LOG_ERROR(
BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"_soc_alpm_insert: failed to delete new prefix"
"0x%08x 0x%08x from pfx trie\n"),prefix[0],prefix[1]));}if(l211){sal_free(
l211);}return l128;l227:if(l124!= NULL){sal_free(l124);}if(l222!= NULL){
sal_free(l222);}l209 = NULL;(void)trie_delete(l207,prefix,l203,&l209);l211 = 
(payload_t*)l209;if(bucket_index!= -1){(void)alpm_bucket_release(l2,
bucket_index,l34);}l226:l209 = NULL;(void)trie_delete(trie,prefix,l203,&l209)
;l210 = (payload_t*)l209;l225:if(l210!= NULL){sal_free(l210);}if(l211!= NULL)
{sal_free(l211);}return l128;}static int l30(int l21,uint32*key,int len,int
l28,int l10,defip_entry_t*lpm_entry,int l31,int l32){uint32 l180;if(l32){
sal_memset(lpm_entry,0,sizeof(defip_entry_t));}
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l21,L3_DEFIPm)),(lpm_entry),
(l96[(l21)]->l76),(l28&SOC_VRF_MAX(l21)));if(l28 == (SOC_VRF_MAX(l21)+1)){
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l21,L3_DEFIPm)),(lpm_entry),
(l96[(l21)]->l78),(0));}else{soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO
(l21,L3_DEFIPm)),(lpm_entry),(l96[(l21)]->l78),(SOC_VRF_MAX(l21)));}if(l10){
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l21,L3_DEFIPm)),(lpm_entry),
(l96[(l21)]->l60),(key[0]));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(
l21,L3_DEFIPm)),(lpm_entry),(l96[(l21)]->l61),(key[1]));
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l21,L3_DEFIPm)),(lpm_entry),
(l96[(l21)]->l64),(1));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l21,
L3_DEFIPm)),(lpm_entry),(l96[(l21)]->l65),(1));
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l21,L3_DEFIPm)),(lpm_entry),
(l96[(l21)]->l77),(l28&SOC_VRF_MAX(l21)));if(l28 == (SOC_VRF_MAX(l21)+1)){
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l21,L3_DEFIPm)),(lpm_entry),
(l96[(l21)]->l79),(0));}else{soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO
(l21,L3_DEFIPm)),(lpm_entry),(l96[(l21)]->l79),(SOC_VRF_MAX(l21)));}
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l21,L3_DEFIPm)),(lpm_entry),
(l96[(l21)]->l75),(1));if(len>= 32){l180 = 0xffffffff;
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l21,L3_DEFIPm)),(lpm_entry),
(l96[(l21)]->l63),(l180));l180 = ~(((len-32) == 32)?0:(0xffffffff)>>(len-32))
;soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l21,L3_DEFIPm)),(lpm_entry)
,(l96[(l21)]->l62),(l180));}else{l180 = ~(0xffffffff>>len);
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l21,L3_DEFIPm)),(lpm_entry),
(l96[(l21)]->l63),(l180));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(
l21,L3_DEFIPm)),(lpm_entry),(l96[(l21)]->l62),(0));}}else{
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l21,L3_DEFIPm)),(lpm_entry),
(l96[(l21)]->l60),(key[0]));assert(len<= 32);l180 = (len == 32)?0xffffffff:~(
0xffffffff>>len);soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l21,
L3_DEFIPm)),(lpm_entry),(l96[(l21)]->l62),(l180));}
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l21,L3_DEFIPm)),(lpm_entry),
(l96[(l21)]->l74),(1));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l21,
L3_DEFIPm)),(lpm_entry),(l96[(l21)]->l66),((1<<soc_mem_field_length(l21,
L3_DEFIPm,MODE_MASK0f))-1));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(
l21,L3_DEFIPm)),(lpm_entry),(l96[(l21)]->l67),((1<<soc_mem_field_length(l21,
L3_DEFIPm,MODE_MASK1f))-1));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(
l21,L3_DEFIPm)),(lpm_entry),(l96[(l21)]->l92),((1<<soc_mem_field_length(l21,
L3_DEFIPm,ENTRY_TYPE_MASK0f))-1));soc_meminfo_fieldinfo_field32_set((&
SOC_MEM_INFO(l21,L3_DEFIPm)),(lpm_entry),(l96[(l21)]->l93),((1<<
soc_mem_field_length(l21,L3_DEFIPm,ENTRY_TYPE_MASK1f))-1));return(SOC_E_NONE)
;}int _soc_alpm_delete_in_bkt(int l2,soc_mem_t l24,int l232,int l138,void*
l233,uint32*l15,int*l121,int l34){int l128;l128 = soc_mem_alpm_delete(l2,l24,
l232,MEM_BLOCK_ALL,l138,l233,l15,l121);if(SOC_SUCCESS(l128)){return l128;}if(
SOC_ALPM_V6_SCALE_CHECK(l2,l34)){return soc_mem_alpm_delete(l2,l24,l232+1,
MEM_BLOCK_ALL,l138,l233,l15,l121);}return l128;}static int l234(int l2,void*
l8,int bucket_index,int l143,int l121){alpm_pivot_t*l146;
defip_alpm_ipv4_entry_t l215,l216;defip_alpm_ipv6_64_entry_t l217,l218;
defip_alpm_ipv4_entry_t l235,l236;defip_aux_scratch_entry_t l13;uint32 l15[
SOC_MAX_MEM_FIELD_WORDS];soc_mem_t l24;void*l219,*l233,*l220 = NULL;int l149;
int l10;int l128 = SOC_E_NONE,l205;uint32 l237[5],prefix[5];int l34,l28;
uint32 l141;int l232;uint32 l11,l138;int l142,l26 = 0;trie_t*trie,*l207;
uint32 l238;defip_entry_t lpm_entry,*l239;payload_t*l210 = NULL,*l240 = NULL,
*l212 = NULL;trie_node_t*l209 = NULL,*l145 = NULL;trie_t*l144 = NULL;l10 = 
l34 = soc_mem_field32_get(l2,L3_DEFIPm,l8,MODE0f);if(l34){if(!(l34 = 
soc_mem_field32_get(l2,L3_DEFIPm,l8,MODE1f))){return(SOC_E_PARAM);}}
SOC_IF_ERROR_RETURN(soc_alpm_lpm_vrf_get(l2,l8,&l149,&l28));if(l149!= 
SOC_L3_VRF_OVERRIDE){if(l28 == SOC_VRF_MAX(l2)+1){l11 = 0;
SOC_ALPM_GET_GLOBAL_BANK_DISABLE(l2,l138);}else{l11 = 2;
SOC_ALPM_GET_VRF_BANK_DISABLE(l2,l138);}l128 = l134(l2,l8,prefix,&l141,&l26);
if(SOC_FAILURE(l128)){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"_soc_alpm_delete: prefix create failed\n")));return l128;}if(!
soc_alpm_mode_get(l2)){if(l149!= SOC_L3_VRF_GLOBAL){if(VRF_TRIE_ROUTES_CNT(l2
,l28,l34)>1){if(l26){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"VRF %d: Cannot delete default ""route if other routes are present in "
"this mode"),l28));return SOC_E_PARAM;}}}l11 = 2;}l24 = (l34)?
L3_DEFIP_ALPM_IPV6_64m:L3_DEFIP_ALPM_IPV4m;l219 = ((l34)?((uint32*)&(l217)):(
(uint32*)&(l215)));l233 = ((l34)?((uint32*)&(l236)):((uint32*)&(l235)));
SOC_ALPM_LPM_LOCK(l2);if(bucket_index == 0){l128 = l147(l2,l8,l24,l219,&l143,
&bucket_index,&l121,TRUE);}else{l128 = l20(l2,l8,l219,0,l24,0,0);}sal_memcpy(
l233,l219,l34?sizeof(l217):sizeof(l215));if(SOC_FAILURE(l128)){
SOC_ALPM_LPM_UNLOCK(l2);LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"_soc_alpm_delete: Unable to find prefix for delete\n")));return l128;}l232 = 
bucket_index;l146 = ALPM_TCAM_PIVOT(l2,l143);trie = PIVOT_BUCKET_TRIE(l146);
l128 = trie_delete(trie,prefix,l141,&l209);l210 = (payload_t*)l209;if(l128!= 
SOC_E_NONE){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"_soc_alpm_delete: Error prefix not present in trie \n")));
SOC_ALPM_LPM_UNLOCK(l2);return l128;}if(l34){l207 = VRF_PREFIX_TRIE_IPV6(l2,
l28);}else{l207 = VRF_PREFIX_TRIE_IPV4(l2,l28);}if(l34){l144 = 
VRF_PIVOT_TRIE_IPV6(l2,l28);}else{l144 = VRF_PIVOT_TRIE_IPV4(l2,l28);}if(!l26
){l128 = trie_delete(l207,prefix,l141,&l209);l240 = (payload_t*)l209;if(
SOC_FAILURE(l128)){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"_soc_alpm_delete: Prefix not present in pfx""trie: 0x%08x 0x%08x\n"),prefix[
0],prefix[1]));goto l241;}l145 = NULL;l128 = trie_find_lpm(l207,prefix,l141,&
l145);l212 = (payload_t*)l145;if(SOC_SUCCESS(l128)){payload_t*l242 = (
payload_t*)(l212->bkt_ptr);if(l242!= NULL){l238 = l242->len;}else{l238 = 0;}}
else{LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"_soc_alpm_delete: Could not find replacement"
"bpm for prefix: 0x%08x 0x%08x\n"),prefix[0],prefix[1]));goto l243;}
sal_memcpy(l237,prefix,sizeof(prefix));do{if(!(l34)){l237[0] = (((32-l141) == 
32)?0:(l237[1])<<(32-l141));l237[1] = 0;}else{int l228 = 64-l141;int l229;if(
l228<32){l229 = l237[3]<<l228;l229|= (((32-l228) == 32)?0:(l237[4])>>(32-l228
));l237[0] = l237[4]<<l228;l237[1] = l229;l237[2] = l237[3] = l237[4] = 0;}
else{l237[1] = (((l228-32) == 32)?0:(l237[4])<<(l228-32));l237[0] = l237[2] = 
l237[3] = l237[4] = 0;}}}while(0);l128 = l30(l2,prefix,l238,l28,l10,&
lpm_entry,0,1);l205 = l147(l2,&lpm_entry,l24,l219,&l143,&bucket_index,&l121,
TRUE);(void)l27(l2,l219,l24,l10,l149,bucket_index,0,&lpm_entry);(void)l30(l2,
l237,l141,l28,l10,&lpm_entry,0,0);if(SOC_URPF_STATUS_GET(l2)){if(SOC_SUCCESS(
l128)){l220 = ((l34)?((uint32*)&(l218)):((uint32*)&(l216)));l205 = 
soc_mem_read(l2,l24,MEM_BLOCK_ANY,_soc_alpm_rpf_entry(l2,l121),l220);}}if((
l238 == 0)&&SOC_FAILURE(l205)){l239 = l34?VRF_TRIE_DEFAULT_ROUTE_IPV6(l2,l28)
:VRF_TRIE_DEFAULT_ROUTE_IPV4(l2,l28);sal_memcpy(&lpm_entry,l239,sizeof(
lpm_entry));l128 = l30(l2,l237,l141,l28,l10,&lpm_entry,0,1);}if(SOC_FAILURE(
l128)){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"_soc_alpm_delete: Could not find replacement"
" prefix for prefix: 0x%08x 0x%08x\n"),prefix[0],prefix[1]));goto l243;}l239 = 
&lpm_entry;}else{l145 = NULL;l128 = trie_find_lpm(l207,prefix,l141,&l145);
l212 = (payload_t*)l145;if(SOC_FAILURE(l128)){LOG_ERROR(BSL_LS_SOC_ALPM,(
BSL_META_U(l2,"_soc_alpm_delete: Could not find default "
"route in the trie for vrf %d\n"),l28));goto l241;}l212->bkt_ptr = NULL;l238 = 
0;l239 = l34?VRF_TRIE_DEFAULT_ROUTE_IPV6(l2,l28):VRF_TRIE_DEFAULT_ROUTE_IPV4(
l2,l28);}l128 = l9(l2,l239,l34,l11,l238,&l13);if(SOC_FAILURE(l128)){goto l243
;}l128 = _soc_alpm_aux_op(l2,DELETE_PROPAGATE,&l13,TRUE,&l142,&l143,&
bucket_index);if(SOC_FAILURE(l128)){goto l243;}if(SOC_URPF_STATUS_GET(l2)){
uint32 l133;if(l220!= NULL){l133 = soc_mem_field32_get(l2,
L3_DEFIP_AUX_SCRATCHm,&l13,DB_TYPEf);l133++;soc_mem_field32_set(l2,
L3_DEFIP_AUX_SCRATCHm,&l13,DB_TYPEf,l133);l133 = soc_mem_field32_get(l2,l24,
l220,SRC_DISCARDf);soc_mem_field32_set(l2,l24,&l13,SRC_DISCARDf,l133);l133 = 
soc_mem_field32_get(l2,l24,l220,DEFAULTROUTEf);soc_mem_field32_set(l2,l24,&
l13,DEFAULTROUTEf,l133);l128 = _soc_alpm_aux_op(l2,DELETE_PROPAGATE,&l13,TRUE
,&l142,&l143,&bucket_index);}if(SOC_FAILURE(l128)){goto l243;}}sal_free(l210)
;if(!l26){sal_free(l240);}PIVOT_BUCKET_ENT_CNT_UPDATE(l146);if((l146->len!= 0
)&&(trie->trie == NULL)){uint32 l244[5];sal_memcpy(l244,l146->key,sizeof(l244
));do{if(!(l10)){l244[0] = (((32-l146->len) == 32)?0:(l244[1])<<(32-l146->len
));l244[1] = 0;}else{int l228 = 64-l146->len;int l229;if(l228<32){l229 = l244
[3]<<l228;l229|= (((32-l228) == 32)?0:(l244[4])>>(32-l228));l244[0] = l244[4]
<<l228;l244[1] = l229;l244[2] = l244[3] = l244[4] = 0;}else{l244[1] = (((l228
-32) == 32)?0:(l244[4])<<(l228-32));l244[0] = l244[2] = l244[3] = l244[4] = 0
;}}}while(0);l30(l2,l244,l146->len,l28,l10,&lpm_entry,0,1);l128 = l7(l2,&
lpm_entry);if(SOC_FAILURE(l128)){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"_soc_alpm_delete: Unable to ""delete pivot 0x%08x 0x%08x \n"),l146->key[0],
l146->key[1]));}}l128 = _soc_alpm_delete_in_bkt(l2,l24,l232,l138,l233,l15,&
l121,l34);if(!SOC_SUCCESS(l128)){SOC_ALPM_LPM_UNLOCK(l2);l128 = SOC_E_FAIL;
return l128;}if(SOC_URPF_STATUS_GET(l2)){l128 = soc_mem_alpm_delete(l2,l24,
SOC_ALPM_RPF_BKT_IDX(l2,l232),MEM_BLOCK_ALL,l138,l233,l15,&l142);if(!
SOC_SUCCESS(l128)){SOC_ALPM_LPM_UNLOCK(l2);l128 = SOC_E_FAIL;return l128;}}if
((l146->len!= 0)&&(trie->trie == NULL)){l128 = alpm_bucket_release(l2,
PIVOT_BUCKET_INDEX(l146),l34);if(SOC_FAILURE(l128)){LOG_ERROR(BSL_LS_SOC_ALPM
,(BSL_META_U(l2,"_soc_alpm_delete: Unable to release""empty bucket: %d\n"),
PIVOT_BUCKET_INDEX(l146)));}l128 = trie_delete(l144,l146->key,l146->len,&l209
);if(SOC_FAILURE(l128)){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"could not delete pivot from pivot trie\n")));}trie_destroy(PIVOT_BUCKET_TRIE
(l146));sal_free(PIVOT_BUCKET_HANDLE(l146));sal_free(l146);
_soc_trident2_alpm_bkt_view_set(l2,l232<<2,INVALIDm);if(
SOC_ALPM_V6_SCALE_CHECK(l2,l34)){_soc_trident2_alpm_bkt_view_set(l2,(l232+1)
<<2,INVALIDm);}}}VRF_TRIE_ROUTES_DEC(l2,l28,l34);if(VRF_TRIE_ROUTES_CNT(l2,
l28,l34) == 0){l128 = l33(l2,l28,l34);}SOC_ALPM_LPM_UNLOCK(l2);return l128;
l243:l205 = trie_insert(l207,prefix,NULL,l141,(trie_node_t*)l240);if(
SOC_FAILURE(l205)){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"_soc_alpm_delete: Unable to reinsert""prefix 0x%08x 0x%08x into pfx trie\n")
,prefix[0],prefix[1]));}l241:l205 = trie_insert(trie,prefix,NULL,l141,(
trie_node_t*)l210);if(SOC_FAILURE(l205)){LOG_ERROR(BSL_LS_SOC_ALPM,(
BSL_META_U(l2,"_soc_alpm_delete: Unable to reinsert"
"prefix 0x%08x 0x%08x into bkt trie\n"),prefix[0],prefix[1]));}
SOC_ALPM_LPM_UNLOCK(l2);return l128;}int soc_alpm_init(int l2){int l171;int
l128 = SOC_E_NONE;l128 = l3(l2);SOC_IF_ERROR_RETURN(l128);l128 = l19(l2);for(
l171 = 0;l171<MAX_PIVOT_COUNT;l171++){ALPM_TCAM_PIVOT(l2,l171) = NULL;}if(
SOC_CONTROL(l2)->alpm_bulk_retry == NULL){SOC_CONTROL(l2)->alpm_bulk_retry = 
sal_sem_create("ALPM bulk retry",sal_sem_BINARY,0);}if(SOC_CONTROL(l2)->
alpm_lookup_retry == NULL){SOC_CONTROL(l2)->alpm_lookup_retry = 
sal_sem_create("ALPM lookup retry",sal_sem_BINARY,0);}if(SOC_CONTROL(l2)->
alpm_insert_retry == NULL){SOC_CONTROL(l2)->alpm_insert_retry = 
sal_sem_create("ALPM insert retry",sal_sem_BINARY,0);}if(SOC_CONTROL(l2)->
alpm_delete_retry == NULL){SOC_CONTROL(l2)->alpm_delete_retry = 
sal_sem_create("ALPM delete retry",sal_sem_BINARY,0);}l128 = 
soc_alpm_128_lpm_init(l2);SOC_IF_ERROR_RETURN(l128);return l128;}static int
l245(int l2){int l171,l128;alpm_pivot_t*l133;for(l171 = 0;l171<
MAX_PIVOT_COUNT;l171++){l133 = ALPM_TCAM_PIVOT(l2,l171);if(l133){if(
PIVOT_BUCKET_HANDLE(l133)){if(PIVOT_BUCKET_TRIE(l133)){l128 = trie_traverse(
PIVOT_BUCKET_TRIE(l133),alpm_delete_node_cb,NULL,_TRIE_INORDER_TRAVERSE);if(
SOC_SUCCESS(l128)){trie_destroy(PIVOT_BUCKET_TRIE(l133));}else{LOG_ERROR(
BSL_LS_SOC_ALPM,(BSL_META_U(l2,"Unable to clear trie state for unit %d\n"),l2
));return l128;}}sal_free(PIVOT_BUCKET_HANDLE(l133));}sal_free(
ALPM_TCAM_PIVOT(l2,l171));ALPM_TCAM_PIVOT(l2,l171) = NULL;}}for(l171 = 0;l171
<= SOC_VRF_MAX(l2)+1;l171++){l128 = trie_traverse(VRF_PREFIX_TRIE_IPV4(l2,
l171),alpm_delete_node_cb,NULL,_TRIE_INORDER_TRAVERSE);if(SOC_SUCCESS(l128)){
trie_destroy(VRF_PREFIX_TRIE_IPV4(l2,l171));}else{LOG_ERROR(BSL_LS_SOC_ALPM,(
BSL_META_U(l2,"unit: %d Unable to clear v4 pfx trie for vrf %d\n"),l2,l171));
return l128;}l128 = trie_traverse(VRF_PREFIX_TRIE_IPV6(l2,l171),
alpm_delete_node_cb,NULL,_TRIE_INORDER_TRAVERSE);if(SOC_SUCCESS(l128)){
trie_destroy(VRF_PREFIX_TRIE_IPV6(l2,l171));}else{LOG_ERROR(BSL_LS_SOC_ALPM,(
BSL_META_U(l2,"unit: %d Unable to clear v6 pfx trie for vrf %d\n"),l2,l171));
return l128;}if(VRF_TRIE_DEFAULT_ROUTE_IPV4(l2,l171)!= NULL){sal_free(
VRF_TRIE_DEFAULT_ROUTE_IPV4(l2,l171));}if(VRF_TRIE_DEFAULT_ROUTE_IPV6(l2,l171
)!= NULL){sal_free(VRF_TRIE_DEFAULT_ROUTE_IPV6(l2,l171));}sal_memset(&
alpm_vrf_handle[l2][l171],0,sizeof(alpm_vrf_handle_t));}sal_memset(&
alpm_vrf_handle[l2][MAX_VRF_ID],0,sizeof(alpm_vrf_handle_t));
VRF_TRIE_INIT_DONE(l2,MAX_VRF_ID,0,1);VRF_TRIE_INIT_DONE(l2,MAX_VRF_ID,1,1);
VRF_TRIE_INIT_DONE(l2,MAX_VRF_ID,2,1);if(SOC_ALPM_BUCKET_BMAP(l2)!= NULL){
sal_free(SOC_ALPM_BUCKET_BMAP(l2));}sal_memset(&soc_alpm_bucket[l2],0,sizeof(
soc_alpm_bucket_t));return SOC_E_NONE;}int soc_alpm_deinit(int l2){l4(l2);
SOC_IF_ERROR_RETURN(soc_alpm_128_lpm_deinit(l2));SOC_IF_ERROR_RETURN(
soc_alpm_128_state_clear(l2));SOC_IF_ERROR_RETURN(l245(l2));if(SOC_CONTROL(l2
)->alpm_bulk_retry){sal_sem_destroy(SOC_CONTROL(l2)->alpm_bulk_retry);
SOC_CONTROL(l2)->alpm_bulk_retry = NULL;}if(SOC_CONTROL(l2)->
alpm_lookup_retry == NULL){sal_sem_destroy(SOC_CONTROL(l2)->alpm_lookup_retry
);SOC_CONTROL(l2)->alpm_lookup_retry = NULL;}if(SOC_CONTROL(l2)->
alpm_insert_retry == NULL){sal_sem_destroy(SOC_CONTROL(l2)->alpm_insert_retry
);SOC_CONTROL(l2)->alpm_insert_retry = NULL;}if(SOC_CONTROL(l2)->
alpm_delete_retry == NULL){sal_sem_destroy(SOC_CONTROL(l2)->alpm_delete_retry
);SOC_CONTROL(l2)->alpm_delete_retry = NULL;}return SOC_E_NONE;}static int
l246(int l2,int l28,int l34){defip_entry_t*lpm_entry,l247;int l248;int index;
int l128 = SOC_E_NONE;uint32 key[2] = {0,0};uint32 l141;alpm_bucket_handle_t*
l213;alpm_pivot_t*l146;payload_t*l240;trie_t*l249;trie_t*l250 = NULL;if(l34 == 
0){trie_init(_MAX_KEY_LEN_48_,&VRF_PIVOT_TRIE_IPV4(l2,l28));l250 = 
VRF_PIVOT_TRIE_IPV4(l2,l28);}else{trie_init(_MAX_KEY_LEN_144_,&
VRF_PIVOT_TRIE_IPV6(l2,l28));l250 = VRF_PIVOT_TRIE_IPV6(l2,l28);}if(l34 == 0)
{trie_init(_MAX_KEY_LEN_48_,&VRF_PREFIX_TRIE_IPV4(l2,l28));l249 = 
VRF_PREFIX_TRIE_IPV4(l2,l28);}else{trie_init(_MAX_KEY_LEN_144_,&
VRF_PREFIX_TRIE_IPV6(l2,l28));l249 = VRF_PREFIX_TRIE_IPV6(l2,l28);}lpm_entry = 
sal_alloc(sizeof(defip_entry_t),"Default LPM entry");if(lpm_entry == NULL){
LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"soc_alpm_vrf_add: unable to allocate memory for ""IPv4 LPM entry\n")));
return SOC_E_MEMORY;}l30(l2,key,0,l28,l34,lpm_entry,0,1);if(l34 == 0){
VRF_TRIE_DEFAULT_ROUTE_IPV4(l2,l28) = lpm_entry;}else{
VRF_TRIE_DEFAULT_ROUTE_IPV6(l2,l28) = lpm_entry;}if(l28 == SOC_VRF_MAX(l2)+1)
{soc_L3_DEFIPm_field32_set(l2,lpm_entry,GLOBAL_ROUTE0f,1);}else{
soc_L3_DEFIPm_field32_set(l2,lpm_entry,DEFAULT_MISS0f,1);}l128 = 
alpm_bucket_assign(l2,&l248,l34);soc_L3_DEFIPm_field32_set(l2,lpm_entry,
ALG_BKT_PTR0f,l248);sal_memcpy(&l247,lpm_entry,sizeof(l247));l128 = l5(l2,&
l247,&index);l213 = sal_alloc(sizeof(alpm_bucket_handle_t),
"ALPM Bucket Handle");if(l213 == NULL){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(
l2,"soc_alpm_vrf_add: Unable to allocate memory for ""PIVOT trie node \n")));
return SOC_E_NONE;}sal_memset(l213,0,sizeof(*l213));l146 = sal_alloc(sizeof(
alpm_pivot_t),"Payload for Pivot");if(l146 == NULL){LOG_ERROR(BSL_LS_SOC_ALPM
,(BSL_META_U(l2,"soc_alpm_vrf_add: Unable to allocate memory for "
"PIVOT trie node \n")));sal_free(l213);return SOC_E_MEMORY;}l240 = sal_alloc(
sizeof(payload_t),"Payload for pfx trie key");if(l240 == NULL){LOG_ERROR(
BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"soc_alpm_vrf_add: Unable to allocate memory for ""pfx trie node \n")));
sal_free(l213);sal_free(l146);return SOC_E_MEMORY;}sal_memset(l146,0,sizeof(*
l146));sal_memset(l240,0,sizeof(*l240));l141 = 0;PIVOT_BUCKET_HANDLE(l146) = 
l213;if(l34){trie_init(_MAX_KEY_LEN_144_,&PIVOT_BUCKET_TRIE(l146));}else{
trie_init(_MAX_KEY_LEN_48_,&PIVOT_BUCKET_TRIE(l146));}PIVOT_BUCKET_INDEX(l146
) = l248;PIVOT_BUCKET_VRF(l146) = l28;PIVOT_BUCKET_IPV6(l146) = l34;
PIVOT_BUCKET_DEF(l146) = TRUE;l146->key[0] = l240->key[0] = key[0];l146->key[
1] = l240->key[1] = key[1];l146->len = l240->len = l141;l128 = trie_insert(
l249,key,NULL,l141,&(l240->node));if(SOC_FAILURE(l128)){sal_free(l240);
sal_free(l146);sal_free(l213);return l128;}l128 = trie_insert(l250,key,NULL,
l141,(trie_node_t*)l146);if(SOC_FAILURE(l128)){trie_node_t*l209 = NULL;(void)
trie_delete(l249,key,l141,&l209);sal_free(l240);sal_free(l146);sal_free(l213)
;return l128;}index = soc_alpm_physical_idx(l2,L3_DEFIPm,index,l34);if(l34 == 
0){ALPM_TCAM_PIVOT(l2,index) = l146;PIVOT_TCAM_INDEX(l146) = index;}else{
ALPM_TCAM_PIVOT(l2,index<<1) = l146;PIVOT_TCAM_INDEX(l146) = index<<1;}
VRF_PIVOT_REF_INC(l2,l28,l34);VRF_TRIE_INIT_DONE(l2,l28,l34,1);return l128;}
static int l33(int l2,int l28,int l34){defip_entry_t*lpm_entry;int l248;int
l130;int l128 = SOC_E_NONE;uint32 key[2] = {0,0},l251[SOC_MAX_MEM_FIELD_WORDS
];payload_t*l210;alpm_pivot_t*l252;trie_node_t*l209;trie_t*l249;trie_t*l250 = 
NULL;if(l34 == 0){lpm_entry = VRF_TRIE_DEFAULT_ROUTE_IPV4(l2,l28);}else{
lpm_entry = VRF_TRIE_DEFAULT_ROUTE_IPV6(l2,l28);}l248 = 
soc_L3_DEFIPm_field32_get(l2,lpm_entry,ALG_BKT_PTR0f);l128 = 
alpm_bucket_release(l2,l248,l34);_soc_trident2_alpm_bkt_view_set(l2,l248<<2,
INVALIDm);l128 = l18(l2,lpm_entry,(void*)l251,&l130);if(SOC_FAILURE(l128)){
LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"soc_alpm_vrf_delete: unable to get internal"" pivot idx for vrf %d/%d\n"),
l28,l34));l130 = -1;}l130 = soc_alpm_physical_idx(l2,L3_DEFIPm,l130,l34);if(
l34 == 0){l252 = ALPM_TCAM_PIVOT(l2,l130);}else{l252 = ALPM_TCAM_PIVOT(l2,
l130<<1);}l128 = l7(l2,lpm_entry);if(SOC_FAILURE(l128)){LOG_ERROR(
BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"soc_alpm_vrf_delete: unable to delete lpm entry "
" for internal default for vrf %d/%d\n"),l28,l34));}sal_free(lpm_entry);if(
l34 == 0){VRF_TRIE_DEFAULT_ROUTE_IPV4(l2,l28) = NULL;l249 = 
VRF_PREFIX_TRIE_IPV4(l2,l28);VRF_PREFIX_TRIE_IPV4(l2,l28) = NULL;}else{
VRF_TRIE_DEFAULT_ROUTE_IPV6(l2,l28) = NULL;l249 = VRF_PREFIX_TRIE_IPV6(l2,l28
);VRF_PREFIX_TRIE_IPV6(l2,l28) = NULL;}VRF_TRIE_INIT_DONE(l2,l28,l34,0);l128 = 
trie_delete(l249,key,0,&l209);l210 = (payload_t*)l209;if(SOC_FAILURE(l128)){
LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"Unable to delete internal default for vrf "" %d/%d\n"),l28,l34));}sal_free(
l210);(void)trie_destroy(l249);if(l34 == 0){l250 = VRF_PIVOT_TRIE_IPV4(l2,l28
);VRF_PIVOT_TRIE_IPV4(l2,l28) = NULL;}else{l250 = VRF_PIVOT_TRIE_IPV6(l2,l28)
;VRF_PIVOT_TRIE_IPV6(l2,l28) = NULL;}l209 = NULL;l128 = trie_delete(l250,key,
0,&l209);if(SOC_FAILURE(l128)){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"Unable to delete internal pivot node for vrf"" %d/%d\n"),l28,l34));}(void)
trie_destroy(l250);sal_free(PIVOT_BUCKET_HANDLE(l252));(void)trie_destroy(
PIVOT_BUCKET_TRIE(l252));sal_free(l252);return l128;}int soc_alpm_insert(int
l2,void*l6,uint32 l25,int l253,int l254){defip_alpm_ipv4_entry_t l215,l216;
defip_alpm_ipv6_64_entry_t l217,l218;soc_mem_t l24;void*l219,*l233;int l149,
l28;int index;int l10;int l128 = SOC_E_NONE;uint32 l26;l10 = 
soc_mem_field32_get(l2,L3_DEFIPm,l6,MODE0f);l24 = (l10)?
L3_DEFIP_ALPM_IPV6_64m:L3_DEFIP_ALPM_IPV4m;l219 = ((l10)?((uint32*)&(l217)):(
(uint32*)&(l215)));l233 = ((l10)?((uint32*)&(l218)):((uint32*)&(l216)));
SOC_IF_ERROR_RETURN(l20(l2,l6,l219,l233,l24,l25,&l26));SOC_IF_ERROR_RETURN(
soc_alpm_lpm_vrf_get(l2,l6,&l149,&l28));if(l149 == SOC_L3_VRF_OVERRIDE){l128 = 
l5(l2,l6,&index);if(SOC_SUCCESS(l128)){VRF_PIVOT_REF_INC(l2,MAX_VRF_ID,l10);
VRF_TRIE_ROUTES_INC(l2,MAX_VRF_ID,l10);}else if(l128 == SOC_E_FULL){
VRF_PIVOT_FULL_INC(l2,MAX_VRF_ID,l10);}return(l128);}else if(l28 == 0){if(
soc_alpm_mode_get(l2)){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"Unit %d, VRF=0 cannot be added in Parallel mode\n"),l2));return SOC_E_PARAM;
}}if(l149!= SOC_L3_VRF_GLOBAL){if(!soc_alpm_mode_get(l2)){if(
VRF_TRIE_ROUTES_CNT(l2,l28,l10) == 0){if(!l26){LOG_ERROR(BSL_LS_SOC_ALPM,(
BSL_META_U(l2,"VRF %d: First route in a VRF has to "
" be a default route in this mode\n"),l149));return SOC_E_PARAM;}}}}if(!
VRF_TRIE_INIT_COMPLETED(l2,l28,l10)){LOG_VERBOSE(BSL_LS_SOC_ALPM,(BSL_META_U(
l2,"soc_alpm_insert:VRF %d is not ""initialized\n"),l28));l128 = l246(l2,l28,
l10);if(SOC_FAILURE(l128)){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"soc_alpm_insert:VRF %d/%d trie init \n""failed\n"),l28,l10));return l128;}
LOG_VERBOSE(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"soc_alpm_insert:VRF %d/%d trie init ""completed\n"),l28,l10));}if(l254&
SOC_ALPM_LOOKUP_HIT){l128 = l150(l2,l6,l219,l233,l24,l253);}else{if(l253 == -
1){l253 = 0;}l128 = l200(l2,l6,l24,l219,l233,&index,SOC_ALPM_BKT_ENTRY_TO_IDX
(l253),l254);}if(l128!= SOC_E_NONE){LOG_WARN(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"unit %d :soc_alpm_insert: Route Insertion Failed :%s\n"),l2,soc_errmsg(l128)
));}return(l128);}int soc_alpm_lookup(int l2,void*l8,void*l15,int*l16,int*
l255){defip_alpm_ipv4_entry_t l215;defip_alpm_ipv6_64_entry_t l217;soc_mem_t
l24;int bucket_index;int l143;void*l219;int l149,l28;int l10,l120;int l128 = 
SOC_E_NONE;l10 = soc_mem_field32_get(l2,L3_DEFIPm,l8,MODE0f);
SOC_IF_ERROR_RETURN(soc_alpm_lpm_vrf_get(l2,l8,&l149,&l28));l128 = l14(l2,l8,
l15,l16,&l120,&l10);if(SOC_SUCCESS(l128)){if(!l10&&(*l16&0x1)){l128 = 
soc_alpm_lpm_ip4entry1_to_0(l2,l15,l15,PRESERVE_HIT);}SOC_IF_ERROR_RETURN(
soc_alpm_lpm_vrf_get(l2,l15,&l149,&l28));if(l149 == SOC_L3_VRF_OVERRIDE){
return SOC_E_NONE;}}if(!VRF_TRIE_INIT_COMPLETED(l2,l28,l10)){LOG_VERBOSE(
BSL_LS_SOC_ALPM,(BSL_META_U(l2,"soc_alpm_lookup:VRF %d is not initialized\n")
,l28));*l255 = 0;return SOC_E_NOT_FOUND;}l24 = (l10)?L3_DEFIP_ALPM_IPV6_64m:
L3_DEFIP_ALPM_IPV4m;l219 = ((l10)?((uint32*)&(l217)):((uint32*)&(l215)));
SOC_ALPM_LPM_LOCK(l2);l128 = l147(l2,l8,l24,l219,&l143,&bucket_index,l16,TRUE
);SOC_ALPM_LPM_UNLOCK(l2);if(SOC_FAILURE(l128)){*l255 = l143;*l16 = 
bucket_index<<2;return l128;}l128 = l27(l2,l219,l24,l10,l149,bucket_index,*
l16,l15);*l255 = SOC_ALPM_LOOKUP_HIT|l143;return(l128);}static int l256(int l2
,void*l8,void*l15,int l28,int*l143,int*bucket_index,int*l121,int l257){int
l128 = SOC_E_NONE;int l171,l258,l34,l142 = 0;uint32 l11,l138;
defip_aux_scratch_entry_t l13;int l259,l260;int index;soc_mem_t l24,l261;int
l262,l263;int l264;uint32 l265[SOC_MAX_MEM_FIELD_WORDS] = {0};int l266 = -1;
int l267 = 0;soc_field_t l268[2] = {IP_ADDR0f,IP_ADDR1f,};l261 = L3_DEFIPm;
l34 = soc_mem_field32_get(l2,l261,l8,MODE0f);l259 = soc_mem_field32_get(l2,
l261,l8,GLOBAL_ROUTE0f);l260 = soc_mem_field32_get(l2,l261,l8,VRF_ID_0f);
LOG_VERBOSE(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"Prefare AUX Scratch for searching TCAM in "
"%s region, Key data: v6 %d global %d vrf %d:\n"),l28 == SOC_L3_VRF_GLOBAL?
"Global":"VRF",l34,l259,l260));if(l28 == SOC_L3_VRF_GLOBAL){l11 = l257?1:0;
SOC_ALPM_GET_GLOBAL_BANK_DISABLE(l2,l138);soc_mem_field32_set(l2,l261,l8,
GLOBAL_ROUTE0f,1);soc_mem_field32_set(l2,l261,l8,VRF_ID_0f,0);}else{l11 = 
l257?3:2;SOC_ALPM_GET_VRF_BANK_DISABLE(l2,l138);}sal_memset(&l13,0,sizeof(
defip_aux_scratch_entry_t));SOC_IF_ERROR_RETURN(l9(l2,l8,l34,l11,0,&l13));if(
l28 == SOC_L3_VRF_GLOBAL){soc_mem_field32_set(l2,l261,l8,GLOBAL_ROUTE0f,l259)
;soc_mem_field32_set(l2,l261,l8,VRF_ID_0f,l260);}SOC_IF_ERROR_RETURN(
_soc_alpm_aux_op(l2,PREFIX_LOOKUP,&l13,TRUE,&l142,l143,bucket_index));if(l142
== 0){LOG_VERBOSE(BSL_LS_SOC_ALPM,(BSL_META_U(l2,"Could not find bucket\n")))
;return SOC_E_NOT_FOUND;}LOG_VERBOSE(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"Hit in memory %s, index %d, ""bucket_index %d\n"),SOC_MEM_NAME(l2,l261),
soc_alpm_logical_idx(l2,l261,(*l143)>>1,1),*bucket_index));l24 = (l34)?
L3_DEFIP_ALPM_IPV6_64m:L3_DEFIP_ALPM_IPV4m;l128 = l127(l2,l8,&l263);if(
SOC_FAILURE(l128)){return l128;}l264 = 24;if(l34){if(SOC_ALPM_V6_SCALE_CHECK(
l2,l34)){l264 = 32;}else{l264 = 16;}}LOG_VERBOSE(BSL_LS_SOC_ALPM,(BSL_META_U(
l2,"Start searching mem %s bucket %d(count %d) ""for Length %d\n"),
SOC_MEM_NAME(l2,l24),*bucket_index,l264,l263));for(l171 = 0;l171<l264;l171++)
{uint32 l219[SOC_MAX_MEM_FIELD_WORDS] = {0};uint32 l180[2] = {0};uint32 l269[
2] = {0};uint32 l270[2] = {0};int l271;l128 = _soc_alpm_mem_index(l2,l24,*
bucket_index,l171,l138,&index);if(l128 == SOC_E_FULL){continue;}
SOC_IF_ERROR_RETURN(soc_mem_read(l2,l24,MEM_BLOCK_ANY,index,(void*)l219));
l271 = soc_mem_field32_get(l2,l24,l219,VALIDf);l262 = soc_mem_field32_get(l2,
l24,l219,LENGTHf);LOG_VERBOSE(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"Bucket %5d Index %6d: Valid %d, Length %d\n"),*bucket_index,index,l271,l262)
);if(!l271||(l262>l263)){continue;}SHR_BITSET_RANGE(l180,(l34?64:32)-l262,
l262);(void)soc_mem_field_get(l2,l24,(uint32*)l219,KEYf,(uint32*)l269);l270[1
] = soc_mem_field32_get(l2,l261,l8,l268[1]);l270[0] = soc_mem_field32_get(l2,
l261,l8,l268[0]);LOG_VERBOSE(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"\tmask %08x %08x\n\t key %08x %08x\n""\thost %08x %08x\n"),l180[1],l180[0],
l269[1],l269[0],l270[1],l270[0]));for(l258 = l34?1:0;l258>= 0;l258--){if((
l270[l258]&l180[l258])!= (l269[l258]&l180[l258])){break;}}if(l258>= 0){
continue;}LOG_VERBOSE(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"Found a match in mem %s bucket %d, ""index %d\n"),SOC_MEM_NAME(l2,l24),*
bucket_index,index));if(l266 == -1||l266<l262){l266 = l262;l267 = index;
sal_memcpy(l265,l219,sizeof(l219));}}if(l266!= -1){l128 = l27(l2,l265,l24,l34
,l28,*bucket_index,l267,l15);if(SOC_SUCCESS(l128)){*l121 = l267;if(bsl_check(
bslLayerSoc,bslSourceAlpm,bslSeverityVerbose,l2)){LOG_VERBOSE(BSL_LS_SOC_ALPM
,(BSL_META_U(l2,"Hit mem %s bucket %d, index %d\n"),SOC_MEM_NAME(l2,l24),*
bucket_index,l267));}}return l128;}*l121 = soc_alpm_logical_idx(l2,l261,(*
l143)>>1,1);LOG_VERBOSE(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"Miss in mem %s bucket %d, use associate data ""in mem %s LOG index %d\n"),
SOC_MEM_NAME(l2,l24),*bucket_index,SOC_MEM_NAME(l2,l261),*l121));
SOC_IF_ERROR_RETURN(soc_mem_read(l2,l261,MEM_BLOCK_ANY,*l121,(void*)l15));if(
(!l34)&&((*l143)&1)){l128 = soc_alpm_lpm_ip4entry1_to_0(l2,l15,l15,
PRESERVE_HIT);}return SOC_E_NONE;}int soc_alpm_find_best_match(int l2,void*l8
,void*l15,int*l16,int l257){int l128 = SOC_E_NONE;int l171,l272,l273;
defip_entry_t l274;uint32 l275[2];uint32 l269[2];uint32 l276[2];uint32 l270[2
];uint32 l277,l278;int l149,l28 = 0;int l279[2] = {0};int l143,bucket_index;
soc_mem_t l261 = L3_DEFIPm;int l192,l34,l280,l281 = 0;soc_field_t l282[] = {
GLOBAL_HIGH0f,GLOBAL_HIGH1f};soc_field_t l283[] = {GLOBAL_ROUTE0f,
GLOBAL_ROUTE1f};l34 = soc_mem_field32_get(l2,l261,l8,MODE0f);if(!
SOC_URPF_STATUS_GET(l2)&&l257){return SOC_E_PARAM;}l272 = soc_mem_index_min(
l2,l261);l273 = soc_mem_index_count(l2,l261);if(SOC_URPF_STATUS_GET(l2)){l273
>>= 1;}if(soc_alpm_mode_get(l2)){l273>>= 1;l272+= l273;}if(l257){l272+= 
soc_mem_index_count(l2,l261)/2;}LOG_VERBOSE(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"Launch LPM searching from index %d count %d\n"),l272,l273));for(l171 = l272;
l171<l272+l273;l171++){SOC_IF_ERROR_RETURN(soc_mem_read(l2,l261,MEM_BLOCK_ANY
,l171,(void*)&l274));l279[0] = soc_mem_field32_get(l2,l261,&l274,VALID0f);
l279[1] = soc_mem_field32_get(l2,l261,&l274,VALID1f);if(l279[0] == 0&&l279[1]
== 0){continue;}l280 = soc_mem_field32_get(l2,l261,&l274,MODE0f);if(l280!= 
l34){continue;}for(l192 = 0;l192<(l34?1:2);l192++){if(l279[l192] == 0){
continue;}l277 = soc_mem_field32_get(l2,l261,&l274,l282[l192]);l278 = 
soc_mem_field32_get(l2,l261,&l274,l283[l192]);if(!l277||!l278){continue;}
LOG_VERBOSE(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"Match a Global High route: ent %d\n"),l192));l275[0] = soc_mem_field32_get(
l2,l261,&l274,IP_ADDR_MASK0f);l275[1] = soc_mem_field32_get(l2,l261,&l274,
IP_ADDR_MASK1f);l269[0] = soc_mem_field32_get(l2,l261,&l274,IP_ADDR0f);l269[1
] = soc_mem_field32_get(l2,l261,&l274,IP_ADDR1f);l276[0] = 
soc_mem_field32_get(l2,l261,l8,IP_ADDR_MASK0f);l276[1] = soc_mem_field32_get(
l2,l261,l8,IP_ADDR_MASK1f);l270[0] = soc_mem_field32_get(l2,l261,l8,IP_ADDR0f
);l270[1] = soc_mem_field32_get(l2,l261,l8,IP_ADDR1f);LOG_VERBOSE(
BSL_LS_SOC_ALPM,(BSL_META_U(l2,"\thmsk %08x %08x\n\thkey %08x %08x\n"
"\tsmsk %08x %08x\n\tskey %08x %08x\n"),l275[1],l275[0],l269[1],l269[0],l276[
1],l276[0],l270[1],l270[0]));if(l34&&(((l275[1]&l276[1])!= l275[1])||((l275[0
]&l276[0])!= l275[0]))){continue;}if(!l34&&((l275[l192]&l276[0])!= l275[l192]
)){continue;}if(l34&&((l270[0]&l275[0]) == (l269[0]&l275[0]))&&((l270[1]&l275
[1]) == (l269[1]&l275[1]))){l281 = TRUE;break;}if(!l34&&((l270[0]&l275[l192])
== (l269[l192]&l275[l192]))){l281 = TRUE;break;}}if(!l281){continue;}
LOG_VERBOSE(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"Hit Global High route in index = %d(%d)\n"),l171,l192));sal_memcpy(l15,&l274
,sizeof(l274));if(!l34&&l192 == 1){l128 = soc_alpm_lpm_ip4entry1_to_0(l2,l15,
l15,PRESERVE_HIT);}*l16 = l171;return l128;}LOG_VERBOSE(BSL_LS_SOC_ALPM,(
BSL_META_U(l2,"Global high lookup miss, use AUX engine to "
"search for VRF and Global Low routes\n")));SOC_IF_ERROR_RETURN(
soc_alpm_lpm_vrf_get(l2,l8,&l149,&l28));l128 = l256(l2,l8,l15,l28,&l143,&
bucket_index,l16,l257);if(l128 == SOC_E_NOT_FOUND){l28 = SOC_L3_VRF_GLOBAL;
LOG_VERBOSE(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"Not found in VRF region, trying Global ""region\n")));l128 = l256(l2,l8,l15,
l28,&l143,&bucket_index,l16,l257);}if(SOC_SUCCESS(l128)){LOG_VERBOSE(
BSL_LS_SOC_ALPM,(BSL_META_U(l2,"Hit in %s region in TCAM index %d, "
"buckekt_index %d\n"),l28 == SOC_L3_VRF_GLOBAL?"Global Low":"VRF",l143,
bucket_index));}else{LOG_VERBOSE(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"Search miss for given address\n")));}return(l128);}int soc_alpm_delete(int l2
,void*l8,int l253,int l254){int l149,l28;int l10;int l128 = SOC_E_NONE;l10 = 
soc_mem_field32_get(l2,L3_DEFIPm,l8,MODE0f);SOC_IF_ERROR_RETURN(
soc_alpm_lpm_vrf_get(l2,l8,&l149,&l28));if(l149 == SOC_L3_VRF_OVERRIDE){l128 = 
l7(l2,l8);if(SOC_SUCCESS(l128)){VRF_PIVOT_REF_DEC(l2,MAX_VRF_ID,l10);
VRF_TRIE_ROUTES_DEC(l2,MAX_VRF_ID,l10);}return(l128);}else{if(!
VRF_TRIE_INIT_COMPLETED(l2,l28,l10)){LOG_VERBOSE(BSL_LS_SOC_ALPM,(BSL_META_U(
l2,"soc_alpm_delete:VRF %d/%d is not initialized\n"),l28,l10));return
SOC_E_NONE;}if(l253 == -1){l253 = 0;}l128 = l234(l2,l8,
SOC_ALPM_BKT_ENTRY_TO_IDX(l253),l254&~SOC_ALPM_LOOKUP_HIT,l253);}return(l128)
;}static int l19(int l2){int l284;l284 = soc_mem_index_count(l2,L3_DEFIPm)+
soc_mem_index_count(l2,L3_DEFIP_PAIR_128m)*2;if(SOC_URPF_STATUS_GET(l2)){l284
>>= 1;}SOC_ALPM_BUCKET_COUNT(l2) = l284*2;SOC_ALPM_BUCKET_BMAP_SIZE(l2) = 
SHR_BITALLOCSIZE(SOC_ALPM_BUCKET_COUNT(l2));SOC_ALPM_BUCKET_BMAP(l2) = 
sal_alloc(SOC_ALPM_BUCKET_BMAP_SIZE(l2),"alpm_shared_bucket_bitmap");if(
SOC_ALPM_BUCKET_BMAP(l2) == NULL){LOG_WARN(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"soc_alpm_shared_mem_init: Memory allocation for "
"bucket bitmap management failed\n")));return SOC_E_MEMORY;}sal_memset(
SOC_ALPM_BUCKET_BMAP(l2),0,SOC_ALPM_BUCKET_BMAP_SIZE(l2));alpm_bucket_assign(
l2,&l284,1);return SOC_E_NONE;}int alpm_bucket_assign(int l2,int*l248,int l34
){int l171,l285 = 1,l286 = 0;if(l34){if(!soc_alpm_mode_get(l2)&&!
SOC_URPF_STATUS_GET(l2)){l285 = 2;}}for(l171 = 0;l171<SOC_ALPM_BUCKET_COUNT(
l2);l171+= l285){SHR_BITTEST_RANGE(SOC_ALPM_BUCKET_BMAP(l2),l171,l285,l286);
if(0 == l286){break;}}if(l171 == SOC_ALPM_BUCKET_COUNT(l2)){return SOC_E_FULL
;}SHR_BITSET_RANGE(SOC_ALPM_BUCKET_BMAP(l2),l171,l285);*l248 = l171;
SOC_ALPM_BUCKET_NEXT_FREE(l2) = l171;return SOC_E_NONE;}int
alpm_bucket_release(int l2,int l248,int l34){int l285 = 1,l286 = 0;if((l248<1
)||(l248>SOC_ALPM_BUCKET_MAX_INDEX(l2))){LOG_ERROR(BSL_LS_SOC_ALPM,(
BSL_META_U(l2,"Unit %d\n, freeing invalid bucket index %d\n"),l2,l248));
return SOC_E_PARAM;}if(l34){if(!soc_alpm_mode_get(l2)&&!SOC_URPF_STATUS_GET(
l2)){l285 = 2;}}SHR_BITTEST_RANGE(SOC_ALPM_BUCKET_BMAP(l2),l248,l285,l286);if
(!l286){return SOC_E_PARAM;}SHR_BITCLR_RANGE(SOC_ALPM_BUCKET_BMAP(l2),l248,
l285);return SOC_E_NONE;}int alpm_bucket_is_assigned(int l2,int l287,int l10,
int*l286){int l285 = 1;if((l287<1)||(l287>SOC_ALPM_BUCKET_MAX_INDEX(l2))){
return SOC_E_PARAM;}if(l10){if(!soc_alpm_mode_get(l2)&&!SOC_URPF_STATUS_GET(
l2)){l285 = 2;}}SHR_BITTEST_RANGE(SOC_ALPM_BUCKET_BMAP(l2),l287,l285,*l286);
return SOC_E_NONE;}static void l108(int l2,void*l15,int index,l103 l109){if(
index&FB_LPM_HASH_IPV6_MASK){l109[0] = soc_meminfo_fieldinfo_field32_get((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l15),(l96[(l2)]->l60));l109[1] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l15),(l96[(
l2)]->l62));l109[2] = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,
L3_DEFIPm)),(l15),(l96[(l2)]->l61));l109[3] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l15),(l96[(
l2)]->l63));if((!(SOC_IS_HURRICANE(l2)))&&(((l96[(l2)]->l76)!= NULL))){int
l288;(void)soc_alpm_lpm_vrf_get(l2,l15,(int*)&l109[4],&l288);}else{l109[4] = 
0;};}else{if(index&0x1){l109[0] = soc_meminfo_fieldinfo_field32_get((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l15),(l96[(l2)]->l61));l109[1] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l15),(l96[(
l2)]->l63));l109[2] = 0;l109[3] = 0x80000001;if((!(SOC_IS_HURRICANE(l2)))&&((
(l96[(l2)]->l77)!= NULL))){int l288;defip_entry_t l289;(void)
soc_alpm_lpm_ip4entry1_to_0(l2,l15,&l289,0);(void)soc_alpm_lpm_vrf_get(l2,&
l289,(int*)&l109[4],&l288);}else{l109[4] = 0;};}else{l109[0] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l15),(l96[(
l2)]->l60));l109[1] = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,
L3_DEFIPm)),(l15),(l96[(l2)]->l62));l109[2] = 0;l109[3] = 0x80000001;if((!(
SOC_IS_HURRICANE(l2)))&&(((l96[(l2)]->l76)!= NULL))){int l288;(void)
soc_alpm_lpm_vrf_get(l2,l15,(int*)&l109[4],&l288);}else{l109[4] = 0;};}}}
static int l290(l103 l105,l103 l106){int l130;for(l130 = 0;l130<5;l130++){{if
((l105[l130])<(l106[l130])){return-1;}if((l105[l130])>(l106[l130])){return 1;
}};}return(0);}static void l291(int l2,void*l6,uint32 l292,uint32 l123,int
l120){l103 l293;if(soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,
L3_DEFIPm)),(l6),(l96[(l2)]->l64))){if(soc_meminfo_fieldinfo_field32_get((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l6),(l96[(l2)]->l75))&&
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l6),(l96[(l2
)]->l74))){l293[0] = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,
L3_DEFIPm)),(l6),(l96[(l2)]->l60));l293[1] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l6),(l96[(l2
)]->l62));l293[2] = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,
L3_DEFIPm)),(l6),(l96[(l2)]->l61));l293[3] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l6),(l96[(l2
)]->l63));if((!(SOC_IS_HURRICANE(l2)))&&(((l96[(l2)]->l76)!= NULL))){int l288
;(void)soc_alpm_lpm_vrf_get(l2,l6,(int*)&l293[4],&l288);}else{l293[4] = 0;};
l122((l107[(l2)]),l290,l293,l120,l123,((uint16)l292<<1)|FB_LPM_HASH_IPV6_MASK
);}}else{if(soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(
l6),(l96[(l2)]->l74))){l293[0] = soc_meminfo_fieldinfo_field32_get((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l6),(l96[(l2)]->l60));l293[1] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l6),(l96[(l2
)]->l62));l293[2] = 0;l293[3] = 0x80000001;if((!(SOC_IS_HURRICANE(l2)))&&(((
l96[(l2)]->l76)!= NULL))){int l288;(void)soc_alpm_lpm_vrf_get(l2,l6,(int*)&
l293[4],&l288);}else{l293[4] = 0;};l122((l107[(l2)]),l290,l293,l120,l123,((
uint16)l292<<1));}if(soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,
L3_DEFIPm)),(l6),(l96[(l2)]->l75))){l293[0] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l6),(l96[(l2
)]->l61));l293[1] = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,
L3_DEFIPm)),(l6),(l96[(l2)]->l63));l293[2] = 0;l293[3] = 0x80000001;if((!(
SOC_IS_HURRICANE(l2)))&&(((l96[(l2)]->l77)!= NULL))){int l288;defip_entry_t
l289;(void)soc_alpm_lpm_ip4entry1_to_0(l2,l6,&l289,0);(void)
soc_alpm_lpm_vrf_get(l2,&l289,(int*)&l293[4],&l288);}else{l293[4] = 0;};l122(
(l107[(l2)]),l290,l293,l120,l123,(((uint16)l292<<1)+1));}}}static void l294(
int l2,void*l8,uint32 l292){l103 l293;int l120 = -1;int l128;uint16 index;if(
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l8),(l96[(l2
)]->l64))){l293[0] = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,
L3_DEFIPm)),(l8),(l96[(l2)]->l60));l293[1] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l8),(l96[(l2
)]->l62));l293[2] = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,
L3_DEFIPm)),(l8),(l96[(l2)]->l61));l293[3] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l8),(l96[(l2
)]->l63));if((!(SOC_IS_HURRICANE(l2)))&&(((l96[(l2)]->l76)!= NULL))){int l288
;(void)soc_alpm_lpm_vrf_get(l2,l8,(int*)&l293[4],&l288);}else{l293[4] = 0;};
index = (l292<<1)|FB_LPM_HASH_IPV6_MASK;}else{l293[0] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l8),(l96[(l2
)]->l60));l293[1] = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,
L3_DEFIPm)),(l8),(l96[(l2)]->l62));l293[2] = 0;l293[3] = 0x80000001;if((!(
SOC_IS_HURRICANE(l2)))&&(((l96[(l2)]->l76)!= NULL))){int l288;(void)
soc_alpm_lpm_vrf_get(l2,l8,(int*)&l293[4],&l288);}else{l293[4] = 0;};index = 
l292;}l128 = l125((l107[(l2)]),l290,l293,l120,index);if(SOC_FAILURE(l128)){
LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l2,"\ndel  index: H %d error %d\n"),
index,l128));}}static int l295(int l2,void*l8,int l120,int*l121){l103 l293;
int l296;int l128;uint16 index = FB_LPM_HASH_INDEX_NULL;l296 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l8),(l96[(l2
)]->l64));if(l296){l293[0] = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO
(l2,L3_DEFIPm)),(l8),(l96[(l2)]->l60));l293[1] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l8),(l96[(l2
)]->l62));l293[2] = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,
L3_DEFIPm)),(l8),(l96[(l2)]->l61));l293[3] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l8),(l96[(l2
)]->l63));if((!(SOC_IS_HURRICANE(l2)))&&(((l96[(l2)]->l76)!= NULL))){int l288
;(void)soc_alpm_lpm_vrf_get(l2,l8,(int*)&l293[4],&l288);}else{l293[4] = 0;};}
else{l293[0] = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)
),(l8),(l96[(l2)]->l60));l293[1] = soc_meminfo_fieldinfo_field32_get((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l8),(l96[(l2)]->l62));l293[2] = 0;l293[3] = 
0x80000001;if((!(SOC_IS_HURRICANE(l2)))&&(((l96[(l2)]->l76)!= NULL))){int l288
;(void)soc_alpm_lpm_vrf_get(l2,l8,(int*)&l293[4],&l288);}else{l293[4] = 0;};}
l128 = l117((l107[(l2)]),l290,l293,l120,&index);if(SOC_FAILURE(l128)){*l121 = 
0xFFFFFFFF;return(l128);}*l121 = index;return(SOC_E_NONE);}static uint16 l110
(uint8*l111,int l112){return(_shr_crc16b(0,l111,l112));}static int l113(int
l21,int l98,int l99,l102**l114){l102*l118;int index;if(l99>l98){return
SOC_E_MEMORY;}l118 = sal_alloc(sizeof(l102),"lpm_hash");if(l118 == NULL){
return SOC_E_MEMORY;}sal_memset(l118,0,sizeof(*l118));l118->l21 = l21;l118->
l98 = l98;l118->l99 = l99;l118->l100 = sal_alloc(l118->l99*sizeof(*(l118->
l100)),"hash_table");if(l118->l100 == NULL){sal_free(l118);return SOC_E_MEMORY
;}l118->l101 = sal_alloc(l118->l98*sizeof(*(l118->l101)),"link_table");if(
l118->l101 == NULL){sal_free(l118->l100);sal_free(l118);return SOC_E_MEMORY;}
for(index = 0;index<l118->l99;index++){l118->l100[index] = 
FB_LPM_HASH_INDEX_NULL;}for(index = 0;index<l118->l98;index++){l118->l101[
index] = FB_LPM_HASH_INDEX_NULL;}*l114 = l118;return SOC_E_NONE;}static int
l115(l102*l116){if(l116!= NULL){sal_free(l116->l100);sal_free(l116->l101);
sal_free(l116);}return SOC_E_NONE;}static int l117(l102*l118,l104 l119,l103
entry,int l120,uint16*l121){int l2 = l118->l21;uint16 l297;uint16 index;l297 = 
l110((uint8*)entry,(32*5))%l118->l99;index = l118->l100[l297];;;while(index!= 
FB_LPM_HASH_INDEX_NULL){uint32 l15[SOC_MAX_MEM_FIELD_WORDS];l103 l109;int l298
;l298 = (index&FB_LPM_HASH_INDEX_MASK)>>1;SOC_IF_ERROR_RETURN(READ_L3_DEFIPm(
l2,MEM_BLOCK_ANY,l298,l15));l108(l2,l15,index,l109);if((*l119)(entry,l109) == 
0){*l121 = (index&FB_LPM_HASH_INDEX_MASK)>>((index&FB_LPM_HASH_IPV6_MASK)?1:0
);;return(SOC_E_NONE);}index = l118->l101[index&FB_LPM_HASH_INDEX_MASK];;};
return(SOC_E_NOT_FOUND);}static int l122(l102*l118,l104 l119,l103 entry,int
l120,uint16 l123,uint16 l124){int l2 = l118->l21;uint16 l297;uint16 index;
uint16 l299;l297 = l110((uint8*)entry,(32*5))%l118->l99;index = l118->l100[
l297];;;;l299 = FB_LPM_HASH_INDEX_NULL;if(l123!= FB_LPM_HASH_INDEX_NULL){
while(index!= FB_LPM_HASH_INDEX_NULL){uint32 l15[SOC_MAX_MEM_FIELD_WORDS];
l103 l109;int l298;l298 = (index&FB_LPM_HASH_INDEX_MASK)>>1;
SOC_IF_ERROR_RETURN(READ_L3_DEFIPm(l2,MEM_BLOCK_ANY,l298,l15));l108(l2,l15,
index,l109);if((*l119)(entry,l109) == 0){if(l124!= index){;if(l299 == 
FB_LPM_HASH_INDEX_NULL){l118->l100[l297] = l124;l118->l101[l124&
FB_LPM_HASH_INDEX_MASK] = l118->l101[index&FB_LPM_HASH_INDEX_MASK];l118->l101
[index&FB_LPM_HASH_INDEX_MASK] = FB_LPM_HASH_INDEX_NULL;}else{l118->l101[l299
&FB_LPM_HASH_INDEX_MASK] = l124;l118->l101[l124&FB_LPM_HASH_INDEX_MASK] = 
l118->l101[index&FB_LPM_HASH_INDEX_MASK];l118->l101[index&
FB_LPM_HASH_INDEX_MASK] = FB_LPM_HASH_INDEX_NULL;}};return(SOC_E_NONE);}l299 = 
index;index = l118->l101[index&FB_LPM_HASH_INDEX_MASK];;}}l118->l101[l124&
FB_LPM_HASH_INDEX_MASK] = l118->l100[l297];l118->l100[l297] = l124;return(
SOC_E_NONE);}static int l125(l102*l118,l104 l119,l103 entry,int l120,uint16
l126){uint16 l297;uint16 index;uint16 l299;l297 = l110((uint8*)entry,(32*5))%
l118->l99;index = l118->l100[l297];;;l299 = FB_LPM_HASH_INDEX_NULL;while(
index!= FB_LPM_HASH_INDEX_NULL){if(l126 == index){;if(l299 == 
FB_LPM_HASH_INDEX_NULL){l118->l100[l297] = l118->l101[l126&
FB_LPM_HASH_INDEX_MASK];l118->l101[l126&FB_LPM_HASH_INDEX_MASK] = 
FB_LPM_HASH_INDEX_NULL;}else{l118->l101[l299&FB_LPM_HASH_INDEX_MASK] = l118->
l101[l126&FB_LPM_HASH_INDEX_MASK];l118->l101[l126&FB_LPM_HASH_INDEX_MASK] = 
FB_LPM_HASH_INDEX_NULL;}return(SOC_E_NONE);}l299 = index;index = l118->l101[
index&FB_LPM_HASH_INDEX_MASK];;}return(SOC_E_NOT_FOUND);}int _ipmask2pfx(
uint32 l300,int*l301){*l301 = 0;while(l300&(1<<31)){*l301+= 1;l300<<= 1;}
return((l300)?SOC_E_PARAM:SOC_E_NONE);}int soc_alpm_lpm_ip4entry0_to_0(int l2
,void*l302,void*l303,int l304){uint32 l129;l129 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(
l2)]->l74));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(
l303),(l96[(l2)]->l74),(l129));l129 = soc_meminfo_fieldinfo_field32_get((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(l2)]->l64));
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(
l2)]->l64),(l129));l129 = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2
,L3_DEFIPm)),(l302),(l96[(l2)]->l60));soc_meminfo_fieldinfo_field32_set((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(l2)]->l60),(l129));l129 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(
l2)]->l62));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(
l303),(l96[(l2)]->l62),(l129));if(((l96[(l2)]->l50)!= NULL)){l129 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(
l2)]->l50));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(
l303),(l96[(l2)]->l50),(l129));}if(((l96[(l2)]->l52)!= NULL)){l129 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(
l2)]->l52));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(
l303),(l96[(l2)]->l52),(l129));l129 = soc_meminfo_fieldinfo_field32_get((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(l2)]->l54));
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(
l2)]->l54),(l129));}else{l129 = soc_meminfo_fieldinfo_field32_get((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(l2)]->l68));
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(
l2)]->l68),(l129));}l129 = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(
l2,L3_DEFIPm)),(l302),(l96[(l2)]->l70));soc_meminfo_fieldinfo_field32_set((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(l2)]->l70),(l129));l129 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(
l2)]->l72));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(
l303),(l96[(l2)]->l72),(l129));if(((l96[(l2)]->l76)!= NULL)){l129 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(
l2)]->l76));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(
l303),(l96[(l2)]->l76),(l129));l129 = soc_meminfo_fieldinfo_field32_get((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(l2)]->l78));
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(
l2)]->l78),(l129));}if(((l96[(l2)]->l48)!= NULL)){l129 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(
l2)]->l48));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(
l303),(l96[(l2)]->l48),(l129));}if(l304){l129 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(
l2)]->l58));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(
l303),(l96[(l2)]->l58),(l129));}l129 = soc_meminfo_fieldinfo_field32_get((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(l2)]->l80));
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(
l2)]->l80),(l129));l129 = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2
,L3_DEFIPm)),(l302),(l96[(l2)]->l82));soc_meminfo_fieldinfo_field32_set((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(l2)]->l82),(l129));l129 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(
l2)]->l84));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(
l303),(l96[(l2)]->l84),(l129));l129 = soc_meminfo_fieldinfo_field32_get((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(l2)]->l86));
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(
l2)]->l86),(l129));l129 = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2
,L3_DEFIPm)),(l302),(l96[(l2)]->l88));soc_meminfo_fieldinfo_field32_set((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(l2)]->l88),(l129));l129 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(
l2)]->l90));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(
l303),(l96[(l2)]->l90),(l129));return(SOC_E_NONE);}int
soc_alpm_lpm_ip4entry1_to_1(int l2,void*l302,void*l303,int l304){uint32 l129;
l129 = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l302)
,(l96[(l2)]->l75));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,
L3_DEFIPm)),(l303),(l96[(l2)]->l75),(l129));l129 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(
l2)]->l65));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(
l303),(l96[(l2)]->l65),(l129));l129 = soc_meminfo_fieldinfo_field32_get((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(l2)]->l61));
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(
l2)]->l61),(l129));l129 = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2
,L3_DEFIPm)),(l302),(l96[(l2)]->l63));soc_meminfo_fieldinfo_field32_set((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(l2)]->l63),(l129));if(((l96[(l2)]->
l51)!= NULL)){l129 = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,
L3_DEFIPm)),(l302),(l96[(l2)]->l51));soc_meminfo_fieldinfo_field32_set((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(l2)]->l51),(l129));}if(((l96[(l2)]->
l53)!= NULL)){l129 = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,
L3_DEFIPm)),(l302),(l96[(l2)]->l53));soc_meminfo_fieldinfo_field32_set((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(l2)]->l53),(l129));l129 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(
l2)]->l55));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(
l303),(l96[(l2)]->l55),(l129));}else{l129 = soc_meminfo_fieldinfo_field32_get
((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(l2)]->l69));
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(
l2)]->l69),(l129));}l129 = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(
l2,L3_DEFIPm)),(l302),(l96[(l2)]->l71));soc_meminfo_fieldinfo_field32_set((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(l2)]->l71),(l129));l129 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(
l2)]->l73));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(
l303),(l96[(l2)]->l73),(l129));if(((l96[(l2)]->l77)!= NULL)){l129 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(
l2)]->l77));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(
l303),(l96[(l2)]->l77),(l129));l129 = soc_meminfo_fieldinfo_field32_get((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(l2)]->l79));
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(
l2)]->l79),(l129));}if(((l96[(l2)]->l49)!= NULL)){l129 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(
l2)]->l49));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(
l303),(l96[(l2)]->l49),(l129));}if(l304){l129 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(
l2)]->l59));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(
l303),(l96[(l2)]->l59),(l129));}l129 = soc_meminfo_fieldinfo_field32_get((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(l2)]->l81));
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(
l2)]->l81),(l129));l129 = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2
,L3_DEFIPm)),(l302),(l96[(l2)]->l83));soc_meminfo_fieldinfo_field32_set((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(l2)]->l83),(l129));l129 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(
l2)]->l85));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(
l303),(l96[(l2)]->l85),(l129));l129 = soc_meminfo_fieldinfo_field32_get((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(l2)]->l87));
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(
l2)]->l87),(l129));l129 = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2
,L3_DEFIPm)),(l302),(l96[(l2)]->l89));soc_meminfo_fieldinfo_field32_set((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(l2)]->l89),(l129));l129 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(
l2)]->l91));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(
l303),(l96[(l2)]->l91),(l129));return(SOC_E_NONE);}int
soc_alpm_lpm_ip4entry0_to_1(int l2,void*l302,void*l303,int l304){uint32 l129;
l129 = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l302)
,(l96[(l2)]->l74));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,
L3_DEFIPm)),(l303),(l96[(l2)]->l75),(l129));l129 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(
l2)]->l64));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(
l303),(l96[(l2)]->l65),(l129));l129 = soc_meminfo_fieldinfo_field32_get((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(l2)]->l60));
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(
l2)]->l61),(l129));l129 = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2
,L3_DEFIPm)),(l302),(l96[(l2)]->l62));soc_meminfo_fieldinfo_field32_set((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(l2)]->l63),(l129));if(!
SOC_IS_HURRICANE(l2)){l129 = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO
(l2,L3_DEFIPm)),(l302),(l96[(l2)]->l50));soc_meminfo_fieldinfo_field32_set((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(l2)]->l51),(l129));}if(((l96[(l2)]->
l52)!= NULL)){l129 = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,
L3_DEFIPm)),(l302),(l96[(l2)]->l52));soc_meminfo_fieldinfo_field32_set((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(l2)]->l53),(l129));l129 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(
l2)]->l54));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(
l303),(l96[(l2)]->l55),(l129));}else{l129 = soc_meminfo_fieldinfo_field32_get
((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(l2)]->l68));
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(
l2)]->l69),(l129));}l129 = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(
l2,L3_DEFIPm)),(l302),(l96[(l2)]->l70));soc_meminfo_fieldinfo_field32_set((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(l2)]->l71),(l129));l129 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(
l2)]->l72));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(
l303),(l96[(l2)]->l73),(l129));if(((l96[(l2)]->l76)!= NULL)){l129 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(
l2)]->l76));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(
l303),(l96[(l2)]->l77),(l129));l129 = soc_meminfo_fieldinfo_field32_get((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(l2)]->l78));
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(
l2)]->l79),(l129));}if(((l96[(l2)]->l48)!= NULL)){l129 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(
l2)]->l48));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(
l303),(l96[(l2)]->l49),(l129));}if(l304){l129 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(
l2)]->l58));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(
l303),(l96[(l2)]->l59),(l129));}l129 = soc_meminfo_fieldinfo_field32_get((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(l2)]->l80));
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(
l2)]->l81),(l129));l129 = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2
,L3_DEFIPm)),(l302),(l96[(l2)]->l82));soc_meminfo_fieldinfo_field32_set((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(l2)]->l83),(l129));l129 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(
l2)]->l84));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(
l303),(l96[(l2)]->l85),(l129));l129 = soc_meminfo_fieldinfo_field32_get((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(l2)]->l86));
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(
l2)]->l87),(l129));l129 = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2
,L3_DEFIPm)),(l302),(l96[(l2)]->l88));soc_meminfo_fieldinfo_field32_set((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(l2)]->l89),(l129));l129 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(
l2)]->l90));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(
l303),(l96[(l2)]->l91),(l129));return(SOC_E_NONE);}int
soc_alpm_lpm_ip4entry1_to_0(int l2,void*l302,void*l303,int l304){uint32 l129;
l129 = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l302)
,(l96[(l2)]->l75));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,
L3_DEFIPm)),(l303),(l96[(l2)]->l74),(l129));l129 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(
l2)]->l65));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(
l303),(l96[(l2)]->l64),(l129));l129 = soc_meminfo_fieldinfo_field32_get((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(l2)]->l61));
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(
l2)]->l60),(l129));l129 = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2
,L3_DEFIPm)),(l302),(l96[(l2)]->l63));soc_meminfo_fieldinfo_field32_set((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(l2)]->l62),(l129));if(!
SOC_IS_HURRICANE(l2)){l129 = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO
(l2,L3_DEFIPm)),(l302),(l96[(l2)]->l51));soc_meminfo_fieldinfo_field32_set((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(l2)]->l50),(l129));}if(((l96[(l2)]->
l53)!= NULL)){l129 = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,
L3_DEFIPm)),(l302),(l96[(l2)]->l53));soc_meminfo_fieldinfo_field32_set((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(l2)]->l52),(l129));l129 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(
l2)]->l55));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(
l303),(l96[(l2)]->l54),(l129));}else{l129 = soc_meminfo_fieldinfo_field32_get
((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(l2)]->l69));
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(
l2)]->l68),(l129));}l129 = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(
l2,L3_DEFIPm)),(l302),(l96[(l2)]->l71));soc_meminfo_fieldinfo_field32_set((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(l2)]->l70),(l129));l129 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(
l2)]->l73));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(
l303),(l96[(l2)]->l72),(l129));if(((l96[(l2)]->l77)!= NULL)){l129 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(
l2)]->l77));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(
l303),(l96[(l2)]->l76),(l129));l129 = soc_meminfo_fieldinfo_field32_get((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(l2)]->l79));
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(
l2)]->l78),(l129));}if(((l96[(l2)]->l49)!= NULL)){l129 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(
l2)]->l49));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(
l303),(l96[(l2)]->l48),(l129));}if(l304){l129 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(
l2)]->l59));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(
l303),(l96[(l2)]->l58),(l129));}l129 = soc_meminfo_fieldinfo_field32_get((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(l2)]->l81));
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(
l2)]->l80),(l129));l129 = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2
,L3_DEFIPm)),(l302),(l96[(l2)]->l83));soc_meminfo_fieldinfo_field32_set((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(l2)]->l82),(l129));l129 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(
l2)]->l85));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(
l303),(l96[(l2)]->l84),(l129));l129 = soc_meminfo_fieldinfo_field32_get((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(l2)]->l87));
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(
l2)]->l86),(l129));l129 = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2
,L3_DEFIPm)),(l302),(l96[(l2)]->l89));soc_meminfo_fieldinfo_field32_set((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(l303),(l96[(l2)]->l88),(l129));l129 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l302),(l96[(
l2)]->l91));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(
l303),(l96[(l2)]->l90),(l129));return(SOC_E_NONE);}static int l305(int l2,
void*l15){return(SOC_E_NONE);}void l1(int l2){int l171;int l306;l306 = ((3*(
64+32+2+1))-1);if(!bsl_check(bslLayerSoc,bslSourceAlpm,bslSeverityVerbose,l2)
){return;}for(l171 = l306;l171>= 0;l171--){if((l171!= ((3*(64+32+2+1))-1))&&(
(l44[(l2)][(l171)].l37) == -1)){continue;}LOG_VERBOSE(BSL_LS_SOC_ALPM,(
BSL_META_U(l2,
"PFX = %d P = %d N = %d START = %d END = %d VENT = %d FENT = %d\n"),l171,(l44
[(l2)][(l171)].l39),(l44[(l2)][(l171)].next),(l44[(l2)][(l171)].l37),(l44[(l2
)][(l171)].l38),(l44[(l2)][(l171)].l40),(l44[(l2)][(l171)].l41)));}
COMPILER_REFERENCE(l305);}static int l307(int l2,int index,uint32*l15){int
l308;int l10;uint32 l309;uint32 l310;int l311;if(!SOC_URPF_STATUS_GET(l2)){
return(SOC_E_NONE);}if(soc_feature(l2,soc_feature_l3_defip_hole)){l308 = (
soc_mem_index_count(l2,L3_DEFIPm)>>1);}else if(SOC_IS_APOLLO(l2)){l308 = (
soc_mem_index_count(l2,L3_DEFIPm)>>1)+0x0400;}else{l308 = (
soc_mem_index_count(l2,L3_DEFIPm)>>1);}l10 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l15),(l96[(
l2)]->l64));if(((l96[(l2)]->l48)!= NULL)){soc_meminfo_fieldinfo_field32_set((
&SOC_MEM_INFO(l2,L3_DEFIPm)),(l15),(l96[(l2)]->l48),(0));
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l15),(l96[(
l2)]->l49),(0));}l309 = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,
L3_DEFIPm)),(l15),(l96[(l2)]->l62));l310 = soc_meminfo_fieldinfo_field32_get(
(&SOC_MEM_INFO(l2,L3_DEFIPm)),(l15),(l96[(l2)]->l63));if(!l10){if(
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l15),(l96[(
l2)]->l74))){soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),
(l15),(l96[(l2)]->l72),((!l309)?1:0));}if(soc_meminfo_fieldinfo_field32_get((
&SOC_MEM_INFO(l2,L3_DEFIPm)),(l15),(l96[(l2)]->l75))){
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l15),(l96[(
l2)]->l73),((!l310)?1:0));}}else{l311 = ((!l309)&&(!l310))?1:0;l309 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l15),(l96[(
l2)]->l74));l310 = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,
L3_DEFIPm)),(l15),(l96[(l2)]->l75));if(l309&&l310){
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l15),(l96[(
l2)]->l72),(l311));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,
L3_DEFIPm)),(l15),(l96[(l2)]->l73),(l311));}}return l166(l2,MEM_BLOCK_ANY,
index+l308,index,l15);}static int l312(int l2,int l313,int l314){uint32 l15[
SOC_MAX_MEM_FIELD_WORDS];SOC_IF_ERROR_RETURN(READ_L3_DEFIPm(l2,MEM_BLOCK_ANY,
l313,l15));l291(l2,l15,l314,0x4000,0);SOC_IF_ERROR_RETURN(l166(l2,
MEM_BLOCK_ANY,l314,l313,l15));SOC_IF_ERROR_RETURN(l307(l2,l314,l15));do{int
l315,l316;l315 = soc_alpm_physical_idx((l2),L3_DEFIPm,(l313),1);l316 = 
soc_alpm_physical_idx((l2),L3_DEFIPm,(l314),1);ALPM_TCAM_PIVOT(l2,l316<<1) = 
ALPM_TCAM_PIVOT(l2,l315<<1);ALPM_TCAM_PIVOT(l2,(l316<<1)+1) = ALPM_TCAM_PIVOT
(l2,(l315<<1)+1);if(ALPM_TCAM_PIVOT((l2),l316<<1)){PIVOT_TCAM_INDEX(
ALPM_TCAM_PIVOT((l2),l316<<1)) = l316<<1;}if(ALPM_TCAM_PIVOT((l2),(l316<<1)+1
)){PIVOT_TCAM_INDEX(ALPM_TCAM_PIVOT((l2),(l316<<1)+1)) = (l316<<1)+1;}
ALPM_TCAM_PIVOT(l2,l315<<1) = NULL;ALPM_TCAM_PIVOT(l2,(l315<<1)+1) = NULL;}
while(0);return(SOC_E_NONE);}static int l317(int l2,int l120,int l10){uint32
l15[SOC_MAX_MEM_FIELD_WORDS];int l313;int l314;uint32 l318,l319;l314 = (l44[(
l2)][(l120)].l38)+1;if(!l10){l313 = (l44[(l2)][(l120)].l38);
SOC_IF_ERROR_RETURN(READ_L3_DEFIPm(l2,MEM_BLOCK_ANY,l313,l15));l318 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l15),(l96[(
l2)]->l74));l319 = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,
L3_DEFIPm)),(l15),(l96[(l2)]->l75));if((l318 == 0)||(l319 == 0)){l291(l2,l15,
l314,0x4000,0);SOC_IF_ERROR_RETURN(l166(l2,MEM_BLOCK_ANY,l314,l313,l15));
SOC_IF_ERROR_RETURN(l307(l2,l314,l15));do{int l320 = soc_alpm_physical_idx((
l2),L3_DEFIPm,(l313),1)<<1;int l229 = soc_alpm_physical_idx((l2),L3_DEFIPm,(
l314),1)*2+(l320&1);if((l319)){l320++;}ALPM_TCAM_PIVOT((l2),l229) = 
ALPM_TCAM_PIVOT((l2),l320);if(ALPM_TCAM_PIVOT((l2),l229)){PIVOT_TCAM_INDEX(
ALPM_TCAM_PIVOT((l2),l229)) = l229;}ALPM_TCAM_PIVOT((l2),l320) = NULL;}while(
0);l314--;}}l313 = (l44[(l2)][(l120)].l37);if(l313!= l314){
SOC_IF_ERROR_RETURN(l312(l2,l313,l314));VRF_PIVOT_SHIFT_INC(l2,MAX_VRF_ID,l10
);}(l44[(l2)][(l120)].l37)+= 1;(l44[(l2)][(l120)].l38)+= 1;return(SOC_E_NONE)
;}static int l321(int l2,int l120,int l10){uint32 l15[SOC_MAX_MEM_FIELD_WORDS
];int l313;int l314;int l322;uint32 l318,l319;l314 = (l44[(l2)][(l120)].l37)-
1;if((l44[(l2)][(l120)].l40) == 0){(l44[(l2)][(l120)].l37) = l314;(l44[(l2)][
(l120)].l38) = l314-1;return(SOC_E_NONE);}if((!l10)&&((l44[(l2)][(l120)].l38)
!= (l44[(l2)][(l120)].l37))){l313 = (l44[(l2)][(l120)].l38);
SOC_IF_ERROR_RETURN(READ_L3_DEFIPm(l2,MEM_BLOCK_ANY,l313,l15));l318 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l15),(l96[(
l2)]->l74));l319 = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,
L3_DEFIPm)),(l15),(l96[(l2)]->l75));if((l318 == 0)||(l319 == 0)){l322 = l313-
1;SOC_IF_ERROR_RETURN(l312(l2,l322,l314));VRF_PIVOT_SHIFT_INC(l2,MAX_VRF_ID,
l10);l291(l2,l15,l322,0x4000,0);SOC_IF_ERROR_RETURN(l166(l2,MEM_BLOCK_ANY,
l322,l313,l15));SOC_IF_ERROR_RETURN(l307(l2,l322,l15));do{int l320 = 
soc_alpm_physical_idx((l2),L3_DEFIPm,(l313),1)<<1;int l229 = 
soc_alpm_physical_idx((l2),L3_DEFIPm,(l322),1)*2+(l320&1);if((l319)){l320++;}
ALPM_TCAM_PIVOT((l2),l229) = ALPM_TCAM_PIVOT((l2),l320);if(ALPM_TCAM_PIVOT((
l2),l229)){PIVOT_TCAM_INDEX(ALPM_TCAM_PIVOT((l2),l229)) = l229;}
ALPM_TCAM_PIVOT((l2),l320) = NULL;}while(0);}else{l291(l2,l15,l314,0x4000,0);
SOC_IF_ERROR_RETURN(l166(l2,MEM_BLOCK_ANY,l314,l313,l15));SOC_IF_ERROR_RETURN
(l307(l2,l314,l15));do{int l315,l316;l315 = soc_alpm_physical_idx((l2),
L3_DEFIPm,(l313),1);l316 = soc_alpm_physical_idx((l2),L3_DEFIPm,(l314),1);
ALPM_TCAM_PIVOT(l2,l316<<1) = ALPM_TCAM_PIVOT(l2,l315<<1);ALPM_TCAM_PIVOT(l2,
(l316<<1)+1) = ALPM_TCAM_PIVOT(l2,(l315<<1)+1);if(ALPM_TCAM_PIVOT((l2),l316<<
1)){PIVOT_TCAM_INDEX(ALPM_TCAM_PIVOT((l2),l316<<1)) = l316<<1;}if(
ALPM_TCAM_PIVOT((l2),(l316<<1)+1)){PIVOT_TCAM_INDEX(ALPM_TCAM_PIVOT((l2),(
l316<<1)+1)) = (l316<<1)+1;}ALPM_TCAM_PIVOT(l2,l315<<1) = NULL;
ALPM_TCAM_PIVOT(l2,(l315<<1)+1) = NULL;}while(0);}}else{l313 = (l44[(l2)][(
l120)].l38);SOC_IF_ERROR_RETURN(l312(l2,l313,l314));VRF_PIVOT_SHIFT_INC(l2,
MAX_VRF_ID,l10);}(l44[(l2)][(l120)].l37)-= 1;(l44[(l2)][(l120)].l38)-= 1;
return(SOC_E_NONE);}static int l323(int l2,int l120,int l10,void*l15,int*l324
){int l325;int l326;int l327;int l328;int l313;uint32 l318,l319;int l128;if((
l44[(l2)][(l120)].l40) == 0){l328 = ((3*(64+32+2+1))-1);if(soc_alpm_mode_get(
l2) == SOC_ALPM_MODE_PARALLEL){if(l120<= (((3*(64+32+2+1))/3)-1)){l328 = (((3
*(64+32+2+1))/3)-1);}}while((l44[(l2)][(l328)].next)>l120){l328 = (l44[(l2)][
(l328)].next);}l326 = (l44[(l2)][(l328)].next);if(l326!= -1){(l44[(l2)][(l326
)].l39) = l120;}(l44[(l2)][(l120)].next) = (l44[(l2)][(l328)].next);(l44[(l2)
][(l120)].l39) = l328;(l44[(l2)][(l328)].next) = l120;(l44[(l2)][(l120)].l41)
= ((l44[(l2)][(l328)].l41)+1)/2;(l44[(l2)][(l328)].l41)-= (l44[(l2)][(l120)].
l41);(l44[(l2)][(l120)].l37) = (l44[(l2)][(l328)].l38)+(l44[(l2)][(l328)].l41
)+1;(l44[(l2)][(l120)].l38) = (l44[(l2)][(l120)].l37)-1;(l44[(l2)][(l120)].
l40) = 0;}else if(!l10){l313 = (l44[(l2)][(l120)].l37);if((l128 = 
READ_L3_DEFIPm(l2,MEM_BLOCK_ANY,l313,l15))<0){return l128;}l318 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l15),(l96[(
l2)]->l74));l319 = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,
L3_DEFIPm)),(l15),(l96[(l2)]->l75));if((l318 == 0)||(l319 == 0)){*l324 = (
l313<<1)+((l319 == 0)?1:0);return(SOC_E_NONE);}l313 = (l44[(l2)][(l120)].l38)
;if((l128 = READ_L3_DEFIPm(l2,MEM_BLOCK_ANY,l313,l15))<0){return l128;}l318 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l15),(l96[(
l2)]->l74));l319 = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,
L3_DEFIPm)),(l15),(l96[(l2)]->l75));if((l318 == 0)||(l319 == 0)){*l324 = (
l313<<1)+((l319 == 0)?1:0);return(SOC_E_NONE);}}l327 = l120;while((l44[(l2)][
(l327)].l41) == 0){l327 = (l44[(l2)][(l327)].next);if(l327 == -1){l327 = l120
;break;}}while((l44[(l2)][(l327)].l41) == 0){l327 = (l44[(l2)][(l327)].l39);
if(l327 == -1){if((l44[(l2)][(l120)].l40) == 0){l325 = (l44[(l2)][(l120)].l39
);l326 = (l44[(l2)][(l120)].next);if(-1!= l325){(l44[(l2)][(l325)].next) = 
l326;}if(-1!= l326){(l44[(l2)][(l326)].l39) = l325;}}return(SOC_E_FULL);}}
while(l327>l120){l326 = (l44[(l2)][(l327)].next);SOC_IF_ERROR_RETURN(l321(l2,
l326,l10));(l44[(l2)][(l327)].l41)-= 1;(l44[(l2)][(l326)].l41)+= 1;l327 = 
l326;}while(l327<l120){SOC_IF_ERROR_RETURN(l317(l2,l327,l10));(l44[(l2)][(
l327)].l41)-= 1;l325 = (l44[(l2)][(l327)].l39);(l44[(l2)][(l325)].l41)+= 1;
l327 = l325;}(l44[(l2)][(l120)].l40)+= 1;(l44[(l2)][(l120)].l41)-= 1;(l44[(l2
)][(l120)].l38)+= 1;*l324 = (l44[(l2)][(l120)].l38)<<((l10)?0:1);sal_memcpy(
l15,soc_mem_entry_null(l2,L3_DEFIPm),soc_mem_entry_words(l2,L3_DEFIPm)*4);
return(SOC_E_NONE);}static int l329(int l2,int l120,int l10,void*l15,int l330
){int l325;int l326;int l313;int l314;uint32 l331[SOC_MAX_MEM_FIELD_WORDS];
uint32 l332[SOC_MAX_MEM_FIELD_WORDS];uint32 l333[SOC_MAX_MEM_FIELD_WORDS];
void*l334;int l128;int l335,l156;l313 = (l44[(l2)][(l120)].l38);l314 = l330;
if(!l10){l314>>= 1;if((l128 = READ_L3_DEFIPm(l2,MEM_BLOCK_ANY,l313,l331))<0){
return l128;}if((l128 = READ_L3_DEFIP_AUX_TABLEm(l2,MEM_BLOCK_ANY,
soc_alpm_physical_idx(l2,L3_DEFIPm,l313,1),l332))<0){return l128;}if((l128 = 
READ_L3_DEFIP_AUX_TABLEm(l2,MEM_BLOCK_ANY,soc_alpm_physical_idx(l2,L3_DEFIPm,
l314,1),l333))<0){return l128;}l334 = (l314 == l313)?l331:l15;if(
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l331),(l96[(
l2)]->l75))){l156 = soc_mem_field32_get(l2,L3_DEFIP_AUX_TABLEm,l332,
BPM_LENGTH1f);if(l330&1){l128 = soc_alpm_lpm_ip4entry1_to_1(l2,l331,l334,
PRESERVE_HIT);soc_mem_field32_set(l2,L3_DEFIP_AUX_TABLEm,l333,BPM_LENGTH1f,
l156);}else{l128 = soc_alpm_lpm_ip4entry1_to_0(l2,l331,l334,PRESERVE_HIT);
soc_mem_field32_set(l2,L3_DEFIP_AUX_TABLEm,l333,BPM_LENGTH0f,l156);}l335 = (
l313<<1)+1;soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(
l331),(l96[(l2)]->l75),(0));}else{l156 = soc_mem_field32_get(l2,
L3_DEFIP_AUX_TABLEm,l332,BPM_LENGTH0f);if(l330&1){l128 = 
soc_alpm_lpm_ip4entry0_to_1(l2,l331,l334,PRESERVE_HIT);soc_mem_field32_set(l2
,L3_DEFIP_AUX_TABLEm,l333,BPM_LENGTH1f,l156);}else{l128 = 
soc_alpm_lpm_ip4entry0_to_0(l2,l331,l334,PRESERVE_HIT);soc_mem_field32_set(l2
,L3_DEFIP_AUX_TABLEm,l333,BPM_LENGTH0f,l156);}l335 = l313<<1;
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l331),(l96[(
l2)]->l74),(0));(l44[(l2)][(l120)].l40)-= 1;(l44[(l2)][(l120)].l41)+= 1;(l44[
(l2)][(l120)].l38)-= 1;}l335 = soc_alpm_physical_idx(l2,L3_DEFIPm,l335,0);
l330 = soc_alpm_physical_idx(l2,L3_DEFIPm,l330,0);ALPM_TCAM_PIVOT(l2,l330) = 
ALPM_TCAM_PIVOT(l2,l335);if(ALPM_TCAM_PIVOT(l2,l330)){PIVOT_TCAM_INDEX(
ALPM_TCAM_PIVOT(l2,l330)) = l330;}ALPM_TCAM_PIVOT(l2,l335) = NULL;if((l128 = 
WRITE_L3_DEFIP_AUX_TABLEm(l2,MEM_BLOCK_ALL,soc_alpm_physical_idx(l2,L3_DEFIPm
,l314,1),l333))<0){return l128;}if(l314!= l313){l291(l2,l334,l314,0x4000,0);
if((l128 = l166(l2,MEM_BLOCK_ANY,l314,l314,l334))<0){return l128;}if((l128 = 
l307(l2,l314,l334))<0){return l128;}}l291(l2,l331,l313,0x4000,0);if((l128 = 
l166(l2,MEM_BLOCK_ANY,l313,l313,l331))<0){return l128;}if((l128 = l307(l2,
l313,l331))<0){return l128;}}else{(l44[(l2)][(l120)].l40)-= 1;(l44[(l2)][(
l120)].l41)+= 1;(l44[(l2)][(l120)].l38)-= 1;if(l314!= l313){if((l128 = 
READ_L3_DEFIPm(l2,MEM_BLOCK_ANY,l313,l331))<0){return l128;}l291(l2,l331,l314
,0x4000,0);if((l128 = l166(l2,MEM_BLOCK_ANY,l314,l313,l331))<0){return l128;}
if((l128 = l307(l2,l314,l331))<0){return l128;}}l330 = soc_alpm_physical_idx(
l2,L3_DEFIPm,l314,1);l335 = soc_alpm_physical_idx(l2,L3_DEFIPm,l313,1);
ALPM_TCAM_PIVOT(l2,l330<<1) = ALPM_TCAM_PIVOT(l2,l335<<1);ALPM_TCAM_PIVOT(l2,
l335<<1) = NULL;if(ALPM_TCAM_PIVOT(l2,l330<<1)){PIVOT_TCAM_INDEX(
ALPM_TCAM_PIVOT(l2,l330<<1)) = l330<<1;}sal_memcpy(l331,soc_mem_entry_null(l2
,L3_DEFIPm),soc_mem_entry_words(l2,L3_DEFIPm)*4);l291(l2,l331,l313,0x4000,0);
if((l128 = l166(l2,MEM_BLOCK_ANY,l313,l313,l331))<0){return l128;}if((l128 = 
l307(l2,l313,l331))<0){return l128;}}if((l44[(l2)][(l120)].l40) == 0){l325 = 
(l44[(l2)][(l120)].l39);assert(l325!= -1);l326 = (l44[(l2)][(l120)].next);(
l44[(l2)][(l325)].next) = l326;(l44[(l2)][(l325)].l41)+= (l44[(l2)][(l120)].
l41);(l44[(l2)][(l120)].l41) = 0;if(l326!= -1){(l44[(l2)][(l326)].l39) = l325
;}(l44[(l2)][(l120)].next) = -1;(l44[(l2)][(l120)].l39) = -1;(l44[(l2)][(l120
)].l37) = -1;(l44[(l2)][(l120)].l38) = -1;}return(l128);}int
soc_alpm_lpm_vrf_get(int l21,void*lpm_entry,int*l28,int*l336){int l149;if(((
l96[(l21)]->l78)!= NULL)){l149 = soc_L3_DEFIPm_field32_get(l21,lpm_entry,
VRF_ID_0f);*l336 = l149;if(soc_L3_DEFIPm_field32_get(l21,lpm_entry,
VRF_ID_MASK0f)){*l28 = l149;}else if(!soc_meminfo_fieldinfo_field32_get((&
SOC_MEM_INFO(l21,L3_DEFIPm)),(lpm_entry),(l96[(l21)]->l80))){*l28 = 
SOC_L3_VRF_GLOBAL;*l336 = SOC_VRF_MAX(l21)+1;}else{*l28 = SOC_L3_VRF_OVERRIDE
;}}else{*l28 = SOC_L3_VRF_DEFAULT;}return(SOC_E_NONE);}static int l35(int l2,
void*entry,int*l17){int l120;int l128;int l10;uint32 l129;int l149;int l337;
l10 = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(entry)
,(l96[(l2)]->l64));if(l10){l129 = soc_meminfo_fieldinfo_field32_get((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(entry),(l96[(l2)]->l62));if((l128 = _ipmask2pfx(
l129,&l120))<0){return(l128);}l129 = soc_meminfo_fieldinfo_field32_get((&
SOC_MEM_INFO(l2,L3_DEFIPm)),(entry),(l96[(l2)]->l63));if(l120){if(l129!= 
0xffffffff){return(SOC_E_PARAM);}l120+= 32;}else{if((l128 = _ipmask2pfx(l129,
&l120))<0){return(l128);}}l120+= 33;}else{l129 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(entry),(l96[
(l2)]->l62));if((l128 = _ipmask2pfx(l129,&l120))<0){return(l128);}}
SOC_IF_ERROR_RETURN(soc_alpm_lpm_vrf_get(l2,entry,&l149,&l128));l337 = 
soc_alpm_mode_get(l2);switch(l149){case SOC_L3_VRF_GLOBAL:if(l337 == 
SOC_ALPM_MODE_PARALLEL){*l17 = l120+((3*(64+32+2+1))/3);}else{*l17 = l120;}
break;case SOC_L3_VRF_OVERRIDE:*l17 = l120+2*((3*(64+32+2+1))/3);break;
default:if(l337 == SOC_ALPM_MODE_PARALLEL){*l17 = l120;}else{*l17 = l120+((3*
(64+32+2+1))/3);}break;}return(SOC_E_NONE);}static int l14(int l2,void*l8,
void*l15,int*l16,int*l17,int*l10){int l128;int l34;int l121;int l120 = 0;l34 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l2,L3_DEFIPm)),(l8),(l96[(l2
)]->l64));if(l34){if(!(l34 = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO
(l2,L3_DEFIPm)),(l8),(l96[(l2)]->l65)))){return(SOC_E_PARAM);}}*l10 = l34;l35
(l2,l8,&l120);*l17 = l120;if(l295(l2,l8,l120,&l121) == SOC_E_NONE){*l16 = 
l121;if((l128 = READ_L3_DEFIPm(l2,MEM_BLOCK_ANY,(*l10)?*l16:(*l16>>1),l15))<0
){return l128;}return(SOC_E_NONE);}else{return(SOC_E_NOT_FOUND);}}static int
l3(int l2){int l306;int l171;int l284;int l338;uint32 l339,l337;if(!
soc_feature(l2,soc_feature_lpm_tcam)){return(SOC_E_UNAVAIL);}if((l337 = 
soc_property_get(l2,spn_L3_ALPM_ENABLE,0))){SOC_IF_ERROR_RETURN(
READ_L3_DEFIP_RPF_CONTROLr(l2,&l339));soc_reg_field_set(l2,
L3_DEFIP_RPF_CONTROLr,&l339,LPM_MODEf,1);if(l337 == SOC_ALPM_MODE_PARALLEL){
soc_reg_field_set(l2,L3_DEFIP_RPF_CONTROLr,&l339,LOOKUP_MODEf,1);}else{
soc_reg_field_set(l2,L3_DEFIP_RPF_CONTROLr,&l339,LOOKUP_MODEf,0);}
SOC_IF_ERROR_RETURN(WRITE_L3_DEFIP_RPF_CONTROLr(l2,l339));l339 = 0;if(
SOC_URPF_STATUS_GET(l2)){soc_reg_field_set(l2,L3_DEFIP_KEY_SELr,&l339,
URPF_LOOKUP_CAM4f,0x1);soc_reg_field_set(l2,L3_DEFIP_KEY_SELr,&l339,
URPF_LOOKUP_CAM5f,0x1);soc_reg_field_set(l2,L3_DEFIP_KEY_SELr,&l339,
URPF_LOOKUP_CAM6f,0x1);soc_reg_field_set(l2,L3_DEFIP_KEY_SELr,&l339,
URPF_LOOKUP_CAM7f,0x1);}SOC_IF_ERROR_RETURN(WRITE_L3_DEFIP_KEY_SELr(l2,l339))
;l339 = 0;if(l337 == SOC_ALPM_MODE_PARALLEL){if(SOC_URPF_STATUS_GET(l2)){
soc_reg_field_set(l2,L3_DEFIP_ALPM_CFGr,&l339,TCAM2_SELf,1);soc_reg_field_set
(l2,L3_DEFIP_ALPM_CFGr,&l339,TCAM3_SELf,1);soc_reg_field_set(l2,
L3_DEFIP_ALPM_CFGr,&l339,TCAM4_SELf,2);soc_reg_field_set(l2,
L3_DEFIP_ALPM_CFGr,&l339,TCAM5_SELf,2);soc_reg_field_set(l2,
L3_DEFIP_ALPM_CFGr,&l339,TCAM6_SELf,3);soc_reg_field_set(l2,
L3_DEFIP_ALPM_CFGr,&l339,TCAM7_SELf,3);}else{soc_reg_field_set(l2,
L3_DEFIP_ALPM_CFGr,&l339,TCAM4_SELf,1);soc_reg_field_set(l2,
L3_DEFIP_ALPM_CFGr,&l339,TCAM5_SELf,1);soc_reg_field_set(l2,
L3_DEFIP_ALPM_CFGr,&l339,TCAM6_SELf,1);soc_reg_field_set(l2,
L3_DEFIP_ALPM_CFGr,&l339,TCAM7_SELf,1);}}else{if(SOC_URPF_STATUS_GET(l2)){
soc_reg_field_set(l2,L3_DEFIP_ALPM_CFGr,&l339,TCAM4_SELf,2);soc_reg_field_set
(l2,L3_DEFIP_ALPM_CFGr,&l339,TCAM5_SELf,2);soc_reg_field_set(l2,
L3_DEFIP_ALPM_CFGr,&l339,TCAM6_SELf,2);soc_reg_field_set(l2,
L3_DEFIP_ALPM_CFGr,&l339,TCAM7_SELf,2);}}SOC_IF_ERROR_RETURN(
WRITE_L3_DEFIP_ALPM_CFGr(l2,l339));if(soc_property_get(l2,
spn_IPV6_LPM_128B_ENABLE,1)){uint32 l340 = 0;if(l337!= SOC_ALPM_MODE_PARALLEL
){uint32 l341;l341 = soc_property_get(l2,spn_NUM_IPV6_LPM_128B_ENTRIES,2048);
if(l341!= 2048){if(SOC_URPF_STATUS_GET(l2)){LOG_CLI((BSL_META_U(l2,
"URPF supported in combined mode only""with 2048 v6-128 entries\n")));return
SOC_E_PARAM;}if((l341!= 1024)&&(l341!= 3072)){LOG_CLI((BSL_META_U(l2,
"Only supported values for v6-128 in"
"nonURPF combined mode are 1024 and 3072\n")));return SOC_E_PARAM;}}}
SOC_IF_ERROR_RETURN(READ_L3_DEFIP_KEY_SELr(l2,&l340));soc_reg_field_set(l2,
L3_DEFIP_KEY_SELr,&l340,V6_KEY_SEL_CAM0_1f,0x1);soc_reg_field_set(l2,
L3_DEFIP_KEY_SELr,&l340,V6_KEY_SEL_CAM2_3f,0x1);soc_reg_field_set(l2,
L3_DEFIP_KEY_SELr,&l340,V6_KEY_SEL_CAM4_5f,0x1);soc_reg_field_set(l2,
L3_DEFIP_KEY_SELr,&l340,V6_KEY_SEL_CAM6_7f,0x1);SOC_IF_ERROR_RETURN(
WRITE_L3_DEFIP_KEY_SELr(l2,l340));}}l306 = (3*(64+32+2+1));SOC_ALPM_LPM_LOCK(
l2);l338 = sizeof(l42)*(l306);if((l44[(l2)]!= NULL)){if(soc_alpm_deinit(l2)<0
){SOC_ALPM_LPM_UNLOCK(l2);return SOC_E_INTERNAL;}}l96[l2] = sal_alloc(sizeof(
l94),"lpm_field_state");if(NULL == l96[l2]){SOC_ALPM_LPM_UNLOCK(l2);return(
SOC_E_MEMORY);}(l96[l2])->l46 = soc_mem_fieldinfo_get(l2,L3_DEFIPm,CLASS_ID0f
);(l96[l2])->l47 = soc_mem_fieldinfo_get(l2,L3_DEFIPm,CLASS_ID1f);(l96[l2])->
l48 = soc_mem_fieldinfo_get(l2,L3_DEFIPm,DST_DISCARD0f);(l96[l2])->l49 = 
soc_mem_fieldinfo_get(l2,L3_DEFIPm,DST_DISCARD1f);(l96[l2])->l50 = 
soc_mem_fieldinfo_get(l2,L3_DEFIPm,ECMP0f);(l96[l2])->l51 = 
soc_mem_fieldinfo_get(l2,L3_DEFIPm,ECMP1f);(l96[l2])->l52 = 
soc_mem_fieldinfo_get(l2,L3_DEFIPm,ECMP_COUNT0f);(l96[l2])->l53 = 
soc_mem_fieldinfo_get(l2,L3_DEFIPm,ECMP_COUNT1f);(l96[l2])->l54 = 
soc_mem_fieldinfo_get(l2,L3_DEFIPm,ECMP_PTR0f);(l96[l2])->l55 = 
soc_mem_fieldinfo_get(l2,L3_DEFIPm,ECMP_PTR1f);(l96[l2])->l56 = 
soc_mem_fieldinfo_get(l2,L3_DEFIPm,GLOBAL_ROUTE0f);(l96[l2])->l57 = 
soc_mem_fieldinfo_get(l2,L3_DEFIPm,GLOBAL_ROUTE1f);(l96[l2])->l58 = 
soc_mem_fieldinfo_get(l2,L3_DEFIPm,HIT0f);(l96[l2])->l59 = 
soc_mem_fieldinfo_get(l2,L3_DEFIPm,HIT1f);(l96[l2])->l60 = 
soc_mem_fieldinfo_get(l2,L3_DEFIPm,IP_ADDR0f);(l96[l2])->l61 = 
soc_mem_fieldinfo_get(l2,L3_DEFIPm,IP_ADDR1f);(l96[l2])->l62 = 
soc_mem_fieldinfo_get(l2,L3_DEFIPm,IP_ADDR_MASK0f);(l96[l2])->l63 = 
soc_mem_fieldinfo_get(l2,L3_DEFIPm,IP_ADDR_MASK1f);(l96[l2])->l64 = 
soc_mem_fieldinfo_get(l2,L3_DEFIPm,MODE0f);(l96[l2])->l65 = 
soc_mem_fieldinfo_get(l2,L3_DEFIPm,MODE1f);(l96[l2])->l66 = 
soc_mem_fieldinfo_get(l2,L3_DEFIPm,MODE_MASK0f);(l96[l2])->l67 = 
soc_mem_fieldinfo_get(l2,L3_DEFIPm,MODE_MASK1f);(l96[l2])->l68 = 
soc_mem_fieldinfo_get(l2,L3_DEFIPm,NEXT_HOP_INDEX0f);(l96[l2])->l69 = 
soc_mem_fieldinfo_get(l2,L3_DEFIPm,NEXT_HOP_INDEX1f);(l96[l2])->l70 = 
soc_mem_fieldinfo_get(l2,L3_DEFIPm,PRI0f);(l96[l2])->l71 = 
soc_mem_fieldinfo_get(l2,L3_DEFIPm,PRI1f);(l96[l2])->l72 = 
soc_mem_fieldinfo_get(l2,L3_DEFIPm,RPE0f);(l96[l2])->l73 = 
soc_mem_fieldinfo_get(l2,L3_DEFIPm,RPE1f);(l96[l2])->l74 = 
soc_mem_fieldinfo_get(l2,L3_DEFIPm,VALID0f);(l96[l2])->l75 = 
soc_mem_fieldinfo_get(l2,L3_DEFIPm,VALID1f);(l96[l2])->l76 = 
soc_mem_fieldinfo_get(l2,L3_DEFIPm,VRF_ID_0f);(l96[l2])->l77 = 
soc_mem_fieldinfo_get(l2,L3_DEFIPm,VRF_ID_1f);(l96[l2])->l78 = 
soc_mem_fieldinfo_get(l2,L3_DEFIPm,VRF_ID_MASK0f);(l96[l2])->l79 = 
soc_mem_fieldinfo_get(l2,L3_DEFIPm,VRF_ID_MASK1f);(l96[l2])->l80 = 
soc_mem_fieldinfo_get(l2,L3_DEFIPm,GLOBAL_HIGH0f);(l96[l2])->l81 = 
soc_mem_fieldinfo_get(l2,L3_DEFIPm,GLOBAL_HIGH1f);(l96[l2])->l82 = 
soc_mem_fieldinfo_get(l2,L3_DEFIPm,ALG_HIT_IDX0f);(l96[l2])->l83 = 
soc_mem_fieldinfo_get(l2,L3_DEFIPm,ALG_HIT_IDX1f);(l96[l2])->l84 = 
soc_mem_fieldinfo_get(l2,L3_DEFIPm,ALG_BKT_PTR0f);(l96[l2])->l85 = 
soc_mem_fieldinfo_get(l2,L3_DEFIPm,ALG_BKT_PTR1f);(l96[l2])->l86 = 
soc_mem_fieldinfo_get(l2,L3_DEFIPm,DEFAULT_MISS0f);(l96[l2])->l87 = 
soc_mem_fieldinfo_get(l2,L3_DEFIPm,DEFAULT_MISS1f);(l96[l2])->l88 = 
soc_mem_fieldinfo_get(l2,L3_DEFIPm,FLEX_CTR_BASE_COUNTER_IDX0f);(l96[l2])->
l89 = soc_mem_fieldinfo_get(l2,L3_DEFIPm,FLEX_CTR_BASE_COUNTER_IDX1f);(l96[l2
])->l90 = soc_mem_fieldinfo_get(l2,L3_DEFIPm,FLEX_CTR_POOL_NUMBER0f);(l96[l2]
)->l91 = soc_mem_fieldinfo_get(l2,L3_DEFIPm,FLEX_CTR_POOL_NUMBER1f);(l96[l2])
->l92 = soc_mem_fieldinfo_get(l2,L3_DEFIPm,ENTRY_TYPE_MASK0f);(l96[l2])->l93 = 
soc_mem_fieldinfo_get(l2,L3_DEFIPm,ENTRY_TYPE_MASK1f);(l44[(l2)]) = sal_alloc
(l338,"LPM prefix info");if(NULL == (l44[(l2)])){sal_free(l96[l2]);l96[l2] = 
NULL;SOC_ALPM_LPM_UNLOCK(l2);return(SOC_E_MEMORY);}sal_memset((l44[(l2)]),0,
l338);for(l171 = 0;l171<l306;l171++){(l44[(l2)][(l171)].l37) = -1;(l44[(l2)][
(l171)].l38) = -1;(l44[(l2)][(l171)].l39) = -1;(l44[(l2)][(l171)].next) = -1;
(l44[(l2)][(l171)].l40) = 0;(l44[(l2)][(l171)].l41) = 0;}l284 = 
soc_mem_index_count(l2,L3_DEFIPm);if(SOC_URPF_STATUS_GET(l2)){l284>>= 1;}if(
l337 == SOC_ALPM_MODE_PARALLEL){(l44[(l2)][(((3*(64+32+2+1))-1))].l38) = (
l284>>1)-1;(l44[(l2)][(((((3*(64+32+2+1))/3)-1)))].l41) = l284>>1;(l44[(l2)][
((((3*(64+32+2+1))-1)))].l41) = (l284-(l44[(l2)][(((((3*(64+32+2+1))/3)-1)))]
.l41));}else{(l44[(l2)][((((3*(64+32+2+1))-1)))].l41) = l284;}if((l107[(l2)])
!= NULL){if(l115((l107[(l2)]))<0){SOC_ALPM_LPM_UNLOCK(l2);return
SOC_E_INTERNAL;}(l107[(l2)]) = NULL;}if(l113(l2,l284*2,l284,&(l107[(l2)]))<0)
{SOC_ALPM_LPM_UNLOCK(l2);return SOC_E_MEMORY;}SOC_ALPM_LPM_UNLOCK(l2);return(
SOC_E_NONE);}static int l4(int l2){if(!soc_feature(l2,soc_feature_lpm_tcam)){
return(SOC_E_UNAVAIL);}SOC_ALPM_LPM_LOCK(l2);if((l107[(l2)])!= NULL){l115((
l107[(l2)]));(l107[(l2)]) = NULL;}if((l44[(l2)]!= NULL)){sal_free(l96[l2]);
l96[l2] = NULL;sal_free((l44[(l2)]));(l44[(l2)]) = NULL;}SOC_ALPM_LPM_UNLOCK(
l2);return(SOC_E_NONE);}static int l5(int l2,void*l6,int*l342){int l120;int
index;int l10;uint32 l15[SOC_MAX_MEM_FIELD_WORDS];int l128 = SOC_E_NONE;int
l343 = 0;sal_memcpy(l15,soc_mem_entry_null(l2,L3_DEFIPm),soc_mem_entry_words(
l2,L3_DEFIPm)*4);SOC_ALPM_LPM_LOCK(l2);l128 = l14(l2,l6,l15,&index,&l120,&l10
);if(l128 == SOC_E_NOT_FOUND){l128 = l323(l2,l120,l10,l15,&index);if(l128<0){
SOC_ALPM_LPM_UNLOCK(l2);return(l128);}}else{l343 = 1;}*l342 = index;if(l128 == 
SOC_E_NONE){if(!l10){if(index&1){l128 = soc_alpm_lpm_ip4entry0_to_1(l2,l6,l15
,PRESERVE_HIT);}else{l128 = soc_alpm_lpm_ip4entry0_to_0(l2,l6,l15,
PRESERVE_HIT);}if(l128<0){SOC_ALPM_LPM_UNLOCK(l2);return(l128);}l6 = (void*)
l15;index>>= 1;}l1(l2);LOG_INFO(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"\nsoc_alpm_lpm_insert: %d %d\n"),index,l120));if(!l343){l291(l2,l6,index,
0x4000,0);}l128 = l166(l2,MEM_BLOCK_ANY,index,index,l6);if(l128>= 0){l128 = 
l307(l2,index,l6);}}SOC_ALPM_LPM_UNLOCK(l2);return(l128);}static int l7(int l2
,void*l8){int l120;int index;int l10;uint32 l15[SOC_MAX_MEM_FIELD_WORDS];int
l128 = SOC_E_NONE;SOC_ALPM_LPM_LOCK(l2);l128 = l14(l2,l8,l15,&index,&l120,&
l10);if(l128 == SOC_E_NONE){LOG_INFO(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"\nsoc_alpm_lpm_delete: %d %d\n"),index,l120));l294(l2,l8,index);l128 = l329(
l2,l120,l10,l15,index);}l1(l2);SOC_ALPM_LPM_UNLOCK(l2);return(l128);}static
int l18(int l2,void*l8,void*l15,int*l16){int l120;int l128;int l10;
SOC_ALPM_LPM_LOCK(l2);l128 = l14(l2,l8,l15,l16,&l120,&l10);
SOC_ALPM_LPM_UNLOCK(l2);return(l128);}static int l9(int l21,void*l8,int l10,
int l11,int l12,defip_aux_scratch_entry_t*l13){uint32 l129;uint32 l344[4] = {
0,0,0,0};int l120 = 0;int l128 = SOC_E_NONE;l129 = soc_mem_field32_get(l21,
L3_DEFIPm,l8,VALID0f);soc_mem_field32_set(l21,L3_DEFIP_AUX_SCRATCHm,l13,
VALIDf,l129);l129 = soc_mem_field32_get(l21,L3_DEFIPm,l8,MODE0f);
soc_mem_field32_set(l21,L3_DEFIP_AUX_SCRATCHm,l13,MODEf,l129);l129 = 
soc_mem_field32_get(l21,L3_DEFIPm,l8,ENTRY_TYPE0f);soc_mem_field32_set(l21,
L3_DEFIP_AUX_SCRATCHm,l13,ENTRY_TYPEf,0);l129 = soc_mem_field32_get(l21,
L3_DEFIPm,l8,GLOBAL_ROUTE0f);soc_mem_field32_set(l21,L3_DEFIP_AUX_SCRATCHm,
l13,GLOBAL_ROUTEf,l129);l129 = soc_mem_field32_get(l21,L3_DEFIPm,l8,ECMP0f);
soc_mem_field32_set(l21,L3_DEFIP_AUX_SCRATCHm,l13,ECMPf,l129);l129 = 
soc_mem_field32_get(l21,L3_DEFIPm,l8,ECMP_PTR0f);soc_mem_field32_set(l21,
L3_DEFIP_AUX_SCRATCHm,l13,ECMP_PTRf,l129);l129 = soc_mem_field32_get(l21,
L3_DEFIPm,l8,NEXT_HOP_INDEX0f);soc_mem_field32_set(l21,L3_DEFIP_AUX_SCRATCHm,
l13,NEXT_HOP_INDEXf,l129);l129 = soc_mem_field32_get(l21,L3_DEFIPm,l8,PRI0f);
soc_mem_field32_set(l21,L3_DEFIP_AUX_SCRATCHm,l13,PRIf,l129);l129 = 
soc_mem_field32_get(l21,L3_DEFIPm,l8,RPE0f);soc_mem_field32_set(l21,
L3_DEFIP_AUX_SCRATCHm,l13,RPEf,l129);l129 =soc_mem_field32_get(l21,L3_DEFIPm,
l8,VRF_ID_0f);soc_mem_field32_set(l21,L3_DEFIP_AUX_SCRATCHm,l13,VRFf,l129);
soc_mem_field32_set(l21,L3_DEFIP_AUX_SCRATCHm,l13,DB_TYPEf,l11);l129 = 
soc_mem_field32_get(l21,L3_DEFIPm,l8,DST_DISCARD0f);soc_mem_field32_set(l21,
L3_DEFIP_AUX_SCRATCHm,l13,DST_DISCARDf,l129);l129 = soc_mem_field32_get(l21,
L3_DEFIPm,l8,CLASS_ID0f);soc_mem_field32_set(l21,L3_DEFIP_AUX_SCRATCHm,l13,
CLASS_IDf,l129);if(l10){l344[2] = soc_mem_field32_get(l21,L3_DEFIPm,l8,
IP_ADDR0f);l344[3] = soc_mem_field32_get(l21,L3_DEFIPm,l8,IP_ADDR1f);}else{
l344[0] = soc_mem_field32_get(l21,L3_DEFIPm,l8,IP_ADDR0f);}soc_mem_field_set(
l21,L3_DEFIP_AUX_SCRATCHm,(uint32*)l13,IP_ADDRf,(uint32*)l344);if(l10){l129 = 
soc_mem_field32_get(l21,L3_DEFIPm,l8,IP_ADDR_MASK0f);if((l128 = _ipmask2pfx(
l129,&l120))<0){return(l128);}l129 = soc_mem_field32_get(l21,L3_DEFIPm,l8,
IP_ADDR_MASK1f);if(l120){if(l129!= 0xffffffff){return(SOC_E_PARAM);}l120+= 32
;}else{if((l128 = _ipmask2pfx(l129,&l120))<0){return(l128);}}}else{l129 = 
soc_mem_field32_get(l21,L3_DEFIPm,l8,IP_ADDR_MASK0f);if((l128 = _ipmask2pfx(
l129,&l120))<0){return(l128);}}soc_mem_field32_set(l21,L3_DEFIP_AUX_SCRATCHm,
l13,IP_LENGTHf,l120);soc_mem_field32_set(l21,L3_DEFIP_AUX_SCRATCHm,l13,
REPLACE_LENf,l12);return(SOC_E_NONE);}int _soc_alpm_aux_op(int l2,
_soc_aux_op_t l345,defip_aux_scratch_entry_t*l13,int l346,int*l142,int*l143,
int*bucket_index){uint32 l339,l347;int l348;soc_timeout_t l349;int l128 = 
SOC_E_NONE;int l350 = 0;if(l346){SOC_IF_ERROR_RETURN(
WRITE_L3_DEFIP_AUX_SCRATCHm(l2,MEM_BLOCK_ANY,0,l13));}l351:l339 = 0;switch(
l345){case INSERT_PROPAGATE:l348 = 0;break;case DELETE_PROPAGATE:l348 = 1;
break;case PREFIX_LOOKUP:l348 = 2;break;case HITBIT_REPLACE:l348 = 3;break;
default:return SOC_E_PARAM;}soc_reg_field_set(l2,L3_DEFIP_AUX_CTRLr,&l339,
OPCODEf,l348);soc_reg_field_set(l2,L3_DEFIP_AUX_CTRLr,&l339,STARTf,1);
SOC_IF_ERROR_RETURN(WRITE_L3_DEFIP_AUX_CTRLr(l2,l339));soc_timeout_init(&l349
,50000,5);l348 = 0;do{SOC_IF_ERROR_RETURN(READ_L3_DEFIP_AUX_CTRLr(l2,&l339));
l348 = soc_reg_field_get(l2,L3_DEFIP_AUX_CTRLr,l339,DONEf);if(l348 == 1){l128
= SOC_E_NONE;break;}if(soc_timeout_check(&l349)){SOC_IF_ERROR_RETURN(
READ_L3_DEFIP_AUX_CTRLr(l2,&l339));l348 = soc_reg_field_get(l2,
L3_DEFIP_AUX_CTRLr,l339,DONEf);if(l348 == 1){l128 = SOC_E_NONE;}else{LOG_WARN
(BSL_LS_SOC_ALPM,(BSL_META_U(l2,"unit %d : DEFIP AUX Operation timeout\n"),l2
));l128 = SOC_E_TIMEOUT;}break;}}while(1);if(SOC_SUCCESS(l128)){if(
soc_reg_field_get(l2,L3_DEFIP_AUX_CTRLr,l339,ERRORf)){soc_reg_field_set(l2,
L3_DEFIP_AUX_CTRLr,&l339,STARTf,0);soc_reg_field_set(l2,L3_DEFIP_AUX_CTRLr,&
l339,ERRORf,0);soc_reg_field_set(l2,L3_DEFIP_AUX_CTRLr,&l339,DONEf,0);
SOC_IF_ERROR_RETURN(WRITE_L3_DEFIP_AUX_CTRLr(l2,l339));LOG_WARN(
BSL_LS_SOC_ALPM,(BSL_META_U(l2,"unit %d: DEFIP AUX Operation encountered "
"parity error !!\n"),l2));l350++;if(SOC_CONTROL(l2)->alpm_bulk_retry){
sal_sem_take(SOC_CONTROL(l2)->alpm_bulk_retry,1000000);}if(l350<5){LOG_WARN(
BSL_LS_SOC_ALPM,(BSL_META_U(l2,"unit %d: Retry DEFIP AUX Operation..\n"),l2))
;goto l351;}else{LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l2,
"unit %d: Aborting DEFIP AUX Operation ""due to un-correctable error !!\n"),
l2));return SOC_E_INTERNAL;}}if(l345 == PREFIX_LOOKUP){if(l142&&l143){*l142 = 
soc_reg_field_get(l2,L3_DEFIP_AUX_CTRLr,l339,HITf);*l143 = soc_reg_field_get(
l2,L3_DEFIP_AUX_CTRLr,l339,BKT_INDEXf);SOC_IF_ERROR_RETURN(
READ_L3_DEFIP_AUX_CTRL_1r(l2,&l347));*bucket_index = soc_reg_field_get(l2,
L3_DEFIP_AUX_CTRL_1r,l347,BKT_PTRf);}}}return l128;}static int l20(int l21,
void*lpm_entry,void*l22,void*l23,soc_mem_t l24,uint32 l25,uint32*l352){uint32
l129;uint32 l344[4] = {0,0};int l120 = 0;int l128 = SOC_E_NONE;int l10;uint32
l26 = 0;l10 = soc_mem_field32_get(l21,L3_DEFIPm,lpm_entry,MODE0f);sal_memset(
l22,0,soc_mem_entry_words(l21,l24)*4);l129 = soc_mem_field32_get(l21,
L3_DEFIPm,lpm_entry,HIT0f);soc_mem_field32_set(l21,l24,l22,HITf,l129);l129 = 
soc_mem_field32_get(l21,L3_DEFIPm,lpm_entry,VALID0f);soc_mem_field32_set(l21,
l24,l22,VALIDf,l129);l129 = soc_mem_field32_get(l21,L3_DEFIPm,lpm_entry,
ECMP0f);soc_mem_field32_set(l21,l24,l22,ECMPf,l129);l129 = 
soc_mem_field32_get(l21,L3_DEFIPm,lpm_entry,ECMP_PTR0f);soc_mem_field32_set(
l21,l24,l22,ECMP_PTRf,l129);l129 = soc_mem_field32_get(l21,L3_DEFIPm,
lpm_entry,NEXT_HOP_INDEX0f);soc_mem_field32_set(l21,l24,l22,NEXT_HOP_INDEXf,
l129);l129 = soc_mem_field32_get(l21,L3_DEFIPm,lpm_entry,PRI0f);
soc_mem_field32_set(l21,l24,l22,PRIf,l129);l129 = soc_mem_field32_get(l21,
L3_DEFIPm,lpm_entry,RPE0f);soc_mem_field32_set(l21,l24,l22,RPEf,l129);l129 = 
soc_mem_field32_get(l21,L3_DEFIPm,lpm_entry,DST_DISCARD0f);
soc_mem_field32_set(l21,l24,l22,DST_DISCARDf,l129);l129 = soc_mem_field32_get
(l21,L3_DEFIPm,lpm_entry,SRC_DISCARD0f);soc_mem_field32_set(l21,l24,l22,
SRC_DISCARDf,l129);l129 = soc_mem_field32_get(l21,L3_DEFIPm,lpm_entry,
CLASS_ID0f);soc_mem_field32_set(l21,l24,l22,CLASS_IDf,l129);l344[0] = 
soc_mem_field32_get(l21,L3_DEFIPm,lpm_entry,IP_ADDR0f);if(l10){l344[1] = 
soc_mem_field32_get(l21,L3_DEFIPm,lpm_entry,IP_ADDR1f);}soc_mem_field_set(l21
,l24,(uint32*)l22,KEYf,(uint32*)l344);if(l10){l129 = soc_mem_field32_get(l21,
L3_DEFIPm,lpm_entry,IP_ADDR_MASK0f);if((l128 = _ipmask2pfx(l129,&l120))<0){
return(l128);}l129 = soc_mem_field32_get(l21,L3_DEFIPm,lpm_entry,
IP_ADDR_MASK1f);if(l120){if(l129!= 0xffffffff){return(SOC_E_PARAM);}l120+= 32
;}else{if((l128 = _ipmask2pfx(l129,&l120))<0){return(l128);}}}else{l129 = 
soc_mem_field32_get(l21,L3_DEFIPm,lpm_entry,IP_ADDR_MASK0f);if((l128 = 
_ipmask2pfx(l129,&l120))<0){return(l128);}}if((l120 == 0)&&(l344[0] == 0)&&(
l344[1] == 0)){l26 = 1;}if(l352!= NULL){*l352 = l26;}soc_mem_field32_set(l21,
l24,l22,LENGTHf,l120);if(l23 == NULL){return(SOC_E_NONE);}if(
SOC_URPF_STATUS_GET(l21)){sal_memset(l23,0,soc_mem_entry_words(l21,l24)*4);
sal_memcpy(l23,l22,soc_mem_entry_words(l21,l24)*4);soc_mem_field32_set(l21,
l24,l23,DST_DISCARDf,0);soc_mem_field32_set(l21,l24,l23,RPEf,0);
soc_mem_field32_set(l21,l24,l23,SRC_DISCARDf,l25&SOC_ALPM_RPF_SRC_DISCARD);
soc_mem_field32_set(l21,l24,l23,DEFAULTROUTEf,l26);}return(SOC_E_NONE);}
static int l27(int l21,void*l22,soc_mem_t l24,int l10,int l28,int l29,int
index,void*lpm_entry){uint32 l129;uint32 l344[4] = {0,0};uint32 l120 = 0;
sal_memset(lpm_entry,0,soc_mem_entry_words(l21,L3_DEFIPm)*4);l129 = 
soc_mem_field32_get(l21,l24,l22,HITf);soc_mem_field32_set(l21,L3_DEFIPm,
lpm_entry,HIT0f,l129);if(l10){soc_mem_field32_set(l21,L3_DEFIPm,lpm_entry,
HIT1f,l129);}l129 = soc_mem_field32_get(l21,l24,l22,VALIDf);
soc_mem_field32_set(l21,L3_DEFIPm,lpm_entry,VALID0f,l129);if(l10){
soc_mem_field32_set(l21,L3_DEFIPm,lpm_entry,VALID1f,l129);}l129 = 
soc_mem_field32_get(l21,l24,l22,ECMPf);soc_mem_field32_set(l21,L3_DEFIPm,
lpm_entry,ECMP0f,l129);l129 = soc_mem_field32_get(l21,l24,l22,ECMP_PTRf);
soc_mem_field32_set(l21,L3_DEFIPm,lpm_entry,ECMP_PTR0f,l129);l129 = 
soc_mem_field32_get(l21,l24,l22,NEXT_HOP_INDEXf);soc_mem_field32_set(l21,
L3_DEFIPm,lpm_entry,NEXT_HOP_INDEX0f,l129);l129 = soc_mem_field32_get(l21,l24
,l22,PRIf);soc_mem_field32_set(l21,L3_DEFIPm,lpm_entry,PRI0f,l129);l129 = 
soc_mem_field32_get(l21,l24,l22,RPEf);soc_mem_field32_set(l21,L3_DEFIPm,
lpm_entry,RPE0f,l129);l129 = soc_mem_field32_get(l21,l24,l22,DST_DISCARDf);
soc_mem_field32_set(l21,L3_DEFIPm,lpm_entry,DST_DISCARD0f,l129);l129 = 
soc_mem_field32_get(l21,l24,l22,SRC_DISCARDf);soc_mem_field32_set(l21,
L3_DEFIPm,lpm_entry,SRC_DISCARD0f,l129);l129 = soc_mem_field32_get(l21,l24,
l22,CLASS_IDf);soc_mem_field32_set(l21,L3_DEFIPm,lpm_entry,CLASS_ID0f,l129);
soc_mem_field32_set(l21,L3_DEFIPm,lpm_entry,ALG_BKT_PTR0f,l29);
soc_mem_field32_set(l21,L3_DEFIPm,lpm_entry,ALG_HIT_IDX0f,index);
soc_mem_field32_set(l21,L3_DEFIPm,lpm_entry,MODE_MASK0f,3);
soc_mem_field32_set(l21,L3_DEFIPm,lpm_entry,ENTRY_TYPE_MASK0f,1);if(l10){
soc_mem_field32_set(l21,L3_DEFIPm,lpm_entry,MODE0f,1);}soc_mem_field_get(l21,
l24,l22,KEYf,l344);if(l10){soc_mem_field32_set(l21,L3_DEFIPm,lpm_entry,
IP_ADDR1f,l344[1]);}soc_mem_field32_set(l21,L3_DEFIPm,lpm_entry,IP_ADDR0f,
l344[0]);l129 = soc_mem_field32_get(l21,l24,l22,LENGTHf);if(l10){if(l129>= 32
){l120 = 0xffffffff;soc_mem_field32_set(l21,L3_DEFIPm,lpm_entry,
IP_ADDR_MASK1f,l120);l120 = ~(((l129-32) == 32)?0:(0xffffffff)>>(l129-32));
soc_mem_field32_set(l21,L3_DEFIPm,lpm_entry,IP_ADDR_MASK0f,l120);}else{l120 = 
~(0xffffffff>>l129);soc_mem_field32_set(l21,L3_DEFIPm,lpm_entry,
IP_ADDR_MASK1f,l120);}}else{assert(l129<= 32);l120 = ~(((l129) == 32)?0:(
0xffffffff)>>(l129));soc_mem_field32_set(l21,L3_DEFIPm,lpm_entry,
IP_ADDR_MASK0f,l120);}if(l28 == SOC_L3_VRF_OVERRIDE){soc_mem_field32_set(l21,
L3_DEFIPm,lpm_entry,GLOBAL_HIGH0f,1);soc_mem_field32_set(l21,L3_DEFIPm,
lpm_entry,GLOBAL_ROUTE0f,1);soc_mem_field32_set(l21,L3_DEFIPm,lpm_entry,
VRF_ID_0f,0);soc_mem_field32_set(l21,L3_DEFIPm,lpm_entry,VRF_ID_MASK0f,0);}
else if(l28 == SOC_L3_VRF_GLOBAL){soc_mem_field32_set(l21,L3_DEFIPm,lpm_entry
,GLOBAL_ROUTE0f,1);soc_mem_field32_set(l21,L3_DEFIPm,lpm_entry,VRF_ID_0f,0);
soc_mem_field32_set(l21,L3_DEFIPm,lpm_entry,VRF_ID_MASK0f,0);}else{
soc_mem_field32_set(l21,L3_DEFIPm,lpm_entry,VRF_ID_0f,l28);
soc_mem_field32_set(l21,L3_DEFIPm,lpm_entry,VRF_ID_MASK0f,SOC_VRF_MAX(l21));}
return(SOC_E_NONE);}int soc_alpm_warmboot_pivot_add(int l21,int l10,void*
lpm_entry,int l353,int l354){int l128 = SOC_E_NONE;uint32 key[2] = {0,0};
alpm_pivot_t*l204 = NULL;alpm_bucket_handle_t*l213 = NULL;int l149 = 0,l28 = 
0;uint32 l355;trie_t*l250 = NULL;uint32 prefix[5] = {0};int l26 = 0;l128 = 
l134(l21,lpm_entry,prefix,&l355,&l26);SOC_IF_ERROR_RETURN(l128);
SOC_IF_ERROR_RETURN(soc_alpm_lpm_vrf_get(l21,lpm_entry,&l149,&l28));l353 = 
soc_alpm_physical_idx(l21,L3_DEFIPm,l353,l10);l213 = sal_alloc(sizeof(
alpm_bucket_handle_t),"ALPM Bucket Handle");if(l213 == NULL){LOG_ERROR(
BSL_LS_SOC_ALPM,(BSL_META_U(l21,"Unable to allocate memory for "
"PIVOT trie node \n")));return SOC_E_NONE;}sal_memset(l213,0,sizeof(*l213));
l204 = sal_alloc(sizeof(alpm_pivot_t),"Payload for Pivot");if(l204 == NULL){
LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l21,"Unable to allocate memory for "
"PIVOT trie node \n")));sal_free(l213);return SOC_E_MEMORY;}sal_memset(l204,0
,sizeof(*l204));PIVOT_BUCKET_HANDLE(l204) = l213;if(l10){trie_init(
_MAX_KEY_LEN_144_,&PIVOT_BUCKET_TRIE(l204));key[0] = 
soc_L3_DEFIPm_field32_get(l21,lpm_entry,IP_ADDR0f);key[1] = 
soc_L3_DEFIPm_field32_get(l21,lpm_entry,IP_ADDR1f);}else{trie_init(
_MAX_KEY_LEN_48_,&PIVOT_BUCKET_TRIE(l204));key[0] = soc_L3_DEFIPm_field32_get
(l21,lpm_entry,IP_ADDR0f);}PIVOT_BUCKET_INDEX(l204) = l354;PIVOT_TCAM_INDEX(
l204) = l353;if(l149!= SOC_L3_VRF_OVERRIDE){if(l10 == 0){l250 = 
VRF_PIVOT_TRIE_IPV4(l21,l28);if(l250 == NULL){trie_init(_MAX_KEY_LEN_48_,&
VRF_PIVOT_TRIE_IPV4(l21,l28));l250 = VRF_PIVOT_TRIE_IPV4(l21,l28);}}else{l250
= VRF_PIVOT_TRIE_IPV6(l21,l28);if(l250 == NULL){trie_init(_MAX_KEY_LEN_144_,&
VRF_PIVOT_TRIE_IPV6(l21,l28));l250 = VRF_PIVOT_TRIE_IPV6(l21,l28);}}
sal_memcpy(l204->key,prefix,sizeof(prefix));l204->len = l355;l128 = 
trie_insert(l250,l204->key,NULL,l204->len,(trie_node_t*)l204);if(SOC_FAILURE(
l128)){sal_free(l213);sal_free(l204);return l128;}}ALPM_TCAM_PIVOT(l21,l353) = 
l204;PIVOT_BUCKET_VRF(l204) = l28;PIVOT_BUCKET_IPV6(l204) = l10;
PIVOT_BUCKET_ENT_CNT_UPDATE(l204);if(key[0] == 0&&key[1] == 0){
PIVOT_BUCKET_DEF(l204) = TRUE;}VRF_PIVOT_REF_INC(l21,l28,l10);return l128;}
static int l356(int l21,int l10,void*lpm_entry,void*l22,soc_mem_t l24,int l353
,int l354,int l357){int l358;int l28;int l128 = SOC_E_NONE;int l26 = 0;uint32
prefix[5] = {0,0,0,0,0};uint32 l141;void*l359 = NULL;trie_t*l360 = NULL;
trie_t*l207 = NULL;trie_node_t*l209 = NULL;payload_t*l361 = NULL;payload_t*
l211 = NULL;alpm_pivot_t*l146 = NULL;if((NULL == lpm_entry)||(NULL == l22)){
return SOC_E_PARAM;}if(l10){if(!(l10 = soc_mem_field32_get(l21,L3_DEFIPm,
lpm_entry,MODE1f))){return(SOC_E_PARAM);}}SOC_IF_ERROR_RETURN(
soc_alpm_lpm_vrf_get(l21,lpm_entry,&l358,&l28));l24 = (l10)?
L3_DEFIP_ALPM_IPV6_64m:L3_DEFIP_ALPM_IPV4m;l359 = sal_alloc(sizeof(
defip_entry_t),"Temp lpm_entr");if(NULL == l359){return SOC_E_MEMORY;}
SOC_IF_ERROR_RETURN(l27(l21,l22,l24,l10,l358,l354,l353,l359));l128 = l134(l21
,l359,prefix,&l141,&l26);if(SOC_FAILURE(l128)){LOG_ERROR(BSL_LS_SOC_ALPM,(
BSL_META_U(l21,"prefix create failed\n")));return l128;}sal_free(l359);l146 = 
ALPM_TCAM_PIVOT(l21,l353);l360 = PIVOT_BUCKET_TRIE(l146);l361 = sal_alloc(
sizeof(payload_t),"Payload for Key");if(NULL == l361){LOG_ERROR(
BSL_LS_SOC_ALPM,(BSL_META_U(l21,"Unable to allocate memory for trie node.\n")
));return SOC_E_MEMORY;}l211 = sal_alloc(sizeof(payload_t),
"Payload for pfx trie key");if(NULL == l211){LOG_ERROR(BSL_LS_SOC_ALPM,(
BSL_META_U(l21,"Unable to allocate memory for pfx trie node\n")));sal_free(
l361);return SOC_E_MEMORY;}sal_memset(l361,0,sizeof(*l361));sal_memset(l211,0
,sizeof(*l211));l361->key[0] = prefix[0];l361->key[1] = prefix[1];l361->key[2
] = prefix[2];l361->key[3] = prefix[3];l361->key[4] = prefix[4];l361->len = 
l141;l361->index = l357;sal_memcpy(l211,l361,sizeof(*l361));l128 = 
trie_insert(l360,prefix,NULL,l141,(trie_node_t*)l361);if(SOC_FAILURE(l128)){
goto l362;}if(l10){l207 = VRF_PREFIX_TRIE_IPV6(l21,l28);}else{l207 = 
VRF_PREFIX_TRIE_IPV4(l21,l28);}if(!l26){l128 = trie_insert(l207,prefix,NULL,
l141,(trie_node_t*)l211);if(SOC_FAILURE(l128)){goto l226;}}return l128;l226:(
void)trie_delete(l360,prefix,l141,&l209);l361 = (payload_t*)l209;l362:
sal_free(l361);sal_free(l211);return l128;}static int l363(int l21,int l34,
int l28,int l130,int bkt_ptr){int l128 = SOC_E_NONE;uint32 l141;uint32 key[2]
= {0,0};trie_t*l364 = NULL;payload_t*l240 = NULL;defip_entry_t*lpm_entry = 
NULL;lpm_entry = sal_alloc(sizeof(defip_entry_t),"Default LPM entry");if(
lpm_entry == NULL){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l21,
"unable to allocate memory for LPM entry\n")));return SOC_E_MEMORY;}l30(l21,
key,0,l28,l34,lpm_entry,0,1);if(l28 == SOC_VRF_MAX(l21)+1){
soc_L3_DEFIPm_field32_set(l21,lpm_entry,GLOBAL_ROUTE0f,1);}else{
soc_L3_DEFIPm_field32_set(l21,lpm_entry,DEFAULT_MISS0f,1);}
soc_L3_DEFIPm_field32_set(l21,lpm_entry,ALG_BKT_PTR0f,bkt_ptr);if(l34 == 0){
VRF_TRIE_DEFAULT_ROUTE_IPV4(l21,l28) = lpm_entry;trie_init(_MAX_KEY_LEN_48_,&
VRF_PREFIX_TRIE_IPV4(l21,l28));l364 = VRF_PREFIX_TRIE_IPV4(l21,l28);}else{
VRF_TRIE_DEFAULT_ROUTE_IPV6(l21,l28) = lpm_entry;trie_init(_MAX_KEY_LEN_144_,
&VRF_PREFIX_TRIE_IPV6(l21,l28));l364 = VRF_PREFIX_TRIE_IPV6(l21,l28);}l240 = 
sal_alloc(sizeof(payload_t),"Payload for pfx trie key");if(l240 == NULL){
LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l21,
"Unable to allocate memory for pfx trie node \n")));return SOC_E_MEMORY;}
sal_memset(l240,0,sizeof(*l240));l141 = 0;l240->key[0] = key[0];l240->key[1] = 
key[1];l240->len = l141;l128 = trie_insert(l364,key,NULL,l141,&(l240->node));
if(SOC_FAILURE(l128)){sal_free(l240);return l128;}VRF_TRIE_INIT_DONE(l21,l28,
l34,1);return l128;}int soc_alpm_warmboot_prefix_insert(int l21,int l10,void*
lpm_entry,void*l22,int l353,int l354,int l357){int l358;int l28;int l128 = 
SOC_E_NONE;soc_mem_t l24;l353 = soc_alpm_physical_idx(l21,L3_DEFIPm,l353,l10)
;l24 = (l10)?L3_DEFIP_ALPM_IPV6_64m:L3_DEFIP_ALPM_IPV4m;SOC_IF_ERROR_RETURN(
soc_alpm_lpm_vrf_get(l21,lpm_entry,&l358,&l28));if(l358 == 
SOC_L3_VRF_OVERRIDE){return(l128);}if(!VRF_TRIE_INIT_COMPLETED(l21,l28,l10)){
LOG_VERBOSE(BSL_LS_SOC_ALPM,(BSL_META_U(l21,"VRF %d is not initialized\n"),
l28));l128 = l363(l21,l10,l28,l353,l354);if(SOC_FAILURE(l128)){LOG_ERROR(
BSL_LS_SOC_ALPM,(BSL_META_U(l21,"VRF %d/%d trie init \n""failed\n"),l28,l10))
;return l128;}LOG_VERBOSE(BSL_LS_SOC_ALPM,(BSL_META_U(l21,
"VRF %d/%d trie init completed\n"),l28,l10));}l128 = l356(l21,l10,lpm_entry,
l22,l24,l353,l354,l357);if(l128!= SOC_E_NONE){LOG_WARN(BSL_LS_SOC_ALPM,(
BSL_META_U(l21,"unit %d : Route Insertion Failed :%s\n"),l21,soc_errmsg(l128)
));return(l128);}VRF_TRIE_ROUTES_INC(l21,l28,l10);return(l128);}int
soc_alpm_warmboot_bucket_bitmap_set(int l2,int l34,int l253){int l285 = 1;if(
l34){if(!soc_alpm_mode_get(l2)&&!SOC_URPF_STATUS_GET(l2)){l285 = 2;}}if(
SOC_ALPM_BUCKET_BMAP(l2) == NULL){return SOC_E_INTERNAL;}SHR_BITSET_RANGE(
SOC_ALPM_BUCKET_BMAP(l2),l253,l285);return SOC_E_NONE;}int
soc_alpm_warmboot_lpm_reinit_done(int l21){int l130;int l365 = ((3*(64+32+2+1
))-1);int l284 = soc_mem_index_count(l21,L3_DEFIPm);if(SOC_URPF_STATUS_GET(
l21)){l284>>= 1;}if(!soc_alpm_mode_get(l21)){(l44[(l21)][(((3*(64+32+2+1))-1)
)].l39) = -1;for(l130 = ((3*(64+32+2+1))-1);l130>-1;l130--){if(-1 == (l44[(
l21)][(l130)].l37)){continue;}(l44[(l21)][(l130)].l39) = l365;(l44[(l21)][(
l365)].next) = l130;(l44[(l21)][(l365)].l41) = (l44[(l21)][(l130)].l37)-(l44[
(l21)][(l365)].l38)-1;l365 = l130;}(l44[(l21)][(l365)].next) = -1;(l44[(l21)]
[(l365)].l41) = l284-(l44[(l21)][(l365)].l38)-1;}else{(l44[(l21)][(((3*(64+32
+2+1))-1))].l39) = -1;for(l130 = ((3*(64+32+2+1))-1);l130>(((3*(64+32+2+1))-1
)/3);l130--){if(-1 == (l44[(l21)][(l130)].l37)){continue;}(l44[(l21)][(l130)]
.l39) = l365;(l44[(l21)][(l365)].next) = l130;(l44[(l21)][(l365)].l41) = (l44
[(l21)][(l130)].l37)-(l44[(l21)][(l365)].l38)-1;l365 = l130;}(l44[(l21)][(
l365)].next) = -1;(l44[(l21)][(l365)].l41) = l284-(l44[(l21)][(l365)].l38)-1;
l365 = (((3*(64+32+2+1))-1)/3);(l44[(l21)][((((3*(64+32+2+1))-1)/3))].l39) = 
-1;for(l130 = ((((3*(64+32+2+1))-1)/3)-1);l130>-1;l130--){if(-1 == (l44[(l21)
][(l130)].l37)){continue;}(l44[(l21)][(l130)].l39) = l365;(l44[(l21)][(l365)]
.next) = l130;(l44[(l21)][(l365)].l41) = (l44[(l21)][(l130)].l37)-(l44[(l21)]
[(l365)].l38)-1;l365 = l130;}(l44[(l21)][(l365)].next) = -1;(l44[(l21)][(l365
)].l41) = (l284>>1)-(l44[(l21)][(l365)].l38)-1;}return(SOC_E_NONE);}int
soc_alpm_warmboot_lpm_reinit(int l21,int l10,int l130,void*lpm_entry){int l17
;defip_entry_t*l366;if(soc_L3_DEFIPm_field32_get(l21,lpm_entry,VALID0f)||
soc_L3_DEFIPm_field32_get(l21,lpm_entry,VALID1f)){l291(l21,lpm_entry,l130,
0x4000,0);}if(soc_L3_DEFIPm_field32_get(l21,lpm_entry,VALID0f)){
SOC_IF_ERROR_RETURN(l35(l21,lpm_entry,&l17));if((l44[(l21)][(l17)].l40) == 0)
{(l44[(l21)][(l17)].l37) = l130;(l44[(l21)][(l17)].l38) = l130;}else{(l44[(
l21)][(l17)].l38) = l130;}(l44[(l21)][(l17)].l40)++;if(l10){return(SOC_E_NONE
);}}else{if(soc_L3_DEFIPm_field32_get(l21,lpm_entry,VALID1f)){l366 = 
sal_alloc(sizeof(defip_entry_t),"lpm_entry_hi");soc_alpm_lpm_ip4entry1_to_0(
l21,lpm_entry,l366,TRUE);SOC_IF_ERROR_RETURN(l35(l21,l366,&l17));if((l44[(l21
)][(l17)].l40) == 0){(l44[(l21)][(l17)].l37) = l130;(l44[(l21)][(l17)].l38) = 
l130;}else{(l44[(l21)][(l17)].l38) = l130;}sal_free(l366);(l44[(l21)][(l17)].
l40)++;}}return(SOC_E_NONE);}typedef struct l367{int v4;int v6_64;int v6_128;
int l368;int l369;int l370;int l286;}l371;typedef enum l372{l373 = 0,l374,
l375,l376,l377,l378}l379;static void l380(int l2,alpm_vrf_counter_t*l381){
l381->v4 = soc_mem_index_count(l2,L3_DEFIPm)*2;l381->v6_128 = 
soc_mem_index_count(l2,L3_DEFIP_PAIR_128m);if(soc_property_get(l2,
spn_IPV6_LPM_128B_ENABLE,1)){l381->v6_64 = l381->v6_128;}else{l381->v6_64 = 
l381->v4>>1;}if(SOC_URPF_STATUS_GET(l2)){l381->v4>>= 1;l381->v6_128>>= 1;l381
->v6_64>>= 1;}}static void l382(int l2,int l149,alpm_vrf_handle_t*l383,l379
l384){alpm_vrf_counter_t*l385;int l171,l386,l387,l388;int l343 = 0;
alpm_vrf_counter_t l381;switch(l384){case l373:LOG_CLI((BSL_META_U(l2,
"\nAdd Counter:\n")));break;case l374:LOG_CLI((BSL_META_U(l2,
"\nDelete Counter:\n")));break;case l375:LOG_CLI((BSL_META_U(l2,
"\nInternal Debug Counter - 1:\n")));break;case l376:l380(l2,&l381);LOG_CLI((
BSL_META_U(l2,"\nPivot Occupancy: Max v4/v6-64/v6-128 = %d/%d/%d\n"),l381.v4,
l381.v6_64,l381.v6_128));break;case l377:LOG_CLI((BSL_META_U(l2,
"\nInternal Debug Counter - LPM Shift:\n")));break;case l378:LOG_CLI((
BSL_META_U(l2,"\nInternal Debug Counter - LPM Full:\n")));break;default:break
;}LOG_CLI((BSL_META_U(l2,"\n      VRF  v4      v6-64   v6-128  |   Total\n"))
);LOG_CLI((BSL_META_U(l2,"-----------------------------------------------\n")
));l386 = l387 = l388 = 0;for(l171 = 0;l171<MAX_VRF_ID+1;l171++){int l389,
l390,l391;if(l383[l171].init_done == 0&&l171!= MAX_VRF_ID){continue;}if(l149
!= -1&&l149!= l171){continue;}l343 = 1;switch(l384){case l373:l385 = &l383[
l171].add;break;case l374:l385 = &l383[l171].del;break;case l375:l385 = &l383
[l171].bkt_split;break;case l377:l385 = &l383[l171].lpm_shift;break;case l378
:l385 = &l383[l171].lpm_full;break;case l376:l385 = &l383[l171].pivot_used;
break;default:l385 = &l383[l171].pivot_used;break;}l389 = l385->v4;l390 = 
l385->v6_64;l391 = l385->v6_128;l386+= l389;l387+= l390;l388+= l391;do{
LOG_CLI((BSL_META_U(l2,"%9d  %-7d %-7d %-7d |   %-7d %s\n"),(l171 == 
MAX_VRF_ID?-1:l171),(l389),(l390),(l391),((l389+l390+l391)),(l171) == 
MAX_VRF_ID?"GHi":(l171) == SOC_VRF_MAX(l2)+1?"GLo":""));}while(0);}if(l343 == 
0){LOG_CLI((BSL_META_U(l2,"%9s\n"),"Specific VRF not found"));}else{LOG_CLI((
BSL_META_U(l2,"-----------------------------------------------\n")));do{
LOG_CLI((BSL_META_U(l2,"%9s  %-7d %-7d %-7d |   %-7d \n"),"Total",(l386),(
l387),(l388),((l386+l387+l388))));}while(0);}return;}int soc_alpm_debug_show(
int l2,int l149,uint32 flags){int l171,l392,l343 = 0;l371*l393;l371 l394;l371
l395;if(l149>(SOC_VRF_MAX(l2)+1)){return SOC_E_PARAM;}l392 = MAX_VRF_ID*
sizeof(l371);l393 = sal_alloc(l392,"_alpm_dbg_cnt");if(l393 == NULL){return
SOC_E_MEMORY;}sal_memset(l393,0,l392);l394.v4 = ALPM_IPV4_BKT_COUNT;l394.
v6_64 = ALPM_IPV6_64_BKT_COUNT;l394.v6_128 = ALPM_IPV6_128_BKT_COUNT;if(!
soc_alpm_mode_get(l2)&&!SOC_URPF_STATUS_GET(l2)){l394.v6_64<<= 1;l394.v6_128
<<= 1;}LOG_CLI((BSL_META_U(l2,"\nBucket Occupancy:\n")));if(flags&(
SOC_ALPM_DEBUG_SHOW_FLAG_BKT)){do{LOG_CLI((BSL_META_U(l2,
"\n  BKT/VRF  Min     Max     Cur     |   Comment\n")));}while(0);LOG_CLI((
BSL_META_U(l2,"-----------------------------------------------\n")));}for(
l171 = 0;l171<MAX_PIVOT_COUNT;l171++){alpm_pivot_t*l396 = ALPM_TCAM_PIVOT(l2,
l171);if(l396!= NULL){l371*l397;int l28 = PIVOT_BUCKET_VRF(l396);if(l28<0||
l28>(SOC_VRF_MAX(l2)+1)){continue;}if(l149!= -1&&l149!= l28){continue;}if(
flags&SOC_ALPM_DEBUG_SHOW_FLAG_BKT){l343 = 1;do{LOG_CLI((BSL_META_U(l2,
"%5d/%-4d %-7d %-7d %-7d |   %-7s\n"),l171,l28,PIVOT_BUCKET_MIN(l396),
PIVOT_BUCKET_MAX(l396),PIVOT_BUCKET_COUNT(l396),PIVOT_BUCKET_DEF(l396)?"Def":
(l28) == SOC_VRF_MAX(l2)+1?"GLo":""));}while(0);}l397 = &l393[l28];if(
PIVOT_BUCKET_IPV6(l396) == L3_DEFIP_MODE_128){l397->v6_128+= 
PIVOT_BUCKET_COUNT(l396);l397->l370+= l394.v6_128;}else if(PIVOT_BUCKET_IPV6(
l396) == L3_DEFIP_MODE_64){l397->v6_64+= PIVOT_BUCKET_COUNT(l396);l397->l369
+= l394.v6_64;}else{l397->v4+= PIVOT_BUCKET_COUNT(l396);l397->l368+= l394.v4;
}l397->l286 = TRUE;}}if(flags&SOC_ALPM_DEBUG_SHOW_FLAG_BKT){if(l343 == 0){
LOG_CLI((BSL_META_U(l2,"%9s\n"),"Specific VRF not found"));}}sal_memset(&l395
,0,sizeof(l395));l343 = 0;if(flags&SOC_ALPM_DEBUG_SHOW_FLAG_BKTUSG){LOG_CLI((
BSL_META_U(l2,"\n      VRF  v4      v6-64   v6-128  |   Total\n")));LOG_CLI((
BSL_META_U(l2,"-----------------------------------------------\n")));for(l171
= 0;l171<MAX_VRF_ID;l171++){l371*l397;if(l393[l171].l286!= TRUE){continue;}if
(l149!= -1&&l149!= l171){continue;}l343 = 1;l397 = &l393[l171];do{(&l395)->v4
+= (l397)->v4;(&l395)->l368+= (l397)->l368;(&l395)->v6_64+= (l397)->v6_64;(&
l395)->l369+= (l397)->l369;(&l395)->v6_128+= (l397)->v6_128;(&l395)->l370+= (
l397)->l370;}while(0);do{LOG_CLI((BSL_META_U(l2,
"%9d  %02d.%d%%   %02d.%d%%   %02d.%d%%   |   %02d.%d%% %5s\n"),(l171),(l397
->l368)?(l397->v4)*100/(l397->l368):0,(l397->l368)?(l397->v4)*1000/(l397->
l368)%10:0,(l397->l369)?(l397->v6_64)*100/(l397->l369):0,(l397->l369)?(l397->
v6_64)*1000/(l397->l369)%10:0,(l397->l370)?(l397->v6_128)*100/(l397->l370):0,
(l397->l370)?(l397->v6_128)*1000/(l397->l370)%10:0,((l397->l368+l397->l369+
l397->l370))?((l397->v4+l397->v6_64+l397->v6_128))*100/((l397->l368+l397->
l369+l397->l370)):0,((l397->l368+l397->l369+l397->l370))?((l397->v4+l397->
v6_64+l397->v6_128))*1000/((l397->l368+l397->l369+l397->l370))%10:0,(l171) == 
SOC_VRF_MAX(l2)+1?"GLo":""));}while(0);}if(l343 == 0){LOG_CLI((BSL_META_U(l2,
"%9s\n"),"Specific VRF not found"));}else{LOG_CLI((BSL_META_U(l2,
"-----------------------------------------------\n")));do{LOG_CLI((BSL_META_U
(l2,"%9s  %02d.%d%%   %02d.%d%%   %02d.%d%%   |   %02d.%d%% \n"),"Total",((&
l395)->l368)?((&l395)->v4)*100/((&l395)->l368):0,((&l395)->l368)?((&l395)->v4
)*1000/((&l395)->l368)%10:0,((&l395)->l369)?((&l395)->v6_64)*100/((&l395)->
l369):0,((&l395)->l369)?((&l395)->v6_64)*1000/((&l395)->l369)%10:0,((&l395)->
l370)?((&l395)->v6_128)*100/((&l395)->l370):0,((&l395)->l370)?((&l395)->
v6_128)*1000/((&l395)->l370)%10:0,(((&l395)->l368+(&l395)->l369+(&l395)->l370
))?(((&l395)->v4+(&l395)->v6_64+(&l395)->v6_128))*100/(((&l395)->l368+(&l395)
->l369+(&l395)->l370)):0,(((&l395)->l368+(&l395)->l369+(&l395)->l370))?(((&
l395)->v4+(&l395)->v6_64+(&l395)->v6_128))*1000/(((&l395)->l368+(&l395)->l369
+(&l395)->l370))%10:0));}while(0);}}if(flags&SOC_ALPM_DEBUG_SHOW_FLAG_PVT){
l382(l2,l149,alpm_vrf_handle[l2],l376);}if(flags&SOC_ALPM_DEBUG_SHOW_FLAG_CNT
){l382(l2,l149,alpm_vrf_handle[l2],l373);l382(l2,l149,alpm_vrf_handle[l2],
l374);}if(flags&SOC_ALPM_DEBUG_SHOW_FLAG_INTDBG){l382(l2,l149,alpm_vrf_handle
[l2],l375);l382(l2,l149,alpm_vrf_handle[l2],l378);l382(l2,l149,
alpm_vrf_handle[l2],l377);}sal_free(l393);return SOC_E_NONE;}
#endif
#endif /* ALPM_ENABLE */
