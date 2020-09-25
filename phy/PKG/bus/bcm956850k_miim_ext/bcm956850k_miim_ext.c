/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Default internal XGXS PHY access functions.
 */

#include <cdk_config.h>

#ifdef CDK_CONFIG_ARCH_XGSM_INSTALLED

#include <cdk/cdk_device.h>
#include <cdk/arch/xgsm_miim.h>

#include <phy/phy.h>

static uint32_t
_phy_addr(int port)
{
    if (port > 108) {
        return (port - 108) + CDK_XGSM_MIIM_EBUS(5);
    }
    if (port > 84) {
        return (port - 84) + CDK_XGSM_MIIM_EBUS(4);
    }
    if (port > 64) {
        return (port - 64) + CDK_XGSM_MIIM_EBUS(3);
    }
    if (port > 44) {
        return (port - 44) + CDK_XGSM_MIIM_EBUS(2);
    }
    if (port > 20) {
        return (port - 20) + CDK_XGSM_MIIM_EBUS(1);
    }
    return port + CDK_XGSM_MIIM_EBUS(0);
}

static int 
_read(int unit, uint32_t addr, uint32_t reg, uint32_t *val)
{
    return cdk_xgsm_miim_read(unit, addr, reg, val);
}

static int 
_write(int unit, uint32_t addr, uint32_t reg, uint32_t val)
{
    return cdk_xgsm_miim_write(unit, addr, reg, val);
}

static int
_phy_inst(int port)
{
    return (port - 1) & 3;
}

phy_bus_t phy_bus_bcm956850k_miim_ext = {
    "bcm956850k_miim_ext",
    _phy_addr,
    _read,
    _write,
    _phy_inst
};

#else

/* ISO C forbids empty source files */
int bcm956850k_miim_ext_not_empty;

#endif /* CDK_CONFIG_ARCH_XGSM_INSTALLED */
