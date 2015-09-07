/*
 * $Id: bfcmap_drv.h 1.5 Broadcom SDK $
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
 */
#ifndef BFCMAP_DRV_H
#define BFCMAP_DRV_H

struct bfcmap_chip_info_s;
struct bfcmap_port_ctrl_s;
struct bfcmap_dev_ctrl_s;
struct bfcmap_api_table_s;
struct bfcmap_counter_info_s;
struct bfcmap_counter_cb_s;

typedef int (*msec_unit_init_f)(struct bfcmap_dev_ctrl_s*);
typedef int (*msec_unit_uninit_f)(struct bfcmap_dev_ctrl_s*);
typedef int (*msec_config_set)(struct bfcmap_port_ctrl_s*,bfcmap_port_config_t*);
typedef int (*msec_config_get)(struct bfcmap_port_ctrl_s*,bfcmap_port_config_t*);
/*typedef int (*msec_capability_get)(struct bfcmap_port_ctrl_s*,bfcmap_port_capability_t*);*/
typedef int (*msec_port_init)(struct bfcmap_port_ctrl_s*,bfcmap_port_config_t*);
typedef int (*msec_port_uninit)(struct bfcmap_port_ctrl_s*);
typedef int (*msec_port_reset)(struct bfcmap_port_ctrl_s*);
typedef int (*msec_port_shutdown)(struct bfcmap_port_ctrl_s*);
typedef int (*msec_port_link_enable)(struct bfcmap_port_ctrl_s*);
typedef int (*msec_port_speed_set)(struct bfcmap_port_ctrl_s*, bfcmap_port_speed_t );
typedef int (*msec_port_bounce)(struct bfcmap_port_ctrl_s*);


typedef int (*msec_event_handler)(struct bfcmap_dev_ctrl_s*);

/* diagnostic functions */
typedef int (*msec_cipher_op_encrypt)(struct bfcmap_port_ctrl_s*,
#if 0
                                      bfcmap_crypto_aes128_gcm_t *key,
#endif
                                      buint8_t *protected_data, int pdlen,
                                      buint8_t *confidential_data, int cdlen,
                                      buint8_t *out_data, int outlen);

typedef int (*msec_cipher_op_decrypt)(struct bfcmap_port_ctrl_s*,
#if 0
                                      bfcmap_crypto_aes128_gcm_t *key,
#endif
                                      buint8_t *protected_data, int pdlen,
                                      buint8_t *confidential_data, int cdlen,
                                      buint8_t *icv_data, int icvlen,
                                      buint8_t *out_data, int outlen);


typedef struct bfcmap_api_table_s {
    /* Unit APIs */
    msec_unit_init_f        unit_init;
    msec_unit_uninit_f      unit_uninit;

    /* Port based APIs */
    msec_config_set         config_set;
    msec_config_get         config_get;
    /*msec_capability_get     capability_get;*/
    msec_port_init          port_init;
    msec_port_uninit        port_uninit;
    msec_port_reset         port_reset;
    msec_port_shutdown      port_shutdown;
    msec_port_link_enable   port_link_enable;
    msec_port_speed_set     port_speed_set;
    msec_port_bounce        port_bounce;

    msec_event_handler      event_handler;

    /* diagnostic functions */
    msec_cipher_op_encrypt  cipher_op_encrypt;
    msec_cipher_op_decrypt  cipher_op_decrypt;

} bfcmap_drv_t;

#define BFCMAP_DRV_FN(_mpc,_mf)   (_mpc)->api->_mf

#define _BFCMAP_DRV_CALL(_mpc, _mf, _ma)                     \
        (_mpc == NULL ? BFCMAP_E_PARAM :                     \
         (BFCMAP_DRV_FN(_mpc,_mf) == NULL ?                  \
         BFCMAP_E_UNAVAIL : BFCMAP_DRV_FN(_mpc,_mf) _ma))

#define _BFCMAP_DRV_CALL_1ARG(_mpc, _mf, _ma, _arg)                     \
        (_mpc == NULL ? BFCMAP_E_PARAM :                     \
         (BFCMAP_DRV_FN(_mpc,_mf) == NULL ?                  \
         BFCMAP_E_UNAVAIL : BFCMAP_DRV_FN(_mpc,_mf)(_ma, _arg)))

#define BFCMAP_PORT_CONFIG_SET(_mpc, _ma) \
        _BFCMAP_DRV_CALL((_mpc), config_set, ((_mpc), (_ma)))

#define BFCMAP_PORT_CONFIG_GET(_mpc, _ma) \
        _BFCMAP_DRV_CALL((_mpc), config_get, ((_mpc), (_ma)))

#define BFCMAP_PORT_CAPABILITY_GET(_mpc, _ma) \
        _BFCMAP_DRV_CALL((_mpc), capability_get, ((_mpc), (_ma)))

#define BFCMAP_PORT_INIT(_mpc, _ma) \
        _BFCMAP_DRV_CALL((_mpc), port_init, ((_mpc), (_ma)))

#define BFCMAP_PORT_UNINIT(_mpc) \
        _BFCMAP_DRV_CALL((_mpc), port_uninit, ((_mpc)))

#define BFCMAP_PORT_RESET(_mpc) \
        _BFCMAP_DRV_CALL((_mpc), port_reset, ((_mpc)))

#define BFCMAP_PORT_SHUTDOWN(_mpc) \
        _BFCMAP_DRV_CALL((_mpc), port_shutdown, ((_mpc)))

#define BFCMAP_PORT_LINK_ENABLE(_mpc) \
        _BFCMAP_DRV_CALL((_mpc), port_link_enable, ((_mpc)))

#define BFCMAP_PORT_BOUNCE(_mpc) \
        _BFCMAP_DRV_CALL((_mpc), port_bounce, ((_mpc)))

#define BFCMAP_PORT_SPEED_SET(_mpc, _speed) \
        _BFCMAP_DRV_CALL_1ARG((_mpc), port_speed_set, ((_mpc)), (_speed))

#define BFCMAP_DEVICE_INIT(_mdc) \
        _BFCMAP_DRV_CALL((_mdc)->ports, unit_init, ((_mdc)))

#define BFCMAP_DEVICE_UNINIT(_mdc) \
        _BFCMAP_DRV_CALL((_mdc)->ports, unit_uninit, ((_mdc)))

#define BFCMAP_PORT_GET_CFG(_mpc, _ma) \
        _BFCMAP_DRV_CALL((_mpc), get_cfg, ((_mpc), (_ma)))

#define BFCMAP_DEVICE_EVENT_HANDLER(_mdc)      \
        _BFCMAP_DRV_CALL((_mdc)->ports, event_handler, ((_mdc)))

#define BFCMAP_PORT_DIAG_CIPHER_OP_ENCRYPT(_mpc,k,ppd,pdl,pcd,cdl,pout,pol) \
        _BFCMAP_DRV_CALL((_mpc), cipher_op_encrypt, \
                        ((_mpc), (k), (ppd), (pdl), (pcd), (cdl), (pout), (pol)))

#define BFCMAP_PORT_DIAG_CIPHER_OP_DECRYPT(_mpc,k,ppd,pdl,pcd,cdl,picv,icvl,pout,pol) \
        _BFCMAP_DRV_CALL((_mpc), cipher_op_decrypt, \
                        ((_mpc), (k), (ppd), (pdl), (pcd), (cdl), (picv), (icvl), \
                        (pout), (pol)))

#endif /* BFCMAP_DRV_H */

