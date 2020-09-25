/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Default XGS internal PHY access functions.
 */

#include <cdk_config.h>

#ifdef CDK_CONFIG_ARCH_XGS_INSTALLED

#include <cdk/cdk_device.h>
#include <cdk/arch/xgs_miim.h>

#include <phy/phy.h>

static uint32_t
_phy_addr(int port)
{
    if (port < 26) {
        return ((port - 2) & ~0x7) + CDK_XGS_MIIM_INTERNAL + 1;
    } else if (port < 28) {
        return 25 + CDK_XGS_MIIM_INTERNAL;
    } else if (port < 30) {
        return 26 + CDK_XGS_MIIM_INTERNAL;
    }
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

static int
_phy_inst(int port)
{
    if (port >= 26 && port < 30) {
        /* DXGXS ports, lane number either 0 or 2 */
        return (port & 1) ? 2 : 0;
    }
    /* QSGMII ports */
    return port - 2;
}

phy_bus_t phy_bus_bcm56142_miim_int = {
    "bcm56142_miim_int",
    _phy_addr,
    _read,
    _write,
    _phy_inst
};

#else

/* ISO C forbids empty source files */
int bcm56142_miim_int_not_empty;

#endif /* CDK_CONFIG_ARCH_XGS_INSTALLED */
