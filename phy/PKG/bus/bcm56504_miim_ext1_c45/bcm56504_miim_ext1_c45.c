/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * BCM56504 PHY access functions for external bus 1 using clause 45.
 */

#include <cdk_config.h>

#ifdef CDK_CONFIG_ARCH_XGS_INSTALLED

#include <cdk/cdk_device.h>
#include <cdk/arch/xgs_miim.h>

#include <phy/phy.h>

static uint32_t
_phy_addr(int port)
{
    return port - 24 + 1 + CDK_XGS_MIIM_BUS_1;
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

phy_bus_t phy_bus_bcm56504_miim_ext1_c45 = {
    "bcm56504_miim_ext1_c45",
    _phy_addr,
    _read,
    _write
};

#else

/* ISO C forbids empty source files */
int bcm56504_miim_ext1_c45_not_empty;

#endif /* CDK_CONFIG_ARCH_XGS_INSTALLED */
