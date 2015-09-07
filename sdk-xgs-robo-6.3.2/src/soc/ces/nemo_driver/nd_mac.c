/* $Id: nd_mac.c 1.5 Broadcom SDK $
 * Copyright 2011 BATM
 */

#include "pub/nd_api.h"
#include "pub/nd_hw.h"
#include "nd_registers.h"
#include "nd_mac.h"
#include "nd_debug.h"
#include "nd_bit.h"

/********************************************************************************************************/
#ifdef CES16_BCM_VERSION
#define AG_REG_PIF_REV                                                  0x000300
#define AG_REG_PIF_SCRATCH                                              0x000304
#define AG_REG_PIF_COMMAND_CONFIG_1                                     0x000308
#define AG_REG_PIF_COMMAND_CONFIG_2                                     0x00030A
#define AG_REG_PIF_MAC_0                                                0x00030C
#define AG_REG_PIF_MAC_1                                                0x000310
#define AG_REG_PIF_FRM_LENGTH                                           0x000314
#define AG_REG_PIF_PAUSE_QUANT                                          0x000318
#define AG_REG_PIF_RX_SECTION_EMPTY                                     0x00031C
#define AG_REG_PIF_RX_SECTION_FULL                                      0x000320
#define AG_REG_PIF_TX_SECTION_EMPTY                                     0x000324
#define AG_REG_PIF_TX_SECTION_FULL                                      0x000328
#define AG_REG_PIF_RX_ALMOST_EMPTY                                      0x00032C
#define AG_REG_PIF_RX_ALMOST_FULL                                       0x000330
#define AG_REG_PIF_TX_ALMOST_EMPTY                                      0x000334
#define AG_REG_PIF_TX_ALMOST_FULL                                       0x000338
#define AG_REG_PIF_MDIO_ADDR_0                                          0x00033C
#define AG_REG_PIF_MDIO_ADDR_1                                          0x000340
#define AG_REG_PIF_REG_STAT                                             0x000358
#define AG_REG_PIF_TX_IPG_LENGTH                                        0x00035C
/* */
/* PIF   Packet interface statistics registers */
/* */
#define AG_REG_PIF_MACID_0                                              0x000360
#define AG_REG_PIF_MACID_1                                              0x000364
#define AG_REG_PIF_FRAMES_TRANSMITTED_OK                                0x000368
#define AG_REG_PIF_FRAMES_RECEIVED_OK                                   0x00036C
#define AG_REG_PIF_FRAME_CHECK_SEQUENCE_ERRORS                          0x000370
#define AG_REG_PIF_ALIGNMENT_ERRORS                                     0x000374
#define AG_REG_PIF_OCTETS_TRANSMITTED_OK                                0x000378
#define AG_REG_PIF_OCTETS_RECEIVED_OK                                   0x00037C
#define AG_REG_PIF_TX_PAUSE_MAC_CTRL_FRAMES                             0x000380
#define AG_REG_PIF_RX_PAUSE_MAC_CTRL_FRAMES                             0x000384
#define AG_REG_PIF_IN_ERRORS                                            0x000388
#define AG_REG_PIF_OUT_ERRORS                                           0x00038C
#define AG_REG_PIF_IN_UNICAST_PACKETS                                   0x000390
#define AG_REG_PIF_IN_MULTICAST_PACKETS                                 0x000394
#define AG_REG_PIF_IN_BROADCAST_PACKETS                                 0x000398
#define AG_REG_PIF_OUT_DISCARDS                                         0x00039C
#define AG_REG_PIF_OUT_UNICAST_PACKETS                                  0x0003A0
#define AG_REG_PIF_OUT_MULTICAST_PACKETS                                0x0003A4
#define AG_REG_PIF_OUT_BROADCAST_PACKETS                                0x0003A8
#define AG_REG_PIF_STATS_DROP_EVENTS                                    0x0003AC
#define AG_REG_PIF_STATS_OCTETS                                         0x0003B0
#define AG_REG_PIF_STATS_PACKETS                                        0x0003B4
#define AG_REG_PIF_STATS_UNDERSIZE_PACKETS                              0x0003B8
#define AG_REG_PIF_STATS_OVERSIZE_PACKETS                               0x0003BC
#define AG_REG_PIF_STATS_PACKETS_64_OCTETS                              0x0003C0
#define AG_REG_PIF_STATS_PACKETS_65_TO_127_OCTETS                       0x0003C4
#define AG_REG_PIF_STATS_PACKETS_128_TO_255_OCTETS                      0x0003C8
#define AG_REG_PIF_STATS_PACKETS_256_TO_511_OCTETS                      0x0003CC
#define AG_REG_PIF_STATS_PACKETS_512_TO_1023_OCTETS                     0x0003D0
#define AG_REG_PIF_STATS_PACKETS_1024_TO_1518_OCTETS                    0x0003D4

#define AG_REG_PIF_PHY_DEVICE_0_MDIO(entry)                            (0x002300 + AG_ND_32BIT_ENTRY(entry))

#else
/*This is for BCM*/
#define AG_REG_PIF_PIFCSR                                               0x000300   /*PIF Control and status register*/
#define AG_REG_PIF_TXFILLTH                                             0x000310   /*Transmit Fill Threshold*/
#define AG_REG_PIF_TXAFULLTH                                            0x000312   /*Transmit Almost Full Threshold Register*/
#define AG_REG_PIF_TXAEMPTH                                             0x000314   /*Transmit Almost Empty Threshold Register*/
#define AG_REG_PIF_TXPREAMBLE                                           0x000316   /*Transmit Preamble Register*/
#define AG_REG_PIF_RXFILLTH                                             0x000320   /*Receive Fill Threshold Register*/
#endif                                                                       

/*******************************************************************************************************/
/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_mac_config */
/* */

AgResult
ag_nd_opcode_read_config_mac(AgNdDevice *p_device, AgNdMsgConfigMac *p_msg)
{
   #ifndef CES16_BCM_VERSION
    AG_U32                      n_data;
    AgResult                    res = AG_S_OK;
    AgNdMacCmdConfig            *p_cc = NULL;
    AgNdRegMACCmdConfiguration1 s_cmd_conf1;
    AgNdRegMACCmdConfiguration2 s_cmd_conf2;

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);

    /* */
    /* revision */
    /* */
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_REV, &n_data);
    p_msg->n_core_revision = (AG_U16)(n_data & 0xFFFF);
    p_msg->n_customer_specific_revision = (AG_U16)((n_data >> 16) & 0xFFFF);

    /* */
    /* scratch */
    /* */
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_SCRATCH, &(p_msg->n_scratch));

    /* */
    /* MAC address */
    /* */
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_MAC_0, &n_data);
    p_msg->a_mac[0] = (AG_U8) (n_data & 0xFF);
    p_msg->a_mac[1] = (AG_U8) ((n_data >> 8) & 0xFF);
    p_msg->a_mac[2] = (AG_U8) ((n_data >> 16) & 0xFF);
    p_msg->a_mac[3] = (AG_U8) ((n_data >> 24) & 0xFF);

    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_MAC_1, &n_data);
    p_msg->a_mac[4] = (AG_U8) (n_data & 0xFF);
    p_msg->a_mac[5] = (AG_U8) ((n_data >> 8) & 0xFF);

    /* */
    /* frame length */
    /* */
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_FRM_LENGTH, &n_data);
    p_msg->n_frame_length = (AG_U16) (n_data & 0x3FFF);

    /* */
    /* Pause quanta */
    /* */
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_PAUSE_QUANT, &n_data);
    p_msg->n_pause_quanta = (AG_U16) (n_data & 0xFFFF);

    /* */
    /* Thresholds */
    /* */
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_RX_SECTION_EMPTY, &(p_msg->n_rx_section_empty));
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_RX_SECTION_FULL, &(p_msg->n_rx_section_full));
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_TX_SECTION_EMPTY, &(p_msg->n_tx_section_empty));
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_TX_SECTION_FULL, &(p_msg->n_tx_section_full));
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_RX_ALMOST_EMPTY, &(p_msg->n_rx_almost_empty));
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_RX_ALMOST_FULL, &(p_msg->n_rx_almost_full));
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_TX_ALMOST_EMPTY, &(p_msg->n_tx_almost_empty));
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_TX_ALMOST_FULL, &(p_msg->n_tx_almost_full));

    /* */
    /* PHYs addresses */
    /* */
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_MDIO_ADDR_0, &n_data);
    p_msg->n_mdio_address_0 = (AG_U8)(n_data & 0x1F);
/*    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_MDIO_ADDR_1, &n_data); */
/*    p_msg->n_mdio_address_1 = (AG_U8)(n_data & 0x1F); */

    /* */
    /* status register */
    /*  */
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_REG_STAT, &(p_msg->n_reg_stat));

    /* */
    /* inter packet gap */
    /* */
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_TX_IPG_LENGTH, &n_data);
    p_msg->n_tx_inter_packet_gap_length = (AG_U8)(n_data & 0x1F);

    /* */
    /* read command config */
    /* */
    p_cc = &(p_msg->x_command_config);

    ag_nd_reg_read(p_device,AG_REG_PIF_COMMAND_CONFIG_1,&(s_cmd_conf1.n_reg));

    p_cc->b_rx_error_discard_enable = s_cmd_conf1.x_fields.b_rx_err_disc;
    p_cc->b_ena_10mbps = s_cmd_conf1.x_fields.b_ena_10;
    p_cc->b_no_length_check = s_cmd_conf1.x_fields.b_no_lgth_check;
    p_cc->b_control_frame_enable = s_cmd_conf1.x_fields.b_cntl_frm_ena;
    p_cc->b_node_wake_up_request_indication = s_cmd_conf1.x_fields.b_wakeup;
    p_cc->b_put_core_in_sleep_mode = s_cmd_conf1.x_fields.b_sleep;
    p_cc->b_enable_magic_packet_detection = s_cmd_conf1.x_fields.b_magic_ena;
    p_cc->e_tx_address_selection = s_cmd_conf1.x_fields.n_tx_addr_sel;
                        
    ag_nd_reg_read(p_device,AG_REG_PIF_COMMAND_CONFIG_2,&(s_cmd_conf2.n_reg));
    p_cc->b_enable_loopback = s_cmd_conf2.x_fields.b_loop_ena;
    p_cc->b_hash_24_bit_only = s_cmd_conf2.x_fields.b_mhash_sel;
    p_cc->b_software_reset = s_cmd_conf2.x_fields.b_sw_reset;
    p_cc->b_is_late_collision_condition = s_cmd_conf2.x_fields.b_late_col;
    p_cc->b_is_excessive_collision_condition = s_cmd_conf2.x_fields.b_excess_col;
    p_cc->b_enable_half_duplex = s_cmd_conf2.x_fields.b_hd_ena;
    p_cc->b_insert_mac_addr_on_transmit = s_cmd_conf2.x_fields.b_tx_addr_ins;
    p_cc->b_ignore_pause_frame_quanta = s_cmd_conf2.x_fields.b_pause_ignore;
    p_cc->b_fwd_pause_frames = s_cmd_conf2.x_fields.b_pause_fwd;
    p_cc->b_fwd_crc_field = s_cmd_conf2.x_fields.b_crc_fwd;
    p_cc->b_enable_frame_padding = s_cmd_conf2.x_fields.b_pad_en;
    p_cc->b_enable_promiscuous_mode = s_cmd_conf2.x_fields.b_promis_en;
    p_cc->b_enable_gigabit_ethernet = s_cmd_conf2.x_fields.b_eth_speed;
    p_cc->b_enable_mac_receive = s_cmd_conf2.x_fields.b_rx_ena;
    p_cc->b_enable_mac_transmit = s_cmd_conf2.x_fields.b_tx_ena;
    return res;
   #else
    return AG_S_OK;
   #endif
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_mac_config */
/* */

AgResult
ag_nd_opcode_write_config_mac(AgNdDevice *p_device, AgNdMsgConfigMac *p_msg)
{
   #ifndef CES16_BCM_VERSION
    AG_U32 n_data;
    AgNdRegMACCmdConfiguration1 s_cmd_conf1;
    AgNdRegMACCmdConfiguration2 s_cmd_conf2;
    AgNdMacCmdConfig            *p_cc = NULL;


    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "%s%s\n", AG_ND_TRACE_API_CALL, __func__);

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_core_revision=%hu\n", p_msg->n_core_revision);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_customer_specific_revision=%hu\n", p_msg->n_customer_specific_revision);

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_frame_length=%hu\n", p_msg->n_frame_length);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_mdio_address_0=%hhu\n", p_msg->n_mdio_address_0);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_pause_quanta=%hu\n", p_msg->n_pause_quanta);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_reg_stat=%lu\n", p_msg->n_reg_stat);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_scratch=%lu\n", p_msg->n_scratch);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_tx_inter_packet_gap_length=%hhu\n", p_msg->n_tx_inter_packet_gap_length);

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_rx_almost_empty=%lu\n", p_msg->n_rx_almost_empty);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_rx_almost_full=%lu\n", p_msg->n_rx_almost_full);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_rx_section_empty=%lu\n", p_msg->n_rx_section_empty);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_rx_section_full=%lu\n", p_msg->n_rx_section_full);

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_tx_almost_empty=%lu\n", p_msg->n_tx_almost_empty);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_tx_almost_full=%lu\n", p_msg->n_tx_almost_full);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_tx_section_empty=%lu\n", p_msg->n_tx_section_empty);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "n_tx_section_full=%lu\n", p_msg->n_tx_section_full);

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_control_frame_enable=%lu\n", p_msg->x_command_config.b_control_frame_enable);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_enable_frame_padding=%lu\n", p_msg->x_command_config.b_enable_frame_padding);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_enable_half_duplex=%lu\n", p_msg->x_command_config.b_enable_half_duplex);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_enable_mac_receive=%lu\n", p_msg->x_command_config.b_enable_mac_receive);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_enable_mac_transmit=%lu\n", p_msg->x_command_config.b_enable_mac_transmit);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_enable_magic_packet_detection=%lu\n", p_msg->x_command_config.b_enable_magic_packet_detection);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_enable_promiscuous_mode=%lu\n", p_msg->x_command_config.b_enable_promiscuous_mode);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_fwd_crc_field=%lu\n", p_msg->x_command_config.b_fwd_crc_field);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_fwd_pause_frames=%lu\n", p_msg->x_command_config.b_fwd_pause_frames);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_ignore_pause_frame_quanta=%lu\n", p_msg->x_command_config.b_ignore_pause_frame_quanta);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_insert_mac_addr_on_transmit=%lu\n", p_msg->x_command_config.b_insert_mac_addr_on_transmit);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_no_length_check%lu\n", p_msg->x_command_config.b_no_length_check);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_put_core_in_sleep_mode=%lu\n", p_msg->x_command_config.b_put_core_in_sleep_mode);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_rx_error_discard_enable=%lu\n", p_msg->x_command_config.b_rx_error_discard_enable);
    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "b_software_reset=%lu\n", p_msg->x_command_config.b_software_reset);

    AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, "mac=%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n",
            p_msg->a_mac[0],
            p_msg->a_mac[1],
            p_msg->a_mac[2],
            p_msg->a_mac[3],
            p_msg->a_mac[4],
            p_msg->a_mac[5]);



#ifdef AG_ND_ENABLE_VALIDATION
    /* */
    /* sanity checks */
    /* */

    if (p_msg->n_frame_length > 0x3FFF)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if (p_msg->n_pause_quanta > 0xFFFF)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

    if (p_msg->n_mdio_address_0 > 0x1F)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);

/*    if (p_msg->n_mdio_address_1 > 0x1F) */
/*    { */
/*        AG_ND_TRACE(p_device, AG_ND_TRACE_API, AG_ND_TRACE_DEBUG, */
/*            "ag_nd_opcode_write_mac_config erroneous MDIO_1 address:%hhu\n", (AG_U32)p_msg->n_mdio_address_1); */
/*        return AG_ND_ERR(p_device, AG_E_ND_ARG); */
/*    } */
#endif



    /* */
    /* scratch */
    /* */
    ag_nd_reg_write_32bit(p_device, AG_REG_PIF_SCRATCH, p_msg->n_scratch);

    /* */
    /* MAC address */
    /*  */
    n_data =   (AG_U32)(p_msg->a_mac[0]);
    n_data |= ((AG_U32)(p_msg->a_mac[1])) << 8;
    n_data |= ((AG_U32)(p_msg->a_mac[2])) << 16;
    n_data |= ((AG_U32)(p_msg->a_mac[3])) << 24;
    ag_nd_reg_write_32bit(p_device, AG_REG_PIF_MAC_0, n_data);

    n_data =   (AG_U32)(p_msg->a_mac[4]);
    n_data |= ((AG_U32)(p_msg->a_mac[5])) << 8;
    ag_nd_reg_write_32bit(p_device, AG_REG_PIF_MAC_1, n_data);

    /* */
    /* frame length */
    /* */
    ag_nd_reg_write_32bit(p_device, AG_REG_PIF_FRM_LENGTH, (AG_U32)p_msg->n_frame_length);

    /* */
    /* pausee quanta */
    /* */

    ag_nd_reg_write_32bit(p_device, AG_REG_PIF_PAUSE_QUANT, 0xFFFF & (AG_U32)p_msg->n_pause_quanta);

    /* */
    /* thresholds */
    /* */
    ag_nd_reg_write_32bit(p_device, AG_REG_PIF_RX_SECTION_EMPTY, p_msg->n_rx_section_empty);
    ag_nd_reg_write_32bit(p_device, AG_REG_PIF_RX_SECTION_FULL, p_msg->n_rx_section_full);
    ag_nd_reg_write_32bit(p_device, AG_REG_PIF_TX_SECTION_EMPTY, p_msg->n_tx_section_empty);
    ag_nd_reg_write_32bit(p_device, AG_REG_PIF_TX_SECTION_FULL, p_msg->n_tx_section_full);
    ag_nd_reg_write_32bit(p_device, AG_REG_PIF_RX_ALMOST_EMPTY, p_msg->n_rx_almost_empty);
    ag_nd_reg_write_32bit(p_device, AG_REG_PIF_RX_ALMOST_FULL, p_msg->n_rx_almost_full);
    ag_nd_reg_write_32bit(p_device, AG_REG_PIF_TX_ALMOST_EMPTY, p_msg->n_tx_almost_empty);
    ag_nd_reg_write_32bit(p_device, AG_REG_PIF_TX_ALMOST_FULL, p_msg->n_tx_almost_full);

    /* */
    /* PHY addresses */
    /* */
    ag_nd_reg_write_32bit(p_device, AG_REG_PIF_MDIO_ADDR_0, (AG_U32)p_msg->n_mdio_address_0);
/*    ag_nd_reg_write_32bit(p_device, AG_REG_PIF_MDIO_ADDR_1, (AG_U32)p_msg->n_mdio_address_1); */


    /* */
    /* interpacket gap */
    /* */
    ag_nd_reg_write_32bit(p_device, AG_REG_PIF_TX_IPG_LENGTH, (AG_U32)p_msg->n_tx_inter_packet_gap_length);

    /* */
    /* Handle Command Config Register */
    /* */
    s_cmd_conf1.n_reg = 0;
    s_cmd_conf2.n_reg = 0;

    p_cc = &(p_msg->x_command_config);

    s_cmd_conf1.x_fields.b_rx_err_disc = (field_t) p_cc->b_rx_error_discard_enable;
    s_cmd_conf1.x_fields.b_no_lgth_check = (field_t) p_cc->b_no_length_check;
    s_cmd_conf1.x_fields.b_cntl_frm_ena = (field_t) p_cc->b_control_frame_enable;
    s_cmd_conf1.x_fields.b_sleep = (field_t) p_cc->b_put_core_in_sleep_mode;
    s_cmd_conf1.x_fields.b_magic_ena = (field_t) p_cc->b_enable_magic_packet_detection;
    s_cmd_conf2.x_fields.b_sw_reset = (field_t) p_cc->b_software_reset;
    s_cmd_conf2.x_fields.b_hd_ena = (field_t) p_cc->b_enable_half_duplex;
    s_cmd_conf2.x_fields.b_pause_ignore = (field_t) p_cc->b_ignore_pause_frame_quanta;
    s_cmd_conf2.x_fields.b_pause_fwd = (field_t) p_cc->b_fwd_pause_frames;
    s_cmd_conf2.x_fields.b_crc_fwd = (field_t) p_cc->b_fwd_crc_field;
    s_cmd_conf2.x_fields.b_pad_en = (field_t) p_cc->b_enable_frame_padding;

	if (p_device->b_ptp_support)
	    s_cmd_conf2.x_fields.b_promis_en = (field_t)1 ;/* p_cc->b_enable_promiscuous_mode; */
	else
    	s_cmd_conf2.x_fields.b_promis_en = (field_t)p_cc->b_enable_promiscuous_mode;

    s_cmd_conf2.x_fields.b_rx_ena = (field_t) p_cc->b_enable_mac_receive;
    s_cmd_conf2.x_fields.b_tx_ena = (field_t) p_cc->b_enable_mac_transmit;
    /* */
    /* Ovverride HW configuration for eth speed. It should be already configured  */
    /* by FPGA, but override it to be on the safe side. */
    /*  */
    if (AG_ND_ARCH_MASK_NEMO & p_device->n_chip_mask)
    {
        /* */
        /* set to 100Mbps */
        /*  */
        s_cmd_conf1.x_fields.b_ena_10 = (field_t) 0;
        s_cmd_conf2.x_fields.b_eth_speed = (field_t) 0;
    }
    else
    {
        /* */
        /* set to 1000Mbps */
        /*  */
        s_cmd_conf1.x_fields.b_ena_10 = (field_t) 0;
        s_cmd_conf2.x_fields.b_eth_speed = (field_t) 1;
    }

    /*  */
    /* the supplemental MAC addresses feature is not supported (exists, but isn't  */
    /* tested/verified), the only configration permitted is to MAC address insertion on TX */
    /*  */
    s_cmd_conf2.x_fields.b_tx_addr_ins = (field_t) p_cc->b_insert_mac_addr_on_transmit;
    s_cmd_conf1.x_fields.n_tx_addr_sel = (field_t) p_cc->e_tx_address_selection = 0; 

    /* */
    /* multicast filtering is not supported (exists, but isn't tested/verified) */
    /*  */
    s_cmd_conf2.x_fields.b_mhash_sel = (field_t)0;

    /* */
    /* Ovverride HW configuration for eth speed. It should be already configured  */
    /* by FPGA, but override it to be on the safe side. */
    /*  */
    if (AG_ND_ARCH_MASK_NEMO & p_device->n_chip_mask)
    {
        /* */
        /* set to 100Mbps */
        /*  */
        s_cmd_conf1.x_fields.b_ena_10 = (field_t) 0;
        s_cmd_conf2.x_fields.b_eth_speed = (field_t) 0;
    }
    else
    {
        /* */
        /* set to 1000Mbps */
        /*  */
        s_cmd_conf1.x_fields.b_ena_10 = (field_t) 0;
        s_cmd_conf2.x_fields.b_eth_speed = (field_t) 1;
    }

    /*  */
    /* the supplemental MAC addresses feature is not supported (exists, but isn't  */
    /* tested/verified), the only configration permitted is to MAC address insertion on TX */
    /*  */
    s_cmd_conf2.x_fields.b_tx_addr_ins = (field_t) p_cc->b_insert_mac_addr_on_transmit;
    s_cmd_conf1.x_fields.n_tx_addr_sel = (field_t) p_cc->e_tx_address_selection = 0; 

    /* */
    /* MAC loopback is not supported (ifdefed in VLSI design) */
    /*  */
    s_cmd_conf2.x_fields.b_loop_ena = (field_t) 0;

    /* */
    /* multicast filtering is not supported (exists, but isn't tested/verified) */
    /*  */
    s_cmd_conf2.x_fields.b_mhash_sel = (field_t) 0;

    n_data = ((s_cmd_conf1.n_reg << 16) | s_cmd_conf2.n_reg);
    ag_nd_reg_write_32bit(p_device, AG_REG_PIF_COMMAND_CONFIG_1, n_data);
   #endif

    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_mac_pm */
/* */
AgResult
ag_nd_opcode_read_mac_pm(AgNdDevice *p_device, AgNdMsgPmMac *p_msg)
{
   #ifndef CES16_BCM_VERSION
    AG_U32 n_data;

    AG_ND_TRACE(p_device, AG_ND_TRACE_PERIODIC, AG_ND_TRACE_DEBUG, "ag_nd_opcode_read_mac_PM\n");
  

    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_MAC_0, &n_data);
    p_msg->a_mac_id[0] = (AG_U8) (n_data & 0xFF);
    p_msg->a_mac_id[1] = (AG_U8) ((n_data >> 8) & 0xFF);
    p_msg->a_mac_id[2] = (AG_U8) ((n_data >> 16) & 0xFF);
    p_msg->a_mac_id[3] = (AG_U8) ((n_data >> 24) & 0xFF);

    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_MAC_1, &n_data);
    p_msg->a_mac_id[4] = (AG_U8) (n_data & 0xFF);
    p_msg->a_mac_id[5] = (AG_U8) ((n_data >> 8) & 0xFF);

    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_FRAMES_TRANSMITTED_OK,             &(p_msg->n_frames_transmitted_ok));
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_FRAMES_RECEIVED_OK,                &(p_msg->n_frames_received_ok));
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_FRAME_CHECK_SEQUENCE_ERRORS,       &(p_msg->n_frame_check_sequence_errors));
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_ALIGNMENT_ERRORS,                  &(p_msg->n_alignment_errors));
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_OCTETS_TRANSMITTED_OK,             &(p_msg->n_octets_transmitted_ok));
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_OCTETS_RECEIVED_OK,                &(p_msg->n_octets_received_ok));
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_TX_PAUSE_MAC_CTRL_FRAMES,          &(p_msg->n_tx_pause_mac_ctrl_frames));
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_RX_PAUSE_MAC_CTRL_FRAMES,          &(p_msg->n_rx_pause_mac_ctrl_frames));
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_IN_ERRORS,                         &(p_msg->n_if_in_errors));
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_OUT_ERRORS,                        &(p_msg->n_if_out_errors));
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_IN_UNICAST_PACKETS,                &(p_msg->n_if_in_ucast_pkts));
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_IN_MULTICAST_PACKETS,              &(p_msg->n_if_in_multicast_pkts));
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_IN_BROADCAST_PACKETS,              &(p_msg->n_if_in_broadcast_pkts));
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_OUT_DISCARDS,                      &(p_msg->n_if_out_disacrds));
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_OUT_UNICAST_PACKETS,               &(p_msg->n_if_out_ucast_pkts));
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_OUT_MULTICAST_PACKETS,             &(p_msg->n_if_out_multicast_pkts));
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_OUT_BROADCAST_PACKETS,             &(p_msg->n_if_out_broadcast_pkts));
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_STATS_DROP_EVENTS,                 &(p_msg->n_ether_stats_drop_events));
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_STATS_OCTETS,                      &(p_msg->n_ether_stats_octets));
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_STATS_PACKETS,                     &(p_msg->n_ether_stats_pkts));
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_STATS_UNDERSIZE_PACKETS,           &(p_msg->n_ether_stats_undersize_pkts));
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_STATS_OVERSIZE_PACKETS,            &(p_msg->n_ether_stats_oversize_pkts));
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_STATS_PACKETS_64_OCTETS,           &(p_msg->n_ether_stats_pkts_64_octets));
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_STATS_PACKETS_65_TO_127_OCTETS,    &(p_msg->n_ether_stats_pkts_65_to_127_octets));
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_STATS_PACKETS_128_TO_255_OCTETS,   &(p_msg->n_ether_stats_pkts_128_to_255_octets));
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_STATS_PACKETS_256_TO_511_OCTETS,   &(p_msg->n_ether_stats_pkts_256_to_511_octets));
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_STATS_PACKETS_512_TO_1023_OCTETS,  &(p_msg->n_ether_stats_pkts_512_to_1023_octets));
    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_STATS_PACKETS_1024_TO_1518_OCTETS, &(p_msg->n_ether_stats_pkts_1024_to_1518_octets));
   #endif
    return AG_S_OK;
}


/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_read_config_phy */
/* */
AgResult 
ag_nd_opcode_read_config_phy(AgNdDevice *p_device, AgNdMsgConfigPhy *p_msg)
{
  #ifndef CES16_BCM_VERSION

    AG_U32 n_value;


#ifdef AG_ND_ENABLE_VALIDATION
    if (p_msg->n_reg_idx >= AG_ND_PHY_REG_MAX)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif

    ag_nd_reg_read_32bit(p_device, AG_REG_PIF_PHY_DEVICE_0_MDIO(p_msg->n_reg_idx), &n_value);
    p_msg->n_reg_value = (AG_U16)n_value;
  #else
    p_msg->n_reg_value = (AG_U16)0xffff; 
  #endif
    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_opcode_write_config_phy */
/* */
AgResult 
ag_nd_opcode_write_config_phy(AgNdDevice *p_device, AgNdMsgConfigPhy *p_msg)
{
  #ifndef CES16_BCM_VERSION
#ifdef AG_ND_ENABLE_VALIDATION
    if (p_msg->n_reg_idx >= AG_ND_PHY_REG_MAX)
        return AG_ND_ERR(p_device, AG_E_ND_ARG);
#endif


    ag_nd_reg_write_32bit(p_device, AG_REG_PIF_PHY_DEVICE_0_MDIO(p_msg->n_reg_idx), (AG_U32)p_msg->n_reg_value);
  #endif
    return AG_S_OK;
}

/*///////////////////////////////////////////////////////////////////////////// */
/* ag_nd_mac_bit_rw_reg32_one_pass */
/* */
/* Performs one pass of RW test on array of 32 bit registers.  */
/* The test is as follows: */
/*  1. set all the registers to requested value  */
/*  2. verify */
/* */
void
ag_nd_mac_bit_rw_reg32_one_pass(
    AgNdDevice               *p_device, 
    AgNdRegProperties        *reg, 
    AG_U32                   n_wdata,   /* test data */
    AG_U32                   n_offset)  /* offset from the register base address e.g. channel/circuit offset */
{
    AG_U32 i = 0;
    AG_U32 n_rdata;
    AG_U16 n_rdata_lo;
    AG_U16 n_rdata_hi;
    AG_U32 n_addr;
    AG_U16 n_wdata_lo;
    AG_U16 n_wdata_hi;
 

    assert(p_device);



    /* */
    /* write the requested value */
    /* */
    n_wdata = n_wdata & reg->n_mask;
    n_wdata_lo = AG_ND_LOW16(n_wdata);
    n_wdata_hi = AG_ND_HIGH16(n_wdata);

    for (i = 0; i < reg->n_size * 4; i += 4)
    {
        n_addr = reg->n_address + i + n_offset;
        ag_nd_reg_write(p_device, n_addr, n_wdata_hi);
        ag_nd_reg_write(p_device, n_addr + 2, n_wdata_lo);
    }   


    /* */
    /* verify read == written */
    /* */
    for (i = 0; i < reg->n_size * 4; i += 4)
    {
        n_addr = reg->n_address + i + n_offset;
        ag_nd_reg_read(p_device, n_addr, &n_rdata_hi);
        ag_nd_reg_read(p_device, n_addr + 2, &n_rdata_lo);
        n_rdata = AG_ND_MAKE32LH(n_rdata_lo, n_rdata_hi);
        n_rdata &= reg->n_mask;

        ag_nd_bit_rw_word32_report(
            p_device, 
            n_wdata, 
            n_rdata, 
            n_addr, 
            reg->p_name, 
            "%-60s",
            n_rdata == n_wdata);
    }
}


/*///////////////////////////////////////////////////////////////////////////// */
/*  ag_nd_mac_bit_rw_reg32 */
/* */
/* RW register test */
/* */
void
ag_nd_mac_bit_rw_reg32(
    AgNdDevice          *p_device, 
    AgNdRegProperties   *p_reg, 
    AG_U32              n_offset,
    AG_BOOL             b_test)
{
    AgNdRegMACCmdConfiguration1 s_cmd_conf1;
    AgNdRegMACCmdConfiguration2 s_cmd_conf2;


    assert(p_device);
    assert(p_reg);


    /* */
    /* the system requirement is to not interrupt traffic to/from host */
    /* so we do not allow to reset MAC registers while rx or tx are enabled */
    /* during device open/close operations. */
    /* */
    /* from the other hand, BIT is destructive operation by definition and */
    /* does resets the registers. */
    /*  */
    ag_nd_reg_read(p_device, AG_REG_PIF_COMMAND_CONFIG_1, &(s_cmd_conf1.n_reg));
    ag_nd_reg_read(p_device, AG_REG_PIF_COMMAND_CONFIG_2, &(s_cmd_conf2.n_reg));

    if ((s_cmd_conf2.x_fields.b_rx_ena || s_cmd_conf2.x_fields.b_tx_ena) && !b_test)
        return;

    /* */
    /* test register */
    /* */
    if (b_test)
    {
        ag_nd_mac_bit_rw_reg32_one_pass(p_device, p_reg, 0x00000000, n_offset);
        ag_nd_mac_bit_rw_reg32_one_pass(p_device, p_reg, 0xffffffff, n_offset);
        ag_nd_mac_bit_rw_reg32_one_pass(p_device, p_reg, 0x55555555, n_offset);
        ag_nd_mac_bit_rw_reg32_one_pass(p_device, p_reg, 0xaaaaaaaa, n_offset);
    }

    /* */
    /* set register to default value */
    /* */
    ag_nd_mac_bit_rw_reg32_one_pass(p_device, p_reg, p_reg->n_reset, n_offset);
}
    

/*///////////////////////////////////////////////////////////////////////////// */
/*  ag_nd_mac_bit_ro_reg32 */
/* */
/* RW register test */
/* */
void
ag_nd_mac_bit_ro_reg32(
    AgNdDevice          *p_device, 
    AgNdRegProperties   *p_reg, 
    AG_U32              n_offset,
    AG_BOOL             b_test)
{
    AG_U32 i;
    AG_U16 n_rdata;
    AG_U32 n_addr;
    AG_BOOL b_passed = AG_TRUE;
    AG_U32 n_verbose;

    assert(p_device);
    assert(p_reg);


    return;


    if (b_test)
    {
        for (i = 0; i < p_reg->n_size * 2; i += 2)
        {
            n_addr = p_reg->n_address + i + n_offset;
            ag_nd_reg_read(p_device, n_addr, &n_rdata);
    
            if (b_passed)
                n_verbose = AG_ND_BIT_VERBOSE_LEVEL_INSANE;
            else
            {
                n_verbose = AG_ND_BIT_VERBOSE_LEVEL_QUIET;
                p_device->x_bit.b_passed = AG_FALSE;
            }
              
            ag_nd_bit_print(p_device, n_verbose, "%-60s 0x%06lX %s: read 0x%04uX", 
                       p_reg->p_name,
                       n_addr,
                       b_passed ? "passed" : "failed",
                       n_rdata);
        }
    }
}

