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
        return 25;
    } else if (port == 2) {
        return 1 + CDK_XGS_MIIM_BUS_2;
    } else if (port <= 13) {
        return port - 7;
    } else if (port == 14) {
        return 2 + CDK_XGS_MIIM_BUS_2;
    } else if (port <= 25) {
        return port - 13;
    } else if (port <= 31) {
        return port - 23 + CDK_XGS_MIIM_BUS_2;
    } else if (port <= 42) {
        return port - 24;
    } else if (port <= 53) {
        return port - 29;
    }
    /* Note: should never get here */
    return 0;
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

phy_bus_t phy_bus_bcm956629k24s_miim_ext = {
    "bcm956629k24s_miim_ext",
    _phy_addr,
    _read,
    _write
};

#else

/* ISO C forbids empty source files */
int bcm956629k24s_miim_int_not_empty;

#endif /* CDK_CONFIG_ARCH_XGS_INSTALLED */
