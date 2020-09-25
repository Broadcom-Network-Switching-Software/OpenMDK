/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Default XGS internal XGXS PHY access functions.
 */

#include <cdk_config.h>

#ifdef CDK_CONFIG_ARCH_XGS_INSTALLED

#include <cdk/cdk_device.h>
#include <cdk/arch/xgs_miim.h>

#include <phy/phy.h>

static uint32_t
_phy_addr(int port)
{
    if (port == 1) {
        return 25 + CDK_XGS_MIIM_BUS_1;
    } else if (port <= 7) {
        return port - 1;
    } else if (port <= 19) {
        return port - 7;
    } else if (port == 26) {
        return 13;
    } else if (port == 27) {
        return 19;
    } else if (port <= 36) {
        return port - 18;
    } else if (port <= 47) {
        return port - 23;
    }
    /* Note: should never get here */
    return port;
}

static int 
_read(int unit, uint32_t addr, uint32_t reg, uint32_t *val)
{
    return cdk_xgs_miim_read(unit, addr, reg, val);
}

static int 
_write(int unit, uint32_t addr, uint32_t reg, uint32_t val)
{
    return cdk_xgs_miim_write(unit, addr, reg, val);
}

phy_bus_t phy_bus_bcm56680_miim_ext = {
    "bcm56680_miim_ext",
    _phy_addr,
    _read,
    _write
};

#else

/* ISO C forbids empty source files */
int bcm56680_miim_ext_not_empty;

#endif /* CDK_CONFIG_ARCH_XGS_INSTALLED */
