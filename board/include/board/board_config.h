/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.1,
 * WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING
 * ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.$
 */

#ifndef __BOARD_CONFIG_H__
#define __BOARD_CONFIG_H__

#include <cdk/cdk_device.h>
#include <phy/phy.h>

typedef struct board_chip_config_s {

    /* Typically used for non-functional ports  */
    cdk_port_map_port_t *skip_ports;

    /* Board-specific PHY bus drivers */
    phy_bus_t **phy_bus_list;

    /* Function to retrieve LED microcode */
    void *(*ledprog_info)(int *);

    /* Dynamic chip configuration for multi-mode devices */
    uint32_t chip_dyn_config;
    int num_port_configs;
    cdk_port_config_t *port_configs;

} board_chip_config_t;

typedef struct board_config_s {

    /* Configuration name */
    const char *cfg_name;

    /* Chip configurations for this board */
    board_chip_config_t **chip_configs;

    /* PHY driver callbacks */
    int (*phy_reset_cb)(phy_ctrl_t *);
    int (*phy_init_cb)(phy_ctrl_t *);

    /* Additional device initialization (e.g. uplinks/trunks) */
    int (*post_init)(int);

} board_config_t;

typedef struct board_config_map_s {
    const char *name;
    board_config_t *bcfg;
} board_config_map_t;

extern board_config_t *
board_config_get(const char *name, board_config_map_t *brd_cfg_map);

#endif /* __BOARD_CONFIG_H__ */
