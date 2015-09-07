/*
 * $Id: re_report.c 1.26 Broadcom SDK $
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
 * File:       bregex.c
 * Purpose:    Regex Eevent notification API
 */

#include <sal/core/libc.h>

#include <soc/drv.h>
#include <soc/mem.h>
#include <soc/debug.h>

#include <bcm/error.h>
#include <bcm/cosq.h>

#include <bcm_int/esw/mbcm.h>
#include <bcm_int/esw/firebolt.h>
#include <bcm_int/esw/scorpion.h>
#include <bcm_int/esw/triumph.h>
#include <bcm_int/esw/triumph2.h>
#include <bcm_int/esw/trident.h>
#include <bcm_int/esw/hurricane.h>
#include <bcm_int/esw/fb4regex.h>

#include <bcm_int/esw_dispatch.h>

#if defined(INCLUDE_REGEX)

#if defined(BCM_TRIUMPH3_SUPPORT)

#define _BCM_FT_REPORT_MAX_CB   4

typedef enum _bcm_ft_report_cb_state_e {
    _BCM_FT_REPORT_CALLBACK_STATE_INVALID = 0,
    _BCM_FT_REPORT_CALLBACK_STATE_ACTIVE,
    _BCM_FT_REPORT_CALLBACK_STATE_REGISTERED,
    _BCM_FT_REPORT_CALLBACK_STATE_UNREGISTERED
} _bcm_ft_report_cb_state_t;

typedef struct _bcm_ft_report_cb_entry_s {
    _bcm_ft_report_cb_state_t state;
    uint32  reports;
    bcm_regex_report_cb callback;
    void *userdata;
} _bcm_ft_report_cb_entry_t;

typedef struct _bcm_ft_report_ctrl_s {
    _bcm_ft_report_cb_entry_t callback_entry[_BCM_FT_REPORT_MAX_CB];
    sal_thread_t pid;          /* export fifo processing thread process id */
    VOL sal_usecs_t interval;  /* export fifo processing polling interval */
} _bcm_ft_report_ctrl_t;

static _bcm_ft_report_ctrl_t *_bcm_ft_report_ctrl[BCM_MAX_NUM_UNITS];

extern int 
_bcm_tr3_get_match_id(int unit, int signature_id, int *match_id);

STATIC int
_bcm_ft_key_field_get(uint32 *fld, int bp, int len, uint32 *kfld)
{
    int ii, bval;

    for (ii = 0; ii < (31 + len)/32; ii++) {
        kfld[ii] = 0;
    }

    for (ii = 0; ii < len; ii++) {
        bval = fld[(bp+ii)/32] & (1 << ((bp + ii) % 32));
        if (bval) {
            kfld[ii/32] |= 1 << (ii % 32);
        }
    }
    return 0;
}

STATIC int
_bcm_ft_ctr_enabled(int unit)
{
    uint32 regval, fval;

    SOC_IF_ERROR_RETURN(READ_FT_CONFIGr(unit, &regval));
    fval = soc_reg_field_get(unit, FT_CONFIGr, regval, CNT_MODEf);

    if ((fval == 1) || (fval == 2)) {
        return 1;
    }
    return 0;
}

STATIC int
_bcm_ft_report_process_export_entry(int unit,
                         void *entry, bcm_regex_report_t *data, 
                         int *pending, uint32 *pdir)
{
    soc_mem_t mem;
    uint32 val, pk[8], dir;
    uint8  *p8;
    uint32 type, ip1, ip2, p1, p2, pt;
    int rv = 0, i;

    mem = _bcm_ft_ctr_enabled(unit) ? FT_EXPORT_FIFOm : FT_EXPORT_DATA_ONLYm;

    type = soc_mem_field32_get(unit, mem, entry, KEY_TYPEf);
    *pending = 0;

    if ((type == 1) || (type == 2)) {
        val = soc_mem_field32_get(unit, mem, entry, SIGNATURE_IDf);
        rv = _bcm_tr3_get_match_id(unit, val, &data->match_id);
        if (rv) {
            return BCM_E_NOT_FOUND;
        }

        dir = soc_mem_field32_get(unit, mem, entry, KEY_DIRECTIONf);
        val = soc_mem_field32_get(unit, mem, entry, EVENT_IDf);
        data->flags = 0;
        if (val & 0x1) {
            data->flags |= BCM_REGEX_REPORT_NEW;
        }
        if ((val & 0x2) &&
            (soc_mem_field32_get(unit, mem, entry, SME_MATCHf))) {
            data->flags |= BCM_REGEX_REPORT_MATCHED;
        }
        if (val & 0x4) {
            data->flags |= BCM_REGEX_REPORT_END;
        }

        soc_mem_field_get(unit, mem, entry, KEYf, pk);
   
        if (type == 1) {
            _bcm_ft_key_field_get(pk, 0, 16, &p1);
            _bcm_ft_key_field_get(pk, 16, 16, &p2);
            _bcm_ft_key_field_get(pk, 32, 8, &pt);
            _bcm_ft_key_field_get(pk, 40, 32, &ip1);
            _bcm_ft_key_field_get(pk, 72, 32, &ip2);

            if (dir == 1) {
              data->dip = ip1;
              data->sip = ip2;
              data->src_port = p1;
              data->dst_port = p2;
            } else {
              data->sip = ip2;
              data->dip = ip1;
              data->src_port = p2;
              data->dst_port = p1;
            }
        } else if (type == 2) {
            *pending = 1;
            p8 = (uint8*) pk;
            for (i = 0; i < 8; i++) {
                if (dir == 0) {
                    data->sip6[i+8] = *p8;
                } else {
                    data->dip6[i+8] = *p8;
                }
                p8++;
            }
        }
        data->protocol = pt;
       
        data->packet_count_to_server = soc_mem_field32_get(unit, mem, 
                                                           entry, L2_PKT_CNT_Ff);
        data->byte_count_to_server = soc_mem_field32_get(unit, mem, 
                                                           entry, L2_BYTE_CNT_Ff);
        data->packet_count_to_client = soc_mem_field32_get(unit, mem, 
                                                           entry, L2_PKT_CNT_Rf);
        data->byte_count_to_client = soc_mem_field32_get(unit, mem, 
                                                           entry, L2_BYTE_CNT_Rf);
        data->start_timestamp = soc_mem_field32_get(unit, mem, entry, ID_TIMESTAMPf);
        data->last_timestamp = soc_mem_field32_get(unit, mem, entry, LAST_TIMESTAMPf);
    } else {
        p8 = (uint8*) pk;
        dir = *pdir;
        for (i = 0; i < 16; i++) {
            if (dir == 0) {
              data->dip6[i] = *p8;
            } else {
              data->sip6[i] = *p8;
            }
            p8++;
        }
        for (i = 0; i < 8; i++) {
            if (dir == 0) {
                data->sip6[i] = *p8;
            } else {
                data->dip6[i] = *p8;
            }
            p8++;
        }
        *pdir = dir;
    }

    data->flags |= (*pdir == 1) ? BCM_REGEX_MATCH_TO_SERVER : 
                                 BCM_REGEX_MATCH_TO_CLIENT;

    return BCM_E_NONE;
}

STATIC void
_bcm_report_fifo_dma_thread(void *unit_vp)
{
    int unit = PTR_TO_INT(unit_vp);
    _bcm_ft_report_ctrl_t *rctrl = _bcm_ft_report_ctrl[unit];
    _bcm_ft_report_cb_entry_t *cb_entry;
    bcm_regex_report_t data;
    int rv, entries_per_buf, interval, count, i, j, non_empty;
    int chan, entry_words, pending=0;
    void *host_buf, *host_entry;
    uint32 dir, *buff_max, rval;
    uint8  overflow, timeout;
    soc_mem_t   ftmem;
    soc_control_t *soc = SOC_CONTROL(unit);
    int cmc = SOC_PCI_CMC(unit);

    chan = SOC_MEM_FIFO_DMA_CHANNEL_0;
    entries_per_buf = 1024; 

    ftmem = _bcm_ft_ctr_enabled(unit) ? FT_EXPORT_FIFOm : FT_EXPORT_DATA_ONLYm;
    entry_words = soc_mem_entry_words(unit, ftmem);
    host_buf = soc_cm_salloc(unit, entries_per_buf*entry_words*sizeof(uint32),
                      "FT export fifo DMA Buffer");
    if (host_buf == NULL) {
        soc_event_generate(unit, SOC_SWITCH_EVENT_THREAD_ERROR, 
                           SOC_SWITCH_EVENT_THREAD_REGEX_REPORT, __LINE__, 
                           BCM_E_MEMORY);
        goto cleanup_exit;
    }

    rv = soc_mem_fifo_dma_start(unit, chan,
                                ftmem, MEM_BLOCK_ANY, 
                                entries_per_buf, host_buf); 
    if (BCM_FAILURE(rv)) {
        soc_event_generate(unit, SOC_SWITCH_EVENT_THREAD_ERROR, 
                           SOC_SWITCH_EVENT_THREAD_REGEX_REPORT,
                           __LINE__, rv);
        goto cleanup_exit;
    }

    host_entry = host_buf;
    buff_max = (uint32 *)host_entry + (entries_per_buf * entry_words);

    while ((interval = rctrl->interval) > 0) {
        overflow = 0; timeout = 0;
        if (soc->ftreportIntrEnb) {
            soc_cmicm_intr0_enable(unit, IRQ_CMCx_FIFO_CH_DMA(chan));
            if (sal_sem_take(SOC_CONTROL(unit)->ftreportIntr, interval) < 0) {
                soc_cm_debug(DK_VERBOSE | DK_INTR, 
                             " polling timeout ft_export_fifo=%d\n", interval);
            } else {
                soc_cm_debug(DK_VERBOSE | DK_INTR, "woken up interval=%d\n", interval);
                /* check for timeout or overflow and either process or continue */
                rval = soc_pci_read(unit, 
                        CMIC_CMCx_FIFO_CHy_RD_DMA_STAT_OFFSET(cmc, chan));
                overflow = soc_reg_field_get(unit, CMIC_CMC0_FIFO_CH0_RD_DMA_STATr,
                                             rval, HOSTMEM_TIMEOUTf);
                timeout = soc_reg_field_get(unit, 
                    CMIC_CMC0_FIFO_CH0_RD_DMA_STATr, rval, HOSTMEM_OVERFLOWf);
                overflow |= timeout ? 1 : 0;
            }
        } else {
            sal_usleep(interval);
        }

        if (rctrl->interval <= 0) {
            break;
        }

        /* reconsile the user registered callbacks. */
        for (i = 0; i < _BCM_FT_REPORT_MAX_CB; i++) {
            cb_entry = &rctrl->callback_entry[i];
            switch (cb_entry->state) {
            case _BCM_FT_REPORT_CALLBACK_STATE_REGISTERED:
                cb_entry->state = _BCM_FT_REPORT_CALLBACK_STATE_ACTIVE;
                break;
            case _BCM_FT_REPORT_CALLBACK_STATE_UNREGISTERED:
                cb_entry->state = _BCM_FT_REPORT_CALLBACK_STATE_INVALID;
                break;
            default:
                break;
            }
        }

        non_empty = FALSE;
        do {
            rv = soc_mem_fifo_dma_get_num_entries(unit, chan, &count);
            if (SOC_SUCCESS(rv)) {
                non_empty = TRUE;
                for (i = 0; i < count; i++) {
                    rv = _bcm_ft_report_process_export_entry(unit, host_entry,
                                                     &data, &pending, &dir);
                    host_entry = (uint32 *)host_entry + entry_words;
                    /* handle roll over */
                    if ((uint32 *)host_entry >= buff_max) {
                        host_entry = host_buf;
                    }
                    for (j = 0; 
                         (j < _BCM_FT_REPORT_MAX_CB) && !pending && !rv; j++) {
                        cb_entry = &rctrl->callback_entry[j];
                        if ((cb_entry->state ==
                            _BCM_FT_REPORT_CALLBACK_STATE_ACTIVE) &&
                            (data.flags & cb_entry->reports)) {
                            cb_entry->callback(unit, &data,
                                               cb_entry->userdata);
                        }
                    }
                }
                if (overflow) {
                    rval = 0;
                    soc_reg_field_set(unit, CMIC_CMC0_FIFO_CH0_RD_DMA_STAT_CLRr, 
                                        &rval, HOSTMEM_OVERFLOWf, 1);
                    soc_reg_field_set(unit, CMIC_CMC0_FIFO_CH0_RD_DMA_STAT_CLRr, 
                                        &rval, HOSTMEM_TIMEOUTf, 1);
                    soc_pci_write(unit, 
                        CMIC_CMCx_FIFO_CHy_RD_DMA_STAT_CLR_OFFSET(cmc, chan), rval);
                } 
                (void)_soc_mem_sbus_fifo_dma_set_entries_read(unit, chan, i);
            } else {
                non_empty = FALSE;
            }
        } while (non_empty);
    }

cleanup_exit:
    (void)soc_mem_fifo_dma_stop(unit, chan);

    if (host_buf != NULL) {
        soc_cm_sfree(unit, host_buf);
    }
    rctrl->pid = SAL_THREAD_ERROR;
    sal_thread_exit(0);
}

STATIC int
_bcm_regex_report_init(int unit)
{
    _bcm_ft_report_ctrl_t *rctrl = _bcm_ft_report_ctrl[unit];

    if (rctrl == NULL) {
        rctrl = sal_alloc(sizeof(_bcm_ft_report_ctrl_t), "Re_Report_ctrl Data");
        if (rctrl == NULL) {
            return BCM_E_MEMORY;
        }
        sal_memset(rctrl, 0, sizeof(_bcm_ft_report_ctrl_t));

        _bcm_ft_report_ctrl[unit] = rctrl;

        rctrl->pid = SAL_THREAD_ERROR;
    }

    return BCM_E_NONE;
}

static int
bcm_regex_report_control(int unit, sal_usecs_t interval)
{
    _bcm_ft_report_ctrl_t *rctrl = _bcm_ft_report_ctrl[unit];
    char name[32];

    rctrl = _bcm_ft_report_ctrl[unit];

    sal_snprintf(name, sizeof(name), "bcmFtExportDma.%d", unit);

    rctrl->interval = interval;
    if (interval) {
        if (rctrl->pid == SAL_THREAD_ERROR) {
            rctrl->pid = sal_thread_create(name, SAL_THREAD_STKSZ, 50,
                                                _bcm_report_fifo_dma_thread,
                                                INT_TO_PTR(unit));
            if (rctrl->pid == SAL_THREAD_ERROR) {
                soc_cm_debug(DK_ERR, "%s: Could not start thread\n",
                             __FUNCTION__);
                return BCM_E_MEMORY;
            }
        }
    } else {
        /* Wake up thread so it will check the changed interval value */
        sal_sem_give(SOC_CONTROL(unit)->ftreportIntr);
    }

    return BCM_E_NONE;
}

int
_bcm_esw_regex_report_register(int unit, uint32 reports,
                       bcm_regex_report_cb callback, void *userdata)
{
    _bcm_ft_report_ctrl_t *rctrl;
    _bcm_ft_report_cb_entry_t *entry;
    int free_index, i;

    if (!soc_feature(unit, soc_feature_regex)) {
        return BCM_E_UNAVAIL;
    }

    if (_bcm_ft_report_ctrl[unit] == NULL) {
        BCM_IF_ERROR_RETURN(_bcm_regex_report_init(unit));
    }
    rctrl = _bcm_ft_report_ctrl[unit];

    if (callback == NULL) {
        return BCM_E_PARAM;
    }

    free_index = -1;
    for (i = 0; i < _BCM_FT_REPORT_MAX_CB; i++) {
        entry = &rctrl->callback_entry[i];
        if (entry->state == _BCM_FT_REPORT_CALLBACK_STATE_INVALID ||
            (rctrl->pid == SAL_THREAD_ERROR &&
             entry->state == _BCM_FT_REPORT_CALLBACK_STATE_UNREGISTERED)) {
            if (free_index < 0) {
                free_index = i;
            }
        } else if (entry->state == _BCM_FT_REPORT_CALLBACK_STATE_ACTIVE ||
                   entry->state == _BCM_FT_REPORT_CALLBACK_STATE_REGISTERED) {
            if (entry->callback == callback && entry->userdata == userdata) {
                return BCM_E_NONE;
            }
        }
    }

    if (free_index < 0) {
        return BCM_E_RESOURCE;
    }
    entry = &rctrl->callback_entry[free_index];
    entry->callback = callback;
    entry->userdata = userdata;
    entry->reports = reports;
    entry->state = _BCM_FT_REPORT_CALLBACK_STATE_REGISTERED;

    return bcm_regex_report_control(unit, 100000);
}

int
_bcm_esw_regex_report_unregister(int unit, uint32 reports,
                         bcm_regex_report_cb callback,
                         void *userdata)
{
    _bcm_ft_report_ctrl_t *rctrl;
    _bcm_ft_report_cb_entry_t *entry;
    int i, num_active = 0;

    if (!soc_feature(unit, soc_feature_regex)) {
        return BCM_E_UNAVAIL;
    }

    if (_bcm_ft_report_ctrl[unit] == NULL) {
        return BCM_E_INIT;
    }
    rctrl = _bcm_ft_report_ctrl[unit];

    for (i = 0; i < _BCM_FT_REPORT_MAX_CB; i++) {
        entry = &rctrl->callback_entry[i];
        if (entry->callback == callback && entry->userdata == userdata) {
            entry->reports &= ~reports;
            if (!entry->reports) {
                entry->state = _BCM_FT_REPORT_CALLBACK_STATE_UNREGISTERED;
                continue;
            }
        }
        if ((entry->state == _BCM_FT_REPORT_CALLBACK_STATE_ACTIVE) ||
            (entry->state == _BCM_FT_REPORT_CALLBACK_STATE_REGISTERED)) {
            num_active++;
        }
    }
    if (num_active == 0) {
        bcm_regex_report_control(unit, 0);
    }
    return BCM_E_NONE;
}

int _bcm_esw_regex_report_reset(int unit)
{
    _bcm_ft_report_ctrl_t *rctrl;
    _bcm_ft_report_cb_entry_t *entry;
    int i;

    if (!soc_feature(unit, soc_feature_regex)) {
        return BCM_E_UNAVAIL;
    }

    if (_bcm_ft_report_ctrl[unit] == NULL) {
        return BCM_E_NONE;
    }

    rctrl = _bcm_ft_report_ctrl[unit];

    for (i = 0; i < _BCM_FT_REPORT_MAX_CB; i++) {
        entry = &rctrl->callback_entry[i];
        if ((entry->state == _BCM_FT_REPORT_CALLBACK_STATE_ACTIVE) ||
            (entry->state == _BCM_FT_REPORT_CALLBACK_STATE_REGISTERED)) {
            _bcm_esw_regex_report_unregister(unit, entry->reports,
                                             entry->callback, entry->userdata);
        }
    }
    return BCM_E_NONE;
}

#else
int
_bcm_esw_regex_report_register(int unit, uint32 reports,
                       bcm_regex_report_cb callback, void *userdata)
{
    return BCM_E_UNAVAIL;
}

int
_bcm_esw_regex_report_unregister(int unit, uint32 reports,
                         bcm_regex_report_cb callback,
                         void *userdata)
{
    return BCM_E_UNAVAIL;
}

int _bcm_esw_regex_report_reset(int unit)
{
    return BCM_E_UNAVAIL;
}


#endif /* BCM_TRIUMPH3_SUPPORT */

#endif /* INCLUDE_REGEX */

