/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Default XGS internal XGXS PHY access functions.
 */

#include <cdk_config.h>

#ifdef CDK_CONFIG_ARCH_ROBO_INSTALLED

#include <cdk/cdk_device.h>
#include <cdk/arch/robo_miim.h>
#include <cdk/cdk_debug.h>
#include <phy/phy.h>

static uint32_t
_phy_addr(int port)
{
    return port + 0x10;
}

static int 
_read(int unit, uint32_t addr, uint32_t reg, uint32_t *val)
{
    return cdk_robo_miim_read(unit, addr, reg, val);
}

static int 
_write(int unit, uint32_t addr, uint32_t reg, uint32_t val)
{
    return cdk_robo_miim_write(unit, addr, reg, val);
}

phy_bus_t phy_bus_bcm89500_miim_int = {
    "bcm89500_miim_int",
    _phy_addr,
    _read,
    _write
};

#else

/* ISO C forbids empty source files */
int bcm89500_miim_int_not_empty;

#endif /* CDK_CONFIG_ARCH_XGS_INSTALLED */
