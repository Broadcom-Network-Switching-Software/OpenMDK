/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Default XGS internal PHY access functions.
 */

#include <cdk_config.h>

#if defined(CDK_CONFIG_INCLUDE_BCM53540_A0) && CDK_CONFIG_INCLUDE_BCM53540_A0 == 1

#include <cdk/arch/xgsd_chip.h>
#include <cdk/arch/xgsd_miim.h>
#include <cdk/chip/bcm53540_a0_defs.h>
#include <../PKG/chip/bcm53540_a0/bcm53540_a0_internal.h>

#include <phy/phy.h>

static uint32_t
_phy_addr(int port)
{
    if (port >= 2 && port < 14) {
        return (port - 1) + CDK_XGSD_MIIM_IBUS(0);
    }
    if (port >= 18 && port < 26) {
        return (port - 5) + CDK_XGSD_MIIM_IBUS(0);
    }
    if (port >= 26 && port < 30) {
        if (IS_GPHY(0, port)) {
            return (port - 5) + CDK_XGSD_MIIM_IBUS(0);
        } else {
            return (26 - 6) + CDK_XGSD_MIIM_IBUS(1);
        }
    }
    if (port >= 34 && port < 38) {
        return (34 - 18) + CDK_XGSD_MIIM_IBUS(1);
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

static uint32_t
_bus_cap(uint32_t addr)
{
    return CDK_E_NONE;
}

phy_bus_t phy_bus_bcm53540_miim_int = {
    "bcm53540_miim_int",
    _phy_addr,
    _read,
    _write,
    _phy_inst,
    _bus_cap
};

#else

/* ISO C forbids empty source files */
int bcm53540_miim_int_not_empty;

#endif /* CDK_CONFIG_INCLUDE_BCM53540_A0 */
