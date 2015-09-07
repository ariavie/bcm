/*
 * $Id: alpm_128.c,v 1.13 Broadcom SDK $
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
 * File:    alpm_128.c
 * Purpose: Primitives for LPM management in ALPM - Mode for IPv6-128.
 * Requires:
 */

#include <shared/bsl.h>

#include <soc/mem.h>
#include <soc/drv.h>
#include <soc/debug.h>
#include <soc/error.h>
#include <soc/lpm.h>
#include <soc/trident2.h>
#ifdef ALPM_ENABLE
#include <shared/util.h>
#include <shared/l3.h>
#include <soc/alpm.h>
#include <soc/esw/alpm_int.h>
#include <soc/esw/trie.h>

#if 1
void soc_alpm_128_lpm_state_dump(int l1);static int l2(int l1,void*l3,int*
index);static int l4(int l1,void*l5);static int l6(int l1,void*l5,int l7,int
l8,int l9,defip_aux_scratch_entry_t*l10);static int l11(int l1,void*l5,void*
l12,int*l13,int*l14,int*l7);static int l15(int l1,void*l5,void*l12,int*l13);
static int l16(int l17,void*lpm_entry,void*l18,void*l19,soc_mem_t l20,uint32
l21,uint32*l22);static int l23(int l17,void*l18,soc_mem_t l20,int l7,int l24,
int l25,int index,void*lpm_entry);static int l26(int l17,uint32*key,int len,
int l24,int l7,defip_pair_128_entry_t*lpm_entry,int l27,int l28);static int
l29(int l1,int l24,int l30);typedef struct l31{int l32;int l33;int l34;int
next;int l35;int l36;}l37,*l38;static l38 l39[SOC_MAX_NUM_DEVICES];typedef
struct l40{soc_field_info_t*l41;soc_field_info_t*l42;soc_field_info_t*l43;
soc_field_info_t*l44;soc_field_info_t*l45;soc_field_info_t*l46;
soc_field_info_t*l47;soc_field_info_t*l48;soc_field_info_t*l49;
soc_field_info_t*l50;soc_field_info_t*l51;soc_field_info_t*l52;
soc_field_info_t*l53;soc_field_info_t*l54;soc_field_info_t*l55;
soc_field_info_t*l56;soc_field_info_t*l57;soc_field_info_t*l58;
soc_field_info_t*l59;soc_field_info_t*l60;soc_field_info_t*l61;
soc_field_info_t*l62;soc_field_info_t*l63;soc_field_info_t*l64;
soc_field_info_t*l65;soc_field_info_t*l66;soc_field_info_t*l67;
soc_field_info_t*l68;soc_field_info_t*l69;soc_field_info_t*l70;
soc_field_info_t*l71;soc_field_info_t*l72;soc_field_info_t*l73;
soc_field_info_t*l74;soc_field_info_t*l75;soc_field_info_t*l76;
soc_field_info_t*l77;soc_field_info_t*l78;soc_field_info_t*l79;
soc_field_info_t*l80;soc_field_info_t*l81;soc_field_info_t*l82;
soc_field_info_t*l83;soc_field_info_t*l84;soc_field_info_t*l85;
soc_field_info_t*l86;soc_field_info_t*l87;soc_field_info_t*l88;}l89,*l90;
static l90 l91[SOC_MAX_NUM_DEVICES];typedef struct l92{int l17;int l93;int l94
;uint16*l95;uint16*l96;}l97;typedef uint32 l98[9];typedef int(*l99)(l98 l100,
l98 l101);static l97*l102[SOC_MAX_NUM_DEVICES];static void l103(int l1,void*
l12,int index,l98 l104);static uint16 l105(uint8*l106,int l107);static int
l108(int l17,int l93,int l94,l97**l109);static int l110(l97*l111);static int
l112(l97*l113,l99 l114,l98 entry,int l115,uint16*l116);static int l117(l97*
l113,l99 l114,l98 entry,int l115,uint16 l118,uint16 l119);static int l120(l97
*l113,l99 l114,l98 entry,int l115,uint16 l121);static int l122(int l1,const
void*entry,int*l115){int l123,l124;int l125[4] = {IP_ADDR_MASK0_LWRf,
IP_ADDR_MASK1_LWRf,IP_ADDR_MASK0_UPRf,IP_ADDR_MASK1_UPRf};uint32 l126;l126 = 
soc_mem_field32_get(l1,L3_DEFIP_PAIR_128m,entry,l125[0]);if((l124 = 
_ipmask2pfx(l126,l115))<0){return(l124);}for(l123 = 1;l123<4;l123++){l126 = 
soc_mem_field32_get(l1,L3_DEFIP_PAIR_128m,entry,l125[l123]);if(*l115){if(l126
!= 0xffffffff){return(SOC_E_PARAM);}*l115+= 32;}else{if((l124 = _ipmask2pfx(
l126,l115))<0){return(l124);}}}return SOC_E_NONE;}static void l127(uint32*
l128,int l129,int l30){uint32 l130,l131,l32,prefix[5];int l123;sal_memcpy(
prefix,l128,sizeof(uint32)*BITS2WORDS(_MAX_KEY_LEN_144_));sal_memset(l128,0,
sizeof(uint32)*BITS2WORDS(_MAX_KEY_LEN_144_));l130 = 128-l129;l32 = (l130+31)
/32;if((l130%32) == 0){l32++;}l130 = l130%32;for(l123 = l32;l123<= 4;l123++){
prefix[l123]<<= l130;l131 = prefix[l123+1]&~(0xffffffff>>l130);l131 = (((32-
l130) == 32)?0:(l131)>>(32-l130));if(l123<4){prefix[l123]|= l131;}}for(l123 = 
l32;l123<= 4;l123++){l128[3-(l123-l32)] = prefix[l123];}}static void l132(int
l17,void*lpm_entry,int l14){int l123;soc_field_t l133[4] = {
IP_ADDR_MASK0_LWRf,IP_ADDR_MASK1_LWRf,IP_ADDR_MASK0_UPRf,IP_ADDR_MASK1_UPRf};
for(l123 = 0;l123<4;l123++){soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,
lpm_entry,l133[l123],0);}for(l123 = 0;l123<4;l123++){if(l14<= 32)break;
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,l133[3-l123],0xffffffff)
;l14-= 32;}soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,l133[3-l123],
~(((l14) == 32)?0:(0xffffffff)>>(l14)));}static int l134(int l1,void*entry,
uint32*l128,uint32*l14,int*l22){int l123;int l115 = 0,l32;int l124 = 
SOC_E_NONE;uint32 l130,l131;uint32 prefix[5];sal_memset(l128,0,sizeof(uint32)
*BITS2WORDS(_MAX_KEY_LEN_144_));sal_memset(prefix,0,sizeof(uint32)*BITS2WORDS
(_MAX_KEY_LEN_144_));prefix[0] = soc_mem_field32_get(l1,L3_DEFIP_PAIR_128m,
entry,IP_ADDR0_LWRf);prefix[1] = soc_mem_field32_get(l1,L3_DEFIP_PAIR_128m,
entry,IP_ADDR1_LWRf);prefix[2] = soc_mem_field32_get(l1,L3_DEFIP_PAIR_128m,
entry,IP_ADDR0_UPRf);prefix[3] = soc_mem_field32_get(l1,L3_DEFIP_PAIR_128m,
entry,IP_ADDR1_UPRf);if(l22!= NULL){*l22 = (prefix[0] == 0)&&(prefix[1] == 0)
&&(prefix[2] == 0)&&(prefix[3] == 0)&&(l115 == 0);}l124 = l122(l1,entry,&l115
);if(SOC_FAILURE(l124)){return l124;}l130 = 128-l115;l32 = l130/32;l130 = 
l130%32;for(l123 = l32;l123<4;l123++){prefix[l123]>>= l130;l131 = prefix[l123
+1]&((1<<l130)-1);l131 = (((32-l130) == 32)?0:(l131)<<(32-l130));prefix[l123]
|= l131;}for(l123 = l32;l123<4;l123++){l128[4-(l123-l32)] = prefix[l123];}*
l14 = l115;return SOC_E_NONE;}int l135(int l1,uint32*prefix,uint32 l129,int l7
,int l24,int*l136,int*l137,int*bucket_index){int l124 = SOC_E_NONE;trie_t*
l138;trie_node_t*l139 = NULL;alpm_pivot_t*l140;l138 = VRF_PIVOT_TRIE_IPV6_128
(l1,l24);l124 = trie_find_lpm(l138,prefix,l129,&l139);if(SOC_FAILURE(l124)){
LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l1,"Pivot find failed\n")));return l124
;}l140 = (alpm_pivot_t*)l139;*l136 = 1;*l137 = PIVOT_TCAM_INDEX(l140);*
bucket_index = PIVOT_BUCKET_INDEX(l140);return SOC_E_NONE;}static int l141(
int l1,void*l5,soc_mem_t l20,void*l142,int*l137,int*bucket_index,int*l13,int
l143){uint32 l12[SOC_MAX_MEM_FIELD_WORDS];int l144,l24,l30;int l116;uint32 l8
,l145;int l124 = SOC_E_NONE;int l136 = 0;l30 = L3_DEFIP_MODE_128;
SOC_IF_ERROR_RETURN(soc_alpm_128_lpm_vrf_get(l1,l5,&l144,&l24));if(l144 == 0)
{if(soc_alpm_mode_get(l1)){return SOC_E_PARAM;}}if(l24 == SOC_VRF_MAX(l1)+1){
l8 = 0;SOC_ALPM_GET_GLOBAL_BANK_DISABLE(l1,l145);}else{l8 = 2;
SOC_ALPM_GET_VRF_BANK_DISABLE(l1,l145);}if(l144!= SOC_L3_VRF_OVERRIDE){if(
l143){uint32 prefix[5],l129;int l22 = 0;l124 = l134(l1,l5,prefix,&l129,&l22);
if(SOC_FAILURE(l124)){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l1,
"_soc_alpm_insert: prefix create failed\n")));return l124;}l124 = l135(l1,
prefix,l129,l30,l24,&l136,l137,bucket_index);SOC_IF_ERROR_RETURN(l124);}else{
defip_aux_scratch_entry_t l10;sal_memset(&l10,0,sizeof(
defip_aux_scratch_entry_t));SOC_IF_ERROR_RETURN(l6(l1,l5,l30,l8,0,&l10));
SOC_IF_ERROR_RETURN(_soc_alpm_aux_op(l1,PREFIX_LOOKUP,&l10,TRUE,&l136,l137,
bucket_index));}if(l136){l16(l1,l5,l12,0,l20,0,0);l124 = 
_soc_alpm_find_in_bkt(l1,l20,*bucket_index,l145,l12,l142,&l116,l30);if(
SOC_SUCCESS(l124)){*l13 = l116;}}else{l124 = SOC_E_NOT_FOUND;}}return l124;}
static int l146(int l1,void*l5,void*l142,void*l147,soc_mem_t l20,int l116){
defip_aux_scratch_entry_t l10;int l144,l30,l24;int bucket_index;uint32 l8,
l145;int l124 = SOC_E_NONE;int l136 = 0,l131 = 0;int l137;l30 = 
L3_DEFIP_MODE_128;SOC_IF_ERROR_RETURN(soc_alpm_128_lpm_vrf_get(l1,l5,&l144,&
l24));if(l24 == SOC_VRF_MAX(l1)+1){l8 = 0;SOC_ALPM_GET_GLOBAL_BANK_DISABLE(l1
,l145);}else{l8 = 2;SOC_ALPM_GET_VRF_BANK_DISABLE(l1,l145);}if(!
soc_alpm_mode_get(l1)){l8 = 2;}if(l144!= SOC_L3_VRF_OVERRIDE){sal_memset(&l10
,0,sizeof(defip_aux_scratch_entry_t));SOC_IF_ERROR_RETURN(l6(l1,l5,l30,l8,0,&
l10));SOC_IF_ERROR_RETURN(soc_mem_write(l1,l20,MEM_BLOCK_ANY,l116,l142));if(
l124!= SOC_E_NONE){return SOC_E_FAIL;}if(SOC_URPF_STATUS_GET(l1)){
SOC_IF_ERROR_RETURN(soc_mem_write(l1,l20,MEM_BLOCK_ANY,_soc_alpm_rpf_entry(l1
,l116),l147));if(l124!= SOC_E_NONE){return SOC_E_FAIL;}}l131 = 
soc_mem_field32_get(l1,L3_DEFIP_AUX_SCRATCHm,&l10,IP_LENGTHf);
soc_mem_field32_set(l1,L3_DEFIP_AUX_SCRATCHm,&l10,REPLACE_LENf,l131);
SOC_IF_ERROR_RETURN(_soc_alpm_aux_op(l1,DELETE_PROPAGATE,&l10,TRUE,&l136,&
l137,&bucket_index));if(SOC_URPF_STATUS_GET(l1)){l131 = soc_mem_field32_get(
l1,L3_DEFIP_AUX_SCRATCHm,&l10,DB_TYPEf);l131+= 1;soc_mem_field32_set(l1,
L3_DEFIP_AUX_SCRATCHm,&l10,DB_TYPEf,l131);SOC_IF_ERROR_RETURN(
_soc_alpm_aux_op(l1,DELETE_PROPAGATE,&l10,TRUE,&l136,&l137,&bucket_index));}}
return l124;}static int l148(int l1,int l149,int l150){int l124,l131,l151,
l152;defip_aux_table_entry_t l153,l154;l151 = SOC_ALPM_128_ADDR_LWR(l149);
l152 = SOC_ALPM_128_ADDR_UPR(l149);l124 = soc_mem_read(l1,L3_DEFIP_AUX_TABLEm
,MEM_BLOCK_ANY,l151,&l153);SOC_IF_ERROR_RETURN(l124);l124 = soc_mem_read(l1,
L3_DEFIP_AUX_TABLEm,MEM_BLOCK_ANY,l152,&l154);SOC_IF_ERROR_RETURN(l124);
soc_mem_field32_set(l1,L3_DEFIP_AUX_TABLEm,&l153,BPM_LENGTH0f,l150);
soc_mem_field32_set(l1,L3_DEFIP_AUX_TABLEm,&l153,BPM_LENGTH1f,l150);
soc_mem_field32_set(l1,L3_DEFIP_AUX_TABLEm,&l154,BPM_LENGTH0f,l150);
soc_mem_field32_set(l1,L3_DEFIP_AUX_TABLEm,&l154,BPM_LENGTH1f,l150);l131 = 
soc_mem_field32_get(l1,L3_DEFIP_AUX_TABLEm,&l153,DB_TYPE0f);l124 = 
soc_mem_write(l1,L3_DEFIP_AUX_TABLEm,MEM_BLOCK_ANY,l151,&l153);
SOC_IF_ERROR_RETURN(l124);l124 = soc_mem_write(l1,L3_DEFIP_AUX_TABLEm,
MEM_BLOCK_ANY,l152,&l154);SOC_IF_ERROR_RETURN(l124);if(SOC_URPF_STATUS_GET(l1
)){l131++;soc_mem_field32_set(l1,L3_DEFIP_AUX_TABLEm,&l153,BPM_LENGTH0f,l150)
;soc_mem_field32_set(l1,L3_DEFIP_AUX_TABLEm,&l153,BPM_LENGTH1f,l150);
soc_mem_field32_set(l1,L3_DEFIP_AUX_TABLEm,&l154,BPM_LENGTH0f,l150);
soc_mem_field32_set(l1,L3_DEFIP_AUX_TABLEm,&l154,BPM_LENGTH1f,l150);
soc_mem_field32_set(l1,L3_DEFIP_AUX_TABLEm,&l153,DB_TYPE0f,l131);
soc_mem_field32_set(l1,L3_DEFIP_AUX_TABLEm,&l153,DB_TYPE1f,l131);
soc_mem_field32_set(l1,L3_DEFIP_AUX_TABLEm,&l154,DB_TYPE0f,l131);
soc_mem_field32_set(l1,L3_DEFIP_AUX_TABLEm,&l154,DB_TYPE1f,l131);l151+= (2*
soc_mem_index_count(l1,L3_DEFIP_PAIR_128m)+soc_mem_index_count(l1,L3_DEFIPm))
/2;l152+= (2*soc_mem_index_count(l1,L3_DEFIP_PAIR_128m)+soc_mem_index_count(
l1,L3_DEFIPm))/2;l124 = soc_mem_write(l1,L3_DEFIP_AUX_TABLEm,MEM_BLOCK_ANY,
l151,&l153);SOC_IF_ERROR_RETURN(l124);l124 = soc_mem_write(l1,
L3_DEFIP_AUX_TABLEm,MEM_BLOCK_ANY,l152,&l154);}return l124;}static int l155(
int l1,int l156,void*entry,defip_aux_table_entry_t*l157,int l158){uint32 l131
,l8,l159 = 0;soc_mem_t l20 = L3_DEFIP_PAIR_128m;soc_mem_t l160 = 
L3_DEFIP_AUX_TABLEm;int l124 = SOC_E_NONE,l115,l24;void*l161,*l162;l161 = (
void*)l157;l162 = (void*)(l157+1);SOC_IF_ERROR_RETURN(soc_mem_read(l1,l160,
MEM_BLOCK_ANY,SOC_ALPM_128_ADDR_LWR(l156),l157));SOC_IF_ERROR_RETURN(
soc_mem_read(l1,l160,MEM_BLOCK_ANY,SOC_ALPM_128_ADDR_UPR(l156),l157+1));l131 = 
soc_mem_field32_get(l1,l20,entry,VRF_ID_0_LWRf);soc_mem_field32_set(l1,l160,
l161,VRF0f,l131);l131 = soc_mem_field32_get(l1,l20,entry,VRF_ID_1_LWRf);
soc_mem_field32_set(l1,l160,l161,VRF1f,l131);l131 = soc_mem_field32_get(l1,
l20,entry,VRF_ID_0_UPRf);soc_mem_field32_set(l1,l160,l162,VRF0f,l131);l131 = 
soc_mem_field32_get(l1,l20,entry,VRF_ID_1_UPRf);soc_mem_field32_set(l1,l160,
l162,VRF1f,l131);l131 = soc_mem_field32_get(l1,l20,entry,MODE0_LWRf);
soc_mem_field32_set(l1,l160,l161,MODE0f,l131);l131 = soc_mem_field32_get(l1,
l20,entry,MODE1_LWRf);soc_mem_field32_set(l1,l160,l161,MODE1f,l131);l131 = 
soc_mem_field32_get(l1,l20,entry,MODE0_UPRf);soc_mem_field32_set(l1,l160,l162
,MODE0f,l131);l131 = soc_mem_field32_get(l1,l20,entry,MODE1_UPRf);
soc_mem_field32_set(l1,l160,l162,MODE1f,l131);l131 = soc_mem_field32_get(l1,
l20,entry,VALID0_LWRf);soc_mem_field32_set(l1,l160,l161,VALID0f,l131);l131 = 
soc_mem_field32_get(l1,l20,entry,VALID1_LWRf);soc_mem_field32_set(l1,l160,
l161,VALID1f,l131);l131 = soc_mem_field32_get(l1,l20,entry,VALID0_UPRf);
soc_mem_field32_set(l1,l160,l162,VALID0f,l131);l131 = soc_mem_field32_get(l1,
l20,entry,VALID1_UPRf);soc_mem_field32_set(l1,l160,l162,VALID1f,l131);l124 = 
l122(l1,entry,&l115);SOC_IF_ERROR_RETURN(l124);soc_mem_field32_set(l1,l160,
l161,IP_LENGTH0f,l115);soc_mem_field32_set(l1,l160,l161,IP_LENGTH1f,l115);
soc_mem_field32_set(l1,l160,l162,IP_LENGTH0f,l115);soc_mem_field32_set(l1,
l160,l162,IP_LENGTH1f,l115);l131 = soc_mem_field32_get(l1,l20,entry,
IP_ADDR0_LWRf);soc_mem_field32_set(l1,l160,l161,IP_ADDR0f,l131);l131 = 
soc_mem_field32_get(l1,l20,entry,IP_ADDR1_LWRf);soc_mem_field32_set(l1,l160,
l161,IP_ADDR1f,l131);l131 = soc_mem_field32_get(l1,l20,entry,IP_ADDR0_UPRf);
soc_mem_field32_set(l1,l160,l162,IP_ADDR0f,l131);l131 = soc_mem_field32_get(
l1,l20,entry,IP_ADDR1_UPRf);soc_mem_field32_set(l1,l160,l162,IP_ADDR1f,l131);
l131 = soc_mem_field32_get(l1,l20,entry,ENTRY_TYPE0_LWRf);soc_mem_field32_set
(l1,l160,l161,ENTRY_TYPE0f,l131);l131 = soc_mem_field32_get(l1,l20,entry,
ENTRY_TYPE1_LWRf);soc_mem_field32_set(l1,l160,l161,ENTRY_TYPE1f,l131);l131 = 
soc_mem_field32_get(l1,l20,entry,ENTRY_TYPE0_UPRf);soc_mem_field32_set(l1,
l160,l162,ENTRY_TYPE0f,l131);l131 = soc_mem_field32_get(l1,l20,entry,
ENTRY_TYPE1_UPRf);soc_mem_field32_set(l1,l160,l162,ENTRY_TYPE1f,l131);l124 = 
soc_alpm_128_lpm_vrf_get(l1,entry,&l24,&l115);SOC_IF_ERROR_RETURN(l124);if(
SOC_URPF_STATUS_GET(l1)){if(l158>= (soc_mem_index_count(l1,L3_DEFIP_PAIR_128m
)>>1)){l159 = 1;}}switch(l24){case SOC_L3_VRF_OVERRIDE:soc_mem_field32_set(l1
,l160,l161,VALID0f,0);soc_mem_field32_set(l1,l160,l161,VALID1f,0);
soc_mem_field32_set(l1,l160,l162,VALID0f,0);soc_mem_field32_set(l1,l160,l162,
VALID1f,0);l8 = 0;break;case SOC_L3_VRF_GLOBAL:l8 = l159?1:0;break;default:l8
= l159?3:2;break;}soc_mem_field32_set(l1,l160,l161,DB_TYPE0f,l8);
soc_mem_field32_set(l1,l160,l161,DB_TYPE1f,l8);soc_mem_field32_set(l1,l160,
l162,DB_TYPE0f,l8);soc_mem_field32_set(l1,l160,l162,DB_TYPE1f,l8);if(l159){
l131 = soc_mem_field32_get(l1,l20,entry,ALG_BKT_PTRf);if(l131){l131+= 
SOC_ALPM_BUCKET_COUNT(l1);soc_mem_field32_set(l1,l20,entry,ALG_BKT_PTRf,l131)
;}}return SOC_E_NONE;}static int l163(int l1,int l164,int index,int l165,void
*entry){defip_aux_table_entry_t l157[2];l165 = soc_alpm_physical_idx(l1,
L3_DEFIP_PAIR_128m,l165,1);SOC_IF_ERROR_RETURN(l155(l1,l165,entry,&l157[0],
index));SOC_IF_ERROR_RETURN(WRITE_L3_DEFIP_PAIR_128m(l1,MEM_BLOCK_ANY,index,
entry));index = soc_alpm_physical_idx(l1,L3_DEFIP_PAIR_128m,index,1);
SOC_IF_ERROR_RETURN(WRITE_L3_DEFIP_AUX_TABLEm(l1,MEM_BLOCK_ANY,
SOC_ALPM_128_ADDR_LWR(index),l157));SOC_IF_ERROR_RETURN(
WRITE_L3_DEFIP_AUX_TABLEm(l1,MEM_BLOCK_ANY,SOC_ALPM_128_ADDR_UPR(index),l157+
1));return SOC_E_NONE;}static int l166(int l1,void*l5,soc_mem_t l20,void*l142
,void*l147,int*l13,int bucket_index,int l137){alpm_pivot_t*l140,*l167,*l168;
defip_aux_scratch_entry_t l10;uint32 l12[SOC_MAX_MEM_FIELD_WORDS];uint32
prefix[5],l169,l129;uint32 l170[5];int l30,l24,l144;int l116;int l124 = 
SOC_E_NONE,l171;uint32 l8,l145,l150 = 0;int l136 =0;int l149;int l172 = 0;
trie_t*trie,*l173;trie_node_t*l174,*l175 = NULL,*l139 = NULL;payload_t*l176,*
l177,*l178;defip_pair_128_entry_t lpm_entry;alpm_bucket_handle_t*l179;int l123
,l180 = -1,l22 = 0;alpm_mem_prefix_array_t l181;defip_alpm_ipv6_128_entry_t
l182,l183;void*l184,*l185;int*l119 = NULL;trie_t*l138 = NULL;int l186;
defip_alpm_raw_entry_t*l187 = NULL;defip_alpm_raw_entry_t*l188;
defip_alpm_raw_entry_t*l189;defip_alpm_raw_entry_t*l190;
defip_alpm_raw_entry_t*l191;l30 = L3_DEFIP_MODE_128;SOC_IF_ERROR_RETURN(
soc_alpm_128_lpm_vrf_get(l1,l5,&l144,&l24));if(l24 == SOC_VRF_MAX(l1)+1){l8 = 
0;SOC_ALPM_GET_GLOBAL_BANK_DISABLE(l1,l145);}else{l8 = 2;
SOC_ALPM_GET_VRF_BANK_DISABLE(l1,l145);}l20 = L3_DEFIP_ALPM_IPV6_128m;l184 = 
((uint32*)&(l182));l185 = ((uint32*)&(l183));l124 = l134(l1,l5,prefix,&l129,&
l22);if(SOC_FAILURE(l124)){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l1,
"_soc_alpm_128_insert: prefix create failed\n")));return l124;}sal_memset(&
l10,0,sizeof(defip_aux_scratch_entry_t));SOC_IF_ERROR_RETURN(l6(l1,l5,l30,l8,
0,&l10));if(bucket_index == 0){l124 = l135(l1,prefix,l129,l30,l24,&l136,&l137
,&bucket_index);SOC_IF_ERROR_RETURN(l124);if(l136 == 0){LOG_ERROR(
BSL_LS_SOC_ALPM,(BSL_META_U(l1,"_soc_alpm_128_insert: "
" Could not find bucket to insert prefix\n")));return SOC_E_NOT_FOUND;}}l124 = 
_soc_alpm_insert_in_bkt(l1,l20,bucket_index,l145,l142,l12,&l116,l30);if(l124
== SOC_E_NONE){*l13 = l116;if(SOC_URPF_STATUS_GET(l1)){l171 = soc_mem_write(
l1,l20,MEM_BLOCK_ANY,_soc_alpm_rpf_entry(l1,l116),l147);if(SOC_FAILURE(l171))
{return l171;}}}if(l124 == SOC_E_FULL){l172 = 1;}l140 = ALPM_TCAM_PIVOT(l1,
l137);trie = PIVOT_BUCKET_TRIE(l140);l168 = l140;l176 = sal_alloc(sizeof(
payload_t),"Payload for 128b Key");if(l176 == NULL){LOG_ERROR(BSL_LS_SOC_ALPM
,(BSL_META_U(l1,"_soc_alpm_128_insert: Unable to allocate memory for "
"trie node \n")));return SOC_E_MEMORY;}l177 = sal_alloc(sizeof(payload_t),
"Payload for pfx trie 128b key");if(l177 == NULL){LOG_ERROR(BSL_LS_SOC_ALPM,(
BSL_META_U(l1,"_soc_alpm_128_insert: Unable to allocate memory for "
"pfx trie node \n")));sal_free(l176);return SOC_E_MEMORY;}sal_memset(l176,0,
sizeof(*l176));sal_memset(l177,0,sizeof(*l177));l176->key[0] = prefix[0];l176
->key[1] = prefix[1];l176->key[2] = prefix[2];l176->key[3] = prefix[3];l176->
key[4] = prefix[4];l176->len = l129;l176->index = l116;sal_memcpy(l177,l176,
sizeof(*l176));l177->bkt_ptr = l176;l124 = trie_insert(trie,prefix,NULL,l129,
(trie_node_t*)l176);if(SOC_FAILURE(l124)){goto l192;}l173 = 
VRF_PREFIX_TRIE_IPV6_128(l1,l24);if(!l22){l124 = trie_insert(l173,prefix,NULL
,l129,(trie_node_t*)l177);}else{l139 = NULL;l124 = trie_find_lpm(l173,0,0,&
l139);l178 = (payload_t*)l139;if(SOC_SUCCESS(l124)){l178->bkt_ptr = l176;}}
l169 = l129;if(SOC_FAILURE(l124)){goto l193;}if(l172){l124 = 
alpm_bucket_assign(l1,&bucket_index,l30);if(SOC_FAILURE(l124)){LOG_ERROR(
BSL_LS_SOC_ALPM,(BSL_META_U(l1,"_soc_alpm_128_insert: Unable to allocate"
"new bucket for split\n")));bucket_index = -1;goto l194;}l124 = trie_split(
trie,_MAX_KEY_LEN_144_,FALSE,l170,&l129,&l174,NULL,FALSE);if(SOC_FAILURE(l124
)){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l1,
"_soc_alpm_128_insert: Could not split bucket"
"for prefix 0x%08x 0x%08x 0x%08x 0x%08x\n"),prefix[1],prefix[2],prefix[3],
prefix[4]));goto l194;}l139 = NULL;l124 = trie_find_lpm(l173,l170,l129,&l139)
;l178 = (payload_t*)l139;if(SOC_FAILURE(l124)){LOG_ERROR(BSL_LS_SOC_ALPM,(
BSL_META_U(l1,"unit %d Unable to find lpm for pivot: "
"0x%08x 0x%08x\n 0x%08x 0x%08x 0x%08x length: %d\n"),l1,l170[0],l170[1],l170[
2],l170[3],l170[4],l129));goto l194;}if(l178->index){if(l178->bkt_ptr == l176
){sal_memcpy(l184,l142,sizeof(defip_alpm_ipv6_128_entry_t));}else{l124 = 
soc_mem_read(l1,l20,MEM_BLOCK_ANY,((payload_t*)l178->bkt_ptr)->index,l184);}
if(SOC_FAILURE(l124)){goto l194;}l124 = l23(l1,l184,l20,l30,l144,bucket_index
,0,&lpm_entry);if(SOC_FAILURE(l124)){goto l194;}l150 = ((payload_t*)(l178->
bkt_ptr))->len;}else{l124 = soc_mem_read(l1,L3_DEFIP_PAIR_128m,MEM_BLOCK_ANY,
soc_alpm_logical_idx(l1,L3_DEFIP_PAIR_128m,SOC_ALPM_128_DEFIP_TO_PAIR(l137>>1
),1),&lpm_entry);}l179 = sal_alloc(sizeof(alpm_bucket_handle_t),
"ALPM 128 Bucket Handle");if(l179 == NULL){LOG_ERROR(BSL_LS_SOC_ALPM,(
BSL_META_U(l1,"_soc_alpm_128_insert: Unable to allocate "
" memory for PIVOT trie node \n")));l124 = SOC_E_MEMORY;goto l194;}sal_memset
(l179,0,sizeof(*l179));l140 = sal_alloc(sizeof(alpm_pivot_t),
"Payload for new 128b Pivot");if(l140 == NULL){LOG_ERROR(BSL_LS_SOC_ALPM,(
BSL_META_U(l1,"_soc_alpm_128_insert: Unable to allocate "
"memory for PIVOT trie node \n")));l124 = SOC_E_MEMORY;goto l194;}sal_memset(
l140,0,sizeof(*l140));PIVOT_BUCKET_HANDLE(l140) = l179;l124 = trie_init(
_MAX_KEY_LEN_144_,&PIVOT_BUCKET_TRIE(l140));PIVOT_BUCKET_TRIE(l140)->trie = 
l174;PIVOT_BUCKET_INDEX(l140) = bucket_index;PIVOT_BUCKET_VRF(l140) = l24;
PIVOT_BUCKET_IPV6(l140) = l30;PIVOT_BUCKET_DEF(l140) = FALSE;l140->key[0] = 
l170[0];l140->key[1] = l170[1];l140->key[2] = l170[2];l140->key[3] = l170[3];
l140->key[4] = l170[4];l140->len = l129;l138 = VRF_PIVOT_TRIE_IPV6_128(l1,l24
);l127((l170),(l129),(l30));l26(l1,l170,l129,l24,l30,&lpm_entry,0,0);
soc_L3_DEFIP_PAIR_128m_field32_set(l1,&lpm_entry,ALG_BKT_PTRf,bucket_index);
sal_memset(&l181,0,sizeof(l181));l124 = trie_traverse(PIVOT_BUCKET_TRIE(l140)
,alpm_mem_prefix_array_cb,&l181,_TRIE_INORDER_TRAVERSE);if(SOC_FAILURE(l124))
{LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l1,
"_soc_128_alpm_insert: Bucket split failed"
"for prefix 0x%08x 0x%08x 0x%08x 0x%08x\n"),prefix[1],prefix[2],prefix[3],
prefix[4]));goto l194;}l119 = sal_alloc(sizeof(*l119)*l181.count,
"Temp storage for location of prefixes in new 128b bucket");if(l119 == NULL){
l124 = SOC_E_MEMORY;goto l194;}sal_memset(l119,-1,sizeof(*l119)*l181.count);
l186 = sizeof(defip_alpm_raw_entry_t)*ALPM_RAW_BKT_COUNT_DW;l187 = sal_alloc(
4*l186,"Raw memory buffer");if(l187 == NULL){l124 = SOC_E_MEMORY;goto l194;}
sal_memset(l187,0,4*l186);l188 = (defip_alpm_raw_entry_t*)l187;l189 = (
defip_alpm_raw_entry_t*)((uint8*)l188+l186);l190 = (defip_alpm_raw_entry_t*)(
(uint8*)l189+l186);l191 = (defip_alpm_raw_entry_t*)((uint8*)l190+l186);l124 = 
_soc_alpm_raw_bucket_read(l1,l20,PIVOT_BUCKET_INDEX(l168),(void*)l188,(void*)
l189);if(SOC_FAILURE(l124)){goto l194;}for(l123 = 0;l123<l181.count;l123++){
payload_t*l115 = l181.prefix[l123];if(l115->index>0){_soc_alpm_raw_mem_read(
l1,l20,l188,l115->index,l184);_soc_alpm_raw_mem_write(l1,l20,l188,l115->index
,soc_mem_entry_null(l1,l20));if(SOC_URPF_STATUS_GET(l1)){
_soc_alpm_raw_mem_read(l1,l20,l189,_soc_alpm_rpf_entry(l1,l115->index),l185);
_soc_alpm_raw_mem_write(l1,l20,l189,_soc_alpm_rpf_entry(l1,l115->index),
soc_mem_entry_null(l1,l20));}l124 = _soc_alpm_mem_index(l1,l20,bucket_index,
l123,l145,&l116);if(SOC_SUCCESS(l124)){_soc_alpm_raw_mem_write(l1,l20,l190,
l116,l184);if(SOC_URPF_STATUS_GET(l1)){_soc_alpm_raw_mem_write(l1,l20,l191,
_soc_alpm_rpf_entry(l1,l116),l185);}}}else{l124 = _soc_alpm_mem_index(l1,l20,
bucket_index,l123,l145,&l116);if(SOC_SUCCESS(l124)){l180 = l123;*l13 = l116;
_soc_alpm_raw_parity_set(l1,l20,l142);_soc_alpm_raw_mem_write(l1,l20,l190,
l116,l142);if(SOC_URPF_STATUS_GET(l1)){_soc_alpm_raw_parity_set(l1,l20,l147);
_soc_alpm_raw_mem_write(l1,l20,l191,_soc_alpm_rpf_entry(l1,l116),l147);}}}
l119[l123] = l116;}l124 = _soc_alpm_raw_bucket_write(l1,l20,bucket_index,l145
,(void*)l190,(void*)l191,l181.count);if(SOC_FAILURE(l124)){goto l195;}l124 = 
l2(l1,&lpm_entry,&l149);if(SOC_FAILURE(l124)){LOG_ERROR(BSL_LS_SOC_ALPM,(
BSL_META_U(l1,"_soc_alpm_128_insert: Unable to add new ""pivot to tcam\n")));
if(l124 == SOC_E_FULL){VRF_PIVOT_FULL_INC(l1,l24,l30);}goto l195;}l149 = 
soc_alpm_physical_idx(l1,L3_DEFIP_PAIR_128m,l149,l30);l124 = l148(l1,l149,
l150);if(SOC_FAILURE(l124)){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l1,
"_soc_alpm_insert: Unable to init bpm_len ""for index %d\n"),l149));goto l196
;}l124 = trie_insert(l138,l140->key,NULL,l140->len,(trie_node_t*)l140);if(
SOC_FAILURE(l124)){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l1,
"failed to insert into pivot trie\n")));goto l196;}ALPM_TCAM_PIVOT(l1,
SOC_ALPM_128_ADDR_LWR(l149)<<1) = l140;PIVOT_TCAM_INDEX(l140) = 
SOC_ALPM_128_ADDR_LWR(l149)<<1;VRF_PIVOT_REF_INC(l1,l24,l30);for(l123 = 0;
l123<l181.count;l123++){l181.prefix[l123]->index = l119[l123];}sal_free(l119)
;l124 = _soc_alpm_raw_bucket_write(l1,l20,PIVOT_BUCKET_INDEX(l168),l145,(void
*)l188,(void*)l189,-1);if(SOC_FAILURE(l124)){goto l196;}sal_free(l187);if(
l180 == -1){l124 = _soc_alpm_insert_in_bkt(l1,l20,PIVOT_BUCKET_HANDLE(l168)->
bucket_index,l145,l142,l12,&l116,l30);if(SOC_FAILURE(l124)){LOG_ERROR(
BSL_LS_SOC_ALPM,(BSL_META_U(l1,"_soc_alpm_128_insert: Could not insert new "
"prefix into trie after split\n")));goto l194;}if(SOC_URPF_STATUS_GET(l1)){
l124 = soc_mem_write(l1,l20,MEM_BLOCK_ANY,_soc_alpm_rpf_entry(l1,l116),l147);
}*l13 = l116;l176->index = l116;}PIVOT_BUCKET_ENT_CNT_UPDATE(l140);
VRF_BUCKET_SPLIT_INC(l1,l24,l30);}VRF_TRIE_ROUTES_INC(l1,l24,l30);if(l22){
sal_free(l177);}SOC_IF_ERROR_RETURN(_soc_alpm_aux_op(l1,DELETE_PROPAGATE,&l10
,TRUE,&l136,&l137,&bucket_index));SOC_IF_ERROR_RETURN(_soc_alpm_aux_op(l1,
INSERT_PROPAGATE,&l10,FALSE,&l136,&l137,&bucket_index));if(
SOC_URPF_STATUS_GET(l1)){l129 = soc_mem_field32_get(l1,L3_DEFIP_AUX_SCRATCHm,
&l10,DB_TYPEf);l129+= 1;soc_mem_field32_set(l1,L3_DEFIP_AUX_SCRATCHm,&l10,
DB_TYPEf,l129);SOC_IF_ERROR_RETURN(_soc_alpm_aux_op(l1,DELETE_PROPAGATE,&l10,
TRUE,&l136,&l137,&bucket_index));SOC_IF_ERROR_RETURN(_soc_alpm_aux_op(l1,
INSERT_PROPAGATE,&l10,FALSE,&l136,&l137,&bucket_index));}
PIVOT_BUCKET_ENT_CNT_UPDATE(l168);return l124;l196:l171 = l4(l1,&lpm_entry);
if(SOC_FAILURE(l171)){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l1,
"_soc_alpm_128_insert: Failure to free new prefix""at %d\n"),
soc_alpm_logical_idx(l1,L3_DEFIP_PAIR_128m,l149,l30)));}if(ALPM_TCAM_PIVOT(l1
,SOC_ALPM_128_ADDR_LWR(l149)<<1)!= NULL){l171 = trie_delete(l138,l140->key,
l140->len,&l175);if(SOC_FAILURE(l171)){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(
l1,"_soc_alpm_insert: trie delete failure""in bkt move rollback\n")));}}
ALPM_TCAM_PIVOT(l1,SOC_ALPM_128_ADDR_LWR(l149)<<1) = NULL;VRF_PIVOT_REF_DEC(
l1,l24,l30);l195:l167 = l168;for(l123 = 0;l123<l181.count;l123++){payload_t*
prefix = l181.prefix[l123];if(l119[l123]!= -1){sal_memset(l184,0,sizeof(
defip_alpm_ipv6_128_entry_t));l171 = soc_mem_write(l1,l20,MEM_BLOCK_ANY,l119[
l123],l184);_soc_trident2_alpm_bkt_view_set(l1,l119[l123],INVALIDm);if(
SOC_FAILURE(l171)){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l1,
"_soc_alpm_128_insert: mem write failure""in bkt move rollback\n")));}if(
SOC_URPF_STATUS_GET(l1)){l171 = soc_mem_write(l1,l20,MEM_BLOCK_ANY,
_soc_alpm_rpf_entry(l1,l119[l123]),l184);_soc_trident2_alpm_bkt_view_set(l1,
_soc_alpm_rpf_entry(l1,l119[l123]),INVALIDm);if(SOC_FAILURE(l171)){LOG_ERROR(
BSL_LS_SOC_ALPM,(BSL_META_U(l1,"_soc_alpm_128_insert: urpf mem write "
"failure in bkt move rollback\n")));}}}l175 = NULL;l171 = trie_delete(
PIVOT_BUCKET_TRIE(l140),prefix->key,prefix->len,&l175);l176 = (payload_t*)
l175;if(SOC_FAILURE(l171)){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l1,
"_soc_alpm_128_insert: trie delete failure""in bkt move rollback\n")));}if(
prefix->index>0){l171 = trie_insert(PIVOT_BUCKET_TRIE(l167),prefix->key,NULL,
prefix->len,(trie_node_t*)l176);if(SOC_FAILURE(l171)){LOG_ERROR(
BSL_LS_SOC_ALPM,(BSL_META_U(l1,"_soc_alpm_128_insert: trie reinsert failure"
"in bkt move rollback\n")));}}else{if(l176!= NULL){sal_free(l176);}}}if(l180
== -1){l175 = NULL;l171 = trie_delete(PIVOT_BUCKET_TRIE(l167),prefix,l169,&
l175);l176 = (payload_t*)l175;if(SOC_FAILURE(l171)){LOG_ERROR(BSL_LS_SOC_ALPM
,(BSL_META_U(l1,"_soc_alpm_128_insert: expected to clear prefix"
" 0x%08x 0x%08x\n from old trie. Failed\n"),prefix[0],prefix[1]));}if(l176!= 
NULL){sal_free(l176);}}l171 = alpm_bucket_release(l1,PIVOT_BUCKET_INDEX(l140)
,l30);if(SOC_FAILURE(l171)){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l1,
"_soc_alpm_128_insert: new bucket release ""failure: %d\n"),
PIVOT_BUCKET_INDEX(l140)));}trie_destroy(PIVOT_BUCKET_TRIE(l140));sal_free(
l179);sal_free(l140);sal_free(l119);sal_free(l187);l175 = NULL;l171 = 
trie_delete(l173,prefix,l169,&l175);l177 = (payload_t*)l175;if(SOC_FAILURE(
l171)){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l1,
"_soc_alpm_128_insert: failed to delete new prefix"
"0x%08x 0x%08x from pfx trie\n"),prefix[0],prefix[1]));}if(l177){sal_free(
l177);}return l124;l194:if(l119!= NULL){sal_free(l119);}if(l187!= NULL){
sal_free(l187);}l175 = NULL;(void)trie_delete(l173,prefix,l169,&l175);l177 = 
(payload_t*)l175;if(bucket_index!= -1){(void)alpm_bucket_release(l1,
bucket_index,l30);}l193:l175 = NULL;(void)trie_delete(trie,prefix,l169,&l175)
;l176 = (payload_t*)l175;l192:if(l176!= NULL){sal_free(l176);}if(l177!= NULL)
{sal_free(l177);}return l124;}static int l26(int l17,uint32*key,int len,int
l24,int l7,defip_pair_128_entry_t*lpm_entry,int l27,int l28){uint32 l131;if(
l28){sal_memset(lpm_entry,0,sizeof(defip_pair_128_entry_t));}
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l17,L3_DEFIP_PAIR_128m)),(
lpm_entry),(l91[(l17)]->l73),(l24&SOC_VRF_MAX(l17)));
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l17,L3_DEFIP_PAIR_128m)),(
lpm_entry),(l91[(l17)]->l74),(l24&SOC_VRF_MAX(l17)));
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l17,L3_DEFIP_PAIR_128m)),(
lpm_entry),(l91[(l17)]->l71),(l24&SOC_VRF_MAX(l17)));
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l17,L3_DEFIP_PAIR_128m)),(
lpm_entry),(l91[(l17)]->l72),(l24&SOC_VRF_MAX(l17)));if(l24 == (SOC_VRF_MAX(
l17)+1)){l131 = 0;}else{l131 = SOC_VRF_MAX(l17);}
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l17,L3_DEFIP_PAIR_128m)),(
lpm_entry),(l91[(l17)]->l77),(l131));soc_meminfo_fieldinfo_field32_set((&
SOC_MEM_INFO(l17,L3_DEFIP_PAIR_128m)),(lpm_entry),(l91[(l17)]->l78),(l131));
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l17,L3_DEFIP_PAIR_128m)),(
lpm_entry),(l91[(l17)]->l75),(l131));soc_meminfo_fieldinfo_field32_set((&
SOC_MEM_INFO(l17,L3_DEFIP_PAIR_128m)),(lpm_entry),(l91[(l17)]->l76),(l131));
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l17,L3_DEFIP_PAIR_128m)),(
lpm_entry),(l91[(l17)]->l50),(key[0]));soc_meminfo_fieldinfo_field32_set((&
SOC_MEM_INFO(l17,L3_DEFIP_PAIR_128m)),(lpm_entry),(l91[(l17)]->l51),(key[1]))
;soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l17,L3_DEFIP_PAIR_128m)),(
lpm_entry),(l91[(l17)]->l48),(key[2]));soc_meminfo_fieldinfo_field32_set((&
SOC_MEM_INFO(l17,L3_DEFIP_PAIR_128m)),(lpm_entry),(l91[(l17)]->l49),(key[3]))
;soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l17,L3_DEFIP_PAIR_128m)),(
lpm_entry),(l91[(l17)]->l58),(3));soc_meminfo_fieldinfo_field32_set((&
SOC_MEM_INFO(l17,L3_DEFIP_PAIR_128m)),(lpm_entry),(l91[(l17)]->l59),(3));
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l17,L3_DEFIP_PAIR_128m)),(
lpm_entry),(l91[(l17)]->l56),(3));soc_meminfo_fieldinfo_field32_set((&
SOC_MEM_INFO(l17,L3_DEFIP_PAIR_128m)),(lpm_entry),(l91[(l17)]->l57),(3));l132
(l17,(void*)lpm_entry,len);soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(
l17,L3_DEFIP_PAIR_128m)),(lpm_entry),(l91[(l17)]->l69),(1));
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l17,L3_DEFIP_PAIR_128m)),(
lpm_entry),(l91[(l17)]->l70),(1));soc_meminfo_fieldinfo_field32_set((&
SOC_MEM_INFO(l17,L3_DEFIP_PAIR_128m)),(lpm_entry),(l91[(l17)]->l67),(1));
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l17,L3_DEFIP_PAIR_128m)),(
lpm_entry),(l91[(l17)]->l68),(1));soc_meminfo_fieldinfo_field32_set((&
SOC_MEM_INFO(l17,L3_DEFIP_PAIR_128m)),(lpm_entry),(l91[(l17)]->l62),((1<<
soc_mem_field_length(l17,L3_DEFIP_PAIR_128m,MODE_MASK0_LWRf))-1));
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l17,L3_DEFIP_PAIR_128m)),(
lpm_entry),(l91[(l17)]->l63),((1<<soc_mem_field_length(l17,L3_DEFIP_PAIR_128m
,MODE_MASK1_LWRf))-1));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l17,
L3_DEFIP_PAIR_128m)),(lpm_entry),(l91[(l17)]->l60),((1<<soc_mem_field_length(
l17,L3_DEFIP_PAIR_128m,MODE_MASK0_UPRf))-1));
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l17,L3_DEFIP_PAIR_128m)),(
lpm_entry),(l91[(l17)]->l61),((1<<soc_mem_field_length(l17,L3_DEFIP_PAIR_128m
,MODE_MASK1_UPRf))-1));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l17,
L3_DEFIP_PAIR_128m)),(lpm_entry),(l91[(l17)]->l85),((1<<soc_mem_field_length(
l17,L3_DEFIP_PAIR_128m,ENTRY_TYPE_MASK0_LWRf))-1));
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l17,L3_DEFIP_PAIR_128m)),(
lpm_entry),(l91[(l17)]->l86),((1<<soc_mem_field_length(l17,L3_DEFIP_PAIR_128m
,ENTRY_TYPE_MASK1_LWRf))-1));soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO
(l17,L3_DEFIP_PAIR_128m)),(lpm_entry),(l91[(l17)]->l87),((1<<
soc_mem_field_length(l17,L3_DEFIP_PAIR_128m,ENTRY_TYPE_MASK0_UPRf))-1));
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l17,L3_DEFIP_PAIR_128m)),(
lpm_entry),(l91[(l17)]->l88),((1<<soc_mem_field_length(l17,L3_DEFIP_PAIR_128m
,ENTRY_TYPE_MASK1_UPRf))-1));return(SOC_E_NONE);}static int l197(int l1,void*
l5,int bucket_index,int l137,int l116){alpm_pivot_t*l140;
defip_alpm_ipv6_128_entry_t l182,l198,l183;defip_aux_scratch_entry_t l10;
uint32 l12[SOC_MAX_MEM_FIELD_WORDS];soc_mem_t l20;void*l184,*l199,*l185 = 
NULL;int l144;int l7;int l124 = SOC_E_NONE,l171 = SOC_E_NONE;uint32 l200[5],
prefix[5];int l30,l24;uint32 l129;int l201;uint32 l8,l145;int l136,l22 = 0;
trie_t*trie,*l173;uint32 l202;defip_pair_128_entry_t lpm_entry,*l203;
payload_t*l176 = NULL,*l204 = NULL,*l178 = NULL;trie_node_t*l175 = NULL,*l139
= NULL;trie_t*l138 = NULL;l7 = l30 = L3_DEFIP_MODE_128;SOC_IF_ERROR_RETURN(
soc_alpm_128_lpm_vrf_get(l1,l5,&l144,&l24));if(l144!= SOC_L3_VRF_OVERRIDE){if
(l24 == SOC_VRF_MAX(l1)+1){l8 = 0;SOC_ALPM_GET_GLOBAL_BANK_DISABLE(l1,l145);}
else{l8 = 2;SOC_ALPM_GET_VRF_BANK_DISABLE(l1,l145);}l124 = l134(l1,l5,prefix,
&l129,&l22);if(SOC_FAILURE(l124)){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l1,
"_soc_alpm_128_delete: prefix create failed\n")));return l124;}if(!
soc_alpm_mode_get(l1)){if(l144!= SOC_L3_VRF_GLOBAL){if(VRF_TRIE_ROUTES_CNT(l1
,l24,l30)>1){if(l22){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l1,
"VRF %d: Cannot v6-128 delete ""default route if other routes are present "
"in this mode"),l24));return SOC_E_PARAM;}}}l8 = 2;}l20 = 
L3_DEFIP_ALPM_IPV6_128m;l184 = ((uint32*)&(l182));SOC_ALPM_LPM_LOCK(l1);if(
bucket_index == 0){l124 = l141(l1,l5,l20,l184,&l137,&bucket_index,&l116,TRUE)
;}else{l124 = l16(l1,l5,l184,0,l20,0,0);}sal_memcpy(&l198,l184,sizeof(l198));
l199 = &l198;if(SOC_FAILURE(l124)){SOC_ALPM_LPM_UNLOCK(l1);LOG_ERROR(
BSL_LS_SOC_ALPM,(BSL_META_U(l1,"_soc_alpm_128_delete: Unable to find "
"prefix for delete\n")));return l124;}l201 = bucket_index;l140 = 
ALPM_TCAM_PIVOT(l1,l137);trie = PIVOT_BUCKET_TRIE(l140);l124 = trie_delete(
trie,prefix,l129,&l175);l176 = (payload_t*)l175;if(l124!= SOC_E_NONE){
LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l1,
"_soc_alpm_128_delete: Error prefix not ""present in trie \n")));
SOC_ALPM_LPM_UNLOCK(l1);return l124;}l173 = VRF_PREFIX_TRIE_IPV6_128(l1,l24);
l138 = VRF_PIVOT_TRIE_IPV6_128(l1,l24);if(!l22){l124 = trie_delete(l173,
prefix,l129,&l175);l204 = (payload_t*)l175;if(SOC_FAILURE(l124)){LOG_ERROR(
BSL_LS_SOC_ALPM,(BSL_META_U(l1,"_soc_alpm_128_delete: Prefix not present "
"in pfx trie: 0x%08x 0x%08x 0x%08x 0x%08x\n"),prefix[1],prefix[2],prefix[3],
prefix[4]));goto l205;}l139 = NULL;l124 = trie_find_lpm(l173,prefix,l129,&
l139);l178 = (payload_t*)l139;if(SOC_SUCCESS(l124)){payload_t*l206 = (
payload_t*)(l178->bkt_ptr);if(l206!= NULL){l202 = l206->len;}else{l202 = 0;}}
else{LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l1,
"_soc_alpm_128_delete: Could not find"
" replacement bpm for prefix: 0x%08x 0x%08x ""0x%08x 0x%08x\n"),prefix[1],
prefix[2],prefix[3],prefix[4]));goto l207;}sal_memcpy(l200,prefix,sizeof(
prefix));l127((l200),(l129),(l30));l124 = l26(l1,l200,l202,l24,l7,&lpm_entry,
0,1);(void)l23(l1,l184,l20,l7,l144,bucket_index,0,&lpm_entry);(void)l26(l1,
l200,l129,l24,l7,&lpm_entry,0,0);if(SOC_URPF_STATUS_GET(l1)){if(SOC_SUCCESS(
l124)){l185 = ((uint32*)&(l183));l171 = soc_mem_read(l1,l20,MEM_BLOCK_ANY,
_soc_alpm_rpf_entry(l1,l116),l185);}}if((l202 == 0)&&SOC_FAILURE(l171)){l203 = 
VRF_TRIE_DEFAULT_ROUTE_IPV6_128(l1,l24);sal_memcpy(&lpm_entry,l203,sizeof(
lpm_entry));l124 = l26(l1,prefix,l202,l24,l7,&lpm_entry,0,1);}if(SOC_FAILURE(
l124)){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l1,
"_soc_alpm_128_delete: Could not find "
"replacement prefix for prefix: 0x%08x 0x%08x 0x%08x ""0x%08x\n"),prefix[1],
prefix[2],prefix[3],prefix[4]));goto l207;}l203 = &lpm_entry;}else{l139 = 
NULL;l124 = trie_find_lpm(l173,prefix,l129,&l139);l178 = (payload_t*)l139;if(
SOC_FAILURE(l124)){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l1,
"_soc_alpm_128_delete: Could not find "
"default route in the trie for vrf %d\n"),l24));goto l205;}l178->bkt_ptr = 0;
l202 = 0;l203 = VRF_TRIE_DEFAULT_ROUTE_IPV6_128(l1,l24);}l124 = l6(l1,l203,
l30,l8,l202,&l10);if(SOC_FAILURE(l124)){goto l207;}l124 = _soc_alpm_aux_op(l1
,DELETE_PROPAGATE,&l10,TRUE,&l136,&l137,&bucket_index);if(SOC_FAILURE(l124)){
goto l207;}if(SOC_URPF_STATUS_GET(l1)){uint32 l131;if(l185!= NULL){l131 = 
soc_mem_field32_get(l1,L3_DEFIP_AUX_SCRATCHm,&l10,DB_TYPEf);l131++;
soc_mem_field32_set(l1,L3_DEFIP_AUX_SCRATCHm,&l10,DB_TYPEf,l131);l131 = 
soc_mem_field32_get(l1,l20,l185,SRC_DISCARDf);soc_mem_field32_set(l1,l20,&l10
,SRC_DISCARDf,l131);l131 = soc_mem_field32_get(l1,l20,l185,DEFAULTROUTEf);
soc_mem_field32_set(l1,l20,&l10,DEFAULTROUTEf,l131);l124 = _soc_alpm_aux_op(
l1,DELETE_PROPAGATE,&l10,TRUE,&l136,&l137,&bucket_index);}if(SOC_FAILURE(l124
)){goto l207;}}sal_free(l176);if(!l22){sal_free(l204);}
PIVOT_BUCKET_ENT_CNT_UPDATE(l140);if((l140->len!= 0)&&(trie->trie == NULL)){
uint32 l208[5];sal_memcpy(l208,l140->key,sizeof(l208));l127((l208),(l140->len
),(l7));l26(l1,l208,l140->len,l24,l7,&lpm_entry,0,1);l124 = l4(l1,&lpm_entry)
;if(SOC_FAILURE(l124)){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l1,
"_soc_alpm_128_delete: Unable to "
"delete pivot 0x%08x 0x%08x 0x%08x 0x%08x \n"),l140->key[1],l140->key[2],l140
->key[3],l140->key[4]));}}l124 = _soc_alpm_delete_in_bkt(l1,l20,l201,l145,
l199,l12,&l116,l30);if(!SOC_SUCCESS(l124)){SOC_ALPM_LPM_UNLOCK(l1);l124 = 
SOC_E_FAIL;return l124;}if(SOC_URPF_STATUS_GET(l1)){l124 = 
soc_mem_alpm_delete(l1,l20,SOC_ALPM_RPF_BKT_IDX(l1,l201),MEM_BLOCK_ALL,l145,
l199,l12,&l136);if(!SOC_SUCCESS(l124)){SOC_ALPM_LPM_UNLOCK(l1);l124 = 
SOC_E_FAIL;return l124;}}if((l140->len!= 0)&&(trie->trie == NULL)){l124 = 
alpm_bucket_release(l1,PIVOT_BUCKET_INDEX(l140),l30);if(SOC_FAILURE(l124)){
LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l1,
"_soc_alpm_128_delete: Unable to release""empty bucket: %d\n"),
PIVOT_BUCKET_INDEX(l140)));}l124 = trie_delete(l138,l140->key,l140->len,&l175
);if(SOC_FAILURE(l124)){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l1,
"could not delete pivot from pivot trie\n")));}trie_destroy(PIVOT_BUCKET_TRIE
(l140));sal_free(PIVOT_BUCKET_HANDLE(l140));sal_free(l140);
_soc_trident2_alpm_bkt_view_set(l1,l201<<2,INVALIDm);if(
SOC_ALPM_V6_SCALE_CHECK(l1,l30)){_soc_trident2_alpm_bkt_view_set(l1,(l201+1)
<<2,INVALIDm);}}}VRF_TRIE_ROUTES_DEC(l1,l24,l30);if(VRF_TRIE_ROUTES_CNT(l1,
l24,l30) == 0){l124 = l29(l1,l24,l30);}SOC_ALPM_LPM_UNLOCK(l1);return l124;
l207:l171 = trie_insert(l173,prefix,NULL,l129,(trie_node_t*)l204);if(
SOC_FAILURE(l171)){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l1,
"_soc_alpm_128_delete: Unable to reinsert"
"prefix 0x%08x 0x%08x 0x%08x 0x%08x into pfx trie\n"),prefix[1],prefix[2],
prefix[3],prefix[4]));}l205:l171 = trie_insert(trie,prefix,NULL,l129,(
trie_node_t*)l176);if(SOC_FAILURE(l171)){LOG_ERROR(BSL_LS_SOC_ALPM,(
BSL_META_U(l1,"_soc_alpm_128_delete: Unable to reinsert"
"prefix 0x%08x 0x%08x 0x%08x 0x%08x into pfx trie\n"),prefix[1],prefix[2],
prefix[3],prefix[4]));}SOC_ALPM_LPM_UNLOCK(l1);return l124;}int
soc_alpm_128_init(int l1){int l124 = SOC_E_NONE;l124 = soc_alpm_128_lpm_init(
l1);SOC_IF_ERROR_RETURN(l124);return l124;}int soc_alpm_128_state_clear(int l1
){int l123,l124;for(l123 = 0;l123<= SOC_VRF_MAX(l1)+1;l123++){l124 = 
trie_traverse(VRF_PREFIX_TRIE_IPV6_128(l1,l123),alpm_delete_node_cb,NULL,
_TRIE_INORDER_TRAVERSE);if(SOC_SUCCESS(l124)){trie_destroy(
VRF_PREFIX_TRIE_IPV6_128(l1,l123));}else{LOG_ERROR(BSL_LS_SOC_ALPM,(
BSL_META_U(l1,"unit: %d Unable to clear v6_128 pfx trie for ""vrf %d\n"),l1,
l123));return l124;}if(VRF_TRIE_DEFAULT_ROUTE_IPV6_128(l1,l123)!= NULL){
sal_free(VRF_TRIE_DEFAULT_ROUTE_IPV6_128(l1,l123));}}return SOC_E_NONE;}int
soc_alpm_128_deinit(int l1){soc_alpm_128_lpm_deinit(l1);SOC_IF_ERROR_RETURN(
soc_alpm_128_state_clear(l1));return SOC_E_NONE;}static int l209(int l1,int
l24,int l30){defip_pair_128_entry_t*lpm_entry,l210;int l211;int index;int l124
= SOC_E_NONE;uint32 key[5] = {0,0,0,0,0};uint32 l129;alpm_bucket_handle_t*
l179;alpm_pivot_t*l140;payload_t*l204;trie_t*l212;trie_t*l213 = NULL;
trie_init(_MAX_KEY_LEN_144_,&VRF_PIVOT_TRIE_IPV6_128(l1,l24));l213 = 
VRF_PIVOT_TRIE_IPV6_128(l1,l24);trie_init(_MAX_KEY_LEN_144_,&
VRF_PREFIX_TRIE_IPV6_128(l1,l24));l212 = VRF_PREFIX_TRIE_IPV6_128(l1,l24);
lpm_entry = sal_alloc(sizeof(*lpm_entry),"Default 128 LPM entry");if(
lpm_entry == NULL){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l1,
"soc_alpm_128_vrf_add: unable to allocate memory ""for IPv6-128 LPM entry\n")
));return SOC_E_MEMORY;}l26(l1,key,0,l24,l30,lpm_entry,0,1);
VRF_TRIE_DEFAULT_ROUTE_IPV6_128(l1,l24) = lpm_entry;if(l24 == SOC_VRF_MAX(l1)
+1){soc_L3_DEFIP_PAIR_128m_field32_set(l1,lpm_entry,GLOBAL_ROUTEf,1);}else{
soc_L3_DEFIP_PAIR_128m_field32_set(l1,lpm_entry,DEFAULT_MISSf,1);}l124 = 
alpm_bucket_assign(l1,&l211,l30);soc_L3_DEFIP_PAIR_128m_field32_set(l1,
lpm_entry,ALG_BKT_PTRf,l211);sal_memcpy(&l210,lpm_entry,sizeof(l210));l124 = 
l2(l1,&l210,&index);l179 = sal_alloc(sizeof(alpm_bucket_handle_t),
"ALPM Bucket Handle");if(l179 == NULL){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(
l1,"soc_alpm_128_vrf_add: Unable to allocate memory ""for bucket handle \n"))
);return SOC_E_NONE;}sal_memset(l179,0,sizeof(*l179));l140 = sal_alloc(sizeof
(alpm_pivot_t),"Payload for Pivot");if(l140 == NULL){LOG_ERROR(
BSL_LS_SOC_ALPM,(BSL_META_U(l1,
"soc_alpm_128_vrf_add: Unable to allocate memory ""for PIVOT trie node \n")))
;sal_free(l179);return SOC_E_MEMORY;}l204 = sal_alloc(sizeof(payload_t),
"Payload for pfx trie key");if(l204 == NULL){LOG_ERROR(BSL_LS_SOC_ALPM,(
BSL_META_U(l1,"soc_alpm_128_vrf_add: Unable to allocate memory "
"for pfx trie node \n")));sal_free(l179);sal_free(l140);return SOC_E_MEMORY;}
sal_memset(l140,0,sizeof(*l140));sal_memset(l204,0,sizeof(*l204));l129 = 0;
PIVOT_BUCKET_HANDLE(l140) = l179;trie_init(_MAX_KEY_LEN_144_,&
PIVOT_BUCKET_TRIE(l140));PIVOT_BUCKET_INDEX(l140) = l211;PIVOT_BUCKET_VRF(
l140) = l24;PIVOT_BUCKET_IPV6(l140) = l30;PIVOT_BUCKET_DEF(l140) = TRUE;l140
->key[0] = l204->key[0] = key[0];l140->key[1] = l204->key[1] = key[1];l140->
key[2] = l204->key[2] = key[2];l140->key[3] = l204->key[3] = key[3];l140->key
[4] = l204->key[4] = key[4];l140->len = l204->len = l129;l124 = trie_insert(
l212,key,NULL,l129,&(l204->node));if(SOC_FAILURE(l124)){sal_free(l204);
sal_free(l140);sal_free(l179);return l124;}l124 = trie_insert(l213,key,NULL,
l129,(trie_node_t*)l140);if(SOC_FAILURE(l124)){trie_node_t*l175 = NULL;(void)
trie_delete(l212,key,l129,&l175);sal_free(l204);sal_free(l140);sal_free(l179)
;return l124;}index = soc_alpm_physical_idx(l1,L3_DEFIP_PAIR_128m,index,l30);
ALPM_TCAM_PIVOT(l1,SOC_ALPM_128_ADDR_LWR(index)<<1) = l140;PIVOT_TCAM_INDEX(
l140) = SOC_ALPM_128_ADDR_LWR(index)<<1;VRF_PIVOT_REF_INC(l1,l24,l30);
VRF_TRIE_INIT_DONE(l1,l24,l30,1);return l124;}static int l29(int l1,int l24,
int l30){defip_pair_128_entry_t*lpm_entry;int l211;int l214;int l124 = 
SOC_E_NONE;uint32 key[2] = {0,0},l128[SOC_MAX_MEM_FIELD_WORDS];payload_t*l176
;alpm_pivot_t*l215;trie_node_t*l175;trie_t*l212;trie_t*l213 = NULL;lpm_entry = 
VRF_TRIE_DEFAULT_ROUTE_IPV6_128(l1,l24);l211 = 
soc_L3_DEFIP_PAIR_128m_field32_get(l1,lpm_entry,ALG_BKT_PTRf);l124 = 
alpm_bucket_release(l1,l211,l30);_soc_trident2_alpm_bkt_view_set(l1,l211<<2,
INVALIDm);if(SOC_ALPM_V6_SCALE_CHECK(l1,l30)){_soc_trident2_alpm_bkt_view_set
(l1,(l211+1)<<2,INVALIDm);}l124 = l15(l1,lpm_entry,(void*)l128,&l214);if(
SOC_FAILURE(l124)){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l1,
"soc_alpm_vrf_delete: unable to get internal"" pivot idx for vrf %d/%d\n"),
l24,l30));l214 = -1;}l214 = soc_alpm_physical_idx(l1,L3_DEFIP_PAIR_128m,l214,
l30);l215 = ALPM_TCAM_PIVOT(l1,SOC_ALPM_128_ADDR_LWR(l214)<<1);l124 = l4(l1,
lpm_entry);if(SOC_FAILURE(l124)){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l1,
"soc_alpm_128_vrf_delete: unable to delete lpm "
"entry for internal default for vrf %d/%d\n"),l24,l30));}sal_free(lpm_entry);
VRF_TRIE_DEFAULT_ROUTE_IPV6_128(l1,l24) = NULL;l212 = 
VRF_PREFIX_TRIE_IPV6_128(l1,l24);VRF_PREFIX_TRIE_IPV6_128(l1,l24) = NULL;
VRF_TRIE_INIT_DONE(l1,l24,l30,0);l124 = trie_delete(l212,key,0,&l175);l176 = 
(payload_t*)l175;if(SOC_FAILURE(l124)){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(
l1,"Unable to delete internal default for 128b vrf "" %d/%d\n"),l24,l30));}
sal_free(l176);(void)trie_destroy(l212);l213 = VRF_PIVOT_TRIE_IPV6_128(l1,l24
);VRF_PIVOT_TRIE_IPV6_128(l1,l24) = NULL;l175 = NULL;l124 = trie_delete(l213,
key,0,&l175);if(SOC_FAILURE(l124)){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l1,
"Unable to delete internal pivot node for vrf"" %d/%d\n"),l24,l30));}(void)
trie_destroy(l213);sal_free(PIVOT_BUCKET_HANDLE(l215));(void)trie_destroy(
PIVOT_BUCKET_TRIE(l215));sal_free(l215);return l124;}int soc_alpm_128_insert(
int l1,void*l3,uint32 l21,int l216,int l217){defip_alpm_ipv6_128_entry_t l182
,l183;soc_mem_t l20;void*l184,*l199;int l144,l24;int index;int l7;int l124 = 
SOC_E_NONE;uint32 l22;l7 = L3_DEFIP_MODE_128;l20 = L3_DEFIP_ALPM_IPV6_128m;
l184 = ((uint32*)&(l182));l199 = ((uint32*)&(l183));SOC_IF_ERROR_RETURN(l16(
l1,l3,l184,l199,l20,l21,&l22));SOC_IF_ERROR_RETURN(soc_alpm_128_lpm_vrf_get(
l1,l3,&l144,&l24));if(l144 == SOC_L3_VRF_OVERRIDE){l124 = l2(l1,l3,&index);if
(SOC_SUCCESS(l124)){VRF_TRIE_ROUTES_INC(l1,MAX_VRF_ID,l7);VRF_PIVOT_REF_INC(
l1,MAX_VRF_ID,l7);}else if(l124 == SOC_E_FULL){VRF_PIVOT_FULL_INC(l1,
MAX_VRF_ID,l7);}return(l124);}else if(l24 == 0){if(soc_alpm_mode_get(l1)){
LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l1,
"Unit %d, VRF=0 cannot be added in Parallel mode\n"),l1));return SOC_E_PARAM;
}}if(l144!= SOC_L3_VRF_GLOBAL){if(!soc_alpm_mode_get(l1)){if(
VRF_TRIE_ROUTES_CNT(l1,l24,l7) == 0){if(!l22){LOG_ERROR(BSL_LS_SOC_ALPM,(
BSL_META_U(l1,"VRF %d: First route in a VRF has to "
" be a default route in this mode\n"),l144));return SOC_E_PARAM;}}}}if(!
VRF_TRIE_INIT_COMPLETED(l1,l24,l7)){LOG_VERBOSE(BSL_LS_SOC_ALPM,(BSL_META_U(
l1,"soc_alpm_128_insert:VRF %d is not ""initialized\n"),l24));l124 = l209(l1,
l24,l7);if(SOC_FAILURE(l124)){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l1,
"soc_alpm_128_insert:VRF %d/%d trie init \n""failed\n"),l24,l7));return l124;
}LOG_VERBOSE(BSL_LS_SOC_ALPM,(BSL_META_U(l1,
"soc_alpm_128_insert:VRF %d/%d trie init ""completed\n"),l24,l7));}if(l217&
SOC_ALPM_LOOKUP_HIT){l124 = l146(l1,l3,l184,l199,l20,l216);}else{if(l216 == -
1){l216 = 0;}l124 = l166(l1,l3,l20,l184,l199,&index,SOC_ALPM_BKT_ENTRY_TO_IDX
(l216),l217);}if(l124!= SOC_E_NONE){LOG_WARN(BSL_LS_SOC_ALPM,(BSL_META_U(l1,
"unit %d :soc_alpm_128_insert: ""Route Insertion Failed :%s\n"),l1,soc_errmsg
(l124)));}return(l124);}int soc_alpm_128_lookup(int l1,void*l5,void*l12,int*
l13,int*l218){defip_alpm_ipv6_128_entry_t l182;soc_mem_t l20;int bucket_index
;int l137;void*l184;int l144,l24;int l7 = 2,l115;int l124 = SOC_E_NONE;
SOC_IF_ERROR_RETURN(soc_alpm_128_lpm_vrf_get(l1,l5,&l144,&l24));l124 = l11(l1
,l5,l12,l13,&l115,&l7);if(SOC_SUCCESS(l124)){SOC_IF_ERROR_RETURN(
soc_alpm_128_lpm_vrf_get(l1,l12,&l144,&l24));if(l144 == SOC_L3_VRF_OVERRIDE){
return SOC_E_NONE;}}if(!VRF_TRIE_INIT_COMPLETED(l1,l24,l7)){LOG_VERBOSE(
BSL_LS_SOC_ALPM,(BSL_META_U(l1,"soc_alpm_lookup:VRF %d is not "
"initialized\n"),l24));*l13 = 0;*l218 = 0;return SOC_E_NOT_FOUND;}l20 = 
L3_DEFIP_ALPM_IPV6_128m;l184 = ((uint32*)&(l182));SOC_ALPM_LPM_LOCK(l1);l124 = 
l141(l1,l5,l20,l184,&l137,&bucket_index,l13,TRUE);SOC_ALPM_LPM_UNLOCK(l1);if(
SOC_FAILURE(l124)){*l218 = l137;*l13 = bucket_index<<2;return l124;}l124 = 
l23(l1,l184,l20,l7,l144,bucket_index,*l13,l12);*l218 = SOC_ALPM_LOOKUP_HIT|
l137;return(l124);}int l219(int l1,void*l5,void*l12,int l24,int*l137,int*
bucket_index,int*l116,int l220){int l124 = SOC_E_NONE;int l123,l221,l30,l136 = 
0;uint32 l8,l145;defip_aux_scratch_entry_t l10;int l222,l223;int index;
soc_mem_t l20,l224;int l225,l226,l227;soc_field_t l228[4] = {IP_ADDR0_LWRf,
IP_ADDR1_LWRf,IP_ADDR0_UPRf,IP_ADDR1_UPRf};uint32 l229[
SOC_MAX_MEM_FIELD_WORDS] = {0};int l230 = -1;int l231 = 0;l30 = 
L3_DEFIP_MODE_128;l224 = L3_DEFIP_PAIR_128m;l222 = soc_mem_field32_get(l1,
l224,l5,GLOBAL_ROUTEf);l223 = soc_mem_field32_get(l1,l224,l5,VRF_ID_0_LWRf);
LOG_VERBOSE(BSL_LS_SOC_ALPM,(BSL_META_U(l1,
"Prefare AUX Scratch for searching TCAM in "
"%s region, Key data: v6 %d global %d vrf %d\n"),l24 == SOC_L3_VRF_GLOBAL?
"Global":"VRF",l30,l222,l223));if(l24 == SOC_L3_VRF_GLOBAL){l8 = l220?1:0;
SOC_ALPM_GET_GLOBAL_BANK_DISABLE(l1,l145);soc_mem_field32_set(l1,l224,l5,
GLOBAL_ROUTEf,1);soc_mem_field32_set(l1,l224,l5,VRF_ID_0_LWRf,0);}else{l8 = 
l220?3:2;SOC_ALPM_GET_VRF_BANK_DISABLE(l1,l145);}sal_memset(&l10,0,sizeof(
defip_aux_scratch_entry_t));SOC_IF_ERROR_RETURN(l6(l1,l5,l30,l8,0,&l10));if(
l24 == SOC_L3_VRF_GLOBAL){soc_mem_field32_set(l1,l224,l5,GLOBAL_ROUTEf,l222);
soc_mem_field32_set(l1,l224,l5,VRF_ID_0_LWRf,l223);}SOC_IF_ERROR_RETURN(
_soc_alpm_aux_op(l1,PREFIX_LOOKUP,&l10,TRUE,&l136,l137,bucket_index));if(l136
== 0){LOG_VERBOSE(BSL_LS_SOC_ALPM,(BSL_META_U(l1,"Could not find bucket\n")))
;return SOC_E_NOT_FOUND;}LOG_VERBOSE(BSL_LS_SOC_ALPM,(BSL_META_U(l1,
"Hit in memory %s, index %d, ""bucket_index %d\n"),SOC_MEM_NAME(l1,l224),
soc_alpm_logical_idx(l1,l224,SOC_ALPM_128_DEFIP_TO_PAIR((*l137)>>1),1),*
bucket_index));l20 = L3_DEFIP_ALPM_IPV6_128m;l124 = l122(l1,l5,&l226);if(
SOC_FAILURE(l124)){return l124;}l227 = SOC_ALPM_V6_SCALE_CHECK(l1,l30)?16:8;
LOG_VERBOSE(BSL_LS_SOC_ALPM,(BSL_META_U(l1,
"Start searching mem %s bucket %d(count %d) ""for Length %d\n"),SOC_MEM_NAME(
l1,l20),*bucket_index,l227,l226));for(l123 = 0;l123<l227;l123++){uint32 l184[
SOC_MAX_MEM_FIELD_WORDS] = {0};uint32 l232[4] = {0};uint32 l233[4] = {0};
uint32 l234[4] = {0};int l235;l124 = _soc_alpm_mem_index(l1,l20,*bucket_index
,l123,l145,&index);if(l124 == SOC_E_FULL){continue;}SOC_IF_ERROR_RETURN(
soc_mem_read(l1,l20,MEM_BLOCK_ANY,index,(void*)&l184));l235 = 
soc_mem_field32_get(l1,l20,&l184,VALIDf);l225 = soc_mem_field32_get(l1,l20,&
l184,LENGTHf);LOG_VERBOSE(BSL_LS_SOC_ALPM,(BSL_META_U(l1,
"Bucket %5d Index %6d: Valid %d, Length %d\n"),*bucket_index,index,l235,l225)
);if(!l235||(l225>l226)){continue;}SHR_BITSET_RANGE(l232,128-l225,l225);(void
)soc_mem_field_get(l1,l20,(uint32*)&l184,KEYf,(uint32*)l233);l234[3] = 
soc_mem_field32_get(l1,l224,l5,l228[3]);l234[2] = soc_mem_field32_get(l1,l224
,l5,l228[2]);l234[1] = soc_mem_field32_get(l1,l224,l5,l228[1]);l234[0] = 
soc_mem_field32_get(l1,l224,l5,l228[0]);LOG_VERBOSE(BSL_LS_SOC_ALPM,(
BSL_META_U(l1,"\tmask %08x %08x %08x %08x \n""\t key %08x %08x %08x %08x \n"
"\thost %08x %08x %08x %08x \n"),l232[3],l232[2],l232[1],l232[0],l233[3],l233
[2],l233[1],l233[0],l234[3],l234[2],l234[1],l234[0]));for(l221 = 3;l221>= 0;
l221--){if((l234[l221]&l232[l221])!= (l233[l221]&l232[l221])){break;}}if(l221
>= 0){continue;}LOG_VERBOSE(BSL_LS_SOC_ALPM,(BSL_META_U(l1,
"Found a match in mem %s bucket %d, ""index %d\n"),SOC_MEM_NAME(l1,l20),*
bucket_index,index));if(l230 == -1||l230<l225){l230 = l225;l231 = index;
sal_memcpy(l229,l184,sizeof(l184));}}if(l230!= -1){l124 = l23(l1,&l229,l20,
l30,l24,*bucket_index,l231,l12);if(SOC_SUCCESS(l124)){*l116 = l231;if(
bsl_check(bslLayerSoc,bslSourceAlpm,bslSeverityVerbose,l1)){LOG_VERBOSE(
BSL_LS_SOC_ALPM,(BSL_META_U(l1,"Hit mem %s bucket %d, index %d\n"),
SOC_MEM_NAME(l1,l20),*bucket_index,l231));}}return l124;}*l116 = 
soc_alpm_logical_idx(l1,l224,SOC_ALPM_128_DEFIP_TO_PAIR((*l137)>>1),1);
SOC_IF_ERROR_RETURN(soc_mem_read(l1,l224,MEM_BLOCK_ANY,*l116,(void*)l12));
return SOC_E_NONE;}int soc_alpm_128_find_best_match(int l1,void*l5,void*l12,
int*l13,int l220){int l124 = SOC_E_NONE;int l123,l221;int l236,l237;
defip_pair_128_entry_t l238;uint32 l239,l233,l234;int l225,l226;int l240,l241
;int l144,l24 = 0;int l137,bucket_index;soc_mem_t l224;soc_field_t l242[4] = 
{IP_ADDR_MASK1_UPRf,IP_ADDR_MASK0_UPRf,IP_ADDR_MASK1_LWRf,IP_ADDR_MASK0_LWRf}
;soc_field_t l243[4] = {IP_ADDR1_UPRf,IP_ADDR0_UPRf,IP_ADDR1_LWRf,
IP_ADDR0_LWRf};l224 = L3_DEFIP_PAIR_128m;if(!SOC_URPF_STATUS_GET(l1)&&l220){
return SOC_E_PARAM;}l236 = soc_mem_index_min(l1,l224);l237 = 
soc_mem_index_count(l1,l224);if(SOC_URPF_STATUS_GET(l1)){l237>>= 1;}if(
soc_alpm_mode_get(l1)){l237>>= 1;l236+= l237;}if(l220){l236+= 
soc_mem_index_count(l1,l224)/2;}LOG_VERBOSE(BSL_LS_SOC_ALPM,(BSL_META_U(l1,
"Start LPM searchng from %d, count %d\n"),l236,l237));for(l123 = l236;l123<
l236+l237;l123++){SOC_IF_ERROR_RETURN(soc_mem_read(l1,l224,MEM_BLOCK_ANY,l123
,(void*)&l238));if(!soc_mem_field32_get(l1,l224,&l238,VALID0_LWRf)){continue;
}l240 = soc_mem_field32_get(l1,l224,&l238,GLOBAL_HIGHf);l241 = 
soc_mem_field32_get(l1,l224,&l238,GLOBAL_ROUTEf);if(!l241||!l240){continue;}
l124 = l122(l1,l5,&l226);l124 = l122(l1,&l238,&l225);if(SOC_FAILURE(l124)||(
l225>l226)){continue;}for(l221 = 0;l221<4;l221++){l239 = soc_mem_field32_get(
l1,l224,&l238,l242[l221]);l233 = soc_mem_field32_get(l1,l224,&l238,l243[l221]
);l234 = soc_mem_field32_get(l1,l224,l5,l243[l221]);if((l234&l239)!= (l233&
l239)){break;}}if(l221<4){continue;}LOG_VERBOSE(BSL_LS_SOC_ALPM,(BSL_META_U(
l1,"Hit Global High route in index = %d\n"),l221));sal_memcpy(l12,&l238,
sizeof(l238));*l13 = l123;return SOC_E_NONE;}LOG_VERBOSE(BSL_LS_SOC_ALPM,(
BSL_META_U(l1,"Global high lookup miss, use AUX engine to "
"search for Global Low and VRF routes\n")));SOC_IF_ERROR_RETURN(
soc_alpm_128_lpm_vrf_get(l1,l5,&l144,&l24));l124 = l219(l1,l5,l12,l24,&l137,&
bucket_index,l13,l220);if(l124 == SOC_E_NOT_FOUND){l24 = SOC_L3_VRF_GLOBAL;
LOG_VERBOSE(BSL_LS_SOC_ALPM,(BSL_META_U(l1,
"Not found in VRF region, try Global ""region\n")));l124 = l219(l1,l5,l12,l24
,&l137,&bucket_index,l13,l220);}if(SOC_SUCCESS(l124)){l137 = 
soc_alpm_logical_idx(l1,l224,SOC_ALPM_128_DEFIP_TO_PAIR(l137>>1),1);
LOG_VERBOSE(BSL_LS_SOC_ALPM,(BSL_META_U(l1,
"Hit in %s region in TCAM index %d, ""buckekt_index %d\n"),l24 == 
SOC_L3_VRF_GLOBAL?"Global Low":"VRF",l137,bucket_index));}else{LOG_VERBOSE(
BSL_LS_SOC_ALPM,(BSL_META_U(l1,"Search miss for given address\n")));}return(
l124);}int soc_alpm_128_delete(int l1,void*l5,int l216,int l217){int l144,l24
;int l7;int l124 = SOC_E_NONE;l7 = L3_DEFIP_MODE_128;SOC_IF_ERROR_RETURN(
soc_alpm_128_lpm_vrf_get(l1,l5,&l144,&l24));if(l144 == SOC_L3_VRF_OVERRIDE){
l124 = l4(l1,l5);if(SOC_SUCCESS(l124)){VRF_TRIE_ROUTES_DEC(l1,MAX_VRF_ID,l7);
VRF_PIVOT_REF_DEC(l1,MAX_VRF_ID,l7);}return(l124);}else{if(!
VRF_TRIE_INIT_COMPLETED(l1,l24,l7)){LOG_VERBOSE(BSL_LS_SOC_ALPM,(BSL_META_U(
l1,"soc_alpm_128_delete:VRF %d/%d is not ""initialized\n"),l24,l7));return
SOC_E_NONE;}if(l216 == -1){l216 = 0;}l124 = l197(l1,l5,
SOC_ALPM_BKT_ENTRY_TO_IDX(l216),l217&~SOC_ALPM_LOOKUP_HIT,l216);}return(l124)
;}static void l103(int l1,void*l12,int index,l98 l104){l104[0] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l12
),(l91[(l1)]->l50));l104[1] = soc_meminfo_fieldinfo_field32_get((&
SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l12),(l91[(l1)]->l48));l104[2] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l12
),(l91[(l1)]->l54));l104[3] = soc_meminfo_fieldinfo_field32_get((&
SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l12),(l91[(l1)]->l52));l104[4] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l12
),(l91[(l1)]->l51));l104[5] = soc_meminfo_fieldinfo_field32_get((&
SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l12),(l91[(l1)]->l49));l104[6] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l12
),(l91[(l1)]->l55));l104[7] = soc_meminfo_fieldinfo_field32_get((&
SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l12),(l91[(l1)]->l53));if((!(
SOC_IS_HURRICANE(l1)))&&(((l91[(l1)]->l73)!= NULL))){int l244;(void)
soc_alpm_128_lpm_vrf_get(l1,l12,(int*)&l104[8],&l244);}else{l104[8] = 0;};}
static int l245(l98 l100,l98 l101){int l214;for(l214 = 0;l214<9;l214++){{if((
l100[l214])<(l101[l214])){return-1;}if((l100[l214])>(l101[l214])){return 1;}}
;}return(0);}static void l246(int l1,void*l3,uint32 l247,uint32 l118,int l115
){l98 l248;if(soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,
L3_DEFIP_PAIR_128m)),(l3),(l91[(l1)]->l70))&&
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l3)
,(l91[(l1)]->l69))&&soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,
L3_DEFIP_PAIR_128m)),(l3),(l91[(l1)]->l68))&&
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l3)
,(l91[(l1)]->l67))){l248[0] = soc_meminfo_fieldinfo_field32_get((&
SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l3),(l91[(l1)]->l50));l248[1] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l3)
,(l91[(l1)]->l48));l248[2] = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO
(l1,L3_DEFIP_PAIR_128m)),(l3),(l91[(l1)]->l54));l248[3] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l3)
,(l91[(l1)]->l52));l248[4] = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO
(l1,L3_DEFIP_PAIR_128m)),(l3),(l91[(l1)]->l51));l248[5] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l3)
,(l91[(l1)]->l49));l248[6] = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO
(l1,L3_DEFIP_PAIR_128m)),(l3),(l91[(l1)]->l55));l248[7] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l3)
,(l91[(l1)]->l53));if((!(SOC_IS_HURRICANE(l1)))&&(((l91[(l1)]->l73)!= NULL)))
{int l244;(void)soc_alpm_128_lpm_vrf_get(l1,l3,(int*)&l248[8],&l244);}else{
l248[8] = 0;};l117((l102[(l1)]),l245,l248,l115,l118,l247);}}static void l249(
int l1,void*l5,uint32 l247){l98 l248;int l115 = -1;int l124;uint16 index;l248
[0] = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)
),(l5),(l91[(l1)]->l50));l248[1] = soc_meminfo_fieldinfo_field32_get((&
SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l5),(l91[(l1)]->l48));l248[2] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l5)
,(l91[(l1)]->l54));l248[3] = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO
(l1,L3_DEFIP_PAIR_128m)),(l5),(l91[(l1)]->l52));l248[4] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l5)
,(l91[(l1)]->l51));l248[5] = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO
(l1,L3_DEFIP_PAIR_128m)),(l5),(l91[(l1)]->l49));l248[6] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l5)
,(l91[(l1)]->l55));l248[7] = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO
(l1,L3_DEFIP_PAIR_128m)),(l5),(l91[(l1)]->l53));if((!(SOC_IS_HURRICANE(l1)))
&&(((l91[(l1)]->l73)!= NULL))){int l244;(void)soc_alpm_128_lpm_vrf_get(l1,l5,
(int*)&l248[8],&l244);}else{l248[8] = 0;};index = l247;l124 = l120((l102[(l1)
]),l245,l248,l115,index);if(SOC_FAILURE(l124)){LOG_ERROR(BSL_LS_SOC_ALPM,(
BSL_META_U(l1,"\ndel  index: H %d error %d\n"),index,l124));}}static int l250
(int l1,void*l5,int l115,int*l116){l98 l248;int l124;uint16 index = (0xFFFF);
l248[0] = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,
L3_DEFIP_PAIR_128m)),(l5),(l91[(l1)]->l50));l248[1] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l5)
,(l91[(l1)]->l48));l248[2] = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO
(l1,L3_DEFIP_PAIR_128m)),(l5),(l91[(l1)]->l54));l248[3] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l5)
,(l91[(l1)]->l52));l248[4] = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO
(l1,L3_DEFIP_PAIR_128m)),(l5),(l91[(l1)]->l51));l248[5] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l5)
,(l91[(l1)]->l49));l248[6] = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO
(l1,L3_DEFIP_PAIR_128m)),(l5),(l91[(l1)]->l55));l248[7] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l5)
,(l91[(l1)]->l53));if((!(SOC_IS_HURRICANE(l1)))&&(((l91[(l1)]->l73)!= NULL)))
{int l244;(void)soc_alpm_128_lpm_vrf_get(l1,l5,(int*)&l248[8],&l244);}else{
l248[8] = 0;};l124 = l112((l102[(l1)]),l245,l248,l115,&index);if(SOC_FAILURE(
l124)){*l116 = 0xFFFFFFFF;return(l124);}*l116 = index;return(SOC_E_NONE);}
static uint16 l105(uint8*l106,int l107){return(_shr_crc16b(0,l106,l107));}
static int l108(int l17,int l93,int l94,l97**l109){l97*l113;int index;if(l94>
l93){return SOC_E_MEMORY;}l113 = sal_alloc(sizeof(l97),"lpm_hash");if(l113 == 
NULL){return SOC_E_MEMORY;}sal_memset(l113,0,sizeof(*l113));l113->l17 = l17;
l113->l93 = l93;l113->l94 = l94;l113->l95 = sal_alloc(l113->l94*sizeof(*(l113
->l95)),"hash_table");if(l113->l95 == NULL){sal_free(l113);return SOC_E_MEMORY
;}l113->l96 = sal_alloc(l113->l93*sizeof(*(l113->l96)),"link_table");if(l113
->l96 == NULL){sal_free(l113->l95);sal_free(l113);return SOC_E_MEMORY;}for(
index = 0;index<l113->l94;index++){l113->l95[index] = (0xFFFF);}for(index = 0
;index<l113->l93;index++){l113->l96[index] = (0xFFFF);}*l109 = l113;return
SOC_E_NONE;}static int l110(l97*l111){if(l111!= NULL){sal_free(l111->l95);
sal_free(l111->l96);sal_free(l111);}return SOC_E_NONE;}static int l112(l97*
l113,l99 l114,l98 entry,int l115,uint16*l116){int l1 = l113->l17;uint16 l251;
uint16 index;l251 = l105((uint8*)entry,(32*9))%l113->l94;index = l113->l95[
l251];;;while(index!= (0xFFFF)){uint32 l12[SOC_MAX_MEM_FIELD_WORDS];l98 l104;
int l252;l252 = index;SOC_IF_ERROR_RETURN(READ_L3_DEFIP_PAIR_128m(l1,
MEM_BLOCK_ANY,l252,l12));l103(l1,l12,index,l104);if((*l114)(entry,l104) == 0)
{*l116 = index;;return(SOC_E_NONE);}index = l113->l96[index&(0x3FFF)];;};
return(SOC_E_NOT_FOUND);}static int l117(l97*l113,l99 l114,l98 entry,int l115
,uint16 l118,uint16 l119){int l1 = l113->l17;uint16 l251;uint16 index;uint16
l253;l251 = l105((uint8*)entry,(32*9))%l113->l94;index = l113->l95[l251];;;;
l253 = (0xFFFF);if(l118!= (0xFFFF)){while(index!= (0xFFFF)){uint32 l12[
SOC_MAX_MEM_FIELD_WORDS];l98 l104;int l252;l252 = index;SOC_IF_ERROR_RETURN(
READ_L3_DEFIP_PAIR_128m(l1,MEM_BLOCK_ANY,l252,l12));l103(l1,l12,index,l104);
if((*l114)(entry,l104) == 0){if(l119!= index){;if(l253 == (0xFFFF)){l113->l95
[l251] = l119;l113->l96[l119&(0x3FFF)] = l113->l96[index&(0x3FFF)];l113->l96[
index&(0x3FFF)] = (0xFFFF);}else{l113->l96[l253&(0x3FFF)] = l119;l113->l96[
l119&(0x3FFF)] = l113->l96[index&(0x3FFF)];l113->l96[index&(0x3FFF)] = (
0xFFFF);}};return(SOC_E_NONE);}l253 = index;index = l113->l96[index&(0x3FFF)]
;;}}l113->l96[l119&(0x3FFF)] = l113->l95[l251];l113->l95[l251] = l119;return(
SOC_E_NONE);}static int l120(l97*l113,l99 l114,l98 entry,int l115,uint16 l121
){uint16 l251;uint16 index;uint16 l253;l251 = l105((uint8*)entry,(32*9))%l113
->l94;index = l113->l95[l251];;;l253 = (0xFFFF);while(index!= (0xFFFF)){if(
l121 == index){;if(l253 == (0xFFFF)){l113->l95[l251] = l113->l96[l121&(0x3FFF
)];l113->l96[l121&(0x3FFF)] = (0xFFFF);}else{l113->l96[l253&(0x3FFF)] = l113
->l96[l121&(0x3FFF)];l113->l96[l121&(0x3FFF)] = (0xFFFF);}return(SOC_E_NONE);
}l253 = index;index = l113->l96[index&(0x3FFF)];;}return(SOC_E_NOT_FOUND);}
static int l254(int l1,void*l12){return(SOC_E_NONE);}void
soc_alpm_128_lpm_state_dump(int l1){int l123;int l255;l255 = ((3*(128+2+1))-1
);if(!bsl_check(bslLayerSoc,bslSourceAlpm,bslSeverityVerbose,l1)){return;}for
(l123 = l255;l123>= 0;l123--){if((l123!= ((3*(128+2+1))-1))&&((l39[(l1)][(
l123)].l32) == -1)){continue;}LOG_VERBOSE(BSL_LS_SOC_ALPM,(BSL_META_U(l1,
"PFX = %d P = %d N = %d START = %d ""END = %d VENT = %d FENT = %d\n"),l123,(
l39[(l1)][(l123)].l34),(l39[(l1)][(l123)].next),(l39[(l1)][(l123)].l32),(l39[
(l1)][(l123)].l33),(l39[(l1)][(l123)].l35),(l39[(l1)][(l123)].l36)));}
COMPILER_REFERENCE(l254);}static int l256(int l1,int index,uint32*l12){int
l257;uint32 l258,l259,l260;uint32 l261;int l262;if(!SOC_URPF_STATUS_GET(l1)){
return(SOC_E_NONE);}if(soc_feature(l1,soc_feature_l3_defip_hole)){l257 = (
soc_mem_index_count(l1,L3_DEFIP_PAIR_128m)>>1);}else if(SOC_IS_APOLLO(l1)){
l257 = (soc_mem_index_count(l1,L3_DEFIP_PAIR_128m)>>1)+0x0400;}else{l257 = (
soc_mem_index_count(l1,L3_DEFIP_PAIR_128m)>>1);}if(((l91[(l1)]->l42)!= NULL))
{soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(
l12),(l91[(l1)]->l42),(0));}l258 = soc_meminfo_fieldinfo_field32_get((&
SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l12),(l91[(l1)]->l54));l261 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l12
),(l91[(l1)]->l55));l259 = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(
l1,L3_DEFIP_PAIR_128m)),(l12),(l91[(l1)]->l52));l260 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l12
),(l91[(l1)]->l53));l262 = ((!l258)&&(!l261)&&(!l259)&&(!l260))?1:0;l258 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l12
),(l91[(l1)]->l69));l261 = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(
l1,L3_DEFIP_PAIR_128m)),(l12),(l91[(l1)]->l67));l259 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l12
),(l91[(l1)]->l68));l260 = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(
l1,L3_DEFIP_PAIR_128m)),(l12),(l91[(l1)]->l68));if(l258&&l261&&l259&&l260){
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l12
),(l91[(l1)]->l66),(l262));}return l163(l1,MEM_BLOCK_ANY,index+l257,index,l12
);}static int l263(int l1,int l264,int l265){uint32 l12[
SOC_MAX_MEM_FIELD_WORDS];SOC_IF_ERROR_RETURN(READ_L3_DEFIP_PAIR_128m(l1,
MEM_BLOCK_ANY,l264,l12));l246(l1,l12,l265,0x4000,0);SOC_IF_ERROR_RETURN(l163(
l1,MEM_BLOCK_ANY,l265,l264,l12));SOC_IF_ERROR_RETURN(l256(l1,l265,l12));do{
int l266,l267;l266 = soc_alpm_physical_idx((l1),L3_DEFIP_PAIR_128m,(l264),1);
l267 = soc_alpm_physical_idx((l1),L3_DEFIP_PAIR_128m,(l265),1);
ALPM_TCAM_PIVOT(l1,SOC_ALPM_128_ADDR_LWR((l267))<<1) = ALPM_TCAM_PIVOT(l1,
SOC_ALPM_128_ADDR_LWR((l266))<<1);if(ALPM_TCAM_PIVOT(l1,SOC_ALPM_128_ADDR_LWR
((l267))<<1)){PIVOT_TCAM_INDEX(ALPM_TCAM_PIVOT(l1,SOC_ALPM_128_ADDR_LWR((l267
))<<1)) = SOC_ALPM_128_ADDR_LWR((l267))<<1;}ALPM_TCAM_PIVOT(l1,
SOC_ALPM_128_ADDR_LWR((l266))<<1) = NULL;}while(0);return(SOC_E_NONE);}static
int l268(int l1,int l115,int l7){int l264;int l265;l265 = (l39[(l1)][(l115)].
l33)+1;l264 = (l39[(l1)][(l115)].l32);if(l264!= l265){SOC_IF_ERROR_RETURN(
l263(l1,l264,l265));VRF_PIVOT_SHIFT_INC(l1,MAX_VRF_ID,l7);}(l39[(l1)][(l115)]
.l32)+= 1;(l39[(l1)][(l115)].l33)+= 1;return(SOC_E_NONE);}static int l269(int
l1,int l115,int l7){int l264;int l265;l265 = (l39[(l1)][(l115)].l32)-1;if((
l39[(l1)][(l115)].l35) == 0){(l39[(l1)][(l115)].l32) = l265;(l39[(l1)][(l115)
].l33) = l265-1;return(SOC_E_NONE);}l264 = (l39[(l1)][(l115)].l33);
SOC_IF_ERROR_RETURN(l263(l1,l264,l265));VRF_PIVOT_SHIFT_INC(l1,MAX_VRF_ID,l7)
;(l39[(l1)][(l115)].l32)-= 1;(l39[(l1)][(l115)].l33)-= 1;return(SOC_E_NONE);}
static int l270(int l1,int l115,int l7,void*l12,int*l271){int l272;int l273;
int l274;int l275;if((l39[(l1)][(l115)].l35) == 0){l275 = ((3*(128+2+1))-1);
if(soc_alpm_mode_get(l1) == SOC_ALPM_MODE_PARALLEL){if(l115<= (((3*(128+2+1))
/3)-1)){l275 = (((3*(128+2+1))/3)-1);}}while((l39[(l1)][(l275)].next)>l115){
l275 = (l39[(l1)][(l275)].next);}l273 = (l39[(l1)][(l275)].next);if(l273!= -1
){(l39[(l1)][(l273)].l34) = l115;}(l39[(l1)][(l115)].next) = (l39[(l1)][(l275
)].next);(l39[(l1)][(l115)].l34) = l275;(l39[(l1)][(l275)].next) = l115;(l39[
(l1)][(l115)].l36) = ((l39[(l1)][(l275)].l36)+1)/2;(l39[(l1)][(l275)].l36)-= 
(l39[(l1)][(l115)].l36);(l39[(l1)][(l115)].l32) = (l39[(l1)][(l275)].l33)+(
l39[(l1)][(l275)].l36)+1;(l39[(l1)][(l115)].l33) = (l39[(l1)][(l115)].l32)-1;
(l39[(l1)][(l115)].l35) = 0;}l274 = l115;while((l39[(l1)][(l274)].l36) == 0){
l274 = (l39[(l1)][(l274)].next);if(l274 == -1){l274 = l115;break;}}while((l39
[(l1)][(l274)].l36) == 0){l274 = (l39[(l1)][(l274)].l34);if(l274 == -1){if((
l39[(l1)][(l115)].l35) == 0){l272 = (l39[(l1)][(l115)].l34);l273 = (l39[(l1)]
[(l115)].next);if(-1!= l272){(l39[(l1)][(l272)].next) = l273;}if(-1!= l273){(
l39[(l1)][(l273)].l34) = l272;}}return(SOC_E_FULL);}}while(l274>l115){l273 = 
(l39[(l1)][(l274)].next);SOC_IF_ERROR_RETURN(l269(l1,l273,l7));(l39[(l1)][(
l274)].l36)-= 1;(l39[(l1)][(l273)].l36)+= 1;l274 = l273;}while(l274<l115){
SOC_IF_ERROR_RETURN(l268(l1,l274,l7));(l39[(l1)][(l274)].l36)-= 1;l272 = (l39
[(l1)][(l274)].l34);(l39[(l1)][(l272)].l36)+= 1;l274 = l272;}(l39[(l1)][(l115
)].l35)+= 1;(l39[(l1)][(l115)].l36)-= 1;(l39[(l1)][(l115)].l33)+= 1;*l271 = (
l39[(l1)][(l115)].l33);sal_memcpy(l12,soc_mem_entry_null(l1,
L3_DEFIP_PAIR_128m),soc_mem_entry_words(l1,L3_DEFIP_PAIR_128m)*4);return(
SOC_E_NONE);}static int l276(int l1,int l115,int l7,void*l12,int l277){int
l272;int l273;int l264;int l265;uint32 l278[SOC_MAX_MEM_FIELD_WORDS];int l124
;int l131;l264 = (l39[(l1)][(l115)].l33);l265 = l277;(l39[(l1)][(l115)].l35)
-= 1;(l39[(l1)][(l115)].l36)+= 1;(l39[(l1)][(l115)].l33)-= 1;if(l265!= l264){
if((l124 = READ_L3_DEFIP_PAIR_128m(l1,MEM_BLOCK_ANY,l264,l278))<0){return l124
;}l246(l1,l278,l265,0x4000,0);if((l124 = l163(l1,MEM_BLOCK_ANY,l265,l264,l278
))<0){return l124;}if((l124 = l256(l1,l265,l278))<0){return l124;}}l131 = 
soc_alpm_physical_idx(l1,L3_DEFIP_PAIR_128m,l265,1);l277 = 
soc_alpm_physical_idx(l1,L3_DEFIP_PAIR_128m,l264,1);ALPM_TCAM_PIVOT(l1,
SOC_ALPM_128_ADDR_LWR(l131)<<1) = ALPM_TCAM_PIVOT(l1,SOC_ALPM_128_ADDR_LWR(
l277)<<1);if(ALPM_TCAM_PIVOT(l1,SOC_ALPM_128_ADDR_LWR(l131)<<1)){
PIVOT_TCAM_INDEX(ALPM_TCAM_PIVOT(l1,SOC_ALPM_128_ADDR_LWR(l131)<<1)) = 
SOC_ALPM_128_ADDR_LWR(l131)<<1;}ALPM_TCAM_PIVOT(l1,SOC_ALPM_128_ADDR_LWR(l277
)<<1) = NULL;sal_memcpy(l278,soc_mem_entry_null(l1,L3_DEFIP_PAIR_128m),
soc_mem_entry_words(l1,L3_DEFIP_PAIR_128m)*4);l246(l1,l278,l264,0x4000,0);if(
(l124 = l163(l1,MEM_BLOCK_ANY,l264,l264,l278))<0){return l124;}if((l124 = 
l256(l1,l264,l278))<0){return l124;}if((l39[(l1)][(l115)].l35) == 0){l272 = (
l39[(l1)][(l115)].l34);assert(l272!= -1);l273 = (l39[(l1)][(l115)].next);(l39
[(l1)][(l272)].next) = l273;(l39[(l1)][(l272)].l36)+= (l39[(l1)][(l115)].l36)
;(l39[(l1)][(l115)].l36) = 0;if(l273!= -1){(l39[(l1)][(l273)].l34) = l272;}(
l39[(l1)][(l115)].next) = -1;(l39[(l1)][(l115)].l34) = -1;(l39[(l1)][(l115)].
l32) = -1;(l39[(l1)][(l115)].l33) = -1;}return(l124);}int
soc_alpm_128_lpm_vrf_get(int l17,void*lpm_entry,int*l24,int*l279){int l144;if
(((l91[(l17)]->l77)!= NULL)){l144 = soc_L3_DEFIP_PAIR_128m_field32_get(l17,
lpm_entry,VRF_ID_0_LWRf);*l279 = l144;if(soc_L3_DEFIP_PAIR_128m_field32_get(
l17,lpm_entry,VRF_ID_MASK0_LWRf)){*l24 = l144;}else if(!
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l17,L3_DEFIP_PAIR_128m)),(
lpm_entry),(l91[(l17)]->l79))){*l24 = SOC_L3_VRF_GLOBAL;*l279 = SOC_VRF_MAX(
l17)+1;}else{*l24 = SOC_L3_VRF_OVERRIDE;}}else{*l24 = SOC_L3_VRF_DEFAULT;}
return(SOC_E_NONE);}static int l280(int l1,void*entry,int*l14){int l115=0;int
l124;int l144;int l281;l124 = l122(l1,entry,&l115);if(l124<0){return l124;}
l115+= 0;SOC_IF_ERROR_RETURN(soc_alpm_128_lpm_vrf_get(l1,entry,&l144,&l124));
l281 = soc_alpm_mode_get(l1);switch(l144){case SOC_L3_VRF_GLOBAL:if(l281 == 
SOC_ALPM_MODE_PARALLEL){*l14 = l115+((3*(128+2+1))/3);}else{*l14 = l115;}
break;case SOC_L3_VRF_OVERRIDE:*l14 = l115+2*((3*(128+2+1))/3);break;default:
if(l281 == SOC_ALPM_MODE_PARALLEL){*l14 = l115;}else{*l14 = l115+((3*(128+2+1
))/3);}break;}return(SOC_E_NONE);}static int l11(int l1,void*l5,void*l12,int*
l13,int*l14,int*l7){int l124;int l116;int l115 = 0;*l7 = L3_DEFIP_MODE_128;
l280(l1,l5,&l115);*l14 = l115;if(l250(l1,l5,l115,&l116) == SOC_E_NONE){*l13 = 
l116;if((l124 = READ_L3_DEFIP_PAIR_128m(l1,MEM_BLOCK_ANY,(*l7)?*l13:(*l13>>1)
,l12))<0){return l124;}return(SOC_E_NONE);}else{return(SOC_E_NOT_FOUND);}}int
soc_alpm_128_lpm_init(int l1){int l255;int l123;int l282;int l283;if(!
soc_feature(l1,soc_feature_lpm_tcam)){return(SOC_E_UNAVAIL);}l255 = (3*(128+2
+1));l283 = sizeof(l37)*(l255);if((l39[(l1)]!= NULL)){SOC_IF_ERROR_RETURN(
soc_alpm_128_deinit(l1));}l91[l1] = sal_alloc(sizeof(l89),
"lpm_128_field_state");if(NULL == l91[l1]){return(SOC_E_MEMORY);}(l91[l1])->
l41 = soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,CLASS_IDf);(l91[l1])->l42 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,DST_DISCARDf);(l91[l1])->l43 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,ECMPf);(l91[l1])->l44 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,ECMP_COUNTf);(l91[l1])->l45 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,ECMP_PTRf);(l91[l1])->l46 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,GLOBAL_ROUTEf);(l91[l1])->l47 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,HITf);(l91[l1])->l50 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,IP_ADDR0_LWRf);(l91[l1])->l48 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,IP_ADDR0_UPRf);(l91[l1])->l51 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,IP_ADDR1_LWRf);(l91[l1])->l49 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,IP_ADDR1_UPRf);(l91[l1])->l54 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,IP_ADDR_MASK0_LWRf);(l91[l1])->
l52 = soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,IP_ADDR_MASK0_UPRf);(l91[l1
])->l55 = soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,IP_ADDR_MASK1_LWRf);(
l91[l1])->l53 = soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,
IP_ADDR_MASK1_UPRf);(l91[l1])->l58 = soc_mem_fieldinfo_get(l1,
L3_DEFIP_PAIR_128m,MODE0_LWRf);(l91[l1])->l56 = soc_mem_fieldinfo_get(l1,
L3_DEFIP_PAIR_128m,MODE0_UPRf);(l91[l1])->l59 = soc_mem_fieldinfo_get(l1,
L3_DEFIP_PAIR_128m,MODE1_LWRf);(l91[l1])->l57 = soc_mem_fieldinfo_get(l1,
L3_DEFIP_PAIR_128m,MODE1_UPRf);(l91[l1])->l62 = soc_mem_fieldinfo_get(l1,
L3_DEFIP_PAIR_128m,MODE_MASK0_LWRf);(l91[l1])->l60 = soc_mem_fieldinfo_get(l1
,L3_DEFIP_PAIR_128m,MODE_MASK0_UPRf);(l91[l1])->l63 = soc_mem_fieldinfo_get(
l1,L3_DEFIP_PAIR_128m,MODE_MASK1_LWRf);(l91[l1])->l61 = soc_mem_fieldinfo_get
(l1,L3_DEFIP_PAIR_128m,MODE_MASK1_UPRf);(l91[l1])->l64 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,NEXT_HOP_INDEXf);(l91[l1])->l65 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,PRIf);(l91[l1])->l66 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,RPEf);(l91[l1])->l69 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,VALID0_LWRf);(l91[l1])->l67 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,VALID0_UPRf);(l91[l1])->l70 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,VALID1_LWRf);(l91[l1])->l68 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,VALID1_UPRf);(l91[l1])->l73 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,VRF_ID_0_LWRf);(l91[l1])->l71 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,VRF_ID_0_UPRf);(l91[l1])->l74 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,VRF_ID_1_LWRf);(l91[l1])->l72 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,VRF_ID_1_UPRf);(l91[l1])->l77 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,VRF_ID_MASK0_LWRf);(l91[l1])->l75
= soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,VRF_ID_MASK0_UPRf);(l91[l1])->
l78 = soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,VRF_ID_MASK1_LWRf);(l91[l1]
)->l76 = soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,VRF_ID_MASK1_UPRf);(l91[
l1])->l79 = soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,GLOBAL_HIGHf);(l91[l1
])->l80 = soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,ALG_HIT_IDXf);(l91[l1])
->l81 = soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,ALG_BKT_PTRf);(l91[l1])->
l82 = soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,DEFAULT_MISSf);(l91[l1])->
l83 = soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,FLEX_CTR_BASE_COUNTER_IDXf)
;(l91[l1])->l84 = soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,
FLEX_CTR_POOL_NUMBERf);(l91[l1])->l85 = soc_mem_fieldinfo_get(l1,
L3_DEFIP_PAIR_128m,ENTRY_TYPE_MASK0_LWRf);(l91[l1])->l86 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,ENTRY_TYPE_MASK1_LWRf);(l91[l1])
->l87 = soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,ENTRY_TYPE_MASK0_UPRf);(
l91[l1])->l88 = soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,
ENTRY_TYPE_MASK1_UPRf);(l39[(l1)]) = sal_alloc(l283,"LPM 128 prefix info");if
(NULL == (l39[(l1)])){sal_free(l91[l1]);l91[l1] = NULL;return(SOC_E_MEMORY);}
SOC_ALPM_LPM_LOCK(l1);sal_memset((l39[(l1)]),0,l283);for(l123 = 0;l123<l255;
l123++){(l39[(l1)][(l123)].l32) = -1;(l39[(l1)][(l123)].l33) = -1;(l39[(l1)][
(l123)].l34) = -1;(l39[(l1)][(l123)].next) = -1;(l39[(l1)][(l123)].l35) = 0;(
l39[(l1)][(l123)].l36) = 0;}l282 = soc_mem_index_count(l1,L3_DEFIP_PAIR_128m)
;if(SOC_URPF_STATUS_GET(l1)){l282>>= 1;}if(soc_alpm_mode_get(l1) == 
SOC_ALPM_MODE_PARALLEL){(l39[(l1)][(((3*(128+2+1))-1))].l33) = (l282>>1)-1;(
l39[(l1)][(((((3*(128+2+1))/3)-1)))].l36) = l282>>1;(l39[(l1)][((((3*(128+2+1
))-1)))].l36) = (l282-(l39[(l1)][(((((3*(128+2+1))/3)-1)))].l36));}else{(l39[
(l1)][((((3*(128+2+1))-1)))].l36) = l282;}if((l102[(l1)])!= NULL){if(l110((
l102[(l1)]))<0){SOC_ALPM_LPM_UNLOCK(l1);return SOC_E_INTERNAL;}(l102[(l1)]) = 
NULL;}if(l108(l1,l282*2,l282,&(l102[(l1)]))<0){SOC_ALPM_LPM_UNLOCK(l1);return
SOC_E_MEMORY;}SOC_ALPM_LPM_UNLOCK(l1);return(SOC_E_NONE);}int
soc_alpm_128_lpm_deinit(int l1){if(!soc_feature(l1,soc_feature_lpm_tcam)){
return(SOC_E_UNAVAIL);}SOC_ALPM_LPM_LOCK(l1);if((l102[(l1)])!= NULL){l110((
l102[(l1)]));(l102[(l1)]) = NULL;}if((l39[(l1)]!= NULL)){sal_free(l91[l1]);
l91[l1] = NULL;sal_free((l39[(l1)]));(l39[(l1)]) = NULL;}SOC_ALPM_LPM_UNLOCK(
l1);return(SOC_E_NONE);}static int l2(int l1,void*l3,int*l284){int l115;int
index;int l7;uint32 l12[SOC_MAX_MEM_FIELD_WORDS];int l124 = SOC_E_NONE;int
l285 = 0;sal_memcpy(l12,soc_mem_entry_null(l1,L3_DEFIP_PAIR_128m),
soc_mem_entry_words(l1,L3_DEFIP_PAIR_128m)*4);SOC_ALPM_LPM_LOCK(l1);l124 = 
l11(l1,l3,l12,&index,&l115,&l7);if(l124 == SOC_E_NOT_FOUND){l124 = l270(l1,
l115,l7,l12,&index);if(l124<0){SOC_ALPM_LPM_UNLOCK(l1);return(l124);}}else{
l285 = 1;}*l284 = index;if(l124 == SOC_E_NONE){soc_alpm_128_lpm_state_dump(l1
);LOG_INFO(BSL_LS_SOC_ALPM,(BSL_META_U(l1,
"\nsoc_alpm_128_lpm_insert: %d %d\n"),index,l115));if(!l285){l246(l1,l3,index
,0x4000,0);}l124 = l163(l1,MEM_BLOCK_ANY,index,index,l3);if(l124>= 0){l124 = 
l256(l1,index,l3);}}SOC_ALPM_LPM_UNLOCK(l1);return(l124);}static int l4(int l1
,void*l5){int l115;int index;int l7;uint32 l12[SOC_MAX_MEM_FIELD_WORDS];int
l124 = SOC_E_NONE;SOC_ALPM_LPM_LOCK(l1);l124 = l11(l1,l5,l12,&index,&l115,&l7
);if(l124 == SOC_E_NONE){LOG_INFO(BSL_LS_SOC_ALPM,(BSL_META_U(l1,
"\nsoc_alpm_lpm_delete: %d %d\n"),index,l115));l249(l1,l5,index);l124 = l276(
l1,l115,l7,l12,index);}soc_alpm_128_lpm_state_dump(l1);SOC_ALPM_LPM_UNLOCK(l1
);return(l124);}static int l15(int l1,void*l5,void*l12,int*l13){int l115;int
l124;int l7;SOC_ALPM_LPM_LOCK(l1);l124 = l11(l1,l5,l12,l13,&l115,&l7);
SOC_ALPM_LPM_UNLOCK(l1);return(l124);}static int l6(int l17,void*l5,int l7,
int l8,int l9,defip_aux_scratch_entry_t*l10){uint32 l126;uint32 l286[4] = {0,
0,0,0};int l115 = 0;int l124 = SOC_E_NONE;l126 = soc_mem_field32_get(l17,
L3_DEFIP_PAIR_128m,l5,VALID0_LWRf);soc_mem_field32_set(l17,
L3_DEFIP_AUX_SCRATCHm,l10,VALIDf,l126);l126 = soc_mem_field32_get(l17,
L3_DEFIP_PAIR_128m,l5,MODE0_LWRf);soc_mem_field32_set(l17,
L3_DEFIP_AUX_SCRATCHm,l10,MODEf,l126);l126 = soc_mem_field32_get(l17,
L3_DEFIP_PAIR_128m,l5,ENTRY_TYPE0_LWRf);soc_mem_field32_set(l17,
L3_DEFIP_AUX_SCRATCHm,l10,ENTRY_TYPEf,0);l126 = soc_mem_field32_get(l17,
L3_DEFIP_PAIR_128m,l5,GLOBAL_ROUTEf);soc_mem_field32_set(l17,
L3_DEFIP_AUX_SCRATCHm,l10,GLOBAL_ROUTEf,l126);l126 = soc_mem_field32_get(l17,
L3_DEFIP_PAIR_128m,l5,ECMPf);soc_mem_field32_set(l17,L3_DEFIP_AUX_SCRATCHm,
l10,ECMPf,l126);l126 = soc_mem_field32_get(l17,L3_DEFIP_PAIR_128m,l5,
ECMP_PTRf);soc_mem_field32_set(l17,L3_DEFIP_AUX_SCRATCHm,l10,ECMP_PTRf,l126);
l126 = soc_mem_field32_get(l17,L3_DEFIP_PAIR_128m,l5,NEXT_HOP_INDEXf);
soc_mem_field32_set(l17,L3_DEFIP_AUX_SCRATCHm,l10,NEXT_HOP_INDEXf,l126);l126 = 
soc_mem_field32_get(l17,L3_DEFIP_PAIR_128m,l5,PRIf);soc_mem_field32_set(l17,
L3_DEFIP_AUX_SCRATCHm,l10,PRIf,l126);l126 = soc_mem_field32_get(l17,
L3_DEFIP_PAIR_128m,l5,RPEf);soc_mem_field32_set(l17,L3_DEFIP_AUX_SCRATCHm,l10
,RPEf,l126);l126 =soc_mem_field32_get(l17,L3_DEFIP_PAIR_128m,l5,VRF_ID_0_LWRf
);soc_mem_field32_set(l17,L3_DEFIP_AUX_SCRATCHm,l10,VRFf,l126);
soc_mem_field32_set(l17,L3_DEFIP_AUX_SCRATCHm,l10,DB_TYPEf,l8);l126 = 
soc_mem_field32_get(l17,L3_DEFIP_PAIR_128m,l5,DST_DISCARDf);
soc_mem_field32_set(l17,L3_DEFIP_AUX_SCRATCHm,l10,DST_DISCARDf,l126);l126 = 
soc_mem_field32_get(l17,L3_DEFIP_PAIR_128m,l5,CLASS_IDf);soc_mem_field32_set(
l17,L3_DEFIP_AUX_SCRATCHm,l10,CLASS_IDf,l126);l286[0] = soc_mem_field32_get(
l17,L3_DEFIP_PAIR_128m,l5,IP_ADDR0_LWRf);l286[1] = soc_mem_field32_get(l17,
L3_DEFIP_PAIR_128m,l5,IP_ADDR1_LWRf);l286[2] = soc_mem_field32_get(l17,
L3_DEFIP_PAIR_128m,l5,IP_ADDR0_UPRf);l286[3] = soc_mem_field32_get(l17,
L3_DEFIP_PAIR_128m,l5,IP_ADDR1_UPRf);soc_mem_field_set(l17,
L3_DEFIP_AUX_SCRATCHm,(uint32*)l10,IP_ADDRf,(uint32*)l286);l124 = l122(l17,l5
,&l115);if(SOC_FAILURE(l124)){return l124;}soc_mem_field32_set(l17,
L3_DEFIP_AUX_SCRATCHm,l10,IP_LENGTHf,l115);soc_mem_field32_set(l17,
L3_DEFIP_AUX_SCRATCHm,l10,REPLACE_LENf,l9);return(SOC_E_NONE);}static int l16
(int l17,void*lpm_entry,void*l18,void*l19,soc_mem_t l20,uint32 l21,uint32*
l287){uint32 l126;uint32 l286[4];int l115 = 0;int l124 = SOC_E_NONE;uint32 l22
= 0;sal_memset(l18,0,soc_mem_entry_words(l17,l20)*4);l126 = 
soc_mem_field32_get(l17,L3_DEFIP_PAIR_128m,lpm_entry,HITf);
soc_mem_field32_set(l17,l20,l18,HITf,l126);l126 = soc_mem_field32_get(l17,
L3_DEFIP_PAIR_128m,lpm_entry,VALID0_LWRf);soc_mem_field32_set(l17,l20,l18,
VALIDf,l126);l126 = soc_mem_field32_get(l17,L3_DEFIP_PAIR_128m,lpm_entry,
ECMPf);soc_mem_field32_set(l17,l20,l18,ECMPf,l126);l126 = soc_mem_field32_get
(l17,L3_DEFIP_PAIR_128m,lpm_entry,ECMP_PTRf);soc_mem_field32_set(l17,l20,l18,
ECMP_PTRf,l126);l126 = soc_mem_field32_get(l17,L3_DEFIP_PAIR_128m,lpm_entry,
NEXT_HOP_INDEXf);soc_mem_field32_set(l17,l20,l18,NEXT_HOP_INDEXf,l126);l126 = 
soc_mem_field32_get(l17,L3_DEFIP_PAIR_128m,lpm_entry,PRIf);
soc_mem_field32_set(l17,l20,l18,PRIf,l126);l126 = soc_mem_field32_get(l17,
L3_DEFIP_PAIR_128m,lpm_entry,RPEf);soc_mem_field32_set(l17,l20,l18,RPEf,l126)
;l126 = soc_mem_field32_get(l17,L3_DEFIP_PAIR_128m,lpm_entry,DST_DISCARDf);
soc_mem_field32_set(l17,l20,l18,DST_DISCARDf,l126);l126 = soc_mem_field32_get
(l17,L3_DEFIP_PAIR_128m,lpm_entry,SRC_DISCARDf);soc_mem_field32_set(l17,l20,
l18,SRC_DISCARDf,l126);l126 = soc_mem_field32_get(l17,L3_DEFIP_PAIR_128m,
lpm_entry,CLASS_IDf);soc_mem_field32_set(l17,l20,l18,CLASS_IDf,l126);l286[0] = 
soc_mem_field32_get(l17,L3_DEFIP_PAIR_128m,lpm_entry,IP_ADDR0_LWRf);l286[1] = 
soc_mem_field32_get(l17,L3_DEFIP_PAIR_128m,lpm_entry,IP_ADDR1_LWRf);l286[2] = 
soc_mem_field32_get(l17,L3_DEFIP_PAIR_128m,lpm_entry,IP_ADDR0_UPRf);l286[3] = 
soc_mem_field32_get(l17,L3_DEFIP_PAIR_128m,lpm_entry,IP_ADDR1_UPRf);
soc_mem_field_set(l17,l20,(uint32*)l18,KEYf,(uint32*)l286);l124 = l122(l17,
lpm_entry,&l115);if(SOC_FAILURE(l124)){return l124;}if((l115 == 0)&&(l286[0]
== 0)&&(l286[1] == 0)&&(l286[2] == 0)&&(l286[3] == 0)){l22 = 1;}if(l287!= 
NULL){*l287 = l22;}soc_mem_field32_set(l17,l20,l18,LENGTHf,l115);if(l19 == 
NULL){return(SOC_E_NONE);}if(SOC_URPF_STATUS_GET(l17)){sal_memset(l19,0,
soc_mem_entry_words(l17,l20)*4);sal_memcpy(l19,l18,soc_mem_entry_words(l17,
l20)*4);soc_mem_field32_set(l17,l20,l19,DST_DISCARDf,0);soc_mem_field32_set(
l17,l20,l19,RPEf,0);soc_mem_field32_set(l17,l20,l19,SRC_DISCARDf,l21&
SOC_ALPM_RPF_SRC_DISCARD);soc_mem_field32_set(l17,l20,l19,DEFAULTROUTEf,l22);
}return(SOC_E_NONE);}static int l23(int l17,void*l18,soc_mem_t l20,int l7,int
l24,int l25,int index,void*lpm_entry){uint32 l126;uint32 l286[4];uint32 l144,
l288;sal_memset(lpm_entry,0,soc_mem_entry_words(l17,L3_DEFIP_PAIR_128m)*4);
l126 = soc_mem_field32_get(l17,l20,l18,HITf);soc_mem_field32_set(l17,
L3_DEFIP_PAIR_128m,lpm_entry,HITf,l126);l126 = soc_mem_field32_get(l17,l20,
l18,VALIDf);soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,VALID0_LWRf,
l126);soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,VALID1_LWRf,l126);
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,VALID0_UPRf,l126);
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,VALID1_UPRf,l126);l126 = 
soc_mem_field32_get(l17,l20,l18,ECMPf);soc_mem_field32_set(l17,
L3_DEFIP_PAIR_128m,lpm_entry,ECMPf,l126);l126 = soc_mem_field32_get(l17,l20,
l18,ECMP_PTRf);soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,ECMP_PTRf
,l126);l126 = soc_mem_field32_get(l17,l20,l18,NEXT_HOP_INDEXf);
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,NEXT_HOP_INDEXf,l126);
l126 = soc_mem_field32_get(l17,l20,l18,PRIf);soc_mem_field32_set(l17,
L3_DEFIP_PAIR_128m,lpm_entry,PRIf,l126);l126 = soc_mem_field32_get(l17,l20,
l18,RPEf);soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,RPEf,l126);
l126 = soc_mem_field32_get(l17,l20,l18,DST_DISCARDf);soc_mem_field32_set(l17,
L3_DEFIP_PAIR_128m,lpm_entry,DST_DISCARDf,l126);l126 = soc_mem_field32_get(
l17,l20,l18,SRC_DISCARDf);soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,
lpm_entry,SRC_DISCARDf,l126);l126 = soc_mem_field32_get(l17,l20,l18,CLASS_IDf
);soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,CLASS_IDf,l126);
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,ALG_BKT_PTRf,l25);
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,ALG_HIT_IDXf,index);
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,MODE_MASK0_LWRf,3);
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,MODE_MASK1_LWRf,3);
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,MODE_MASK0_UPRf,3);
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,MODE_MASK1_UPRf,3);
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,ENTRY_TYPE_MASK0_LWRf,1)
;soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,ENTRY_TYPE_MASK1_LWRf,1
);soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,ENTRY_TYPE_MASK0_UPRf,
1);soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,ENTRY_TYPE_MASK1_UPRf
,1);soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,MODE0_LWRf,3);
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,MODE1_LWRf,3);
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,MODE0_UPRf,3);
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,MODE1_UPRf,3);
soc_mem_field_get(l17,l20,l18,KEYf,l286);soc_mem_field32_set(l17,
L3_DEFIP_PAIR_128m,lpm_entry,IP_ADDR0_LWRf,l286[0]);soc_mem_field32_set(l17,
L3_DEFIP_PAIR_128m,lpm_entry,IP_ADDR1_LWRf,l286[1]);soc_mem_field32_set(l17,
L3_DEFIP_PAIR_128m,lpm_entry,IP_ADDR0_UPRf,l286[2]);soc_mem_field32_set(l17,
L3_DEFIP_PAIR_128m,lpm_entry,IP_ADDR1_UPRf,l286[3]);l286[0] = l286[1] = l286[
2] = l286[3] = 0;l126 = soc_mem_field32_get(l17,l20,l18,LENGTHf);l132(l17,
lpm_entry,l126);if(l24 == SOC_L3_VRF_OVERRIDE){soc_mem_field32_set(l17,
L3_DEFIP_PAIR_128m,lpm_entry,GLOBAL_HIGHf,1);soc_mem_field32_set(l17,
L3_DEFIP_PAIR_128m,lpm_entry,GLOBAL_ROUTEf,1);l144 = 0;l288 = 0;}else if(l24
== SOC_L3_VRF_GLOBAL){soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,
GLOBAL_ROUTEf,1);l144 = 0;l288 = 0;}else{l144 = l24;l288 = SOC_VRF_MAX(l17);}
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,VRF_ID_0_LWRf,l144);
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,VRF_ID_1_LWRf,l144);
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,VRF_ID_0_UPRf,l144);
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,VRF_ID_1_UPRf,l144);
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,VRF_ID_MASK0_LWRf,l288);
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,VRF_ID_MASK1_LWRf,l288);
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,VRF_ID_MASK0_UPRf,l288);
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,VRF_ID_MASK1_UPRf,l288);
return(SOC_E_NONE);}int soc_alpm_128_warmboot_pivot_add(int l17,int l7,void*
lpm_entry,int l289,int l290){int l124 = SOC_E_NONE;uint32 key[4] = {0,0,0,0};
alpm_pivot_t*l170 = NULL;alpm_bucket_handle_t*l179 = NULL;int l144 = 0,l24 = 
0;uint32 l291;trie_t*l213 = NULL;uint32 prefix[5] = {0};int l22 = 0;l124 = 
l134(l17,lpm_entry,prefix,&l291,&l22);SOC_IF_ERROR_RETURN(l124);
SOC_IF_ERROR_RETURN(soc_alpm_128_lpm_vrf_get(l17,lpm_entry,&l144,&l24));l289 = 
soc_alpm_physical_idx(l17,L3_DEFIP_PAIR_128m,l289,l7);l179 = sal_alloc(sizeof
(alpm_bucket_handle_t),"ALPM Bucket Handle");if(l179 == NULL){LOG_ERROR(
BSL_LS_SOC_ALPM,(BSL_META_U(l17,
"Unable to allocate memory for PIVOT trie node \n")));return SOC_E_NONE;}
sal_memset(l179,0,sizeof(*l179));l170 = sal_alloc(sizeof(alpm_pivot_t),
"Payload for Pivot");if(l170 == NULL){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(
l17,"Unable to allocate memory for PIVOT trie node \n")));sal_free(l179);
return SOC_E_MEMORY;}sal_memset(l170,0,sizeof(*l170));PIVOT_BUCKET_HANDLE(
l170) = l179;trie_init(_MAX_KEY_LEN_144_,&PIVOT_BUCKET_TRIE(l170));key[0] = 
soc_L3_DEFIP_PAIR_128m_field32_get(l17,lpm_entry,IP_ADDR0_LWRf);key[1] = 
soc_L3_DEFIP_PAIR_128m_field32_get(l17,lpm_entry,IP_ADDR1_LWRf);key[2] = 
soc_L3_DEFIP_PAIR_128m_field32_get(l17,lpm_entry,IP_ADDR0_UPRf);key[3] = 
soc_L3_DEFIP_PAIR_128m_field32_get(l17,lpm_entry,IP_ADDR1_UPRf);
PIVOT_BUCKET_INDEX(l170) = l290;PIVOT_TCAM_INDEX(l170) = 
SOC_ALPM_128_ADDR_LWR(l289)<<1;if(l144!= SOC_L3_VRF_OVERRIDE){l213 = 
VRF_PIVOT_TRIE_IPV6_128(l17,l24);if(l213 == NULL){trie_init(_MAX_KEY_LEN_144_
,&VRF_PIVOT_TRIE_IPV6_128(l17,l24));l213 = VRF_PIVOT_TRIE_IPV6_128(l17,l24);}
sal_memcpy(l170->key,prefix,sizeof(prefix));l170->len = l291;l124 = 
trie_insert(l213,l170->key,NULL,l170->len,(trie_node_t*)l170);if(SOC_FAILURE(
l124)){sal_free(l179);sal_free(l170);return l124;}}ALPM_TCAM_PIVOT(l17,
SOC_ALPM_128_ADDR_LWR(l289)<<1) = l170;PIVOT_BUCKET_VRF(l170) = l24;
PIVOT_BUCKET_IPV6(l170) = l7;PIVOT_BUCKET_ENT_CNT_UPDATE(l170);if(key[0] == 0
&&key[1] == 0&&key[2] == 0&&key[3] == 0){PIVOT_BUCKET_DEF(l170) = TRUE;}
VRF_PIVOT_REF_INC(l17,l24,l7);return l124;}static int l292(int l17,int l7,
void*lpm_entry,void*l18,soc_mem_t l20,int l289,int l290,int l293){int l294;
int l24;int l124 = SOC_E_NONE;int l22 = 0;uint32 prefix[5] = {0,0,0,0,0};
uint32 l129;void*l295 = NULL;trie_t*l296 = NULL;trie_t*l173 = NULL;
trie_node_t*l175 = NULL;payload_t*l297 = NULL;payload_t*l177 = NULL;
alpm_pivot_t*l140 = NULL;if((NULL == lpm_entry)||(NULL == l18)){return
SOC_E_PARAM;}SOC_IF_ERROR_RETURN(soc_alpm_128_lpm_vrf_get(l17,lpm_entry,&l294
,&l24));l20 = L3_DEFIP_ALPM_IPV6_128m;l295 = sal_alloc(sizeof(
defip_pair_128_entry_t),"Temp Defip Pair lpm_entry");if(NULL == l295){return
SOC_E_MEMORY;}SOC_IF_ERROR_RETURN(l23(l17,l18,l20,l7,l294,l290,l289,l295));
l124 = l134(l17,l295,prefix,&l129,&l22);if(SOC_FAILURE(l124)){LOG_ERROR(
BSL_LS_SOC_ALPM,(BSL_META_U(l17,"prefix create failed\n")));return l124;}
sal_free(l295);l289 = soc_alpm_physical_idx(l17,L3_DEFIP_PAIR_128m,l289,l7);
l140 = ALPM_TCAM_PIVOT(l17,SOC_ALPM_128_ADDR_LWR(l289)<<1);l296 = 
PIVOT_BUCKET_TRIE(l140);l297 = sal_alloc(sizeof(payload_t),"Payload for Key")
;if(NULL == l297){LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l17,
"Unable to allocate memory for trie node.\n")));return SOC_E_MEMORY;}l177 = 
sal_alloc(sizeof(payload_t),"Payload for pfx trie key");if(NULL == l177){
LOG_ERROR(BSL_LS_SOC_ALPM,(BSL_META_U(l17,
"Unable to allocate memory for pfx trie node\n")));sal_free(l297);return
SOC_E_MEMORY;}sal_memset(l297,0,sizeof(*l297));sal_memset(l177,0,sizeof(*l177
));l297->key[0] = prefix[0];l297->key[1] = prefix[1];l297->key[2] = prefix[2]
;l297->key[3] = prefix[3];l297->key[4] = prefix[4];l297->len = l129;l297->
index = l293;sal_memcpy(l177,l297,sizeof(*l297));l124 = trie_insert(l296,
prefix,NULL,l129,(trie_node_t*)l297);if(SOC_FAILURE(l124)){goto l298;}if(l7){
l173 = VRF_PREFIX_TRIE_IPV6_128(l17,l24);}if(!l22){l124 = trie_insert(l173,
prefix,NULL,l129,(trie_node_t*)l177);if(SOC_FAILURE(l124)){goto l193;}}return
l124;l193:(void)trie_delete(l296,prefix,l129,&l175);l297 = (payload_t*)l175;
l298:sal_free(l297);sal_free(l177);return l124;}static int l299(int l17,int
l30,int l24,int l214,int bkt_ptr){int l124 = SOC_E_NONE;uint32 l129;uint32 key
[5] = {0,0,0,0,0};trie_t*l300 = NULL;payload_t*l204 = NULL;
defip_pair_128_entry_t*lpm_entry = NULL;lpm_entry = sal_alloc(sizeof(
defip_pair_128_entry_t),"Default LPM entry");if(lpm_entry == NULL){LOG_ERROR(
BSL_LS_SOC_ALPM,(BSL_META_U(l17,"unable to allocate memory for LPM entry\n"))
);return SOC_E_MEMORY;}l26(l17,key,0,l24,l30,lpm_entry,0,1);if(l24 == 
SOC_VRF_MAX(l17)+1){soc_L3_DEFIP_PAIR_128m_field32_set(l17,lpm_entry,
GLOBAL_ROUTEf,1);}else{soc_L3_DEFIP_PAIR_128m_field32_set(l17,lpm_entry,
DEFAULT_MISSf,1);}soc_L3_DEFIP_PAIR_128m_field32_set(l17,lpm_entry,
ALG_BKT_PTRf,bkt_ptr);VRF_TRIE_DEFAULT_ROUTE_IPV6_128(l17,l24) = lpm_entry;
trie_init(_MAX_KEY_LEN_144_,&VRF_PREFIX_TRIE_IPV6_128(l17,l24));l300 = 
VRF_PREFIX_TRIE_IPV6_128(l17,l24);l204 = sal_alloc(sizeof(payload_t),
"Payload for pfx trie key");if(l204 == NULL){LOG_ERROR(BSL_LS_SOC_ALPM,(
BSL_META_U(l17,"Unable to allocate memory for pfx trie node \n")));return
SOC_E_MEMORY;}sal_memset(l204,0,sizeof(*l204));l129 = 0;l204->key[0] = key[0]
;l204->key[1] = key[1];l204->len = l129;l124 = trie_insert(l300,key,NULL,l129
,&(l204->node));if(SOC_FAILURE(l124)){sal_free(l204);return l124;}
VRF_TRIE_INIT_DONE(l17,l24,l30,1);return l124;}int
soc_alpm_128_warmboot_prefix_insert(int l17,int l7,void*lpm_entry,void*l18,
int l289,int l290,int l293){int l294;int l24;int l124 = SOC_E_NONE;soc_mem_t
l20;l20 = L3_DEFIP_ALPM_IPV6_128m;SOC_IF_ERROR_RETURN(
soc_alpm_128_lpm_vrf_get(l17,lpm_entry,&l294,&l24));if(l294 == 
SOC_L3_VRF_OVERRIDE){return(l124);}if(!VRF_TRIE_INIT_COMPLETED(l17,l24,l7)){
LOG_VERBOSE(BSL_LS_SOC_ALPM,(BSL_META_U(l17,"VRF %d is not initialized\n"),
l24));l124 = l299(l17,l7,l24,l289,l290);if(SOC_FAILURE(l124)){LOG_ERROR(
BSL_LS_SOC_ALPM,(BSL_META_U(l17,"VRF %d/%d trie init \n""failed\n"),l24,l7));
return l124;}LOG_VERBOSE(BSL_LS_SOC_ALPM,(BSL_META_U(l17,
"VRF %d/%d trie init ""completed\n"),l24,l7));}l124 = l292(l17,l7,lpm_entry,
l18,l20,l289,l290,l293);if(l124!= SOC_E_NONE){LOG_WARN(BSL_LS_SOC_ALPM,(
BSL_META_U(l17,"unit %d : Route Insertion Failed :%s\n"),l17,soc_errmsg(l124)
));return(l124);}VRF_TRIE_ROUTES_INC(l17,l24,l7);return(l124);}int
soc_alpm_128_warmboot_bucket_bitmap_set(int l1,int l30,int l216){int l301 = 1
;if(l30){if(!soc_alpm_mode_get(l1)&&!SOC_URPF_STATUS_GET(l1)){l301 = 2;}}if(
SOC_ALPM_BUCKET_BMAP(l1) == NULL){return SOC_E_INTERNAL;}SHR_BITSET_RANGE(
SOC_ALPM_BUCKET_BMAP(l1),l216,l301);return SOC_E_NONE;}int
soc_alpm_128_warmboot_lpm_reinit_done(int l17){int l214;int l302 = ((3*(128+2
+1))-1);int l282 = soc_mem_index_count(l17,L3_DEFIP_PAIR_128m);if(
SOC_URPF_STATUS_GET(l17)){l282>>= 1;}if(!soc_alpm_mode_get(l17)){(l39[(l17)][
(((3*(128+2+1))-1))].l34) = -1;for(l214 = ((3*(128+2+1))-1);l214>-1;l214--){
if(-1 == (l39[(l17)][(l214)].l32)){continue;}(l39[(l17)][(l214)].l34) = l302;
(l39[(l17)][(l302)].next) = l214;(l39[(l17)][(l302)].l36) = (l39[(l17)][(l214
)].l32)-(l39[(l17)][(l302)].l33)-1;l302 = l214;}(l39[(l17)][(l302)].next) = -
1;(l39[(l17)][(l302)].l36) = l282-(l39[(l17)][(l302)].l33)-1;}else{(l39[(l17)
][(((3*(128+2+1))-1))].l34) = -1;for(l214 = ((3*(128+2+1))-1);l214>(((3*(128+
2+1))-1)/3);l214--){if(-1 == (l39[(l17)][(l214)].l32)){continue;}(l39[(l17)][
(l214)].l34) = l302;(l39[(l17)][(l302)].next) = l214;(l39[(l17)][(l302)].l36)
= (l39[(l17)][(l214)].l32)-(l39[(l17)][(l302)].l33)-1;l302 = l214;}(l39[(l17)
][(l302)].next) = -1;(l39[(l17)][(l302)].l36) = l282-(l39[(l17)][(l302)].l33)
-1;l302 = (((3*(128+2+1))-1)/3);(l39[(l17)][((((3*(128+2+1))-1)/3))].l34) = -
1;for(l214 = ((((3*(128+2+1))-1)/3)-1);l214>-1;l214--){if(-1 == (l39[(l17)][(
l214)].l32)){continue;}(l39[(l17)][(l214)].l34) = l302;(l39[(l17)][(l302)].
next) = l214;(l39[(l17)][(l302)].l36) = (l39[(l17)][(l214)].l32)-(l39[(l17)][
(l302)].l33)-1;l302 = l214;}(l39[(l17)][(l302)].next) = -1;(l39[(l17)][(l302)
].l36) = (l282>>1)-(l39[(l17)][(l302)].l33)-1;}return(SOC_E_NONE);}int
soc_alpm_128_warmboot_lpm_reinit(int l17,int l7,int l214,void*lpm_entry){int
l14;l246(l17,lpm_entry,l214,0x4000,0);SOC_IF_ERROR_RETURN(l280(l17,lpm_entry,
&l14));if((l39[(l17)][(l14)].l35) == 0){(l39[(l17)][(l14)].l32) = l214;(l39[(
l17)][(l14)].l33) = l214;}else{(l39[(l17)][(l14)].l33) = l214;}(l39[(l17)][(
l14)].l35)++;return(SOC_E_NONE);}
#endif
#endif /* ALPM_ENABLE */
