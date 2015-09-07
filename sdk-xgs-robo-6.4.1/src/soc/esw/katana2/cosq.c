/*
 * $Id: cosq.c,v 1.3 Broadcom SDK $
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
 * MMU/Cosq soc routines
 *
 */

#include <sal/core/libc.h>
#include <shared/bsl.h>
#include <soc/debug.h>
#include <soc/util.h>
#include <soc/mem.h>
#include <soc/debug.h>

#if defined(BCM_KATANA2_SUPPORT)
#include <soc/katana2.h>

typedef struct lp_lls_reg_s {
    soc_reg_t   reg;
    soc_field_t field1;
    soc_field_t field2;
} lp_lls_reg_t;

lp_lls_reg_t lp_stream_map[] = {
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_001_000r, S1_MAPPING_DST_000f,
                                                        S1_MAPPING_DST_001f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_003_002r, S1_MAPPING_DST_002f,
                                                        S1_MAPPING_DST_003f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_005_004r, S1_MAPPING_DST_004f,
                                                        S1_MAPPING_DST_005f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_007_006r, S1_MAPPING_DST_006f,
                                                        S1_MAPPING_DST_007f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_009_008r, S1_MAPPING_DST_008f,
                                                        S1_MAPPING_DST_009f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_011_010r, S1_MAPPING_DST_010f,
                                                        S1_MAPPING_DST_011f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_013_012r, S1_MAPPING_DST_012f,
                                                        S1_MAPPING_DST_013f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_015_014r, S1_MAPPING_DST_014f,
                                                        S1_MAPPING_DST_015f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_017_016r, S1_MAPPING_DST_016f,
                                                        S1_MAPPING_DST_017f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_019_018r, S1_MAPPING_DST_018f,
                                                        S1_MAPPING_DST_019f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_021_020r, S1_MAPPING_DST_020f,
                                                        S1_MAPPING_DST_021f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_023_022r, S1_MAPPING_DST_022f,
                                                        S1_MAPPING_DST_023f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_025_024r, S1_MAPPING_DST_024f,
                                                        S1_MAPPING_DST_025f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_027_026r, S1_MAPPING_DST_026f,
                                                        S1_MAPPING_DST_027f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_029_028r, S1_MAPPING_DST_028f,
                                                        S1_MAPPING_DST_029f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_031_030r, S1_MAPPING_DST_030f,
                                                        S1_MAPPING_DST_031f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_033_032r, S1_MAPPING_DST_032f,
                                                        S1_MAPPING_DST_033f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_035_034r, S1_MAPPING_DST_034f,
                                                        S1_MAPPING_DST_035f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_037_036r, S1_MAPPING_DST_036f,
                                                        S1_MAPPING_DST_037f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_039_038r, S1_MAPPING_DST_038f,
                                                        S1_MAPPING_DST_039f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_041_040r, S1_MAPPING_DST_040f,
                                                        S1_MAPPING_DST_041f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_043_042r, S1_MAPPING_DST_042f,
                                                        S1_MAPPING_DST_043f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_045_044r, S1_MAPPING_DST_044f,
                                                        S1_MAPPING_DST_045f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_047_046r, S1_MAPPING_DST_046f,
                                                        S1_MAPPING_DST_047f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_049_048r, S1_MAPPING_DST_048f,
                                                        S1_MAPPING_DST_049f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_051_050r, S1_MAPPING_DST_050f,
                                                        S1_MAPPING_DST_051f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_053_052r, S1_MAPPING_DST_052f,
                                                        S1_MAPPING_DST_053f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_055_054r, S1_MAPPING_DST_054f,
                                                        S1_MAPPING_DST_055f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_057_056r, S1_MAPPING_DST_056f,
                                                        S1_MAPPING_DST_057f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_059_058r, S1_MAPPING_DST_058f,
                                                        S1_MAPPING_DST_059f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_061_060r, S1_MAPPING_DST_060f,
                                                        S1_MAPPING_DST_061f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_063_062r, S1_MAPPING_DST_062f,
                                                        S1_MAPPING_DST_063f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_065_064r, S1_MAPPING_DST_064f,
                                                        S1_MAPPING_DST_065f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_067_066r, S1_MAPPING_DST_066f,
                                                        S1_MAPPING_DST_067f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_069_068r, S1_MAPPING_DST_068f,
                                                        S1_MAPPING_DST_069f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_071_070r, S1_MAPPING_DST_070f,
                                                        S1_MAPPING_DST_071f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_073_072r, S1_MAPPING_DST_072f,
                                                        S1_MAPPING_DST_073f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_075_074r, S1_MAPPING_DST_074f,
                                                        S1_MAPPING_DST_075f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_077_076r, S1_MAPPING_DST_076f,
                                                        S1_MAPPING_DST_077f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_079_078r, S1_MAPPING_DST_078f,
                                                        S1_MAPPING_DST_079f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_081_080r, S1_MAPPING_DST_080f,
                                                        S1_MAPPING_DST_081f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_083_082r, S1_MAPPING_DST_082f,
                                                        S1_MAPPING_DST_083f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_085_084r, S1_MAPPING_DST_084f,
                                                        S1_MAPPING_DST_085f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_087_086r, S1_MAPPING_DST_086f,
                                                        S1_MAPPING_DST_087f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_089_088r, S1_MAPPING_DST_088f,
                                                        S1_MAPPING_DST_089f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_091_090r, S1_MAPPING_DST_090f,
                                                        S1_MAPPING_DST_091f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_093_092r, S1_MAPPING_DST_092f,
                                                        S1_MAPPING_DST_093f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_095_094r, S1_MAPPING_DST_094f,
                                                        S1_MAPPING_DST_095f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_097_096r, S1_MAPPING_DST_096f,
                                                        S1_MAPPING_DST_097f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_099_098r, S1_MAPPING_DST_098f,
                                                        S1_MAPPING_DST_099f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_101_100r, S1_MAPPING_DST_100f,
                                                        S1_MAPPING_DST_101f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_103_102r, S1_MAPPING_DST_102f,
                                                        S1_MAPPING_DST_103f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_105_104r, S1_MAPPING_DST_104f,
                                                        S1_MAPPING_DST_105f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_107_106r, S1_MAPPING_DST_106f,
                                                        S1_MAPPING_DST_107f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_109_108r, S1_MAPPING_DST_108f,
                                                        S1_MAPPING_DST_109f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_111_110r, S1_MAPPING_DST_110f,
                                                        S1_MAPPING_DST_111f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_113_112r, S1_MAPPING_DST_112f,
                                                        S1_MAPPING_DST_113f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_115_114r, S1_MAPPING_DST_114f,
                                                        S1_MAPPING_DST_115f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_117_116r, S1_MAPPING_DST_116f,
                                                        S1_MAPPING_DST_117f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_119_118r, S1_MAPPING_DST_118f,
                                                        S1_MAPPING_DST_119f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_121_120r, S1_MAPPING_DST_120f,
                                                        S1_MAPPING_DST_121f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_123_122r, S1_MAPPING_DST_122f,
                                                        S1_MAPPING_DST_123f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_125_124r, S1_MAPPING_DST_124f,
                                                        S1_MAPPING_DST_125f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_127_126r, S1_MAPPING_DST_126f,
                                                        S1_MAPPING_DST_127f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_129_128r, S1_MAPPING_DST_128f,
                                                        S1_MAPPING_DST_129f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_131_130r, S1_MAPPING_DST_130f,
                                                        S1_MAPPING_DST_131f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_133_132r, S1_MAPPING_DST_132f,
                                                        S1_MAPPING_DST_133f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_135_134r, S1_MAPPING_DST_134f,
                                                        S1_MAPPING_DST_135f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_137_136r, S1_MAPPING_DST_136f,
                                                        S1_MAPPING_DST_137f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_139_138r, S1_MAPPING_DST_138f,
                                                        S1_MAPPING_DST_139f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_141_140r, S1_MAPPING_DST_140f,
                                                        S1_MAPPING_DST_141f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_143_142r, S1_MAPPING_DST_142f,
                                                        S1_MAPPING_DST_143f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_145_144r, S1_MAPPING_DST_144f,
                                                        S1_MAPPING_DST_145f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_147_146r, S1_MAPPING_DST_146f,
                                                        S1_MAPPING_DST_147f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_149_148r, S1_MAPPING_DST_148f,
                                                        S1_MAPPING_DST_149f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_151_150r, S1_MAPPING_DST_150f,
                                                        S1_MAPPING_DST_151f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_153_152r, S1_MAPPING_DST_152f,
                                                        S1_MAPPING_DST_153f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_155_154r, S1_MAPPING_DST_154f,
                                                        S1_MAPPING_DST_155f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_157_156r, S1_MAPPING_DST_156f,
                                                        S1_MAPPING_DST_157f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_159_158r, S1_MAPPING_DST_158f,
                                                        S1_MAPPING_DST_159f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_161_160r, S1_MAPPING_DST_160f,
                                                        S1_MAPPING_DST_161f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_163_162r, S1_MAPPING_DST_162f,
                                                        S1_MAPPING_DST_163f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_165_164r, S1_MAPPING_DST_164f,
                                                        S1_MAPPING_DST_165f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_167_166r, S1_MAPPING_DST_166f,
                                                        S1_MAPPING_DST_167f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_169_168r, S1_MAPPING_DST_168f,
                                                        S1_MAPPING_DST_169f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_171_170r, S1_MAPPING_DST_170f,
                                                        S1_MAPPING_DST_171f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_173_172r, S1_MAPPING_DST_172f,
                                                        S1_MAPPING_DST_173f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_175_174r, S1_MAPPING_DST_174f,
                                                        S1_MAPPING_DST_175f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_177_176r, S1_MAPPING_DST_176f,
                                                        S1_MAPPING_DST_177f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_179_178r, S1_MAPPING_DST_178f,
                                                        S1_MAPPING_DST_179f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_181_180r, S1_MAPPING_DST_180f,
                                                        S1_MAPPING_DST_181f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_183_182r, S1_MAPPING_DST_182f,
                                                        S1_MAPPING_DST_183f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_185_184r, S1_MAPPING_DST_184f,
                                                        S1_MAPPING_DST_185f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_187_186r, S1_MAPPING_DST_186f,
                                                        S1_MAPPING_DST_187f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_189_188r, S1_MAPPING_DST_188f,
                                                        S1_MAPPING_DST_189f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_191_190r, S1_MAPPING_DST_190f,
                                                        S1_MAPPING_DST_191f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_193_192r, S1_MAPPING_DST_192f,
                                                        S1_MAPPING_DST_193f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_195_194r, S1_MAPPING_DST_194f,
                                                        S1_MAPPING_DST_195f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_197_196r, S1_MAPPING_DST_196f,
                                                        S1_MAPPING_DST_197f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_199_198r, S1_MAPPING_DST_198f,
                                                        S1_MAPPING_DST_199f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_201_200r, S1_MAPPING_DST_200f,
                                                        S1_MAPPING_DST_201f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_203_202r, S1_MAPPING_DST_202f,
                                                        S1_MAPPING_DST_203f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_205_204r, S1_MAPPING_DST_204f,
                                                        S1_MAPPING_DST_205f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_207_206r, S1_MAPPING_DST_206f,
                                                        S1_MAPPING_DST_207f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_209_208r, S1_MAPPING_DST_208f,
                                                        S1_MAPPING_DST_209f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_211_210r, S1_MAPPING_DST_210f,
                                                        S1_MAPPING_DST_211f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_213_212r, S1_MAPPING_DST_212f,
                                                        S1_MAPPING_DST_213f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_215_214r, S1_MAPPING_DST_214f,
                                                        S1_MAPPING_DST_215f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_217_216r, S1_MAPPING_DST_216f,
                                                        S1_MAPPING_DST_217f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_219_218r, S1_MAPPING_DST_218f,
                                                        S1_MAPPING_DST_219f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_221_220r, S1_MAPPING_DST_220f,
                                                        S1_MAPPING_DST_221f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_223_222r, S1_MAPPING_DST_222f,
                                                        S1_MAPPING_DST_223f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_225_224r, S1_MAPPING_DST_224f,
                                                        S1_MAPPING_DST_225f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_227_226r, S1_MAPPING_DST_226f,
                                                        S1_MAPPING_DST_227f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_229_228r, S1_MAPPING_DST_228f,
                                                        S1_MAPPING_DST_229f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_231_230r, S1_MAPPING_DST_230f,
                                                        S1_MAPPING_DST_231f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_233_232r, S1_MAPPING_DST_232f,
                                                        S1_MAPPING_DST_233f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_235_234r, S1_MAPPING_DST_234f,
                                                        S1_MAPPING_DST_235f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_237_236r, S1_MAPPING_DST_236f,
                                                        S1_MAPPING_DST_237f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_239_238r, S1_MAPPING_DST_238f,
                                                        S1_MAPPING_DST_239f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_241_240r, S1_MAPPING_DST_240f,
                                                        S1_MAPPING_DST_241f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_243_242r, S1_MAPPING_DST_242f,
                                                        S1_MAPPING_DST_243f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_245_244r, S1_MAPPING_DST_244f,
                                                        S1_MAPPING_DST_245f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_247_246r, S1_MAPPING_DST_246f,
                                                        S1_MAPPING_DST_247f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_249_248r, S1_MAPPING_DST_248f,
                                                        S1_MAPPING_DST_249f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_251_250r, S1_MAPPING_DST_250f,
                                                        S1_MAPPING_DST_251f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_253_252r, S1_MAPPING_DST_252f,
                                                        S1_MAPPING_DST_253f },
{ LLS_LINKPHY_CONFIG_DST_STREAM_TO_S1_MAPPING_255_254r, S1_MAPPING_DST_254f,
                                                        S1_MAPPING_DST_255f },
};

lp_lls_reg_t port_lp_enable[] = {
    { INVALIDr, INVALIDf, INVALIDf },
    { LLS_PORT_COE_LINKPHY_ENABLE_LOWERr, LINKPHY_ENABLE_01f, INVALIDf },
    { LLS_PORT_COE_LINKPHY_ENABLE_LOWERr, LINKPHY_ENABLE_02f, INVALIDf },
    { LLS_PORT_COE_LINKPHY_ENABLE_LOWERr, LINKPHY_ENABLE_03f, INVALIDf },
    { LLS_PORT_COE_LINKPHY_ENABLE_LOWERr, LINKPHY_ENABLE_04f, INVALIDf },
    { LLS_PORT_COE_LINKPHY_ENABLE_LOWERr, LINKPHY_ENABLE_05f, INVALIDf },
    { LLS_PORT_COE_LINKPHY_ENABLE_LOWERr, LINKPHY_ENABLE_06f, INVALIDf },
    { LLS_PORT_COE_LINKPHY_ENABLE_LOWERr, LINKPHY_ENABLE_07f, INVALIDf },
    { LLS_PORT_COE_LINKPHY_ENABLE_LOWERr, LINKPHY_ENABLE_08f, INVALIDf },
    { LLS_PORT_COE_LINKPHY_ENABLE_LOWERr, LINKPHY_ENABLE_09f, INVALIDf },
    { LLS_PORT_COE_LINKPHY_ENABLE_LOWERr, LINKPHY_ENABLE_10f, INVALIDf },
    { LLS_PORT_COE_LINKPHY_ENABLE_LOWERr, LINKPHY_ENABLE_11f, INVALIDf },
    { LLS_PORT_COE_LINKPHY_ENABLE_LOWERr, LINKPHY_ENABLE_12f, INVALIDf },
    { LLS_PORT_COE_LINKPHY_ENABLE_LOWERr, LINKPHY_ENABLE_13f, INVALIDf },
    { LLS_PORT_COE_LINKPHY_ENABLE_LOWERr, LINKPHY_ENABLE_14f, INVALIDf },
    { LLS_PORT_COE_LINKPHY_ENABLE_LOWERr, LINKPHY_ENABLE_15f, INVALIDf },
    { LLS_PORT_COE_LINKPHY_ENABLE_LOWERr, LINKPHY_ENABLE_16f, INVALIDf },
    { LLS_PORT_COE_LINKPHY_ENABLE_LOWERr, LINKPHY_ENABLE_17f, INVALIDf },
    { LLS_PORT_COE_LINKPHY_ENABLE_LOWERr, LINKPHY_ENABLE_18f, INVALIDf },
    { LLS_PORT_COE_LINKPHY_ENABLE_LOWERr, LINKPHY_ENABLE_19f, INVALIDf },
    { LLS_PORT_COE_LINKPHY_ENABLE_LOWERr, LINKPHY_ENABLE_20f, INVALIDf },
    { LLS_PORT_COE_LINKPHY_ENABLE_LOWERr, LINKPHY_ENABLE_21f, INVALIDf },
    { LLS_PORT_COE_LINKPHY_ENABLE_LOWERr, LINKPHY_ENABLE_22f, INVALIDf },
    { LLS_PORT_COE_LINKPHY_ENABLE_LOWERr, LINKPHY_ENABLE_23f, INVALIDf },
    { LLS_PORT_COE_LINKPHY_ENABLE_LOWERr, LINKPHY_ENABLE_24f, INVALIDf },
    { LLS_PORT_COE_LINKPHY_ENABLE_LOWERr, LINKPHY_ENABLE_25f, INVALIDf },
    { LLS_PORT_COE_LINKPHY_ENABLE_LOWERr, LINKPHY_ENABLE_26f, INVALIDf },
    { LLS_PORT_COE_LINKPHY_ENABLE_LOWERr, LINKPHY_ENABLE_27f, INVALIDf },
    { LLS_PORT_COE_LINKPHY_ENABLE_LOWERr, LINKPHY_ENABLE_28f, INVALIDf },
    { LLS_PORT_COE_LINKPHY_ENABLE_LOWERr, LINKPHY_ENABLE_29f, INVALIDf },
    { LLS_PORT_COE_LINKPHY_ENABLE_LOWERr, LINKPHY_ENABLE_30f, INVALIDf },
    { LLS_PORT_COE_LINKPHY_ENABLE_LOWERr, LINKPHY_ENABLE_31f, INVALIDf },
    { LLS_PORT_COE_LINKPHY_ENABLE_UPPERr, LINKPHY_ENABLE_32f, INVALIDf }, 
    { LLS_PORT_COE_LINKPHY_ENABLE_UPPERr, LINKPHY_ENABLE_33f, INVALIDf },
    { LLS_PORT_COE_LINKPHY_ENABLE_UPPERr, LINKPHY_ENABLE_34f, INVALIDf },
    { LLS_PORT_COE_LINKPHY_ENABLE_UPPERr, LINKPHY_ENABLE_35f, INVALIDf },
    { LLS_PORT_COE_LINKPHY_ENABLE_UPPERr, LINKPHY_ENABLE_36f, INVALIDf },
    { LLS_PORT_COE_LINKPHY_ENABLE_UPPERr, LINKPHY_ENABLE_37f, INVALIDf }, 
    { LLS_PORT_COE_LINKPHY_ENABLE_UPPERr, LINKPHY_ENABLE_38f, INVALIDf },
    { LLS_PORT_COE_LINKPHY_ENABLE_UPPERr, LINKPHY_ENABLE_39f, INVALIDf },
    { LLS_PORT_COE_LINKPHY_ENABLE_UPPERr, LINKPHY_ENABLE_40f, INVALIDf },    
};

lp_lls_reg_t port_lp_config[] = {
    { INVALIDr, INVALIDf, INVALIDf },    
    { LLS_PORT_01_LINKPHY_CONFIGr, START_S1f, END_S1f },
    { LLS_PORT_02_LINKPHY_CONFIGr, START_S1f, END_S1f },
    { LLS_PORT_03_LINKPHY_CONFIGr, START_S1f, END_S1f },
    { LLS_PORT_04_LINKPHY_CONFIGr, START_S1f, END_S1f },
    { LLS_PORT_05_LINKPHY_CONFIGr, START_S1f, END_S1f },
    { LLS_PORT_06_LINKPHY_CONFIGr, START_S1f, END_S1f },
    { LLS_PORT_07_LINKPHY_CONFIGr, START_S1f, END_S1f },
    { LLS_PORT_08_LINKPHY_CONFIGr, START_S1f, END_S1f },
    { LLS_PORT_09_LINKPHY_CONFIGr, START_S1f, END_S1f },
    { LLS_PORT_10_LINKPHY_CONFIGr, START_S1f, END_S1f },
    { LLS_PORT_11_LINKPHY_CONFIGr, START_S1f, END_S1f },
    { LLS_PORT_12_LINKPHY_CONFIGr, START_S1f, END_S1f },
    { LLS_PORT_13_LINKPHY_CONFIGr, START_S1f, END_S1f },
    { LLS_PORT_14_LINKPHY_CONFIGr, START_S1f, END_S1f },
    { LLS_PORT_15_LINKPHY_CONFIGr, START_S1f, END_S1f },
    { LLS_PORT_16_LINKPHY_CONFIGr, START_S1f, END_S1f },
    { LLS_PORT_17_LINKPHY_CONFIGr, START_S1f, END_S1f },
    { LLS_PORT_18_LINKPHY_CONFIGr, START_S1f, END_S1f },
    { LLS_PORT_19_LINKPHY_CONFIGr, START_S1f, END_S1f },
    { LLS_PORT_20_LINKPHY_CONFIGr, START_S1f, END_S1f },
    { LLS_PORT_21_LINKPHY_CONFIGr, START_S1f, END_S1f },
    { LLS_PORT_22_LINKPHY_CONFIGr, START_S1f, END_S1f },
    { LLS_PORT_23_LINKPHY_CONFIGr, START_S1f, END_S1f },
    { LLS_PORT_24_LINKPHY_CONFIGr, START_S1f, END_S1f },
    { LLS_PORT_25_LINKPHY_CONFIGr, START_S1f, END_S1f },
    { LLS_PORT_26_LINKPHY_CONFIGr, START_S1f, END_S1f },
    { LLS_PORT_27_LINKPHY_CONFIGr, START_S1f, END_S1f },
    { LLS_PORT_28_LINKPHY_CONFIGr, START_S1f, END_S1f },
    { LLS_PORT_29_LINKPHY_CONFIGr, START_S1f, END_S1f },
    { LLS_PORT_30_LINKPHY_CONFIGr, START_S1f, END_S1f },
    { LLS_PORT_31_LINKPHY_CONFIGr, START_S1f, END_S1f },
    { LLS_PORT_32_LINKPHY_CONFIGr, START_S1f, END_S1f },    
    { LLS_PORT_33_LINKPHY_CONFIGr, START_S1f, END_S1f },
    { LLS_PORT_34_LINKPHY_CONFIGr, START_S1f, END_S1f },
    { LLS_PORT_35_LINKPHY_CONFIGr, START_S1f, END_S1f },
    { LLS_PORT_36_LINKPHY_CONFIGr, START_S1f, END_S1f },
    { LLS_PORT_37_LINKPHY_CONFIGr, START_S1f, END_S1f },
    { LLS_PORT_38_LINKPHY_CONFIGr, START_S1f, END_S1f },
    { LLS_PORT_39_LINKPHY_CONFIGr, START_S1f, END_S1f },
    { LLS_PORT_40_LINKPHY_CONFIGr, START_S1f, END_S1f },
};

int
soc_kt2_cosq_stream_mapping_set(int unit)
{
    uint32 i, rval;
    lp_lls_reg_t *map_reg;
    soc_reg_t sched_regs[] = {
                           LLS_LINKPHY_CONFIG_SEL_SPRI_S1_IN_S0_31_0r,
                           LLS_LINKPHY_CONFIG_SEL_SPRI_S1_IN_S0_63_32r,
                           LLS_LINKPHY_CONFIG_SEL_SPRI_S1_IN_S0_95_64r,
                           LLS_LINKPHY_CONFIG_SEL_SPRI_S1_IN_S0_127_96r
                          };
    soc_field_t sched_fields[] = {
                               SEL_SPRI_S1_IN_S0_31_0f,
                               SEL_SPRI_S1_IN_S0_63_32f,
                               SEL_SPRI_S1_IN_S0_95_64f,
                               SEL_SPRI_S1_IN_S0_127_96f
                              };
    
    map_reg = lp_stream_map;
    for (i = 0; i < 128; i++) {
        rval = 0;
        soc_reg_field_set(unit, map_reg[i].reg, &rval, 
                          map_reg[i].field1, (i * 2));
        soc_reg_field_set(unit, map_reg[i].reg, &rval, 
                          map_reg[i].field2, ((i * 2) + 1));
        SOC_IF_ERROR_RETURN
            (soc_reg32_set(unit, map_reg[i].reg, REG_PORT_ANY, 0, rval));
    }
 
    /* set the streams in spri by default */    
    for (i = 0; i < 4; i++) {
        SOC_IF_ERROR_RETURN
            (soc_reg32_get(unit,sched_regs[i], REG_PORT_ANY, 0, &rval));
        soc_reg_field_set(unit, sched_regs[i], &rval, sched_fields[i],
                          0xFFFFFFFF);
        SOC_IF_ERROR_RETURN
            (soc_reg32_set(unit, sched_regs[i], REG_PORT_ANY, 0, rval));
    }
    
    return SOC_E_NONE;
}

int
soc_kt2_cosq_s0_sched_set(int unit, int hw_index, int mode)
{
    int reg_id;
    uint32 rval, sched_mode;
    soc_reg_t sched_regs[] = {
                           LLS_LINKPHY_CONFIG_SEL_SPRI_S1_IN_S0_31_0r,
                           LLS_LINKPHY_CONFIG_SEL_SPRI_S1_IN_S0_63_32r,
                           LLS_LINKPHY_CONFIG_SEL_SPRI_S1_IN_S0_95_64r,
                           LLS_LINKPHY_CONFIG_SEL_SPRI_S1_IN_S0_127_96r
                          };
    soc_field_t sched_fields[] = {
                               SEL_SPRI_S1_IN_S0_31_0f,
                               SEL_SPRI_S1_IN_S0_63_32f,
                               SEL_SPRI_S1_IN_S0_95_64f,
                               SEL_SPRI_S1_IN_S0_127_96f
                              };
    reg_id = hw_index / 32;

    if (reg_id > 3) {
        return SOC_E_PARAM;
    }   

    SOC_IF_ERROR_RETURN
        (soc_reg32_get(unit, sched_regs[reg_id], REG_PORT_ANY, 0, &rval));
    sched_mode = soc_reg_field_get(unit, sched_regs[reg_id], rval,
                                   sched_fields[reg_id]);
    if (mode) {
        sched_mode |= (1 << (hw_index % 32));
    } else {
        sched_mode &= ~(1 << (hw_index % 32));
    }
    soc_reg_field_set(unit, sched_regs[reg_id], &rval, sched_fields[reg_id],
                      sched_mode);
    SOC_IF_ERROR_RETURN
        (soc_reg32_set(unit, sched_regs[reg_id], REG_PORT_ANY, 0, rval));

    return SOC_E_NONE;
}

int 
soc_kt2_cosq_port_coe_linkphy_status_set(int unit, soc_port_t port, int enable)
{
    uint32 rval;
    lp_lls_reg_t *port_reg;

    port_reg = port_lp_enable;
    if (port_reg[port].reg != INVALIDr) {
        SOC_IF_ERROR_RETURN
            (soc_reg32_get(unit, port_reg[port].reg, REG_PORT_ANY, 0, &rval));
        soc_reg_field_set(unit, port_reg[port].reg, &rval, 
                          port_reg[port].field1, enable ? 1 : 0);
        SOC_IF_ERROR_RETURN
            (soc_reg32_set(unit, port_reg[port].reg, REG_PORT_ANY, 0, rval));
    } else {
        return SOC_E_PARAM;
    }
    
    return SOC_E_NONE;
}

int
soc_kt2_cosq_s1_range_set(int unit, soc_port_t port,
                          int start_s1, int end_s1, int type)
{
    uint32 rval = 0;
    lp_lls_reg_t *port_reg;

    port_reg = port_lp_config;
    if (port_reg[port].reg != INVALIDr) {
        soc_reg_field_set(unit, port_reg[port].reg, &rval, 
                          port_reg[port].field1, start_s1);
        soc_reg_field_set(unit, port_reg[port].reg, &rval, 
                          port_reg[port].field2, end_s1);        
        if (SOC_REG_FIELD_VALID(unit, port_reg[port].reg, LINKPHY_ENABLEDf)) {
            soc_reg_field_set(unit, port_reg[port].reg, &rval, 
                              LINKPHY_ENABLEDf, type);
        }
        SOC_IF_ERROR_RETURN
            (soc_reg32_set(unit, port_reg[port].reg, REG_PORT_ANY, 0, rval));
    } else {
        return SOC_E_PARAM;
    }
    
    return SOC_E_NONE;
}

int
soc_kt2_cosq_repl_map_set(int unit, soc_port_t port,
                          int start, int end, int enable)
{
    mmu_repl_map_tbl_entry_t map_entry;  
    uint32 pp_port;
    int i;
   
    for (i = start; i <= end; i++) {
        sal_memset(&map_entry, 0, sizeof(mmu_repl_map_tbl_entry_t));
        if (enable) {
            pp_port = (i + KT2_MIN_SUBPORT_INDEX);
            soc_MMU_REPL_MAP_TBLm_field_set(unit, &map_entry,
                                        DEST_PPPf, &pp_port); 
        }
        SOC_IF_ERROR_RETURN(WRITE_MMU_REPL_MAP_TBLm(unit,
                                   MEM_BLOCK_ALL, i, &map_entry));
    }

    return SOC_E_NONE;
}

int
soc_kt2_sched_get_node_config(int unit, soc_port_t port, int level, int index,
                              int *pnum_spri, int *pfirst_child,
                              int *pfirst_mc_child, uint32 *pucmap, 
                              uint32 *pspmap)
{
    uint32 entry[SOC_MAX_MEM_WORDS];
    soc_mem_t mem;
    uint32 num_spri = 0, ucmap = 0, f1, f2;
    int first_child = -1, first_mc_child = -1, ii;
    int sp_vec = soc_feature(unit, soc_feature_vector_based_spri);

    *pspmap = 0;
    
    mem = _SOC_KT2_NODE_CONFIG_MEM(level);
    if (mem == INVALIDm) {
        return SOC_E_INTERNAL;
    }
    SOC_IF_ERROR_RETURN(soc_mem_read(unit, mem, MEM_BLOCK_ALL, index, &entry));
    if (sp_vec) {
        f1 = soc_mem_field32_get(unit, mem, &entry, P_NUM_SPRIf);
        f2 = soc_mem_field32_get(unit, mem, &entry, P_VECT_SPRI_7_4f);
        *pspmap = f1 | (f2 << 4);
        for (ii = 0; ii < 32; ii++) {
            if ((1 << ii) & *pspmap) {
                num_spri++;
            }
        }
    } else {
        num_spri = soc_mem_field32_get(unit, mem, &entry, P_NUM_SPRIf);
    }

    if (mem == LLS_L1_CONFIGm) {
        first_child = soc_mem_field32_get(unit, mem, &entry, P_START_UC_SPRIf);
        first_mc_child = soc_mem_field32_get(unit, mem, 
                                         &entry, P_START_MC_SPRIf) + 1024;
        ucmap = soc_mem_field32_get(unit, mem, &entry, P_SPRI_SELECTf);
    } else {
        first_child = soc_mem_field32_get(unit, mem, &entry, P_START_SPRIf);
        first_mc_child = 0;
    }

    if (num_spri == 0) {
        ucmap = 0;
    }

    if (pnum_spri) {
        *pnum_spri = num_spri;
    }
       
    if (pucmap) {
        *pucmap = ucmap;
    }

    if (pfirst_child) {
        *pfirst_child = first_child;
    }

    if (pfirst_mc_child) {
        *pfirst_mc_child = first_mc_child;
    }
    return SOC_E_NONE;
}

int 
soc_kt2_sched_weight_get(int unit, int level, int index, int *weight)
{
    soc_mem_t mem_weight;
    uint32 entry[SOC_MAX_MEM_WORDS];

    mem_weight = _SOC_KT2_NODE_WIEGHT_MEM(level);

    SOC_IF_ERROR_RETURN
        (soc_mem_read(unit, mem_weight, MEM_BLOCK_ALL, index, &entry));

    *weight = soc_mem_field32_get(unit, mem_weight, &entry, C_WEIGHTf);

    LOG_INFO(BSL_LS_SOC_COSQ,
             (BSL_META_U(unit,
                         "sched_weight_get L%d index=%d wt=%d\n"),
              level, index, *weight));
    return SOC_E_NONE;
}

int
soc_kt2_cosq_get_sched_mode(int unit, soc_port_t port, int level, int index,
                              soc_kt2_sched_mode_e *pmode, int *weight)
{
    uint32 entry[SOC_MAX_MEM_WORDS];
    soc_mem_t mem;
    soc_kt2_sched_mode_e mode = SOC_KT2_SCHED_MODE_UNKNOWN;

    SOC_IF_ERROR_RETURN(soc_kt2_sched_weight_get(unit, level, index, weight));

    if (*weight == 0) {
        mode = SOC_KT2_SCHED_MODE_STRICT;
    } else {
        mem = _SOC_KT2_NODE_CONFIG_MEM(SOC_KT2_NODE_LVL_ROOT);
        index = port;
        SOC_IF_ERROR_RETURN(
                soc_mem_read(unit, mem, MEM_BLOCK_ALL, index, &entry));
        if (soc_mem_field32_get(unit, mem, entry, PACKET_MODE_WRR_ACCOUNTING_ENABLEf)) {
            mode = SOC_KT2_SCHED_MODE_WRR;
        } else {
            mode = SOC_KT2_SCHED_MODE_WDRR;
        }
    }

    if (pmode) {
        *pmode = mode;
    }
        
    return SOC_E_NONE;
}

int soc_kt2_get_child_type(int unit, soc_port_t port, int level, 
                                    int *child_type)
{
    *child_type = -1;

    if (level == SOC_KT2_NODE_LVL_ROOT) {
        *child_type = SOC_KT2_NODE_LVL_L0;
    } else if (level == SOC_KT2_NODE_LVL_L0) {
        *child_type = SOC_KT2_NODE_LVL_L1;
    } else if (level == SOC_KT2_NODE_LVL_L1) {
        *child_type = SOC_KT2_NODE_LVL_L2;
    }
    return SOC_E_NONE;
}

int _soc_kt2_invalid_parent_index(int unit, int level)
{
    int index_max = 0;

    switch (level) {
        case SOC_KT2_NODE_LVL_ROOT:
            index_max = 0;
            break;
        case SOC_KT2_NODE_LVL_L0:
            index_max = soc_mem_index_max(unit, LLS_L0_CONFIGm);
            break;
        case SOC_KT2_NODE_LVL_L1:
            index_max = soc_mem_index_max(unit, LLS_L0_PARENTm);
            break;
        case SOC_KT2_NODE_LVL_L2:
            index_max = soc_mem_index_max(unit, LLS_L1_PARENTm);
            break;
        default:
            break;

    }

    return index_max;
}

#define INVALID_PARENT(unit, level)   _soc_kt2_invalid_parent_index((unit),(level))


STATIC int 
_soc_kt2_dump_sched_at(int unit, int port, int level, int offset, int hw_index)
{
    int num_spri, first_child, first_mc_child, rv, cindex;
    uint32 ucmap, spmap;
    soc_kt2_sched_mode_e sched_mode;
    soc_mem_t mem;
    int index_max, ii, ci, child_level, wt = 0, num_child;
    uint32 entry[SOC_MAX_MEM_WORDS];
    char *lvl_name[] = { "Root", "L0", "L1", "L2" };
    char *sched_modes[] = {"X", "SP", "WRR", "WDRR"};

    if ((level > SOC_KT2_NODE_LVL_L0) && (hw_index == INVALID_PARENT(unit, level))) {
        return SOC_E_NONE;
    }

    /* get sched config */
    SOC_IF_ERROR_RETURN(
            soc_kt2_sched_get_node_config(unit, port, level, hw_index, 
                       &num_spri, &first_child, &first_mc_child, &ucmap, &spmap));

    sched_mode = 0;
    if (level != SOC_KT2_NODE_LVL_ROOT) {
        SOC_IF_ERROR_RETURN(
          soc_kt2_cosq_get_sched_mode(unit, port, level, hw_index, &sched_mode, &wt));
    }
    
    if (level == SOC_KT2_NODE_LVL_L1) {
        LOG_CLI((BSL_META_U(unit,
                            "  %s.%d : INDEX=%d NUM_SP=%d FC=%d FMC=%d "
                            "UCMAP=0x%08x SPMAP=0x%08x MODE=%s WT=%d\n"),
                 lvl_name[level], offset, hw_index, num_spri, 
                 first_child, first_mc_child, ucmap, spmap,
                 sched_modes[sched_mode], wt));
    } else {
        LOG_CLI((BSL_META_U(unit,
                            "  %s.%d : INDEX=%d NUM_SPRI=%d FC=%d "
                            "SPMAP=0x%08x MODE=%s Wt=%d\n"),
                 lvl_name[level], offset, hw_index, num_spri, first_child,
                 spmap, sched_modes[sched_mode], wt));
    }

    soc_kt2_get_child_type(unit, port, level, &child_level);
    mem = _SOC_KT2_NODE_PARENT_MEM(child_level);
    
    if(mem == INVALIDm) {
        return SOC_E_INTERNAL;
    }
    index_max = soc_mem_index_max(unit, mem);

    num_child = 0;
    for (ii = 0, ci = 0; ii <= index_max; ii++) {
        rv = soc_mem_read(unit, mem, MEM_BLOCK_ALL, ii, &entry);
        if (rv) {
            LOG_CLI((BSL_META_U(unit,
                                "Failed to read memory at index: %d\n"), ii));
            break;
        }
        
        cindex = soc_mem_field32_get(unit, mem, entry, C_PARENTf);
        if (cindex == hw_index) {
            if (child_level == SOC_KT2_NODE_LVL_L2) {
                SOC_IF_ERROR_RETURN(soc_kt2_cosq_get_sched_mode(unit, port,
                                    SOC_KT2_NODE_LVL_L2, ii, &sched_mode, &wt));
                LOG_CLI((BSL_META_U(unit,
                                    "     L2.%s INDEX=%d Mode=%s WEIGHT=%d\n"), 
                         ((ii < 1024) ? "uc" : "mc"),
                         ii, sched_modes[sched_mode], wt));
            } else {
                _soc_kt2_dump_sched_at(unit, port, child_level, ci, ii);
                ci += 1;
            }
            num_child += 1;
        }
    }
    if (num_child == 0) {
        LOG_CLI((BSL_META_U(unit,
                            "*** No children \n")));
    }
    return SOC_E_NONE;
}

int soc_kt2_dump_port_lls(int unit, int port)
{
    LOG_CLI((BSL_META_U(unit,
                        "-------%s (LLS)------\n"), SOC_PORT_NAME(unit, (port))));

    _soc_kt2_dump_sched_at(unit, port, SOC_KT2_NODE_LVL_ROOT, 0, port);
                            
    return SOC_E_NONE;
}

#endif /* defined(BCM_KATANA2_SUPPORT) */
