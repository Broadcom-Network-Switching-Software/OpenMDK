/*
 * 
 * DO NOT EDIT THIS FILE!
 * This file is auto-generated.
 * Edits to this file will be lost when it is regenerated.
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __BCM53084_A0_BMD_H__
#define __BCM53084_A0_BMD_H__

#include <bmd/bmd.h>
#include <bmd_config.h>
#include <cdk/cdk_types.h>
#include <cdk/cdk_error.h>
#include <bmd/bmd_device.h>
#include <bmdi/bmd_devlist.h>
#include <bmdi/bmd_remap.h>

/* Attach BMD device to a CDK device. */
extern int bcm53084_a0_bmd_attach(
    int unit);

/* Detaches BMD device from a CDK device. */
extern int bcm53084_a0_bmd_detach(
    int unit);

/* Reset switch chip. */
extern int bcm53084_a0_bmd_reset(
    int unit);

/* Initialize switch chip. */
extern int bcm53084_a0_bmd_init(
    int unit);

/* Set port mode. */
extern int bcm53084_a0_bmd_port_mode_set(
    int unit, 
    int port, 
    bmd_port_mode_t mode, 
    uint32_t flags);

/* Get current port mode. */
extern int bcm53084_a0_bmd_port_mode_get(
    int unit, 
    int port, 
    bmd_port_mode_t *mode, 
    uint32_t *flags);

/* Update port mode based on link status. */
extern int bcm53084_a0_bmd_port_mode_update(
    int unit, 
    int port);

/* Create a VLAN. */
extern int bcm53084_a0_bmd_vlan_create(
    int unit, 
    int vlan);

/* Add port to a VLAN. */
extern int bcm53084_a0_bmd_vlan_port_add(
    int unit, 
    int vlan, 
    int port, 
    uint32_t flags);

/* Remove port from a VLAN. */
extern int bcm53084_a0_bmd_vlan_port_remove(
    int unit, 
    int vlan, 
    int port);

/* Get list of ports belonging to VLAN. */
extern int bcm53084_a0_bmd_vlan_port_get(
    int unit, 
    int vlan, 
    int *plist, 
    int *utlist);

/* Destroy VLAN. */
extern int bcm53084_a0_bmd_vlan_destroy(
    int unit, 
    int vlan);

/* Set default VLAN for a port. */
extern int bcm53084_a0_bmd_port_vlan_set(
    int unit, 
    int port, 
    int vlan);

/* Get default VLAN for a port. */
extern int bcm53084_a0_bmd_port_vlan_get(
    int unit, 
    int port, 
    int *vlan);

/* Set spanning tree protocol state. */
extern int bcm53084_a0_bmd_port_stp_set(
    int unit, 
    int port, 
    bmd_stp_state_t state);

/* Get spanning tree protocol state. */
extern int bcm53084_a0_bmd_port_stp_get(
    int unit, 
    int port, 
    bmd_stp_state_t *state);

/* Initialize chip for L2 switching. */
extern int bcm53084_a0_bmd_switching_init(
    int unit);

/* Configure port MAC address. */
extern int bcm53084_a0_bmd_port_mac_addr_add(
    int unit, 
    int port, 
    int vlan, 
    const bmd_mac_addr_t *mac_addr);

/* Delete port MAC address. */
extern int bcm53084_a0_bmd_port_mac_addr_remove(
    int unit, 
    int port, 
    int vlan, 
    const bmd_mac_addr_t *mac_addr);

/* Configure CPU MAC address. */
extern int bcm53084_a0_bmd_cpu_mac_addr_add(
    int unit, 
    int vlan, 
    const bmd_mac_addr_t *mac_addr);

/* Delete CPU MAC address. */
extern int bcm53084_a0_bmd_cpu_mac_addr_remove(
    int unit, 
    int vlan, 
    const bmd_mac_addr_t *mac_addr);

/* Transmit a packet. */
extern int bcm53084_a0_bmd_tx(
    int unit, 
    const bmd_pkt_t *pkt);

/* Submit Rx packet to DMA queue. */
extern int bcm53084_a0_bmd_rx_start(
    int unit, 
    bmd_pkt_t *pkt);

/* Poll for Rx packet complete. */
extern int bcm53084_a0_bmd_rx_poll(
    int unit, 
    bmd_pkt_t **ppkt);

/* Abort Rx DMA. */
extern int bcm53084_a0_bmd_rx_stop(
    int unit);

/* Get statistics counter. */
extern int bcm53084_a0_bmd_stat_get(
    int unit, 
    int port, 
    bmd_stat_t stat, 
    bmd_counter_t *counter);

/* Clear statistics counter. */
extern int bcm53084_a0_bmd_stat_clear(
    int unit, 
    int port, 
    bmd_stat_t stat);

/* Assert test interrupt. */
extern int bcm53084_a0_bmd_test_interrupt_assert(
    int unit);

/* Clear test interrupt. */
extern int bcm53084_a0_bmd_test_interrupt_clear(
    int unit);

/* Download code to subdevice. */
extern int bcm53084_a0_bmd_download(
    int unit, 
    bmd_download_t type, 
    uint8_t *data, 
    int size);

#endif /* __BCM53084_A0_BMD_H__ */
