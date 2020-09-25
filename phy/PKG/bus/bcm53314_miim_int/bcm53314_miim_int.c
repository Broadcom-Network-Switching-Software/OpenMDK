/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Default driver for BCM53314 external PHYs.
 *
 * This driver is compatible with the BCM56219K50T SDK board.
 */

#include <cdk_config.h>

#ifdef CDK_CONFIG_ARCH_XGS_INSTALLED

#include <cdk/cdk_device.h>
#include <cdk/arch/xgs_miim.h>

#include <phy/phy.h>

static uint32_t
_phy_addr(int port)
{
    if (port > 8) {
        port = ((port - 1) & ~0x7) + 1;
    }
    return port + CDK_XGS_MIIM_INTERNAL;
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

static int
_phy_inst(int port)
{
    return port - 1;
}

phy_bus_t phy_bus_bcm53314_miim_int = {
    "bcm53314_miim_int",
    _phy_addr,
    _read,
    _write,
    _phy_inst
};

#else

/* ISO C forbids empty source files */
int bcm53314_miim_int_not_empty;

#endif /* CDK_CONFIG_ARCH_XGS_INSTALLED */
