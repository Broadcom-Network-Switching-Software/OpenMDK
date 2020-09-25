/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Default XGS internal PHY access functions.
 */

#include <cdk_config.h>

#ifdef CDK_CONFIG_ARCH_XGSM_INSTALLED

#include <cdk/cdk_device.h>
#include <cdk/arch/xgsm_miim.h>

#include <phy/phy.h>

static uint32_t
_phy_addr(int port)
{
    if (port >= 2 && port < 18) {
        return (port - 1) + ((port - 2) >> 2) + CDK_XGSM_MIIM_IBUS(0);
    } else if (port < 26) {
        return ((port - 0x2) & ~0x7) +  0x01 + CDK_XGSM_MIIM_IBUS(1);
    } else if (port < 34) {
        return 0x01 + (port - 26) + CDK_XGSM_MIIM_IBUS(1);
    }
    /* Should not get here */
    return 0;
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
    if (port < 10) {
        return 9 - port;
    } else {
        return port - 2;
    }
}

phy_bus_t phy_bus_bcm56150_miim_int = {
    "bcm56150_miim_int",
    _phy_addr,
    _read,
    _write,
    _phy_inst
};

#else

/* ISO C forbids empty source files */
int bcm56150_miim_int_not_empty;

#endif /* CDK_CONFIG_ARCH_XGSM_INSTALLED */
