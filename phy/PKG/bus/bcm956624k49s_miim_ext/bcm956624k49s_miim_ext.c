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
    } else if (port <= 7) {
        return port - 1;
    } else if (port <= 13) {
        return port + 1;
    } else if (port <= 17) {
        return port + 3;
    } else if (port <= 19) {
        return port - 11;
    } else if (port <= 21) {
        return port - 5;
    } else if (port <= 25) {
        return port - 1;
    } else if (port == 26) {
        return 5 + CDK_XGS_MIIM_BUS_1;
    } else if (port == 27) {
        return 21 + CDK_XGS_MIIM_BUS_1;
    } else if (port <= 31) {
        return port - 27 + CDK_XGS_MIIM_BUS_2;
    } else if (port <= 34) {
        return port - 26 + CDK_XGS_MIIM_BUS_1;
    } else if (port <= 36) {
        return port - 18 + CDK_XGS_MIIM_BUS_1;
    } else if (port <= 40) {
        return port - 36 + CDK_XGS_MIIM_BUS_1;
    } else if (port <= 42) {
        return port - 32 + CDK_XGS_MIIM_BUS_1;
    } else if (port <= 45) {
        return port - 21 + CDK_XGS_MIIM_BUS_1;
    } else if (port <= 47) {
        return port - 27 + CDK_XGS_MIIM_BUS_1;
    } else if (port <= 53) {
        return port - 37 + CDK_XGS_MIIM_BUS_1;
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

phy_bus_t phy_bus_bcm956624k49s_miim_ext = {
    "bcm956624k49s_miim_ext",
    _phy_addr,
    _read,
    _write
};

#else

/* ISO C forbids empty source files */
int bcm956624k49s_miim_int_not_empty;

#endif /* CDK_CONFIG_ARCH_XGS_INSTALLED */
