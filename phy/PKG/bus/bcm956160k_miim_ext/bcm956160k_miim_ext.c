/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Default XGS internal XGXS PHY access functions.
 */

#include <cdk_config.h>

#ifdef CDK_CONFIG_ARCH_XGSD_INSTALLED

#include <cdk/cdk_device.h>
#include <cdk/arch/xgsd_miim.h>

#include <phy/phy.h>

static uint32_t
_phy_addr(int port)
{
    if (port >= 2 && port < 10) {
        return (port - 1) + CDK_XGSD_MIIM_EBUS(0);
    } 
    if (port >= 18 && port < 26) {
        return (port - 8) + CDK_XGSD_MIIM_EBUS(0);
    }
    
    return 0;
}

static int 
_read(int unit, uint32_t addr, uint32_t reg, uint32_t *val)
{
    return cdk_xgsd_miim_read(unit, addr, reg, val);
}

static int 
_write(int unit, uint32_t addr, uint32_t reg, uint32_t val)
{
    return cdk_xgsd_miim_write(unit, addr, reg, val);
}

static int
_phy_inst(int port)
{
    return port - 2;
}

phy_bus_t phy_bus_bcm956160k_miim_ext = {
    "bcm956160k_miim_ext",
    _phy_addr,
    _read,
    _write,
    _phy_inst
};

#else

/* ISO C forbids empty source files */
int bcm956160k_miim_int_not_empty;

#endif /* CDK_CONFIG_ARCH_XGSM_INSTALLED */
