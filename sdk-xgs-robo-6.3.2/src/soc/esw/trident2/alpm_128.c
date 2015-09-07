/*
 * $Id: alpm_128.c 1.4.10.3 Broadcom SDK $
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
soc_field_info_t*l83;soc_field_info_t*l84;}l85,*l86;static l86 l87[
SOC_MAX_NUM_DEVICES];typedef struct l88{int l17;int l89;int l90;uint16*l91;
uint16*l92;}l93;typedef uint32 l94[9];typedef int(*l95)(l94 l96,l94 l97);
static l93*l98[SOC_MAX_NUM_DEVICES];static void l99(int l1,void*l12,int index
,l94 l100);static uint16 l101(uint8*l102,int l103);static int l104(int l17,
int l89,int l90,l93**l105);static int l106(l93*l107);static int l108(l93*l109
,l95 l110,l94 entry,int l111,uint16*l112);static int l113(l93*l109,l95 l110,
l94 entry,int l111,uint16 l114,uint16 l115);static int l116(l93*l109,l95 l110
,l94 entry,int l111,uint16 l117);static int l118(int l1,const void*entry,int*
l111){int l119,l120;int l121[4] = {IP_ADDR_MASK0_LWRf,IP_ADDR_MASK1_LWRf,
IP_ADDR_MASK0_UPRf,IP_ADDR_MASK1_UPRf};uint32 l122;l122 = soc_mem_field32_get
(l1,L3_DEFIP_PAIR_128m,entry,l121[0]);if((l120 = _ipmask2pfx(l122,l111))<0){
return(l120);}for(l119 = 1;l119<4;l119++){l122 = soc_mem_field32_get(l1,
L3_DEFIP_PAIR_128m,entry,l121[l119]);if(*l111){if(l122!= 0xffffffff){return(
SOC_E_PARAM);}*l111+= 32;}else{if((l120 = _ipmask2pfx(l122,l111))<0){return(
l120);}}}return SOC_E_NONE;}static void l123(uint32*l124,int l125,int l30){
uint32 l126,l127,l32,prefix[5];int l119;sal_memcpy(prefix,l124,sizeof(uint32)
*BITS2WORDS(_MAX_KEY_LEN_144_));sal_memset(l124,0,sizeof(uint32)*BITS2WORDS(
_MAX_KEY_LEN_144_));l126 = 128-l125;l32 = (l126+31)/32;if((l126%32) == 0){l32
++;}l126 = l126%32;for(l119 = l32;l119<= 4;l119++){prefix[l119]<<= l126;l127 = 
prefix[l119+1]&~(0xffffffff>>l126);l127>>= (32-l126);if(l119<4){prefix[l119]
|= l127;}}for(l119 = l32;l119<= 4;l119++){l124[3-(l119-l32)] = prefix[l119];}
}static void l128(int l17,void*lpm_entry,int l14){int l119;soc_field_t l129[4
] = {IP_ADDR_MASK0_LWRf,IP_ADDR_MASK1_LWRf,IP_ADDR_MASK0_UPRf,
IP_ADDR_MASK1_UPRf};for(l119 = 0;l119<4;l119++){if(l14<= 32)break;
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,l129[3-l119],0xffffffff)
;l14-= 32;}soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,l129[3-l119],
~(0xffffffff>>l14));}static int l130(int l1,void*entry,uint32*l124,uint32*l14
,int*l22){int l119;int l111 = 0,l32;int l120 = SOC_E_NONE;uint32 l126,l127;
uint32 prefix[5];sal_memset(l124,0,sizeof(uint32)*BITS2WORDS(
_MAX_KEY_LEN_144_));sal_memset(prefix,0,sizeof(uint32)*BITS2WORDS(
_MAX_KEY_LEN_144_));prefix[0] = soc_mem_field32_get(l1,L3_DEFIP_PAIR_128m,
entry,IP_ADDR0_LWRf);prefix[1] = soc_mem_field32_get(l1,L3_DEFIP_PAIR_128m,
entry,IP_ADDR1_LWRf);prefix[2] = soc_mem_field32_get(l1,L3_DEFIP_PAIR_128m,
entry,IP_ADDR0_UPRf);prefix[3] = soc_mem_field32_get(l1,L3_DEFIP_PAIR_128m,
entry,IP_ADDR1_UPRf);if(l22!= NULL){*l22 = (prefix[0] == 0)&&(prefix[1] == 0)
&&(prefix[2] == 0)&&(prefix[3] == 0)&&(l111 == 0);}l120 = l118(l1,entry,&l111
);if(SOC_FAILURE(l120)){return l120;}l126 = 128-l111;l32 = l126/32;l126 = 
l126%32;for(l119 = l32;l119<4;l119++){prefix[l119]>>= l126;l127 = prefix[l119
+1]&((1<<l126)-1);l127<<= (32-l126);prefix[l119]|= l127;}for(l119 = l32;l119<
4;l119++){l124[4-(l119-l32)] = prefix[l119];}*l14 = l111;return SOC_E_NONE;}
static int l131(int l1,void*l5,soc_mem_t l20,void*l132,int*l133,int*
bucket_index,int*l13){defip_aux_scratch_entry_t l10;uint32 l12[
SOC_MAX_MEM_FIELD_WORDS];int l134,l24,l30;int l112;uint32 l8,l135;int l120 = 
SOC_E_NONE;int l136 = 0;l30 = L3_DEFIP_MODE_128;SOC_IF_ERROR_RETURN(
soc_alpm_128_lpm_vrf_get(l1,l5,&l134,&l24));if(l134 == 0){if(
soc_alpm_mode_get(l1)){return SOC_E_PARAM;}}if(l24 == SOC_VRF_MAX(l1)+1){l8 = 
0;SOC_ALPM_GET_GLOBAL_BANK_DISABLE(l1,l135);}else{l8 = 2;
SOC_ALPM_GET_VRF_BANK_DISABLE(l1,l135);}if(l134!= SOC_L3_VRF_OVERRIDE){
sal_memset(&l10,0,sizeof(defip_aux_scratch_entry_t));SOC_IF_ERROR_RETURN(l6(
l1,l5,l30,l8,0,&l10));SOC_IF_ERROR_RETURN(_soc_alpm_aux_op(l1,PREFIX_LOOKUP,&
l10,&l136,l133,bucket_index));if(l136){l16(l1,l5,l12,0,l20,0,0);l120 = 
_soc_alpm_find_in_bkt(l1,l20,*bucket_index,l135,l12,l132,&l112,l30);if(
SOC_SUCCESS(l120)){*l13 = l112;}}else{l120 = SOC_E_NOT_FOUND;}}return l120;}
static int l137(int l1,void*l5,void*l132,void*l138,soc_mem_t l20,int*l13){
defip_aux_scratch_entry_t l10;uint32 l12[SOC_MAX_MEM_FIELD_WORDS];int l134,
l30,l24;int l112,bucket_index;uint32 l8,l135;int l120 = SOC_E_NONE;int l136 = 
0;int l133;l30 = L3_DEFIP_MODE_128;SOC_IF_ERROR_RETURN(
soc_alpm_128_lpm_vrf_get(l1,l5,&l134,&l24));if(l24 == SOC_VRF_MAX(l1)+1){l8 = 
0;SOC_ALPM_GET_GLOBAL_BANK_DISABLE(l1,l135);}else{l8 = 2;
SOC_ALPM_GET_VRF_BANK_DISABLE(l1,l135);}if(!soc_alpm_mode_get(l1)){l8 = 2;}if
(l134!= SOC_L3_VRF_OVERRIDE){sal_memset(&l10,0,sizeof(
defip_aux_scratch_entry_t));SOC_IF_ERROR_RETURN(l6(l1,l5,l30,l8,0,&l10));
SOC_IF_ERROR_RETURN(_soc_alpm_aux_op(l1,PREFIX_LOOKUP,&l10,&l136,&l133,&
bucket_index));if(l136){l120 = _soc_alpm_find_in_bkt(l1,l20,bucket_index,l135
,l132,l12,&l112,l30);if(SOC_SUCCESS(l120)){*l13 = l112;SOC_IF_ERROR_RETURN(
soc_mem_write(l1,l20,MEM_BLOCK_ANY,l112,l132));if(l120!= SOC_E_NONE){return
SOC_E_FAIL;}if(SOC_URPF_STATUS_GET(l1)){SOC_IF_ERROR_RETURN(soc_mem_write(l1,
l20,MEM_BLOCK_ANY,_soc_alpm_rpf_entry(l1,l112),l138));if(l120!= SOC_E_NONE){
return SOC_E_FAIL;}}}}else{return(SOC_E_NOT_FOUND);}}return l120;}static int
l139(int l1,int l140,void*entry,defip_aux_table_entry_t*l141,int l142){uint32
l127,l8,l143 = 0;soc_mem_t l20 = L3_DEFIP_PAIR_128m;soc_mem_t l144 = 
L3_DEFIP_AUX_TABLEm;int l120 = SOC_E_NONE,l111,l24;void*l145,*l146;l145 = (
void*)l141;l146 = (void*)(l141+1);SOC_IF_ERROR_RETURN(soc_mem_read(l1,l144,
MEM_BLOCK_ANY,SOC_ALPM_128_ADDR_LWR(l140),l141));SOC_IF_ERROR_RETURN(
soc_mem_read(l1,l144,MEM_BLOCK_ANY,SOC_ALPM_128_ADDR_UPR(l140),l141+1));l127 = 
soc_mem_field32_get(l1,l20,entry,VRF_ID_0_LWRf);soc_mem_field32_set(l1,l144,
l145,VRF0f,l127);l127 = soc_mem_field32_get(l1,l20,entry,VRF_ID_1_LWRf);
soc_mem_field32_set(l1,l144,l145,VRF1f,l127);l127 = soc_mem_field32_get(l1,
l20,entry,VRF_ID_0_UPRf);soc_mem_field32_set(l1,l144,l146,VRF0f,l127);l127 = 
soc_mem_field32_get(l1,l20,entry,VRF_ID_1_UPRf);soc_mem_field32_set(l1,l144,
l146,VRF1f,l127);l127 = soc_mem_field32_get(l1,l20,entry,MODE0_LWRf);
soc_mem_field32_set(l1,l144,l145,MODE0f,l127);l127 = soc_mem_field32_get(l1,
l20,entry,MODE1_LWRf);soc_mem_field32_set(l1,l144,l145,MODE1f,l127);l127 = 
soc_mem_field32_get(l1,l20,entry,MODE0_UPRf);soc_mem_field32_set(l1,l144,l146
,MODE0f,l127);l127 = soc_mem_field32_get(l1,l20,entry,MODE1_UPRf);
soc_mem_field32_set(l1,l144,l146,MODE1f,l127);l127 = soc_mem_field32_get(l1,
l20,entry,VALID0_LWRf);soc_mem_field32_set(l1,l144,l145,VALID0f,l127);l127 = 
soc_mem_field32_get(l1,l20,entry,VALID1_LWRf);soc_mem_field32_set(l1,l144,
l145,VALID1f,l127);l127 = soc_mem_field32_get(l1,l20,entry,VALID0_UPRf);
soc_mem_field32_set(l1,l144,l146,VALID0f,l127);l127 = soc_mem_field32_get(l1,
l20,entry,VALID1_UPRf);soc_mem_field32_set(l1,l144,l146,VALID1f,l127);l120 = 
l118(l1,entry,&l111);SOC_IF_ERROR_RETURN(l120);soc_mem_field32_set(l1,l144,
l145,IP_LENGTH0f,l111);soc_mem_field32_set(l1,l144,l145,IP_LENGTH1f,l111);
soc_mem_field32_set(l1,l144,l146,IP_LENGTH0f,l111);soc_mem_field32_set(l1,
l144,l146,IP_LENGTH1f,l111);l127 = soc_mem_field32_get(l1,l20,entry,
IP_ADDR0_LWRf);soc_mem_field32_set(l1,l144,l145,IP_ADDR0f,l127);l127 = 
soc_mem_field32_get(l1,l20,entry,IP_ADDR1_LWRf);soc_mem_field32_set(l1,l144,
l145,IP_ADDR1f,l127);l127 = soc_mem_field32_get(l1,l20,entry,IP_ADDR0_UPRf);
soc_mem_field32_set(l1,l144,l146,IP_ADDR0f,l127);l127 = soc_mem_field32_get(
l1,l20,entry,IP_ADDR1_UPRf);soc_mem_field32_set(l1,l144,l146,IP_ADDR1f,l127);
l127 = soc_mem_field32_get(l1,l20,entry,ENTRY_TYPE0_LWRf);soc_mem_field32_set
(l1,l144,l145,ENTRY_TYPE0f,l127);l127 = soc_mem_field32_get(l1,l20,entry,
ENTRY_TYPE1_LWRf);soc_mem_field32_set(l1,l144,l145,ENTRY_TYPE1f,l127);l127 = 
soc_mem_field32_get(l1,l20,entry,ENTRY_TYPE0_UPRf);soc_mem_field32_set(l1,
l144,l146,ENTRY_TYPE0f,l127);l127 = soc_mem_field32_get(l1,l20,entry,
ENTRY_TYPE1_UPRf);soc_mem_field32_set(l1,l144,l146,ENTRY_TYPE1f,l127);l120 = 
soc_alpm_128_lpm_vrf_get(l1,entry,&l24,&l111);SOC_IF_ERROR_RETURN(l120);if(
SOC_URPF_STATUS_GET(l1)){if(l142>= (soc_mem_index_count(l1,L3_DEFIP_PAIR_128m
)>>1)){l143 = 1;}}switch(l24){case SOC_L3_VRF_OVERRIDE:soc_mem_field32_set(l1
,l144,l145,VALID0f,0);soc_mem_field32_set(l1,l144,l145,VALID1f,0);
soc_mem_field32_set(l1,l144,l146,VALID0f,0);soc_mem_field32_set(l1,l144,l146,
VALID1f,0);l8 = 0;break;case SOC_L3_VRF_GLOBAL:l8 = l143?1:0;break;default:l8
= l143?3:2;break;}soc_mem_field32_set(l1,l144,l145,DB_TYPE0f,l8);
soc_mem_field32_set(l1,l144,l145,DB_TYPE1f,l8);soc_mem_field32_set(l1,l144,
l146,DB_TYPE0f,l8);soc_mem_field32_set(l1,l144,l146,DB_TYPE1f,l8);if(l143){
l127 = soc_mem_field32_get(l1,l20,entry,ALG_BKT_PTRf);l127+= 
SOC_ALPM_BUCKET_COUNT(l1);soc_mem_field32_set(l1,l20,entry,ALG_BKT_PTRf,l127)
;}return SOC_E_NONE;}static int l147(int l1,int l148,int index,int l149,void*
entry){defip_aux_table_entry_t l141[2];l149 = soc_alpm_physical_idx(l1,
L3_DEFIP_PAIR_128m,l149,1);SOC_IF_ERROR_RETURN(l139(l1,l149,entry,&l141[0],
index));SOC_IF_ERROR_RETURN(WRITE_L3_DEFIP_PAIR_128m(l1,MEM_BLOCK_ANY,index,
entry));index = soc_alpm_physical_idx(l1,L3_DEFIP_PAIR_128m,index,1);
SOC_IF_ERROR_RETURN(WRITE_L3_DEFIP_AUX_TABLEm(l1,MEM_BLOCK_ANY,
SOC_ALPM_128_ADDR_LWR(index),l141));SOC_IF_ERROR_RETURN(
WRITE_L3_DEFIP_AUX_TABLEm(l1,MEM_BLOCK_ANY,SOC_ALPM_128_ADDR_UPR(index),l141+
1));return SOC_E_NONE;}static int l150(int l1,void*l5,soc_mem_t l20,void*l132
,void*l138,int*l13){alpm_pivot_t*l151,*l152,*l153;defip_aux_scratch_entry_t
l10;uint32 l12[SOC_MAX_MEM_FIELD_WORDS];uint32 prefix[5],l154,l125;uint32 l155
[5];int l30,l24,l134;int l112,bucket_index;int l120 = SOC_E_NONE,l156;uint32
l8,l135;int l136 =0;int l133,l157;int l158 = 0;trie_t*trie,*l159;trie_node_t*
l160,*l161;payload_t*l162,*l163;defip_pair_128_entry_t lpm_entry;
alpm_bucket_handle_t*l164;int l119,l165 = -1,l22 = 0;alpm_mem_prefix_array_t
l166;defip_alpm_ipv6_128_entry_t l167,l168;void*l169,*l170;int*l115;l30 = 
L3_DEFIP_MODE_128;SOC_IF_ERROR_RETURN(soc_alpm_128_lpm_vrf_get(l1,l5,&l134,&
l24));if(l24 == SOC_VRF_MAX(l1)+1){l8 = 0;SOC_ALPM_GET_GLOBAL_BANK_DISABLE(l1
,l135);}else{l8 = 2;SOC_ALPM_GET_VRF_BANK_DISABLE(l1,l135);}l20 = 
L3_DEFIP_ALPM_IPV6_128m;l169 = ((uint32*)&(l167));l170 = ((uint32*)&(l168));
sal_memset(&l10,0,sizeof(defip_aux_scratch_entry_t));SOC_IF_ERROR_RETURN(l6(
l1,l5,l30,l8,0,&l10));SOC_IF_ERROR_RETURN(_soc_alpm_aux_op(l1,PREFIX_LOOKUP,&
l10,&l136,&l133,&bucket_index));if(l136 == 0){soc_cm_debug(DK_ERR,
"_soc_alpm_128_insert: Could not find bucket to"" insert prefix\n");return
SOC_E_NOT_FOUND;}l120 = _soc_alpm_insert_in_bkt(l1,l20,bucket_index,l135,l132
,l12,&l112,l30);if(l120 == SOC_E_NONE){*l13 = l112;if(SOC_URPF_STATUS_GET(l1)
){l156 = soc_mem_write(l1,l20,MEM_BLOCK_ANY,_soc_alpm_rpf_entry(l1,l112),l138
);if(SOC_FAILURE(l156)){return l156;}}}if(l120 == SOC_E_FULL){l158 = 1;}l120 = 
l130(l1,l5,prefix,&l125,&l22);if(SOC_FAILURE(l120)){soc_cm_debug(DK_ERR,
"_soc_alpm_128_insert: prefix create failed\n");return l120;}l151 = 
ALPM_TCAM_PIVOT(l1,l133);trie = PIVOT_BUCKET_TRIE(l151);l153 = l151;l162 = 
sal_alloc(sizeof(payload_t),"Payload for 128b Key");if(l162 == NULL){
soc_cm_debug(DK_ERR,"_soc_alpm_128_insert: Unable to allocate memory for "
"trie node \n");return SOC_E_MEMORY;}l163 = sal_alloc(sizeof(payload_t),
"Payload for pfx trie 128b key");if(l163 == NULL){soc_cm_debug(DK_ERR,
"_soc_alpm_128_insert: Unable to allocate memory for ""pfx trie node \n");
sal_free(l162);return SOC_E_MEMORY;}sal_memset(l162,0,sizeof(*l162));
sal_memset(l163,0,sizeof(*l163));l162->key[0] = prefix[0];l162->key[1] = 
prefix[1];l162->key[2] = prefix[2];l162->key[3] = prefix[3];l162->key[4] = 
prefix[4];l162->len = l125;l162->index = l112;sal_memcpy(l163,l162,sizeof(*
l162));l120 = trie_insert(trie,prefix,NULL,l125,(trie_node_t*)l162);if(
SOC_FAILURE(l120)){goto l171;}l159 = VRF_PREFIX_TRIE_IPV6_128(l1,l24);if(!l22
){l120 = trie_insert(l159,prefix,NULL,l125,(trie_node_t*)l163);}l154 = l125;
if(SOC_FAILURE(l120)){goto l172;}if(l158){l120 = alpm_bucket_assign(l1,&
bucket_index,l30);if(SOC_FAILURE(l120)){soc_cm_debug(DK_ERR,
"_soc_alpm_128_insert: Unable to allocate""new bucket for split\n");
bucket_index = -1;goto l173;}l120 = trie_split(trie,_MAX_KEY_LEN_144_,FALSE,
l155,&l125,&l160,NULL,FALSE);if(SOC_FAILURE(l120)){soc_cm_debug(DK_ERR,
"_soc_alpm_128_insert: Could not split bucket"
"for prefix 0x%08x 0x%08x 0x%08x 0x%08x\n",prefix[1],prefix[2],prefix[3],
prefix[4]);goto l173;}l120 = soc_mem_read(l1,L3_DEFIP_PAIR_128m,MEM_BLOCK_ANY
,soc_alpm_logical_idx(l1,L3_DEFIP_PAIR_128m,SOC_ALPM_128_DEFIP_TO_PAIR(l133>>
1),1),&lpm_entry);l123((l155),(l125),(l30));l26(l1,l155,l125,l24,l30,&
lpm_entry,0,0);soc_L3_DEFIP_PAIR_128m_field32_set(l1,&lpm_entry,ALG_BKT_PTRf,
bucket_index);l164 = sal_alloc(sizeof(alpm_bucket_handle_t),
"ALPM 128 Bucket Handle");if(l164 == NULL){soc_cm_debug(DK_ERR,
"_soc_alpm_128_insert: Unable to allocate "" memory for PIVOT trie node \n");
l120 = SOC_E_MEMORY;goto l173;}sal_memset(l164,0,sizeof(*l164));l151 = 
sal_alloc(sizeof(alpm_pivot_t),"Payload for new 128b Pivot");if(l151 == NULL)
{soc_cm_debug(DK_ERR,"_soc_alpm_128_insert: Unable to allocate "
"memory for PIVOT trie node \n");l120 = SOC_E_MEMORY;goto l173;}sal_memset(
l151,0,sizeof(*l151));PIVOT_BUCKET_HANDLE(l151) = l164;l120 = trie_init(
_MAX_KEY_LEN_144_,&PIVOT_BUCKET_TRIE(l151));PIVOT_BUCKET_TRIE(l151)->trie = 
l160;PIVOT_BUCKET_INDEX(l151) = bucket_index;l151->key[0] = l155[0];l151->key
[1] = l155[1];l151->key[2] = l155[2];l151->key[3] = l155[3];l151->key[4] = 
l155[4];l151->len = l125;sal_memset(&l166,0,sizeof(l166));l120 = 
trie_traverse(PIVOT_BUCKET_TRIE(l151),alpm_mem_prefix_array_cb,&l166,
_TRIE_INORDER_TRAVERSE);if(SOC_FAILURE(l120)){soc_cm_debug(DK_ERR,
"_soc_128_alpm_insert: Bucket split failed"
"for prefix 0x%08x 0x%08x 0x%08x 0x%08x\n",prefix[1],prefix[2],prefix[3],
prefix[4]);goto l173;}l115 = sal_alloc(sizeof(*l115)*l166.count,
"Temp storage for location of prefixes in new 128b bucket");if(l115 == NULL){
l120 = SOC_E_MEMORY;goto l173;}sal_memset(l115,-1,sizeof(*l115)*l166.count);
for(l119 = 0;l119<l166.count;l119++){payload_t*prefix = l166.prefix[l119];if(
prefix->index>0){l120 = soc_mem_read(l1,l20,MEM_BLOCK_ANY,prefix->index,l169)
;if(SOC_FAILURE(l120)){soc_cm_debug(DK_ERR,"_soc_128_alpm_insert: Failed to"
"read prefix 0x%08x 0x%08x 0x%08x 0x%08x for move\n",l166.prefix[l119]->key[1
],l166.prefix[l119]->key[2],l166.prefix[l119]->key[3],l166.prefix[l119]->key[
4]);goto l174;}if(SOC_URPF_STATUS_GET(l1)){l120 = soc_mem_read(l1,l20,
MEM_BLOCK_ANY,_soc_alpm_rpf_entry(l1,prefix->index),l170);if(SOC_FAILURE(l120
)){soc_cm_debug(DK_ERR,"_soc_alpm_128_insert: Failed to"
"read rpf prefix 0x%08x 0x%08x 0x%08x 0x%08x for move\n",l166.prefix[l119]->
key[1],l166.prefix[l119]->key[2],l166.prefix[l119]->key[3],l166.prefix[l119]
->key[4]);goto l174;}}l120 = _soc_alpm_insert_in_bkt(l1,l20,bucket_index,l135
,l169,l12,&l112,l30);if(SOC_SUCCESS(l120)){if(SOC_URPF_STATUS_GET(l1)){l120 = 
soc_mem_write(l1,l20,MEM_BLOCK_ANY,_soc_alpm_rpf_entry(l1,l112),l170);}}}else
{l120 = _soc_alpm_insert_in_bkt(l1,l20,bucket_index,l135,l132,l12,&l112,l30);
if(SOC_SUCCESS(l120)){l165 = l119;*l13 = l112;l163->index = l112;if(
SOC_URPF_STATUS_GET(l1)){l120 = soc_mem_write(l1,l20,MEM_BLOCK_ANY,
_soc_alpm_rpf_entry(l1,l112),l138);}}}l115[l119] = l112;if(SOC_FAILURE(l120))
{soc_cm_debug(DK_ERR,"_soc_alpm_128_insert: Failed to"
"write prefix 0x%08x 0x%08x 0x%08x 0x%08x for move\n",l166.prefix[l119]->key[
1],l166.prefix[l119]->key[2],l166.prefix[l119]->key[3],l166.prefix[l119]->key
[4]);goto l174;}}l120 = l2(l1,&lpm_entry,&l157);if(SOC_FAILURE(l120)){
soc_cm_debug(DK_ERR,"_soc_alpm_128_insert: Unable to add new"
"pivot to tcam\n");goto l174;}l157 = soc_alpm_physical_idx(l1,
L3_DEFIP_PAIR_128m,l157,l30);ALPM_TCAM_PIVOT(l1,SOC_ALPM_128_ADDR_LWR(l157)<<
1) = l151;PIVOT_TCAM_INDEX(l151) = SOC_ALPM_128_ADDR_LWR(l157)<<1;for(l119 = 
0;l119<l166.count;l119++){payload_t*prefix = l166.prefix[l119];if(prefix->
index>0){l120 = soc_mem_read(l1,l20,MEM_BLOCK_ANY,prefix->index,l169);
soc_mem_field32_set(l1,l20,l169,VALIDf,0);if(SOC_FAILURE(l120)){soc_cm_debug(
DK_ERR,"_soc_alpm_128_insert: Failed to read"
"bkt entry for invalidate for pfx 0x%08x 0x%08x 0x%08x ""0x%08x\n",prefix->
key[1],prefix->key[2],prefix->key[3],prefix->key[4]);goto l175;}if(
SOC_URPF_STATUS_GET(l1)){l120 = soc_mem_read(l1,l20,MEM_BLOCK_ANY,
_soc_alpm_rpf_entry(l1,prefix->index),l170);soc_mem_field32_set(l1,l20,l170,
VALIDf,0);}l120 = soc_mem_write(l1,l20,MEM_BLOCK_ANY,prefix->index,l169);if(
SOC_FAILURE(l120)){soc_cm_debug(DK_ERR,
"_soc_alpm_128_insert: Failed to clear"
" prefixes from old bkt for pfx 0x%08x 0x%08x 0x%08x\n""0x%08x\n",prefix->key
[1],prefix->key[2],prefix->key[3],prefix->key[4]);goto l175;}if(
SOC_URPF_STATUS_GET(l1)){l120 = soc_mem_write(l1,l20,MEM_BLOCK_ANY,
_soc_alpm_rpf_entry(l1,prefix->index),l170);}}}for(l119 = 0;l119<l166.count;
l119++){payload_t*prefix = l166.prefix[l119];l120 = soc_mem_write(l1,l20,
MEM_BLOCK_ALL,prefix->index,soc_mem_entry_null(l1,l20));if(SOC_FAILURE(l120))
{soc_cm_debug(DK_ERR,"_soc_alpm_insert: Failed to remove"
" prefixes from old bkt for pfx 0x%08x 0x%08x\n",prefix->key[0],prefix->key[1
]);}if(SOC_URPF_STATUS_GET(l1)){l120 = soc_mem_write(l1,l20,MEM_BLOCK_ANY,
_soc_alpm_rpf_entry(l1,prefix->index),soc_mem_entry_null(l1,l20));if(
SOC_FAILURE(l120)){soc_cm_debug(DK_ERR,"_soc_alpm_insert: Failed to remove"
" prefixes from old urpf bkt for pfx 0x%08x 0x%08x\n",prefix->key[0],prefix->
key[1]);}}prefix->index = l115[l119];}sal_free(l115);if(l165 == -1){l120 = 
_soc_alpm_insert_in_bkt(l1,l20,PIVOT_BUCKET_HANDLE(l153)->bucket_index,l135,
l132,l12,&l112,l30);if(SOC_FAILURE(l120)){soc_cm_debug(DK_ERR,
"_soc_alpm_128_insert: Could not insert new ""prefix into trie after split\n"
);goto l173;}if(SOC_URPF_STATUS_GET(l1)){l120 = soc_mem_write(l1,l20,
MEM_BLOCK_ANY,_soc_alpm_rpf_entry(l1,l112),l138);}*l13 = l112;l163->index = 
l162->index = l112;}}VRF_TRIE_ROUTES_INC(l1,l24,l30);if(l22){
SOC_IF_ERROR_RETURN(_soc_alpm_aux_op(l1,DELETE_PROPAGATE,&l10,&l136,&l133,&
bucket_index));sal_free(l163);}SOC_IF_ERROR_RETURN(_soc_alpm_aux_op(l1,
INSERT_PROPAGATE,&l10,&l136,&l133,&bucket_index));if(SOC_URPF_STATUS_GET(l1))
{l125 = soc_mem_field32_get(l1,L3_DEFIP_AUX_SCRATCHm,&l10,DB_TYPEf);l125+= 1;
soc_mem_field32_set(l1,L3_DEFIP_AUX_SCRATCHm,&l10,DB_TYPEf,l125);
SOC_IF_ERROR_RETURN(_soc_alpm_aux_op(l1,DELETE_PROPAGATE,&l10,&l136,&l133,&
bucket_index));SOC_IF_ERROR_RETURN(_soc_alpm_aux_op(l1,INSERT_PROPAGATE,&l10,
&l136,&l133,&bucket_index));}return l120;l175:for(l119 = 0;l119<l166.count;
l119++){payload_t*prefix = l166.prefix[l119];if(prefix->index>0){l156 = 
soc_mem_read(l1,l20,MEM_BLOCK_ANY,prefix->index,l169);if(SOC_FAILURE(l156)){
soc_cm_debug(DK_ERR,"_soc_alpm_128_insert: Failure to read prefix"
"0x%08x 0x%08x for move back\n",prefix->key[0],prefix->key[1]);break;}if(
soc_mem_field32_get(l1,l20,l169,VALIDf)){break;}soc_mem_field32_set(l1,l20,
l169,VALIDf,1);l120 = soc_mem_write(l1,l20,MEM_BLOCK_ALL,prefix->index,l169);
if(SOC_FAILURE(l120)){soc_cm_debug(DK_ERR,
"_soc_alpm_128_insert: Failure to write prefix"
"0x%08x 0x%08x for move back\n",prefix->key[0],prefix->key[1]);break;}if(
SOC_URPF_STATUS_GET(l1)){l156 = soc_mem_read(l1,l20,MEM_BLOCK_ANY,
_soc_alpm_rpf_entry(l1,prefix->index),l169);soc_mem_field32_set(l1,l20,l169,
VALIDf,1);l156 = soc_mem_write(l1,l20,MEM_BLOCK_ALL,_soc_alpm_rpf_entry(l1,
prefix->index),l169);}}}l156 = l4(l1,&lpm_entry);if(SOC_FAILURE(l156)){
soc_cm_debug(DK_ERR,"_soc_alpm_128_insert: Failure to free new prefix"
"at %d\n",soc_alpm_logical_idx(l1,L3_DEFIP_PAIR_128m,l157,l30));}
ALPM_TCAM_PIVOT(l1,SOC_ALPM_128_ADDR_LWR(l157)<<1) = NULL;l174:l152 = l153;
for(l119 = 0;l119<l166.count;l119++){payload_t*prefix = l166.prefix[l119];if(
l115[l119]!= -1){sal_memset(l169,0,sizeof(l132));l156 = soc_mem_write(l1,l20,
MEM_BLOCK_ANY,l115[l119],l169);if(SOC_FAILURE(l156)){soc_cm_debug(DK_ERR,
"_soc_alpm_128_insert: mem write failure""in bkt move rollback\n");}if(
SOC_URPF_STATUS_GET(l1)){l156 = soc_mem_write(l1,l20,MEM_BLOCK_ANY,
_soc_alpm_rpf_entry(l1,l115[l119]),l169);if(SOC_FAILURE(l156)){soc_cm_debug(
DK_ERR,"_soc_alpm_128_insert: urpf mem write "
"failure in bkt move rollback\n");}}}l156 = trie_delete(PIVOT_BUCKET_TRIE(
l151),prefix->key,prefix->len,&l161);l162 = (payload_t*)l161;if(SOC_FAILURE(
l156)){soc_cm_debug(DK_ERR,"_soc_alpm_128_insert: trie delete failure"
"in bkt move rollback\n");}if(prefix->index>0){l156 = trie_insert(
PIVOT_BUCKET_TRIE(l152),prefix->key,NULL,prefix->len,(trie_node_t*)l162);if(
SOC_FAILURE(l156)){soc_cm_debug(DK_ERR,
"_soc_alpm_128_insert: trie reinsert failure""in bkt move rollback\n");}}}if(
l165 == -1){l156 = trie_delete(PIVOT_BUCKET_TRIE(l152),prefix,l154,&l161);if(
SOC_FAILURE(l156)){soc_cm_debug(DK_ERR,
"_soc_alpm_128_insert: expected to clear prefix"
" 0x%08x 0x%08x\n from old trie. Failed\n",prefix[0],prefix[1]);}}l156 = 
alpm_bucket_release(l1,PIVOT_BUCKET_INDEX(l151),l30);if(SOC_FAILURE(l156)){
soc_cm_debug(DK_ERR,"_soc_alpm_128_insert: new bucket release "
"failure: %d\n",PIVOT_BUCKET_INDEX(l151));}trie_destroy(PIVOT_BUCKET_TRIE(
l151));sal_free(l164);sal_free(l151);sal_free(l115);l163 = NULL;l156 = 
trie_delete(l159,prefix,l154,&l161);l163 = (payload_t*)l161;if(SOC_FAILURE(
l156)){soc_cm_debug(DK_ERR,
"_soc_alpm_128_insert: failed to delete new prefix"
"0x%08x 0x%08x from pfx trie\n",prefix[0],prefix[1]);}if(l163){sal_free(l163)
;}return l120;l173:(void)trie_delete(l159,prefix,l154,&l161);l163 = (
payload_t*)l161;if(bucket_index!= -1){(void)alpm_bucket_release(l1,
bucket_index,l30);}l172:(void)trie_delete(trie,prefix,l154,&l161);l162 = (
payload_t*)l161;l171:sal_free(l162);sal_free(l163);return l120;}static int l26
(int l17,uint32*key,int len,int l24,int l7,defip_pair_128_entry_t*lpm_entry,
int l27,int l28){uint32 l127;if(l28){sal_memset(lpm_entry,0,sizeof(
defip_pair_128_entry_t));}soc_L3_DEFIP_PAIR_128m_field32_set(l17,lpm_entry,
VRF_ID_0_LWRf,l24&SOC_VRF_MAX(l17));soc_L3_DEFIP_PAIR_128m_field32_set(l17,
lpm_entry,VRF_ID_1_LWRf,l24&SOC_VRF_MAX(l17));
soc_L3_DEFIP_PAIR_128m_field32_set(l17,lpm_entry,VRF_ID_0_UPRf,l24&
SOC_VRF_MAX(l17));soc_L3_DEFIP_PAIR_128m_field32_set(l17,lpm_entry,
VRF_ID_1_UPRf,l24&SOC_VRF_MAX(l17));if(l24 == (SOC_VRF_MAX(l17)+1)){l127 = 0;
}else{l127 = SOC_VRF_MAX(l17);}soc_L3_DEFIP_PAIR_128m_field32_set(l17,
lpm_entry,VRF_ID_MASK0_LWRf,l127);soc_L3_DEFIP_PAIR_128m_field32_set(l17,
lpm_entry,VRF_ID_MASK1_LWRf,l127);soc_L3_DEFIP_PAIR_128m_field32_set(l17,
lpm_entry,VRF_ID_MASK0_UPRf,l127);soc_L3_DEFIP_PAIR_128m_field32_set(l17,
lpm_entry,VRF_ID_MASK1_UPRf,l127);soc_L3_DEFIP_PAIR_128m_field32_set(l17,
lpm_entry,IP_ADDR0_LWRf,key[0]);soc_L3_DEFIP_PAIR_128m_field32_set(l17,
lpm_entry,IP_ADDR1_LWRf,key[1]);soc_L3_DEFIP_PAIR_128m_field32_set(l17,
lpm_entry,IP_ADDR0_UPRf,key[2]);soc_L3_DEFIP_PAIR_128m_field32_set(l17,
lpm_entry,IP_ADDR1_UPRf,key[3]);soc_L3_DEFIP_PAIR_128m_field32_set(l17,
lpm_entry,MODE0_LWRf,3);soc_L3_DEFIP_PAIR_128m_field32_set(l17,lpm_entry,
MODE1_LWRf,3);soc_L3_DEFIP_PAIR_128m_field32_set(l17,lpm_entry,MODE0_UPRf,3);
soc_L3_DEFIP_PAIR_128m_field32_set(l17,lpm_entry,MODE1_UPRf,3);l128(l17,(void
*)lpm_entry,len);soc_L3_DEFIP_PAIR_128m_field32_set(l17,lpm_entry,VALID0_LWRf
,1);soc_L3_DEFIP_PAIR_128m_field32_set(l17,lpm_entry,VALID1_LWRf,1);
soc_L3_DEFIP_PAIR_128m_field32_set(l17,lpm_entry,VALID0_UPRf,1);
soc_L3_DEFIP_PAIR_128m_field32_set(l17,lpm_entry,VALID1_UPRf,1);
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,MODE_MASK0_LWRf,(1<<
soc_mem_field_length(l17,L3_DEFIP_PAIR_128m,MODE_MASK0_LWRf))-1);
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,MODE_MASK1_LWRf,(1<<
soc_mem_field_length(l17,L3_DEFIP_PAIR_128m,MODE_MASK1_LWRf))-1);
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,MODE_MASK0_UPRf,(1<<
soc_mem_field_length(l17,L3_DEFIP_PAIR_128m,MODE_MASK0_UPRf))-1);
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,MODE_MASK1_UPRf,(1<<
soc_mem_field_length(l17,L3_DEFIP_PAIR_128m,MODE_MASK1_UPRf))-1);
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,ENTRY_TYPE_MASK0_LWRf,(1
<<soc_mem_field_length(l17,L3_DEFIP_PAIR_128m,ENTRY_TYPE_MASK0_LWRf))-1);
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,ENTRY_TYPE_MASK1_LWRf,(1
<<soc_mem_field_length(l17,L3_DEFIP_PAIR_128m,ENTRY_TYPE_MASK1_LWRf))-1);
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,ENTRY_TYPE_MASK0_UPRf,(1
<<soc_mem_field_length(l17,L3_DEFIP_PAIR_128m,ENTRY_TYPE_MASK0_UPRf))-1);
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,ENTRY_TYPE_MASK1_UPRf,(1
<<soc_mem_field_length(l17,L3_DEFIP_PAIR_128m,ENTRY_TYPE_MASK1_UPRf))-1);
return(SOC_E_NONE);}static int l176(int l1,void*l5){alpm_pivot_t*l151;
defip_alpm_ipv6_128_entry_t l167,l177,l168;defip_aux_scratch_entry_t l10;
uint32 l12[SOC_MAX_MEM_FIELD_WORDS];soc_mem_t l20;void*l169,*l178,*l170 = 
NULL;int l134;int l7;int l120 = SOC_E_NONE,l156;uint32 l179[5],prefix[5];int
l30,l24;uint32 l125;int l112,bucket_index;int l180;uint32 l8,l135;int l133,
l136,l22 = 0;trie_t*trie,*l159;uint32 l181;defip_pair_128_entry_t lpm_entry,*
l182;payload_t*l162 = NULL,*l183 = NULL;trie_node_t*l161;l7 = l30 = 
L3_DEFIP_MODE_128;SOC_IF_ERROR_RETURN(soc_alpm_128_lpm_vrf_get(l1,l5,&l134,&
l24));if(l134!= SOC_L3_VRF_OVERRIDE){if(l24 == SOC_VRF_MAX(l1)+1){l8 = 0;
SOC_ALPM_GET_GLOBAL_BANK_DISABLE(l1,l135);}else{l8 = 2;
SOC_ALPM_GET_VRF_BANK_DISABLE(l1,l135);}l120 = l130(l1,l5,prefix,&l125,&l22);
if(SOC_FAILURE(l120)){soc_cm_debug(DK_ERR,
"_soc_alpm_128_delete: prefix create ""failed\n");return l120;}if(!
soc_alpm_mode_get(l1)){if(l134!= SOC_L3_VRF_GLOBAL){if(VRF_TRIE_ROUTES_CNT(l1
,l24,l30)>1){if(l22){soc_cm_debug(DK_ERR,"VRF %d: Cannot v6-128 delete "
"default route if other routes are present ""in this mode",l24);return
SOC_E_PARAM;}}}l8 = 2;}l20 = L3_DEFIP_ALPM_IPV6_128m;l169 = ((uint32*)&(l167)
);SOC_ALPM_LPM_LOCK(l1);l120 = l131(l1,l5,l20,l169,&l133,&bucket_index,&l112)
;sal_memcpy(&l177,l169,sizeof(l177));l178 = &l177;if(SOC_FAILURE(l120)){
SOC_ALPM_LPM_UNLOCK(l1);soc_cm_debug(DK_ERR,
"_soc_alpm_128_delete: Unable to find ""prefix for delete\n");return l120;}
l180 = bucket_index;l151 = ALPM_TCAM_PIVOT(l1,l133);trie = PIVOT_BUCKET_TRIE(
l151);l120 = trie_delete(trie,prefix,l125,&l161);l162 = (payload_t*)l161;if(
l120!= SOC_E_NONE){soc_cm_debug(DK_ERR,
"_soc_alpm_128_delete: Error prefix not ""present in trie \n");goto l184;}
l159 = VRF_PREFIX_TRIE_IPV6_128(l1,l24);if(!l22){l120 = trie_delete(l159,
prefix,l125,&l161);l183 = (payload_t*)l161;if(SOC_FAILURE(l120)){soc_cm_debug
(DK_ERR,"_soc_alpm_128_delete: Prefix not present "
"in pfx trie: 0x%08x 0x%08x 0x%08x 0x%08x\n",prefix[1],prefix[2],prefix[3],
prefix[4]);goto l185;}l120 = trie_find_prefix_bpm(l159,prefix,l125,&l181);if(
SOC_FAILURE(l120)){soc_cm_debug(DK_ERR,"_soc_alpm_128_delete: Could not find"
" replacement bpm for prefix: 0x%08x 0x%08x ""0x%08x 0x%08x\n",prefix[1],
prefix[2],prefix[3],prefix[4]);goto l185;}l120 = l26(l1,prefix,l181,l24,l7,&
lpm_entry,0,1);l156 = l131(l1,&lpm_entry,l20,l169,&l133,&bucket_index,&l112);
sal_memcpy(l179,prefix,sizeof(prefix));l120 = l26(l1,l179,l125,l24,l7,&
lpm_entry,0,0);if(SOC_URPF_STATUS_GET(l1)){if(SOC_SUCCESS(l120)){l170 = ((
uint32*)&(l168));l156 = soc_mem_read(l1,l20,MEM_BLOCK_ANY,_soc_alpm_rpf_entry
(l1,l112),l170);}}if((l181 == 0)&&SOC_FAILURE(l156)){l182 = 
VRF_TRIE_DEFAULT_ROUTE_IPV6_128(l1,l24);sal_memcpy(&lpm_entry,l182,sizeof(
lpm_entry));l120 = l26(l1,prefix,l181,l24,l7,&lpm_entry,0,1);}if(SOC_FAILURE(
l120)){soc_cm_debug(DK_ERR,"_soc_alpm_128_delete: Could not find "
"replacement prefix for prefix: 0x%08x 0x%08x 0x%08x ""0x%08x\n",prefix[1],
prefix[2],prefix[3],prefix[4]);goto l185;}l182 = &lpm_entry;}else{l181 = 0;
l182 = VRF_TRIE_DEFAULT_ROUTE_IPV6_128(l1,l24);}l120 = l6(l1,l182,l30,l8,l181
,&l10);if(SOC_FAILURE(l120)){goto l185;}l120 = _soc_alpm_aux_op(l1,
DELETE_PROPAGATE,&l10,&l136,&l133,&bucket_index);if(SOC_FAILURE(l120)){goto
l185;}if(SOC_URPF_STATUS_GET(l1)){uint32 l127;if(l170!= NULL){l127 = 
soc_mem_field32_get(l1,L3_DEFIP_AUX_SCRATCHm,&l10,DB_TYPEf);l127++;
soc_mem_field32_set(l1,L3_DEFIP_AUX_SCRATCHm,&l10,DB_TYPEf,l127);l127 = 
soc_mem_field32_get(l1,l20,l170,SRC_DISCARDf);soc_mem_field32_set(l1,l20,&l10
,SRC_DISCARDf,l127);l127 = soc_mem_field32_get(l1,l20,l170,DEFAULTROUTEf);
soc_mem_field32_set(l1,l20,&l10,DEFAULTROUTEf,l127);l120 = _soc_alpm_aux_op(
l1,DELETE_PROPAGATE,&l10,&l136,&l133,&bucket_index);}if(SOC_FAILURE(l120)){
goto l185;}}sal_free(l162);if(!l22){sal_free(l183);}l120 = 
_soc_alpm_delete_in_bkt(l1,l20,l180,l135,l178,l12,&l112,l30);if(!SOC_SUCCESS(
l120)){SOC_ALPM_LPM_UNLOCK(l1);l120 = SOC_E_FAIL;return l120;}if(
SOC_URPF_STATUS_GET(l1)){l120 = soc_mem_alpm_delete(l1,l20,
SOC_ALPM_RPF_BKT_IDX(l1,l180),MEM_BLOCK_ALL,l135,l178,l12,&l136);if(!
SOC_SUCCESS(l120)){SOC_ALPM_LPM_UNLOCK(l1);l120 = SOC_E_FAIL;return l120;}}if
((l151->len!= 0)&&(trie->trie == NULL)){l26(l1,l151->key,l151->len,l24,l7,&
lpm_entry,0,1);l120 = l4(l1,&lpm_entry);if(SOC_FAILURE(l120)){soc_cm_debug(
DK_ERR,"_soc_alpm_128_delete: Unable to "
"delete pivot 0x%08x 0x%08x 0x%08x 0x%08x \n",l151->key[1],l151->key[2],l151
->key[3],l151->key[4]);}l120 = alpm_bucket_release(l1,PIVOT_BUCKET_INDEX(l151
),l30);if(SOC_FAILURE(l120)){soc_cm_debug(DK_ERR,
"_soc_alpm_128_delete: Unable to release""empty bucket: %d\n",
PIVOT_BUCKET_INDEX(l151));}trie_destroy(PIVOT_BUCKET_TRIE(l151));sal_free(
PIVOT_BUCKET_HANDLE(l151));sal_free(l151);}}VRF_TRIE_ROUTES_DEC(l1,l24,l30);
if(VRF_TRIE_ROUTES_CNT(l1,l24,l30) == 0){l120 = l29(l1,l24,l30);}
SOC_ALPM_LPM_UNLOCK(l1);return l120;l185:l156 = trie_insert(l159,prefix,NULL,
l125,(trie_node_t*)l183);if(SOC_FAILURE(l156)){soc_cm_debug(DK_ERR,
"_soc_alpm_128_delete: Unable to reinsert"
"prefix 0x%08x 0x%08x 0x%08x 0x%08x into pfx trie\n",prefix[1],prefix[2],
prefix[3],prefix[4]);}l184:l156 = trie_insert(trie,prefix,NULL,l125,(
trie_node_t*)l162);if(SOC_FAILURE(l156)){soc_cm_debug(DK_ERR,
"_soc_alpm_128_delete: Unable to reinsert"
"prefix 0x%08x 0x%08x 0x%08x 0x%08x into pfx trie\n",prefix[1],prefix[2],
prefix[3],prefix[4]);}SOC_ALPM_LPM_UNLOCK(l1);return l120;}int
soc_alpm_128_init(int l1){int l120 = SOC_E_NONE;l120 = soc_alpm_128_lpm_init(
l1);SOC_IF_ERROR_RETURN(l120);return l120;}int soc_alpm_128_state_clear(int l1
){int l119,l120;for(l119 = 0;l119<= SOC_VRF_MAX(l1)+1;l119++){l120 = 
trie_traverse(VRF_PREFIX_TRIE_IPV6_128(l1,l119),alpm_delete_node_cb,NULL,
_TRIE_INORDER_TRAVERSE);if(SOC_SUCCESS(l120)){trie_destroy(
VRF_PREFIX_TRIE_IPV6_128(l1,l119));}else{soc_cm_debug(DK_ERR,
"unit: %d Unable to clear v6_128 pfx trie for ""vrf %d\n",l1,l119);return l120
;}if(VRF_TRIE_DEFAULT_ROUTE_IPV6_128(l1,l119)!= NULL){sal_free(
VRF_TRIE_DEFAULT_ROUTE_IPV6_128(l1,l119));}}return SOC_E_NONE;}int
soc_alpm_128_deinit(int l1){soc_alpm_128_lpm_deinit(l1);SOC_IF_ERROR_RETURN(
soc_alpm_128_state_clear(l1));return SOC_E_NONE;}static int l186(int l1,int
l24,int l30){defip_pair_128_entry_t*lpm_entry;int l187;int index;int l120 = 
SOC_E_NONE;uint32 key[5] = {0,0,0,0,0};uint32 l125;alpm_bucket_handle_t*l164;
alpm_pivot_t*l151;payload_t*l183;trie_t*l188;trie_init(_MAX_KEY_LEN_144_,&
VRF_PREFIX_TRIE_IPV6_128(l1,l24));l188 = VRF_PREFIX_TRIE_IPV6_128(l1,l24);
lpm_entry = sal_alloc(sizeof(*lpm_entry),"Default 128 LPM entry");if(
lpm_entry == NULL){soc_cm_debug(DK_ERR,
"soc_alpm_128_vrf_add: unable to allocate memory ""for IPv6-128 LPM entry\n")
;return SOC_E_MEMORY;}l26(l1,key,0,l24,l30,lpm_entry,0,1);
VRF_TRIE_DEFAULT_ROUTE_IPV6_128(l1,l24) = lpm_entry;if(l24 == SOC_VRF_MAX(l1)
+1){soc_L3_DEFIP_PAIR_128m_field32_set(l1,lpm_entry,GLOBAL_ROUTEf,1);}else{
soc_L3_DEFIP_PAIR_128m_field32_set(l1,lpm_entry,DEFAULT_MISSf,1);}l120 = 
alpm_bucket_assign(l1,&l187,l30);soc_L3_DEFIP_PAIR_128m_field32_set(l1,
lpm_entry,ALG_BKT_PTRf,l187);l120 = l2(l1,lpm_entry,&index);l164 = sal_alloc(
sizeof(alpm_bucket_handle_t),"ALPM Bucket Handle");if(l164 == NULL){
soc_cm_debug(DK_ERR,"soc_alpm_128_vrf_add: Unable to allocate memory "
"for bucket handle \n");return SOC_E_NONE;}sal_memset(l164,0,sizeof(*l164));
l151 = sal_alloc(sizeof(alpm_pivot_t),"Payload for Pivot");if(l151 == NULL){
soc_cm_debug(DK_ERR,"soc_alpm_128_vrf_add: Unable to allocate memory "
"for PIVOT trie node \n");sal_free(l164);return SOC_E_MEMORY;}l183 = 
sal_alloc(sizeof(payload_t),"Payload for pfx trie key");if(l183 == NULL){
soc_cm_debug(DK_ERR,"soc_alpm_128_vrf_add: Unable to allocate memory "
"for pfx trie node \n");sal_free(l164);sal_free(l151);return SOC_E_MEMORY;}
sal_memset(l151,0,sizeof(*l151));sal_memset(l183,0,sizeof(*l183));l125 = 0;
PIVOT_BUCKET_HANDLE(l151) = l164;trie_init(_MAX_KEY_LEN_144_,&
PIVOT_BUCKET_TRIE(l151));PIVOT_BUCKET_INDEX(l151) = l187;PIVOT_TCAM_INDEX(
l151) = index;l151->key[0] = l183->key[0] = key[0];l151->key[1] = l183->key[1
] = key[1];l151->key[2] = l183->key[2] = key[2];l151->key[3] = l183->key[3] = 
key[3];l151->key[4] = l183->key[4] = key[4];l151->len = l183->len = l125;l120
= trie_insert(l188,key,NULL,l125,&(l183->node));if(SOC_FAILURE(l120)){
sal_free(l183);sal_free(l151);sal_free(l164);return l120;}index = 
soc_alpm_physical_idx(l1,L3_DEFIP_PAIR_128m,index,l30);ALPM_TCAM_PIVOT(l1,
SOC_ALPM_128_ADDR_LWR(index)<<1) = l151;VRF_TRIE_INIT_DONE(l1,l24,l30,1);
return l120;}static int l29(int l1,int l24,int l30){defip_pair_128_entry_t*
lpm_entry;int l187;int l189;int l120 = SOC_E_NONE;uint32 key[2] = {0,0},l124[
SOC_MAX_MEM_FIELD_WORDS];payload_t*l162;alpm_pivot_t*l190;trie_node_t*l161;
trie_t*l188;lpm_entry = VRF_TRIE_DEFAULT_ROUTE_IPV6_128(l1,l24);l187 = 
soc_L3_DEFIP_PAIR_128m_field32_get(l1,lpm_entry,ALG_BKT_PTRf);l120 = 
alpm_bucket_release(l1,l187,l30);l120 = l15(l1,lpm_entry,(void*)l124,&l189);
if(SOC_FAILURE(l120)){soc_cm_debug(DK_ERR,
"soc_alpm_vrf_delete: unable to get internal"" pivot idx for vrf %d/%d\n",l24
,l30);l189 = -1;}l189 = soc_alpm_physical_idx(l1,L3_DEFIP_PAIR_128m,l189,l30)
;l190 = ALPM_TCAM_PIVOT(l1,SOC_ALPM_128_ADDR_LWR(l189)<<1);l120 = l4(l1,
lpm_entry);if(SOC_FAILURE(l120)){soc_cm_debug(DK_ERR,
"soc_alpm_128_vrf_delete: unable to delete lpm "
"entry for internal default for vrf %d/%d\n",l24,l30);}sal_free(lpm_entry);
VRF_TRIE_DEFAULT_ROUTE_IPV6_128(l1,l24) = NULL;l188 = 
VRF_PREFIX_TRIE_IPV6_128(l1,l24);VRF_PREFIX_TRIE_IPV6_128(l1,l24) = NULL;
VRF_TRIE_INIT_DONE(l1,l24,l30,0);l120 = trie_delete(l188,key,0,&l161);l162 = 
(payload_t*)l161;if(SOC_FAILURE(l120)){soc_cm_debug(DK_ERR,
"Unable to delete internal default for 128b vrf "" %d/%d\n",l24,l30);}
sal_free(l162);(void)trie_destroy(l188);sal_free(PIVOT_BUCKET_HANDLE(l190));
sal_free(l190);return l120;}int soc_alpm_128_insert(int l1,void*l3,uint32 l21
){defip_alpm_ipv6_128_entry_t l167,l168;soc_mem_t l20;void*l169,*l178;int l134
,l24;int index;int l7;int l120 = SOC_E_NONE;uint32 l22;l7 = L3_DEFIP_MODE_128
;l20 = L3_DEFIP_ALPM_IPV6_128m;l169 = ((uint32*)&(l167));l178 = ((uint32*)&(
l168));SOC_IF_ERROR_RETURN(l16(l1,l3,l169,l178,l20,l21,&l22));
SOC_IF_ERROR_RETURN(soc_alpm_128_lpm_vrf_get(l1,l3,&l134,&l24));if(l134 == 
SOC_L3_VRF_OVERRIDE){l120 = l2(l1,l3,&index);return(l120);}if(l134!= 
SOC_L3_VRF_GLOBAL){if(!soc_alpm_mode_get(l1)){if(VRF_TRIE_ROUTES_CNT(l1,l24,
l7) == 0){if(!l22){soc_cm_debug(DK_ERR,"VRF %d: First route in a VRF has to "
" be a default route in this mode\n",l134);return SOC_E_PARAM;}}}}if(!
VRF_TRIE_INIT_COMPLETED(l1,l24,l7)){soc_cm_debug(DK_VERBOSE,
"soc_alpm_128_insert:VRF %d is not ""initialized\n",l24);l120 = l186(l1,l24,
l7);if(SOC_FAILURE(l120)){soc_cm_debug(DK_ERR,
"soc_alpm_128_insert:VRF %d/%d trie init \n""failed\n",l24,l7);return l120;}
soc_cm_debug(DK_VERBOSE,"soc_alpm_128_insert:VRF %d/%d trie init "
"completed\n",l24,l7);}l120 = l137(l1,l3,l169,l178,l20,&index);if(l120 == 
SOC_E_NOT_FOUND){l120 = l150(l1,l3,l20,l169,l178,&index);}if(l120!= 
SOC_E_NONE){soc_cm_debug(DK_WARN,"unit %d :soc_alpm_128_insert: "
"Route Insertion Failed :%s\n",l1,soc_errmsg(l120));}return(l120);}int
soc_alpm_128_lookup(int l1,void*l5,void*l12,int*l13){
defip_alpm_ipv6_128_entry_t l167;soc_mem_t l20;int bucket_index;int l133;void
*l169;int l134,l24;int l7 = 2,l111;int l120 = SOC_E_NONE;SOC_IF_ERROR_RETURN(
soc_alpm_128_lpm_vrf_get(l1,l5,&l134,&l24));l120 = l11(l1,l5,l12,l13,&l111,&
l7);if(SOC_SUCCESS(l120)){SOC_IF_ERROR_RETURN(soc_alpm_128_lpm_vrf_get(l1,l12
,&l134,&l24));if(l134 == SOC_L3_VRF_OVERRIDE){return SOC_E_NONE;}}if(!
VRF_TRIE_INIT_COMPLETED(l1,l24,l7)){soc_cm_debug(DK_VERBOSE,
"soc_alpm_lookup:VRF %d is not ""initialized\n",l24);return SOC_E_NOT_FOUND;}
l20 = L3_DEFIP_ALPM_IPV6_128m;l169 = ((uint32*)&(l167));SOC_ALPM_LPM_LOCK(l1)
;l120 = l131(l1,l5,l20,l169,&l133,&bucket_index,l13);SOC_ALPM_LPM_UNLOCK(l1);
if(SOC_FAILURE(l120)){return l120;}l120 = l23(l1,l169,l20,l7,l134,
bucket_index,*l13,l12);return(l120);}int soc_alpm_128_delete(int l1,void*l5){
int l134,l24;int l7;int l120 = SOC_E_NONE;l7 = L3_DEFIP_MODE_128;
SOC_IF_ERROR_RETURN(soc_alpm_128_lpm_vrf_get(l1,l5,&l134,&l24));if(l134 == 
SOC_L3_VRF_OVERRIDE){l120 = l4(l1,l5);return(l120);}else{if(!
VRF_TRIE_INIT_COMPLETED(l1,l24,l7)){soc_cm_debug(DK_VERBOSE,
"soc_alpm_128_delete:VRF %d/%d is not ""initialized\n",l24,l7);return
SOC_E_NONE;}l120 = l176(l1,l5);}return(l120);}static void l99(int l1,void*l12
,int index,l94 l100){l100[0] = soc_meminfo_fieldinfo_field32_get((&
SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l12),(l87[(l1)]->l50));l100[1] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l12
),(l87[(l1)]->l48));l100[2] = soc_meminfo_fieldinfo_field32_get((&
SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l12),(l87[(l1)]->l54));l100[3] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l12
),(l87[(l1)]->l52));l100[4] = soc_meminfo_fieldinfo_field32_get((&
SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l12),(l87[(l1)]->l51));l100[5] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l12
),(l87[(l1)]->l49));l100[6] = soc_meminfo_fieldinfo_field32_get((&
SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l12),(l87[(l1)]->l55));l100[7] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l12
),(l87[(l1)]->l53));if((!(SOC_IS_HURRICANE(l1)))&&(((l87[(l1)]->l73)!= NULL))
){int l191;(void)soc_alpm_128_lpm_vrf_get(l1,l12,(int*)&l100[8],&l191);}else{
l100[8] = 0;};}static int l192(l94 l96,l94 l97){int l189;for(l189 = 0;l189<8;
l189++){{if((l96[l189])<(l97[l189])){return-1;}if((l96[l189])>(l97[l189])){
return 1;}};}return(0);}static void l193(int l1,void*l3,uint32 l194,uint32
l114,int l111){l94 l195;if(soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(
l1,L3_DEFIP_PAIR_128m)),(l3),(l87[(l1)]->l70))&&
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l3)
,(l87[(l1)]->l69))&&soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,
L3_DEFIP_PAIR_128m)),(l3),(l87[(l1)]->l68))&&
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l3)
,(l87[(l1)]->l67))){l195[0] = soc_meminfo_fieldinfo_field32_get((&
SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l3),(l87[(l1)]->l50));l195[1] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l3)
,(l87[(l1)]->l48));l195[2] = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO
(l1,L3_DEFIP_PAIR_128m)),(l3),(l87[(l1)]->l54));l195[3] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l3)
,(l87[(l1)]->l52));l195[4] = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO
(l1,L3_DEFIP_PAIR_128m)),(l3),(l87[(l1)]->l51));l195[5] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l3)
,(l87[(l1)]->l49));l195[6] = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO
(l1,L3_DEFIP_PAIR_128m)),(l3),(l87[(l1)]->l55));l195[7] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l3)
,(l87[(l1)]->l53));if((!(SOC_IS_HURRICANE(l1)))&&(((l87[(l1)]->l73)!= NULL)))
{int l191;(void)soc_alpm_128_lpm_vrf_get(l1,l3,(int*)&l195[8],&l191);}else{
l195[8] = 0;};l113((l98[(l1)]),l192,l195,l111,l114,l194);}}void l196(int l1,
void*l5,uint32 l194){l94 l195;int l111 = -1;int l120;uint16 index;l195[0] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l5)
,(l87[(l1)]->l50));l195[1] = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO
(l1,L3_DEFIP_PAIR_128m)),(l5),(l87[(l1)]->l48));l195[2] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l5)
,(l87[(l1)]->l54));l195[3] = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO
(l1,L3_DEFIP_PAIR_128m)),(l5),(l87[(l1)]->l52));l195[4] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l5)
,(l87[(l1)]->l51));l195[5] = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO
(l1,L3_DEFIP_PAIR_128m)),(l5),(l87[(l1)]->l49));l195[6] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l5)
,(l87[(l1)]->l55));l195[7] = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO
(l1,L3_DEFIP_PAIR_128m)),(l5),(l87[(l1)]->l53));if((!(SOC_IS_HURRICANE(l1)))
&&(((l87[(l1)]->l73)!= NULL))){int l191;(void)soc_alpm_128_lpm_vrf_get(l1,l5,
(int*)&l195[8],&l191);}else{l195[8] = 0;};index = l194;l120 = l116((l98[(l1)]
),l192,l195,l111,index);if(SOC_FAILURE(l120)){soc_cm_debug(DK_ERR,
"\ndel  index: H %d error %d\n",index,l120);}}int l197(int l1,void*l5,int l111
,int*l112){l94 l195;int l120;uint16 index = (0xFFFF);l195[0] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l5)
,(l87[(l1)]->l50));l195[1] = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO
(l1,L3_DEFIP_PAIR_128m)),(l5),(l87[(l1)]->l48));l195[2] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l5)
,(l87[(l1)]->l54));l195[3] = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO
(l1,L3_DEFIP_PAIR_128m)),(l5),(l87[(l1)]->l52));l195[4] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l5)
,(l87[(l1)]->l51));l195[5] = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO
(l1,L3_DEFIP_PAIR_128m)),(l5),(l87[(l1)]->l49));l195[6] = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l5)
,(l87[(l1)]->l55));l195[7] = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO
(l1,L3_DEFIP_PAIR_128m)),(l5),(l87[(l1)]->l53));if((!(SOC_IS_HURRICANE(l1)))
&&(((l87[(l1)]->l73)!= NULL))){int l191;(void)soc_alpm_128_lpm_vrf_get(l1,l5,
(int*)&l195[8],&l191);}else{l195[8] = 0;};l120 = l108((l98[(l1)]),l192,l195,
l111,&index);if(SOC_FAILURE(l120)){*l112 = 0xFFFFFFFF;return(l120);}*l112 = 
index;return(SOC_E_NONE);}static uint16 l101(uint8*l102,int l103){return(
_shr_crc16b(0,l102,l103));}static int l104(int l17,int l89,int l90,l93**l105)
{l93*l109;int index;if(l90>l89){return SOC_E_MEMORY;}l109 = sal_alloc(sizeof(
l93),"lpm_hash");if(l109 == NULL){return SOC_E_MEMORY;}sal_memset(l109,0,
sizeof(*l109));l109->l17 = l17;l109->l89 = l89;l109->l90 = l90;l109->l91 = 
sal_alloc(l109->l90*sizeof(*(l109->l91)),"hash_table");if(l109->l91 == NULL){
sal_free(l109);return SOC_E_MEMORY;}l109->l92 = sal_alloc(l109->l89*sizeof(*(
l109->l92)),"link_table");if(l109->l92 == NULL){sal_free(l109->l91);sal_free(
l109);return SOC_E_MEMORY;}for(index = 0;index<l109->l90;index++){l109->l91[
index] = (0xFFFF);}for(index = 0;index<l109->l89;index++){l109->l92[index] = 
(0xFFFF);}*l105 = l109;return SOC_E_NONE;}static int l106(l93*l107){if(l107!= 
NULL){sal_free(l107->l91);sal_free(l107->l92);sal_free(l107);}return
SOC_E_NONE;}static int l108(l93*l109,l95 l110,l94 entry,int l111,uint16*l112)
{int l1 = l109->l17;uint16 l198;uint16 index;l198 = l101((uint8*)entry,(32*9)
)%l109->l90;index = l109->l91[l198];;;while(index!= (0xFFFF)){uint32 l12[
SOC_MAX_MEM_FIELD_WORDS];l94 l100;int l199;l199 = index;SOC_IF_ERROR_RETURN(
READ_L3_DEFIP_PAIR_128m(l1,MEM_BLOCK_ANY,l199,l12));l99(l1,l12,index,l100);if
((*l110)(entry,l100) == 0){*l112 = index;;return(SOC_E_NONE);}index = l109->
l92[index&(0x3FFF)];;};return(SOC_E_NOT_FOUND);}static int l113(l93*l109,l95
l110,l94 entry,int l111,uint16 l114,uint16 l115){int l1 = l109->l17;uint16
l198;uint16 index;uint16 l200;l198 = l101((uint8*)entry,(32*9))%l109->l90;
index = l109->l91[l198];;;;l200 = (0xFFFF);if(l114!= (0xFFFF)){while(index!= 
(0xFFFF)){uint32 l12[SOC_MAX_MEM_FIELD_WORDS];l94 l100;int l199;l199 = index;
SOC_IF_ERROR_RETURN(READ_L3_DEFIP_PAIR_128m(l1,MEM_BLOCK_ANY,l199,l12));l99(
l1,l12,index,l100);if((*l110)(entry,l100) == 0){if(l115!= index){;if(l200 == 
(0xFFFF)){l109->l91[l198] = l115;l109->l92[l115&(0x3FFF)] = l109->l92[index&(
0x3FFF)];l109->l92[index&(0x3FFF)] = (0xFFFF);}else{l109->l92[l200&(0x3FFF)] = 
l115;l109->l92[l115&(0x3FFF)] = l109->l92[index&(0x3FFF)];l109->l92[index&(
0x3FFF)] = (0xFFFF);}};return(SOC_E_NONE);}l200 = index;index = l109->l92[
index&(0x3FFF)];;}}l109->l92[l115&(0x3FFF)] = l109->l91[l198];l109->l91[l198]
= l115;return(SOC_E_NONE);}static int l116(l93*l109,l95 l110,l94 entry,int
l111,uint16 l117){uint16 l198;uint16 index;uint16 l200;l198 = l101((uint8*)
entry,(32*9))%l109->l90;index = l109->l91[l198];;;l200 = (0xFFFF);while(index
!= (0xFFFF)){if(l117 == index){;if(l200 == (0xFFFF)){l109->l91[l198] = l109->
l92[l117&(0x3FFF)];l109->l92[l117&(0x3FFF)] = (0xFFFF);}else{l109->l92[l200&(
0x3FFF)] = l109->l92[l117&(0x3FFF)];l109->l92[l117&(0x3FFF)] = (0xFFFF);}
return(SOC_E_NONE);}l200 = index;index = l109->l92[index&(0x3FFF)];;}return(
SOC_E_NOT_FOUND);}static int l201(int l1,void*l12){return(SOC_E_NONE);}void
soc_alpm_128_lpm_state_dump(int l1){int l119;int l202;l202 = ((3*(128+2+1))-1
);if(!soc_cm_debug_check(DK_L3|DK_SOCMEM|DK_VERBOSE)){return;}for(l119 = l202
;l119>= 0;l119--){if((l119!= ((3*(128+2+1))-1))&&((l39[(l1)][(l119)].l32) == 
-1)){continue;}soc_cm_debug(DK_L3|DK_SOCMEM|DK_VERBOSE,
"PFX = %d P = %d N = %d START = %d END = %d VENT = %d FENT = %d\n",l119,(l39[
(l1)][(l119)].l34),(l39[(l1)][(l119)].next),(l39[(l1)][(l119)].l32),(l39[(l1)
][(l119)].l33),(l39[(l1)][(l119)].l35),(l39[(l1)][(l119)].l36));}
COMPILER_REFERENCE(l201);}static int l203(int l1,int index,uint32*l12){int
l204;uint32 l205,l206,l207;uint32 l208;int l209;if(!SOC_URPF_STATUS_GET(l1)){
return(SOC_E_NONE);}if(soc_feature(l1,soc_feature_l3_defip_hole)){l204 = (
soc_mem_index_count(l1,L3_DEFIP_PAIR_128m)>>1);}else if(SOC_IS_APOLLO(l1)){
l204 = (soc_mem_index_count(l1,L3_DEFIP_PAIR_128m)>>1)+0x0400;}else{l204 = (
soc_mem_index_count(l1,L3_DEFIP_PAIR_128m)>>1);}if(((l87[(l1)]->l42)!= NULL))
{soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(
l12),(l87[(l1)]->l42),(0));}l205 = soc_meminfo_fieldinfo_field32_get((&
SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l12),(l87[(l1)]->l54));l208 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l12
),(l87[(l1)]->l55));l206 = soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(
l1,L3_DEFIP_PAIR_128m)),(l12),(l87[(l1)]->l52));l207 = 
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l12
),(l87[(l1)]->l53));l209 = ((!l205)&&(!l208)&&(!l206)&&(!l207))?1:0;
soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(l1,L3_DEFIP_PAIR_128m)),(l12
),(l87[(l1)]->l66),(l209));return l147(l1,MEM_BLOCK_ANY,index+l204,index,l12)
;}static int l210(int l1,int l211,int l212){uint32 l12[
SOC_MAX_MEM_FIELD_WORDS];SOC_IF_ERROR_RETURN(READ_L3_DEFIP_PAIR_128m(l1,
MEM_BLOCK_ANY,l211,l12));l193(l1,l12,l212,0x4000,0);SOC_IF_ERROR_RETURN(l147(
l1,MEM_BLOCK_ANY,l212,l211,l12));SOC_IF_ERROR_RETURN(l203(l1,l212,l12));do{
int l213,l214;l213 = soc_alpm_physical_idx((l1),L3_DEFIP_PAIR_128m,(l211),1);
l214 = soc_alpm_physical_idx((l1),L3_DEFIP_PAIR_128m,(l212),1);
ALPM_TCAM_PIVOT(l1,SOC_ALPM_128_ADDR_LWR((l214))<<1) = ALPM_TCAM_PIVOT(l1,
SOC_ALPM_128_ADDR_LWR((l213))<<1);ALPM_TCAM_PIVOT(l1,SOC_ALPM_128_ADDR_LWR((
l213))<<1) = NULL;}while(0);return(SOC_E_NONE);}static int l215(int l1,int
l111,int l7){int l211;int l212;l212 = (l39[(l1)][(l111)].l33)+1;l211 = (l39[(
l1)][(l111)].l32);if(l211!= l212){SOC_IF_ERROR_RETURN(l210(l1,l211,l212));}(
l39[(l1)][(l111)].l32)+= 1;(l39[(l1)][(l111)].l33)+= 1;return(SOC_E_NONE);}
static int l216(int l1,int l111,int l7){int l211;int l212;l212 = (l39[(l1)][(
l111)].l32)-1;if((l39[(l1)][(l111)].l35) == 0){(l39[(l1)][(l111)].l32) = l212
;(l39[(l1)][(l111)].l33) = l212-1;return(SOC_E_NONE);}l211 = (l39[(l1)][(l111
)].l33);SOC_IF_ERROR_RETURN(l210(l1,l211,l212));(l39[(l1)][(l111)].l32)-= 1;(
l39[(l1)][(l111)].l33)-= 1;return(SOC_E_NONE);}static int l217(int l1,int l111
,int l7,void*l12,int*l218){int l219;int l220;int l221;int l222;if((l39[(l1)][
(l111)].l35) == 0){l222 = ((3*(128+2+1))-1);if(soc_alpm_mode_get(l1) == 
SOC_ALPM_MODE_PARALLEL){if(l111<= (((3*(128+2+1))/3)-1)){l222 = (((3*(128+2+1
))/3)-1);}}while((l39[(l1)][(l222)].next)>l111){l222 = (l39[(l1)][(l222)].
next);}l220 = (l39[(l1)][(l222)].next);if(l220!= -1){(l39[(l1)][(l220)].l34) = 
l111;}(l39[(l1)][(l111)].next) = (l39[(l1)][(l222)].next);(l39[(l1)][(l111)].
l34) = l222;(l39[(l1)][(l222)].next) = l111;(l39[(l1)][(l111)].l36) = ((l39[(
l1)][(l222)].l36)+1)/2;(l39[(l1)][(l222)].l36)-= (l39[(l1)][(l111)].l36);(l39
[(l1)][(l111)].l32) = (l39[(l1)][(l222)].l33)+(l39[(l1)][(l222)].l36)+1;(l39[
(l1)][(l111)].l33) = (l39[(l1)][(l111)].l32)-1;(l39[(l1)][(l111)].l35) = 0;}
l221 = l111;while((l39[(l1)][(l221)].l36) == 0){l221 = (l39[(l1)][(l221)].
next);if(l221 == -1){l221 = l111;break;}}while((l39[(l1)][(l221)].l36) == 0){
l221 = (l39[(l1)][(l221)].l34);if(l221 == -1){if((l39[(l1)][(l111)].l35) == 0
){l219 = (l39[(l1)][(l111)].l34);l220 = (l39[(l1)][(l111)].next);if(-1!= l219
){(l39[(l1)][(l219)].next) = l220;}if(-1!= l220){(l39[(l1)][(l220)].l34) = 
l219;}}return(SOC_E_FULL);}}while(l221>l111){l220 = (l39[(l1)][(l221)].next);
SOC_IF_ERROR_RETURN(l216(l1,l220,l7));(l39[(l1)][(l221)].l36)-= 1;(l39[(l1)][
(l220)].l36)+= 1;l221 = l220;}while(l221<l111){SOC_IF_ERROR_RETURN(l215(l1,
l221,l7));(l39[(l1)][(l221)].l36)-= 1;l219 = (l39[(l1)][(l221)].l34);(l39[(l1
)][(l219)].l36)+= 1;l221 = l219;}(l39[(l1)][(l111)].l35)+= 1;(l39[(l1)][(l111
)].l36)-= 1;(l39[(l1)][(l111)].l33)+= 1;*l218 = (l39[(l1)][(l111)].l33);
sal_memcpy(l12,soc_mem_entry_null(l1,L3_DEFIP_PAIR_128m),soc_mem_entry_words(
l1,L3_DEFIP_PAIR_128m)*4);return(SOC_E_NONE);}static int l223(int l1,int l111
,int l7,void*l12,int l224){int l219;int l220;int l211;int l212;uint32 l225[
SOC_MAX_MEM_FIELD_WORDS];int l120;int l127;l211 = (l39[(l1)][(l111)].l33);
l212 = l224;(l39[(l1)][(l111)].l35)-= 1;(l39[(l1)][(l111)].l36)+= 1;(l39[(l1)
][(l111)].l33)-= 1;if(l212!= l211){if((l120 = READ_L3_DEFIP_PAIR_128m(l1,
MEM_BLOCK_ANY,l211,l225))<0){return l120;}l193(l1,l225,l212,0x4000,0);if((
l120 = l147(l1,MEM_BLOCK_ANY,l212,l211,l225))<0){return l120;}if((l120 = l203
(l1,l212,l225))<0){return l120;}}l127 = soc_alpm_physical_idx(l1,
L3_DEFIP_PAIR_128m,l212,1);l224 = soc_alpm_physical_idx(l1,L3_DEFIP_PAIR_128m
,l211,1);ALPM_TCAM_PIVOT(l1,SOC_ALPM_128_ADDR_LWR(l127)<<1) = ALPM_TCAM_PIVOT
(l1,SOC_ALPM_128_ADDR_LWR(l224)<<1);ALPM_TCAM_PIVOT(l1,SOC_ALPM_128_ADDR_LWR(
l224)<<1) = NULL;sal_memcpy(l225,soc_mem_entry_null(l1,L3_DEFIP_PAIR_128m),
soc_mem_entry_words(l1,L3_DEFIP_PAIR_128m)*4);l193(l1,l225,l211,0x4000,0);if(
(l120 = l147(l1,MEM_BLOCK_ANY,l211,l211,l225))<0){return l120;}if((l120 = 
l203(l1,l211,l225))<0){return l120;}if((l39[(l1)][(l111)].l35) == 0){l219 = (
l39[(l1)][(l111)].l34);assert(l219!= -1);l220 = (l39[(l1)][(l111)].next);(l39
[(l1)][(l219)].next) = l220;(l39[(l1)][(l219)].l36)+= (l39[(l1)][(l111)].l36)
;(l39[(l1)][(l111)].l36) = 0;if(l220!= -1){(l39[(l1)][(l220)].l34) = l219;}(
l39[(l1)][(l111)].next) = -1;(l39[(l1)][(l111)].l34) = -1;(l39[(l1)][(l111)].
l32) = -1;(l39[(l1)][(l111)].l33) = -1;}return(l120);}int
soc_alpm_128_lpm_vrf_get(int l17,void*lpm_entry,int*l24,int*l226){int l134;if
(((l87[(l17)]->l77)!= NULL)){l134 = soc_L3_DEFIP_PAIR_128m_field32_get(l17,
lpm_entry,VRF_ID_0_LWRf);*l226 = l134;if(soc_L3_DEFIP_PAIR_128m_field32_get(
l17,lpm_entry,VRF_ID_MASK0_LWRf)){*l24 = l134;}else if(!
soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(l17,L3_DEFIP_PAIR_128m)),(
lpm_entry),(l87[(l17)]->l79))){*l24 = SOC_L3_VRF_GLOBAL;*l226 = SOC_VRF_MAX(
l17)+1;}else{*l24 = SOC_L3_VRF_OVERRIDE;}}else{*l24 = SOC_L3_VRF_DEFAULT;}
return(SOC_E_NONE);}static int l227(int l1,void*entry,int*l14){int l111=0;int
l120;int l134;int l228;l120 = l118(l1,entry,&l111);if(l120<0){return l120;}
l111+= 0;SOC_IF_ERROR_RETURN(soc_alpm_128_lpm_vrf_get(l1,entry,&l134,&l120));
l228 = soc_alpm_mode_get(l1);switch(l134){case SOC_L3_VRF_GLOBAL:if(l228 == 
SOC_ALPM_MODE_PARALLEL){*l14 = l111+((3*(128+2+1))/3);}else{*l14 = l111;}
break;case SOC_L3_VRF_OVERRIDE:*l14 = l111+2*((3*(128+2+1))/3);break;default:
if(l228 == SOC_ALPM_MODE_PARALLEL){*l14 = l111;}else{*l14 = l111+((3*(128+2+1
))/3);}break;}return(SOC_E_NONE);}static int l11(int l1,void*l5,void*l12,int*
l13,int*l14,int*l7){int l120;int l112;int l111 = 0;*l7 = L3_DEFIP_MODE_128;
l227(l1,l5,&l111);*l14 = l111;if(l197(l1,l5,l111,&l112) == SOC_E_NONE){*l13 = 
l112;if((l120 = READ_L3_DEFIP_PAIR_128m(l1,MEM_BLOCK_ANY,(*l7)?*l13:(*l13>>1)
,l12))<0){return l120;}return(SOC_E_NONE);}else{return(SOC_E_NOT_FOUND);}}int
soc_alpm_128_lpm_init(int l1){int l202;int l119;int l229;int l230;uint32 l228
= 0;if(!soc_feature(l1,soc_feature_lpm_tcam)){return(SOC_E_UNAVAIL);}l202 = (
3*(128+2+1));l230 = sizeof(l37)*(l202);if((l39[(l1)]!= NULL)){
SOC_IF_ERROR_RETURN(soc_alpm_128_deinit(l1));}l87[l1] = sal_alloc(sizeof(l85)
,"lpm_128_field_state");if(NULL == l87[l1]){return(SOC_E_MEMORY);}(l87[l1])->
l41 = soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,CLASS_IDf);(l87[l1])->l42 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,DST_DISCARDf);(l87[l1])->l43 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,ECMPf);(l87[l1])->l44 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,ECMP_COUNTf);(l87[l1])->l45 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,ECMP_PTRf);(l87[l1])->l46 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,GLOBAL_ROUTEf);(l87[l1])->l47 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,HITf);(l87[l1])->l50 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,IP_ADDR0_LWRf);(l87[l1])->l48 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,IP_ADDR0_UPRf);(l87[l1])->l51 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,IP_ADDR1_LWRf);(l87[l1])->l49 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,IP_ADDR1_UPRf);(l87[l1])->l54 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,IP_ADDR_MASK0_LWRf);(l87[l1])->
l52 = soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,IP_ADDR_MASK0_UPRf);(l87[l1
])->l55 = soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,IP_ADDR_MASK1_LWRf);(
l87[l1])->l53 = soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,
IP_ADDR_MASK1_UPRf);(l87[l1])->l58 = soc_mem_fieldinfo_get(l1,
L3_DEFIP_PAIR_128m,MODE0_LWRf);(l87[l1])->l56 = soc_mem_fieldinfo_get(l1,
L3_DEFIP_PAIR_128m,MODE0_UPRf);(l87[l1])->l59 = soc_mem_fieldinfo_get(l1,
L3_DEFIP_PAIR_128m,MODE1_LWRf);(l87[l1])->l57 = soc_mem_fieldinfo_get(l1,
L3_DEFIP_PAIR_128m,MODE1_UPRf);(l87[l1])->l62 = soc_mem_fieldinfo_get(l1,
L3_DEFIP_PAIR_128m,MODE_MASK0_LWRf);(l87[l1])->l60 = soc_mem_fieldinfo_get(l1
,L3_DEFIP_PAIR_128m,MODE_MASK0_UPRf);(l87[l1])->l63 = soc_mem_fieldinfo_get(
l1,L3_DEFIP_PAIR_128m,MODE_MASK1_LWRf);(l87[l1])->l61 = soc_mem_fieldinfo_get
(l1,L3_DEFIP_PAIR_128m,MODE_MASK1_UPRf);(l87[l1])->l64 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,NEXT_HOP_INDEXf);(l87[l1])->l65 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,PRIf);(l87[l1])->l66 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,RPEf);(l87[l1])->l69 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,VALID0_LWRf);(l87[l1])->l67 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,VALID0_UPRf);(l87[l1])->l70 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,VALID1_LWRf);(l87[l1])->l68 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,VALID1_UPRf);(l87[l1])->l73 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,VRF_ID_0_LWRf);(l87[l1])->l71 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,VRF_ID_0_UPRf);(l87[l1])->l74 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,VRF_ID_1_LWRf);(l87[l1])->l72 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,VRF_ID_1_UPRf);(l87[l1])->l77 = 
soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,VRF_ID_MASK0_LWRf);(l87[l1])->l75
= soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,VRF_ID_MASK0_UPRf);(l87[l1])->
l78 = soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,VRF_ID_MASK1_LWRf);(l87[l1]
)->l76 = soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,VRF_ID_MASK1_UPRf);(l87[
l1])->l79 = soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,GLOBAL_HIGHf);(l87[l1
])->l80 = soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,ALG_HIT_IDXf);(l87[l1])
->l81 = soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,ALG_BKT_PTRf);(l87[l1])->
l82 = soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,DEFAULT_MISSf);(l87[l1])->
l83 = soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,FLEX_CTR_BASE_COUNTER_IDXf)
;(l87[l1])->l84 = soc_mem_fieldinfo_get(l1,L3_DEFIP_PAIR_128m,
FLEX_CTR_POOL_NUMBERf);(l39[(l1)]) = sal_alloc(l230,"LPM 128 prefix info");if
(NULL == (l39[(l1)])){sal_free(l87[l1]);l87[l1] = NULL;return(SOC_E_MEMORY);}
SOC_ALPM_LPM_LOCK(l1);sal_memset((l39[(l1)]),0,l230);for(l119 = 0;l119<l202;
l119++){(l39[(l1)][(l119)].l32) = -1;(l39[(l1)][(l119)].l33) = -1;(l39[(l1)][
(l119)].l34) = -1;(l39[(l1)][(l119)].next) = -1;(l39[(l1)][(l119)].l35) = 0;(
l39[(l1)][(l119)].l36) = 0;}l229 = soc_mem_index_count(l1,L3_DEFIP_PAIR_128m)
;if(SOC_URPF_STATUS_GET(l1)){l229>>= 1;}if(l228 == SOC_ALPM_MODE_PARALLEL){(
l39[(l1)][(((3*(128+2+1))-1))].l33) = (l229>>1)-1;(l39[(l1)][(((((3*(128+2+1)
)/3)-1)))].l36) = l229>>1;(l39[(l1)][((((3*(128+2+1))-1)))].l36) = (l229-(l39
[(l1)][(((((3*(128+2+1))/3)-1)))].l36));}else{(l39[(l1)][((((3*(128+2+1))-1))
)].l36) = l229;}if((l98[(l1)])!= NULL){if(l106((l98[(l1)]))<0){
SOC_ALPM_LPM_UNLOCK(l1);return SOC_E_INTERNAL;}(l98[(l1)]) = NULL;}if(l104(l1
,l229*2,l229,&(l98[(l1)]))<0){SOC_ALPM_LPM_UNLOCK(l1);return SOC_E_MEMORY;}
SOC_ALPM_LPM_UNLOCK(l1);return(SOC_E_NONE);}int soc_alpm_128_lpm_deinit(int l1
){if(!soc_feature(l1,soc_feature_lpm_tcam)){return(SOC_E_UNAVAIL);}
SOC_ALPM_LPM_LOCK(l1);if((l98[(l1)])!= NULL){l106((l98[(l1)]));(l98[(l1)]) = 
NULL;}if((l39[(l1)]!= NULL)){sal_free(l87[l1]);l87[l1] = NULL;sal_free((l39[(
l1)]));(l39[(l1)]) = NULL;}SOC_ALPM_LPM_UNLOCK(l1);return(SOC_E_NONE);}static
int l2(int l1,void*l3,int*l231){int l111;int index;int l7;uint32 l12[
SOC_MAX_MEM_FIELD_WORDS];int l120 = SOC_E_NONE;int l232 = 0;sal_memcpy(l12,
soc_mem_entry_null(l1,L3_DEFIP_PAIR_128m),soc_mem_entry_words(l1,
L3_DEFIP_PAIR_128m)*4);SOC_ALPM_LPM_LOCK(l1);l120 = l11(l1,l3,l12,&index,&
l111,&l7);if(l120 == SOC_E_NOT_FOUND){l120 = l217(l1,l111,l7,l12,&index);if(
l120<0){SOC_ALPM_LPM_UNLOCK(l1);return(l120);}}else{l232 = 1;}*l231 = index;
if(l120 == SOC_E_NONE){soc_alpm_128_lpm_state_dump(l1);soc_cm_debug(DK_L3|
DK_SOCMEM,"\nsoc_alpm_128_lpm_insert: %d %d\n",index,l111);if(!l232){l193(l1,
l3,index,0x4000,0);}l120 = l147(l1,MEM_BLOCK_ANY,index,index,l3);if(l120>= 0)
{l120 = l203(l1,index,l3);}}SOC_ALPM_LPM_UNLOCK(l1);return(l120);}static int
l4(int l1,void*l5){int l111;int index;int l7;uint32 l12[
SOC_MAX_MEM_FIELD_WORDS];int l120 = SOC_E_NONE;SOC_ALPM_LPM_LOCK(l1);l120 = 
l11(l1,l5,l12,&index,&l111,&l7);if(l120 == SOC_E_NONE){soc_cm_debug(DK_L3|
DK_SOCMEM,"\nsoc_alpm_lpm_delete: %d %d\n",index,l111);l196(l1,l5,index);l120
= l223(l1,l111,l7,l12,index);}soc_alpm_128_lpm_state_dump(l1);
SOC_ALPM_LPM_UNLOCK(l1);return(l120);}static int l15(int l1,void*l5,void*l12,
int*l13){int l111;int l120;int l7;SOC_ALPM_LPM_LOCK(l1);l120 = l11(l1,l5,l12,
l13,&l111,&l7);SOC_ALPM_LPM_UNLOCK(l1);return(l120);}static int l6(int l17,
void*l5,int l7,int l8,int l9,defip_aux_scratch_entry_t*l10){uint32 l122;
uint32 l233[4] = {0,0,0,0};int l111 = 0;int l120 = SOC_E_NONE;l122 = 
soc_mem_field32_get(l17,L3_DEFIP_PAIR_128m,l5,VALID0_LWRf);
soc_mem_field32_set(l17,L3_DEFIP_AUX_SCRATCHm,l10,VALIDf,l122);l122 = 
soc_mem_field32_get(l17,L3_DEFIP_PAIR_128m,l5,MODE0_LWRf);soc_mem_field32_set
(l17,L3_DEFIP_AUX_SCRATCHm,l10,MODEf,l122);l122 = soc_mem_field32_get(l17,
L3_DEFIP_PAIR_128m,l5,ENTRY_TYPE0_LWRf);soc_mem_field32_set(l17,
L3_DEFIP_AUX_SCRATCHm,l10,ENTRY_TYPEf,0);l122 = soc_mem_field32_get(l17,
L3_DEFIP_PAIR_128m,l5,GLOBAL_ROUTEf);soc_mem_field32_set(l17,
L3_DEFIP_AUX_SCRATCHm,l10,GLOBAL_ROUTEf,l122);l122 = soc_mem_field32_get(l17,
L3_DEFIP_PAIR_128m,l5,ECMPf);soc_mem_field32_set(l17,L3_DEFIP_AUX_SCRATCHm,
l10,ECMPf,l122);l122 = soc_mem_field32_get(l17,L3_DEFIP_PAIR_128m,l5,
ECMP_PTRf);soc_mem_field32_set(l17,L3_DEFIP_AUX_SCRATCHm,l10,ECMP_PTRf,l122);
l122 = soc_mem_field32_get(l17,L3_DEFIP_PAIR_128m,l5,NEXT_HOP_INDEXf);
soc_mem_field32_set(l17,L3_DEFIP_AUX_SCRATCHm,l10,NEXT_HOP_INDEXf,l122);l122 = 
soc_mem_field32_get(l17,L3_DEFIP_PAIR_128m,l5,PRIf);soc_mem_field32_set(l17,
L3_DEFIP_AUX_SCRATCHm,l10,PRIf,l122);l122 = soc_mem_field32_get(l17,
L3_DEFIP_PAIR_128m,l5,RPEf);soc_mem_field32_set(l17,L3_DEFIP_AUX_SCRATCHm,l10
,RPEf,l122);l122 =soc_mem_field32_get(l17,L3_DEFIP_PAIR_128m,l5,VRF_ID_0_LWRf
);soc_mem_field32_set(l17,L3_DEFIP_AUX_SCRATCHm,l10,VRFf,l122);
soc_mem_field32_set(l17,L3_DEFIP_AUX_SCRATCHm,l10,DB_TYPEf,l8);l122 = 
soc_mem_field32_get(l17,L3_DEFIP_PAIR_128m,l5,DST_DISCARDf);
soc_mem_field32_set(l17,L3_DEFIP_AUX_SCRATCHm,l10,DST_DISCARDf,l122);l122 = 
soc_mem_field32_get(l17,L3_DEFIP_PAIR_128m,l5,CLASS_IDf);soc_mem_field32_set(
l17,L3_DEFIP_AUX_SCRATCHm,l10,CLASS_IDf,l122);l233[0] = soc_mem_field32_get(
l17,L3_DEFIP_PAIR_128m,l5,IP_ADDR0_LWRf);l233[1] = soc_mem_field32_get(l17,
L3_DEFIP_PAIR_128m,l5,IP_ADDR1_LWRf);l233[2] = soc_mem_field32_get(l17,
L3_DEFIP_PAIR_128m,l5,IP_ADDR0_UPRf);l233[3] = soc_mem_field32_get(l17,
L3_DEFIP_PAIR_128m,l5,IP_ADDR1_UPRf);soc_mem_field_set(l17,
L3_DEFIP_AUX_SCRATCHm,(uint32*)l10,IP_ADDRf,(uint32*)l233);l120 = l118(l17,l5
,&l111);if(SOC_FAILURE(l120)){return l120;}soc_mem_field32_set(l17,
L3_DEFIP_AUX_SCRATCHm,l10,IP_LENGTHf,l111);soc_mem_field32_set(l17,
L3_DEFIP_AUX_SCRATCHm,l10,REPLACE_LENf,l9);return(SOC_E_NONE);}static int l16
(int l17,void*lpm_entry,void*l18,void*l19,soc_mem_t l20,uint32 l21,uint32*
l234){uint32 l122;uint32 l233[4];int l111 = 0;int l120 = SOC_E_NONE;uint32 l22
= 0;sal_memset(l18,0,soc_mem_entry_bytes(l17,l20));l122 = soc_mem_field32_get
(l17,L3_DEFIP_PAIR_128m,lpm_entry,HITf);soc_mem_field32_set(l17,l20,l18,HITf,
l122);l122 = soc_mem_field32_get(l17,L3_DEFIP_PAIR_128m,lpm_entry,VALID0_LWRf
);soc_mem_field32_set(l17,l20,l18,VALIDf,l122);l122 = soc_mem_field32_get(l17
,L3_DEFIP_PAIR_128m,lpm_entry,ECMPf);soc_mem_field32_set(l17,l20,l18,ECMPf,
l122);l122 = soc_mem_field32_get(l17,L3_DEFIP_PAIR_128m,lpm_entry,ECMP_PTRf);
soc_mem_field32_set(l17,l20,l18,ECMP_PTRf,l122);l122 = soc_mem_field32_get(
l17,L3_DEFIP_PAIR_128m,lpm_entry,NEXT_HOP_INDEXf);soc_mem_field32_set(l17,l20
,l18,NEXT_HOP_INDEXf,l122);l122 = soc_mem_field32_get(l17,L3_DEFIP_PAIR_128m,
lpm_entry,PRIf);soc_mem_field32_set(l17,l20,l18,PRIf,l122);l122 = 
soc_mem_field32_get(l17,L3_DEFIP_PAIR_128m,lpm_entry,RPEf);
soc_mem_field32_set(l17,l20,l18,RPEf,l122);l122 = soc_mem_field32_get(l17,
L3_DEFIP_PAIR_128m,lpm_entry,DST_DISCARDf);soc_mem_field32_set(l17,l20,l18,
DST_DISCARDf,l122);l122 = soc_mem_field32_get(l17,L3_DEFIP_PAIR_128m,
lpm_entry,SRC_DISCARDf);soc_mem_field32_set(l17,l20,l18,SRC_DISCARDf,l122);
l122 = soc_mem_field32_get(l17,L3_DEFIP_PAIR_128m,lpm_entry,CLASS_IDf);
soc_mem_field32_set(l17,l20,l18,CLASS_IDf,l122);l233[0] = soc_mem_field32_get
(l17,L3_DEFIP_PAIR_128m,lpm_entry,IP_ADDR0_LWRf);l233[1] = 
soc_mem_field32_get(l17,L3_DEFIP_PAIR_128m,lpm_entry,IP_ADDR1_LWRf);l233[2] = 
soc_mem_field32_get(l17,L3_DEFIP_PAIR_128m,lpm_entry,IP_ADDR0_UPRf);l233[3] = 
soc_mem_field32_get(l17,L3_DEFIP_PAIR_128m,lpm_entry,IP_ADDR1_UPRf);
soc_mem_field_set(l17,l20,(uint32*)l18,KEYf,(uint32*)l233);l120 = l118(l17,
lpm_entry,&l111);if(SOC_FAILURE(l120)){return l120;}if((l111 == 0)&&(l233[0]
== 0)&&(l233[1] == 0)&&(l233[2] == 0)&&(l233[3] == 0)){l22 = 1;}if(l234!= 
NULL){*l234 = l22;}soc_mem_field32_set(l17,l20,l18,LENGTHf,l111);if(l19 == 
NULL){return(SOC_E_NONE);}if(SOC_URPF_STATUS_GET(l17)){sal_memset(l19,0,
soc_mem_entry_bytes(l17,l20));sal_memcpy(l19,l18,soc_mem_entry_bytes(l17,l20)
);soc_mem_field32_set(l17,l20,l19,DST_DISCARDf,0);soc_mem_field32_set(l17,l20
,l19,RPEf,0);soc_mem_field32_set(l17,l20,l19,SRC_DISCARDf,l21&
SOC_ALPM_RPF_SRC_DISCARD);soc_mem_field32_set(l17,l20,l19,DEFAULTROUTEf,l22);
}return(SOC_E_NONE);}static int l23(int l17,void*l18,soc_mem_t l20,int l7,int
l24,int l25,int index,void*lpm_entry){uint32 l122;uint32 l233[4];uint32 l134,
l235;sal_memset(lpm_entry,0,soc_mem_entry_bytes(l17,L3_DEFIP_PAIR_128m));l122
= soc_mem_field32_get(l17,l20,l18,HITf);soc_mem_field32_set(l17,
L3_DEFIP_PAIR_128m,lpm_entry,HITf,l122);l122 = soc_mem_field32_get(l17,l20,
l18,VALIDf);soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,VALID0_LWRf,
l122);soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,VALID1_LWRf,l122);
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,VALID0_UPRf,l122);
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,VALID1_UPRf,l122);l122 = 
soc_mem_field32_get(l17,l20,l18,ECMPf);soc_mem_field32_set(l17,
L3_DEFIP_PAIR_128m,lpm_entry,ECMPf,l122);l122 = soc_mem_field32_get(l17,l20,
l18,ECMP_PTRf);soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,ECMP_PTRf
,l122);l122 = soc_mem_field32_get(l17,l20,l18,NEXT_HOP_INDEXf);
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,NEXT_HOP_INDEXf,l122);
l122 = soc_mem_field32_get(l17,l20,l18,PRIf);soc_mem_field32_set(l17,
L3_DEFIP_PAIR_128m,lpm_entry,PRIf,l122);l122 = soc_mem_field32_get(l17,l20,
l18,RPEf);soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,RPEf,l122);
l122 = soc_mem_field32_get(l17,l20,l18,DST_DISCARDf);soc_mem_field32_set(l17,
L3_DEFIP_PAIR_128m,lpm_entry,DST_DISCARDf,l122);l122 = soc_mem_field32_get(
l17,l20,l18,SRC_DISCARDf);soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,
lpm_entry,SRC_DISCARDf,l122);l122 = soc_mem_field32_get(l17,l20,l18,CLASS_IDf
);soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,CLASS_IDf,l122);
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
soc_mem_field_get(l17,l20,l18,KEYf,l233);soc_mem_field32_set(l17,
L3_DEFIP_PAIR_128m,lpm_entry,IP_ADDR0_LWRf,l233[0]);soc_mem_field32_set(l17,
L3_DEFIP_PAIR_128m,lpm_entry,IP_ADDR1_LWRf,l233[1]);soc_mem_field32_set(l17,
L3_DEFIP_PAIR_128m,lpm_entry,IP_ADDR0_UPRf,l233[2]);soc_mem_field32_set(l17,
L3_DEFIP_PAIR_128m,lpm_entry,IP_ADDR1_UPRf,l233[3]);l233[0] = l233[1] = l233[
2] = l233[3] = 0;l122 = soc_mem_field32_get(l17,l20,l18,LENGTHf);l128(l17,
lpm_entry,l122);if(l24 == SOC_L3_VRF_OVERRIDE){soc_mem_field32_set(l17,
L3_DEFIP_PAIR_128m,lpm_entry,GLOBAL_HIGHf,1);soc_mem_field32_set(l17,
L3_DEFIP_PAIR_128m,lpm_entry,GLOBAL_ROUTEf,1);l134 = 0;l235 = 0;}else if(l24
== SOC_L3_VRF_GLOBAL){soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,
GLOBAL_ROUTEf,1);l134 = 0;l235 = 0;}else{l134 = l24;l235 = SOC_VRF_MAX(l17);}
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,VRF_ID_0_LWRf,l134);
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,VRF_ID_1_LWRf,l134);
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,VRF_ID_0_UPRf,l134);
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,VRF_ID_1_UPRf,l134);
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,VRF_ID_MASK0_LWRf,l235);
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,VRF_ID_MASK1_LWRf,l235);
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,VRF_ID_MASK0_UPRf,l235);
soc_mem_field32_set(l17,L3_DEFIP_PAIR_128m,lpm_entry,VRF_ID_MASK1_UPRf,l235);
return(SOC_E_NONE);}
#endif
