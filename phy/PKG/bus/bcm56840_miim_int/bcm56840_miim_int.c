/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Default internal XGXS PHY access functions.
 */

#include <cdk_config.h>

#ifdef CDK_CONFIG_ARCH_XGS_INSTALLED

#include <cdk/cdk_device.h>
#include <cdk/arch/xgs_miim.h>

#include <phy/phy.h>

static uint32_t
_phy_addr(int port)
{
    port = ((port - 1) & ~0x3) + 1;
    if (port > 48) {
        return (port - 48) + CDK_XGS_MIIM_IBUS(2);
    }
    if (port > 24) {
        return (port - 24) + CDK_XGS_MIIM_IBUS(1);
    }
    return port + CDK_XGS_MIIM_IBUS(0);
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

phy_bus_t phy_bus_bcm56840_miim_int = {
    "bcm56840_miim_int",
    _phy_addr,
    _read,
    _write,
    _phy_inst
};

#else

/* ISO C forbids empty source files */
int bcm56840_miim_int_not_empty;

#endif /* CDK_CONFIG_ARCH_XGS_INSTALLED */
