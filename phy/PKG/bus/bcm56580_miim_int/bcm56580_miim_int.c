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

/*
 * For BCM56580 due to the hardware port remapping, we need to adjust  
 * the PHY addresses. The PHY addresses are remapped as follows:
 *
 * Original : 0 ... 13 14 15 16 17 18 19 
 * Remapped : 0 ... 13 16 17 18 19 14 15
 */

static uint32_t
_phy_addr(int port)
{
    int phy_addr = port + CDK_XGS_MIIM_INTERNAL + CDK_XGS_MIIM_BUS_1;

    if (port == 14 || port == 15) {
        phy_addr += 4;
    } else if (port > 15) {
        phy_addr -= 2;
    }
    return phy_addr;
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

phy_bus_t phy_bus_bcm56580_miim_int = {
    "bcm56580_miim_int",
    _phy_addr,
    _read,
    _write
};

#else

/* ISO C forbids empty source files */
int bcm56580_miim_int_not_empty;

#endif /* CDK_CONFIG_ARCH_XGS_INSTALLED */
