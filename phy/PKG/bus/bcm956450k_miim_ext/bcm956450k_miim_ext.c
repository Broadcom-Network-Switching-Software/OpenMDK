/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Default XGS internal XGXS PHY access functions.
 */

#include <cdk_config.h>

#ifdef CDK_CONFIG_ARCH_XGSM_INSTALLED

#include <cdk/cdk_device.h>
#include <cdk/arch/xgsm_miim.h>

#include <phy/phy.h>

static uint32_t
_phy_addr(int port)
{
    if (port >= 38) {
        port = (port - 38) + 0x6;
        return port + CDK_XGSM_MIIM_EBUS(1);
    } 
    if (port >= 35) {
        port = (port - 35) + 0x2;
        return port + CDK_XGSM_MIIM_EBUS(1);
    } 
    if (port >= 32) {
        port = (port - 32) + 0xA;
        return port + CDK_XGSM_MIIM_EBUS(1);
    }        
    if (port >= 29) {
        port = (port - 29) + 0xE;
        return port + CDK_XGSM_MIIM_EBUS(1);
    }
    if (port >= 25) {
        port = (port - 25) * 4 + 1;
        return port + CDK_XGSM_MIIM_EBUS(1);
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

phy_bus_t phy_bus_bcm956450k_miim_ext = {
    "bcm956450k_miim_ext",
    _phy_addr,
    _read,
    _write
};

#else

/* ISO C forbids empty source files */
int bcm956450k_miim_int_not_empty;

#endif /* CDK_CONFIG_ARCH_XGS_INSTALLED */
