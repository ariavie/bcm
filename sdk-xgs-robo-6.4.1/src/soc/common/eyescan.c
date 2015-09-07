/* $Id: eyescan.c,v 1.46 Broadcom SDK $
 * $Id: 
 *
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
 * SOC EYESCAN
 */

#include <shared/bsl.h>

#include <sal/types.h>
#include <soc/eyescan.h>
#include <soc/drv.h>
#include <soc/error.h>
#include <soc/mem.h>
#include <soc/phy/phyctrl.h>

#ifdef PHYMOD_SUPPORT
#include <phymod/phymod_diagnostics.h>
#endif

#ifdef _ERR_MSG_MODULE_NAME
  #error "_ERR_MSG_MODULE_NAME redefined"
#endif
#define _ERR_MSG_MODULE_NAME BSL_LS_SOC_PHY

#ifndef __KERNEL__
#include <math.h> 
#endif /* #ifndef __KERNEL__ */

STATIC soc_port_phy_eyescan_counter_cb_t counter_func[SOC_MAX_NUM_DEVICES][socPortPhyEyescanNofCounters] = {{{0}}};

int 
soc_port_phy_eyescan_counter_register(int unit, soc_port_phy_eyscan_counter_t counter, soc_port_phy_eyescan_counter_cb_t* cf)
{
    SOC_INIT_FUNC_DEFS;

    if (!SOC_UNIT_VALID(unit)) {
        _SOC_EXIT_WITH_ERR(SOC_E_UNIT, (_SOC_MSG("Invalid unit")));
    }

    if(counter >= socPortPhyEyescanNofCounters || counter < 0) {
        _SOC_EXIT_WITH_ERR(SOC_E_PARAM, (_SOC_MSG("Counter %d isn't supported"),counter));
    }

    if(NULL == cf) { /*clear callbacks*/
        counter_func[unit][counter].clear_func = NULL;
        counter_func[unit][counter].get_func = NULL;
    } else {
        counter_func[unit][counter] = *cf;
    }

exit:
    SOC_FUNC_RETURN;
} 

STATIC int 
soc_port_phy_eyescan_check_bounds(int unit, soc_port_t port, uint32 inst, int flags, soc_port_phy_eyscan_counter_t counter, soc_port_phy_eye_bounds_t* bounds)
{
    int vmax, vmin, l_hmax, r_hmax;
    int locked;
    SOC_INIT_FUNC_DEFS;
    
    if((socPortPhyEyescanCounterRelativePhy!=counter) && bounds->vertical_min<0) {
        LOG_CLI((BSL_META_U(unit,
                            "Counter mode doesn't support negative vertical_min. Updated to 0.\n")));
        bounds->vertical_min = 0;
    } 
    if(flags & SRD_EYESCAN_FLAG_VERTICAL_ONLY) {
        bounds->horizontal_min = 0;
        bounds->horizontal_max = 0;
    } 
    
    MIIM_LOCK(unit);
    locked = 1;
    
    if(!(flags & SRD_EYESCAN_FLAG_VERTICAL_ONLY)){
        _SOC_IF_ERR_EXIT(soc_phyctrl_diag_ctrl(unit, port, inst,
                       PHY_DIAG_CTRL_CMD, PHY_DIAG_CTRL_EYE_GET_MAX_LEFT_HOFFSET, (int *) (&l_hmax)));
        _SOC_IF_ERR_EXIT(soc_phyctrl_diag_ctrl(unit, port, inst,
                       PHY_DIAG_CTRL_CMD, PHY_DIAG_CTRL_EYE_GET_MAX_RIGHT_HOFFSET, (int *) (&r_hmax)));
    }

    _SOC_IF_ERR_EXIT(soc_phyctrl_diag_ctrl(unit, port, inst,
                   PHY_DIAG_CTRL_CMD, PHY_DIAG_CTRL_EYE_GET_MIN_VOFFSET, (int *) (&vmin)));
    _SOC_IF_ERR_EXIT(soc_phyctrl_diag_ctrl(unit, port, inst,
                   PHY_DIAG_CTRL_CMD, PHY_DIAG_CTRL_EYE_GET_MAX_VOFFSET, (int *) (&vmax)));
    
    locked = 0;
    MIIM_UNLOCK(unit);
    
    if(bounds->vertical_min < vmin){
        LOG_CLI((BSL_META_U(unit,
                            "vertical_min smaller than min available. Updated to %d.\n"), vmin));
        bounds->vertical_min = vmin;
    }
    if (bounds->vertical_max > vmax) {
        LOG_CLI((BSL_META_U(unit,
                            "vertical_max larger than max available. Updated to %d.\n"), vmax));
        bounds->vertical_max = vmax;
    }
    
    if(!(flags & SRD_EYESCAN_FLAG_VERTICAL_ONLY)){
        if(bounds->horizontal_min < l_hmax){
            LOG_CLI((BSL_META_U(unit,
                                "horizontal_min smaller than min available. Updated to %d.\n"), l_hmax));
            bounds->horizontal_min = l_hmax;
        }
        if (bounds->horizontal_max > r_hmax) {
            LOG_CLI((BSL_META_U(unit,
                                "horizontal_max larger than max available. Updated to %d.\n"), r_hmax));
            bounds->horizontal_max = r_hmax;   
        }
    }
    
    if(bounds->vertical_min < -1*SOC_PORT_PHY_EYESCAN_V_INDEX){
        LOG_CLI((BSL_META_U(unit,
                            "vertical_min smaller than min available. Updated to %d.\n"), -1*SOC_PORT_PHY_EYESCAN_V_INDEX));
        bounds->vertical_min = -1*SOC_PORT_PHY_EYESCAN_V_INDEX;
    }
    if (bounds->vertical_max > SOC_PORT_PHY_EYESCAN_V_INDEX) {
        LOG_CLI((BSL_META_U(unit,
                            "vertical_max larger than max available. Updated to %d.\n"), SOC_PORT_PHY_EYESCAN_V_INDEX));
        bounds->vertical_max = SOC_PORT_PHY_EYESCAN_V_INDEX;
    }
    
    if(!(flags & SRD_EYESCAN_FLAG_VERTICAL_ONLY)){
        if(bounds->horizontal_min < -1*SOC_PORT_PHY_EYESCAN_H_INDEX){
            LOG_CLI((BSL_META_U(unit,
                                "horizontal_min smaller than min available. Updated to %d.\n"), -1*SOC_PORT_PHY_EYESCAN_H_INDEX));
            bounds->horizontal_min = -1*SOC_PORT_PHY_EYESCAN_H_INDEX;
        }
        if (bounds->horizontal_max > SOC_PORT_PHY_EYESCAN_H_INDEX) {
            LOG_CLI((BSL_META_U(unit,
                                "horizontal_max larger than max available. Updated to %d.\n"), SOC_PORT_PHY_EYESCAN_H_INDEX));
            bounds->horizontal_max = SOC_PORT_PHY_EYESCAN_H_INDEX;   
        }
    }
 
 
exit:
    if (locked) {
        MIIM_UNLOCK(unit);
    }
    SOC_FUNC_RETURN;
}


STATIC int 
soc_port_phy_eyescan_enable(int unit, uint32 inst, int flags, soc_port_t port, 
                            soc_port_phy_eyscan_counter_t counter, int enable, int* voffset)
{
    int  data;
    int p1_mapped[] = {0,1,3,2,4,5,7,6,8,9,11,10,12,13,15,14,16,17,19,18,20,21,23,22,24,25,27,26,28,29,31,30};
    int hoffset_init=0;
    SOC_INIT_FUNC_DEFS;

    MIIM_LOCK(unit);
    
    switch(counter) {
    case socPortPhyEyescanCounterRelativePhy:
        if(enable) {
            _SOC_IF_ERR_EXIT(soc_phyctrl_diag_ctrl(unit, port, inst,
                    PHY_DIAG_CTRL_CMD, PHY_DIAG_CTRL_EYE_ENABLE_LIVELINK, (void *) (0)));

            _SOC_IF_ERR_EXIT(soc_phyctrl_diag_ctrl(unit, port, inst,
                    PHY_DIAG_CTRL_CMD, PHY_DIAG_CTRL_EYE_GET_INIT_VOFFSET, voffset));
             /* use the mapped P1 value */
             data = *voffset & 0x1f;
             data = p1_mapped[data];
             data |= (*voffset) & 0x20;
             *voffset = data;

        } else {
            if(SRD_EYESCAN_INVALID_VOFFSET != *voffset) {
                if(!(flags & SRD_EYESCAN_FLAG_VERTICAL_ONLY)) {
                    _SOC_IF_ERR_EXIT(soc_phyctrl_diag_ctrl(unit, port, inst,
                           PHY_DIAG_CTRL_CMD, PHY_DIAG_CTRL_EYE_SET_HOFFSET, &hoffset_init));
                }

                _SOC_IF_ERR_EXIT(soc_phyctrl_diag_ctrl(unit, port, inst,
                       PHY_DIAG_CTRL_CMD, PHY_DIAG_CTRL_EYE_SET_VOFFSET, (int *) voffset));
            }
            _SOC_IF_ERR_EXIT(soc_phyctrl_diag_ctrl(unit, port, inst,
                   PHY_DIAG_CTRL_CMD, PHY_DIAG_CTRL_EYE_DISABLE_LIVELINK, (void *) (0)));
        }
        break;
    case socPortPhyEyescanCounterPrbsPhy:
    case socPortPhyEyescanCounterPrbsMac:
    case socPortPhyEyescanCounterCrcMac:
    case socPortPhyEyescanCounterBerMac: 
    default:
        if(enable) {
            _SOC_IF_ERR_EXIT(soc_phyctrl_diag_ctrl(unit, port, inst,
                    PHY_DIAG_CTRL_CMD, PHY_DIAG_CTRL_EYE_ENABLE_DEADLINK, (void *) (0)));

        } else {
            _SOC_IF_ERR_EXIT(soc_phyctrl_diag_ctrl(unit, port, inst,
                    PHY_DIAG_CTRL_CMD, PHY_DIAG_CTRL_EYE_SET_HOFFSET,  &hoffset_init));

            _SOC_IF_ERR_EXIT(soc_phyctrl_diag_ctrl(unit, port, inst,
                       PHY_DIAG_CTRL_CMD, PHY_DIAG_CTRL_EYE_SET_VOFFSET,  &hoffset_init));

            _SOC_IF_ERR_EXIT(soc_phyctrl_diag_ctrl(unit, port, inst,
                    PHY_DIAG_CTRL_CMD, PHY_DIAG_CTRL_EYE_DISABLE_DEADLINK, (void *) (0)));
        }
        break;
    }
   
exit:
    MIIM_UNLOCK(unit);
    SOC_FUNC_RETURN;
}

STATIC int 
soc_port_phy_eyescan_counter_clear(int unit, soc_port_t port, uint32 inst, soc_port_phy_eyscan_counter_t counter, sal_usecs_t* start_time)
{
    uint32 error;
    SOC_INIT_FUNC_DEFS;

     if(NULL != counter_func[unit][counter].get_func) {
        counter_func[unit][counter].clear_func(unit, port);
    } else {
        switch(counter) {
        case socPortPhyEyescanCounterRelativePhy:
            _SOC_IF_ERR_EXIT(soc_phyctrl_diag_ctrl(unit, port, inst,
                        PHY_DIAG_CTRL_CMD, PHY_DIAG_CTRL_EYE_CLEAR_LIVELINK, (void *) (0)));
            break;
        case socPortPhyEyescanCounterPrbsPhy:
            _SOC_IF_ERR_EXIT(soc_phyctrl_diag_ctrl(unit, port, inst,
                        PHY_DIAG_CTRL_CMD, PHY_DIAG_CTRL_EYE_READ_DEADLINK, &error));
            break;
        default:
            _SOC_EXIT_WITH_ERR(SOC_E_PARAM, (_SOC_MSG("Counter isn't supported by the device")));
            break;
        }
    } 

    *start_time = sal_time_usecs();

exit:
    SOC_FUNC_RETURN;
}

STATIC int 
soc_port_phy_eyescan_start(int unit,  soc_port_t port, uint32 inst, soc_port_phy_eyscan_counter_t counter)
{
    SOC_INIT_FUNC_DEFS;

    switch(counter) {
    case socPortPhyEyescanCounterRelativePhy:
        _SOC_IF_ERR_EXIT(soc_phyctrl_diag_ctrl(unit, port, inst,
                    PHY_DIAG_CTRL_CMD, PHY_DIAG_CTRL_EYE_START_LIVELINK, (void *) (0)));

        break;
    case socPortPhyEyescanCounterPrbsMac:
    case socPortPhyEyescanCounterPrbsPhy:
    case socPortPhyEyescanCounterCrcMac:
    case socPortPhyEyescanCounterBerMac: 
    default:
        _SOC_IF_ERR_EXIT(soc_phyctrl_diag_ctrl(unit, port, inst,
                    PHY_DIAG_CTRL_CMD, PHY_DIAG_CTRL_EYE_START_DEADLINK, (void *) (0)));
        break;
    }

exit:
    SOC_FUNC_RETURN;
}

STATIC int 
soc_port_phy_eyescan_stop(int unit, soc_port_t port, uint32 inst, soc_port_phy_eyscan_counter_t counter, sal_usecs_t* end_time)
{
    SOC_INIT_FUNC_DEFS;

    switch(counter) {
    case socPortPhyEyescanCounterRelativePhy:
        _SOC_IF_ERR_EXIT(soc_phyctrl_diag_ctrl(unit, port, inst,
                    PHY_DIAG_CTRL_CMD, PHY_DIAG_CTRL_EYE_STOP_LIVELINK, (void *) (0)));
        break;
    default:
        break;
    }
    
    *end_time = sal_time_usecs();

exit:
    SOC_FUNC_RETURN;
}

STATIC int 
soc_port_phy_eyescan_counter_get(int unit, soc_port_t port, uint32 inst, soc_port_phy_eyscan_counter_t counter, uint32* err_count)
{
    SOC_INIT_FUNC_DEFS;

    if(NULL != counter_func[unit][counter].get_func) {
        counter_func[unit][counter].get_func(unit, port, err_count);
    } else {

        switch(counter) {
        case socPortPhyEyescanCounterRelativePhy:
            _SOC_IF_ERR_EXIT(soc_phyctrl_diag_ctrl(unit, port, inst,
                        PHY_DIAG_CTRL_CMD, PHY_DIAG_CTRL_EYE_READ_LIVELINK, err_count));
            break;
        case socPortPhyEyescanCounterPrbsPhy:
            _SOC_IF_ERR_EXIT(soc_phyctrl_diag_ctrl(unit, port, inst,
                        PHY_DIAG_CTRL_CMD, PHY_DIAG_CTRL_EYE_READ_DEADLINK, err_count));
            break;
        default:
            _SOC_EXIT_WITH_ERR(SOC_E_PARAM, (_SOC_MSG("Counter isn't supported by the device")));    
            break;
        }
    }

exit:
    SOC_FUNC_RETURN;
}

int _soc_port_phy_eyescan_run( 
    int unit, 
    uint32 inst,            /* phy instance containing phy number and interface */
    int flags, 
    soc_port_phy_eyescan_params_t* params, 
    uint32 nof_ports,
    soc_port_t* ports,
    int* lane_num,
    soc_port_phy_eyescan_results_t* results /*array of result per port*/) 
{
#if 0
    int port=0,index=0;
#endif
    int h,v,j;
    int under_threshold, first_run, default_sample_time;
    int total_time, desired_time, time_before;
    sal_usecs_t* start_time = NULL;
    sal_usecs_t end_time = 0;
    uint32 err_count;
    int* v_init_offset = NULL;
    soc_port_t* local_ports = NULL;
    int* local_lane_num = NULL;
    pbmp_t valid_ports_bitmap;
    uint32 is_enable;
    int dev, intf;
    SOC_INIT_FUNC_DEFS;

    /*validate input*/
    if (!SOC_UNIT_VALID(unit)) {
        _SOC_EXIT_WITH_ERR(SOC_E_UNIT, (_SOC_MSG("Invalid unit")));
    }

    SOC_NULL_CHECK(params);
    SOC_NULL_CHECK(ports);
    SOC_NULL_CHECK(results);

    if(params->counter >= socPortPhyEyescanNofCounters || params->counter < 0) {
        _SOC_EXIT_WITH_ERR(SOC_E_PARAM, (_SOC_MSG("Counter %d isn't supported"),params->counter));
    }

    /*allocate & initialize local_ports array*/
    local_ports = (soc_port_t*)sal_alloc(sizeof(soc_port_t)*nof_ports,"eyecan local ports");
    for (j = 0; j < nof_ports; j++) {
        local_ports[j] = -1;
    }
    if(NULL == local_ports) {
        _SOC_EXIT_WITH_ERR(SOC_E_MEMORY, (_SOC_MSG("Failed to allocate local_ports array")));
    }

    /*allocate & initialize local_lane_num array*/
    local_lane_num = (int*)sal_alloc(sizeof(int)*nof_ports,"eyecan local_lane_num");
    if(NULL == local_lane_num) {
        _SOC_EXIT_WITH_ERR(SOC_E_MEMORY, (_SOC_MSG("Failed to allocate local_lane_num array")));
    }

    /*get valid ports*/
    SOC_PBMP_ASSIGN(valid_ports_bitmap, SOC_INFO(unit).port.bitmap);

    intf = PHY_DIAG_INTF_DFLT;
    dev = PHY_DIAG_INST_DEV(inst);
    if (dev == PHY_DIAG_DEV_EXT) {
        intf = PHY_DIAG_INST_INTF(inst);
    }

    for(j = 0 ; j < nof_ports ; j++) {
        if (SOC_PBMP_MEMBER(valid_ports_bitmap, ports[j])) {
            if (dev == PHY_DIAG_DEV_EXT) {
                _SOC_IF_ERR_EXIT
                    (soc_phyctrl_redirect_control_get(unit, ports[j], dev, -1, 
                                                      (intf == PHY_DIAG_INTF_SYS),
                                                      SOC_PHY_CONTROL_RX_SEQ_DONE,
                                                      &is_enable));
            } else {
                _SOC_IF_ERR_EXIT
                    (soc_phyctrl_control_get(unit, ports[j],
                                             SOC_PHY_CONTROL_RX_SEQ_DONE,
                                             &is_enable));
            }
            if(is_enable) {
                local_ports[j] = ports[j];
            } else {
                /*do not run eyescan*/
                local_ports[j] = -1;
            }
        } else {
            _SOC_EXIT_WITH_ERR(SOC_E_PORT, (_SOC_MSG("Invalid port %d"),ports[j])); 
        }
    }

    /* check if the lane num is NULL */
    if (lane_num != NULL) {
        phy_ctrl_t *pc;               
        for(j = 0 ; j < nof_ports ; j++) {
            if (local_ports[j] == -1) continue;
            pc = INT_PHY_SW_STATE(unit, local_ports[j]);
            local_lane_num[j] = pc->lane_num;
            pc->lane_num = lane_num[j];
        }
    }

    /*allocate start_time array*/
    start_time = (uint32*)sal_alloc(sizeof(sal_usecs_t)*nof_ports,"eyecan start time");
    if(NULL == start_time) {
        _SOC_EXIT_WITH_ERR(SOC_E_MEMORY, (_SOC_MSG("Failed to allocate start_time array")));
    }

    /*allocate v_init_offset array*/
    v_init_offset = (int*)sal_alloc(sizeof(int)*nof_ports,"eyecan v_init_offset");
    if(NULL == v_init_offset) {
        _SOC_EXIT_WITH_ERR(SOC_E_MEMORY, (_SOC_MSG("Failed to allocate v_init_offset array")));
    }
    for(j = 0 ; j < nof_ports ; j++) {
        v_init_offset[j] = SRD_EYESCAN_INVALID_VOFFSET;
    }

    /*enable eyescan*/
    for (j = 0; j < nof_ports; j++) {
        /*if serdes is unlock continue*/
        if (local_ports[j] == -1) continue;
        _SOC_IF_ERR_EXIT(soc_port_phy_eyescan_enable(unit, inst, flags, local_ports[j], params->counter, 1, &(v_init_offset[j])));
    }

    sal_usleep(100000);

    /*check bounds*/
    for (j = 0; j < nof_ports; j++) {
        /*if serdes is unlock continue*/
        if (local_ports[j] == -1) continue;
        _SOC_IF_ERR_EXIT(soc_port_phy_eyescan_check_bounds(unit, local_ports[j], inst, flags, params->counter, &(params->bounds)));
    }
    
    default_sample_time = params->sample_time;
   
    /*scan*/
    for(h = params->bounds.horizontal_min; h <= params->bounds.horizontal_max; h++) {
        if(h % params->sample_resolution != 0) {
            if(!(flags & SRD_EYESCAN_FLAG_VERTICAL_ONLY)){
                continue;
            }
        }
        desired_time = 0;
        params->sample_time = default_sample_time;
        
        for(v = params->bounds.vertical_max; v >= params->bounds.vertical_min; v--) {
            if(v % params->sample_resolution != 0) {
                continue;
            }
            
            if (flags & SRD_EYESCAN_FLAG_DSC_PRINT) {
                LOG_CLI((BSL_META_U(unit,
                                    "\nStarting BER msmt at offset: v=%d h=%d\n"), v, h));
            }
            
            under_threshold = params->nof_threshold_links;
            first_run = 1;     
            total_time = 0;   
            for (j = 0; j < nof_ports; j++) {
                /*if serdes is unlock continue*/
                if (local_ports[j] == -1) continue;
                
                results[j].run_time[h + SOC_PORT_PHY_EYESCAN_H_INDEX][v + SOC_PORT_PHY_EYESCAN_V_INDEX] = 0;
                results[j].error_count[h + SOC_PORT_PHY_EYESCAN_H_INDEX][v + SOC_PORT_PHY_EYESCAN_V_INDEX] = 0;            
            }
            
            while ((under_threshold >= params->nof_threshold_links) && (params->sample_time <= params->time_upper_bound)) {
                under_threshold = 0;
                if (first_run) {
                    desired_time = params->sample_time;
                    total_time = params->sample_time;
                } else {
                    if ((desired_time >= params->time_upper_bound) || (total_time >= params->time_upper_bound)) {
                        params->sample_time = (params->time_upper_bound + 1);
                        break;
                    } 
                    desired_time = desired_time * TIME_MULTIPLIER;
                    params->sample_time = desired_time - total_time;
                    if (params->sample_time > params->time_upper_bound) {
                        params->sample_time = params->time_upper_bound - total_time;
                    }
                    total_time += params->sample_time;
                } 
                
                for (j = 0; (j < nof_ports); j++) {
                    /*if serdes is unlock continue*/
                    if (local_ports[j] == -1) continue;
            
                    if (first_run) {
                        /*set vertical and horizinal*/
                        if(!(flags & SRD_EYESCAN_FLAG_VERTICAL_ONLY)) {
                            _SOC_IF_ERR_EXIT(soc_phyctrl_diag_ctrl(unit, local_ports[j], inst,
                               PHY_DIAG_CTRL_CMD, PHY_DIAG_CTRL_EYE_SET_HOFFSET, (int*) (&h)));
                        }
                        _SOC_IF_ERR_EXIT(soc_phyctrl_diag_ctrl(unit, local_ports[j], inst,
                           PHY_DIAG_CTRL_CMD, PHY_DIAG_CTRL_EYE_SET_VOFFSET, (int *) (&v)));/*start measure*/
                        _SOC_IF_ERR_EXIT(soc_port_phy_eyescan_start(unit, local_ports[j], inst, params->counter));

                        if ((flags & SRD_EYESCAN_FLAG_DSC_PRINT)){
                            soc_phyctrl_control_set(unit, local_ports[j], SOC_PHY_CONTROL_DUMP, 1);
                        }
                    }
                    
                    /*clear counters*/
                    _SOC_IF_ERR_EXIT(soc_port_phy_eyescan_counter_clear(unit, local_ports[j], inst, params->counter, &(start_time[j])));
    
                }
                sal_usleep(params->sample_time*1000);

                for (j = 0; j < nof_ports; j++) {

                    /*if serdes is unlock continue*/
                    if (local_ports[j] == -1) continue;

                    /*stop measure*/
                    _SOC_IF_ERR_EXIT(soc_port_phy_eyescan_stop(unit, local_ports[j], inst, params->counter, &end_time));
                    _SOC_IF_ERR_EXIT(soc_port_phy_eyescan_counter_get(unit, local_ports[j], inst,  params->counter, &err_count));
    
                    time_before = results[j].run_time[h + SOC_PORT_PHY_EYESCAN_H_INDEX][v + SOC_PORT_PHY_EYESCAN_V_INDEX];
                    
                    if(end_time > start_time[j]) {
                        results[j].run_time[h + SOC_PORT_PHY_EYESCAN_H_INDEX][v + SOC_PORT_PHY_EYESCAN_V_INDEX] += (end_time - start_time[j])/1000;
                    } else {
                        results[j].run_time[h + SOC_PORT_PHY_EYESCAN_H_INDEX][v + SOC_PORT_PHY_EYESCAN_V_INDEX] += ((SAL_USECS_TIMESTAMP_ROLLOVER - start_time[j]) + end_time)/1000;
                    }
                    results[j].error_count[h + SOC_PORT_PHY_EYESCAN_H_INDEX][v + SOC_PORT_PHY_EYESCAN_V_INDEX] += (err_count);
                    
                    if (results[j].error_count[h + SOC_PORT_PHY_EYESCAN_H_INDEX][v + SOC_PORT_PHY_EYESCAN_V_INDEX] < params->error_threshold) {
                        under_threshold++;
                    }
                    
                    if (flags & SRD_EYESCAN_FLAG_DSC_PRINT) {
                         LOG_CLI((BSL_META_U(unit,
                                             "Added %d errors in %d miliseconds for port %d\n"), err_count, (results[j].run_time[h + SOC_PORT_PHY_EYESCAN_H_INDEX][v + SOC_PORT_PHY_EYESCAN_V_INDEX] - time_before), local_ports[j]));
                    }
#if 0                    
                    port = local_ports[j];
                    index = j;
#endif
                }

                first_run = 0;
                if (params->counter == socPortPhyEyescanCounterRelativePhy) {
                    break;
                }
            }
            if (first_run) {
                for (j = 0; j < nof_ports; j++) {
                    results[j].run_time[h + SOC_PORT_PHY_EYESCAN_H_INDEX][v + SOC_PORT_PHY_EYESCAN_V_INDEX] = (params->sample_time-1);
                }
            }
            
            if ((params->counter != socPortPhyEyescanCounterRelativePhy) && (desired_time < params->time_upper_bound)) {
                params->sample_time = total_time;
            }
            
            if ((params->sample_time < params->time_upper_bound) && (total_time >= params->time_upper_bound)) {
                params->sample_time = params->time_upper_bound;
            }
        }
        
        if(flags & SRD_EYESCAN_FLAG_VERTICAL_ONLY) {
            break;
        }
    }

    /*highest number of errors for unlock serdes*/
     for (j = 0; j < nof_ports; j++) {
         /*if serdes is unlock continue*/
         if (local_ports[j] != -1) continue;

         LOG_CLI((BSL_META_U(unit,
                             "ERROR: link %d is unlock.\n"), ports[j]));
        /*error results*/
        for(h = params->bounds.horizontal_min ; h <= params->bounds.horizontal_max ; h++) {
            if(h % params->sample_resolution != 0) {
                if(!(flags & SRD_EYESCAN_FLAG_VERTICAL_ONLY)){
                    continue;
                }
            }
            for (v = params->bounds.vertical_min ; v <= params->bounds.vertical_max ; v++) {
                if(v % params->sample_resolution != 0) {
                    continue;
                }

                results[j].run_time[h + SOC_PORT_PHY_EYESCAN_H_INDEX][v + SOC_PORT_PHY_EYESCAN_V_INDEX] = 0;
                results[j].error_count[h + SOC_PORT_PHY_EYESCAN_H_INDEX][v + SOC_PORT_PHY_EYESCAN_V_INDEX] = 0;

            }
        }
     }



exit:
    SOC_FREE(start_time);
    if (v_init_offset) {

        /*disable eyescan mode*/
        for (j = 0; j < nof_ports; j++) {
            /*if serdes is unlock continue*/
            if (local_ports != NULL) {
                if (local_ports[j] == -1) continue;
        
                soc_port_phy_eyescan_enable(unit, inst, flags, local_ports[j], params->counter, 0, &(v_init_offset[j]));
            }
        }
        SOC_FREE(v_init_offset);
    }

    /* check if the lane num needs to be recovered */
    if (lane_num != NULL) {
        phy_ctrl_t *pc;               
        for (j = 0 ; j < nof_ports ; j++) {
            if (local_ports != NULL) {
                if (local_ports[j] == -1) continue;
                pc = INT_PHY_SW_STATE(unit, local_ports[j]);
                if (local_lane_num != NULL) {
                    pc->lane_num = local_lane_num[j];
                }
            }
        }
    }

    SOC_FREE(local_ports);
    SOC_FREE(local_lane_num);

    SOC_FUNC_RETURN;

}

#ifdef PHYMOD_SUPPORT

#ifndef TSC_ID0
#define TSC_ID0        0x600d
#endif
#ifndef TSC_ID1
#define TSC_ID1        0x8770
#endif

static int is_eyescan_algorithm_legacy_mode(int unit, uint32 nof_ports, soc_port_t* ports)
{
        int legacy;
        phy_ctrl_t *pc;

        legacy = 1;

        if (!nof_ports) return (legacy);

        pc = INT_PHY_SW_STATE(unit, ports[0]);
        if (!pc) return(0);

        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              " Phy ID0: 0x%X, Phy ID1: 0x%X Phy Model: 0x%X \n"),
                   pc->phy_id0, pc->phy_id1, pc->phy_model));
        if ((pc->phy_id0 == TSC_ID0) && (pc->phy_id1 == TSC_ID1) && (pc->phy_model == 0x37)) {
            legacy = 0;
        }

        return (legacy); 
}

const phymod_phy_access_t* _get_phymod_phy_access(soc_phymod_ctrl_t *pmc, int phy_idx)
{
    if (!pmc) return (NULL); 
    if (!pmc->phy[phy_idx]) return (NULL);
    return(&pmc->phy[phy_idx]->pm_phy);
}

int _soc_port_phy_eyescan_run_lowber( 
    int                             unit, 
    uint32                          inst,	/* phy instance containing phy number and interface */
    int                             flags, 
    soc_port_phy_eyescan_params_t*  params, 
    uint32                          nof_ports,
    soc_port_t*                     ports,
    int*                            lane_num,
    soc_port_phy_eyescan_results_t* results)    /*array of result per port*/
{
    int           j, dev, intf, rc;
    /* int           default_sample_time; */
    int*          v_init_offset = NULL;
    int*          local_lane_num = NULL;
    uint32_t        is_enable;
    sal_usecs_t*  start_time = NULL;
    soc_port_t*   local_ports = NULL;
    pbmp_t        valid_ports_bitmap;
    soc_phymod_ctrl_t **pmc = NULL;
    const phymod_phy_access_t   *pa;
    int idx;
    phymod_phy_eyescan_options_t eyescan_options;

    SOC_INIT_FUNC_DEFS;

    /*validate input*/
    if (!SOC_UNIT_VALID(unit)) {
        _SOC_EXIT_WITH_ERR(SOC_E_UNIT, (_SOC_MSG("Invalid unit")));
    }
    SOC_NULL_CHECK(params);
    SOC_NULL_CHECK(ports);
    SOC_NULL_CHECK(results);

    if(params->counter >= socPortPhyEyescanNofCounters || params->counter < 0) {
        _SOC_EXIT_WITH_ERR(SOC_E_PARAM, (_SOC_MSG("Counter %d isn't supported"),params->counter));
    }

    /*allocate & initialize local_ports array*/
    local_ports = (soc_port_t*)sal_alloc(sizeof(soc_port_t)*nof_ports,"eyecan local ports");
    if(NULL == local_ports) {
        _SOC_EXIT_WITH_ERR(SOC_E_MEMORY, (_SOC_MSG("Failed to allocate local_ports array")));
    }

    for (j = 0; j < nof_ports; j++) {
        local_ports[j] = -1;
    }

    /*allocate & initialize local_lane_num array*/
    local_lane_num = (int*)sal_alloc(sizeof(int)*nof_ports,"eyecan local_lane_num");
    if(NULL == local_lane_num) {
        _SOC_EXIT_WITH_ERR(SOC_E_MEMORY, (_SOC_MSG("Failed to allocate local_lane_num array")));
    }

    /*get valid ports*/
    SOC_PBMP_ASSIGN(valid_ports_bitmap, SOC_INFO(unit).port.bitmap);
    LOG_ERROR(BSL_LS_SOC_COMMON,
              (BSL_META_U(unit,
                          "valid_ports_bitmap=0x%X\n"),
               valid_ports_bitmap.pbits[0]));

    dev  = PHY_DIAG_INST_DEV(inst);
    intf = (dev == PHY_DIAG_DEV_EXT)? PHY_DIAG_INST_INTF(inst) : PHY_DIAG_INTF_DFLT;

    for(j = 0 ; j < nof_ports ; j++) {

        if (SOC_PBMP_MEMBER(valid_ports_bitmap, ports[j])) {
            if (dev == PHY_DIAG_DEV_EXT) {
                _SOC_IF_ERR_EXIT
                    (soc_phyctrl_redirect_control_get(unit, ports[j], dev, -1, 
                                                      (intf == PHY_DIAG_INTF_SYS),
                                                      SOC_PHY_CONTROL_RX_SEQ_DONE,
                                                      &is_enable));
            } else {
                _SOC_IF_ERR_EXIT
                    (soc_phyctrl_control_get(unit, ports[j],
                                             SOC_PHY_CONTROL_RX_SEQ_DONE,
                                             &is_enable));
            }
            if(is_enable) {
                LOG_ERROR(BSL_LS_SOC_COMMON,
                          (BSL_META_U(unit,
                                      "ports[%d]=%d is Enabled \n"),
                           j, ports[j]));
                local_ports[j] = ports[j];
            } else {
                /*do not run eyescan*/
                LOG_ERROR(BSL_LS_SOC_COMMON,
                          (BSL_META_U(unit,
                                      "port%d is Disabled\n"),
                           j));
                local_ports[j] = -1;
            }
        } else {
             LOG_ERROR(BSL_LS_SOC_COMMON,
                       (BSL_META_U(unit,
                                   "Exit on Invalid port%d \n"),
                        j));
            _SOC_EXIT_WITH_ERR(SOC_E_PORT, (_SOC_MSG("Invalid port %d"),ports[j])); 
        }
    }

    /*allocate & initialize phyaccess structures */
    pmc = (soc_phymod_ctrl_t**)sal_alloc(sizeof(soc_phymod_ctrl_t*)*nof_ports,"eyescan phymod_ctrl pointers");
    if(NULL == pmc) {
         LOG_ERROR(BSL_LS_SOC_COMMON,
                   (BSL_META_U(unit,
                               "PMC Allocate Fail.\n")));
        _SOC_EXIT_WITH_ERR(SOC_E_MEMORY, (_SOC_MSG("Failed to allocate phymod_ctrl array")));
    }
    memset(pmc, 0, sizeof(soc_phymod_ctrl_t*)*nof_ports);

    for(j = 0 ; j < nof_ports ; j++) {
        phy_ctrl_t *pc;

        if (local_ports[j] == -1) continue;
        pc = INT_PHY_SW_STATE(unit, local_ports[j]);
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "PC = %p..\n"),
                   pc));
        if (pc == NULL) return (SOC_E_INTERNAL);

        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "PMC for Port %d is assigned..\n"),
                   j));
        pmc[j] = &pc->phymod_ctrl;

        if (pmc[j] == NULL) return (SOC_E_INTERNAL);

        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "PMC for Port %d is not NULL..\n"),
                   j));
    }

    /* check if the lane num is NULL */
    if (lane_num != NULL) {
        phy_ctrl_t *pc;               
        for(j = 0 ; j < nof_ports ; j++) {
            if (local_ports[j] == -1) continue;
            pc = INT_PHY_SW_STATE(unit, local_ports[j]);
            local_lane_num[j] = pc->lane_num;
            pc->lane_num = lane_num[j];
        }
    }

    /*allocate start_time array*/
    start_time = (uint32*)sal_alloc(sizeof(sal_usecs_t)*nof_ports,"eyecan start time");
    if(NULL == start_time) {
        _SOC_EXIT_WITH_ERR(SOC_E_MEMORY, (_SOC_MSG("Failed to allocate start_time array")));
    }

    /*allocate v_init_offset array*/
    v_init_offset = (int*)sal_alloc(sizeof(int)*nof_ports,"eyecan v_init_offset");
    if(NULL == v_init_offset) {
        _SOC_EXIT_WITH_ERR(SOC_E_MEMORY, (_SOC_MSG("Failed to allocate v_init_offset array")));
    }
    for(j = 0 ; j < nof_ports ; j++) {
        v_init_offset[j] = SRD_EYESCAN_INVALID_VOFFSET;
    }

    /*check bounds*/
    for (j = 0; j < nof_ports; j++) {
        /*if serdes is unlock continue*/
        if (local_ports[j] == -1) continue;
        _SOC_IF_ERR_EXIT(soc_port_phy_eyescan_check_bounds(unit, local_ports[j], inst, flags, params->counter, &(params->bounds)));
    }
    
    /* default_sample_time = params->sample_time; */

    eyescan_options.timeout_in_milliseconds = 2000;
    eyescan_options.horz_max = 31; 
    eyescan_options.horz_min = -31;
    eyescan_options.hstep = 1;
    eyescan_options.vert_max = 31;
    eyescan_options.vert_min = -31;
    eyescan_options.vstep = 1;
    eyescan_options.mode = 0x00;
 
    /*enable eyescan*/
    for (j = 0; j < nof_ports; j++) {
        uint32_t buffer[64*64];
        int      speed;
        /*if serdes is unlock continue*/
        if (local_ports[j] == -1) continue;
        for (idx = 0; idx < pmc[j]->num_phys; idx++) {
             pa = _get_phymod_phy_access(pmc[j], idx);
             if (!pa) {
                  LOG_ERROR(BSL_LS_SOC_COMMON,
                            (BSL_META_U(unit,
                                        "ERROR: phymod phy Access on port%d is NULL\n"),
                             j));
                  return (SOC_E_INTERNAL);
             }

            rc = soc_phyctrl_speed_get(unit, local_ports[j], &speed);
            if (rc != SOC_E_NONE) {
                _SOC_EXIT_WITH_ERR(rc, (_SOC_MSG("soc_phyctrl_speed_get failed")));
            }
            eyescan_options.linerate_in_khz = speed * 1000;
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  " Measure lowbar eye on port%d speed=%d\n"),
                       j, speed));
            rc = phymod_phy_meas_lowber_eye(pa, eyescan_options, buffer);
            if (rc) {
                 LOG_CLI((BSL_META_U(unit, "Error: start ber scan test rc = %d\n"), rc));
                 return (rc);
            }
            rc = phymod_phy_display_lowber_eye(pa, eyescan_options, buffer);
            if (rc) {
                 LOG_CLI((BSL_META_U(unit, "Error: start ber scan test rc = %d\n"), rc));
                 return (rc);
            }

        }
    }



exit:
    SOC_FREE(start_time);

    if (v_init_offset) {

        /*disable eyescan mode*/
        for (j = 0; j < nof_ports; j++) {
            /*if serdes is unlock continue*/
            if (local_ports != NULL) 
                if (local_ports[j] == -1) continue;
            for (idx = 0; idx < pmc[j]->num_phys; idx++) {
                pa = _get_phymod_phy_access(pmc[j], idx);
                if (!pa) return (SOC_E_INTERNAL);

                rc = phymod_phy_pmd_ber_end_cmd(pa, 0, 2000);
                if (rc) return (rc);
            }
        }
        SOC_FREE(v_init_offset);
    }

    /* check if the lane num needs to be recovered */
    if (lane_num != NULL) {
        phy_ctrl_t *pc;               
        for (j = 0 ; j < nof_ports ; j++) {
            if (local_ports != NULL) {
                if (local_ports[j] == -1) continue;
                pc = INT_PHY_SW_STATE(unit, local_ports[j]);
                if (local_lane_num != NULL) {
                    pc->lane_num = local_lane_num[j];
                }
            }
        }
    }

    SOC_FREE(pmc);
    SOC_FREE(local_ports);
    SOC_FREE(local_lane_num);

    SOC_FUNC_RETURN;

}
static int _soc_eye_scan_get_local_ports(int unit, int inst, int nof_ports, 
                                         soc_port_t*  ports, soc_port_t* local_ports)
{
    pbmp_t              valid_ports_bitmap;
    int                 j, dev, intf/*, rc*/;
    uint32              is_enable;


    /*get valid ports*/
    SOC_PBMP_ASSIGN(valid_ports_bitmap, SOC_INFO(unit).port.bitmap);

    dev  = PHY_DIAG_INST_DEV(inst);
    intf = (dev == PHY_DIAG_DEV_EXT)? PHY_DIAG_INST_INTF(inst) : PHY_DIAG_INTF_DFLT;

    for(j = 0 ; j < nof_ports ; j++) {
        if (SOC_PBMP_MEMBER(valid_ports_bitmap, ports[j])) {
            if (dev == PHY_DIAG_DEV_EXT) {
                   /*rc = */soc_phyctrl_redirect_control_get(unit, ports[j], dev, -1, 
                                                      (intf == PHY_DIAG_INTF_SYS),
                                                      SOC_PHY_CONTROL_RX_SEQ_DONE,
                                                      &is_enable);
                   /*_SOC_IF_ERR_EXIT(rc);*/
            } else {
                   /*rc = */soc_phyctrl_control_get(unit, ports[j],
                                             SOC_PHY_CONTROL_RX_SEQ_DONE,
                                             &is_enable);
                   /*_SOC_IF_ERR_EXIT(rc);*/
            }

            local_ports[j] = (is_enable)? ports[j] : -1;
        } else {
             LOG_ERROR(BSL_LS_SOC_COMMON,
                       (BSL_META_U(unit,
                                   "Exit on Invalid port%d \n"),
                        j));
             return (SOC_E_PORT);
        }
    }
    return (0);
}

int _soc_port_phy_eyescan_run_uc( 
    int                             unit, 
    uint32                          inst,	/* phy instance containing phy number and interface */
    int                             flags, 
    soc_port_phy_eyescan_params_t*  params, 
    uint32                          nof_ports,
    soc_port_t*                     ports,
    int*                            lane_num,
    soc_port_phy_eyescan_results_t* results)    /*array of result per port*/
{
    int                 j; 
    int*                local_lane_num = NULL;
    soc_port_t*         local_ports = NULL;
    soc_phymod_ctrl_t **pmc = NULL;
    const phymod_phy_access_t    *pa;
    int                 idx, ii, rc;
    uint32_t            stripe[64];
    uint16_t            status;

    SOC_INIT_FUNC_DEFS;

    /*validate input*/
    if (!SOC_UNIT_VALID(unit)) {
        _SOC_EXIT_WITH_ERR(SOC_E_UNIT, (_SOC_MSG("Invalid unit")));
    }
    SOC_NULL_CHECK(params);
    SOC_NULL_CHECK(ports);
    SOC_NULL_CHECK(results);

    /* Allocate Mem */
    local_ports    = (soc_port_t*)        sal_alloc(sizeof(soc_port_t)*nof_ports,"eyecan local ports");
    local_lane_num = (int*)               sal_alloc(sizeof(int)*nof_ports,"eyecan local_lane_num");
    pmc            = (soc_phymod_ctrl_t**)sal_alloc(sizeof(soc_phymod_ctrl_t*)*nof_ports,"eyescan phymod_ctrl pointers");

    if((NULL == local_ports) || (NULL == local_lane_num) || (NULL == pmc)) {
         LOG_ERROR(BSL_LS_SOC_COMMON,
                   (BSL_META_U(unit,
                               "lcl=%p, Ln=%p, pmc=%p\n"),
                    local_ports, lane_num, pmc));
        _SOC_EXIT_WITH_ERR(SOC_E_MEMORY, (_SOC_MSG("Failed to allocate Memory")));
    }

    /* Initialize Local Ports */
    _SOC_IF_ERR_EXIT(_soc_eye_scan_get_local_ports(unit, inst, nof_ports, ports, local_ports));

    for(j = 0 ; j < nof_ports ; j++) {
        phy_ctrl_t *pc;

        if (local_ports[j] == -1) continue;

        pc = INT_PHY_SW_STATE(unit, local_ports[j]);
        if (pc == NULL) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "Err: %s:%d PC = %p..\n"),
                       __func__, __LINE__, pc));
            return (SOC_E_INTERNAL);
        }

        pmc[j] = &pc->phymod_ctrl;
        if (pmc[j] == NULL) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "Err: %s:%d pmc[%d] = %p..\n"),
                       __func__, __LINE__, j, pc));
            return (SOC_E_INTERNAL);
        }

        if (lane_num != NULL) {
            local_lane_num[j] = pc->lane_num;
            pc->lane_num = lane_num[j];
        }

        /*enable eyescan*/
        for (idx = 0; idx < pmc[j]->num_phys; idx++) {
             pa = _get_phymod_phy_access(pmc[j], idx);
             if (!pa) {
                  LOG_ERROR(BSL_LS_SOC_COMMON,
                            (BSL_META_U(unit,
                                        "ERROR: phymod Access on port%d is NULL\n"),
                             j));
                  return (SOC_E_INTERNAL);
             }
            _SOC_IF_ERR_EXIT(phymod_phy_meas_eye_scan_start(pa, 0)); /* 0 = Horiz, 1 = Vertical */
        }
    }


    sal_usleep(100000);

    /* Read Scan Stripe */
    for (rc=0, j = 0; j < nof_ports; j++) {
        if (local_ports[j] == -1) continue;
        for (idx = 0; idx < pmc[j]->num_phys; idx++) {
            pa = _get_phymod_phy_access(pmc[j], idx);
            if (!pa) {
                  LOG_ERROR(BSL_LS_SOC_COMMON,
                            (BSL_META_U(unit,
                                        "ERROR: phymod Access on port%d is NULL\n"),
                             local_ports[j]));
                  return (SOC_E_INTERNAL);
            }
            for (ii = 31; ii >= -31; ii--) { 
                if (ii == 31) {
                    LOG_ERROR(BSL_LS_SOC_COMMON,
                              (BSL_META_U(unit,
                                          "\n\n\n")));
	            LOG_ERROR(BSL_LS_SOC_COMMON,
	                      (BSL_META_U(unit,
	                                  "    +--------------------------------------------------------------------+\n")));
                    LOG_ERROR(BSL_LS_SOC_COMMON,
                              (BSL_META_U(unit,
                                          "    | EYESCAN Port: %02d Phy: %02d                                           |\n"),
                               local_ports[j], idx));
	            LOG_ERROR(BSL_LS_SOC_COMMON,
	                      (BSL_META_U(unit,
	                                  "    +--------------------------------------------------------------------+\n")));
                    /*display eyescan header*/
                    phymod_phy_display_eye_scan_header(_get_phymod_phy_access(pmc[0], 0), 1);
                }
                /*if serdes is unlock continue*/
                sal_usleep(100000);
                rc = phymod_phy_read_eye_scan_stripe(pa, stripe, &status); 
                if (rc) {
                      phymod_phy_eye_scan_debug_info_dump(pa);
                      LOG_ERROR(BSL_LS_SOC_COMMON,
                                (BSL_META_U(unit,
                                            "ERROR:rc=0x%X: %s:%d phy=%d..\n"),
                                 rc, __func__, __LINE__, idx));
                      break;
                }
                phymod_phy_display_eye_scan_stripe(pa, ii,&stripe[0]);
                LOG_ERROR(BSL_LS_SOC_COMMON,
                          (BSL_META_U(unit,
                                      "\n")));
            }
            if (!rc) {
                phymod_phy_display_eye_scan_footer(_get_phymod_phy_access(pmc[0], 0), 1);
                LOG_ERROR(BSL_LS_SOC_COMMON,
                          (BSL_META_U(unit,
                                      "\n")));
            }
        }
    }

exit:

    /*disable eyescan mode*/
    for (j = 0; j < nof_ports; j++) {

        if (local_ports[j] == -1) continue;
        for (idx = 0; idx < pmc[j]->num_phys; idx++) {
            pa = _get_phymod_phy_access(pmc[j], idx);
            if (!pa) {
                  LOG_ERROR(BSL_LS_SOC_COMMON,
                            (BSL_META_U(unit,
                                        "ERROR: phymod Access on port%d is NULL\n"),
                             local_ports[j]));
                  return (SOC_E_INTERNAL);
            }
            _SOC_IF_ERR_EXIT(phymod_phy_meas_eye_scan_done(pa));
        }
    }

    /* check if the lane num needs to be recovered */
    if (lane_num != NULL) {
        phy_ctrl_t *pc;               
        for (j = 0 ; j < nof_ports ; j++) {
            if (local_ports[j] == -1) continue;
            pc = INT_PHY_SW_STATE(unit, local_ports[j]);
            if (local_lane_num != NULL) {
                pc->lane_num = local_lane_num[j];
            }
        }
    }

    SOC_FREE(pmc);
    SOC_FREE(local_ports);
    SOC_FREE(local_lane_num);

    SOC_FUNC_RETURN;
}
#endif

int soc_port_phy_eyescan_run( 
    int unit, 
    uint32 inst,            /* phy instance containing phy number and interface */
    int flags, 
    soc_port_phy_eyescan_params_t* params, 
    uint32 nof_ports,
    soc_port_t* ports,
    int* lane_num,
    soc_port_phy_eyescan_results_t* results /*array of result per port*/) 
{
#ifdef PHYMOD_SUPPORT
    return (is_eyescan_algorithm_legacy_mode(unit, nof_ports, ports)?  
               _soc_port_phy_eyescan_run  : _soc_port_phy_eyescan_run_uc)
		          (unit, inst, flags, params, nof_ports, 
                           ports, lane_num, results);
#else
    return (_soc_port_phy_eyescan_run(unit, inst, flags, params, nof_ports, ports, lane_num, results));
#endif
}


#ifndef __KERNEL__

#ifdef COMPILER_HAS_DOUBLE
STATIC COMPILER_DOUBLE 
_eyescan_util_round_real( COMPILER_DOUBLE original_value, int decimal_places ) 
{
    
    COMPILER_DOUBLE shift_digits[EYESCAN_UTIL_MAX_ROUND_DIGITS+1] = { 0.0, 10.0, 100.0, 1000.0, 
                          10000.0, 100000.0, 1000000.0, 10000000.0, 100000000.0 };
    COMPILER_DOUBLE shifted;
    COMPILER_DOUBLE rounded;   
    
    if (decimal_places > EYESCAN_UTIL_MAX_ROUND_DIGITS ) {
        LOG_CLI((BSL_META("ERROR: Maximum digits to the right of decimal for rounding "
                          "exceeded. Max %d, requested %d\n"), 
                 EYESCAN_UTIL_MAX_ROUND_DIGITS, decimal_places));
        return 0.0;
    } 
    /* shift to preserve the desired digits to the right of the decimal */   
    shifted = original_value * shift_digits[decimal_places];

    /* convert to integer and back to COMPILER_DOUBLE to truncate undesired precision */
    shifted = (COMPILER_DOUBLE)(floor(shifted+0.5));

    /* shift back to place decimal point correctly */   
    rounded = shifted / shift_digits[decimal_places];
    return rounded;
}
#endif /* COMPILER HAS DOUBLE */

#ifdef COMPILER_HAS_DOUBLE
STATIC int  
_eye_margin_diagram_cal(int unit, EYE_DIAG_INFOt *pInfo, soc_port_phy_eyescan_results_t* results) 
{
    COMPILER_DOUBLE lbers[MAX_EYE_LOOPS]; /*Internal linear scale sqrt(-log(ber))*/
    COMPILER_DOUBLE margins[MAX_EYE_LOOPS]; /* Eye margin @ each measurement*/
    COMPILER_DOUBLE bers[MAX_EYE_LOOPS]; /* computed bit error rate */
    int delta_n;
    COMPILER_DOUBLE Exy = 0.0;
    COMPILER_DOUBLE Eyy = 0.0;
    COMPILER_DOUBLE Exx = 0.0;
    COMPILER_DOUBLE Ey  = 0.0;
    COMPILER_DOUBLE Ex  = 0.0;
    COMPILER_DOUBLE alpha;
    COMPILER_DOUBLE beta;
    COMPILER_DOUBLE proj_ber;
    COMPILER_DOUBLE proj_margin_12;
    COMPILER_DOUBLE proj_margin_15;
    COMPILER_DOUBLE proj_margin_18;
    COMPILER_DOUBLE sq_err2;
    COMPILER_DOUBLE ierr;
    int beta_max=0;
    int ber_conf_scale[20];
    int start_n;
    int stop_n;
    int low_confidence;
    int loop_index;
    COMPILER_DOUBLE outputs[4];
    COMPILER_DOUBLE eye_unit;
    int n_mono;
    int loop_var, hi_confidence_cnt, veye_cnt, veye_cnt_orig ;
    SOC_INIT_FUNC_DEFS;
    
    /* Initialize BER confidence scale */
    ber_conf_scale[0] = 3.02;
    ber_conf_scale[1] = 4.7863;
    ber_conf_scale[2] = 3.1623;
    ber_conf_scale[3] = 2.6303;
    ber_conf_scale[4] = 2.2909;
    ber_conf_scale[5] = 2.138;
    ber_conf_scale[6] = 1.9953;
    ber_conf_scale[7] = 1.9055;
    ber_conf_scale[8] = 1.8197;
    ber_conf_scale[9] = 1.7783;
    ber_conf_scale[10] = 1.6982;
    ber_conf_scale[11] = 1.6596;
    ber_conf_scale[12] = 1.6218;
    ber_conf_scale[13] = 1.6218;
    ber_conf_scale[14] = 1.5849;
    ber_conf_scale[15] = 1.5488;
    ber_conf_scale[16] = 1.5488;
    ber_conf_scale[17] = 1.5136;
    ber_conf_scale[18] = 1.5136;
    ber_conf_scale[19] = 1.4791;
    
    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "first_good_ber_idx: %d, first_small_errcnt_idx: %d\n"),
                 pInfo->first_good_ber_idx,pInfo->first_small_errcnt_idx));

    /* re-calculate the range selected for extroplation */
    hi_confidence_cnt = 0;
    veye_cnt          = 0;
    veye_cnt_orig     = pInfo->veye_cnt ;
    for (loop_var = 0; loop_var < pInfo->offset_max; loop_var++) {
        veye_cnt += 1;
        if (pInfo->total_errs[loop_var] >= HI_CONFIDENCE_ERR_CNT) {
            hi_confidence_cnt = hi_confidence_cnt + 1;
        }
        
        if (((hi_confidence_cnt >= 2) && (pInfo->total_errs[loop_var] <
                                          HI_CONFIDENCE_MIN_ERR_CNT)) ||
            ((hi_confidence_cnt <  2) &&
             (pInfo->total_errs[loop_var] < LO_CONFIDENCE_MIN_ERR_CNT)) ) {
            break;  /* exit for loop */
        }
    }
    pInfo->veye_cnt = veye_cnt ;

    /* Find the highest data point to use, currently based on at least 1E-8 BER level */
    if (pInfo->first_good_ber_idx == INDEX_UNINITIALIZED) {
        start_n = pInfo->veye_cnt;
    } else {
        start_n = pInfo->first_good_ber_idx;
        if(start_n > pInfo->veye_cnt) {
            start_n =  pInfo->veye_cnt;
        }
    }
    stop_n = pInfo->veye_cnt;
    
    /* Find the lowest data point to use */
    if (pInfo->first_small_errcnt_idx == INDEX_UNINITIALIZED) {
        stop_n = pInfo->veye_cnt;
    } else { 
        stop_n = pInfo->first_small_errcnt_idx;
        if(stop_n > pInfo->veye_cnt) {
            stop_n = pInfo->veye_cnt;
        }
    }
    
    /* determine the number of non-monotonic outliers */
    n_mono = 0;
    for (loop_index = start_n; loop_index < stop_n; loop_index++) {
        if (pInfo->mono_flags[loop_index] == 1) {
            n_mono = n_mono + 1;
        }
    } 
    
    eye_unit = VEYE_UNIT;
    
    for (loop_index = 0; loop_index < pInfo->veye_cnt; loop_index++) {
        if (pInfo->total_errs[loop_index] == 0) {
            bers[loop_index] = (1000.0/(COMPILER_DOUBLE)pInfo->total_elapsed_time[loop_index])/pInfo->rate;
        } else {
            bers[loop_index] = (COMPILER_DOUBLE)(1000.0*(COMPILER_DOUBLE)pInfo->total_errs[loop_index])/
                   (COMPILER_DOUBLE)pInfo->total_elapsed_time[loop_index]/pInfo->rate;
        }
        bers[loop_index] /= 1000;
        margins[loop_index] = (pInfo->offset_max-loop_index)*eye_unit;
    }
    
    if( start_n >= pInfo->veye_cnt ) {        
        outputs[0] = -_eyescan_util_round_real(log(bers[pInfo->veye_cnt-1])/log(10), 1);
        outputs[1] = -100.0;
        outputs[2] = -100.0;
        outputs[3] = -100.0;
        /*  No need to print out the decimal portion of the BER */
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "BER *worse* than 1e-%d\n"),
                     (int)outputs[0]));
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "Negative margin @ 1e-12, 1e-15 & 1e-18\n")));
        results->ext_better = -1;
        results->ext_results_int = (int)outputs[0];
        results->ext_results_remainder = 0;
    } else {
        low_confidence = 0;
        if( (stop_n-start_n - n_mono) < 2 ) {
            /* Code triggered when less than 2 statistically valid extrapolation points */
            LOG_CLI((BSL_META_U(unit,
                                "\nWARNING: less accurate extrapolation because of less than 2 statistically valid extrapolation points.\n")));
            for( loop_index = stop_n; loop_index < pInfo->veye_cnt; loop_index++ ) {
                if( pInfo->total_errs[loop_index] < 20 ) {
                    bers[loop_index] = ber_conf_scale[pInfo->total_errs[loop_index]] * bers[loop_index];
                } else {
                    bers[loop_index] = ber_conf_scale[19] * bers[loop_index];
                }
                pInfo->mono_flags[loop_index] = 0;    /* remove flags; or assess again */
            }
            /* Add artificial point at 100% margin to enable interpolation */
            margins[pInfo->veye_cnt] = 100.0;
            bers[pInfo->veye_cnt] = 0.1;
            low_confidence = 1;
            stop_n = pInfo->veye_cnt + 1;
        }
        
        /* Below this point the code assumes statistically valid point available */
        delta_n = stop_n - start_n - n_mono;
        
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "start_n: %d, stop_n: %d, veye: %d, n_mono: %d\n"),
                     start_n,stop_n,pInfo->veye_cnt,n_mono));
        
        /* Find all the correlations */
        for (loop_index = start_n; loop_index < stop_n; loop_index++) {
            lbers[loop_index] = (COMPILER_DOUBLE)sqrt(-log(bers[loop_index]));
        }
        
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "\tstart=%d, stop=%d, low_confidence=%d\n"),
                     start_n, stop_n, low_confidence));
        for (loop_index = start_n; loop_index < stop_n; loop_index++){
            LOG_VERBOSE(BSL_LS_SOC_COMMON,
                        (BSL_META_U(unit,
                                    "\ttotal_errs[%d]=0x%08x\n"),
                         loop_index,(int)pInfo->total_errs[loop_index]));
            LOG_VERBOSE(BSL_LS_SOC_COMMON,
                        (BSL_META_U(unit,
                                    "\tbers[%d]=%f\n"),
                         loop_index,bers[loop_index]));
            LOG_VERBOSE(BSL_LS_SOC_COMMON,
                        (BSL_META_U(unit,
                                    "\tlbers[%d]=%f\n"),
                         loop_index,lbers[loop_index]));
        }
        
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "delta_n = %d\n"),
                     delta_n));
        
        for( loop_index = start_n; loop_index < stop_n; loop_index++ ) {
            if (pInfo->mono_flags[loop_index] == 0) {
                Exy = Exy + margins[loop_index] * lbers[loop_index]/(COMPILER_DOUBLE)delta_n;
                Eyy = Eyy + lbers[loop_index]*lbers[loop_index]/(COMPILER_DOUBLE)delta_n;
                Exx = Exx + margins[loop_index]*margins[loop_index]/(COMPILER_DOUBLE)delta_n;
                Ey  = Ey + lbers[loop_index]/(COMPILER_DOUBLE)delta_n;
                Ex  = Ex + margins[loop_index]/(COMPILER_DOUBLE)delta_n;
            }
        }
        
        /* Compute fit slope and offset */
        alpha = (Exy - Ey * Ex)/(Exx - Ex * Ex);
        beta = Ey - Ex * alpha;
        
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "Exy=%f, Eyy=%f, Exx=%f, Ey=%f,Ex=%f alpha=%f, beta=%f\n"),
                     Exy,Eyy,Exx,Ey,Ex,alpha,beta));
        
        /* JPA> Due to the limit of floats, I need to test for a maximum Beta of 9.32 */
        if(beta > 9.32){
            beta_max=1;
            LOG_CLI((BSL_META_U(unit,
                                "\n\tWARNING: intermediate float variable is maxed out, what this means is:\n")));
            LOG_CLI((BSL_META_U(unit,
                                "\t\t- The *extrapolated* minimum BER will be reported as 1E-37.\n")));
            LOG_CLI((BSL_META_U(unit,
                                "\t\t- This may occur if the channel is near ideal (e.g. test loopback)\n")));
            LOG_CLI((BSL_META_U(unit,
                                "\t\t- While not discrete, reporting an extrapolated BER < 1E-37 is numerically corect, and informative\n\n")));
        }
        
        
        proj_ber = exp(-beta * beta);
        proj_margin_12 = (sqrt(-log(1e-12)) - beta)/alpha;
        proj_margin_15 = (sqrt(-log(1e-15)) - beta)/alpha;
        proj_margin_18 = (sqrt(-log(1e-18)) - beta)/alpha;
        
        sq_err2 = 0;
        for (loop_index = start_n; loop_index < stop_n; loop_index++) {
            ierr = (lbers[loop_index] - (alpha*margins[loop_index] + beta));
            sq_err2 = sq_err2 + ierr*ierr/(COMPILER_DOUBLE)delta_n;
        }
        
        outputs[0] = -_eyescan_util_round_real(log(proj_ber)/log(10),1);
        outputs[1] = _eyescan_util_round_real(proj_margin_18,1);
        outputs[2] = _eyescan_util_round_real(proj_margin_12,1);
        outputs[3] = _eyescan_util_round_real(proj_margin_15,1);
        
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "\t\tlog1e-12=%f, sq=%f\n"),
                     (COMPILER_DOUBLE)log(1e-12),(COMPILER_DOUBLE)sqrt(-log(1e-12))));
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "\t\talpha=%f\n"),
                     alpha));
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "\t\tbeta=%f\n"),
                     beta));
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "\t\tproj_ber=%f\n"),
                     proj_ber));
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "\t\tproj_margin12=%f\n"),
                     proj_margin_12));
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "\t\tproj_margin12=%f\n"),
                     proj_margin_15));
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "\t\tproj_margin18=%f\n"),
                     proj_margin_18));
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "\t\toutputs[0]=%f\n"),
                     outputs[0]));
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "\t\toutputs[1]=%f\n"),
                     outputs[1]));
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "\t\toutputs[2]=%f\n"),
                     outputs[2]));
        
        /* Extrapolated results, low confidence */
        if( low_confidence == 1 ) {
            if(beta_max){
                LOG_VERBOSE(BSL_LS_SOC_COMMON,
                            (BSL_META_U(unit,
                                        "BER(extrapolated) is *better* than 1e-37\n")));
                LOG_VERBOSE(BSL_LS_SOC_COMMON,
                            (BSL_META_U(unit,
                                        "Margin @ 1e-12    is *better* than %f\n"),
                             outputs[2]));
                LOG_VERBOSE(BSL_LS_SOC_COMMON,
                            (BSL_META_U(unit,
                                        "Margin @ 1e-15    is *better* than %f\n"),
                             outputs[3]));
                LOG_VERBOSE(BSL_LS_SOC_COMMON,
                            (BSL_META_U(unit,
                                        "Margin @ 1e-18    is *better* than %f\n"),
                             outputs[1]));
                results->ext_better = 1;
                results->ext_results_int = 37;
                results->ext_results_remainder = 0;
            }
            else{
                LOG_VERBOSE(BSL_LS_SOC_COMMON,
                            (BSL_META_U(unit,
                                        "BER(extrapolated) is *better* than 1e-%f\n"),
                             outputs[0]));
                LOG_VERBOSE(BSL_LS_SOC_COMMON,
                            (BSL_META_U(unit,
                                        "Margin @ 1e-12    is *better* than %f\n"),
                             outputs[2]));
                LOG_VERBOSE(BSL_LS_SOC_COMMON,
                            (BSL_META_U(unit,
                                        "Margin @ 1e-15    is *better* than %f\n"),
                             outputs[3]));
                LOG_VERBOSE(BSL_LS_SOC_COMMON,
                            (BSL_META_U(unit,
                                        "Margin @ 1e-18    is *better* than %f\n"),
                             outputs[1]));
                results->ext_better = 1;
                results->ext_results_int = (int)outputs[0];
                results->ext_results_remainder = (((int)(outputs[0]*100))%100);
            }
            
        /* JPA> Extrapolated results, high confidence */
        } else {           
            if(beta_max){
                LOG_VERBOSE(BSL_LS_SOC_COMMON,
                            (BSL_META_U(unit,
                                        "BER(extrapolated) = 1e-37\n")));
                LOG_VERBOSE(BSL_LS_SOC_COMMON,
                            (BSL_META_U(unit,
                                        "Margin @ 1e-12    is *better* than %f\n"),
                             outputs[2]));
                LOG_VERBOSE(BSL_LS_SOC_COMMON,
                            (BSL_META_U(unit,
                                        "Margin @ 1e-15    is *better* than %f\n"),
                             outputs[3]));
                LOG_VERBOSE(BSL_LS_SOC_COMMON,
                            (BSL_META_U(unit,
                                        "Margin @ 1e-18    is *better* than %f\n"),
                             outputs[1]));
                results->ext_better = 0;
                results->ext_results_int = 37;
                results->ext_results_remainder = 0;
            }
            else{
                LOG_VERBOSE(BSL_LS_SOC_COMMON,
                            (BSL_META_U(unit,
                                        "BER(extrapolated) = 1e-%4.2f\n"),
                             outputs[0]));
                LOG_VERBOSE(BSL_LS_SOC_COMMON,
                            (BSL_META_U(unit,
                                        "Margin @ 1e-12    = %4.2f%%\n"),
                             outputs[2]));
                LOG_VERBOSE(BSL_LS_SOC_COMMON,
                            (BSL_META_U(unit,
                                        "Margin @ 1e-15    = %4.2f%%\n"),
                             outputs[3]));
                LOG_VERBOSE(BSL_LS_SOC_COMMON,
                            (BSL_META_U(unit,
                                        "Margin @ 1e-18    = %4.2f%%\n"),
                             outputs[1]));
                results->ext_better = 0;
                results->ext_results_int = (int)outputs[0];
                results->ext_results_remainder = (((int)(outputs[0]*100))%100);
            }
        }
    }
    pInfo->veye_cnt = veye_cnt_orig ;
    SOC_FUNC_RETURN;    
}
#else    
STATIC int  
_eye_margin_diagram_cal(int unit, EYE_DIAG_INFOt *pInfo, soc_port_phy_eyescan_results_t* results) 
{
    SOC_INIT_FUNC_DEFS;
    _SOC_EXIT_WITH_ERR(SOC_E_UNAVAIL, (_SOC_MSG("_eye_margin_diagram_cal unavailable with this compiler")));

exit:
    SOC_FUNC_RETURN;    
}
#endif

#ifdef COMPILER_HAS_DOUBLE
STATIC int 
_eye_margin_ber_cal(int unit, EYE_DIAG_INFOt *pInfo) 
{
    COMPILER_DOUBLE bers[MAX_EYE_LOOPS]; /* computed bit error rate */
    COMPILER_DOUBLE margins[MAX_EYE_LOOPS]; /* Eye margin @ each measurement*/
    int loop_var;
    COMPILER_DOUBLE eye_unit;
    COMPILER_DOUBLE curr_ber_log;
    COMPILER_DOUBLE prev_ber_log = 0;
    COMPILER_DOUBLE good_ber_level = -7.8;
    SOC_INIT_FUNC_DEFS;
     
    /* Initialize BER array */
    for (loop_var = 0; loop_var < MAX_EYE_LOOPS; loop_var++) {
        bers[loop_var] = 0.0;
    }
    
    eye_unit = VEYE_UNIT;
    
    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "\nBER measurement at each offset, num_data_points: %d\n"),
                 pInfo->veye_cnt));
    
    for (loop_var = 0; loop_var < pInfo->veye_cnt; loop_var++) { 
        margins[loop_var] = (pInfo->offset_max-loop_var)*eye_unit;
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "BER measurement at offset: %f\n"),
                     margins[loop_var]));
         
        /* Compute BER */
        if (pInfo->total_errs[loop_var] == 0) { 
            bers[loop_var] = 1000.0/(COMPILER_DOUBLE)pInfo->total_elapsed_time[loop_var]/pInfo->rate;
            bers[loop_var] /= 1000;
            
            LOG_VERBOSE(BSL_LS_SOC_COMMON,
                        (BSL_META_U(unit,
                                    "BER @ %04f %% = 1e%04f (%d errors in %d miliseconds)\n"),
                         (COMPILER_DOUBLE)((pInfo->offset_max-loop_var)*eye_unit), 
                         1.0*(log10(bers[loop_var])),
                         pInfo->total_errs[loop_var],
                         pInfo->total_elapsed_time[loop_var]));
        } else { 
            bers[loop_var] = ((COMPILER_DOUBLE)1000.0*((COMPILER_DOUBLE)(pInfo->total_errs[loop_var])))/
                             (COMPILER_DOUBLE)pInfo->total_elapsed_time[loop_var]/pInfo->rate;
            
            /* the rate unit is KHZ, add -3(log10(1/1000)) for actual display  */
            bers[loop_var] /= 1000;
            LOG_VERBOSE(BSL_LS_SOC_COMMON,
                        (BSL_META_U(unit,
                                    "BER @ %2.2f%% = 1e%2.2f (%d errors in %d miliseconds)\n"),
                         (pInfo->offset_max-loop_var)*eye_unit,
                         log10(bers[loop_var]),
                         pInfo->total_errs[loop_var],
                         pInfo->total_elapsed_time[loop_var]));
        }
        curr_ber_log = log10(bers[loop_var]);
        
        /* Detect and record nonmonotonic data points */
        if ((loop_var > 0) && (curr_ber_log > prev_ber_log)) {
            pInfo->mono_flags[loop_var] = 1;
        }
        prev_ber_log = curr_ber_log;
        
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "cur_be_log %2.2f\n"),
                     curr_ber_log));
        /* find the first data point with good BER */
        if ((curr_ber_log <= good_ber_level) && 
            (pInfo->first_good_ber_idx == INDEX_UNINITIALIZED)) { 
            LOG_VERBOSE(BSL_LS_SOC_COMMON,
                        (BSL_META_U(unit,
                                    "cur_be_log %2.2f, loop_var %d\n"),
                         curr_ber_log,loop_var));
            pInfo->first_good_ber_idx = loop_var;
        }
        if ((pInfo->total_errs[loop_var] < HI_CONFIDENCE_MIN_ERR_CNT) && (pInfo->first_small_errcnt_idx == INDEX_UNINITIALIZED)) {
            /* find the first data point with small error count */
            pInfo->first_small_errcnt_idx = loop_var;
        }
    
    }   /* for loop_var */
    
    SOC_FUNC_RETURN;   
}
#endif /* COMPILER_HAS_DOUBLE */

int
soc_port_phy_eyescan_extrapolate(int unit, 
                                 int flags, 
                                 soc_port_phy_eyescan_params_t* params, 
                                 uint32 nof_ports, 
                                 soc_port_t* ports,
                                 soc_port_phy_eyescan_results_t* results){
#ifdef COMPILER_HAS_DOUBLE
    int i, cnt, vertical_i;
    int rv, speed;
    EYE_DIAG_INFOt veye_info;
    SOC_INIT_FUNC_DEFS;
    sal_memset(&veye_info, 0, sizeof(EYE_DIAG_INFOt)); 
    /* extrapolate */
    if (!(flags & SRD_EYESCAN_FLAG_AVOID_EXTRAPOLATION) && (flags & SRD_EYESCAN_FLAG_VERTICAL_ONLY) && 
        (params->counter!=socPortPhyEyescanCounterRelativePhy)){
        
        for (i = 0 ; i < nof_ports; i++) {
            rv = soc_phyctrl_speed_get(unit, ports[i], &speed);
            if (rv != SOC_E_NONE) {
                _SOC_EXIT_WITH_ERR(rv, (_SOC_MSG("soc_phyctrl_speed_get failed")));
            }
            
            veye_info.first_good_ber_idx = INDEX_UNINITIALIZED;
            veye_info.first_small_errcnt_idx = INDEX_UNINITIALIZED;
            veye_info.offset_max = params->bounds.vertical_max;
            veye_info.rate = speed*1000;
    
            cnt = 0;
            for (vertical_i = params->bounds.vertical_min ; vertical_i <= params->bounds.vertical_max; vertical_i++){
                if(vertical_i % params->sample_resolution != 0) {
                   continue;
                }
                cnt++;
                if(cnt == MAX_EYE_LOOPS) {
                    break;
                }
            }
            veye_info.veye_cnt = cnt; 
            for (vertical_i = params->bounds.vertical_min ; vertical_i <= params->bounds.vertical_max; vertical_i++){
                cnt--;
                if(vertical_i % params->sample_resolution != 0) {
                   continue;
                }
                veye_info.total_errs[cnt] = results[i].error_count[SOC_PORT_PHY_EYESCAN_H_INDEX][vertical_i + SOC_PORT_PHY_EYESCAN_V_INDEX];
                veye_info.total_elapsed_time[cnt] = results[i].run_time[SOC_PORT_PHY_EYESCAN_H_INDEX][vertical_i + SOC_PORT_PHY_EYESCAN_V_INDEX];
                veye_info.mono_flags[cnt] = 0;
            }
            if (veye_info.veye_cnt > MAX_EYE_LOOPS) {
                _SOC_EXIT_WITH_ERR(SOC_E_PARAM, (_SOC_MSG("ERROR: veye_cnt > MAX_EYE_LOOPS\n")));
            }
            _eye_margin_ber_cal(unit, &veye_info);
            _eye_margin_diagram_cal(unit, &veye_info, &(results[i]));
            results[i].ext_done = 1;         
        }
    }

exit:
    SOC_FUNC_RETURN;   

#else
    return SOC_E_NONE;
#endif /* COMPILER_HAS_DOUBLE*/
}

#else   /* #ifndef __KERNEL__ */

int
soc_port_phy_eyescan_extrapolate(int unit, 
                                 int flags, 
                                 soc_port_phy_eyescan_params_t* params, 
                                 uint32 nof_ports, 
                                 soc_port_t* ports, 
                                 soc_port_phy_eyescan_results_t* results){
    
    SOC_INIT_FUNC_DEFS;
   
    LOG_CLI((BSL_META_U(unit,
                        "\nExtrapolation is not supported in Linux kernel mode\n")));

    SOC_FUNC_RETURN; 
    
}

#endif /* #ifndef __KERNEL__ */
