/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __BMD_PHY_CTRL_H__
#define __BMD_PHY_CTRL_H__

#include <bmd_config.h>

#if BMD_CONFIG_INCLUDE_PHY == 1

#include <phy/phy.h>
#include <phy/phy_drvlist.h>

/* Allow backward comaptibility */
#define BMD_PHY_DRVLIST_INSTALLED

/* Callback used by bmd_phy_reset() */
extern int (*phy_reset_cb)(phy_ctrl_t *pc);

/* Callback used by bmd_phy_init() */
extern int (*phy_init_cb)(phy_ctrl_t *pc);

typedef struct bmd_phy_port_info_s {
    /* List of default PHY buses available for this port */
    phy_bus_t **phy_bus;
    /* Pointer to first (outermost) PHY */
    phy_ctrl_t *phy_ctrl;
} bmd_phy_port_info_t;

typedef struct bmd_phy_info_s {
    bmd_phy_port_info_t phy_port_info[BMD_CONFIG_MAX_PORTS];
} bmd_phy_info_t;

extern bmd_phy_info_t bmd_phy_info[];

/* Default list of PHY drivers based on PHY library configuration */
extern phy_driver_t *bmd_phy_drv_list[];

typedef int (*bmd_phy_probe_func_t)(int, int, phy_driver_t **);

#define BMD_PHY_INFO(_u) bmd_phy_info[_u]

#define BMD_PHY_PORT_INFO(_u,_p) BMD_PHY_INFO(_u).phy_port_info[_p]

#define BMD_PORT_PHY_BUS(_u,_p) BMD_PHY_PORT_INFO(_u,_p).phy_bus
#define BMD_PORT_PHY_CTRL(_u,_p) BMD_PHY_PORT_INFO(_u,_p).phy_ctrl

/*
 * Configure PHY buses
 *
 * Note that the default buses are installed by bmd_attach().
 */
extern int
bmd_phy_bus_set(int unit, int port, phy_bus_t **bus_list);

/*
 * Utility function for PHY probing function
 */
extern int
bmd_phy_bus_get(int unit, int port, phy_bus_t ***bus_list);

extern int
bmd_phy_add(int unit, int port, phy_ctrl_t *pc);

extern phy_ctrl_t *
bmd_phy_del(int unit, int port);

/*
 * Install PHY probing function and drivers
 */
extern int 
bmd_phy_probe_init(bmd_phy_probe_func_t probe, phy_driver_t **drv_list);

/* 
 * Default PHY probing function.
 *
 * Note that this function is not referenced directly from the BMD,
 * but it can be installed using bmd_phy_probe_init().
 */
extern int 
bmd_phy_probe_default(int unit, int port, phy_driver_t **drv_list);

/*
 * Register PHY reset callback function.
 *
 * The callback function can be used for board specific configuration
 * like XAUI lane remapping and/or polarity flip.
 */
extern int 
bmd_phy_reset_cb_register(int (*reset_cb)(phy_ctrl_t *pc));

/*
 * Register PHY init callback function.
 *
 * The callback function can be used for board specific configuration
 * like LED modes, special MDIX setup and PHY-sepcific extensions.
 */
extern int 
bmd_phy_init_cb_register(int (*init_cb)(phy_ctrl_t *pc));

#endif /* BMD_CONFIG_INCLUDE_PHY */

#endif /* __BMD_PHY_CTRL_H__ */
