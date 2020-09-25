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
        return 0x19;
    } else if (port < 10) {
        return port + 7;
    } else if (port < 14) {
        return port + 11;
    } else if (port < 26) {
        /* Should never get here */
        return 0;
    } else if (port < 30) {
        return port - 25 + CDK_XGS_MIIM_BUS_2;
    } else if (port < 38) {
        return port - 29;
    } else if (port < 42) {
        return port - 21;
    } else if (port == 42) {
        return 5 + CDK_XGS_MIIM_BUS_2;
    } else if (port == 50) {
        return 6 + CDK_XGS_MIIM_BUS_2;
    }
    /* Should never get here */
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

phy_bus_t phy_bus_bcm956636k25s_miim_ext = {
    "bcm956636k25s_miim_ext",
    _phy_addr,
    _read,
    _write
};

#else

/* ISO C forbids empty source files */
int bcm956636k25s_miim_int_not_empty;

#endif /* CDK_CONFIG_ARCH_XGS_INSTALLED */
