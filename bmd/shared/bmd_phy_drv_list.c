/*
 *
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * This API abstracts the PHY driver interface to the BMD
 * driver implementations, i.e. a change to the PHY driver 
 * interface should only require changes to this file.
 *
 * If no PHY support is compiled in or no PHYs are detected 
 * for a port, the API will report link up and an invalid
 * (negative) port speed. This behavior simplifies handling
 * of back-to-back MAC configurations as well as simulation
 * environments.
 */

#include <bmd/bmd.h>

#if BMD_CONFIG_INCLUDE_PHY == 1

#include <phy/phy.h>
#include <phy/phy_drvlist.h>

/*
 * Supported PHY drivers
 */
phy_driver_t *bmd_phy_drv_list[] = {
#define PHY_DEVLIST_ENTRY(_un, _ln, _fl, _desc, _r0, _r1) &_ln##_drv,
#include <phy/phy_devlist.h>
    &unknown_drv,
    NULL
};

#endif
